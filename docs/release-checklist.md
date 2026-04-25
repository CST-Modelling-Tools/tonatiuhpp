# Windows Updater Release Checklist

Use this checklist when preparing a GitHub release that must be recognized by the Tonatiuh++ Windows updater.

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
- The GitHub release tag must match the application version, with only the leading `v` added.

Example:
- App version: `0.1.8.18`
- Tag: `v0.1.8.18`

## 3. Build the Windows release asset

- Build the Windows installer from the exact tagged commit.
- Confirm the produced installer filename is exactly:

`TonatiuhPP-<version>-windows-x64.exe`

Example:

`TonatiuhPP-0.1.8.18-windows-x64.exe`

## 4. Generate the SHA-256 checksum

- Compute the SHA-256 checksum of the exact installer file to be uploaded.
- Create a checksum file named exactly:

`TonatiuhPP-<version>-windows-x64.exe.sha256`

- The checksum file content must reference the exact installer filename.

Accepted formats:

```text
<sha256>  TonatiuhPP-0.1.8.18-windows-x64.exe
```

or

```text
<sha256> *TonatiuhPP-0.1.8.18-windows-x64.exe
```

## 5. Upload GitHub release assets

Upload these assets to the GitHub release for the matching tag:

- `TonatiuhPP-<version>-windows-x64.exe`
- `TonatiuhPP-<version>-windows-x64.exe.sha256`

## 6. Verify release metadata

Confirm all of the following:

- The release belongs to:
  - `https://github.com/CST-Modelling-Tools/tonatiuhpp`
- The release page is for the correct tag:
  - `https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v<version>`
- The Windows installer asset is attached to that exact release.
- The checksum asset is attached to that exact release.
- The installer and checksum filenames match exactly.
- The checksum was generated from the uploaded installer, not a different rebuild.

## 7. Naming rules required by the updater

The current Windows updater expects:

- tag name:
  - `v<version>`
- installer filename:
  - `TonatiuhPP-<version>-windows-x64.exe`
- checksum filename:
  - `TonatiuhPP-<version>-windows-x64.exe.sha256`

Do not publish near-match names such as:

- `TonatiuhPP-<version>-windows-x64-portable.exe`
- `TonatiuhPP-<version>-setup.exe`
- old-version installers under the new release
- checksum files that reference a different filename

## 8. Supported updater behavior

Current updater behavior on Windows:

- checks the latest GitHub release
- compares installed version with the latest tag
- downloads the checksum first
- downloads the installer second
- verifies installer size
- verifies SHA-256 checksum
- offers to start the installer and close the application

Current updater does not:

- auto-install silently
- support Linux/macOS archive self-update
- accept nonstandard Windows installer filenames

## 9. Final pre-release sanity check

Before publishing, verify:

- App version in `source/CMakeLists.txt` is correct
- Git tag matches the app version
- Installer filename matches the expected pattern
- Checksum filename matches the installer filename
- Checksum content references the exact installer filename
- Assets are uploaded to the correct GitHub release

## 10. Recommended manual validation

After publishing:

- Run an older installed Windows build of Tonatiuh++
- Open `Help > Updates`
- Confirm the updater detects the new version
- Confirm it downloads and verifies the installer
- Confirm `Start Installer and Close` launches the installer successfully
