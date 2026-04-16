# Qt Installer Framework skeleton

This directory contains the minimal Qt Installer Framework (Qt IFW) foundation for Tonatiuh++.

Files:
- `config/config.xml` — installer metadata and default install paths
- `packages/com.tonatiuh.app/meta/package.xml` — package metadata for the application
- `packages/com.tonatiuh.app/data/` — placeholder package payload directory
- `prepare_ifw_payload.py` — script to stage CMake install output into package data and deploy Qt runtime
- `create_installer.py` — script to generate Qt IFW installer from staged payload

Notes:
- No installer payload is populated yet.
- No updater repository or update configuration is included yet.
- Version values are currently hard-coded from `source/CMakeLists.txt` and should be automated later.
- `installscript.qs` is intentionally omitted; no custom installer scripting is required in this initial skeleton.

## Prerequisites

- Qt Installer Framework installed and `binarycreator` in PATH or specified via `--binarycreator`
- Qt deployment tools available on PATH for runtime staging:
  - Windows: `windeployqt`
  - macOS: `macdeployqt` (supported only for `.app` bundle builds)
- CMake build tree configured (e.g., `cmake -S source -B build`)
- Python 3 for running the scripts

## Staging the package payload

Run the helper script to stage CMake install output into the Qt IFW package data tree and deploy Qt runtime dependencies.

Example:

```bash
python installer/prepare_ifw_payload.py --build-dir build --config Release
```

This will:
- build the project from `build/`
- install into `build/install-staging/`
- apply Qt runtime deployment on Windows and macOS when supported
- copy the staged tree into `installer/packages/com.tonatiuh.app/data/`

The script only allows the package data target under the intended Qt IFW package path, to avoid accidental staging elsewhere.

On Linux, the staging step validates that Qt runtime libraries and platform plugins are present in the staged tree.

The staged payload is then ready for later Qt IFW packaging.

## Generating the installer locally

After staging the payload, run the installer generation script.

Examples:

### Windows
```bash
python installer/create_installer.py --binarycreator "C:\Qt\Tools\QtInstallerFramework\4.6\bin\binarycreator.exe"
```

### macOS
```bash
python installer/create_installer.py --binarycreator "/opt/Qt/Tools/QtInstallerFramework/4.6/bin/binarycreator"
```

### Linux
```bash
python installer/create_installer.py --binarycreator "/opt/Qt/Tools/QtInstallerFramework/4.6/bin/binarycreator"
```

This will:
- validate that the Qt IFW skeleton and staged payload exist
- invoke `binarycreator` to generate the installer
- place the output in `installer/output/` with a platform-specific extension (e.g., `.exe` on Windows, no extension on Unix)

## What is intentionally not yet handled

- dependency bundling beyond Qt runtime deployment
- updater repository configuration
- signing or notarization
- non-Windows IFW CI/CD automation
- maintenance tool integration
- OS-specific installer polish beyond binarycreator defaults

## GitHub Actions usage

The Windows release workflow in `.github/workflows/release.yml` reuses the local validated packaging path directly:
- configure and build Tonatiuh++ with the Visual Studio generator
- run `prepare_ifw_payload.py` with an explicit `windeployqt` path
- run `create_installer.py` with an explicit `binarycreator` path

Linux and macOS release jobs remain archive-based in the current release workflow.
