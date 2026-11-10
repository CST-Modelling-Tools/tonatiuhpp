# Developer setup and tests

## Prerequisites and configuration

Tonatiuh++ uses CMake and C++20, Qt 6, Coin3D, SoQt, Eigen, and other dependencies configured by the repository. The source of truth for exact dependencies and versions is `source/CMakeLists.txt`, `scripts/build_deps.py`, and `.github/workflows/ci.yml`.

Keep personal SDK locations and installation directories outside tracked `.vscode/settings.json`: use a local, untracked `CMakeUserPresets.json` or CMake command-line cache arguments instead. Do not edit third-party dependencies or the scientific reference files just to get a build to pass.

## Windows (VS Code / PowerShell)

From the repository root, with the project's dependencies already configured:

```powershell
cmake -S source -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Use your existing configured `build` directory when it already works; reconfiguring is not required for documentation-only changes. A clean configure may download GoogleTest through CMake `FetchContent` unless a compatible local `GTest` package is available.

For targeted scientific/headless tests:

```powershell
ctest --test-dir build -C Release -R '^(scientific[.]|headless[.])' --output-on-failure
```

CTest labels overlap; use the full CTest run for an authoritative complete result.

## Linux / macOS

The CI workflow configures a Release Ninja build and runs CTest on each platform. Developer configuration is equivalent once the local dependencies and platform-specific packages are installed:

```sh
cmake -S source -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## GUI, standalone headless, and installation

The normal build produces the GUI executable and the independent `tonatiuhpp_m5_core_headless` program. The independent headless executable uses the same scientific targets; it is installed alongside the GUI executable and official plugins.

Build-tree tests are **not** a substitute for installed-runtime or real interactive GUI testing. If installed-runtime validation is needed, install to a disposable directory and verify both binaries, native dependencies, plugin discovery, GUI startup and shutdown, and export behavior. Do not install over a working copy during routine refactoring.

For delivery configurations, CMake uses `-DBUILD_TESTING=OFF` to exclude test-only binaries and the opt-in native GUI acceptance driver. The release workflow already passes this flag. No public release is planned as part of routine code cleanup.

See [Scientific validation](scientific-validation.md) for regression and reference-integrity checks, and [Continuous integration](change-aware-ci.md) for the hosted test policy.
