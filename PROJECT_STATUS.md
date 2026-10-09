# Project Status

Last updated: 2026-10-09

Purpose: lightweight handoff for current Tonatiuh++ project and release context. Keep stable agent rules in `AGENT.md`; update this file when release context changes.

## Development checkpoint — Milestone 3 (2026-10-09)

- Active branch: `test/benchmark-reference-integrity-m3-20261009`, from merged M2 and CI-cache master `a8d8fc7c`. Validation and review pending.
- Add a fast, dependency-free cross-platform check of the published 500M benchmark-v2 JSON, little-endian float64 binary grid and CSV: immutable reference identity, grid dimensions and bounds, SHA-256, exact CSV/binary round trips, and numerical min/average/max agreement. No production physics, RNG, fixtures, or scientific reference values changed.
- The repository's benchmark examples currently do not contain the `benchmark_heliostat_field_v1.tnhpp` scene referred to by the 500M configuration/reference. The check is **artifact integrity only**, not a new 500M reproduction. Full physical benchmark and release packaging validation remain pending.
- Run the integrity test during CI on Windows, Linux and macOS; existing scientific/headless tests remain enabled. See `docs/benchmark-reference-integrity-m3.md`.

## Development checkpoint — Milestone 2 (2026-10-09)

- Active development branch: `test/cross-platform-headless-validation-m2-20261009`, based on M1-merged master `cad20757`; PR pending review, tests not yet run.
- Register the real headless CTest smoke suite by default against the build-tree executable on Windows as well as Linux and macOS; retain the installed executable override.
- Add a repeatability smoke test that compares exact SHA-256 flux-grid hashes and power/flux metrics across two independent 20,001-ray benchmark processes using seed 123456789 and three scheduler chunks.
- Enforce headless test registration in CI; Windows CI failures are no longer ignored. All existing 11 scientific regressions remain unchanged; expected Windows test selection: 21 cases.
- No ray physics, random-number derivation, energy normalization or reference datasets changed. Validation and scientific author review pending; see `docs/headless-validation-m2.md`.

## Development checkpoint — Milestone 1 (2026-10-09)

- M1 merged through PR #7 into master commit `cad20757`, preserving the validated feature source tree.
- First bounded slice: shared `SimulationConfig` centralizes ray count, canonical master seed, 200 × 200 default sun-grid divisions and scientific input validation. Production GUI, flux, headless CLI, headless scripts and benchmark adapters supply the same configuration to trace preparation. Existing ray execution fields, photon exporter settings, and public parsing APIs are left intact.
- Two new scientific configuration tests cover existing validation messages, defaults and rejection from both preparation entry points. Nine M0C tests retained; expected discovery count **11**.
- Windows/MSVC Release builds passed, 11/11 scientific tests passed, a 4,096-ray headless CLI check exited 0, and Windows/Linux/macOS CI passed before M1 was approved and merged. No ray physics, seed derivation, chunk scheduling, photon exporter policy or benchmark reference values intentionally changed.
- Documentation: `docs/simulation-configuration-m1.md`.

## Development checkpoint — Milestone 0C (2026-10-08)

- Approved policy: simulation GUI and headless modes use analytical ray-tracing geometry to size the sun aperture; legacy GUI results can change where Coin display-mesh bounds diverge.
- M0C was merged in PR #6 to master commit `488b9c87` after Windows 9/9 scientific tests passed, a successful real Windows GUI test and three-platform CI success.
- GUI preview resizing and both trace-preparation entry points use analytical instance-tree bounds, eliminating production dependencies on Coin graphical mesh refresh timing for sun sizing. Flux analysis uses the same GUI preparation path.
- Added direct GUI/headless aperture-area, sampled-cell, and power-per-ray parity test on cylinder and Fresnel scenes after deliberately perturbing GUI sun sizing. The approved analytical geometry policy changes historical GUI energy normalization where bounds differed; further flux benchmark comparison remains a release-readiness task.

## Development checkpoint — Milestone 0B (2026-10-08)

- Active branch: `test/scientific-equivalence-m0b-20261008`, based on merged M0A `master`.
- Work in progress: additional two-surface Fresnel and instrumented exponential-air scientific regressions, plus an opt-in native QApplication/SoQt sun-aperture diagnostic.
- Windows/MSVC Release rebuilt and passed **8/8 scientific CTest tests after** the production Coin action-traversal correction (3.45 seconds total; developer-reported 2026-10-08). The native GUI diagnostic now completes both fixtures without the prior access violation; exit code 1 indicates a measured numerical aperture mismatch.
- With pending Coin field sensors processed, the GUI and headless sampled sun-cell masks match (cylinder 60/60; Fresnel two-surface 323/323). Full-precision GUI versus headless areas: cylinder 0.14738736997989096 versus 0.14738737141625147 (relative difference ~9.7455e-9); Fresnel 1.1908791977337683 versus 1.1965395657699263 (relative difference ~0.00473062, or 0.473%). Analytical planar `ShapeRT::getBox()` thickness differs from the flat Coin mesh and can change area-based energy normalization. A GUI/headless aperture-geometry policy and latest cross-platform CI review are still required before merging PR #5.
- Other ray-tracing physics, RNG, scheduling, and photon-export algorithms are unchanged.
- See `docs/scientific-equivalence-m0b.md` for commands and limitations.

## Development checkpoint — Milestone 0A (2026-10-08)

- Active development branch: `test/scientific-equivalence-m0a-20261008`, based on the v0.1.8.26 release source.
- Added initial `QCoreApplication` scientific tests for repeatability of GUI-style and headless-style preparation, using independent cylinder scene loads and deterministic hit histograms.
- GUI/headless borrowed-versus-owned comparison and photon-recording versus no-recording comparison passed explicitly and were promoted to enabled CTest regressions. Final Windows/MSVC Release CTest validation: 4/4 tests passed (2.49 seconds total) on 2026-10-08.
- No ray physics, RNG, scheduling, photon exporter, or release artifact changes were made in this milestone.
- The Windows/MSVC Release test target compiled and all four enabled scientific regressions passed locally. An initial GUI-style harness crash with SEH `0xc0000005` stopped occurring after switching test sun sizing to ray-tracing bounds; its precise cause was not confirmed by a debugger. Actual GUI sun sizing, other geometries, cross-platform tests, and the official benchmark remain outside this milestone. See `docs/scientific-equivalence.md`.
- The July 2026 release-checklist sections below are retained as historical handoff context and should be refreshed separately.
## Current Priorities

- Prepare the Tonatiuh++ `v0.1.8.26` release from the published `v0.1.8.25` baseline; source metadata, release notes, benchmark v2 assets, citation metadata, and the release checklist are prepared, but do not tag until build, test, packaging, and platform validation are complete.
- Validate the `v0.1.8.26` canonical tracing/RNG changes, Qt worker QoS behavior, benchmark reference v2, release notes, runtime help, headless commands, CTest coverage, updater path, and platform packages.
- Remove or reduce remaining runtime warnings visible from command-prompt launches.
- Validate the IFW updater path from the latest published IFW baseline to the next tagged release on Windows, Linux, and macOS.

## Current Baseline

- Branch: `master`.
- Current published release and latest Git tag: `v0.1.8.25`.
- Current application version in `source/CMakeLists.txt`: `0.1.8.26`.
- Next intended release target: `v0.1.8.26`.
- Release readiness: source and documentation preparation is complete; clean builds, full tests, GUI/headless validation, Linux/macOS scientific comparison, packaging, updater checks, signing, tagging, GitHub publication, and Zenodo publication remain external steps.
- Previous release baseline for updater validation: published `v0.1.8.25` IFW installations.
- Intended release under validation: `v0.1.8.26`, focused on the canonical tracing architecture, deterministic per-chunk RNG streams, removal of legacy RNG/profiling infrastructure, hybrid-CPU QoS correction, and benchmark reference v2.
- Release packaging source of truth: `.github/workflows/release.yml` and `installer/`.
- Headless execution foundation is now implemented in the existing `tonatiuhpp` executable; there is no separate console executable or installer target.
- Current headless commands: `tonatiuhpp --headless --help`, `tonatiuhpp --headless validate-scene <scene.tnhpp>`, `tonatiuhpp --headless trace-scene <scene.tnhpp> --rays N --seed S --no-export`, `tonatiuhpp --headless benchmark <benchmark_config.json>`, and `tonatiuhpp --headless run-script <script.tnhpps>`.
- Release notes: `docs/release-notes-v0.1.8.26.md`.
- Release checklist: `docs/release-checklist-v0.1.8.26.md`.
- Benchmark v1 dataset DOI documented in `docs/headless-benchmark.md`: `https://doi.org/10.5281/zenodo.20395328`.

## Recent Completed Milestones

- Headless startup foundation: `--headless` is parsed before GUI startup, uses `QCoreApplication`, avoids `MainWindow`, SoQt widgets, hidden windows, progress dialogs, and `SceneTreeModel`, and validates `.tnhpp` scene loading through shared core services.
- Headless scene loading: core Coin/nodekit/interaction initialization, Tonatiuh type registration, scene-factory plugin filtering, project search paths, and shared `.tnhpp` loading were extracted for reuse by GUI and headless paths.
- Headless validation was tested on Windows with an installed scene using plugin mesh assets: `tonatiuhpp.exe --headless validate-scene ".../solatom_module.tnhpp"` reported `Scene validation succeeded`.
- Headless trace-scene foundation: builds a non-GUI `InstanceNode` tree, sizes the sun aperture from ray-tracing bounds instead of Coin GUI bounding-box traversal, runs the kernel ray tracer with deterministic private per-chunk `std::mt19937_64` streams, disables photon export by passing no photon buffer/exporter, and reports progress, elapsed seconds, and rays/s.
- Random-number subsystem modernization: tracing now uses one private `std::mt19937_64` stream per stable 10,000-ray chunk, seeded only from the resolved master seed and chunk index through a fixed-width SplitMix64-style derivation. The shared RNG, RNG mutex, `RandomParallel` cache, legacy custom generators, RNG plugin selection, and internal tracing/RNG profiler instrumentation were removed. Exact historical photon sequences and flux-grid hashes may change; external scientific and cross-platform validation remains required, and future performance analysis must use external profilers.
- Hybrid-CPU scheduling fix: on Qt 6.9 and newer, each long-lived ray worker temporarily requests `QThread::QualityOfService::High` and restores its previous service level on exit. This corrected observed Windows headless runs where 20 runnable workers were time-sliced across the four i7-12700K E-cores despite unrestricted affinity. Rebuilt 10,000,000- and 500,000,000-ray benchmarks used all 20 logical CPUs. The full run completed in `867.486` seconds at `576378` rays/s with `42.217052` MW total power and `19.525198` MW/m2 maximum flux; total-power (`0.007896%`) and maximum-flux (`0.228023%`) tolerances passed. Its expected post-RNG-refactor flux-grid hash mismatch remains for external reference validation. GUI regression and Linux/macOS validation remain required.
- Benchmark v2 reference candidate: the completed 500,000,000-ray post-RNG-refactor result is integrated under `examples/benchmarks/` as `benchmark_reference_500M_v2.json`, `benchmark_reference_flux_grid_500M_v2.bin`, and `benchmark_reference_flux_grid_500M_v2.csv`. The 80,000-byte binary contains the expected 100x100 little-endian float64 grid and its SHA256 matches the JSON value `36c7dc17cee8e7fdef820f08d4482e7c3dcdee3b5eed4748ea3949a6cf4af472`; the CSV also has exactly 100 rows and 100 columns. A second 500,000,000-ray run in a separate Windows process completed in `798.608` seconds at `626089` rays/s and passed total power, maximum flux, and exact flux-grid hash comparison with zero metric error. Cross-platform comparison and durable Zenodo archival remain required before treating v2 as authoritative.
- Headless trace validation was tested on Windows with an installed plugin/mesh scene: `tonatiuhpp.exe --headless trace-scene ".../solatom_module.tnhpp" --rays 10000000 --seed 123456789 --no-export` completed in `13.384000` seconds at `747160.789002` rays/s.
- Headless benchmark v1: `tonatiuhpp --headless benchmark <benchmark_config.json>` parses benchmark JSON, loads the configured scene without GUI startup, runs the existing ray tracer without photon export or photon buffering, accumulates target-side hits into the configured flux grid, computes power/flux metrics and a little-endian float64 grid SHA256, writes result JSON, and supports optional reference comparison fields.
- Headless benchmark validation was tested on Windows with `benchmark_heliostat_field_v1.tnhpp` at 10,000 rays and seed `123456789`: elapsed time and rays/s were reported, result JSON was written, `total_power_mw` was `44.31264485072039`, `maximum_flux_mw_m2` was `61.354459529685954`, and `flux_grid_sha256` was `b90ae20902ed96237ea4e29f87064493ee6b91cc9b8ad3848f139885698f1074`.
- Benchmark reference comparison was tested with the same 10,000-ray result as the reference and produced `benchmark_pass: true`, matching flux-grid hash, and zero total-power/maximum-flux error.
- Headless benchmark stabilization review: added guards for excessive grid allocations, non-finite hit coordinates, invalid power-per-ray values, and non-finite computed metrics before writing benchmark JSON.
- Medium benchmark determinism was tested on Windows with `benchmark_heliostat_field_v1.tnhpp` at 1,000,000 rays and seed `123456789`: repeated runs produced matching total/minimum/average/maximum flux metrics and matching `flux_grid_sha256` (`d314e2720957ab15e581d29bcc6e868b1005bae5fdcdf15e23c4446bfc501d64`); elapsed time and rays/s varied as expected.
- Tonatiuh++ now has one supported parallel ray-tracing implementation: the shared QCore-compatible `RayTraceExecutor`, using long-lived workers on the global Qt thread pool, deterministic private per-chunk random streams, and serialized photon-buffer access. GUI `MainWindow::Run()`, headless `trace-scene`, benchmark tracing, and `tn.traceScene()` all call this executor.
- RayTraceExecutor now runs one long-lived worker loop per available global-pool thread. Workers atomically acquire fixed 10,000-ray chunks until the trace is exhausted, replacing per-work-item `QtConcurrent::map` scheduling without restoring RayTraceRunner or adding another backend.
- Embedded tracing and RNG profiling diagnostics have been removed completely; no hidden environment-variable switch remains. External profilers are required for performance analysis, while benchmark-level elapsed time, throughput, worker count, chunk count, and chunk size remain normal result metadata.
- The alternative `RayTraceRunner` std::thread/chunk/per-chunk-RNG model was removed, including configurable worker/chunk scheduling and per-chunk seed derivation. `ParallelRayTraceExecutor` no longer exists as a separate class or backend; its GUI-faithful behavior was consolidated into `RayTraceExecutor` and the duplicate inline GUI scheduler was extracted to the same service.
- Benchmark `worker_count` and `chunk_size` inputs are no longer supported because they do not control the historical GUI execution model. Configurations containing either field are rejected with a compatibility error; result JSON continues to report the effective Qt thread/partition shape for machine-readable compatibility.
- Headless benchmark worker/chunk tuning on Windows confirmed that `20` logical workers remain appropriate on the benchmark host and that reducing the default chunk size from `100000` to `10000` improves 100,000,000-ray throughput slightly (`601189` to `609515` rays/s) while preserving deterministic grid hashes for fixed rays, seed, and chunk size.
- Benchmark result JSON reports the effective Qt worker/work-item shape through `worker_count`, `chunk_count`, and `chunk_size`; these are result metadata rather than configurable scheduling controls.
- RayTraceExecutor correctness hardening now atomically rejects overlapping asynchronous starts on one executor instance until the active future has been fully joined, and benchmark thread-local accumulator caches are isolated by a unique process-wide run generation. The canonical scheduler, ray physics, and benchmark formulas are unchanged.
- Trace preparation and execution are now separate internal phases. All five tracing callers use `TracePreparation` to produce a move-only `PreparedTraceContext`, and the execution-only `RayTraceExecutor` accepts that shared contract without rebuilding or interpreting a scene.
- GUI tracing and flux analysis prepare borrowed contexts over their persistent `SceneTreeModel`/`InstanceNode` hierarchy, existing sun instance, resolved master seed, photon buffer, and export selections. Headless trace, benchmark, and `tn.traceScene()` prepare owned contexts containing exactly one newly built instance tree plus the sun instance and explicit master seed, retained through executor completion.
- Headless benchmark reference workflow now supports optional `flux_grid_output_file`, `reference_flux_grid_file`, and reference JSON `flux_grid_file` CSV comparison while preserving the existing little-endian float64 row-major `flux_grid_sha256` hash.
- Headless benchmark flux-grid references now support raw little-endian float64 row-major binary output/reference files through `flux_grid_binary_output_file`, `reference_flux_grid_binary_file`, and reference JSON `flux_grid_binary_file`; binary references are preferred over CSV when both are supplied.
- Release documentation for `v0.1.8.20` now covers the headless commands, benchmark JSON schema, CSV/binary flux-grid references, deterministic comparison guidance, and Zenodo benchmark dataset DOI.
- Properties panel usability fixes for `v0.1.8.21`: expanded/collapsed groups are preserved across compatible selections, transform-related groups expand by default when no stored state exists, and the property-name column is sized more usefully by default.
- macOS 3D navigation fix for `v0.1.8.21`: Shift-drag translation now has an `__APPLE__` path and temporary camera-view modifier handling uses Qt keyboard modifiers.
- macOS release hardening for `v0.1.8.21`: CMake emits the canonical `TonatiuhPP.app` bundle, IFW staging validates the app bundle layout, release CI validates `Info.plist`, executable architecture, and bundled Mach-O dependencies, and release artifacts are explicitly Apple Silicon / arm64.
- About dialog links for `v0.1.8.21` now show only the current Tonatiuh++ repository and the AI-HPC4CST Project website.
- Runtime noise reduction: removed stray `ShapeMesh` debug prints that emitted resolved OBJ paths and project search paths during scene loading.
- View/model stability: idempotent node sensor attaches, no nested `ParametersModel` reset, and stale `SceneTreeModel` index guards.
- Dependency bootstrap: clearer missing Eigen/Boost diagnostics and a hardened Windows dependency path.
- Installer/release flow: IFW repository release metadata rendering, `com.tonatiuhpp.app` package id normalization, GitHub Pages root platform repositories, and removal of unsupported `binarycreator` hybrid mode.
- Photon export/raytracing reliability: retained photons survive export failures, fatal export failures propagate, file export is failure-aware and sequential, and photon buffer/file split limits are honored.
- Updater path: legacy GitHub release parsing was replaced with the Qt IFW `MaintenanceTool` flow, including `check-updates` classification and user-approved updater launch.
- Runtime help packaging for `v0.1.8.23`: Sphinx runtime help generation is wired into CI and release workflows, generated `help/html` is installed with the runtime payload, and help branding was refreshed for Tonatiuh++.
- Linux runtime packaging for `v0.1.8.23`: Linux installs now use a launcher script, bundled Qt runtime/plugins, `qt.conf`, and validation that the bundled Qt libraries and plugins are self-consistent.
- Linux release validation for `v0.1.8.23`: bundled Qt plugin metadata strings are treated as advisory when unavailable, while hard validation still checks the launcher, `qt.conf`, the xcb platform plugin, bundled QtCore, Qt plugin category layout, and `ldd` resolution into the bundled `lib/` runtime.
- macOS release artifact staging for `v0.1.8.23`: after runtime files are moved into `TonatiuhPP.app/Contents`, the release workflow prunes top-level development install artifacts so the strict app-bundle-only artifact validation can remain in place.
- IFW release-script hardening for `v0.1.8.23`: the package install script now declares the required IFW `Component` constructor so `repogen` can parse it on Linux/macOS, while `.tnhpp` and `.tnhpps` file-type registration remains guarded to Windows installer runs.
- Windows release build hardening for `v0.1.8.23`: optional post-build and install-time third-party DLL fallback copies no longer fail when `third_party/_install/bin` is absent, and GoogleTest discovery is deferred to CTest time on Windows instead of executing test binaries during the release build.
- Updater launch compatibility for `v0.1.8.23`: MaintenanceTool startup now uses the Qt 6.6-compatible `QProcess` instance `startDetached()` path so the Linux/macOS/Windows release builds preserve detached launch behavior, working directory, process ID capture, and the bundled Linux Qt runtime environment.
- Release retarget for `v0.1.8.24`: because `v0.1.8.23` artifacts were already published before the final release-blocker fixes were complete, the source version and release notes now target `v0.1.8.24` as the superseding packaging/build-fix release.
- Launch/file-association hardening for `v0.1.8.23`: positional `.tnhpp` files open on GUI startup, positional `.tnhpps` startup paths use the GUI script window/editor and load the clicked script without automatic execution, while existing `-i script.tnhpps` execution behavior is preserved.
- Windows file icon hardening for `v0.1.8.23`: Windows builds now embed the existing Tonatiuh++ `Tonatiuh.ico` as a native executable icon, and the IFW package registers `.tnhpp` and `.tnhpps` file types with `TonatiuhPP.Project` and `TonatiuhPP.Script` ProgIDs pointing at the installed executable icon.
- CTest smoke-test foundation for `v0.1.8.23`: `BUILD_TESTING` now enables headless CTest coverage for `--headless --help`, invalid headless arguments with the expected failure exit code, and `validate-scene` on the plugin-free `examples/benchmarks/cylinder.tnhpp` fixture when present; `TONATIUHPP_TEST_EXECUTABLE` can point CTest at an installed runtime, and Windows skips headless tests with a clear warning when that executable is not set.
- Headless command hardening for `v0.1.8.23`: `trace-scene` and `benchmark` now print consistent key-value summary lines for scene path, rays, seed, no-export status, elapsed time, throughput, scheduling fields, and result/output paths where applicable; CTest smoke coverage now includes small `trace-scene` and benchmark runs on the plugin-free cylinder fixture.
- Minimal true-headless script runner for `v0.1.8.25`: `tonatiuhpp --headless run-script <script.tnhpps>` evaluates scripts through a `QCoreApplication`/`QJSEngine` host without `QApplication`, `SoQt::init`, `MainWindow`, `ScriptWindow`, `NodeObject`, or GUI widgets. The first API is limited to `print(value)`, `tn.writeJson(path, value)`, `tn.validateScene(path)`, `tn.runBenchmark(path)`, and deterministic no-export `tn.traceScene({ scene, rays, seed, noExport: true })`; legacy `tonatiuhpp -i script.tnhpps` remains unchanged and GUI-bound.
- RC metadata/docs retarget for `v0.1.8.25`: source version metadata now reports `0.1.8.25` so application, IFW installer, IFW package, and `Updates.xml` version rendering align with the next release candidate while `v0.1.8.24` remains the latest published updater baseline.
- GoogleTest unit-test foundation for `v0.1.8.23`: `BUILD_TESTING` now resolves GoogleTest with `find_package(GTest CONFIG QUIET)` or a pinned `FetchContent` fallback to GoogleTest `v1.14.0`, adds small math targets under `tests/unit/libraries/math`, and registers deterministic `Interval` and `Box2D` unit tests with CTest through `gtest_discover_tests()`. GUI, Qt widget, Coin, SoQt, plugin, and ray-tracing behavior are intentionally unchanged and untested by these first targets.

## Current Release Workflow

- Source version metadata is set to `0.1.8.26`; tag `v0.1.8.26` only after clean build, complete test, package, updater, and platform validation are complete.
- Pushing a `v*` tag triggers the GitHub Actions `Release` workflow. `workflow_dispatch` accepts a `fake_tag` for simulation and skips the publish job.
- The workflow resolves the version with `installer/sync_ifw_metadata.py --check-tag`, derives a UTC release date, builds Linux/macOS in a matrix, and builds Windows on `windows-2022`.
- Packaging stages the release payload, generates per-platform IFW repositories with `repogen`, generates IFW installers with `binarycreator`, uploads assets and checksums, and deploys the IFW repositories to GitHub Pages.
- Current installer URL rendering uses base `https://cst-modelling-tools.github.io/tonatiuhpp` plus the platform path: `/windows`, `/linux`, or `/macos`.

## Updater Architecture

- Tonatiuh++ does not download GitHub release assets for updates; update-capable installs are Qt IFW installs.
- `IfwUpdateService` locates the Qt IFW `MaintenanceTool` near the installed application, runs `check-updates`, classifies the result, and starts the tool detached with `--start-updater`.
- `UpdateDialog` displays the installed version, update status, details, and MaintenanceTool path; the install button is enabled only when an update is available.
- `MainWindow` performs a delayed startup check, exposes `Help > Updates`, prompts before launching the updater, then closes Tonatiuh++ so installed files can be replaced. Manual restart is still expected after update completion.

## Known Technical Debt / Warnings

- Benchmark v1 execution is implemented for the configured receiver transform and grid, and a structurally validated 500,000,000-ray post-RNG-refactor benchmark v2 reference candidate has been generated and reproduced exactly in a separate Windows process. Cross-platform scientific review and durable dataset archival remain required before replacing any historical authoritative hash.
- Benchmark result JSON includes elapsed time and rays/s, so whole-file byte-for-byte equality is not expected across repeated runs; deterministic comparison should use metrics and `flux_grid_sha256`.
- GUI presentation and photon-export lifecycle remain in `MainWindow::Run()`, while its parallel scheduling is provided by the same `RayTraceExecutor` used headlessly. Future tracing work must extend this canonical executor rather than introduce another backend.
- Benchmark performance, deterministic hashes, reference outputs, and GUI/headless scientific equivalence require external build and runtime validation after the executor consolidation.
- The RayTraceExecutor worker-loop strategy and new per-chunk RNG stream decomposition require external performance and scientific-output validation at representative 10 million, 50 million, and 500 million ray counts. Historical exact hashes may change even though ray physics, sampling formulas, and benchmark formulas remain unchanged.
- The Qt 6.9+ ray-worker `High` quality-of-service request is validated through 10,000,000- and 500,000,000-ray headless runs on one hybrid Windows host; confirm GUI throughput and Linux/macOS behavior. Qt 6.8 and older retain their existing scheduling behavior.
- The preparation/execution split requires external GUI and headless validation before release: compare fixed-seed scientific outputs, exercise GUI photon export and cancellation, verify flux analysis, and confirm that owned headless context lifetime extends through the final worker join.
- Headless mode still needs CI-style validation on Linux and macOS before relying on it for cluster workflows.
- Headless `run-script` is a first milestone for automation only. It does not expose scene mutation, screenshots, GUI-compatible `MainWindow` APIs, photon export, or legacy script UI behavior; `tn.traceScene` requires `noExport: true`.
- In the Codex shell environment on Windows, direct CMake/Ninja invocations intermittently wedged and left stale generated metadata; prefer VS Code CMake Tools or the user's normal terminal build path for validation unless this is rechecked.
- Manual restart is still expected after IFW updates.
- macOS signing and Gatekeeper behavior still need full validation.
- Remaining runtime warnings visible from command-prompt launches need triage and reduction; current build still reports existing `FluxAnalysis.cpp` warnings C4834 and C4805.
- Release documentation should be rechecked if IFW URL paths or package IDs change; current scripts use `https://cst-modelling-tools.github.io/tonatiuhpp/{windows,linux,macos}`, while older checklist text may still mention `/ifw/...`.
- User-reported startup or normal-use segmentation faults remain unresolved. Current packaging evidence points away from missing bundled Qt libraries as the only explanation; next diagnostics should collect terminal output with Qt plugin diagnostics, Windows Event Viewer faulting module, GPU/OpenGL driver details, and whether headless scene validation succeeds on the same scene.
- Automated test coverage now includes headless CTest smoke tests plus initial GoogleTest math unit targets. Broader GoogleTest coverage, regression fixtures, GUI smoke coverage, and scientific validation tests remain pending.

## Pending Validation

- Confirm normal GUI startup still behaves as before after the headless entry-point changes.
- Confirm `tonatiuhpp --headless validate-scene` on representative plugin-based scenes on Linux and macOS.
- Confirm normal GUI startup still behaves as before after the parallel headless `trace-scene` changes.
- Validate GUI photon export, cancellation, progress, retained-photon handling, ray display, and cumulative power calculation through the shared `RayTraceExecutor`.
- Confirm normal GUI startup still behaves as before after the headless `benchmark` changes.
- Confirm positional `.tnhpps` startup opens the GUI script window/editor, loads the script, and does not execute it automatically.
- Confirm Windows installed `.tnhpp` and `.tnhpps` associations use Tonatiuh++ icons in Explorer and launch the installed executable with the selected file path.
- Confirm the expanded headless CTest smoke tests pass on Linux/macOS build-tree runners and on Windows when configured with `TONATIUHPP_TEST_EXECUTABLE` pointing at an installed runtime.
- Confirm `tonatiuhpp --headless run-script` smoke coverage passes on Linux/macOS build-tree runners and on Windows installed-runtime CTest, including `print`, `tn.writeJson`, `tn.validateScene`, `tn.runBenchmark`, and no-export `tn.traceScene`.
- Expand GoogleTest beyond the current math `Interval` and `Box2D` coverage, then add regression fixtures, GUI smoke tests, and benchmark/scientific validation after the smoke foundation is stable.
- Confirm `trace-scene` and `benchmark` produce no photon files from the installed application output directory on representative runs.
- Review and publish the integrated 500,000,000-ray benchmark v2 reference through the durable Zenodo dataset, preserving its SHA256 and existing metric tolerances; retain historical v1 artifacts for reproducibility.
- Confirm the `v0.1.8.26` `Release` workflow succeeds on Windows, Linux, and macOS from the matching tag.
- Confirm GitHub Pages serves each generated `v0.1.8.26` IFW repository at the exact URL embedded in its installer.
- Confirm every generated repository has `Updates.xml` and package metadata for `com.tonatiuhpp.app`.
- Test update detection from published `v0.1.8.25` to `v0.1.8.26` on Windows, Linux, and macOS.
- Test update installation from published `v0.1.8.25` to `v0.1.8.26` on Windows, Linux, and macOS, including non-blocking startup check, `Help > Updates`, install prompt, MaintenanceTool launch, application shutdown, and manual restart.
- Reconcile or explicitly re-check release documentation if URL paths or package IDs change.

## Pre-release Checklist

- Install published `v0.1.8.25` IFW builds on Windows, Linux, and macOS as the update source.
- Verify `source/CMakeLists.txt` has the intended `0.1.8.26` version.
- Create and push the matching `v<version>` tag.
- Confirm the `Release` workflow and GitHub Pages deploy complete successfully.
- Confirm release assets include Windows, Linux, and macOS IFW installers and checksums, plus any intended manual archives.
- Verify each platform installer embeds the correct IFW repository URL and each URL serves `Updates.xml`.
- After `v0.1.8.26` release repositories are published, confirm `Help > Updates` in the installed v0.1.8.25 app detects `v0.1.8.26`.
- Run manual updater installation validation from installed `v0.1.8.25` to `v0.1.8.26` on Windows, Linux, and macOS.
