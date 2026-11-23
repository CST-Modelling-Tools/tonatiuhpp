# Tonatiuh++ v0.1.8.28 — Release notes

These notes summarize changes since the published `v0.1.8.27` tag. The v0.1.8.28 release was authorized by the maintainer on 2026-11-23 after full-scale scientific numerical acceptance, three-platform packaging rehearsal, and Windows installed GUI/CLI/plugin tests. The release-to-release timing comparison remains formally **INCONCLUSIVE / UNSTABLE ENVIRONMENT** despite a consistent observed runtime improvement. Linux/macOS installed-runtime and updater compatibility checks were not completed before the maintainer's explicit decision to publish.

## Scientific behavior and material models

- Enforce exact launched-ray termination accounting and explicit energy-budget outcome categories. Trace failures and cancellations are distinct from legitimate physical outcomes.
- Replace ambiguous material Boolean continuation behavior with kernel-owned `MaterialRT::interact()` and explicit scattering/probability contracts. Revise event selection, atmospheric-segment handling and supported material implementations. These changes can intentionally change ray paths and numerical output compared with v0.1.8.27; they require independent scientific validation rather than forced bitwise equality to the old version.
- Remove four unused legacy material implementations that relied on an obsolete material API. They were not part of the officially built material set; see `docs/legacy-material-models.md` for their historical formulas and limitations.

## Application behavior and architecture

- Harden scientific plugin ownership, registry and execution lifecycle boundaries.
- Improve scene-tree ownership and GUI editing/Undo/Redo lifecycle handling, including a fix for an invalid transform downcast in `MainWindow::SetValue` and native Fresnel scene regression coverage.
- Remove production C++ lambda expressions and introduce a repository-owned lexical policy audit in cross-platform CI. The scanner is a conservative syntax screen, not a compiler AST check.

## Testing and engineering

- Add and refine scientific, architectural, GUI and CLI tests; use behavior-based test and artifact naming.
- Expand maintainer documentation and adopt a release-before-development-cycle-switch policy with a mandatory previous-release performance comparison.

## Full-scale scientific acceptance evidence (2026-11-23)

The maintainer ran the canonical 500-million-ray heliostat scene on the Windows Release candidate (seed `123456789`, 16 workers, 50,000 chunks of 10,000 rays, 100 x 100 front-side receiver). Execution completed in 1574.168 seconds (317628.106 rays/s). An independent file-level check verified the generated binary SHA-256, binary/CSV cell equality, finite nonnegative values, reconstructed statistics and integrated power. The candidate collected 42.20696058499865 MW with peak flux 19.48813968933321 MW/m2: relative differences of -0.023904% and -0.189796% versus the immutable reference, within existing 0.1% and 5% numerical tolerances.

The candidate grid SHA-256 is `c92f5e2bd4ef664707dd7b135e20e283db7fcd0067b01c63a8ec65b57989adab`, **not** the historical exact-grid hash. The user-requested cellwise comparison found correlation 0.99996308 and integrated absolute grid difference 0.768261% of reference power, with no preapproved spatial-error PASS threshold. This is a **full-scale numerical/output-integrity PASS**, not a PASS of the unchanged historical exact-grid auditor or universal physical correctness. See `docs/release-acceptance-v0.1.8.28.md` for complete evidence.

## Release acceptance and documented limitations

- [x] Version `0.1.8.28` confirmed in CMake and `CITATION.cff`; metadata compatibility was checked by the successful rehearsal's `sync_ifw_metadata.py --check-tag` steps. The precise release commit is the Git commit associated with tag `v0.1.8.28`. The existing Zenodo DOI `10.5281/zenodo.20395329` was carried forward; its concept-versus-version meaning is not established by these checks.
- [x] Development CI succeeded on Windows, Linux and macOS for source checkpoint `7c2ccfff751d9e84eb86b45d8d5ca0e5e45db02e`, PR #67 head, and its merged `master` commit `1bbef1ee6924b38106f4d16132303c2529db387a`.
- [x] Windows Release CTest passed 207/207; full-scale 500M numerical/output-integrity checks passed and all 10,000 cells were compared. Historical exact-grid equality differs as expected under changed-physics sampling. This does not establish universal physical correctness; the maintainer authorized publication with the documented validation evidence.
- [x] Perform 5M, 50M and 75M same-machine ABBA comparisons with v0.1.8.27. All verdicts are **INCONCLUSIVE / UNSTABLE ENVIRONMENT**; observed paired runtime changes are approximately -18%, -20.52% and -20.04%. The maintainer explicitly chose to close additional timing investigation while acknowledging the unresolved stability risk; do not call this a formal performance PASS.
- [x] Nonpublishing `fake_tag=v0.1.8.28` release rehearsal [#45238381151](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45238381151) passed Windows x64, Linux x64, macOS ARM64 packaging and uploaded installer/IFW-repository artifacts; the publish job was skipped. The maintainer confirmed successful Windows installation and installed GUI, CLI and official `MaterialSpecular` plugin behavior. Independent checksum verification of downloaded assets, installed-runtime tests on Linux/macOS, and updater compatibility remain unverified; publication was explicitly authorized with those limitations.
- [x] The maintainer explicitly authorized official publication on 2026-11-23. The `v0.1.8.28` tag triggers GitHub Actions release packaging and publication; the tag-run must still be checked for successful publication, installer checksums and deployed update repositories.

**Safety:** A fresh 500M **candidate** run is now an explicit manual release gate. Never regenerate or overwrite the immutable historical 500M scene or reference outputs.

See `docs/release-cycle-checklist.md`, `docs/scientific-validation.md`, and `docs/precommit-performance.md` for the full acceptance procedures. Timing has been measured; formal stability criteria were not met.
