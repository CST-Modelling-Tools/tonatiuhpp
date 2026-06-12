# Project Status

Last updated: 2026-06-13

Purpose: lightweight handoff for current Tonatiuh++ project and release context. Keep stable agent rules in `AGENT.md`; update this file when release context changes.

## Current Priorities

- Prepare, publish, and validate the Tonatiuh++ `v0.1.8.21` maintenance release.
- Validate the `v0.1.8.21` release notes, Properties panel usability fixes, macOS navigation fix, macOS arm64 packaging, and About dialog link updates.
- Remove or reduce remaining runtime warnings visible from command-prompt launches.
- Validate the IFW updater path from the latest published IFW baseline to `v0.1.8.21` on Windows, Linux, and macOS.

## Current Baseline

- Branch: `master`.
- Current application version in `source/CMakeLists.txt`: `0.1.8.21`.
- Previous release baseline for updater validation: `v0.1.8.20` if published; otherwise use the latest published IFW build with the updater software.
- Intended release under validation: `v0.1.8.21`, focused on post-v0.1.8.20 usability fixes, macOS release hardening, and About dialog link updates.
- Release packaging source of truth: `.github/workflows/release.yml` and `installer/`.
- Headless execution foundation is now implemented in the existing `tonatiuhpp` executable; there is no separate console executable or installer target.
- Current headless commands: `tonatiuhpp --headless --help`, `tonatiuhpp --headless validate-scene <scene.tnhpp>`, `tonatiuhpp --headless trace-scene <scene.tnhpp> --rays N --seed S --no-export`, and `tonatiuhpp --headless benchmark <benchmark_config.json>`.
- Release notes draft: `docs/release-notes-v0.1.8.21.md`.
- Benchmark v1 dataset DOI documented in `docs/headless-benchmark.md`: `https://doi.org/10.5281/zenodo.20395328`.

## Recent Completed Milestones

- Headless startup foundation: `--headless` is parsed before GUI startup, uses `QCoreApplication`, avoids `MainWindow`, SoQt widgets, hidden windows, progress dialogs, and `SceneTreeModel`, and validates `.tnhpp` scene loading through shared core services.
- Headless scene loading: core Coin/nodekit/interaction initialization, Tonatiuh type registration, scene-factory plugin filtering, project search paths, and shared `.tnhpp` loading were extracted for reuse by GUI and headless paths.
- Headless validation was tested on Windows with an installed scene using plugin mesh assets: `tonatiuhpp.exe --headless validate-scene ".../solatom_module.tnhpp"` reported `Scene validation succeeded`.
- Headless trace-scene foundation: builds a non-GUI `InstanceNode` tree, sizes the sun aperture from ray-tracing bounds instead of Coin GUI bounding-box traversal, runs the kernel ray tracer serially with a deterministic `RandomSTL` stream, disables photon export by passing no photon buffer/exporter, and reports progress, elapsed seconds, and rays/s.
- Headless trace validation was tested on Windows with an installed plugin/mesh scene: `tonatiuhpp.exe --headless trace-scene ".../solatom_module.tnhpp" --rays 10000000 --seed 123456789 --no-export` completed in `13.384000` seconds at `747160.789002` rays/s.
- Headless benchmark v1: `tonatiuhpp --headless benchmark <benchmark_config.json>` parses benchmark JSON, loads the configured scene without GUI startup, runs the existing ray tracer without photon export or photon buffering, accumulates target-side hits into the configured flux grid, computes power/flux metrics and a little-endian float64 grid SHA256, writes result JSON, and supports optional reference comparison fields.
- Headless benchmark validation was tested on Windows with `benchmark_heliostat_field_v1.tnhpp` at 10,000 rays and seed `123456789`: elapsed time and rays/s were reported, result JSON was written, `total_power_mw` was `44.31264485072039`, `maximum_flux_mw_m2` was `61.354459529685954`, and `flux_grid_sha256` was `b90ae20902ed96237ea4e29f87064493ee6b91cc9b8ad3848f139885698f1074`.
- Benchmark reference comparison was tested with the same 10,000-ray result as the reference and produced `benchmark_pass: true`, matching flux-grid hash, and zero total-power/maximum-flux error.
- Headless benchmark stabilization review: added guards for excessive grid allocations, non-finite hit coordinates, invalid power-per-ray values, and non-finite computed metrics before writing benchmark JSON.
- Medium benchmark determinism was tested on Windows with `benchmark_heliostat_field_v1.tnhpp` at 1,000,000 rays and seed `123456789`: repeated runs produced matching total/minimum/average/maximum flux metrics and matching `flux_grid_sha256` (`d314e2720957ab15e581d29bcc6e868b1005bae5fdcdf15e23c4446bfc501d64`); elapsed time and rays/s varied as expected.
- Headless trace-scene now uses the shared worker/chunk `RayTraceRunner` path. On Windows with `benchmark_heliostat_field_v1.tnhpp`, 10,000,000 rays and seed `123456789` completed in `15.859000` seconds at `630556.781638` rays/s with `20` workers and `100` chunks of `100000` rays.
- Headless benchmark and trace-scene now share the same no-export `RayTraceRunner` execution backend. On Windows with the same 10,000,000-ray benchmark config, repeated benchmark runs completed in `15.642000` and `15.863000` seconds and produced matching deterministic metrics and `flux_grid_sha256` (`68e49409216b69410c976cdf27645051532de06b725c46e9fd7d75d1c623c397`).
- RayTraceRunner GUI-migration foundation: the shared runner now has an explicit output mode, cancellation callback, result flags for cancellation/export failure, rays-traced accounting, and optional prepared photon-buffer/export-surface plumbing. GUI exporter startup, append/non-append lifecycle, retained-photon safeguards, ray display, and `endExport(power)` remain owned by `MainWindow::Run()` until a dedicated GUI migration milestone.
- Headless benchmark worker/chunk tuning on Windows confirmed that `20` logical workers remain appropriate on the benchmark host and that reducing the default chunk size from `100000` to `10000` improves 100,000,000-ray throughput slightly (`601189` to `609515` rays/s) while preserving deterministic grid hashes for fixed rays, seed, and chunk size.
- Headless benchmark config now documents optional `worker_count` and `chunk_size` scheduling controls in `docs/benchmark_config_schema_v1.json` and `docs/headless-benchmark.md`; result JSON reports effective `worker_count`, `chunk_count`, and `chunk_size`.
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
- Launch/file-association hardening for `v0.1.8.23`: positional `.tnhpps` startup paths now use the GUI script window/editor and load the clicked script without automatic execution, while existing `.tnhpp` project startup and `-i script.tnhpps` execution behavior are preserved.
- Windows file icon hardening for `v0.1.8.23`: Windows builds now embed the existing Tonatiuh++ `Tonatiuh.ico` as a native executable icon, and the IFW package registers `.tnhpp` and `.tnhpps` file types with `TonatiuhPP.Project` and `TonatiuhPP.Script` ProgIDs pointing at the installed executable icon.
- CTest smoke-test foundation for `v0.1.8.23`: `BUILD_TESTING` now enables headless CTest coverage for `--headless --help`, invalid headless arguments with the expected failure exit code, and `validate-scene` on the plugin-free `examples/benchmarks/cylinder.tnhpp` fixture when present; `TONATIUHPP_TEST_EXECUTABLE` can point CTest at an installed runtime, and Windows skips headless tests with a clear warning when that executable is not set.
- GoogleTest unit-test foundation for `v0.1.8.23`: `BUILD_TESTING` now resolves GoogleTest with `find_package(GTest CONFIG QUIET)` or a pinned `FetchContent` fallback to GoogleTest `v1.14.0`, adds the small `tonatiuhpp_math_tests` target under `tests/unit/libraries/math`, and registers deterministic `Interval` unit tests with CTest through `gtest_discover_tests()`. GUI, Qt widget, Coin, SoQt, plugin, and ray-tracing behavior are intentionally unchanged and untested by this first target.

## Current Release Workflow

- Set the version in `source/CMakeLists.txt`, then tag the release as `v<version>`; the next intended release is `v0.1.8.21`.
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

- Benchmark v1 is implemented for the configured receiver transform and grid, but the full 500,000,000-ray baseline and authoritative reference JSON still need to be generated and archived.
- Benchmark result JSON includes elapsed time and rays/s, so whole-file byte-for-byte equality is not expected across repeated runs; deterministic comparison should use metrics and `flux_grid_sha256`.
- GUI `MainWindow::Run()` still uses its existing QtConcurrent/photon-export orchestration. Migrating it to `RayTraceRunner` remains pending because it must preserve exporter startup, retained-photon safeguards, append/non-append buffer lifecycle, progress-dialog cancellation, `ShowRaysIn3DView()`, and `endExport(power)` semantics. The runner can now accept an already-prepared photon buffer, but it intentionally does not own GUI exporter lifecycle yet.
- Headless mode still needs CI-style validation on Linux and macOS before relying on it for cluster workflows.
- In the Codex shell environment on Windows, direct CMake/Ninja invocations intermittently wedged and left stale generated metadata; prefer VS Code CMake Tools or the user's normal terminal build path for validation unless this is rechecked.
- Manual restart is still expected after IFW updates.
- macOS signing and Gatekeeper behavior still need full validation.
- Remaining runtime warnings visible from command-prompt launches need triage and reduction; current build still reports existing `FluxAnalysis.cpp` warnings C4834 and C4805.
- Release documentation should be rechecked if IFW URL paths or package IDs change; current scripts use `https://cst-modelling-tools.github.io/tonatiuhpp/{windows,linux,macos}`, while older checklist text may still mention `/ifw/...`.
- User-reported startup or normal-use segmentation faults remain unresolved. Current packaging evidence points away from missing bundled Qt libraries as the only explanation; next diagnostics should collect terminal output with Qt plugin diagnostics, Windows Event Viewer faulting module, GPU/OpenGL driver details, and whether headless scene validation succeeds on the same scene.
- Automated test coverage now includes headless CTest smoke tests plus the initial GoogleTest math unit target. Broader GoogleTest coverage, regression fixtures, GUI smoke coverage, and scientific validation tests remain pending.

## Pending Validation

- Confirm normal GUI startup still behaves as before after the headless entry-point changes.
- Confirm `tonatiuhpp --headless validate-scene` on representative plugin-based scenes on Linux and macOS.
- Confirm normal GUI startup still behaves as before after the parallel headless `trace-scene` changes.
- Migrate GUI `MainWindow::Run()` to a photon-export-aware `RayTraceRunner` path after adding explicit runner support for photon buffers, export-surface filtering, cancellation, retained-photon error propagation, and photon-power finalization.
- Confirm normal GUI startup still behaves as before after the headless `benchmark` changes.
- Confirm positional `.tnhpps` startup opens the GUI script window/editor, loads the script, and does not execute it automatically.
- Confirm Windows installed `.tnhpp` and `.tnhpps` associations use Tonatiuh++ icons in Explorer and launch the installed executable with the selected file path.
- Confirm the new headless CTest smoke tests pass on Linux/macOS build-tree runners and on Windows when configured with `TONATIUHPP_TEST_EXECUTABLE` pointing at an installed runtime.
- Expand GoogleTest beyond the initial math `Interval` coverage, then add regression fixtures, GUI smoke tests, and benchmark/scientific validation after the smoke foundation is stable.
- Confirm `trace-scene` and `benchmark` produce no photon files from the installed application output directory on representative runs.
- Run the full benchmark v1 target of 500,000,000 rays, generate the authoritative reference JSON, and preserve the resulting SHA256 and metric tolerances.
- Confirm the `v0.1.8.21` `Release` workflow succeeds on Windows, Linux, and macOS from the matching tag.
- Confirm GitHub Pages serves each generated `v0.1.8.21` IFW repository at the exact URL embedded in its installer.
- Confirm every generated repository has `Updates.xml` and package metadata for `com.tonatiuhpp.app`.
- Test update detection from the latest published IFW baseline to `v0.1.8.21` on Windows, Linux, and macOS.
- Test update installation from the latest published IFW baseline to `v0.1.8.21` on Windows, Linux, and macOS, including non-blocking startup check, `Help > Updates`, install prompt, MaintenanceTool launch, application shutdown, and manual restart.
- Reconcile or explicitly re-check release documentation if URL paths or package IDs change.

## Pre-release Checklist

- Install the latest published IFW build on Windows, Linux, and macOS as the update source.
- Verify `source/CMakeLists.txt` has the intended `0.1.8.21` version.
- Create and push the matching `v<version>` tag.
- Confirm the `Release` workflow and GitHub Pages deploy complete successfully.
- Confirm release assets include Windows, Linux, and macOS IFW installers and checksums, plus any intended manual archives.
- Verify each platform installer embeds the correct IFW repository URL and each URL serves `Updates.xml`.
- After `v0.1.8.21` release repositories are published, confirm `Help > Updates` in the installed baseline app detects `v0.1.8.21`.
- Run manual updater installation validation from the installed baseline to `v0.1.8.21` on Windows, Linux, and macOS.
