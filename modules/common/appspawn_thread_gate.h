/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef APPSPAWN_THREAD_GATE_H
#define APPSPAWN_THREAD_GATE_H

#include <stdint.h>
#include <sys/types.h>
#include "appspawn_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file appspawn_thread_gate.h
 * @brief Thread-count gate for the spawn main process (cgroup pids dynamic gate).
 *
 * The spawn main process keeps itself inside /dev/pids/<spawner>_main with
 * pids.max=1 so that any thread creation is rejected by the kernel (EAGAIN).
 * Spawning requires a short "gate open" window (pids.max=GATE_MAX) because a
 * fork also charges one task into the cgroup. The child process notifies the
 * parent right after SetSelinuxCon succeeds (strict setcon recovery point,
 * B1.3); the parent then moves the child out to the transitional group and
 * writes pids.max back to the limit value ("migrate first, tighten second").
 */

/** Sentinel appId for entries that can never be matched by a frame (F3/F4/F6). */
#define GATE_APPID_NONE 0

/**
 * @brief Arm the thread gate in the main spawn process (one-shot, at AppSpawnRun).
 *
 * Sequence: self-migrate into <spawner>_main -> scan and drain leftover pids in
 * the group -> self-check pids.current -> write pids.max (write-then-read-back
 * verify). Partial failure rolls the process back to the transitional group
 * before degrading. On any failure the spawn service keeps running unarmed
 * (constraint falls back to the AGENTS.md single-thread rule) and the state is
 * observable via logs/hisyvent.
 *
 * @param content Spawn service content, used to derive the <spawner> group name.
 * @return 0 on success (armed); -1 when the gate is not armed (degraded, logged).
 */
int SpawnGateArm(AppSpawnMgr *content);

/**
 * @brief Open the gate synchronously right before a fork/clone in the main process.
 *
 * Sets the fork-sync-section flag; if the gate is closed (empty in-flight table
 * and no pending flag), writes pids.max=GATE_MAX. No pid is known at this point
 * (the child pid exists only after fork returns), hence the two-phase protocol
 * with SpawnGateRegisterPid. On open-write failure (D3-a) retries once, then
 * clears the sync flag and counts one gate failure; the caller must abort the
 * spawn (the following fork would fail with EAGAIN anyway).
 *
 * @return 0 when the gate is open (or already open); -1 when opening failed.
 */
int SpawnGateEnter(void);

/**
 * @brief Register a freshly forked/cloned child in the in-flight table (parent side).
 *
 * Called synchronously right after fork/clone returns >= 0. Records the parent
 * side pid plus the appId used for frame matching. The registration happens
 * before any frame can be processed (single loop thread ordering invariant).
 *
 * @param pid Child pid as seen by the parent (fork/clone return value).
 * @param appId Client id for frame pairing (GATE_APPID_NONE when the path has
 *              no client context and can never be frame-matched).
 */
void SpawnGateRegisterPid(pid_t pid, uint32_t appId);

/**
 * @brief Roll back synchronously when the fork in the current sync section failed.
 *
 * Clears the sync-section flag and closes the gate when the table is empty, so
 * a failed fork never leaves a gate.timeout-wide open window.
 */
void SpawnGateEnterFail(void);

/**
 * @brief Consume an in-flight entry: migrate the child out and close the gate if idle.
 *
 * Looks the pid up in the in-flight table (sole authoritative state source).
 * On hit: writes the child pid into <spawner>_spawned/cgroup.procs (an ESRCH
 * write means the child already died - entry dropped, kernel auto-uncharge),
 * removes the entry, and closes the gate when the table becomes empty (with
 * pids.current self-check before the write-back). Unmatched pids are a no-op,
 * which makes this the shared consumption path for frame notifications, the
 * synchronous F3/F4/F6 paths and the SIGCHLD death check.
 *
 * @param pid Child pid to consume (parent side view).
 */
void SpawnGateLeave(pid_t pid);

/**
 * @brief Child-side notification, called right after SetSelinuxCon succeeds.
 *
 * First line is a no-op guard when the gate is not armed (cold-run children,
 * unarmed service). The frame carries the client id (never getpid(): children
 * may live in a different pid namespace where getpid() never matches what the
 * parent registered) plus the child thread count snapshot. The write end is
 * non-blocking; a full pipe drops the frame (D2 timeout is the safety net).
 *
 * @param client Client context of the spawning request (frame appId source).
 */
void SpawnGateNotify(AppSpawnClient *client);

#ifdef __cplusplus
}
#endif
#endif /* APPSPAWN_THREAD_GATE_H */
