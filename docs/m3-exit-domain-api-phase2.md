# Tonatiuh++ M3 architectural exit — Phase 2: explicit GUI API boundary and shared sun-position scripting

**State:** SOURCE CANDIDATE; requires maintainer's Windows Release build, CTest and native GUI verification, and later hosted Windows/macOS/Linux CI. This is **not the complete M3 exit**. Applies **only on top of the exact accepted Phase 1 working tree**, starting from `ab1dc1073d566bf39308567aa0eec082f1794994` on `refactor/m3-script-api-exit`. Does not commit, push or open a PR.

## Verified input evidence

The maintainer reported Phase 1: Windows Release `tonatiuhpp` and scientific test builds succeeded, seven focused interception tests passed, the six selected headless CTests passed, all 57 scientific CTests passed, exact native GUI/headless receiver CSV equality **6/6 rows** and tracker equality **12/12 rows**. A 64-ray GUI `show=true` script displayed rays and returned finite power; invalid ray counts produced line-1 JS errors; same-editor recovery succeeded. This is bounded, local evidence, not proof that all legacy scripts execute or that the M3 exit is met.

## Source changes in this candidate

1. Replace the unbounded `MainWindow` fallback in `GuiScriptSceneAdapter` with a **plain-JavaScript proxy target** that exposes only explicit shared services and an enumerated allowlist of historical commands. The source for this proxy lives in a widget-free `GuiScriptFacadeBridge.h` so it can be unit-tested under `ScriptHost` without constructing `MainWindow`. Both `tn` and `tonatiuh` share one facade. Script scratch properties remain JS-only; arbitrary property assignments cannot reach `MainWindow`.
2. Route **both historical `ChangeSunPosition` signatures** (azimuth/elevation and calendar/location) through the same widget-free `ScriptSceneServices` from GUI, legacy GUI-backed CLI and true-headless CLI. Match the existing `sunpos(cTime,cLocation,...)` coordinate and time conventions, and write the same `SunPosition` Coin fields used by the old GUI implementation. Invalid/missing/nonfinite arguments and unavailable scene/sun fail with a script exception rather than a GUI dialog or null dereference. No ray tracing is performed by this API.
3. Keep all known GUI-only document/view operations as an explicit presentation allowlist. Keep 15 as-yet-unmigrated legacy *domain* methods in a **named temporary allowlist**, including `SetValue`, GUI `Run`/`RunFluxAnalysis`, ray-grid/ray-count inputs and photon-export options. They retain their existing behavior only in the GUI-backed host; **they still depend on MainWindow** and block formal M3 closure. No exporter policy or flux output is modified in this package.
4. The old, non-invokable C++ `ScriptRayTracer::Trace()` had no trace implementation but returned success. Return an explicit failure (`0`) with a diagnostic instead. Retain the symbol for potential C++ compatibility. No direct C++ or script calls to this method were found in the repository search, and its `Trace` method is not exposed to QJSEngine; do not misrepresent this as replacing any active ray-tracing path.
5. Add native widget-free scientific tests for shared 2-arg/8-arg sun position, invalid-input atomicity, and for the new GUI facade allowlist/denial/JS scratch behavior. Register an automated cross-platform headless CTest for a small `.tnhpps` scene smoke that runs unmodified from GUI and true-headless without ray tracing or writing files.

## Capability and compatibility matrix (post-Phase 2 source candidate)

| Legacy name(s) | GUI Script Editor and GUI-backed `-i` | True-headless `run-script` | Classification / outstanding action |
|---|---|---|---|
| `tn.getScene`, `Clear`, `InsertScene`, `saveScene`, `loadScene` | Shared `ScriptSceneServices` + GUI document adapter | Shared `ScriptSceneServices` + owned document | **Shared domain**; GUI view/undo effects stay in adapter |
| `tn.FindInterception(surface,rays[,show])`, `NodeObject.FindInterception(...)` | Shared power via `ScriptInterceptionService`; optional GUI-only ray display | Shared power, `show=true` rejected before trace | **Shared numerical domain**, explicit GUI presentation exception, bounded parity accepted in Phase 1 |
| `tn.ChangeSunPosition(azimuth,elevation)` | New shared nonvisual `ScriptSceneServices` | Identical function | **Shared domain** in this candidate, with strict finite-number validation |
| `tn.ChangeSunPosition(year,month,day,hours,minutes,seconds,latitude,longitude)` | New shared nonvisual `sunpos` calculation + `SunPosition` update | Identical function | **Shared domain** in this candidate; follows historical UT and geographic coordinate order |
| `NodeObject` creation/field/tracker methods, `DataObject` CSV reads/writes | Shared `ScriptHost`/wrappers, GUI factory adapter | Shared `ScriptHost`/wrappers, headless factory registry | **Shared domain** for established methods |
| `tn.traceScene`, `startTrace`, `cancelTrace`, `finishTrace` | Shared `HeadlessScriptApi` and session; no-export request | Same API/session | **Shared domain** for currently supported file-based requests; exporter integration deferred |
| `tn.SetValue`, `Run`, `RunFluxAnalysis`, `SetRaysNumber`, `SetRaysGrid` | **Explicit temporary legacy MainWindow delegation** | Stable unsupported-capability result | **Unmigrated domain; M3 still BLOCKED** |
| `tn.SetExport*`, `AddExportSurfaceURL`, `SetPhotonBufferSize`, `SetPhotonBufferAppend` | **Explicit temporary legacy MainWindow delegation** | Stable unsupported-capability result | **Unmigrated domain/export contract; M3/M4 policy still unresolved** |
| `tn.Screenshot`, `render`, `Select`, `Copy`, `Cut`, `Paste`, `Delete`, `fileNew`, `fileOpen`, `fileSave`, `fileSaveAs`, `InsertNode`, `InsertShape`, `InsertTracker`, `InsertArray`, `InsertShapeSurface`, `CreateComponentNode`, `nodeImport`, `nodeExport`, GUI selection/editor helpers | **Explicit GUI-only presentation/document allowlist**, with existing GUI side effects | Stable unsupported-capability result | **Presentation-only or interactive editing**; document usage requires user save/undo awareness |
| `tn.writeJson`, `validateScene`, `runBenchmark` | Explicit unsupported-capability result | Existing `HeadlessScriptApi` implementations | **Host-specific capability disparity**, requires product classification before formal M3 closure |
| Arbitrary `MainWindow`/QObject methods (`windowTitle`, `metaObject`, `deleteLater`, etc.) | No reflection or call-through from `tn` proxy; unsupported API error on invocation | Unsupported API error on invocation | **Blocked**, not part of scripting contract; previously inadvertently exposed through unrestricted proxy |
| `ScriptRayTracer::Trace()` | C++ ABI symbol retained; explicit failure only | Not registered | **Dormant helper**, no JS invokable and no false success |

**Original 19-script corpus:** All 19 have syntax-only validation, and the two unmodified CPC examples have genuine headless execution. Others require representative bounded exercises and external input/sidecar classification. The existing example inventory contains no active `tn.ChangeSunPosition` usage; this candidate introduces an explicit smoke for both historical signatures rather than claiming new full-corpus coverage. Do not execute expensive facility/optimization sweeps or frozen benchmarks.

## Windows install (non-destructive guarded patch)

The installer checks that all ten Phase 1 files still match the exact output of the bundled Phase 1 reference transformer when applied to the committed `ab1dc107` base, that the Phase 1 documentation is unchanged, and that the two additional Phase 2 legacy files are pristine. It refuses staged changes, wrong branch/commit, existing new output files or conflicting local edits. It writes **only Phase 2 source additions** and backs up the eight modified files under `build/tests/m3_exit_phase2_backup/<UTC timestamp>/`.

```powershell
cd C:\OpenSource\tonatiuhpp
$phase2 = "$HOME\Downloads\Tonatiuhpp_M3_Exit_Domain_API_Phase2"
Expand-Archive "$HOME\Downloads\Tonatiuhpp_M3_Exit_Domain_API_Phase2.zip" -DestinationPath $phase2 -Force
python "$phase2\apply_m3_exit_phase2.py" --repo 'C:\OpenSource\tonatiuhpp' --check
# If and ONLY IF PREFLIGHT PASSED:
python "$phase2\apply_m3_exit_phase2.py" --repo 'C:\OpenSource\tonatiuhpp' --apply
git diff --check
git diff --stat
git status --short --branch
```

## Native acceptance

First build the two known targets, retaining the previously established DLL `PATH` configuration (MSVC loader error `0xc0000135` indicates a missing runtime DLL, **not** a scientific regression).

```powershell
cmake --build build --config Release --target tonatiuhpp tonatiuhpp_scientific_equivalence_tests --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'M3 Phase 2 Release build failed' }
ctest --test-dir build -C Release -R '^scientific[.](ScientificScriptSunPosition|ScientificGuiScriptFacade)[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'M3 Phase 2 focused scientific regressions failed' }
ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'M3 Phase 2 full scientific regressions failed' }
ctest --test-dir build -C Release -R '^headless[.](m3_exit_phase2_sun_position|m3_script_parity_baseline|m3_script_tracker_parity|run_script_cpc_curved|run_script_cpc_faceted|run_script_interception_bounded|m3_script_cancel_recover)$' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'M3 Phase 2 selected headless regressions failed' }
```

**Important ordering:** Headless parity CTests clear the GUI-output artifact. Run all headless CTests **before** rerunning native GUI parity scripts. With the Release GUI, re-run the existing fixed-seed 0 / 200×200 receiver and tracker fixtures and compare using the Phase 1 strict comparator. Repeat 64-ray `show=true` and invalid-then-recovery smoke only if observed behavior changed. Then run `tests/scripts/m3_exit_phase2_sun_smoke.tnhpps` in the GUI Script Editor **after saving current scene**; it must finish with `M3 Phase 2 shared sun-position smoke passed.` The **same unmodified script** is also run headlessly by CTest `headless.m3_exit_phase2_sun_position` (no export or benchmark). Do not overwrite or compare the receiver/tracker CSVs with this smoke, which does not create artifacts.

Host CI across all three platforms is still needed before a PR can merge. This package does not invoke the 500-million-ray benchmark, modify scientific reference datasets, change RNG/photon export or touch GUI exporter internals.

## Formal M3 exit blockers after Phase 2

- Temporary legacy domain method delegations still touch `MainWindow`/`SceneTreeModel` indirectly. These **must** be migrated, or an explicit compatibility policy approved and enforced, before claiming the domain API independent.
- GUI/headless utility-only capabilities and exporter/flux integration require agreed API classification.
- Representative non-destructive original-script runtime coverage remains partial (two of nineteen original examples execute unmodified in automated acceptance).
- The full branch still requires hosted three-platform CI and review. M4 work must not be declared started because Phase 2 is accepted.

Proposed eventual commit subject: `refactor(script): close M3 GUI-independent domain API`.
