# Tonatiuh++ M3 architectural exit -- Phase 3A: shared field edits and script sun grid

**Status: guarded source candidate, NOT YET COMPILED OR ACCEPTED in native Windows; M3 remains OPEN.**

Apply ONLY to the **uncommitted**, **locally accepted Phase 1 + Phase 2** working tree on `refactor/m3-script-api-exit`, at `HEAD ab1dc1073d566bf39308567aa0eec082f1794994`. The provided installer reconstructs the exact previous-phase edits from the merged Git baseline and checks the previous-phase new files before touching anything. No GitHub writes, commits, staging, benchmark reruns, science reference changes, or export changes are performed.

## Accepted prerequisite evidence (maintainer-reported)

- **Phase 1:** Windows Release application/scientific builds, 57/57 scientific and 6/6 headless CTests, 6/6 receiver and 12/12 tracker matched-input GUI/headless artifact rows, 64-ray GUI ray visualization, controlled invalid-count error and same-editor recovery.
- **Phase 2:** Windows Release application/scientific builds, 62/62 scientific and 7/7 selected headless CTests, repeat exact 6/6 receiver and 12/12 tracker rows, and GUI script `M3 Phase 2 shared sun-position smoke passed`.
- Both are **local acceptance**, not hosted cross-platform CI, not full M3 closeout.

## Implementation in this candidate

1. `tn.SetValue(nodeUrl, field, value)` is handled by the **same widget-free `ScriptSceneServices`** in true-headless and GUI-backed script hosts. The service resolves a layout `InstanceNode` by **exact complete URL** using the shared `SceneInstanceBuilder`, rejects zero/ambiguous URLs, and resolves `TSeparatorKit` edits to its transform (the historical GUI `SetValue` convention). It checks that the requested field exists before invoking any host edit callback. Headless performs a direct Coin field update and reports parse failure at the script call site. GUI invokes `MainWindow::setScriptFieldValue` **only as a QUndoStack/document-modified adapter**, preserving historical undo and document lifecycle, not as a URL/field-domain resolver. String fields use direct value assignment in headless; other fields use Coin's `SoField::set` parser.
2. `tn.SetRaysGrid(width,height)` is a common script signature and validation policy with **positive integer dimensions**. Each script host stores its accepted grid state. True-headless `FindInterception` now uses the stored 2D grid rather than a hard-coded 200x200. The GUI adapter synchronizes `MainWindow::SetRaysGrid` for subsequent GUI runs, while the existing GUI interception-configuration callback continues to supply the GUI's effective seed/grid on each call. Rejected grid changes leave the per-host grid state unchanged.
3. `GuiScriptFacadeBridge.h` forwards both domain operations only through the shared scene facade; neither operation remains in the temporary `MainWindow` method allowlist. The true-headless facade exports the same method names. GUI view/edit helpers remain GUI-specific; scripts still see controlled errors for unsupported true-headless operations.
4. Adds **five** native scientific tests: a real layout transform edit; invalid URL/field rejection with call-site location and unchanged geometry; host undo-adapter rejection with unchanged field; invalid/valid grid callback behavior; recovery after a failed grid callback. Updates the existing facade mock test to prove the stale legacy `MainWindow.SetValue` is NOT called.
5. Adds one bounded 128-ray (64x48 grid, seed 0) unchanged GUI/true-headless fixture and a CTest runner that copies the script into the build tree and stores a **six-row** headless scientific CSV. An independent PowerShell comparator checks exact row-by-row GUI/headless equivalence. The original 19 example scripts and frozen benchmark are not executed beyond the existing accepted CPC tests.

## Remaining M3 blockers (do NOT waive)

- GUI `tn.Run`, `tn.RunFluxAnalysis`, `tn.SetRaysNumber`, photon-export setters and buffer controls **still delegate to MainWindow**. `Run` and exporter contracts must not be replaced with no-export execution: that would silently change historical photon outputs, cancellation, undo or dialog behavior. Subsequent Phase 3 work needs separate domain-backed run configuration and a documented exporter/flux-policy boundary consistent with M4.
- `tn.SetValue` now handles layout-instance node URLs that the legacy GUI scene-tree model could address. It deliberately does not invent an API for world nodes absent from the legacy `SceneTreeModel`; missing/ambiguous URLs fail. Field parsing for GUI `CmdSetFieldText` retains its existing command semantics (unlike headless's explicit parser-error result); broader strict input equivalence still needs focused review.
- `tn.SetRaysGrid` changes **in-memory interception grid** in true-headless. It does NOT implicitly alter file-based `traceScene(options)` parameters; that API explicitly accepts its own trace configuration. `tn.SetRaysNumber` and script-controlled GUI `Run` remain separate and unmigrated.
- `tn.writeJson`, `validateScene`, `runBenchmark` are headless-only; presentation operations are GUI-only. Capabilities must be classified before declaring M3 complete.
- Of 19 original tracked scripts, all compile; only the two CPC examples have unmodified accepted runtime execution. Facility/optimization scripts may require sidecar files, complex plugins and unbounded traces. No blanket corpus acceptance is claimed.
- Hosted Windows/macOS/Linux CI and a consolidated architectural exit review are still required.

## Guarded Windows installation

```powershell
cd C:\OpenSource\tonatiuhpp
$phase3 = "$HOME\Downloads\Tonatiuhpp_M3_Exit_Field_Grid_Phase3A"
Expand-Archive "$HOME\Downloads\Tonatiuhpp_M3_Exit_Field_Grid_Phase3A.zip" -DestinationPath $phase3 -Force
python "$phase3\apply_m3_exit_phase3a.py" --repo 'C:\OpenSource\tonatiuhpp' --check
# ONLY if PREFLIGHT PASSED:
python "$phase3\apply_m3_exit_phase3a.py" --repo 'C:\OpenSource\tonatiuhpp' --apply
if ($LASTEXITCODE -ne 0) { throw 'Phase 3A application failed' }
git diff --check
if ($LASTEXITCODE -ne 0) { throw 'Whitespace check failed' }
git diff --stat
git status --short --branch
```

The installer checks the exact earlier Phase 1/2 patch results, refuses staged changes, refuses any existing new target path, backs up modified files under `build/tests/m3_exit_phase3a_backup/<UTC timestamp>`, and rolls back any partial write failure. It never touches GitHub, stages or commits changes.

## Windows native acceptance

Use the already established Qt/third-party runtime DLL `PATH` from the Phase 2 session. An executable error `0xc0000135` means Windows could not load a DLL, not a scientific failure.

```powershell
cmake --build build --config Release --target tonatiuhpp tonatiuhpp_scientific_equivalence_tests --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Phase 3A Windows Release build failed' }
ctest --test-dir build -C Release -R '^scientific[.](ScientificScriptFieldEdit|ScientificScriptGrid|ScientificGuiScriptFacade)[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Phase 3A focused scientific tests failed' }
# Expect 7/7 (5 added, 2 existing proxy tests).
ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Phase 3A scientific suite failed' }
# Expect 67/67 scientific tests (prior 62 + 5).
ctest --test-dir build -C Release -R '^headless[.](m3_exit_phase3a_field_grid|m3_exit_phase2_sun_position|m3_script_parity_baseline|m3_script_tracker_parity|run_script_cpc_curved|run_script_cpc_faceted|run_script_interception_bounded|m3_script_cancel_recover)$' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Phase 3A selected headless suite failed' }
# Expect 8/8; execute ALL headless tests BEFORE GUI artifacts.
```

**Native GUI acceptance (required; does not run automatically):**

1. **Save GUI work.** Start rebuilt Release GUI; select fixed seed **0**. The fixture clears the GUI document. Clear scene-tree selection if necessary to insert at the root.
2. Run the **identical copied headless fixture** in the GUI Script Editor:
   `C:\OpenSource\tonatiuhpp\build\tests\m3_exit_phase3a_field_grid\script\m3_exit_phase3a_field_grid_smoke.tnhpps`.
   It sets its **own 64x48** sun grid and traces exactly **128** rays, then writes `m3_exit_phase3a_result.csv` in the copied script directory. Confirm `M3 Phase 3A field/grid smoke passed: <power>` with no errors and responsive GUI.
3. Compare **after** running GUI, without rerunning `headless.m3_exit_phase3a_field_grid` in between:
   ```powershell
   & "$phase3\compare_m3_exit_phase3a.ps1" -RepositoryRoot 'C:\OpenSource\tonatiuhpp'
   if ($LASTEXITCODE -ne 0) { throw 'Phase 3A native GUI/headless mismatch' }
   ```
   Required: `EXACT MATCH: Phase 3A field/grid scientific artifact (6/6 rows)`.
4. GUI undo sanity: with a fresh GUI scene, use a tiny manual script that edits an **existing** layout object's transform with `tn.SetValue` (do not use `tn.Clear` between edit and undo); verify Undo restores the previous transform. This is a GUI-command lifecycle check, not an additional scientific trace.
5. Optional **unchanged** Phase 1/2 receiver/tracker parity can be repeated with fixed seed 0 and 200x200 grid as a broader regression. Phase 3A does not modify interception or tracker numerics, optics, RNG or exporter.

**Scientific safeguard:** No frozen 500-million-ray benchmark; no unbounded legacy facility scripts; no changes to optics/RNG, photon format, exporter state or accepted science reference files.

**Eventual consolidated commit (NOT NOW):**

`refactor(script): complete shared M3 scripting domain boundary`

Commit body should include the inherited Phase 1/2 acceptance, Phase 3A local evidence, GUI/headless artifact equivalence and M3 remaining blockers until final architectural closure. Do not mark full M3 complete or start M4 yet.
