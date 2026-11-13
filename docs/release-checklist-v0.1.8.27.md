# Tonatiuh++ v0.1.8.27 — Release acceptance checklist

**State:** candidate preparation, not an authorized public release.

**Current evidence:** all three development CI jobs passed at `7db344a7` (run `43442481832`); exact v0.1.8.27 candidate and actual installer updates are not yet accepted.

## Source and verification

- [ ] Confirm `master` clean and synchronized; record exact commit SHA.
- [ ] Check `source/CMakeLists.txt` version is `0.1.8.27` and `CITATION.cff` matches.
- [ ] Run `python installer/sync_ifw_metadata.py --check-tag v0.1.8.27 --print-version`.
- [ ] Review release notes against the commits since published `v0.1.8.26`.
- [ ] Confirm exact-candidate Windows, Linux and macOS CI results and inventories.
- [ ] Run Python automation unit tests and bounded native CLI studies on supported platforms.
- [ ] Verify installed `tonatiuhpp-cli check-updates` and `tonatiuhpp-cli update` both target the same IFW package as GUI Help > Updates; ensure a background tracing process is never silently interrupted.
- [ ] Record any scientific equivalence deviations explicitly. Do **not** run/regenerate the frozen 500M benchmark.

## Packaging and installation

- [ ] Run `.github/workflows/release.yml` manually with `fake_tag=v0.1.8.27` **before tagging**. This runs release builds without the publish job.
- [ ] Verify Windows x64 IFW installer, Linux x64 IFW installer, macOS arm64 IFW installer and corresponding SHA-256 files.
- [ ] Verify Windows installed GUI and CLI start without build-tree DLL or plugin paths, can open representative scenes, and run bounded traces / scripts.
- [ ] Verify Linux bundled Qt launcher/runtime and macOS `.app` + CLI layout on their target platforms.
- [ ] Test Windows IFW installer install, normal launch, file associations, repair/uninstall as applicable.
- [ ] Record installed file versions for `tonatiuhpp-gui`, `tonatiuhpp-cli`, official plugins, Qt/Coin runtime components, and packaged data; verify expected files are staged together.
- [ ] For an unpublished candidate, validate install/uninstall and installer repository structure offline. A true public online update requires a reachable candidate repository or the authorized public publication; do not replace the existing stable repository in place merely for a test.
- [ ] Evaluate macOS Gatekeeper/signing/notarization and actual installer transactions separately; packaging checks alone do not prove them.
- [ ] Confirm installer remote URLs use `https://cst-modelling-tools.github.io/tonatiuhpp/{windows,linux,macos}`.

## Online updater and publication

- [ ] Preserve an installed v0.1.8.26 IFW baseline for each platform before testing updates.
- [ ] Obtain explicit approval for `v0.1.8.27` tag creation and publication.
- [ ] Create and push the immutable `v0.1.8.27` tag at the approved commit; **this triggers publication**.
- [ ] Verify the release workflow, GitHub release assets/checksums, and Pages repositories (`Updates.xml`) for all three platforms.
- [ ] Test update detection and installation from v0.1.8.26 to v0.1.8.27, including restart, with either a safely isolated candidate repository or after authorized publication. Confirm CLI-initiated and GUI-initiated updating produce the same complete installed version, not just one executable.
- [ ] Finalize release announcement and any external archival/Zenodo process only with maintainer approval.

## Stop conditions

Stop publication if any release workflow platform fails, staged runtime is missing essential libraries/plugins, version/tag metadata differs, checksum verification fails, or updater behavior risks an existing installation. Never replace frozen scientific references to make a release pass.
