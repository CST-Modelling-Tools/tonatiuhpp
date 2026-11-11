# Project status

Last updated: 2026-11-11

**Active work:** Five-phase testing-architecture rationalization and acceptance planning. This is maintenance, not a public release or a new scientific model.

**This document is the current handoff.** The old 2026-11-10 P0/P1/P2 *candidate/pending* diaries and runtime-refactor plan have been superseded by merged code; their full text remains in Git history before this status update. Do not resume those old plans or mistake historical "pending" entries for current work. Read `AGENT.md` before making changes.

## Verified code and validation baseline

- Repository: `CST-Modelling-Tools/tonatiuhpp`; default branch `master`. Starting baseline for this plan: `01400118f50557ae82883b523bb42905a2970c11` (PR #41 merge, 2026-11-11).
- PR #39 (M0-M5 shared scientific engine), PR #40 (permanent GUI/CLI entrypoints, shared runtime, large File-export limit correction) and PR #41 (test rationalization) are **merged**. PR #40 merge: `edf301a354cdec36c61f9d5c880c016fe7784528`; PR #41 merge: `01400118f50557ae82883b523bb42905a2970c11`.
- Production executables are `tonatiuhpp-gui` (Qt Widgets/SoQt graphical application) and `tonatiuhpp-cli` (QCoreApplication command-line application). They share one scientific kernel and the same production simulation/scripting services. The CLI does **not** accept `--headless`; the GUI rejects it. Retain the two-executable architecture. A dynamically loaded GUI module was evaluated and rejected for current requirements because its usability benefit does not justify the extra loading, cross-platform deployment and service-ownership complexity.
- Important compiled targets: `TonatiuhppKernel`, `TonatiuhppLibraries`, `TonatiuhSimulation`, `TonatiuhScripting`, `TonatiuhScriptRuntime`, `TonatiuhppGuiPresentation`. The headless/kernel dependency boundary excludes Qt Widgets, Qt PrintSupport and SoQt; some shared code still intentionally uses QtGui/Qml.
- PR #41 removed nine obsolete milestone-specific CTests (189 -> 180), deleted ten unused test-only files, made scientific tests link the production-compiled simulation/scripting services rather than compile 15 duplicated application sources, and corrected CLI-process evidence metadata. No production scientific implementation changed.
- **Local maintainer evidence:** Windows MSVC Release full CTest result `180/180 passed, 0 failed` after the second PR #41 improvement, 70.61 seconds elapsed. This establishes a tested Windows build; it is not a universal performance measurement.
- **Hosted evidence:** PR #41 GitHub Actions CI run `43012836153` completed successfully, including `build (windows-latest)`, `build (ubuntu-24.04)` and `build (macos-latest)`. Link: https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43012836153 . Passing build-tree CI does **not** prove every installer, clean installed-runtime GUI path, or future GUI automation works.
- Existing accepted frozen 500-million-ray Windows heliostat benchmark (M5/PR #39) and its immutable scientific reference remain valid for their documented scope. **Do not rerun 500 million rays**, rewrite benchmark artifacts, alter RNG/seed behavior, tracker policy, photon binary format, ray-propagation physics or flux/power normalization in testing-infrastructure work. See `docs/scientific-validation.md` for exact reference hashes and historical acceptance.

## Testing landscape audited on the PR #41 baseline

- `source/CMakeLists.txt` calls `include(CTest)` and includes the test tree for `BUILD_TESTING=ON`. `tests/CMakeLists.txt` (570 lines on the baseline) loads Google Test and registers most CLI/integration tests; scientific and unit GoogleTests use `gtest_discover_tests()`.
- **180 registered CTests:** 143 GoogleTest cases (130 scientific plus 13 mathematical/random unit tests), plus 37 other CTest entries for CLI/process/architecture validation. The last reported overlapping CTest labels were: `architecture=2`, `equivalence=13`, `headless=36`, `scientific=144`, `smoke=37`, `unit=13`. Label totals are **not additive** and the `smoke` label currently covers some long scientific integration scenarios, so it is not a trustworthy fast tier.
- CTest is the intended unified test executor, while Google Test is the C++ assertion/fixture framework. Keep both. Python `unittest` is appropriate for testing Python CI/comparator/auditor tools; these checks are currently invoked separately by `.github/workflows/ci.yml` and are not all part of the 180-test CTest inventory.
- `tests/CMakeLists.txt` registers CMake-driven multi-process and output-comparison tests, Python subprocess tests, direct CLI commands, and conditional tests whose registration depends on fixtures or Python availability. Python is initially searched with `QUIET` but subsequently required: classify each requirement clearly and never silently omit mandatory coverage.
- Remaining milestone-named directory `tests/m5/` contains exactly: `M5GuiNativeAcceptance.cpp`, `M5GuiNativeAcceptance.h` and `m5_2_adapter_acceptance.py`. The Python tool now compares **two independent CLI processes**, not GUI versus CLI; its `gui_artifact` only verifies GUI presence. Do not misrepresent this as native GUI equivalence.
- The GUI native acceptance helper is compiled into `tonatiuhpp-gui` under `BUILD_TESTING` and activated only by environment variables such as `TONATIUHPP_M5_GUI_NATIVE_REPORT` and `TONATIUHPP_M5_GUI_NATIVE_MODE`. Its three current modes are `interactive-file`, `seeded-interception` and `interactive-flux`. It actually drives GUI dialogs and the Script Editor but is **not** registered as routine CTest. Numerical acceptance should require independent reference/comparator evidence, not merely nonempty or positive output.
- `tests/scientific/GuiSunSizingDiagnostic.cpp` is an opt-in diagnostic, not regular unattended acceptance. `tests/soqt_smoketest/` is a separate standalone CMake project, not a regular registered CTest. Do not count either as ordinary CI coverage.
- `tests/cmake/` contains multiple historical `run_m3_*`, `run_m4_*` and `run_heliostat_benchmark_m5.cmake` drivers. Some use carefully guarded disposable paths before `file(REMOVE_RECURSE)`; others do not have the same safety guard. Process-launching, output naming, timeouts, failure diagnostics and cleanup policies are inconsistent.
- Build-tree success does not prove installed GUI and CLI paths. `installer/prepare_ifw_payload.py`, `.github/workflows/release.yml` and the existing `m5_2_adapter_acceptance.py --mode installed` logic provide foundations for installed-runtime validation; evaluate rather than recreate.
- `.github/workflows/ci.yml` runs a three-OS native Release build plus CTest for non-Markdown changes and validates Python tooling separately. Markdown-only changes use its tested classifier and can skip expensive native builds; a new tests policy must document this distinction.
- No evidence was collected in this planning handoff that installed GUI/CLI acceptance is uniformly automated on all three operating systems. Treat that as an open coverage gap rather than a failure of the existing unit/scientific tests.

## Approved FIVE-PHASE implementation plan

**Working principle:** Five coherent engineering phases, **not** five mandatory commits or five micro-PRs. Preserve all meaningful scientific coverage. Finish a phase as a useful whole before polishing peripheral details. No changes to physics or benchmark reference data. Maintain this status at meaningful checkpoints with exact evidence and state (local / branch / PR / merged).

### Phase 1 / A — Testing inventory, policy and classification

**Goal:** Make the testing contract and coverage discoverable before reorganizing files.

**Deliverables:** Author `docs/testing.md` as the current testing policy; produce or document a reproducible CTest inventory (prefer `ctest --show-only=json-v1`, with a small helper only if it materially helps). Include test names, owners/domain, level, runtime tier, execution interface, fixture/reference, platform prerequisites, deterministic/approximate assertions, and whether execution is build-tree, GUI or installed. Explicitly inventory separately executed Python `unittest` and standalone developer diagnostics.

Define a consistent label taxonomy: level (`unit`/`integration`/`system`), domain (for example `scientific`/`scripting`/`export`/`gui`/`architecture`), and tier (`fast`/`regression`/`extended`). Distinguish same-process component checks, CLI repeatability, numerical reference validation and real GUI/CLI comparison. Establish an unambiguous small quick-run command and the authoritative full regression command. Ensure required coverage is not confused with optional diagnostics.

**Gate:** Current 180-test baseline is accounted for on the reference Windows build; additional non-CTest checks are identified, labels are checked against actual discovery, and no test scenario or scientific evidence is dropped. **State at handoff: audited, implementation not started.**

### Phase 2 / B — Test layout and CMake modularization

**Goal:** Organize tests by enduring behavior instead of M3/M4/M5 milestone labels.

**Deliverables:** Split the large `tests/CMakeLists.txt` into coherent, function-oriented subdirectories (e.g. `unit`, `scientific`, `integration/cli`, `integration/scripting`, `integration/export`, `integration/gui`, `architecture`, `support`, `fixtures`). Relocate the three `tests/m5/` survivors into their correct GUI/CLI areas. Review remaining historical driver/fixture names and only rename where doing so improves stable comprehension and all references can be updated safely. Retain established scientific fixture contents, report schemas, script APIs, generated artifact formats, baseline paths and output semantics.

**Gate:** Fresh configure/build succeeds; 180 existing scenarios remain discoverable and pass with no silent skips (a changed count needs explicit scenario-level justification); all affected file paths, CMake generator expressions, Windows DLL staging, Python references, docs and CI agree. Validate on Windows/Ubuntu/macOS before merging the structural change. **State: not started.**

### Phase 3 / C — Test infrastructure reliability and reporting

**Goal:** Make test execution safe, deterministic, diagnosable and parallel-safe.

**Deliverables:** Consolidate duplicate subprocess/exit-code/output-management logic **only where maintenance gains justify it**; keep simple CMake drivers simple and retain Python for substantial structured comparisons. Use disposable test-root directories under the build tree; validate cleanup targets (including symlinks/path traversal), avoid accidental shared-output races, enforce process-level and CTest-level timeouts, and preserve stdout/stderr plus useful artifacts for failures. Remove ambiguous conditional registration for mandatory scenes/dependencies. Rationalize `smoke` vs `fast` semantics, and generate CI-readable CTest JUnit/summary artifacts where supported. Test the harness itself with deliberately failing cases.

**Gate:** Full suites succeed both sequentially and with bounded parallelism, failure injection yields correct nonzero results, cleanup refuses unsafe paths, no mandatory test silently disappears, and all three CI platforms pass. **State: not started.**

### Phase 4 / D — Genuine GUI scientific acceptance

**Goal:** Exercise real `tonatiuhpp-gui` flows as distinct from CLI reproducibility.

**Deliverables:** Wrap the existing native GUI helper in bounded process-level CTest drivers with isolated temporary output, reliable reports and explicit shutdown. Initially cover its three real GUI modes: interactive File export, seeded Script Editor interception, and interactive Flux Analysis. Validate generated numerical artifacts against independently trusted scientific expectations/comparators where applicable, not only against duplicate GUI/CLI executions or "positive output". Guard test-only behavior so production `BUILD_TESTING=OFF` binaries remain free of acceptance machinery.

**Gate:** A real display/Qt/SoQt/OpenGL environment is confirmed for each platform before making GUI checks required. Bounded timeouts, real widget interaction, independent evidence and no frozen 500M run. Add platform-specific CI gating only after reliable execution is proven; do not claim all three GUI environments are functional without actual evidence. **State: not started.**

### Phase 5 / E — Installed-runtime and packaging acceptance

**Goal:** Verify that what users install works independently of the source/build tree.

**Deliverables:** Integrate disposable CMake-install / IFW-payload checks with direct installed `tonatiuhpp-cli` and graphical `tonatiuhpp-gui` acceptance. Reuse the existing CLI acceptance `installed` mode after checking its assumptions. Ensure scientific plugins, Qt platform plugins, SoQt/Coin runtime dependencies, macOS `.app`/Frameworks layouts, Linux RPATH and Windows DLL discovery function without borrowing the development SDK or build-directory paths. Distinguish installed-runtime smoke checks from comprehensive installer/upgrade testing; only claim what has actually been executed.

**Gate:** External-prefix/packaged installations pass bounded CLI and available real GUI scenarios, are relocated or launched outside source/build contexts, and pass platform-appropriate Windows/Linux/macOS checks. No modifications to user installation locations; no release authorization implied. **State: not started.**

## Execution and Git checkpoint policy (owner decision 2026-11-11)

- **Use milestone mode, not micro-step mode.** Complete coherent slices of work; do not ask for confirmation after every rename or minor test-driver edit. Prefer fewer significant commits and review checkpoints over many incremental housekeeping commits.
- **Five phases do not imply five pull requests.** Prefer batching A+B into one substantive organizational/testing PR with three-platform CI, because A by itself is mostly inventory and the next real cross-platform risk arises when CMake paths and test layout change. Phase C may be grouped with that PR if it stays manageable, or stand alone if its failure-handling changes warrant an independent cross-platform review. Phase D and Phase E ordinarily deserve separate acceptance checkpoints because GUI display and installation/packaging validation carry distinct risks. Adjust grouping to actual scope and validation evidence; do not enforce a rigid commit count.
- Commit/push/PR when changes are **material and reviewable**, needed for CI on other OSes, or needed as a durable handoff; never for every micro-edit. Keep a logical work branch. Do not write directly to `master`, merge, release, delete user files or disturb a dirty local index without express authorization.
- The initial `maintenance/testing-architecture` remote branch is a **continuity checkpoint** based on `master` `01400118`. At this handoff its only intended change is this status document; it is not a claim that Phase A or B implementation has been completed. It is deliberately not a new PR yet. A future chat must read `PROJECT_STATUS.md` from this branch, not the previous default-branch version, until this change is merged.
- Before any local edits, confirm the user's actual checkout branch, `HEAD` and `git status --short`. Remote branch state does not describe uncommitted Windows-local changes. Do not overwrite working files or reapply old ZIP patches blindly.
- When phase work is accepted, record exact platform/commands/results, file changes, branch/commit/PR and scope of acceptance in this document; do not mark planned tests passed before they run. Supply a Conventional Commits message and brief modified-file summary as `AGENT.md` requests.

## Next-chat resumption instructions

1. Read `AGENT.md` and **this file from `maintenance/testing-architecture`** first; verify current branch against `master` and any newly merged PRs. Review `docs/architecture.md`, `docs/developer-setup.md`, `docs/scientific-validation.md` and `docs/change-aware-ci.md` for existing behavior.
2. Read `tests/CMakeLists.txt`, `tests/scientific/CMakeLists.txt`, `tests/unit/**/CMakeLists.txt`, `tests/m5/*`, `tests/cmake/*`, `.github/workflows/ci.yml` and `.github/workflows/release.yml`. These are the implementation touchpoints, not a license to blanket-rename all old test schemas.
3. Continue with **Phase A as the first deliverable and Phase B as the first substantial checkpoint**. Work in a single coherent pass: establish inventory/policy; revise CMake structure and functional file locations; preserve test identity and fixtures; verify local native tests before asking for a three-OS PR CI checkpoint. No premature micro-commits.
4. After A+B validation, continue through C, D and E using the gates above, combining commits where technically sensible. No 500M rerun, no unintended GUI/CLI architectural redesign, no public release.
5. Keep the status short and authoritative. Detailed superseded runtime-refactor diaries remain in repository Git history; they should not be resurrected or treated as active tasks.
