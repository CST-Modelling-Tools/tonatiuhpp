# Project Status

Last updated: 2026-05-22

Purpose: lightweight handoff for current Tonatiuh++ project and release context. Keep stable agent rules in `AGENT.md`; update this file when release context changes.

## Current Priorities

- Stabilize and scale benchmark v1 on top of the new headless execution foundation without adding a second executable.
- Prepare, publish, and validate the Tonatiuh++ `v0.1.8.19` release.
- Remove or reduce remaining runtime warnings visible from command-prompt launches.
- Validate the IFW updater path from an installed `v0.1.8.18` IFW build to `v0.1.8.19` on Windows, Linux, and macOS.

## Current Baseline

- Branch: `master`.
- Current application version in `source/CMakeLists.txt`: `0.1.8.19`.
- Published updater baseline: `v0.1.8.18`; it already includes the IFW updater software.
- Intended release under validation: `v0.1.8.19`, focused on proving the updater flow from installed `v0.1.8.18` IFW builds.
- Release packaging source of truth: `.github/workflows/release.yml` and `installer/`.
- Headless execution foundation is now implemented in the existing `tonatiuhpp` executable; there is no separate console executable or installer target.
- Current headless commands: `tonatiuhpp --headless --help`, `tonatiuhpp --headless validate-scene <scene.tnhpp>`, `tonatiuhpp --headless trace-scene <scene.tnhpp> --rays N --seed S --no-export`, and `tonatiuhpp --headless benchmark <benchmark_config.json>`.

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
- Runtime noise reduction: removed stray `ShapeMesh` debug prints that emitted resolved OBJ paths and project search paths during scene loading.
- View/model stability: idempotent node sensor attaches, no nested `ParametersModel` reset, and stale `SceneTreeModel` index guards.
- Dependency bootstrap: clearer missing Eigen/Boost diagnostics and a hardened Windows dependency path.
- Installer/release flow: IFW repository release metadata rendering, `com.tonatiuhpp.app` package id normalization, GitHub Pages root platform repositories, and removal of unsupported `binarycreator` hybrid mode.
- Photon export/raytracing reliability: retained photons survive export failures, fatal export failures propagate, file export is failure-aware and sequential, and photon buffer/file split limits are honored.
- Updater path: legacy GitHub release parsing was replaced with the Qt IFW `MaintenanceTool` flow, including `check-updates` classification and user-approved updater launch.

## Current Release Workflow

- Set the version in `source/CMakeLists.txt`, then tag the release as `v<version>`; the next intended release is `v0.1.8.19`.
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

## Pending Validation

- Confirm normal GUI startup still behaves as before after the headless entry-point changes.
- Confirm `tonatiuhpp --headless validate-scene` on representative plugin-based scenes on Linux and macOS.
- Confirm normal GUI startup still behaves as before after the parallel headless `trace-scene` changes.
- Migrate GUI `MainWindow::Run()` to a photon-export-aware `RayTraceRunner` path after adding explicit runner support for photon buffers, export-surface filtering, cancellation, retained-photon error propagation, and photon-power finalization.
- Confirm normal GUI startup still behaves as before after the headless `benchmark` changes.
- Confirm `trace-scene` and `benchmark` produce no photon files from the installed application output directory on representative runs.
- Run the full benchmark v1 target of 500,000,000 rays, generate the authoritative reference JSON, and preserve the resulting SHA256 and metric tolerances.
- Confirm the `v0.1.8.19` `Release` workflow succeeds on Windows, Linux, and macOS from the matching tag.
- Confirm GitHub Pages serves each generated `v0.1.8.19` IFW repository at the exact URL embedded in its installer.
- Confirm every generated repository has `Updates.xml` and package metadata for `com.tonatiuhpp.app`.
- Test update detection from an installed `v0.1.8.18` IFW build to `v0.1.8.19` on Windows, Linux, and macOS.
- Test update installation from `v0.1.8.18` to `v0.1.8.19` on Windows, Linux, and macOS, including non-blocking startup check, `Help > Updates`, install prompt, MaintenanceTool launch, application shutdown, and manual restart.
- Reconcile or explicitly re-check release documentation if URL paths or package IDs change.

## Pre-release Checklist

- Install the published `v0.1.8.18` IFW build on Windows, Linux, and macOS as the update source.
- Verify `source/CMakeLists.txt` has the intended `0.1.8.19` version.
- Create and push the matching `v<version>` tag.
- Confirm the `Release` workflow and GitHub Pages deploy complete successfully.
- Confirm release assets include Windows, Linux, and macOS IFW installers and checksums, plus any intended manual archives.
- Verify each platform installer embeds the correct IFW repository URL and each URL serves `Updates.xml`.
- After `v0.1.8.19` release repositories are published, confirm `Help > Updates` in the installed `v0.1.8.18` app detects `v0.1.8.19`.
- Run manual updater installation validation from `v0.1.8.18` to `v0.1.8.19` on Windows, Linux, and macOS.
