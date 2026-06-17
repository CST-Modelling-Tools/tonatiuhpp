# Tonatiuh++ v0.1.8.24 Release Notes

Tonatiuh++ v0.1.8.24 supersedes v0.1.8.23. The v0.1.8.23 artifacts were published before the final release-blocker fixes were complete, so v0.1.8.24 carries forward the v0.1.8.23 maintenance work and adds packaging/build fixes needed for a reliable Windows, Linux, and macOS release.

## Highlights

- Preserves the v0.1.8.23 runtime packaging, installed documentation, Windows launch behavior, headless workflow hardening, and GoogleTest unit-test foundation.
- Fixes macOS release staging so the final artifact contains only `TonatiuhPP.app` at the top level after runtime resources, plugins, frameworks, examples, help, and resources are moved into the app bundle.
- Hardens Linux Qt runtime validation by keeping required checks for the launcher, `qt.conf`, `libqxcb.so`, bundled `libQt6Core.so.6`, Qt plugin layout, and `ldd` resolution into the bundled `lib/` directory, while treating unavailable embedded plugin metadata strings as advisory.
- Fixes Qt IFW package script loading by declaring the required `Component` constructor and keeping `.tnhpp` and `.tnhpps` file association registration guarded to Windows installer runs.
- Hardens Windows release builds by making optional third-party DLL fallback copies skip cleanly when `third_party/_install/bin` is absent.
- Defers GoogleTest test discovery to CTest time on Windows so release builds do not need to execute freshly built unit-test binaries during the build step.
- Fixes MaintenanceTool updater launch compatibility with Qt 6.6.3 by using a `QProcess` instance for detached launch while preserving the working directory, process ID capture, and the bundled Linux Qt runtime environment.

## Validation Notes

- The v0.1.8.24 release is the current published baseline for the next release cycle.
- Windows, Linux, and macOS packages should be validated from clean installs because this release exists specifically to replace the earlier v0.1.8.23 artifacts.
- Windows installed-runtime CTest should still be run only after CMake Install refreshes the configured install prefix.
- `Help > Documentation`, file associations/icons, Linux launcher behavior, macOS app-bundle layout, and a small installed-runtime headless benchmark should be checked before tagging or publishing replacement artifacts.

## Known Issues

- User-reported startup or normal-use crash reports remain unresolved and still need platform diagnostics.
- macOS signing and Gatekeeper behavior remain separate release-hardening work.
- The full benchmark v1 reference run and authoritative reference artifacts still need to be generated and archived before treating the benchmark dataset as a complete scientific baseline.
