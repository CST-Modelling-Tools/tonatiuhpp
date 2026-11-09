# Tonatiuh++ M3 exit — Phase 3B2B1: bounded scripted Run with real File photon export

**Status: guarded source candidate. Native Windows acceptance PENDING.** Builds on the maintainer-attested, uncommitted and locally accepted M3 Phases 1, 2, 3A, 3B1 (+ Qt missing-argument correction), and 3B2A on `refactor/m3-script-api-exit`, HEAD `ab1dc1073d566bf39308567aa0eec082f1794994`. This slice is **not** completed Phase 3B2B, does **not** close M3, and does **not** constitute M4 backend separation.

## Contract and boundaries

- The script-visible `tn.Run()` now resolves through `ScriptSceneServices::Run` for BOTH GUI and true-headless hosts. No script `Run` dispatches directly to `MainWindow` from the JavaScript Proxy. The services assemble a per-invocation `ScriptRunRequest` from host-local ray count, sun grid and the explicit exporter-option state introduced by accepted 3B2A. Native errors still point to the calling script line; historical void/undefined return on success is retained.
- The GUI adapter uses `MainWindow::runScriptFromFacade`, which calls the **unchanged production GUI Run execution path** (existing `SimulationSession`, progress dialog, exporter plugin, `PhotonsBuffer`, cumulative normalization and retained-buffer guard). A new acceptance flag prevents GUI early returns, worker cancellation or failed export from being silently reported as success to JavaScript. GUI users still have their full historical capabilities, including append and dialog-defined options.
- The true-headless adapter executes a real **single bounded `SimulationSession`** using the same production File exporter plugin, same `PhotonsBuffer`, same photon record serializer and the existing headless trace preparation. No new photon file format, optics, RNG generator, worker partition or benchmark reference is introduced. Supported opt-in configurations are **File** or **No export**, explicit `tn.SetRaysNumber(1..200000)`, the per-host sun grid, and **non-append**. The native GUI comparison must set its fixed seed to **0**, which is the headless scripted Run fixed seed in this bounded slice. Repeated successful headless `tn.Run()` in the same script host is explicitly rejected pending cumulative/append parity.
- File mode requires an **already existing, absolute, empty output directory**, one file (`FileSize=-1`), a simple basename and valid file exporter parameters. Headless preflight performs a reversible write probe and refuses overwriting any existing content before exporter startup or tracing; `tn.Run` errors at the script call line. The same plugin has historically overwritten File output on a fresh GUI run; this deliberate fail-closed headless restriction prevents accidental user-data deletion while the unified exporter backend is incomplete. Existing GUI behavior is unchanged.
- Headless `HeadlessTracePreparationInput` now optionally resolves selected photon export surface URLs against its owned `SceneInstanceTree`. It rejects unknown or ambiguous URLs before the worker begins. This input is **empty by default**, preserving all previous headless CLI/benchmark/no-export semantics. The GUI continues to use its existing `SceneTreeModel` resolution until full extraction.
- The `File` exporter has **not been made GUI-free**: its own error paths still call `QMessageBox` and its full append/retained-buffer recovery is still GUI-coupled. Unexpected I/O failures after a trace has started remain an unresolved fault-handling boundary, not a validated headless recovery guarantee. This slice must not be described as completely shared exporter ownership or full source-level M3 exit.

## Source changes

11 guarded source/build changes:

- `source/application/script/ScriptSceneServices.h/.cpp` — shared `tn.Run()` request and error-dispatch API.
- `source/application/script/GuiScriptSceneAdapter.cpp` and `source/application/script/GuiScriptFacadeBridge.h` — explicit GUI callback and shared JS routing, remove legacy `Run` delegation.
- `source/application/main/MainWindow.h/.cpp` — GUI run adapter and a completion flag, without replacing the GUI exporter state machine.
- `source/application/headless/HeadlessScriptHost.cpp` — opt-in real one-shot Run callback and same JS method exposure.
- `source/application/core/TracePreparation.h/.cpp` — optional selected-surface URL resolution only when requested by a new headless scripted export.
- `tests/scientific/ScriptLegacyCorpusTests.cpp` and `tests/CMakeLists.txt` — four domain dispatch tests plus a bounded real File-export CTest.

5 new repo files:

- `source/application/script/ScriptRunRequest.h` — GUI-free typed run request.
- `source/application/headless/HeadlessScriptRunFileBackend.h` — bounded production File adapter (not a new serialization backend).
- `tests/scripts/m3_exit_phase3b2b1_run_file.tnhpps.in` — no in-place execution; CTest substitutes paths in the build tree.
- `tests/cmake/run_m3_exit_phase3b2b1_file.cmake` — two independent headless File traces, canonical photon record repeatability comparison, negative no-overwrite check, GUI fixture generation.
- `docs/m3-exit-phase3b2b1-bounded-run.md` — this record.

A separate `compare_m3_exit_phase3b2b1.ps1` utility in the delivery ZIP compares GUI vs headless real photon files using the repository's existing `tests/scientific/compare_photon_exports.py`. Canonical comparison checks exact physical-position bits, front/back flags, stable surface URL mapping and serialized per-photon power; it does NOT demand raw file ordering or global sequential photon IDs.

## Guarded local installation

The package contains the complete Phase 1/2/3A/3B1/3B2A reconstruction chain. It checks branch/HEAD and the unstaged index, verifies every accepted source/build/test/fixture (except the explicitly preserved local `PROJECT_STATUS.md` and the two accepted Phase 3A doc editions), and tolerates **only** the known Phase 3B2A scientific-test EOF blank-line correction, with no arbitrary source drift. The installer backs up overwritten files and rolls back a partial write; it does not stage, commit, push or change project status. **Never force installation past a failed preflight.**

```powershell
cd C:\OpenSource\tonatiuhpp
$pkg = "$HOME\Downloads\Tonatiuhpp_M3_Exit_Bounded_File_Run_Phase3B2B1"
Expand-Archive "$HOME\Downloads\Tonatiuhpp_M3_Exit_Bounded_File_Run_Phase3B2B1.zip" -DestinationPath $pkg -Force
python "$pkg\apply_m3_exit_phase3b2b1.py" --repo 'C:\OpenSource\tonatiuhpp' --check
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B2B1 preflight blocked' }
# Only after clean preflight:
python "$pkg\apply_m3_exit_phase3b2b1.py" --repo 'C:\OpenSource\tonatiuhpp' --apply
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B2B1 apply blocked' }
git diff --check
if ($LASTEXITCODE -ne 0) { throw 'Whitespace validation failed' }
```

## Native Windows acceptance — NOT YET RUN

Build in the existing PowerShell session with the accepted Qt/third-party runtime PATH. Do not rerun the frozen 500M benchmark.

```powershell
cmake --build build --config Release --target tonatiuhpp tonatiuhpp_scientific_equivalence_tests --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B2B1 Release build failed' }
ctest --test-dir build -C Release -R '^scientific[.](ScientificScriptRunDispatch|ScientificGuiScriptFacade)[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Focused scientific tests failed' }
# Expected: 6/6 (four new + two prior facade tests)
ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Scientific suite failed' }
# Expected: 79/79 (75 accepted + four)
ctest --test-dir build -C Release -R '^headless[.](m3_exit_phase3b2b1_run_file|m3_exit_phase3b2a_export_config|m3_exit_phase3b1_run_inputs|m3_exit_phase3a_field_grid|m3_exit_phase2_sun_position|m3_script_parity_baseline|m3_script_tracker_parity|run_script_cpc_curved|run_script_cpc_faceted|run_script_interception_bounded|m3_script_cancel_recover|photon_export_scientific_equivalence)$' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Headless File regression failed' }
# Expected: 12/12 if the A0 File-export equivalence CTest is configured.
```

The new CTest runs **2048 rays, grid 200×200, seed 0** on the existing `examples/benchmarks/cylinder.tnhpp`; uses the real production File plugin and selected `//Node/Shape` surface; verifies independent output record multisets, metadata and no-overwrite failure; generates the GUI fixture. It only deletes its own `build/tests/m3_exit_phase3b2b1_run_file/{headless_a,headless_b,gui}` temporary output directories and never touches a source-tree photon file. Check real headless result for existing output and scientific record failures before proceeding.

Launch the rebuilt **native GUI** in a fresh session, select **fixed master seed 0**, open this generated copied script in the GUI Script Editor, then execute:

`C:\OpenSource\tonatiuhpp\build\tests\m3_exit_phase3b2b1_run_file\script\m3_exit_phase3b2b1_gui.tnhpps`

The GUI should complete its normal `Run` progress/dialog and print `M3 Phase 3B2B1 bounded File Run passed.` The GUI File exporter writes two files under `build/tests/m3_exit_phase3b2b1_run_file/gui`. Then compare:

```powershell
& "$pkg\compare_m3_exit_phase3b2b1.ps1" -RepositoryRoot 'C:\OpenSource\tonatiuhpp'
if ($LASTEXITCODE -ne 0) { throw 'GUI/headless photon File parity failed' }
```

The required comparator result is `Photon export canonical records match (...)` followed by `M3 EXIT PHASE 3B2B1: NATIVE GUI/HEADLESS REAL FILE PARITY PASSED`. No data-order assertions. Do not rerun the headless suite after the GUI fixture or it will invalidate the GUI result.

Controlled failure/recovery evidence is provided by the four domain dispatch tests and the independent headless no-overwrite CTest. Do not deliberately trigger GUI or headless mid-write File-export failures until plugin error dialogs are removed and retained-buffer recovery is implemented.

## Unresolved gates and next work

1. Extract GUI `Run()`'s actual exporter and normalization lifecycle into a nonvisual service with consistent source-selected filtering, ownership, cancellation and append across hosts. This package only unifies **script API dispatch** and demonstrates bounded actual File output via existing components.
2. Replace plugin modal `QMessageBox` failure paths and establish retention-safe retry/repair for unwritten photons before claiming headless error resilience.
3. Validate repeated append/non-append scripted runs, power normalization, multiple File output segments, invalid/ambiguous surface cases and deterministic saved-scene reloads on real builds; only the one-shot single-file case is accepted here.
4. Migrate `RunFluxAnalysis()` without `SceneTreeModel` in Phase 3B3; evaluate all historical scripts at runtime (19 currently compile, only two CPC fixtures run unchanged).
5. M3 remains OPEN, M4 remains separate and pending, frozen 500M benchmark remains untouched.

## Eventual conventional commit (NOT PERFORMED)

`refactor(script): introduce bounded shared Run dispatch and File export parity`

Body: `Route GUI and headless tn.Run through ScriptSceneServices, retain the historical GUI lifecycle via a completion-aware adapter, and opt in true-headless one-shot File output through the existing SimulationSession and photon plugin. Add selected-surface URL resolution, non-overwriting headless output preflight, exact physical-record acceptance and controlled recovery tests. Defer append, exporter error UI extraction, flux and full M3 closeout; preserve optics, RNG, photon format, frozen references and uncommitted change discipline.`

## Corrected reconstruction preflight (2026-11-05)

The initially supplied Phase 3B2B1 source installer omitted two newly edited
files, `source/application/core/TracePreparation.h` and
`source/application/core/TracePreparation.cpp`, from its prior-phase source
reconstruction. Earlier phases did not touch these files, so they were
absent from the Phase 3B2A reconstruction dictionary. The original installer
failed closed with `Not all Phase 3B2B1 source anchors can be reconstructed`;
**no local source files were modified** by that failed `--check`.

The corrected installer reconstructs those two baselines from the pinned
`ab1dc1073d566bf39308567aa0eec082f1794994` Git commit and enforces
full exact-content comparison along with the original nine anchors. It does
not bypass source drift, relax any C++ tests or change the Phase 3B2B1
scientific implementation. All native validation remains pending.
