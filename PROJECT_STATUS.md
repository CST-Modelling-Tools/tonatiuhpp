# Project Status

Last updated: 2026-05-13

Purpose: lightweight handoff for current Tonatiuh++ project and release context. Keep stable agent rules in `AGENT.md`; update this file when release context changes.

## Current Priorities

- Prepare the next Tonatiuh++ release.
- Remove or reduce remaining runtime warnings visible from command-prompt launches.
- Validate IFW installer/update behavior on Windows, Linux, and macOS.

## Current Baseline

- Branch: `master`.
- Application version: `0.1.8.18` from `source/CMakeLists.txt`.
- HEAD context: `v0.1.8.18-5-g38534050`.
- Release packaging source of truth: `.github/workflows/release.yml` and `installer/`.

## Recent Completed Milestones

- View/model stability: idempotent node sensor attaches, no nested `ParametersModel` reset, and stale `SceneTreeModel` index guards.
- Dependency bootstrap: clearer missing Eigen/Boost diagnostics and a hardened Windows dependency path.
- Installer/release flow: IFW repository release metadata rendering, `com.tonatiuhpp.app` package id normalization, GitHub Pages root platform repositories, and removal of unsupported `binarycreator` hybrid mode.
- Photon export/raytracing reliability: retained photons survive export failures, fatal export failures propagate, file export is failure-aware and sequential, and photon buffer/file split limits are honored.
- Updater path: legacy GitHub release parsing was replaced with the Qt IFW `MaintenanceTool` flow, including `check-updates` classification and user-approved updater launch.

## Current Release Workflow

- Set the version in `source/CMakeLists.txt`, then tag the release as `v<version>`; the current version is `0.1.8.18`.
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

- Manual restart is still expected after IFW updates.
- macOS signing and Gatekeeper behavior still need full validation.
- Remaining runtime warnings visible from command-prompt launches need triage and reduction.
- Release documentation should be rechecked if IFW URL paths or package IDs change; current scripts use `https://cst-modelling-tools.github.io/tonatiuhpp/{windows,linux,macos}`, while older checklist text may still mention `/ifw/...`.

## Pending Validation

- Confirm the `Release` workflow succeeds on Windows, Linux, and macOS from a matching tag.
- Confirm GitHub Pages serves each generated IFW repository at the exact URL embedded in its installer.
- Confirm every generated repository has `Updates.xml` and package metadata for `com.tonatiuhpp.app`.
- Test updating from an older IFW install on Windows, Linux, and macOS, including non-blocking startup check, `Help > Updates`, install prompt, MaintenanceTool launch, application shutdown, and manual restart.
- Reconcile or explicitly re-check release documentation if URL paths or package IDs change.

## Pre-release Checklist

- Verify `source/CMakeLists.txt` has the intended version.
- Create and push the matching `v<version>` tag.
- Confirm the `Release` workflow and GitHub Pages deploy complete successfully.
- Confirm release assets include Windows, Linux, and macOS IFW installers and checksums, plus any intended manual archives.
- Verify each platform installer embeds the correct IFW repository URL and each URL serves `Updates.xml`.
- Run manual updater validation from older IFW installations on Windows, Linux, and macOS.
