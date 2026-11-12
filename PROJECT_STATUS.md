# Project status

Last updated: 2026-11-12

**Active work:** Phase E — installed-runtime and packaging acceptance on a dedicated development branch; initial relocated CMake-install pilot underway. Phase D is ACCEPTED and merged as PR #45 (merge commit `4c4121f5e52ac354dbe4d67bcb5472946d74be05`), with required Windows WGL, Ubuntu X11/GLX and macOS Cocoa native GUI CTests all passing. A+B+C are also accepted. This remains maintenance work, NOT an authorized product release.

**This document is the current handoff.** Older 2026-11-10 P0/P1/P2 candidate/pending diaries and runtime-refactor plans have been superseded by merged code and remain in Git history. Do not resume those plans. Read `AGENT.md` and `docs/testing.md` before changing code.

## Verified code and validation baseline

- Repository: `CST-Modelling-Tools/tonatiuhpp`; default branch `master`. **Accepted starting point for Phase D:** PR #43 merge commit `0c0b3ee3a9a16c733340a999ff3372efba9dcd46` (2026-11-12). The five-phase plan originally began at PR #41 merge `01400118f50557ae82883b523bb42905a2970c11` (2026-11-11).
- Earlier accepted production work: PR #39 (M0–M5 shared scientific engine), PR #40 (permanent GUI/CLI entrypoints, shared runtime and large File-export limit correction; merge `edf301a354cdec36c61f9d5c880c016fe7784528`), and PR #41 (scientific test rationalization; merge `01400118f50557ae82883b523bb42905a2970c11`).
- **Phase A+B:** PR #42 merged at `0e2edff29483ee0dd944ee585687b4a54d4390a0` (2026-11-11). It established `docs/testing.md`, the reproducible CTest inventory, modular test registration and behavior-oriented test paths. PR #42 CI run `43102812883` passed Windows, Ubuntu and macOS.
- **Phase C:** PR #43, `test(reliability): harden CTest execution and CI evidence`, merged on 2026-11-12 at `0c0b3ee3a9a16c733340a999ff3372efba9dcd46`. Its CI run `43160351782` completed successfully across `windows-latest`, `ubuntu-24.04` and `macos-latest`: **180/180**, **179/179** and **179/179** CTests respectively. Reported full-suite elapsed times: Windows 60.70 s, Ubuntu 42.60 s and macOS 39.49 s. Evidence: https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/43 and https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43160351782 .
- **Windows maintainer Phase C evidence:** Release configure/build, preserved 180-test inventory, Python tooling/negative cleanup-guard checks, complete sequential and bounded parallel CTest runs (180/180), corrected `fast` tier (15/15; 13 units plus 2 CLI checks), and an interactive native GUI launch with tracing/flux. The GUI exercise is manual/opt-in validation, **not yet** required automated GUI CTest evidence.
- Production executables are `tonatiuhpp-gui` (Qt Widgets/SoQt application) and `tonatiuhpp-cli` (QCoreApplication CLI). Both share one scientific kernel and the production simulation/scripting services. The CLI does **not** accept `--headless`; the GUI rejects it. Retain the two-executable architecture; a dynamically loaded GUI module was previously evaluated and rejected.
- Important compiled targets: `TonatiuhppKernel`, `TonatiuhppLibraries`, `TonatiuhSimulation`, `TonatiuhScripting`, `TonatiuhScriptRuntime` and `TonatiuhppGuiPresentation`. The headless/kernel dependency boundary excludes Qt Widgets, Qt PrintSupport and SoQt; some shared code still intentionally uses QtGui/Qml.
- The accepted frozen 500-million-ray Windows heliostat benchmark (M5/PR #39) and its scientific reference remain valid only for their documented scope. **Never rerun the 500-million-ray benchmark or regenerate its frozen artifacts** during routine development/CI, and do not change physics, RNG/seed behavior, tracker policy, photon binary format, or flux/power normalization as part of testing work. See `docs/scientific-validation.md` for provenance and hashes. Passing build-tree CI is **not** installed-runtime, native GUI automation or release acceptance.

## Current testing landscape (accepted through PR #43)

- `source/CMakeLists.txt` includes CTest; `tests/CMakeLists.txt` coordinates modular registrations under `tests/unit/`, `tests/scientific/`, `tests/integration/cli/`, `tests/integration/scientific/` and `tests/architecture/`. Google Test uses `gtest_discover_tests()`. The historical Windows reference has **143 GoogleTests** (130 scientific, 13 unit) and 37 other CTests; the supported hosted baselines are **180 Windows**, **179 Ubuntu/macOS**. The sole intended Windows-only registration is `scientific.ScientificPluginPaths.WindowsBuildTreeIncludesMaterialPluginRoot`.
- CTest is the authoritative native discovery/execution/report interface. The full Release regression remains `ctest --test-dir build -C Release --output-on-failure`; a measured quick tier is `ctest --test-dir build -C Release -L fast --output-on-failure` with **15 tests** (13 unit and 2 CLI argument/startup tests). Historical `smoke` labels are broader and **must not** be mistaken for `fast`; overlapping label totals are not additive.
- Phase C made required Python and scientific fixtures configuration requirements, rather than silently losing registered scenarios. `tests/cmake/PrepareTestOutput.cmake` guards disposable build-tree output cleanup against traversal, symlink and redirected destinations; reviewed nested CMake process launches have explicit timeouts. `tests/tooling/test_test_output_guard.py` covers negative-path behavior. CI checks platform-aware inventories and test sentinels, and uploads `build/ctest-inventory.json` and `build/ctest-results.xml` (JUnit).
- Separate Python `unittest` checks validate CI classification, scientific auditors/comparators and CTest tooling; a passing native CTest total does **not** automatically include separately invoked Python tests or optional diagnostics. The standalone `tests/soqt_smoketest/` is also outside ordinary CTest registration.
- `tests/integration/cli/cli_process_repeatability.py` establishes scientific repeatability across **two independent real CLI processes**; it does **not** prove GUI/CLI equivalence. Its `--mode installed` offers a starting point for Phase E but does not mean installed-runtime acceptance has passed.
- The real native GUI acceptance helper lives in `tests/integration/gui/NativeGuiAcceptance.cpp` and `.h`, compiled into `tonatiuhpp-gui` only with `BUILD_TESTING=ON`. It is opt-in through `TONATIUHPP_M5_GUI_NATIVE_REPORT` and `TONATIUHPP_M5_GUI_NATIVE_MODE` (plus mode-specific variables), supports `interactive-file`, `seeded-interception` and `interactive-flux`, and exercises real Qt dialogs/Script Editor/flux paths. It is **not yet registered as routine automated GUI CTest**. Merely obtaining output, a positive flux or matching CLI runs does not establish independent physical accuracy.
- `tests/scientific/GuiSunSizingDiagnostic.cpp` is opt-in, not unattended acceptance. `installer/prepare_ifw_payload.py`, `.github/workflows/release.yml` and the installed mode of `cli_process_repeatability.py` provide Phase E foundations. No uniform unattended, three-OS GUI scientific acceptance or installed-artifact acceptance is claimed.
- `.github/workflows/ci.yml` performs three-OS Release build-tree validation for relevant changes and separately validates Python tooling. Markdown-only changes may use its tested change classifier and skip the native build; a documentation-only CI result must not be described as a fresh three-OS scientific regression.

## Approved FIVE-PHASE implementation plan

**Working principle:** Five coherent engineering phases, **not** five mandatory commits or five micro-PRs. Preserve all meaningful scientific coverage. Finish a phase as a useful whole before polishing peripheral details. No changes to physics or benchmark reference data. Maintain this status at meaningful checkpoints with exact evidence and state (local / branch / PR / merged).

### Phase 1 / A — Testing inventory, policy and classification

**Goal:** Make the testing contract and coverage discoverable before reorganizing files.

**Deliverables:** Author `docs/testing.md` as the current testing policy; produce or document a reproducible CTest inventory (prefer `ctest --show-only=json-v1`, with a small helper only if it materially helps). Include test names, owners/domain, level, runtime tier, execution interface, fixture/reference, platform prerequisites, deterministic/approximate assertions, and whether execution is build-tree, GUI or installed. Explicitly inventory separately executed Python `unittest` and standalone developer diagnostics.

Define a consistent label taxonomy: level (`unit`/`integration`/`system`), domain (for example `scientific`/`scripting`/`export`/`gui`/`architecture`), and tier (`fast`/`regression`/`extended`). Distinguish same-process component checks, CLI repeatability, numerical reference validation and real GUI/CLI comparison. Establish an unambiguous small quick-run command and the authoritative full regression command. Ensure required coverage is not confused with optional diagnostics.

**Gate:** Current 180-test baseline is accounted for on the reference Windows build; additional non-CTest checks are identified, labels are checked against actual discovery, and no test scenario or scientific evidence is dropped. **State: ACCEPTED; PR #42 merged with Windows + Ubuntu + macOS CI passing (2026-11-11).**

### Phase 2 / B — Test layout and CMake modularization

**Goal:** Organize tests by enduring behavior instead of M3/M4/M5 milestone labels.

**Deliverables:** Split the large `tests/CMakeLists.txt` into coherent, function-oriented subdirectories (e.g. `unit`, `scientific`, `integration/cli`, `integration/scripting`, `integration/export`, `integration/gui`, `architecture`, `support`, `fixtures`). Relocate the three `tests/m5/` survivors into their correct GUI/CLI areas. Review remaining historical driver/fixture names and only rename where doing so improves stable comprehension and all references can be updated safely. Retain established scientific fixture contents, report schemas, script APIs, generated artifact formats, baseline paths and output semantics.

**Gate:** Fresh configure/build succeeds; 180 existing scenarios remain discoverable and pass with no silent skips (a changed count needs explicit scenario-level justification); all affected file paths, CMake generator expressions, Windows DLL staging, Python references, docs and CI agree. Validate on Windows/Ubuntu/macOS before merging the structural change. **State: ACCEPTED; PR #42 merged and all three CI platforms passed.**

### Phase 3 / C — Test infrastructure reliability and reporting

**Goal:** Make test execution safe, deterministic, diagnosable and parallel-safe.

**Deliverables:** Consolidate duplicate subprocess/exit-code/output-management logic **only where maintenance gains justify it**; keep simple CMake drivers simple and retain Python for substantial structured comparisons. Use disposable test-root directories under the build tree; validate cleanup targets (including symlinks/path traversal), avoid accidental shared-output races, enforce process-level and CTest-level timeouts, and preserve stdout/stderr plus useful artifacts for failures. Remove ambiguous conditional registration for mandatory scenes/dependencies. Rationalize `smoke` vs `fast` semantics, and generate CI-readable CTest JUnit/summary artifacts where supported. Test the harness itself with deliberately failing cases.

**Gate:** Full suites succeed both sequentially and with bounded parallelism, failure injection yields correct nonzero results, cleanup refuses unsafe paths, no mandatory test silently disappears, and all three CI platforms pass. **State: ACCEPTED; PR #43 merged on 2026-11-12 after Windows local validation and successful Windows, Ubuntu and macOS hosted CI (run 43160351782).**

### Phase 4 / D — Genuine GUI scientific acceptance

**Goal:** Exercise real `tonatiuhpp-gui` flows as distinct from CLI reproducibility.

**Deliverables:** Wrap the existing native GUI helper in bounded process-level CTest drivers with isolated temporary output, reliable reports and explicit shutdown. Initially cover its three real GUI modes: interactive File export, seeded Script Editor interception, and interactive Flux Analysis. Validate generated numerical artifacts against independently trusted scientific expectations/comparators where applicable, not only against duplicate GUI/CLI executions or "positive output". Guard test-only behavior so production `BUILD_TESTING=OFF` binaries remain free of acceptance machinery.

**Gate:** A real display/Qt/SoQt/OpenGL environment is confirmed for each platform before making GUI checks required. Bounded timeouts, real widget interaction, independent evidence and no frozen 500M run. Add platform-specific CI gating only after reliable execution is proven; do not claim all three GUI environments are functional without actual evidence. **State: not started.**

### Phase 5 / E — Installed-runtime and packaging acceptance

**Goal:** Verify that what users install works independently of the source/build tree.

**Deliverables:** Integrate disposable CMake-install / IFW-payload checks with direct installed `tonatiuhpp-cli` and graphical `tonatiuhpp-gui` acceptance. Reuse the existing `tests/integration/cli/cli_process_repeatability.py --mode installed` CLI acceptance logic after checking its assumptions. Ensure scientific plugins, Qt platform plugins, SoQt/Coin runtime dependencies, macOS `.app`/Frameworks layouts, Linux RPATH and Windows DLL discovery function without borrowing the development SDK or build-directory paths. Distinguish installed-runtime smoke checks from comprehensive installer/upgrade testing; only claim what has actually been executed.

**Gate:** External-prefix/packaged installations pass bounded CLI and available real GUI scenarios, are relocated or launched outside source/build contexts, and pass platform-appropriate Windows/Linux/macOS checks. No modifications to user installation locations; no release authorization implied. **State: not started.**

## Execution and Git checkpoint policy (owner decision 2026-11-11)

- **Work in milestone mode, not micro-step mode.** Make coherent, reviewable changes and avoid cosmetic refactoring, fragmented micro-commits, and premature release claims.
- Phases A+B were accepted together in PR #42; Phase C was accepted in PR #43. Phase D (native GUI acceptance) and Phase E (installed-runtime acceptance) should have distinct validation and review checkpoints because they exercise different contracts. The five phases do not require five commits or a rigid PR count.
- Commit/push/open PRs for meaningful changes, CI validation or durable handoffs, not every tiny edit. Use a dedicated work branch. **Do not write directly to `master`, merge, release, delete user files or disturb a dirty local index without express authorization.**
- Before modifying the Windows checkout, confirm `git branch --show-current`, `git rev-parse HEAD` and `git status --short`. The maintainer reported synchronizing the local checkout after the Phase C merge (2026-11-12), but GitHub cannot verify uncommitted Windows-local state.
- For acceptance, record exact platform, commands, results, artifact/reference provenance, branch, commit/PR and scope. Passing one interface contract must not be relabeled as another. Supply a Conventional Commits message and modified-file summary as required by `AGENT.md`.

## Next-chat resumption instructions

1. Read `PROJECT_STATUS.md`, `AGENT.md`, `docs/testing.md` and `docs/scientific-validation.md` from the active branch, and compare with `master`. The accepted Phase D code baseline is `0c0b3ee3a9a16c733340a999ff3372efba9dcd46`; Phases A, B and C must **not** be restarted.
2. Verify the actual local Windows branch/HEAD/working tree before modifying local files. Keep a separate Phase D work branch; no direct `master` edits.
3. Begin Phase D by auditing `tests/integration/gui/NativeGuiAcceptance.cpp` and its integration in `tonatiuhpp-gui`. Preserve the real widget interactions and existing three modes; distinguish the helper's current opt-in behavior from CTest registration and confirmed platform support.
4. Implement bounded, isolated, process-level native GUI acceptance with reliable reports, explicit shutdown and **independent numerical references/comparators** where applicable. Never substitute two-CLI repeatability for GUI acceptance or claim correctness from nonempty/positive output alone.
5. Verify a real display/Qt/SoQt/OpenGL environment before enabling GUI acceptance as required CI on an OS. Collect exact Windows, Ubuntu and macOS evidence separately; do not enable unsupported display configurations or claim all three platforms were tested prematurely.
6. Maintain the immutable 500-million-ray benchmark, scientific implementation and `BUILD_TESTING=OFF` production separation. After Phase D acceptance, pursue Phase E in a separate installed-runtime/packaging checkpoint; no release is implied.

## Accepted A+B+C and Phase D starting checkpoint (2026-11-12)

- **A and B — ACCEPTED:** PR #42 merged 2026-11-11 at `0e2edff29483ee0dd944ee585687b4a54d4390a0`; hosted CI run `43102812883` passed three OSes. Windows maintainer evidence: 180/180 CTests, preserved identities, inventory/tooling validation and real GUI launch. Permanent behavior-oriented test layout is in place.
- **C — ACCEPTED:** PR #43 merged 2026-11-12 at `0c0b3ee3a9a16c733340a999ff3372efba9dcd46`. Its GitHub Actions PR run `43160351782` passed all three build jobs: Windows 180/180 (60.70 s), Ubuntu 24.04 179/179 (42.60 s), macOS 179/179 (39.49 s). Windows local maintainer validation included sequential and parallel 180/180, negative output-cleanup/tooling checks, 15/15 corrected fast-tier tests (13 unit) and interactive GUI tracing/flux. These are build-tree and separately reported manual GUI results, **not automated Phase D acceptance**.
- **D — NEXT / NOT STARTED:** Register and scientifically validate actual `tonatiuhpp-gui` interactions in bounded CTest drivers; establish reliable display-backed CI evidence per OS before making requirements mandatory. Do not change scientific references or trigger the frozen benchmark.
- **E — NOT STARTED:** Independently validate installed GUI/CLI runtime paths, dependencies, scientific plugins and selected small numerical outputs from disposable external install/payload roots.
- **Current scope:** All A–C acceptance evidence precedes this status-only update. Updating this Markdown document does **not** rerun any tests, authorize a release, or imply acceptance of D or E.

## Phase D implementation candidate checkpoint (2026-11-12)

- **Status:** Implementation candidate, **not accepted or merged**. Development branch:
  `maintenance/native-gui-scientific-acceptance`, based on Phase D baseline
  `a7473a2d805529c3efb1cfb00c043978255d15be` (PR #44 merged).
- Proposed optional registration: `TONATIUHPP_ENABLE_NATIVE_GUI_ACCEPTANCE=ON`
  adds three real GUI CTests, with optional Linux `xvfb-run` launch through
  `TONATIUHPP_GUI_ACCEPTANCE_USE_XVFB=ON`. Default existing Windows 180 /
  Linux and macOS 179 CTest inventory stays unchanged.
- The test-only Python runner executes actual GUI Qt workflows with bounded
  process lifetimes, isolated logs and reports, an independent cylinder GUI/CLI
  photon-export comparator, and 8x8 m planar 1,000 W/m² interception/flux
  analytical checks using the preexisting small scientific scripts.
- Python synthetic numerical-oracle positive/negative tests passed locally
  (13 tests). **Real Qt native acceptance has not yet been run or demonstrated
  on any supported OS in this checkpoint; CI acceptance is pending.** This
  statement is not an authorization to merge Phase D, modify physics or
  rerun the 500-million-ray frozen benchmark.
- Next: run on the maintainer's native Windows Release build in a genuine
  visible SoQt/OpenGL session, review retained per-mode scientific evidence,
  then establish reliable Ubuntu/macOS display-backed CI separately; record
  exact results before enabling required platform GUI gates.

### Phase D native GUI acceptance — three-platform evidence (2026-11-12)

- Native Windows Release maintainer evidence: **183/183** CTests, including
  the three real graphical workflows, plus independently reviewed numerical
  `acceptance.json` output. Earlier isolated native GUI run: 3/3.
- Ubuntu 24.04 hosted X11/GLX (Mesa llvmpipe) in CI run
  [43185720709](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43185720709):
  **3/3** native GUI CTests, true widget/SoQt session and numerical reports,
  alongside successful ordinary 179/179 build-tree CTests.
- macOS Cocoa in that same CI run, after correcting regenerated obsolete
  AGL linker flags: **3/3** native GUI CTests, real Cocoa widget session and
  numerical reports, alongside successful ordinary 179/179 build-tree CTests.
- The real File-export GUI and CLI produce identical canonical photon
  multisets **within each platform**: 3184 records on Windows/Linux and 3197
  on macOS (per-photon power 0.0719665 W). This is not a claim of exact
  cross-platform RNG equality. Script interception is 63857.145538085264 W
  Windows/Linux and 63889.13809597008 W macOS; native Qt flux integrals are
  63857.145538085264 W Windows/Linux and 64225.0599537606 W macOS, against
  the independently expected 64000 W ideal planar absorber reference.
- **Phase D tested-scope cross-platform evidence is now verified.** The
  three graphical tests are being promoted to required Ubuntu/macOS CI
  checks; hosted Windows graphical feasibility is being probed independently
  while the real Windows desktop evidence remains accepted. Final PR review,
  the new required-gate CI run and user-authorized merge are still pending.
- Acceptance scope remains limited to the three small reference fixtures
  and documented 20% Monte Carlo/analytic criterion. No change to scientific
  kernel, sampling, photon serialization or immutable 500-million-ray
  heliostat benchmark. Phase E installed-runtime acceptance is separate.

### Phase D Windows hosted crash remediation (2026-11-12)

- Windows WER/CDB [43212203419](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43212203419):
  null OpenGL dispatch in `Qt6Gui!QOpenGLFunctions::glViewport` during
  native expose/flush; `0xC0000005` underlying exception, outer
  `0xC000041D` callback termination. A 1024x768 `WinSta0\\Default`
  desktop existed; graphics adapter was `Microsoft Hyper-V Video`.
  No scientific GUI test action or numerical assertion had begun.
- Pinned Mesa3D 25.1.9 MSVC software WGL remediation:
  [43220096714](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43220096714)
  achieved **3/3 actual hosted Windows native GUI tests** and
  **180/180 ordinary Windows CTests**. Canonical GUI/CLI photons
  3184/3184, interception and Qt flux 63857.145538085264 W,
  and script flux 63857.114 W. Native Windows desktop evidence
  183/183 remains independently valid.
- This milestone removes temporary WER diagnostic code/checkpoint
  instrumentation and makes hosted Windows graphical acceptance **required**
  just as Linux/macOS graphical gates already are. Mesa dependency is
  SHA256 pinned and staged only inside CI's disposable build/bin/Release
  directory, then removed before upload; not a production dependency.
- **Final required-gate CI run pending** at this checkpoint. Do not merge
  until all three mandatory OS graphical checks pass and the maintainer
  explicitly approves merging. Immutable 500-million-ray benchmark
  and production scientific kernel remain unchanged.

## Phase E initial implementation checkpoint (2026-11-12)

- **Base:** accepted master `4c4121f5e52ac354dbe4d67bcb5472946d74be05`
  (Phase D PR #45 merged). Branch:
  `maintenance/installed-runtime-packaging-acceptance`. No direct
  master changes or merge are authorized.
- **Initial Phase E scope:** new disposable relocated CMake-install pilot
  on the three-OS CI matrix. Reuse the preexisting scientific installed
  CLI repeatability and actual Qt GUI acceptance scripts; require
  installed GUI/CLI executables and installed cylinder/sphere example
  scenes. Launch scientific subprocesses from unrelated working
  directories with SDK/library-loader environment overrides removed.
  On Windows, continue to use the verified SHA256-pinned Mesa software
  WGL only as temporary hosted CI infrastructure.
- **New evidence:** installer relocation guard self-tests explicitly
  reject unexpected paths, source-tree installs and symlinked install
  roots. The GUI reports remain actual Qt/SoQt real-dialog and scientific
  analytical comparisons, not offscreen simulations.
- **Pilot status:** exploratory and `continue-on-error` until each
  platform's relocation, native dependency loading, plugin lookup and
  numerical results are examined. A green overall CI build does not
  mean this pilot succeeded.
- **Remaining acceptance:** repair any actual installed-runtime
  failures, audit macOS Frameworks/plugin RPATH, verify production
  `BUILD_TESTING=OFF` release-like payloads, and distinguish CMake
  installs from Qt IFW installer/update behavior. Frozen 500M artifact
  remains immutable; no production physics change. PR stays draft
  until scientific evidence supports a final acceptance decision.

### Phase E first CI evidence — genuine installed-runtime failures

- First pilot [43269447246](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43269447246)
  ran Linux/macOS/Windows CMake installation into disposable runner temporary
  roots. The installed CLI passed on Linux and Windows; all three relocated
  GUI/runtime paths were **not yet accepted**. The step was
  `continue-on-error`, hence green overall jobs must not be conflated
  with pilot success.
- Linux: missing `libQt6OpenGL.so.6` at GUI launch (exit 127) despite the
  library being present in installed `lib`. Patch installed bundled shared
  library `RUNPATH` for transitive dependencies.
- macOS: missing private `@rpath/libTonatiuhppKernel.dylib` from raw
  CMake-installed CLI; macdeployqt does not bundle all private runtime
  modules. Reuse release app consolidation in the pilot.
- Windows: missing runtime DLL at installed GUI startup
  (`0xC0000135`), while installed CLI passed. Add explicit Qt6OpenGL
  DLL deployment for SoQt's indirect dependency, then require actual
  rerun evidence before concluding this was the missing DLL.
- No benchmark regeneration or production scientific changes. Next:
  inspect second pilot outcomes and harden remaining installed dependencies;
  explicit new production `BUILD_TESTING=OFF` payload validation remains
  required for final Phase E acceptance.

### Phase E consolidated fix and production-smoke pilot (2026-11-12)

- [Second CI run 43272756323](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43272756323)
  completed with default CI and Phase D native GUI gates green.
  **Linux/macOS relocated CMake installs passed** installed CLI plus
  all three actual GUI scientific modes. Windows installed CLI passed
  but installed GUI still exited `0xC0000135` before scientific output.
  The non-gating pilot step was marked as failed despite parent CI success.
- Windows PE import audit of the hosted CI artifact:
  `TonatiuhppGuiPresentation.dll` requires `Qt6PrintSupport.dll`,
  absent from the captured build runtime; `SoQt1.dll` requires
  `Qt6OpenGL.dll`, present after the earlier targeted fix. Candidate
  install correction explicitly bundles Release Qt6PrintSupport,
  Qt6OpenGL and Qt6OpenGLWidgets. A new fail-closed installed-GUI
  dependency preflight and positive/negative tool tests expose the
  exact missing DLL, independently of opaque process exit status.
- Start a **separate BUILD_TESTING=OFF** exploratory pilot after the
  test-enabled installed acceptance: reconfigure and rebuild the real
  production GUI/CLI, relocate its install, run actual production CLI
  scientific acceptance and real native GUI liveness from a foreign CWD.
  The production GUI does NOT have test-only Qt dialog hooks, and a
  seven-second startup smoke cannot establish full GUI numerical
  acceptance. No IFW installer/updater test is implied.
- Three-platform CI for these candidate changes remains to be
  executed and reviewed. Only then consider promoting completed
  installed-runtime acceptance to a required gate; IFW packaging,
  release environment independence and maintenance updates remain
  separate evidence requirements. **PR #46 remains draft/unmerged.**

### Phase E preliminary Qt IFW production-payload pilot

- The next exploratory CI checkpoint will stage the existing
  `BUILD_TESTING=OFF` CMake install through
  `installer/prepare_ifw_payload.py`, make a separate copy of the
  `com.tonatiuhpp.app/data` payload in runner temporary storage,
  relocate it, and execute production CLI scientific acceptance plus
  actual native GUI startup. This is separate from the already-tested
  `BUILD_TESTING=ON` dialog acceptance and plain production CMake
  install smoke. Do not infer full GUI numerical acceptance or actual
  IFW installer/update/uninstall execution from these checks.
- Pilot remains `continue-on-error` until individual Linux, macOS and
  Windows results, loader paths and numerical evidence are reviewed.
  All prior required Phase D tests remain unchanged; PR #46 stays draft.
