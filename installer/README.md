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
