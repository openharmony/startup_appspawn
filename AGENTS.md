# AGENTS.md

OpenHarmony `appspawn` component (subsystem `startup`, part `appspawn`). The app-process spawner/service manager, started by `init` on boot. Not a standalone project — it builds inside the OpenHarmony top-level source tree; there is no Makefile/CMake/npm here.

## Architecture reference

`docs/` is a generated knowledge base (in Chinese): `docs/index.md` is the entry point with a module index + glossary (TLV, APL, DEC, SPM, HNP, prefork, etc.), `docs/architecture.md` covers data flow and cross-module wiring, and `docs/modules/` has one detail page per module. Read these first when ramping up — they explain the hook-stage pipeline (pre-fork → post-fork → child-execute) and how modules plug into it.

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
