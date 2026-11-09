# Tonatiuh++ M3 exit — Phase 3B2A: shared scripted photon-export options

**Status:** guarded source candidate, native Windows acceptance **PENDING**. This is the **export configuration control-plane slice**, **not** completed Phase 3B2, not a new photon-export backend, and not M3 exit. No photon files or scientific traces are produced by this package's smoke test.

## Baseline and evidence

Branch `refactor/m3-script-api-exit`, pinned merged HEAD `ab1dc1073d566bf39308567aa0eec082f1794994`. The following **uncommitted** source candidate series is required: Phases 1, 2 and 3A; accepted Phase 3B1; and its `ScriptSceneServices.h` `SetRaysNumber(const QJSValue& count = QJSValue())` Qt arity correction. Maintainer-reported Phase 3B1 local acceptance: MSVC Release build, focused 5/5, scientific 70/70, selected headless 9/9, seven-row GUI/headless exact ray-input artifact, controlled GUI error and same-editor recovery. Earlier accepted Phase 3A: scientific 67/67, eight selected headless, six-row exact bounded parity and GUI undo/redo. No hosted CI is claimed for these uncommitted changes.

## Why a distinct export-options slice

The GUI's `Run()` still owns photon exporter plugin selection, `PhotonsBuffer` state, selected-surface filtering, cumulative ray-count normalization, GUI progress/cancellation, writer lifecycle and retention of unsaved photons after failed export. `MainWindow::ResetPhotonExporter()` explicitly refuses to destroy a failed buffer that holds unwritten photons. The limited diagnostic headless `File` exporter is not a drop-in implementation of all these semantics. Replacing legacy `Run()` with a no-export trace or silently creating a different headless photon format would corrupt the scientific/script compatibility contract.

This slice gives GUI and true-headless scripts a **single widget-free validation API and per-host record of explicit exporter-option changes**; the GUI adapter still synchronizes historical GUI-owned exporter settings, not photon data. It intentionally does not call `Run()`, start a worker or write a file.

## Changes

1. New `script/ScriptPhotonExportConfig.h` defines `ScriptPhotonExportChange` and `ScriptPhotonExportState` without `QWidget`, `MainWindow`, `SceneTreeModel`, photon exporters or tracing dependencies. The state contains only **explicit script-set** optional fields; it is not a snapshot of all current GUI defaults.
2. Shared `ScriptSceneServices` implements all ten former legacy script export setting calls under their existing names: `SetExportAllPhotonMap()`, `SetExportCoordinates(enabled,global)`, `SetExportIntersectionSurface(enabled)`, `SetExportIntersectionSurfaceSide(enabled)`, `SetExportPhotonMapType(name)`, `SetExportPreviousNextPhotonID(enabled)`, `SetExportTypeParameterValue(name,value)`, `AddExportSurfaceURL(url)`, `SetPhotonBufferSize(size)` and `SetPhotonBufferAppend(enabled)`.
3. Validation rejects missing/incorrectly typed flags, blank names/URLs, nonstring parameter values and zero/fractional/nonfinite/out-of-`uint` buffer sizes. It reports descriptive JavaScript errors at the actual caller line. This is a **validation compatibility tightening**: Qt's legacy QObject setters previously coerced some types. Valid typed signatures remain unchanged; each setter returns `undefined`.
4. Original historical no-op behavior is maintained when there is **no photon map settings object**: all setters except `SetExportPhotonMapType` and the two buffer settings do nothing. A GUI editor may already have settings selected via the dialog; the adapter tells the domain service if such a settings object is present. Existing GUI fields are **not silently reset** when a script host opens.
5. A successful GUI mutation is applied by `MainWindow::applyScriptPhotonExportChange` through existing setters. That adapter checks plugin name availability and postconditions. It **rejects reset-changing operations** while a failed exporter retains unwritten photons, preventing the script service from committing a mutation that the GUI has not accepted. The first scripted exporter selection initializes the new GUI `PhotonsSettings` to the same five flag defaults used by `MainWindow::Run()`, avoiding reads of uninitialized flags by subsequent setters.
6. The true-headless script host exposes all ten same setting names and retains host-local option requests, **but has no exporter plugin opened or export started**. Headless exporter-name availability and path validation are explicitly deferred to the subsequent shared run/export execution backend. Do **not** infer that `tn.Run` is supported headlessly from these setters.
7. GUI's explicit JS allowlist no longer delegates the ten setter names to the `MainWindow` QObject. `Run` and `RunFluxAnalysis` remain designated legacy blockers. Existing GUI-only presentation methods remain isolated.
8. Five scientific tests cover the initial no-op policy; independent host state and all ten settings; invalid arguments and source-line attribution; callback rejection atomicity and recovery; and buffer settings without exporter selection. The GUI proxy mock is extended to confirm that legacy exporter setters are never called from script. One bounded **configuration-only** headless CTest runs the unchanged script with no tracing, workers, photon files or destructive output.

## Boundaries that this package does **not** solve

- This is **not** photon-export parity. It does not compare actual photon data; it cannot yet satisfy the canonical M3 exit requirement for script-controlled `Run`. It does not migrate `Run()`, exporter creation, normalized power, buffer flushing, multi-run append/recovery, selected-surface *execution* filtering, or GUI cancellation/progress.
- GUI `PhotonsSettings` set by a preexisting dialog is not copied into the host's optional state: only subsequently scripted deltas are recorded. The adapter forwards those deltas to the actual GUI state. Full initial-state snapshot and shared request assembly remain Phase 3B2B prerequisites.
- Existing GUI `MainWindow::SetExport...` methods still serve UI callers. The script-specific domain decisions and argument validation now live in `ScriptSceneServices`, but the existing exporter state machine and all photon outputs remain GUI-owned.
- If an exporter name is accepted by the headless config-only host but unavailable as a plugin at execution time, **no file is produced**; eventual portable execution must reject it explicitly before changing filesystem output. GUI checks its loaded exporter factories immediately.
- The scripted `RunFluxAnalysis` still uses `FluxAnalysis`/`SceneTreeModel` and is reserved for Phase 3B3. The 19 original script examples retain syntax tests, with only the two original CPC examples having unmodified runtime acceptance. No facility sweeps or benchmark reruns are authorized.

## Guarded installation

Extract ZIP to `$HOME\Downloads\Tonatiuhpp_M3_Exit_Export_Options_Phase3B2A` (the ZIP contains files at root). From an unstaged Windows checkout:

```powershell
cd C:\OpenSource\tonatiuhpp
$phase3b2a = "$HOME\Downloads\Tonatiuhpp_M3_Exit_Export_Options_Phase3B2A"
Expand-Archive "$HOME\Downloads\Tonatiuhpp_M3_Exit_Export_Options_Phase3B2A.zip" -DestinationPath $phase3b2a -Force
python "$phase3b2a\apply_m3_exit_phase3b2a.py" --repo 'C:\OpenSource\tonatiuhpp' --check
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B2A preflight blocked' }
# ONLY after successful PRELIGHT and review:
python "$phase3b2a\apply_m3_exit_phase3b2a.py" --repo 'C:\OpenSource\tonatiuhpp' --apply
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B2A apply failed' }
git diff --check
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B2A diff whitespace check failed' }
git status --short --branch
```

The installer rebuilds exact prior accepted patches from the pinned Git commit, checks all prior source/build/tests/fixtures, and accepts only the known original/status-preserving Phase 3A documentation editions; `PROJECT_STATUS.md` is preserved even if locally edited. It does not tolerate unexpected source drift. Atomic writes and a versioned backup under `build/tests/m3_exit_phase3b2a_backup/` protect changes; partial installs are not automatically overwritten. It performs no Git staging, commit, push, PR, CI or benchmark operation.

## Windows Release validation — acceptance **PENDING**

Use the existing PowerShell environment with correct Qt/third-party DLL `PATH`. No frozen or large-ray benchmark:

```powershell
cmake --build build --config Release --target tonatiuhpp tonatiuhpp_scientific_equivalence_tests --parallel 4
if ($LASTEXITCODE -ne 0) { throw '3B2A Release build failed' }
ctest --test-dir build -C Release -R '^scientific[.](ScientificScriptExportConfig|ScientificGuiScriptFacade)[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw '3B2A focused tests failed' }
# Expected: 7/7 (five new + two existing).
ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw '3B2A scientific suite failed' }
# Expected: 75/75 (prior 70 + five).
ctest --test-dir build -C Release -R '^headless[.](m3_exit_phase3b2a_export_config|m3_exit_phase3b1_run_inputs|m3_exit_phase3a_field_grid|m3_exit_phase2_sun_position|m3_script_parity_baseline|m3_script_tracker_parity|run_script_cpc_curved|run_script_cpc_faceted|run_script_interception_bounded|m3_script_cancel_recover)$' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw '3B2A headless suite failed' }
# Expected: 10/10. These are no-export regression cases.
```

Then run the copied no-export **configuration-only** script in a fresh native GUI Script Editor session (save any existing work). Do not call GUI `Run` in this acceptance fixture:

`C:\OpenSource\tonatiuhpp\build\tests\m3_exit_phase3b2a_export_config\script\m3_exit_phase3b2a_export_config.tnhpps`

Expected: `M3 Phase 3B2A export-option configuration smoke passed.` and a responsive GUI. The script uses the installed `No export` plugin name, does not trace or write a photon file. Also run `tn.SetPhotonBufferSize(0)` separately to observe a controlled error, followed by `tn.SetPhotonBufferSize(4096)` to verify same-editor recovery. **No numerical/export parity is claimed or asked for in this phase.**

## Next engineering gates

**Phase 3B2B:** shared run request assembling effective rays/seed/grid, current GUI exporter state snapshot, plugin/name availability, selected surface resolution and retention-safe exporter lifecycle; preserve actual GUI `Run()` semantics. Measure bounded **real photon records and metadata** from GUI and true-headless with matched effective inputs, including selected surface, append and failed-export cases before replacing GUI delegation. The deferred factory/serializer backend separation must be scoped consistently with M4 and must not advance M4 by implication.

**Phase 3B3:** move flux calculation/TSV writing off `SceneTreeModel`/GUI dialogs; measure authentic bounded flux files.

**M3 exit:** no domain script API depends on `MainWindow` or `SceneTreeModel`, supported historical scripts execute the same domain operations through GUI/headless, and native/hosted scientific parity and error gates pass. Still OPEN.

## Eventual consolidated conventional commit (NOT PERFORMED)

`refactor(script): unify shared scripting and exporter configuration boundary`

Body: `Route script photon exporter option requests through a per-host nonvisual typed state and validation API. Retain the historical GUI settings adapter and protect failed-export buffers against script-triggered reset. Add bounded source-line, host-isolation, rejection/recovery and headless smoke tests. Leave Run, actual photon export and flux integration pending measured cross-host equivalence; preserve optics, seed policy, photon format, project status and frozen benchmark references.`
