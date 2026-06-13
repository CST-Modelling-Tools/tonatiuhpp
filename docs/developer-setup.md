# Developer Setup

## VS Code Settings Policy

The tracked `.vscode/settings.json` file should contain only portable workspace settings that are useful to all contributors. Do not commit machine-specific paths, local install prefixes, SDK paths, or installed executable paths there.

Keep local CMake values in an untracked `CMakeUserPresets.json` file or pass them on the command line.

## GoogleTest Unit Tests

When `BUILD_TESTING` is enabled, CMake registers unit tests with CTest through GoogleTest. Configuration first tries `find_package(GTest CONFIG QUIET)`; if no package config is found, it uses `FetchContent` to download the pinned GoogleTest `v1.14.0` release into the build tree. GoogleTest is not vendored in this repository.

For offline configuration, install GoogleTest locally and make it discoverable with `CMAKE_PREFIX_PATH`, or configure with `-DBUILD_TESTING=OFF`.

The initial unit-test target can be built and run with:

```sh
cmake -S source -B build -DBUILD_TESTING=ON
cmake --build build --target tonatiuhpp_math_tests
ctest --test-dir build --output-on-failure
```

## Windows VS Code CTest Workflow

On Windows, headless CTest smoke tests are intended to run against an installed Tonatiuh++ runtime. Keep the machine-specific configuration in `source/CMakeUserPresets.json`; this file is local to the developer machine and is ignored by Git.

For VS Code Testing with CMake Tools, create or update `source/CMakeUserPresets.json` locally:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "windows-installed-runtime-tests",
      "displayName": "Windows installed runtime tests",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/../build",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "BUILD_TESTING": "ON",
        "CMAKE_INSTALL_PREFIX": "D:/tonatiuhpp",
        "TONATIUHPP_TEST_EXECUTABLE": "D:/tonatiuhpp/bin/tonatiuhpp.exe"
      }
    }
  ]
}
```

The install prefix and test executable must agree:

- `CMAKE_INSTALL_PREFIX=D:/tonatiuhpp` installs the current build into the local runtime tree.
- `TONATIUHPP_TEST_EXECUTABLE=D:/tonatiuhpp/bin/tonatiuhpp.exe` tells CTest to run the installed executable with its adjacent Qt, Coin, SoQt, plugin, example, and resource files.

In VS Code:

1. Select the local `Windows installed runtime tests` configure preset from CMake Tools.
2. Reconfigure the project so CMake regenerates tests with the local cache values.
3. Build Tonatiuh++.
4. Run CMake Tools: `Install` to refresh `D:/tonatiuhpp`.
5. Run the tests from the Testing pane.

Installed-runtime CTest on Windows must be run after `Install`; otherwise CTest may execute a stale `D:/tonatiuhpp/bin/tonatiuhpp.exe` that does not match the current build tree tests.

The same tests can be run from a terminal after configure/install:

```sh
ctest --test-dir build --output-on-failure
```

The same value can be supplied without VS Code:

```sh
cmake -S source -B build -DBUILD_TESTING=ON -DCMAKE_INSTALL_PREFIX=D:/tonatiuhpp -DTONATIUHPP_TEST_EXECUTABLE=D:/tonatiuhpp/bin/tonatiuhpp.exe
cmake --build build --target install
ctest --test-dir build --output-on-failure
```
