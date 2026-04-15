# Qt Installer Framework skeleton

This directory contains the minimal Qt Installer Framework (Qt IFW) foundation for Tonatiuh++.

Files:
- `config/config.xml` — installer metadata and default install paths
- `packages/com.tonatiuh.app/meta/package.xml` — package metadata for the application
- `packages/com.tonatiuh.app/data/` — placeholder package payload directory

Notes:
- No installer payload is populated yet.
- No updater repository or update configuration is included yet.
- Version values are currently hard-coded from `source/CMakeLists.txt` and should be automated later.
- `installscript.qs` is intentionally omitted; no custom installer scripting is required in this initial skeleton.

## Staging the package payload

Run the helper script to stage CMake install output into the Qt IFW package data tree.

Example:

```bash
python installer/prepare_ifw_payload.py --build-dir build --config Release
```

This will:
- build the project from `build/`
- install into `build/install-staging/`
- copy the staged tree into `installer/packages/com.tonatiuh.app/data/`

The script only allows the package data target under the intended Qt IFW package path, to avoid accidental staging elsewhere.

The staged payload is then ready for later Qt IFW packaging.
