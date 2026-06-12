# Developer Setup

## VS Code Settings Policy

The tracked `.vscode/settings.json` file should contain only portable workspace settings that are useful to all contributors. Do not commit machine-specific paths, local install prefixes, SDK paths, or installed executable paths there.

Keep local CMake values in an untracked `CMakeUserPresets.json` file or pass them on the command line.

## Windows Headless CTest Runtime

On Windows, headless CTest smoke tests are intended to run against an installed Tonatiuh++ runtime. Set `TONATIUHPP_TEST_EXECUTABLE` locally so CTest can use the installed executable and its adjacent runtime files.

For VS Code Testing with CMake Tools, create `source/CMakeUserPresets.json` locally:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "windows-installed-tests",
      "displayName": "Windows installed runtime tests",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/../build",
      "cacheVariables": {
        "BUILD_TESTING": "ON",
        "TONATIUHPP_TEST_EXECUTABLE": "C:/path/to/tonatiuhpp/bin/tonatiuhpp.exe"
      }
    }
  ]
}
```

Choose that configure preset in VS Code, configure the project, and then use the Testing view or run:

```sh
ctest --test-dir build --output-on-failure
```

The same value can be supplied without VS Code:

```sh
cmake -S source -B build -DTONATIUHPP_TEST_EXECUTABLE=C:/path/to/tonatiuhpp/bin/tonatiuhpp.exe
ctest --test-dir build --output-on-failure
```
