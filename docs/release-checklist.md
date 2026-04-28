# IFW Release Checklist

Use this checklist when preparing a Tonatiuh++ release that must be visible to the Qt Installer Framework MaintenanceTool.

## 1. Set the application version

- Update `source/CMakeLists.txt`:
  - `project(TonatiuhPP VERSION X.Y.Z.W LANGUAGES CXX)`
- Ensure the version matches the intended release exactly.

Example:
- App version: `0.1.8.18`
- Tag: `v0.1.8.18`

## 2. Create the release tag

- Create and push the Git tag in this format:
  - `vX.Y.Z`
  - or `vX.Y.Z.W` if using four components
- The tag must match the application version, with only the leading `v` added.
- Pushing a matching tag to GitHub triggers the repository `Release` workflow.

## 3. Build release payloads

Preferred path:

- Let the GitHub Actions `Release` workflow build all release payloads from the exact tagged commit.

The workflow currently publishes GitHub release distribution assets and IFW online repositories:

- Windows IFW installer and checksum
- Linux IFW installer and checksum
- macOS IFW installer and checksum
- Linux archive and checksum
- macOS archive and checksum
- IFW repository: `ifw/windows`
- IFW repository: `ifw/linux`
- IFW repository: `ifw/macos`

The official update-capable distribution path on every platform is the IFW installer. Archive assets may remain available as manual convenience packages, but they are not the updater-enabled installation path.

## 4. Verify IFW repositories

Confirm GitHub Pages contains all platform repositories:

- `https://cst-modelling-tools.github.io/tonatiuhpp/ifw/windows/Updates.xml`
- `https://cst-modelling-tools.github.io/tonatiuhpp/ifw/linux/Updates.xml`
- `https://cst-modelling-tools.github.io/tonatiuhpp/ifw/macos/Updates.xml`

Each repository must contain:

- `Updates.xml`
- generated IFW package data for `com.tonatiuh.app`
- package metadata for the release version

## 5. Verify installer update configuration

Each IFW installer must point to its platform repository:

- Windows installer: `https://cst-modelling-tools.github.io/tonatiuhpp/ifw/windows`
- Linux installer: `https://cst-modelling-tools.github.io/tonatiuhpp/ifw/linux`
- macOS installer: `https://cst-modelling-tools.github.io/tonatiuhpp/ifw/macos`

The application does not implement GitHub release download logic for updates. It asks the installed IFW MaintenanceTool to check and apply updates.

## 6. Verify GitHub release distribution assets

GitHub release assets are for distribution and manual download, not for updater metadata.

Confirm the release belongs to:

- `https://github.com/CST-Modelling-Tools/tonatiuhpp`

Confirm the release page is for the correct tag:

- `https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v<version>`

Confirm uploaded assets and checksum files match the workflow output for the tagged commit.

## 7. Final pre-release sanity check

Before publishing, verify:

- App version in `source/CMakeLists.txt` is correct
- Git tag matches the app version
- Release workflow completed successfully
- GitHub Pages deploy completed successfully
- All three IFW platform repositories contain `Updates.xml`
- Windows, Linux, and macOS IFW installers are attached to the GitHub release
- Distribution assets are uploaded to the correct GitHub release

## 8. Recommended manual validation

After publishing:

- Install an older Tonatiuh++ build made with IFW repository configuration on each platform.
- Start Tonatiuh++.
- Confirm startup update checking is non-blocking.
- Open `Help > Updates`.
- Confirm the MaintenanceTool detects the new version.
- Click `Install Updates`.
- Confirm Tonatiuh++ prompts for unsaved work, starts the MaintenanceTool, and closes before the update runs.
