# Tonatiuh++ M3 architectural exit — Phase 3B1: shared scripted run-ray setting

**State:** guarded source candidate; Windows native acceptance PENDING. Full M3 exit **OPEN**. This is the first bounded part of Phase 3B, NOT a claim that `tn.Run`, `tn.RunFluxAnalysis` or exporter parity are complete.

**Prerequisite:** `refactor/m3-script-api-exit` at `HEAD ab1dc1073d566bf39308567aa0eec082f1794994`, with Phases 1/2/3A applied but **uncommitted**. Windows acceptance of Phase 3A was maintainer-reported: 67/67 scientific, 8/8 selected headless, 6/6 exact GUI/headless field/grid artifact rows, native GUI smoke, and GUI `SetValue` undo/redo 3/3. These are local, not hosted cross-platform CI.

## Why a bounded Phase 3B1

The GUI `MainWindow::Run()` **already uses** `SimulationSession` but surrounds it with exporter selection, `PhotonsBuffer` lifetime, plugin state, selected-surface policy, incremental normalization across runs, progress/cancel dialog, diagnostic signatures, and failed-export retention. The file exporter has a limited separate headless diagnostic path; it is *not* a drop-in replacement for the legacy scripted GUI Run contract.

`MainWindow::RunFluxAnalysis(...)` constructs `FluxAnalysis`, which uses `SceneTreeModel`, GUI progress machinery, surface-specific photon recording and a legacy TSV writer. The output depends on the selected surface, side, `u`/`v` bins, seed policy, world transform and power normalization. Changing it to plain `FindInterception` would discard this result. Neither path should be replaced until their actual output/lifecycle contracts are implemented nonvisually and tested against native GUI.

This package migrates the **next safe domain input** without touching those contracts. It is meaningful M3 progress, not the complete Phase 3B/3 or M3 exit.

## Implemented changes

1. `tn.SetRaysNumber(count)` now belongs to the same widget-free, per-host `ScriptSceneServices` used by GUI and true-headless scripts, instead of falling through to a `MainWindow` method. It requires a numeric **positive integral value no larger than `uint` maximum** (the existing legacy setter's parameter type). Bad negative, fractional, NaN, infinity, string, bool, zero, overflow or missing values throw at the script call site; no wrapped/truncated values reach the GUI.
2. The service stores `std::optional<uint> scriptRunRayCount()` **per script host** for future shared `Run` preparation. Unset means the user has not explicitly selected run rays. A failed GUI callback never commits new script state. `SetRaysNumber` does not trace or export photons and retains the original undefined JS return value on success.
3. The GUI scene adapter invokes existing `MainWindow::SetRaysNumber(uint)` **only as a setting synchronization callback**. GUI `Run()` continues to consume `m_raysNumber` under its unchanged exporter/lifecycle policy. The GUI script proxy explicitly forwards to `scene.SetRaysNumber`, no longer to the unrestricted legacy domain method.
4. True-headless `tn.SetRaysNumber` accepts the identical signature and error policy. The selected ray count is stored for the future portable run request; **it does not alter** explicit `tn.FindInterception(surface, rays)`, `tn.traceScene(options)` or `tn.startTrace(options)` counts. This intentionally prevents hidden changes to currently accepted tracing semantics.
5. Three new scientific tests exercise per-host isolation/no implicit tracing; invalid inputs, call-site attribution and unchanged state; callback failure atomicity and recovery. The existing two GUI facade tests continue, with the mock proving the stale `MainWindow::SetRaysNumber` is never called.
6. New bounded true-headless CTest copies one script into `build/tests/m3_exit_phase3b1_run_inputs/script/`, runs only **128 interception rays** (seed 0, 64x48 grid, no export), writes a **seven-row** CSV baseline and removes old GUI artifacts. Native GUI runs the **same copied script**; a separate PowerShell comparator checks exact row strings. It does **not** call `tn.Run`.

### API boundary after Phase 3B1

| Script API | GUI editor and legacy GUI-backed CLI | True-headless `run-script` | State |
| --- | --- | --- | --- |
| `SetValue`, `SetRaysGrid`, `SetRaysNumber`, `ChangeSunPosition`, `FindInterception` | Shared scene API with GUI adapters | Same shared scene API | **Migrated** |
| `traceScene`, `startTrace`, `cancelTrace`, `finishTrace` | Shared file-based no-export session API | Same | Existing scoped parity |
| `Run()` | Legacy GUI Run / exporter workflow | Unsupported with explicit error | **M3 blocker** |
| `RunFluxAnalysis(...)` | Legacy GUI flux/write path | Unsupported with explicit error | **M3 blocker** |
| `SetExportAllPhotonMap`, `SetExportCoordinates`, `SetExportIntersectionSurface`, `SetExportIntersectionSurfaceSide`, `SetExportPhotonMapType`, `SetExportPreviousNextPhotonID`, `SetExportTypeParameterValue`, `AddExportSurfaceURL`, `SetPhotonBufferSize`, `SetPhotonBufferAppend` | Legacy GUI export state | Unsupported with explicit error | **M3/M4 backend dependency** |
| `SetRaysScreen`, `Screenshot`, `render`, `Select`, GUI editing/dialogs | GUI-only presentation | Unsupported with explicit error | Presentation exception |
| `writeJson`, `validateScene`, `runBenchmark` | Not universally exposed from GUI | Headless-only tools | Documented asymmetry; remaining review |

**Compatibility caveat:** accepted historical *valid numeric* `SetRaysNumber` scripts continue to set the same GUI field. Previously silent invalid coercions such as `'128'`, `0`, or `-1` are now rejected early. This is a deliberate explicit validation policy; full legacy script execution beyond the accepted CPC examples is still outstanding.

## Guarded install

Use PowerShell in `C:\OpenSource\tonatiuhpp`:

```powershell
$phase3b1 = "$HOME\Downloads\Tonatiuhpp_M3_Exit_Run_Inputs_Phase3B1"
Expand-Archive "$HOME\Downloads\Tonatiuhpp_M3_Exit_Run_Inputs_Phase3B1.zip" -DestinationPath $phase3b1 -Force
python "$phase3b1\apply_m3_exit_phase3b1.py" --repo 'C:\OpenSource\tonatiuhpp' --check
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B1 preflight blocked' }
# Only if PREFLIGHT PASSED:
python "$phase3b1\apply_m3_exit_phase3b1.py" --repo 'C:\OpenSource\tonatiuhpp' --apply
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B1 application failed' }
git diff --check
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B1 diff check failed' }
git status --short --branch
```

The installer reconstructs exact prior patches from merged HEAD, verifies **all prior source/build/scripts/documents**, accepts **only** the two known Phase 3A documentation editions, and preserves any independent `PROJECT_STATUS.md` edits. It checks that none of Phase 3B1 is partially applied, is **idempotent** if exactly installed, and backs up changed files under `build/tests/m3_exit_phase3b1_backup/<UTC timestamp>`. Never use `--force`, reset your source, or rerun older installers to "fix" a mismatch.

## Native Windows acceptance

Run from the same PowerShell session with correct Qt/third-party DLL `PATH` (see prior accepted setup). Build first:

```powershell
cmake --build build --config Release --target tonatiuhpp tonatiuhpp_scientific_equivalence_tests --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B1 Release build failed' }
ctest --test-dir build -C Release -R '^scientific[.](ScientificScriptRunInputs|ScientificGuiScriptFacade)[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B1 focused tests failed' }
# Expect 5/5: three new run-input tests and two existing GUI facade tests.
ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B1 scientific suite failed' }
# Expect 70/70 (67 accepted + 3).
ctest --test-dir build -C Release -R '^headless[.](m3_exit_phase3b1_run_inputs|m3_exit_phase3a_field_grid|m3_exit_phase2_sun_position|m3_script_parity_baseline|m3_script_tracker_parity|run_script_cpc_curved|run_script_cpc_faceted|run_script_interception_bounded|m3_script_cancel_recover)$' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B1 selected headless suite failed' }
# Expect 9/9. Run all headless baseline tests BEFORE the GUI parity script.
```

Then:

1. Save current GUI work, launch Release GUI, select fixed seed **0**, and run this exact copied build-tree fixture: `C:\OpenSource\tonatiuhpp\build\tests\m3_exit_phase3b1_run_inputs\script\m3_exit_phase3b1_run_inputs_smoke.tnhpps`. It clears the active scene. It configures 128 Run rays and 64x48 sun grid, then explicitly performs only 128 interception rays. Confirm `M3 Phase 3B1 run-input smoke passed: <power>` and a responsive GUI.
2. Without rerunning headless CTest, compare the seven-row artifact:
   ```powershell
   & "$phase3b1\compare_m3_exit_phase3b1.ps1" -RepositoryRoot 'C:\OpenSource\tonatiuhpp'
   if ($LASTEXITCODE -ne 0) { throw 'Phase 3B1 native parity failed' }
   ```
   Required: `EXACT MATCH: Phase 3B1 ray-input scientific artifact (7/7 rows)`.
3. In the same Script Editor, check `tn.SetRaysNumber(0)` yields a controlled error attributed to the user-script line, followed by `tn.SetRaysNumber(128)` in a subsequent execution successfully completing. This must **not** launch a trace or exporter. The scientific tests cover the same contract headlessly.

**Do not run the frozen 500-million-ray benchmark, unbounded facility scripts, legacy GUI `Run` or photon export as part of this Phase 3B1 gate.** No optics, RNG, chunk scheduling, photon format, exporter implementations or accepted frozen reference files are changed.

## Remaining Phase 3B / full M3 exit work (not waived)

- Specify and implement a shared in-memory `SimulationRequest`/`Run` path including photon exporter options and lifetime, previous/next ID and file output semantics, selected surfaces, append/recovery and cancellation, while preserving old `Run()` behavior. Migration must be jointly reviewed with the M4 exporter backend boundary, not replaced by a no-export shortcut.
- Remove FluxAnalysis's `SceneTreeModel` dependency and GUI progress/dialog dependency from the domain flux calculation; preserve the existing historical TSV data layout, binning, scaling, front/back selection, repeated runs and numeric inputs, and test actual artifacts before replacing the GUI call.
- Decide the portable headless contract and explicit errors for remaining GUI-only/document-only commands. Expand bounded runtime coverage of original 19 sample scripts with sidecars/plugins and constrained ray budgets; hosted CI across Windows, Ubuntu, macOS after maintainer commit/PR approval.
- Consolidate the evidence in `PROJECT_STATUS.md` **without overwriting local maintainer content**. Do not mark full M3 closed or enter M4 yet.

## Proposed eventual consolidated commit (not performed)

`refactor(script): complete shared M3 scripting domain boundary`

Document all inherited Phase 1/2/3A validations, Phase 3B1's native evidence, remaining blockers and final acceptance after true domain parity exists. No commit, staging, GitHub write or benchmark run is included in this package.
