# Tonatiuh++ M3 architectural exit — Phase 1: shared scripted interception

**State:** source-change candidate only; **not yet applied, compiled, or accepted** on the maintainer's native Windows checkout. This package does not declare the full M3 milestone closed. The project baseline is merged PR #35, `ab1dc1073d566bf39308567aa0eec082f1794994`, with successful post-merge Windows/Ubuntu/macOS CI [run #39694140397](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/39694140397). This package performs no GitHub operations.

## Scope of source changes

1. `ScriptInterceptionService` accepts a full `SimulationConfig`, with a compatibility overload retaining its original `(rays, masterSeed)` C++ API. It uses the existing `SimulationSession` and front-hit counter, not another propagation implementation; all existing default headless inputs remain 200×200 grid and seed 0.
2. `MainWindow::scriptInterceptionConfiguration` is a **GUI adapter**, supplying the actual GUI's selected fixed/automatic seed and current sun-grid dimensions on **each call**. The random generator, ray propagation, chunk scheduling, and photon/export formats remain untouched.
3. `ScriptSceneServices` validates arguments, obtains the effective per-host configuration, and invokes the **same** nonvisual service from GUI and true-headless scripts. `NodeObject.FindInterception` now goes through the same argument validation and interception service in each host (instead of a GUI-only callback and unsupported headless behavior). A host may not alter the requested ray count.
4. `GuiScriptSceneAdapter` forwards GUI `tn.FindInterception` directly to the shared `ScriptSceneServices` native method, instead of its legacy `MainWindow` numeric path. Two native `Q_INVOKABLE` overloads preserve the two-argument form and the historical optional `show` third argument. Headless requests to show rays fail explicitly before tracing.
5. Historical GUI `tn.FindInterception(surface, rays, true)` retains GUI ray rendering through a **separate presentation-only callback**. To avoid coupling the scientific return to `SceneTreeModel`, the numerical result is computed by the shared service first. The legacy `FluxAnalysis` pass is then used **only to produce the rendered rays**, using the *same resolved seed and grid*. This two-pass behavior has an added rendering cost, so it is an explicit remaining presentation/export dependency to review in M4; the returned power is never taken from FluxAnalysis. Do not claim export parity from this exception.
6. Adds focused native tests for a non-default 64×48 grid with exact repeatability, invalid host-grid rejection before rays are traced, headless `show=true` rejection with line attribution, and `NodeObject` argument validation. The already accepted M3.3 6-row receiver and 12-row tracker parity fixtures remain unchanged and must be repeated in native GUI after the patch.
7. Corrects the **factual integration status** in `PROJECT_STATUS.md`, but leaves *full M3* explicitly **OPEN** pending the remaining API and corpus gates.

## API compatibility classification — current evidence

| API family | Shared domain implementation after this phase? | Notes / remaining work |
|---|---|---|
| `tn.getScene`, `Clear`, `InsertScene`, `saveScene`, `loadScene` | Yes (with host-specific GUI document effects) | Preserve GUI selection/undo and relative paths; covered by earlier M3.3 acceptance. |
| `NodeObject` factory, parameter, tracker and `DataObject` file operations | Yes | GUI plugin-factory adapter and owned headless factories; existing bounded tests. |
| `tn.FindInterception(surface, rays)` and `NodeObject.FindInterception(...)` | **Source route unified in this candidate** | GUI dynamic seed/grid vs headless default seed 0/grid 200; rerun measured native exact parity. |
| `tn.FindInterception(surface, rays, true)` | Shared scientific power + GUI presentation adapter | Native GUI-only render; headless raises an explicit error. Old `FluxAnalysis` is retained **only** for drawing photons; a second trace is performed. |
| `tn.traceScene`, `startTrace`, `cancelTrace`, `finishTrace` | Yes for supported file-based, no-export calls | Uses `HeadlessScriptApi`/`SimulationSession` in both GUI/headless; no photon export support. |
| `tn.writeJson`, `validateScene`, `runBenchmark` | **Headless only** | Retained headless compatibility; GUI method exposure remains to be classified/implemented for full M3 exit. |
| `tn.ChangeSunPosition`, `SetValue`, `Run`, `RunFluxAnalysis`, ray/seed settings and exporter controls | **No** | Still available through GUI `MainWindow` fallback; no silently breaking removal. Full M3 exit **blocked** pending migration to nonvisual APIs or an explicitly approved compatibility policy. |
| `tn.Screenshot`, `render`, UI selection, dialogs | GUI presentation | Can stay GUI-only if explicit, documented, and predictably unsupported headlessly. |
| `ScriptRayTracer::Trace()` | Dormant C++ no-op; no JS-exposed invokable was found | Return `1` is misleading; audit other C++ callers and remove/fail explicitly only after compatibility review. |

## Original tracked-script corpus

Nineteen original `.tnhpps` files exist under `examples/`. All have syntax-only compilation tests. Two unchanged CPC examples have in-memory scientific execution and real headless CLI tests; the remainder have no full accepted GUI/headless parity sweep. The **eight** original interception-dependent scripts include 100,000–1,000,000 rays per call plus sweeps; do not run those full workloads under this package. Script-source inventory: `docs/m3-script-legacy-compatibility-step4.md`. Bounded existing M3.3 receiver/tracker/scene-I/O/recovery fixtures remain the parity acceptance set; future M3 closeout must extend representative corpus coverage with small, safe fixtures/sidecar isolation.

## Preflight and application (native Windows PowerShell)

Run only on the *actual checkout*, after confirming `git status --short --branch` is clean and `master` is the merged `ab1dc107` baseline. Create a feature branch before applying; **do not commit, push or open a PR until the native evidence is reviewed**.

```powershell
cd C:\OpenSource\tonatiuhpp
git status --short --branch
git switch -c refactor/m3-script-api-exit

$package = "$HOME\Downloads\Tonatiuhpp_M3_Exit_Interception_Phase1"
Expand-Archive "$HOME\Downloads\Tonatiuhpp_M3_Exit_Interception_Phase1.zip" -DestinationPath $package -Force
python "$package\apply_m3_exit_phase1.py" --repo 'C:\OpenSource\tonatiuhpp' --check
# Only after PREFLIGHT PASSED:
python "$package\apply_m3_exit_phase1.py" --repo 'C:\OpenSource\tonatiuhpp' --apply

git diff --check
git diff --stat
git status --short --branch
```

The script validates the exact merged base commit and every unique source anchor before it changes anything. It requires no staged changes and no existing modifications to target files. It backs up all edited source files under `build/tests/m3_exit_phase1_backup/<UTC timestamp>/`, leaves the result **unstaged**, and creates `docs/m3-exit-interception-phase1.md`. New docs are never overwritten.

## Native acceptance — only bounded tests

```powershell
cmake --build build --config Release --target tonatiuhpp tonatiuhpp_scientific_equivalence_tests --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'M3 exit phase1 Release build failed' }

ctest --test-dir build -C Release -R '^scientific[.]ScientificScriptInterception[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'M3 exit interception scientific tests failed' }

ctest --test-dir build -C Release -R '^headless[.](m3_script_parity_baseline|m3_script_tracker_parity|m3_script_cancel_recover|run_script_interception_bounded|run_script_cpc_curved|run_script_cpc_faceted)$' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'M3 script headless regression failed' }

ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Scientific regressions failed' }
```

**GUI acceptance:** Save any open GUI work. With no scene-tree selection, set fixed master seed `0` and `200×200` sun grid. Open `build/tests/m3_script_parity/script/m3_3_gui_headless_parity.tnhpps` in the rebuilt Release GUI and execute. Compare all six rows to `build/tests/m3_script_parity/headless_expected.csv`, including the expected `63857.145538085264 W` power (legacy accepted reference). Repeat the tracker fixture at `build/tests/m3_script_tracker_parity/script/m3_3_tracker_parity.tnhpps` and compare all twelve rows to its `headless_expected.csv`. Verify editor responsiveness and same-window recovery after an invalid input. Also test a bounded GUI `tn.FindInterception(surface, 64, true)` visual ray-display call on a valid receiver and ensure it returns a finite scientific value and renders rays; this is a separate GUI-only presentation check. Do **not** attempt the frozen 500-million-ray run, large facility sweeps, or long benchmarks.

After running both native GUI scripts, compare the normalized artifacts with the strict comparator bundled in this package:

```powershell
& "$package\compare_m3_exit_gui_headless.ps1" -RepositoryRoot 'C:\OpenSource\tonatiuhpp'
```

It requires a real GUI-created output and all 6/6 receiver plus 12/12 tracker rows to match exactly; missing or stale outputs cannot be accepted as successful GUI runs if the preceding CTests were rerun as instructed (those tests clear the GUI output before the comparison).

**If the GUI numerical parity diverges, do not tune physics or change frozen references to force the match.** Record both CSVs, GUI effective seed/grid settings, and source call locations; the package is not accepted until resolved.

## Outstanding formal M3 exit gates after this slice

- **Unrestricted `MainWindow` fallback is still present** for other legacy domain APIs. The patch intentionally does not remove or reclassify them before compatibility decisions and tests. This alone prevents declaring full M3 complete.
- GUI/headless `writeJson`, `validateScene`, `runBenchmark` and exporter/flux-control exposure have not been made symmetric.
- Tracked-corpus runtime/support matrix remains partial. No complete 19-script cross-host equivalence claim is justified.
- The obsolete `ScriptRayTracer::Trace()` no-op still needs C++ caller review.
- The new source requires native MSVC Release build, selected CTests, GUI parity and hosted three-platform CI after maintainer approval. This document itself is a candidate, not acceptance evidence.

Once these gates are resolved and the code passes native/hosted validation, a single consolidated M3 exit PR can be reviewed and merged. Only then should `PROJECT_STATUS.md` declare full M3 completion; M4 should follow that decision, not precede it.

Proposed eventual Conventional Commit subject: `refactor(script): close M3 GUI-independent domain API`
