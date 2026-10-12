# M1 — Effective simulation-preparation policies and first canonical pipeline

**Status:** first bounded M1.1/M1.2 preparation consolidation **MERGED** in [PR #28](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/28), merge commit `95f0ece14ec499f3d7b0d0a2bb20b7357527b9a0` (2026-10-12 Europe/Madrid). Local Windows Release build, 27 targeted tests, and all three hosted CI jobs passed. A real GUI tracing/Flux Analysis execution smoke also completed. **M1.3 and full M1 acceptance remain open.**
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

## Changes merged by PR #28 (M1.1 and M1.2 first bounded slice)

1. Preserve the existing public `prepareGuiTrace` and `prepareHeadlessTrace` adapters so current callers remain source-compatible.
2. Keep GUI borrowed-tree synchronization before common preparation. Keep headless opt-in tracker update and owned instance-tree construction before common preparation.
3. Route both adapters to one private `prepareResolvedTrace` implementation for analytical instance-tree updates, bounds checks, sun-aperture sizing, sun-cell sampling, transform initialization, effective seed/ray assignment and context handoff.
4. Preserve the GUI/headless historical sun-transform assignment order on sampling success/failure and current error/progress strings.
5. Preserve GUI-chosen air/export-surface selection and headless non-vacuum air policy. Ownership stays in `PreparedTraceContext`; no extra tree build, RNG, scheduler, optics or photon changes.
6. Add `ScientificTracePreparation.CommonPipelinePreservesAdapterContracts`, comparing independently loaded GUI-style borrowed and headless-owned preparations, synchronization/progress contracts, sampled cells and power/area/irradiance.

## Acceptance and known limitations

- **Windows/MSVC Release local build PASSED** (maintainer-reported, 2026-10-12): `cmake --build build --config Release --target tonatiuhpp tonatiuhpp_scientific_equivalence_tests --parallel 8`. The application and scientific tests compiled. Existing nonfatal `FluxAnalysis.cpp` warnings `C4834` and `C4805` were observed.
- **Scientific CTests PASSED:** `ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure`: **24/24**, including `ScientificTracePreparation.CommonPipelinePreservesAdapterContracts`; real test time **19.10 s**.
- **Headless CTests PASSED:** `ctest --test-dir build -C Release -R '^headless[.](trace_cylinder_scene|trace_tracker_opt_in|trace_tracker_legacy)$' --output-on-failure`: **3/3**, real test time **2.15 s**.
- **Hosted CI PASSED:** [Actions run #38183984014](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/38183984014), successful full native build/test jobs for **Ubuntu 24.04, Windows, and macOS**, at PR head `33219d15337d0e954843b7bc29b7cc459ef73e02`.
- **Native GUI execution smoke PASSED** (maintainer-reported): unchanged `examples/benchmarks/cylinder.tnhpp`, console output `gui_master_seed: 123456789 (fixed)` followed by `QtConcurrent finished` and `Elapsed time (Run): 73`; Flux Analysis reported `flux_analysis_master_seed: 123456789` and `Elapsed time: 31`, with no error text. The console transcript confirms execution but **not** whether the Flux Analysis map contained nonzero values; previous A1 real-GUI export characterization is separate evidence.
- **Scientific scope:** this gate validates the **bounded preparation consolidation**, not full GUI/CLI/script equivalence. No 500-million-ray benchmark rerun, frozen reference change, propagation algorithm change, RNG change, or photon-file format change was made.
- **Next package: M1.3**, verifying caller-level effective inputs/outputs and eliminating remaining preparation duplication without changing historical effective policies. Existing callers retain the public GUI/headless adapter names, both converging on one numerical preparation helper.
- **Deferred packages:** M2 propagation-loop consolidation, M3 shared scripting/session lifecycle, M4 export and flux parity, and M5 independent GUI-free core build.
