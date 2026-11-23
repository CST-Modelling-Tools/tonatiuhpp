# Tonatiuh++ v0.1.8.28 — Candidate release notes (DRAFT)

**Status: UNRELEASED CANDIDATE.** These notes summarize changes on `master` since the published `v0.1.8.27` tag. The candidate has not been performance-accepted, installer-validated, tagged or authorized for publication. The exact release-candidate commit SHA must be recorded after this version-bump PR merges.

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

## Release acceptance — NOT YET VERIFIED

- [ ] Confirm exact candidate SHA and matching `0.1.8.28` metadata, citation version, and Qt Installer Framework package versions.
- [ ] Complete Windows, Linux and macOS CI for the exact candidate.
- [ ] Complete independent scientific acceptance for intentionally changed optical behavior and document numerical differences relative to v0.1.8.27.
- [ ] Perform controlled same-machine reduced-ray performance comparisons with the previous published release using the established ABBA harness; report a PASS, confirmed regression, or inconclusive result. Do not interpret changed scientific results as a timing failure or as proof of equivalence.
- [ ] Rehearse release builds with `fake_tag=v0.1.8.28` (nonpublishing workflow), verify all installers and checksums, and exercise installed GUI/CLI/plugin behavior and updater compatibility.
- [ ] Obtain explicit maintainer authorization before creating/pushing the `v0.1.8.28` tag, which triggers publication.

**Safety:** Do not execute or regenerate the frozen 500-million-ray benchmark or overwrite its approved references as part of routine release preparation.

See `docs/release-cycle-checklist.md`, `docs/scientific-validation.md`, and `docs/precommit-performance.md` for the full acceptance procedures. Timing is not yet measured for this candidate.
