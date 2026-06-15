# Tonatiuh++ v0.1.8.23 Release Notes Draft

Tonatiuh++ v0.1.8.23 is planned as a maintenance release focused on runtime packaging, installed documentation, Windows launch behavior, headless workflow hardening, and the first GoogleTest unit-test foundation.

These notes remain a draft until the release workflow is validated and Windows, Linux, and macOS package checks are complete.

## Highlights

- Hardened Linux runtime packaging by bundling a self-consistent Qt runtime and plugin set, installing a launcher script, writing `qt.conf`, and validating that the bundled Qt libraries and plugins match the packaged runtime.
- Fixed Linux in-app updater launch so `MaintenanceTool` starts with the same bundled Qt runtime and plugin environment as the installed Tonatiuh++ launcher.
- Wired Sphinx runtime help generation into the CI and release workflows so packaged builds include `help/html`, and refreshed runtime help branding for Tonatiuh++.
- Improved Windows launch behavior for installed file associations: positional `.tnhpp` project files open on GUI startup, and positional `.tnhpps` script files open in the GUI script window/editor without automatic execution.
- Embedded the Tonatiuh++ executable icon on Windows and registered `.tnhpp` and `.tnhpps` file types with Tonatiuh++ ProgIDs pointing at the installed executable icon.
- Standardized headless `trace-scene` and `benchmark` console output with key-value fields for scene path, rays, seed, photon-export state, elapsed time, throughput, scheduling fields, and result/output paths where applicable.
- Added CTest smoke coverage for headless help, invalid headless arguments, scene validation, no-export trace, and benchmark execution on the plugin-free cylinder fixture.
- Added the first GoogleTest unit-test foundation, covering deterministic `Interval` and `Box2D` math behavior while keeping GUI, Qt widget, Coin, SoQt, plugin, and ray-tracing behavior unchanged.
- Documented the Windows VS Code installed-runtime CTest workflow, including local `source/CMakeUserPresets.json`, `CMAKE_INSTALL_PREFIX=D:/tonatiuhpp`, `TONATIUHPP_TEST_EXECUTABLE=D:/tonatiuhpp/bin/tonatiuhpp.exe`, and the requirement to install before running CTest.

## Validation Notes

- Windows installed-runtime CTest should be run only after CMake Install refreshes the configured install prefix; otherwise tests may execute a stale installed executable.
- Linux and macOS artifacts still need clean-machine launch validation before tagging.
- `Help > Documentation` should be checked on every platform to confirm the packaged `help/html/index.html` opens correctly.
- A small installed-runtime headless benchmark should be run on every supported platform before release.

## Known Issues

- User-reported startup or normal-use crash reports remain unresolved. Current packaging work reduces likely runtime-deployment causes, but final diagnosis still needs terminal output with Qt plugin diagnostics, OS crash details, GPU/OpenGL driver details, and confirmation of whether headless scene validation succeeds on the same scene.
- macOS signing and Gatekeeper behavior remain separate release-hardening work.
- The full benchmark v1 reference run and authoritative reference artifacts still need to be generated and archived before treating the benchmark dataset as a complete scientific baseline.
