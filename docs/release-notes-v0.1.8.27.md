# Tonatiuh++ v0.1.8.27 — Release candidate notes

This release contains the changes since v0.1.8.26. Publication was approved by the maintainer following successful cross-platform CI, successful three-platform installer packaging, and a verified Windows in-place upgrade. End-to-end installation and upgrade transactions on Linux and macOS were deferred and have not been manually verified.

## Highlights

- **External Python automation** (`tonatiuhpp_automation`, Python 3.10+): run the production CLI from Python, validate scenes, configure independent optical-study cases, set sun positions, update trackers, and trace scenes. Studies can obtain intercepted power and receiver flux distributions through existing C++ scientific services, with structured results, reproducibility checks, and failure diagnostics. Python remains an external controller; the C++ ray tracer and the existing JavaScript scripting host are retained.
- **GUI and CLI architecture**: share the production scientific kernel and runtime services while keeping separate `tonatiuhpp-gui` and `tonatiuhpp-cli` executables. Architecture and runtime validation were strengthened across build-tree and packaged configurations.
- **Testing and CI improvements**: reorganized and hardened CTest, cross-interface scientific validation, GUI/CLI execution checks, and Windows/Linux/macOS CI. Scientific benchmarks and protected reference outputs remain unchanged.
- **Source cleanup**: removed obsolete Doxygen scaffolding and disconnected legacy SunPath/WeatherView code while retaining the active sun-position implementation.
- **Windows build maintenance**: migrated MSVC development-environment setup away from the deprecated `ilammy/msvc-dev-cmd` GitHub Action.

## Unified GUI and CLI software updates

- The installed `tonatiuhpp-cli` now provides `check-updates` and `update` commands, backed by the same Qt Installer Framework MaintenanceTool used by the GUI. An update targets the entire installer-managed application payload: GUI, CLI, shared libraries, official plugins, and packaged resources.
- Update installation is opt-in; it is never initiated implicitly by a trace, script, Python automation job, or ordinary CLI invocation. `check-updates` never installs. `update` hands off to the interactive MaintenanceTool if an update is available; a noninteractive unattended upgrade is not promised.
- The commands require an IFW-managed installation with a discoverable MaintenanceTool. Build-tree and standalone archive executions are not self-updating. Update transactions and process shutdown/restart still require real installer acceptance before release. See [CLI updates](cli-updates.md).

## Python automation limitations

- Automated execution uses a locally available `tonatiuhpp-cli` binary and its runtime dependencies.
- Photon and flux results are calculated by native Tonatiuh++ services; timing, ray counts, and worker statistics must not be interpreted as scientific output.
- Current study-level interception and flux facilities have distinct trace/seed semantics, documented in `docs/python-automation.md`.
- This release does not introduce embedded Python, weather-data processing, annual-performance modeling, or an optimization solver.

## Validation and release gates

Maintainer-reported local Windows validation of the unified updater milestone: **184/184 CTests passed**, including `headless.update_commands`, **19 Python automation unit tests passed** (one POSIX-only test skipped), and **3/3 dedicated updater CLI smoke tests** passed. The fixed CI inventory checks on commit `7db344a7f3870bf2fd6beb0b0dfc1fcbcd591f03` passed on Windows, Ubuntu and macOS (GitHub Actions run `43442481832`). These results establish the development baseline, not validation of the version-bumped release candidate or a real IFW update transaction.

**Release acceptance evidence:** development CI passed on Windows, Ubuntu 24.04, and macOS for commit `030df753b6b380f206208a7177ca25bf93adce48` (run `43460739436`). The nonpublishing release rehearsal passed for Windows, Linux x64, and macOS arm64 (run `43488353429`), producing all three installer assets and matching IFW repository artifacts; publication was skipped. The Windows v0.1.8.26 IFW MaintenanceTool successfully detected the isolated v0.1.8.27 repository and upgraded the installed package in place. Post-upgrade checks confirmed the new GUI and CLI executables, removal of the obsolete GUI binary, required shared libraries and resources, working GUI launch, CLI scene validation, and a fixed-seed 1,000-ray trace. **Linux and macOS end-to-end installer transactions and in-place upgrades were not manually tested and remain deferred.** Packaging success must not be interpreted as verified upgrade compatibility on those platforms. Signing/notarization, if required by distribution policy, must be reviewed separately.

The frozen 500-million-ray benchmark and its approved reference assets are **not** part of routine release-candidate testing and must not be regenerated.

## Documentation

- [Python automation](python-automation.md)
- [Headless CLI](headless-benchmark.md)
- [Testing guidance](testing.md)
- [CLI updater usage](cli-updates.md)
- [Release acceptance checklist](release-checklist-v0.1.8.27.md)
