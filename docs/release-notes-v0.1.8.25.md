# Tonatiuh++ v0.1.8.25 Release Notes Draft

Tonatiuh++ v0.1.8.25 follows the published v0.1.8.24 release and is the next intended release target.

These notes remain a draft until the source version metadata is updated to 0.1.8.25, the release workflow is validated, and Windows, Linux, and macOS package checks are complete.

## Highlights

- Adds the first minimal true-headless script runner: `tonatiuhpp --headless run-script <script.tnhpps>`.
- Runs headless scripts through a `QCoreApplication` and `QJSEngine` host without `QApplication`, `SoQt::init`, `MainWindow`, `ScriptWindow`, `NodeObject`, or GUI widgets.
- Exposes a deliberately small first automation API: `print(value)`, `tn.writeJson(path, value)`, `tn.validateScene(path)`, `tn.runBenchmark(path)`, and deterministic no-export `tn.traceScene(options)`.
- Keeps legacy `tonatiuhpp -i script.tnhpps` behavior unchanged and GUI-bound for compatibility.
- Keeps `tonatiuhpp --headless -i script.tnhpps` unsupported; true non-GUI script automation uses the explicit `run-script` command.
- Adds CTest smoke coverage for missing script arguments, rejection of GUI-only script API calls, `tn.traceScene` validation failures, and a small cylinder-scene script that validates a scene, runs no-export tracing, writes JSON, and runs a benchmark.

## Validation Notes

- Windows installed-runtime CTest should still be run only after CMake Install refreshes the configured install prefix.
- Headless script smoke coverage should be run from the installed runtime on Windows and from the build-tree executable on Linux/macOS where supported.
- The release workflow should be rerun from a matching `v0.1.8.25` tag only after source metadata is bumped and platform validation is complete.

## Known Issues

- Headless `run-script` is intentionally limited. It does not support scene mutation, screenshots, GUI-compatible `MainWindow` APIs, dialogs, widget access, or photon export; `tn.traceScene` requires `noExport: true`.
- User-reported startup or normal-use crash reports remain unresolved and still need platform diagnostics.
- macOS signing and Gatekeeper behavior remain separate release-hardening work.
