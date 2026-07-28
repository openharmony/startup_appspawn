# AGENTS.md

OpenHarmony `appspawn` component (subsystem `startup`, part `appspawn`). The app-process spawner/service manager, started by `init` on boot. Not a standalone project — it builds inside the OpenHarmony top-level source tree; there is no Makefile/CMake/npm here.

## Architecture reference

`docs/` is a generated knowledge base (in Chinese): `docs/index.md` is the entry point with a module index + glossary, `docs/architecture.md` covers data flow and cross-module wiring (pre-fork → post-fork → child-execute), and `docs/modules/` has one detail page per module.

## Knowledge routing

Before editing, state in your reply: the task category, the `docs/` page(s) you opened, and any constraint you found there. If no doc matches, say so explicitly — do not guess.

Task-based routing:

| Task | Read first |
|---|---|
| Sandbox / mount / namespace change | `docs/modules/module_sandbox.md` |
| UID/GID, capability, APL/DEC, tokenId | `docs/modules/module_spm.md` |
| Hook stage / module lifecycle / new module | `docs/architecture.md`, `docs/modules/module_modulemgr_engine.md` |
| IPC / spawn request / service / message handler | `docs/modules/module_standard.md`, `docs/modules/module_server_common.md`, `docs/modules/module_client_api.md` |
| TLV message encode/decode | `docs/modules/module_client_api.md`, `docs/modules/module_util.md` |
| Adapter (Ace / NWEB / native / asan) | `docs/modules/module_adapters.md`, `docs/modules/module_ace_adapter.md` |
| HNP (native package) | `docs/modules/module_hnp.md`, `docs/hnp_wiki.md` |
| cgroup / namespace / isolation | `docs/modules/module_common_modules.md` |
| Lite (small-system) path | `docs/modules/module_lite.md` |

Path-based routing:

| Path | Read first |
|---|---|
| `modules/sandbox/**`, `appdata-sandbox*.json` | `docs/modules/module_sandbox.md` |
| `modules/spm/**` | `docs/modules/module_spm.md` |
| `modules/module_engine/**` | `docs/modules/module_modulemgr_engine.md` |
| `standard/appspawn_service.c`, `appspawn_msgmgr.c`, `appspawn_appmgr.c` | `docs/modules/module_standard.md`, `docs/modules/module_server_common.md` |
| `modules/common/**` | `docs/modules/module_common_modules.md` |
| `util/**` | `docs/modules/module_util.md` |
| `lite/**` | `docs/modules/module_lite.md` |

Vocabulary routing (when these terms appear in an issue/log/API/file, load the doc before editing):

| Term | Meaning (authoritative, from `docs/index.md`) | Doc |
|---|---|---|
| TLV | Type-Length-Value — message encoding used by appspawn | `docs/modules/module_client_api.md` |
| APL | Ability Privilege Level (NORMAL / SYSTEM_BASIC / SYSTEM_CORE) | `docs/modules/module_spm.md` |
| DAC | Discretionary Access Control (UID/GID) | `docs/modules/module_spm.md` |
| DEC | Dynamic Enhance Control — tokenId+path strategy via `/dev/dec`; **not fscrypt** | `docs/modules/module_sandbox.md` |
| SPM | Security Process Manager — kernel subsystem, manages tokenid/uid refcounts | `docs/modules/module_spm.md` |
| HNP | Harmony Native Package | `docs/modules/module_hnp.md` |
| prefork | pre-created child-process mechanism (single reservation, **not a pool**) | `docs/modules/module_standard.md` |
| cold run | cold-start mode: execute directly in an already-forked child | `docs/architecture.md` |
| Hook Stage | pre-fork / post-fork / child-execute pipeline stages | `docs/architecture.md` |

## Where to look

| Task type | Open these first |
|---|---|
| Add a new `.so` module | an existing `modules/*/BUILD.gn`, `modules/module_engine/include/appspawn_hook.h`, `modules/module_engine/stub/libappspawn.stub.json` |
| Change sandbox behavior | `modules/sandbox/sandbox.gni`, `appdata-sandbox*.json` (repo root), `docs/modules/module_sandbox.md` |
| Add/change a hook stage or priority | `modules/module_engine/include/appspawn_hook.h` (`STAGE_*`, `HOOK_PRIO_*`) |
| Add an exported engine API | `modules/module_engine/stub/libappspawn.stub.json` (source of truth) |
| Add an error code | `util/include/appspawn_error.h` |
| Change build flags | `appspawn.gni` (`declare_args`), `bundle.json` |
| Add a new binary variant | `standard/BUILD.gn` (define table), `standard/appspawn_main.c` (`g_appSpawnStartArgTemplate`) |

## Build

- GN + Ninja via the OpenHarmony build framework (`import("//build/ohos.gni")`). Built from the OpenHarmony source root, e.g. `./build.sh --product-name <product> --build-target appspawn` (top label `//base/startup/appspawn:appspawn_all`).
- Component manifest: `bundle.json`. Feature toggles live in `appspawn.gni` (`declare_args`) — set them at the product-config level, not in-tree.
- `lite/` is the small-system path (used when `ohos_lite` is defined); `standard/` is the standard-system path. Separate code paths, not conditionals within the same files.

## Multiple executables from one source

`standard/appspawn_main.c` is the single `main()`. The same core `.c` files are compiled into 5 executables, distinguished only by a compile-time define (set in `standard/BUILD.gn`):

| binary | define | purpose |
|---|---|---|
| `appspawn` | (default) | JS/ArkTS apps |
| `nwebspawn` | `NWEB_SPAWN` | render/web processes |
| `nativespawn` | `NATIVE_SPAWN` | native processes |
| `cjappspawn` | `CJAPP_SPAWN` | CJ apps |
| `hybridspawn` | `HYBRID_SPAWN` | hybrid apps |

Runtime mode is also selectable via `-mode` argv (see `g_appSpawnStartArgTemplate` in `appspawn_main.c`). When editing shared code, keep it correct under all 5 define variants unless the file is variant-specific.

## Module system

Feature logic lives in dynamically loaded `.so` modules under `modules/` (e.g. `ace_adapter`, `asan`, `nweb_adapter`, `sandbox`, `common`, `spm`). A module self-registers at load time using:

- `MODULE_CONSTRUCTOR(void) { ... }` — a `__attribute__((constructor))` wrapper from `modules/module_engine/include/appspawn_hook.h`
- hook registration: `AddPreloadHook`, `AddServerStageHook`, `AddAppSpawnHook`, `AddProcessMgrHook`

Hook stages (`STAGE_*`) and priorities (`HOOK_PRIO_*`) are defined in `appspawn_hook.h`. The engine symbols a module may call are fixed by `modules/module_engine/stub/libappspawn.stub.json` — adding a new exported API requires updating that stub. New module = `ohos_shared_library` depending on `libappspawn_module_engine`, installed to `lib*/appspawn/<type>` (copy an existing `modules/*/BUILD.gn`).

## Sandbox

Two implementations gated by the `appspawn_sandbox_new` GN arg (see `modules/sandbox/sandbox.gni`): `normal/` (legacy, default) and `modern/` (new). Sandbox JSON configs at repo root (`appdata-sandbox*.json`) install to `/etc/sandbox`.

## Constraints and boundaries

appspawn is a security-critical boot service: it drops capabilities, assigns UID/GID, mounts per-app sandboxes, and enforces APL/DEC. Treat the following as escalation triggers, not normal edits.

Do not (change without explicit owner sign-off):

- Modify any `appdata-sandbox*.json` at repo root — these define the on-device sandbox layout installed to `/etc/sandbox`. A bad change breaks app isolation.
- Change UID/GID assignment, capability-set (`caps`) handling, DAC, or APL/DEC enforcement code (mostly under `modules/spm/` and the spawn-request path in `standard/`).
- Modify IPC transaction handlers in `standard/appspawn_service.c` / `appspawn_msgmgr.c` / `appspawn_appmgr.c` without a protocol-compatibility check — appspawn is started by `init` and talked to by AbilityManagerService; the wire format is cross-process and cross-version.
- Reorder, rename, or repurpose `STAGE_*` or `HOOK_PRIO_*` constants in `modules/module_engine/include/appspawn_hook.h` — module load order and hook priority are part of the ABI modules depend on.
- Edit the engine's exported surface without updating `modules/module_engine/stub/libappspawn.stub.json` — the stub is the source of truth for what modules may call.
- Change `appspawn.gni` `declare_args` defaults in-tree — product config owns them (see Build).
- Hand-edit anything under `docs/` — it is a generated knowledge base. Fix the generator or the source it was built from.

Ask before:

- Any change that affects app launch time, boot sequence, or the `-mode` argv contract.
- Any change to `lite/` that could regress small-system devices.
- Any new third-party dependency (license + size review).

Architecture invariants:

- `lite/` and `standard/` are separate code paths, not `#ifdef`s inside shared files.
- The 5 binaries share the same core `.c` files — shared code must stay correct under all 5 compile-time defines (see Multiple executables).
- Modules self-register via `MODULE_CONSTRUCTOR` and hook stages; they must not call engine symbols outside the stub JSON.

## Code conventions

- C + C++ mix. Use `securec.h` (bounds_checking_function) memory functions (`sprintf_s`, `memcpy_s`, …), not raw libc. Pervasive across the codebase.
- Logging: use `APPSPAWN_LOGI/LOGE/LOGV/LOGW/LOGF` and `APPSPAWN_CHECK(cond, recovery, fmt, ...)` from `util/include/appspawn_utils.h`. Format strings MUST use HiLog privacy markers like `%{public}s` / `%{public}d` — plain `%s` is flagged. Don't call `printf`/`HiLog` directly.
- Error codes are bit-packed via macros in `util/include/appspawn_error.h` (`DECLARE_APPSPAWN_ERRORCODE(module, submodule, error)`); reuse the scheme rather than inventing raw numbers.
- Apache-2.0 Huawei copyright header required on new files (copy from an existing file).

## Tests

Tests are **device tests**, not host-runnable from this checkout:

- Unit (`test/unittest/`, group `test:unittest`, main binary `AppSpawn_ut`) and module (`test/moduletest/`) tests are pushed to an OpenHarmony device/emulator and run via `hdc`. Coverage flow: `test/unittest/gencoverage.sh` (push `AppSpawn_ut`, run with `GCOV_PREFIX`, pull `.gcda`, `lcov` + `llvm-cov`).
- Unit tests rely on link-time symbol stubbing: `-Dprivate=public` plus `capset=CapsetStub`, `mount=MountStub`, etc. (see `test/unittest/app_spawn_standard_test/BUILD.gn`). Mocks live in `test/mock/`.
- Fuzz: `test/fuzztest/` (`ohos_fuzztest`), group `test:fuzztest`. Some standard-test targets are skipped when `use_libfuzzer` is set.
- Autotests: `test/autotest/` are Python (`devicetest` + `hypium` `UiDriver`) running on-device via `hdc shell`.

There is no in-repo lint/typecheck/format script; CI is driven by OpenHarmony top-level tooling. `OAT.xml` is license auditing only.

## Done

A task is not complete until the final reply contains all of:

- The exact build target(s) used, e.g. `./build.sh --product-name <product> --build-target appspawn` (top label `//base/startup/appspawn:appspawn_all`), with a build pass/fail status line.
- For code changes: which test group(s) apply (`test:unittest`, `test:moduletest`, `test:fuzztest`, `test:autotest`) and their pass/fail status. For a sandbox / permission / IPC change, name the specific moduletest or fuzztest target you considered — do not just say "tests pass".
- A short list of files touched, tagged with which "Constraints and boundaries" category they fall into (sandbox JSON / caps-UID-GID / IPC / hook-stage / stub API / build flag / other).
- For sandbox JSON, IPC, hook-stage, or stub-API changes: an explicit note that the change has security or compatibility implications and needs owner review.

If validation cannot be run (no device, or no OpenHarmony source root available):

- State explicitly that build/test could not be run locally.
- Do not claim success, and do not paste fabricated pass markers.
- List the exact commands the owner should run to validate, including product name and target label.
