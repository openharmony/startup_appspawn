/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "spawn_policy.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "appspawn_utils.h"
#include "cJSON.h"
#include "securec.h"

#define SPAWN_POLICY_PATH "/system/etc/appspawn/appspawn-spawn-policy.json"
// TBD-R1/TBD-R2 默认假设（见 §3.7），安全 owner 确认后改 JSON 即可，零代码
#define DEFAULT_SUDO_CAPS_MASK 0x3fffffffffULL
#define DEFAULT_SUDO_SELINUX_CTX "u:r:sudo:s0"

static SpawnPolicy g_policy[SPAWN_POLICY_MAX];
static int g_policyCount = 0;
static bool g_policyInited = false;

// 内置默认 app 策略：JSON 缺失/解析失败时回退，保证不裸奔
static void LoadDefaultAppPolicy(void)
{
    SpawnPolicy *p = &g_policy[0];
    (void)memset_s(p, sizeof(SpawnPolicy), 0, sizeof(SpawnPolicy));
    (void)strcpy_s(p->name, sizeof(p->name), "app");
    p->uidFrom = POLICY_SRC_TLV_DAC;
    p->gidFrom = POLICY_SRC_TLV_DAC;
    p->capsFrom = POLICY_SRC_TLV_CAPS;
    p->selinuxFrom = POLICY_SRC_TLV_SELINUX;
    (void)strcpy_s(p->sandboxCfg, sizeof(p->sandboxCfg), "appdata-sandbox.json");
    p->sandboxRootFrom = POLICY_SRC_TLV_BUNDLE;
    p->execFrom = POLICY_SRC_TLV_HAP_ENTRY;
    p->notifyAms = true;
    g_policyCount = 1;
}

// uid/gid 来源串 -> enum
static PolicySrc ParseUidGidSrc(const cJSON *json, const char *field, int64_t *fixedVal)
{
    const cJSON *src = cJSON_GetObjectItem(json, field);
    if (cJSON_IsNumber(src)) {
        *fixedVal = (int64_t)src->valuedouble;
        return POLICY_SRC_FIXED;
    }
    if (cJSON_IsString(src) && strcmp(src->valuestring, "tlv:dac") == 0) {
        return POLICY_SRC_TLV_DAC;
    }
    return POLICY_SRC_INVALID;
}

// caps 来源串 -> enum（"sudo" 或 "tlv:caps"）
static PolicySrc ParseCapsSrc(const cJSON *json, uint64_t *mask)
{
    const cJSON *caps = cJSON_GetObjectItem(json, "caps");
    if (cJSON_IsString(caps)) {
        if (strcmp(caps->valuestring, "sudo") == 0) {
            *mask = DEFAULT_SUDO_CAPS_MASK;
            return POLICY_SRC_SUDO;
        }
        if (strcmp(caps->valuestring, "tlv:caps") == 0) {
            return POLICY_SRC_TLV_CAPS;
        }
    }
    return POLICY_SRC_INVALID;
}

// selinux 来源串 -> enum（"sudo" 或 "tlv:selinux"）
static PolicySrc ParseSelinuxSrc(const cJSON *json, char *ctx, size_t ctxLen)
{
    const cJSON *se = cJSON_GetObjectItem(json, "selinux");
    if (cJSON_IsString(se)) {
        if (strcmp(se->valuestring, "sudo") == 0) {
            (void)strcpy_s(ctx, ctxLen, DEFAULT_SUDO_SELINUX_CTX);
            return POLICY_SRC_SELINUX_SUDO;
        }
        if (strcmp(se->valuestring, "tlv:selinux") == 0) {
            return POLICY_SRC_TLV_SELINUX;
        }
    }
    return POLICY_SRC_INVALID;
}

static PolicySrc ParseSrcStr(const cJSON *json, const char *field)
{
    const cJSON *v = cJSON_GetObjectItem(json, field);
    if (!cJSON_IsString(v)) {
        return POLICY_SRC_INVALID;
    }
    const char *s = v->valuestring;
    if (strcmp(s, "tlv:bundle") == 0) return POLICY_SRC_TLV_BUNDLE;
    if (strcmp(s, "ext:owner") == 0) return POLICY_SRC_EXT_OWNER;
    if (strcmp(s, "tlv:hap_entry") == 0) return POLICY_SRC_TLV_HAP_ENTRY;
    if (strcmp(s, "ext:daemon_bin_path") == 0) return POLICY_SRC_EXT_BIN_PATH;
    return POLICY_SRC_INVALID;
}

static void ParseOnePolicy(const cJSON *item, SpawnPolicy *p)
{
    (void)memset_s(p, sizeof(SpawnPolicy), 0, sizeof(SpawnPolicy));
    const cJSON *name = cJSON_GetObjectItem(item, "name");
    if (cJSON_IsString(name)) {
        (void)strcpy_s(p->name, sizeof(p->name), name->valuestring);
    }
    int64_t fixedUid = 0;
    int64_t fixedGid = 0;
    p->uidFrom = ParseUidGidSrc(item, "uidFrom", &fixedUid);
    if (p->uidFrom == POLICY_SRC_INVALID) {  // 无 uidFrom，看固定 uid
        const cJSON *uid = cJSON_GetObjectItem(item, "uid");
        if (cJSON_IsNumber(uid)) {
            p->uidFrom = POLICY_SRC_FIXED;
            fixedUid = (int64_t)uid->valuedouble;
        }
    }
    p->gidFrom = ParseUidGidSrc(item, "gidFrom", &fixedGid);
    if (p->gidFrom == POLICY_SRC_INVALID) {
        const cJSON *gid = cJSON_GetObjectItem(item, "gid");
        if (cJSON_IsNumber(gid)) {
            p->gidFrom = POLICY_SRC_FIXED;
            fixedGid = (int64_t)gid->valuedouble;
        }
    }
    p->uid = (uid_t)fixedUid;
    p->gid = (gid_t)fixedGid;
    p->capsFrom = ParseCapsSrc(item, &p->capsMask);
    p->selinuxFrom = ParseSelinuxSrc(item, p->selinuxCtx, sizeof(p->selinuxCtx));
    const cJSON *sandboxCfg = cJSON_GetObjectItem(item, "sandboxCfg");
    if (cJSON_IsString(sandboxCfg)) {
        (void)strcpy_s(p->sandboxCfg, sizeof(p->sandboxCfg), sandboxCfg->valuestring);
    }
    p->sandboxRootFrom = ParseSrcStr(item, "sandboxRootFrom");
    p->execFrom = ParseSrcStr(item, "execFrom");
    const cJSON *notify = cJSON_GetObjectItem(item, "notifyAms");
    p->notifyAms = cJSON_IsTrue(notify);
}

int SpawnPolicyInit(void)
{
    if (g_policyInited) {
        return 0;
    }
    g_policyInited = true;
    LoadDefaultAppPolicy();  // 先置默认，JSON 失败也有兜底

    FILE *fp = fopen(SPAWN_POLICY_PATH, "r");
    APPSPAWN_CHECK(fp != NULL, return -1,
        "SpawnPolicy: open %{public}s failed: %{public}d, use default app policy", SPAWN_POLICY_PATH, errno);
    char *content = NULL;
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    long fsize = ftell(fp);
    if (fsize <= 0) {
        fclose(fp);
        return -1;
    }
    (void)fseek(fp, 0, SEEK_SET);
    content = (char *)calloc(1, (size_t)fsize + 1);
    APPSPAWN_CHECK(content != NULL, fclose(fp); return -1, "SpawnPolicy: alloc failed");
    size_t rsize = fread(content, 1, (size_t)fsize, fp);
    fclose(fp);
    if (rsize == 0) {
        free(content);
        return -1;
    }

    cJSON *root = cJSON_Parse(content);
    free(content);
    APPSPAWN_CHECK(root != NULL, return -1, "SpawnPolicy: parse json failed");
    cJSON *arr = cJSON_GetObjectItem(root, "policies");
    if (cJSON_IsArray(arr)) {
        int n = cJSON_GetArraySize(arr);
        if (n > SPAWN_POLICY_MAX) {
            n = SPAWN_POLICY_MAX;
        }
        g_policyCount = 0;
        for (int i = 0; i < n; i++) {
            ParseOnePolicy(cJSON_GetArrayItem(arr, i), &g_policy[g_policyCount]);
            if (g_policy[g_policyCount].name[0] != '\0') {
                g_policyCount++;
            }
        }
    }
    cJSON_Delete(root);
    APPSPAWN_LOGI("SpawnPolicy: loaded %{public}d policies from %{public}s", g_policyCount, SPAWN_POLICY_PATH);
    return 0;
}

static const SpawnPolicy *FindPolicyByName(const char *name)
{
    for (int i = 0; i < g_policyCount; i++) {
        if (strcmp(g_policy[i].name, name) == 0) {
            return &g_policy[i];
        }
    }
    return NULL;
}

const SpawnPolicy *ResolvePolicy(const AppSpawningCtx *ctx)
{
    // P2 旁路态：返回 app 策略（值与原硬编码一致），不改变现有孵化行为。
    // P4：粗分类 msgType==MSG_SPAWN_DAEMON → daemon；细分类 有属主 ext → daemon-owned，无 → daemon-system。
    (void)ctx;
    const SpawnPolicy *app = FindPolicyByName("app");
    return app != NULL ? app : &g_policy[0];
}
