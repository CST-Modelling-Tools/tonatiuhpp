# Tonatiuh++ v0.1.8.21 Release Notes Draft

Tonatiuh++ v0.1.8.21 is a maintenance release focused on post-v0.1.8.20 usability fixes and macOS release hardening.

## Highlights

- Improved the Properties panel so expanded and collapsed property groups are preserved across scene-component selections when matching property paths exist.
- Expanded high-value transform-related property groups by default on first display, including transform, translation, rotation, scale, and scaleFactor.
- Improved the default property-name column width so common property and group names are readable without manual resizing.
- Fixed macOS 3D-view Shift-drag translation by adding the missing macOS navigation path.
- Hardened macOS app packaging around the canonical `TonatiuhPP.app` bundle, including bundle layout, `Info.plist`, executable, and bundled Mach-O dependency validation.
- Made macOS release artifacts explicitly target Apple Silicon / arm64 Mac hardware and label the macOS assets as arm64.
- Updated the About dialog links to list the current Tonatiuh++ repository and the AI-HPC4CST Project website.

## Notes

- This release does not add Intel macOS or universal2 artifacts.
- macOS signing and notarization remain separate release-hardening work.
- Headless execution and benchmark v1 behavior remain based on the v0.1.8.20 release infrastructure.
