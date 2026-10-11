# M1 — Effective simulation-preparation policies and first canonical pipeline

**Status:** implementation proposed on `refactor/m1-canonical-trace-preparation-20261012`; native build/scientific validation pending.
**Governing roadmap:** [GUI/Core Separation and Scientific Equivalence Plan](gui-core-separation-scientific-equivalence-plan.md), packages M1.1–M1.3.

## Effective inputs by invocation (source baseline: `master` at `abf54d4`)

| Entry point | Rays and sun grid | Effective seed | Scene/tracker state | Ownership, air, photons |
| --- | --- | --- | --- | --- |
| GUI `MainWindow::Run()` | GUI dialog/in-memory fields; starting rays 10,000 and sun grid 200x200 | Opt-in fixed seed or historical `QTime::currentTime().msec()` automatic seed | Loaded GUI document; `SceneTreeModel` provides borrowed instance tree, GUI scene lifecycle updates trackers | Borrowed layout/sun; GUI resolves non-vacuum air and selected exporter surfaces; photon buffer on |
| GUI `FluxAnalysis::run()` | Requested flux rays; sun grid from GUI fields; receiver-bin grid is separate | Constructor receives GUI fixed/automatic resolved seed; per invocation uses `m_masterSeed++` | Borrowed GUI layout; optional `SceneTreeModel::UpdateSceneModel` callback during prepare | Borrowed layout/sun; selected surface and retained photons; GUI supplies air |
| CLI `--headless trace-scene` | Required explicit `--rays N`; default sun grid 200x200 | Required explicit `--seed S` (zero valid) | Loaded scene; **default leaves trackers as loaded**; `--update-trackers` calls `updateParents()` then `updateTrackers()` prior to building instance tree | Owned headless tree/sun; non-vacuum air resolved; optional restricted File-export diagnostic; otherwise no buffer |
| CLI `--headless benchmark` | JSON `rays` (default 500,000,000) and default sun grid 200x200 | JSON `seed` (default 123456789) | Loaded scene; default tracker updates **off** for frozen scientific reference | Owned headless tree/sun; non-vacuum air; hit callback for flux accumulators; no photon export |
| CLI `--headless run-script` → `tn.traceScene` | `options.rays` required positive; default sun grid 200x200 | `options.seed` optional; default 0 | Independently loaded scene for every `traceScene` call; tracker updates **off** | Owned headless tree/sun; no export required (`noExport:true`) |
| GUI script `tn.Run()` / `tn.FindInterception()` | Uses GUI state and GUI flux settings | GUI current seed policy (interception calls resolve an automatic GUI seed) | In-memory GUI document with GUI scene preparation | Delegates to `MainWindow` and/or `FluxAnalysis`; uses GUI's photon handling |
| Legacy positional `.tnhpps` invocation (without `--headless`) | Calls GUI `MainWindow` API | GUI default/current seed policy | Creates `MainWindow` and SoQt; not a true headless scripting implementation | Uses GUI-linked paths, not `HeadlessScriptHost` |

**Compatibility warning:** GUI/headless tracker defaults and seed defaults differ intentionally today. M1 unifies the **computational preparation implementation** without silently changing these effective inputs. Standardizing invocation defaults is a separate, explicit policy decision. Script API parity belongs to M3 and exporter parity to M4.

## Changes in this PR (M1.1 and M1.2 first bounded slice)

1. Preserve the existing public `prepareGuiTrace` and `prepareHeadlessTrace` adapters so current callers remain source-compatible.
2. Keep GUI borrowed-tree synchronization before common preparation. Keep headless opt-in tracker update and owned instance-tree construction before common preparation.
3. Route both adapters to one private `prepareResolvedTrace` implementation for analytical instance-tree updates, bounds checks, sun-aperture sizing, sun-cell sampling, transform initialization, effective seed/ray assignment and context handoff.
4. Preserve the GUI/headless historical sun-transform assignment order on sampling success/failure and current error/progress strings.
5. Preserve GUI-chosen air/export-surface selection and headless non-vacuum air policy. Ownership stays in `PreparedTraceContext`; no extra tree build, RNG, scheduler, optics or photon changes.
6. Add `ScientificTracePreparation.CommonPipelinePreservesAdapterContracts`, comparing independently loaded GUI-style borrowed and headless-owned preparations, synchronization/progress contracts, sampled cells and power/area/irradiance.

## Acceptance and known limitations

- **Not yet run:** local C++ compilation, scientific CTests, real GUI smoke tests, cross-platform CI. The code must not be labelled scientifically accepted merely because it is committed.
- **Required local check:** incremental Release build of `tonatiuhpp` and `tonatiuhpp_scientific_equivalence_tests`; `ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure`, plus relevant short headless `trace-scene`/tracker tests.
- **Next migration:** complete M1.3 by verifying caller-level effective inputs/outputs and removing any remaining preparation duplication, then close M1 only after the scientific regression gate. Existing callers continue to use the two compatibility adapter names; both now lead to one implementation.
- **Not in this PR:** M2 propagation unification; M3 shared scripting host; M4 exporter redesign; M5 build target separation; changes to reference fixtures, 500M reference or ray-tracing physics.
