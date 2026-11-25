# Tonatiuh++ testing architecture

## Current build and scene-tree acceptance

The merged [instance-tree unification (PR #70)](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/70) has two regression tests: `SceneInstanceBuilderRegression.FullBuildMatchesInsertedNestedSubtree` and `SceneInstanceBuilderRegression.InsertSharedGroupKeepsSiblingAndSubtreeIdentities`. The native GUI `gui.native_scene_undo_redo` test additionally checks Qt model consistency and canonical instance-tree structure across edits and undo/redo.

The pre-BVH `master` baseline had **205 standard CTests on Windows** and **204 on Linux/macOS** (or **209 / 208** with four opt-in native GUI tests). The merged BVH implementation adds four scientific regression registrations, as detailed below.

**Validated baseline:** the maintainer's Windows Release suite passed **209/209 CTests** (including four native GUI tests; 230.06 seconds). [CI run #45439316050](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45439316050) passed on Windows, Ubuntu and macOS, including standard CTest and platform-specific native GUI acceptance. These historical results validate the earlier merged tree-construction refactor, not the BVH release qualification.

The merged BVH-only implementation adds **four** `SceneBvhRegression` Google Tests. The maintainer reports **213/213 Windows Release CTests passed** (224.11 seconds), including four native GUI acceptance tests. Production recursive traversal is removed, and a **test-only O(N) linear scan** independently verifies nearest-hit surface and geometry. Fixed-seed regressions compare complete per-ray launch/termination, reflection paths, occurrence pointers, differential geometry, photon records and energy budgets across Fresnel (vacuum and air), multiple reflections, tracker updates and shared coincident instances, using two deterministic chunk seeds per scene. Standard BVH-enabled CTest inventory is **209 Windows / 208 Linux/macOS**, or **213 / 212** with native GUI. [PR #72 CI run #45521725484](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45521725484) passed on Windows, Ubuntu and macOS.

A fresh 500-million-ray Release candidate run at the feature source commit matched the previous v0.1.8.28 candidate grid SHA-256 exactly; independent read-only checks verified all 10,000 binary/CSV grid cells and reconstructed power/flux statistics. The historical frozen grid remains different for preexisting physics reasons. See [proposed v0.1.8.29 release acceptance](release-acceptance-v0.1.8.29.md); release-tagged binaries and installers are pending validation.

For a local Windows Release configuration with native GUI acceptance enabled:

```powershell
python scripts/ctest_inventory.py --build-dir build --config Release --expect-count 213 --require-sentinels --json-out build/ctest-inventory.json
ctest --test-dir build -C Release -R 'SceneInstanceBuilderRegression|SceneBvhRegression|gui[.]native_scene_undo_redo' --output-on-failure
ctest --test-dir build -C Release --output-on-failure
```

## Merged naming migration — verified post-merge status (2026-11-23)

The combined G0 + G1 + G1.1 + G2 + G2.1 migration merged to `master` as
[PR #60](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/60), merge commit `b8934c0dc604ed7e4a27246ba61b94cf064af1d5`. The
[three-platform CI run #287](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45129823592) completed successfully, with
Windows, Ubuntu and macOS jobs all passing. Maintainer-reported final Windows
Release evidence: **207/207 CTests passed** (including four native GUI tests,
226.68 seconds elapsed), plus **44 Python tooling tests: 41 passed, three
expected Windows directory-symlink privilege skips**. All four test-naming
safeguards passed. No frozen 500-million-ray benchmark execution or reference
regeneration was part of this migration. The pre-merge pending/failed-stage
accounts below are historical and superseded by this verified outcome.

## Historical Windows acceptance: behavioral test-naming migration (2026-11-23)

The maintainer ran the full **Windows Release** suite after the combined
G0 + G1 + G1.1 + G2 + G2.1 migration: **207/207 CTests passed**, zero failures,
**226.68 s** real elapsed time. This includes **four native GUI acceptance
tests**; CTest reports 169 tests with the `scientific` label (labels overlap).
The separate Python tooling suite reported **44 run, 41 passed, three skipped**
for unavailable Windows directory-symlink privileges, and **zero failures**.
All four behavioral naming safeguards passed.

The earlier G1/G2 naming-guard failures documented below were corrected and
superseded by these successful checks. The frozen 500-million-ray benchmark
was **not** rerun. The independent audit's exact historical `m5_full_*`
full-run directory format remains supported; routine benchmark paths are
behavior-named. The migration changes neither scientific algorithms nor
frozen reference data. The combined work remains uncommitted pending review,
explicit authorization, and hosted **Windows/Linux/macOS CI** before merge.

## G2.1 benchmark-runner naming guard correction

The first G2 Windows Python tooling run executed 44 checks: 40 passed,
three expected symlink-permission skips and one failure in the in-file naming
guard. The new behavioral CMake benchmark runner intentionally continues to
emit `m5_full_<10 hex>` for the optional *full* mode, because the unchanged
frozen 500-million-ray auditor accepts that specific historical directory
format. G1.1 had permitted the auditor and its synthetic unit test, but not
its producer. G2.1 adds exactly that third file to the narrow exception and
checks the old full-mode path plus the descriptive routine-mode path.

No benchmark output, benchmark generator, reference, auditor or production
source changes in G2.1. Rerun 44 Python tests (expect 41 passed and three
Windows privilege skips) and the full 207 native Release CTests after the
combined migration. Hosted cross-platform CI and merge remain pending.

## G2 active test-interface naming (candidate; unvalidated)

The G1.1 Python-only rerun passed all 43 tooling tests (40 passed and three
Windows directory-symlink skips). G2 changes only the names of test-only
native-GUI hooks, photon File failure injection, signature/ray-history opt-in
capture keys, CMake-private dependency guards and test-generated evidence
schemas; producers/consumers are changed together. Scientific calculations,
input scenes, RNG, photon binary formats and frozen 500M references are not
modified. The full historical benchmark output directory `m5_full_<10 hex>`
remains accepted by its unchanged independent auditor. Routine heliostat
smoke/repeatability output directories become behavior-named.

New expected Python tooling inventory: 44 tests including four test-name
checks. Windows Release native GUI-enabled inventory stays at 207 tests.
G2 has not been built or run on Windows/Linux/macOS; cross-platform PR CI
and merge are pending.

## G1.1 naming guard correction — awaiting Windows revalidation

The maintainer reported a complete Windows Release **207/207 CTest PASS**
(228.64 seconds, four real GUI tests) for uncommitted G0+G1. Of 43 Python
tooling tests, **39 passed, one naming guard failed, and three skipped** for
unavailable Windows directory-symlink privileges. The guard detected two
legacy names of disposable build-tree CTest output directories, plus the
historic `m5_full_*` directory contract in the frozen 500-million-ray
*auditor* and synthetic audit test. G1.1 corrects the disposable output
paths and associated strict cleanup checks, and explicitly scopes the
auditor exception to those two original test files and strings. The
benchmark auditor and frozen evidence are unmodified. Full Windows
revalidation after G1.1 and cross-platform CI remain pending.

Baseline date: 2026-11-11. This policy applies to the two-executable, single-scientific-kernel architecture introduced before PR #41. It does not authorize release or changes to scientific references.

## G1 test-owned evidence and scene naming (candidate; not yet validated)

After locally validated G0, this additional test-only migration gives
produced evidence files, scene-tree nodes and console diagnostics durable
behavior-based names. The exact mapping is documented in
`docs/test-naming-migration.md`. Fixture geometry, photon values, seed/RNG,
export formats, versioned schemas and native-GUI test-hook environment keys
are unchanged. The extra Python guard raises standalone tooling registration
to 43 tests, while Windows CTest registrations remain 207 with native GUI
acceptance enabled. G1 has not yet been built or run; obtain full Windows
Release and hosted cross-platform evidence before any merge.

## Test source naming cleanup G0 (Windows validated; CI pending)

After merged PR #59, 27 retained m3/m4/m5-labeled test driver, fixture,
verifier and benchmark-runner **filenames** still obscured what the test
exercises. `docs/test-naming-migration.md` lists a one-to-one rename mapping
and explains every preservation boundary. This is test-only work; registered CTest names and counts are unchanged
(207 Windows with four opt-in GUI tests). Two Python naming-guard tests
raise the separate tooling inventory from 40 to 42. **G0 local Windows
Release validation (maintainer-reported, 2026-11-22): 207/207 CTests
passed in 212.58 seconds; Python tooling 39 passed / 3 skipped out of 42.**
Hosted Windows/Linux/macOS CI and merge are not yet claimed. Schema
identifiers, result data names, scene names and test-only environment
variables remain for a separate, audited compatibility decision; this
is not a claim that all milestone strings have disappeared. The frozen
500-million-ray reference remains immutable.

## Historical test inventory and maintenance policy (2026-11-22)

Following merged PR #58, the default Windows Release inventory is **203
CTests**; the Linux/macOS default is **202**. When
`TONATIUHPP_ENABLE_NATIVE_GUI_ACCEPTANCE=ON`, four genuine display-backed
GUI CTests are registered. The maintainer validated **207/207** Windows
Release CTests after Batch E, including **4/4** native GUI modes; **206**
is the expected opt-in Linux/macOS registration count, not a locally
observed result. The separately invoked Python tooling inventory contains
**40 tests**: 37 passed and 3 skipped on the Windows runner because
unprivileged directory symlinks were unavailable.

**Batch F local Windows Release evidence (2026-11-22):** after the
`MainWindow::SetValue` transform-part type correction, the maintainer
reported **207/207 CTests passed** in **209.95 seconds**, including the
four real GUI native acceptance tests and three full Fresnel Undo/Redo
cycles; the separate Python tooling run reported **37 passes / 3 skipped**
out of 40 tests. This is Windows-only evidence; hosted three-platform CI
and merge are not yet verified.

The fourth native mode `gui.native_scene_undo_redo` executes the original
`Fresnel.tnhpps` through the Qt Script Editor, then exercises real GUI
Cut, Insert/Rename, Paste, Copy/Paste and `MainWindow::SetValue` for the
`TSeparatorKit` transform. Its `TTransform` readback and three complete
seven-command Undo/Redo cycles protect field-edit persistence, model
states, selection safety and Coin3D node identity. This is the focused
regression for the locally Windows-validated Batch F type-safe transform-part adjustment.
It does not run ray tracing or the frozen 500M reference.

Names should describe observable supported behavior, not the milestone
that introduced them. Historical fixture paths, output-directory safety
guards, report schemas and compatibility inputs must not be bulk-renamed.
Update `scripts/ctest_inventory.py` sentinels if a test identity changes.
Cross-process repeatability is not GUI/CLI equivalence or an independent
physical reference. The earlier 180/179 and other dated baselines below
are historical; do not use them as current expected inventories.

For the focused native GUI field-edit gate on a real display:

```powershell
cmake -S source -B build -DBUILD_TESTING=ON -DTONATIUHPP_ENABLE_NATIVE_GUI_ACCEPTANCE=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release -R '^gui[.]native_scene_undo_redo$' --output-on-failure -V
ctest --test-dir build -C Release --output-on-failure
python -m unittest discover -s tests/tooling -p "test_*.py" -v
```

For current builds without native GUI acceptance, turn that CMake option
OFF and use the BVH candidate's standard inventory above (209 Windows,
208 Linux/macOS). Run native GUI acceptance separately on a display-capable runner.

## Purpose and evidence

Tests protect four different contracts: (1) scientific correctness against independent physical or approved reference results, (2) repeatability of a given execution interface and input, (3) application/architecture behavior and interface integration, and (4) portability of compiled and installed artifacts. Passing one contract does not prove the others. In particular, two matching CLI traces establish reproducibility, **not** native GUI-to-CLI equivalence or general physical correctness.

The executable under test must be the real production `tonatiuhpp-cli` or `tonatiuhpp-gui` and must link the real production libraries. Avoid compiling duplicate scientific implementations in test targets. Preserve the frozen 500-million-ray benchmark as an accepted, immutable historical reference: do not execute or regenerate it for routine tests, documentation, refactors, or CI. Bounded preflight validation for arguments above 10 million rays is permitted only if it fails before tracing any rays.

## Framework roles

- **CTest**, included by CMake's `include(CTest)`, is the authoritative native test runner, discovery system, label/report interface and CI execution entry point.
- **Google Test** defines C++ assertions, fixtures and test cases. `gtest_discover_tests()` exposes each C++ GoogleTest case to CTest. `find_package(GTest CONFIG QUIET)` is tried first, with a pinned GoogleTest `v1.14.0` FetchContent fallback in `tests/CMakeLists.txt`.
- **Python** and **CMake script** drivers may launch the real applications, create isolated test data, and compare complex numerical artifacts. Python's stdlib `unittest` independently tests Python CI, audit and comparator utilities; it is not an alternative ray-tracing framework.
- A passing CTest invocation does not include separately invoked Python tests, opt-in GUI diagnostics or packaging tests unless those checks are explicitly registered with CTest. Record these separately.

## Reference baseline (merged PR #41)

- Windows Release, maintainer-reported full CTest: **180/180 passed**, 0 failures, 70.61 s (after production-library linkage change).
- **143 GoogleTests** (130 scientific, 13 unit) and **37 other registered CTests** (CLI, integration and architecture). The numbers describe a reference configuration, not a permanently required count on every generator or platform.
- Reported overlapping label totals: `architecture=2`, `equivalence=13`, `headless=36`, `scientific=144`, `smoke=37`, `unit=13`. **Do not add label counts together.** The current `smoke` label includes relatively expensive tests and must not be used as a synonym for a fast gate.
- GitHub Actions PR #41 run 43012836153 succeeded on Windows, Ubuntu and macOS. This demonstrates the configured build-tree CI matrix, not universal GUI or installer acceptance.
- The CI workflow separately runs `tests/ci/test_ci_change_classifier.py`, `tests/scientific/test_audit_500m_benchmark.py`, and `tests/scientific/test_compare_gui_flux_exports.py` using Python `unittest`. Other comparator self-tests and the standalone `tests/soqt_smoketest` are not automatically included in CTest.

## Test contract and classification

Maintain one entry per test family and identify: unique CTest name; C++ / CLI / GUI / installed entry point actually exercised; unit / integration / system test level; domain; required prerequisites; fixtures/reference source; assertion type (analytical, approved reference, cross-interface, repeatability, negative-path, smoke); output isolation; intended CI execution tier; expected runtime and platform restrictions; and owner or owning directory. If a value is unknown, report **unknown** rather than invent it.

### Label taxonomy (target policy)

Use three *orthogonal* dimensions over time:

| Dimension | Intended labels | Rule |
| --- | --- | --- |
| Level | `unit`, `integration`, `system` | What boundary the test exercises |
| Domain | `scientific`, `math`, `random`, `scripting`, `export`, `architecture`, `gui`, `deployment`, `tooling` | What behavior is checked |
| Tier | `fast`, `regression`, `extended` | When and how often the test should run |

Domain and level are not replacements for assertion semantics. A test tagged `equivalence` must explicitly state whether it compares independent numerical references, native GUI vs CLI, or two CLI processes. Keep `headless` as an interface/domain modifier where useful, never as the sole test level. The Phase A+B migration preserves existing CTest names and labels to maintain exact test identity. Rationalize labels in the separately validated Phase C reliability/reporting checkpoint. A new `fast` tier must be based on measured bounds and deliberate test selection, not the old `smoke` label.

### Test family ownership

| Family | Interface / evidence | Primary location |
| --- | --- | --- |
| Unit math and random | GoogleTest assertions | `tests/unit/` |
| Scientific component regressions | GoogleTest, compiled production services, reference/analytical checks | `tests/scientific/` |
| CLI behavior and scripts | Real `tonatiuhpp-cli`, expected exit status / bounded process outcomes | `tests/integration/cli/` and `tests/integration/scientific/` |
| Export/scientific process comparisons | Native CLI processes plus independent Python comparisons where possible | `tests/integration/scientific/`; shared drivers under `tests/cmake/` until audited |
| Architecture boundaries | Real compiled artifact/link/import checks | `tests/architecture/` |
| GUI native acceptance | Actual Qt dialogs, Script Editor and scene Undo/Redo | `tests/integration/gui/` (four opt-in native GUI CTests) |
| Installed runtime | External install and plugin/runtime discovery | Planned `tests/integration/installation/`; release/installer workflow |
| Python tooling | Python stdlib `unittest` of auditors, comparators, CI classifier | `tests/ci/`, `tests/scientific/` and future `tests/tooling/` |

For A+B, `tests/cmake/`, `tests/scripts/` and `tests/scientific/fixtures/` remain in place to avoid changing scientific data and existing runtime contracts. Historical milestone strings in report schemas, environment variables, scripts and reference artifacts are not generic rename targets.

## Reproducible inventory

After a configured Release build, use the bundled inventory helper to capture all CTest registrations, labels, commands, configured timeouts and conservative inferred categories:

```powershell
python scripts/ctest_inventory.py --build-dir build --config Release --json-out build/test-inventory.json --markdown-out build/test-inventory.md --expect-count 203
```

Save the baseline JSON outside the source tree before changing registrations. To compare after a CMake restructure:

```powershell
python scripts/ctest_inventory.py --build-dir build --config Release --baseline build/test-inventory-before.json --json-out build/test-inventory-after.json --markdown-out build/test-inventory-after.md --expect-count 203
```

The tool compares **individual CTest names**, not only the total count. It deliberately records unclassified properties as unknown and does not claim an existing `smoke` label is fast. For baseline collection without the helper, use `ctest --test-dir build -C Release --show-only=json-v1` (save the stdout to a file). Distinct single-config Linux/macOS builds can use the same invocation; their configured test count must be reviewed rather than assumed identical.

Full local Release regression on Windows:

```powershell
ctest --test-dir build -C Release --output-on-failure --parallel 4
```

GitHub Actions uses a three-platform native Release build and CTest for non-Markdown changes. Markdown-only changes may be handled by the documented CI change classifier instead. The frozen 500M run is never part of this suite.

## Phase A+B structural invariants

After the CMake/layout migration:

1. All existing 180 test *identities* in the baseline configuration remain registered; duplicate registrations and silent omission of required scenes are not acceptable.
2. The CMake integration files are included from `tests/CMakeLists.txt` so its variable scope and `CMAKE_CURRENT_BINARY_DIR` stay unchanged. Each relocated script refers to the original test-root explicitly instead of relying on its new `CMAKE_CURRENT_LIST_DIR`.
3. Only test-driver source locations are changed. Existing fixtures, test execution arguments, program entrypoints, stdout formats, fixed seeds and scientific references are unchanged.
4. The relocated GUI acceptance helper remains test-only under `BUILD_TESTING=ON` and inert without its existing explicit environment variables; actual GUI automation is Phase D, not falsely claimed by A+B.
5. CMake configure/build/CTest must succeed locally and on the Windows, Ubuntu and macOS GitHub Actions matrix before A+B is marked accepted.

## Remaining phase gates

- **C - Reliability:** consistent safe temporary paths, subprocess timeouts, meaningful failure artifacts, no missing mandatory tests, bounded sequential/parallel runs, CI-readable test reports.
- **D - GUI:** real Qt/SoQt/OpenGL GUI operations on a proven display runner; native Run, Script Editor and Flux Analysis; trusted numerical comparisons; fail-safe exit; no routine 500M benchmark.
- **E - Installed:** disposable external install, GUI/CLI startup, native plugin loading and small exports using installed dependencies rather than developer SDK/build paths. Verify Windows/Linux/macOS separately and distinguish runtime acceptance from complete IFW updater validation.

Do not combine unsupported GUI, installed and CLI claims. Record any scientific reference provenance and exact platform/build scope alongside the test result. Review `PROJECT_STATUS.md` before continuing, and update its validation/branch/commit state only after evidence is available.

## Phase C: execution safety, a real fast tier, and CI evidence

- CTest configures with mandatory Python and checked-in scientific fixtures: a missing required prerequisite must fail configuration, not silently reduce the test inventory. CI checks the platform baseline (180 Windows, 179 Ubuntu/macOS) and required sentinels; the one Windows-only test is `scientific.ScientificPluginPaths.WindowsBuildTreeIncludesMaterialPluginRoot`.
- `ctest --test-dir build -C Release -L fast --output-on-failure` is a deliberate, small **15-test** quick tier (13 mathematical/random GoogleTests plus the CLI help/invalid-arguments checks). The historical `smoke` label still includes long scientific tests and is **not** a quick tier. Full `ctest --test-dir build -C Release --output-on-failure` remains the authoritative native regression.
- Recursive cleanup in registered CTest drivers is routed through `tests/cmake/PrepareTestOutput.cmake`; it accepts only a named direct child of an existing CTest build root, rejects redirected/symlink paths, and never deletes arbitrary source or user directories. The guard is covered by synthetic Python `unittest` cases, including deliberate failure paths.
- Nested CMake `execute_process()` calls in the reviewed drivers have their own bounded timeouts, subordinate to applicable CTest limits. This improves failure containment without changing the test's scientific inputs or the actual comparator assertions.
- CI emits `build/ctest-inventory.json` and `build/ctest-results.xml` (CTest JUnit), included in the existing build artifact upload. These reports establish build-tree regression coverage only; they do not claim native GUI, installed-runtime or installer acceptance.
- The immutable frozen 500-million-ray reference and the opt-in full benchmark driver remain unchanged and must not be executed by routine CI or this refactor.

## Phase D: native GUI scientific acceptance (opt-in validation candidate)

The initial Phase D suite is separate from the accepted build-tree CTest
inventory until a **real display/Qt/SoQt/OpenGL runner** has demonstrated
success. It is not a substitute for the full native regression suite, and it
does not run the frozen 500-million-ray benchmark.

Configure a separate test-enabled build with
`-DTONATIUHPP_ENABLE_NATIVE_GUI_ACCEPTANCE=ON`, or reconfigure the
already-tested Release build (the flag affects only CTest registration):

```powershell
cmake -S source -B build -DTONATIUHPP_ENABLE_NATIVE_GUI_ACCEPTANCE=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release -N -R '^gui[.]native_'
ctest --test-dir build -C Release -R '^gui[.]native_' --output-on-failure
```

Running the real GUI may take focus. The test runner starts separate genuine
`tonatiuhpp-gui` processes and executes actual Ray Tracing, Script Editor
and Flux Analysis widgets. Do not use `QT_QPA_PLATFORM=offscreen` or
`QT_QPA_PLATFORM=minimal`; a working OpenGL/SoQt display is required.
For a verified Linux X11/GLX runner, optionally configure with
`-DTONATIUHPP_GUI_ACCEPTANCE_USE_XVFB=ON` and install `xvfb-run`
and Mesa GLX. No OS is automatically claimed supported merely because it
can compile the GUI or run headless CTests.

Three distinct CTests are registered when explicitly enabled:

- `gui.native_interactive_file`: real widget-selected 2,048-ray File
  photon export on the existing cylinder scene, compared as canonical photon
  record multisets and power metadata with an independent 2,048-ray CLI run
  using the same seed. This is real **GUI/CLI cross-interface evidence**, not
  two CLI runs.
- `gui.native_seeded_interception`: actual Script Editor invokes the
  existing 2,048-ray planar 8 m x 8 m absorber fixture; its scientific CSV
  must satisfy the independently known 64,000 W ideal direct-normal
  interception at 1,000 W/m² with an initial conservative 20% Monte Carlo/
  implementation tolerance. The fixture itself is unchanged.
- `gui.native_interactive_flux`: actual 8x8 Qt color map and visible flux
  statistics must agree with their own 1 m²-cell integral and max; native GUI
  Script Editor TSV outputs must also satisfy the 64,000 W analytical total
  (20% tolerance) and agree with the Flux dialog's independently executed
  total within the same bound.

These thresholds are **candidate assertions** pending real three-OS evidence;
do not narrow or relax them merely to make CI green without assessing physics
and fixture provenance. The analytic reference assumes a zenith-illuminated,
fully absorbing 8x8 m planar surface and normal irradiance 1,000 W/m².
Comparing two outputs alone is not independent physical validation.

Each mode gets a fresh unique output directory within the configured build
test root; it retains the native JSON report, `acceptance.json`, stdout/stderr,
CLI logs where applicable, script outputs and Qt flux PNG, even on failure.
The GUI helper's 120 s watchdog is supplemented by a 145 s process timeout
and a 180 s CTest timeout. The real GUI helper remains test-only under
`BUILD_TESTING=ON` and inert without its explicitly set environment variables.

Python tooling checks (run without a native GUI or a display):

```powershell
python -m unittest discover -s tests/tooling -p 'test_*.py' -v
```

**Acceptance is pending.** Before enabling required GUI gates, capture real
Windows, Ubuntu and macOS results independently, review the scientific
numerical reports and runner graphics setup, and compare CTest identities with
the accepted default 180/179 inventory. Keep Phase E installed-runtime
acceptance separate.

### Phase D acceptance evidence and required graphical CI

Native GUI acceptance was verified on three genuine GUI backends on
2026-11-12: a maintainer Windows Release full CTest run (**183/183**, three
GUI workflows), hosted Ubuntu 24.04 X11/GLX Mesa `llvmpipe` (**3/3 native
GUI workflows**) and hosted macOS Cocoa (**3/3 native GUI workflows**).
The ordinary three-platform build-tree CI checks passed on the same PR.
Linux and macOS outcomes were checked in the graphical step logs rather
than inferred from their green parent jobs; early pilots were
`continue-on-error` and therefore could not prove acceptance from their
overall job status alone.

With Phase D enabled only for the separate graphical CI steps, the regular
`ctest` inventory remains **180 Windows / 179 Linux and macOS**. The
Linux and macOS graphical steps now become **required CI gates**:
they configure `TONATIUHPP_ENABLE_NATIVE_GUI_ACCEPTANCE=ON`, explicitly
verify discovery of three GUI CTests and run all three with verbose numerical
output/JUnit evidence. Linux uses genuine X11/GLX through `xvfb-run`,
and macOS uses Cocoa with the generated-link AGL scrub after CMake
reconfiguration. An initial hosted Windows graphical probe is exploratory
(`continue-on-error`), because a local physical Windows desktop and a
hosted Windows session are different environments.

Numerical evidence from run
[43185720709](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43185720709):
  
| Metric | Windows local | Linux CI | macOS CI |
| --- | ---: | ---: | ---: |
| GUI/CLI canonical photon records (each) | 3184 | 3184 | 3197 |
| Photon power (W/photon) | 0.0719665 | 0.0719665 | 0.0719665 |
| Script interception power (W) | 63857.145538085264 | 63857.145538085264 | 63889.13809597008 |
| Qt 8×8 flux integral (W) | 63857.145538085264 | 63857.145538085264 | 64225.0599537606 |
| GUI script TSV flux integral (W) | 63857.114 | 63857.114 | 64225.057 |
| Independent analytical reference (W) | 64000 | 64000 | 64000 |

**Interpretation:** GUI/CLI photon equality is **within each platform**;
differences in seeded ray sampling and photon count across platforms are not
silently treated as exact cross-platform equivalence. The bounded analytical
planar-reference power assertion uses a conservative 20% threshold for these
small fixtures, not a universal uncertainty estimate or validation of all
solar optical models. Independent GUI Script Editor and Qt Flux Dialog results
are compared at their respective precisions. The 500-million-ray frozen
heliostat benchmark has not been rerun or modified.

The initial Phase D graphical pilot evidence was accepted for its tested scope.
Final hosted-Windows remediation and the required three-OS graphical CI status
are documented below. Installed package/runtime acceptance remains **Phase E**.

### Windows hosted CI — OpenGL crash diagnosis and verified remediation

The hosted Windows GUI originally crashed before opening its first test dialog.
WER and CDB in [diagnostic run 43212203419](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43212203419)
identified an access violation `0xC0000005` at
`Qt6Gui!QOpenGLFunctions::glViewport`: the current OpenGL dispatch pointer
was null during a native Qt `QOpenGLWindow` expose/flush callback, yielding
the outer process exit `0xC000041D`. The hosted machine *did* have a
1024×768 `WinSta0\\Default` desktop and used `Microsoft Hyper-V Video`.
This was a Qt/OpenGL runtime problem before scientific calculations, not
evidence of an incorrect photon export or flux implementation.

[Remediation run 43220096714](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43220096714)
passed **180/180 ordinary Windows CTests and 3/3 native Windows GUI CTests**
using Mesa3D **25.1.9 MSVC** software WGL. Actual results:
GUI/CLI photon records **3184/3184** with canonical record equality,
interception **63857.145538085264 W**, Qt map integral
**63857.145538085264 W**, script flux **63857.114 W**,
against the independently defined **64000 W** planar reference.
These match the real Windows desktop results.

On CI only, `scripts/install_ci_windows_mesa.ps1` downloads
`mesa3d-25.1.9-release-msvc.7z` from
`pal1000/mesa-dist-win`, checks SHA-256
`dd83c3c4cc4d7706c9e21b277388447a2fc5367b0912f9ee45621e36aeb3b7d2`,
verifies AMD64 PE architectures for its required WGL libraries,
stages x64 software-OpenGL DLLs beside the disposable build GUI, and removes
them before artifact upload. It does not install global drivers or alter
the production physics, RNG, kernel, packaging, or installed GUI.
The real `QT_QPA_PLATFORM=windows` and native SoQt/GUI workflow are retained,
not replaced by `offscreen` or `minimal`. This establishes software-OpenGL
acceptance, not hardware GPU qualification.

Following this verified exploratory success, the Windows native GUI step
becomes a **required** CI gate alongside Linux X11/GLX and macOS Cocoa.
The first new required-gate run must itself pass before marking Phase D
ready for final maintainer acceptance. The frozen 500-million-ray benchmark
remains unchanged.

## Phase E — exploratory relocated CMake-install acceptance (2026-11-12)

Phase E starts **after merged Phase D PR #45** (master merge
`4c4121f5e52ac354dbe4d67bcb5472946d74be05`).
Unlike build-tree GUI testing, installed acceptance must launch actual
**installed** GUI/CLI executables and their scientific plugins from a
relocated prefix outside the source and build directories. The independent
test scripts and comparators still originate in the checkout; the
installed `examples/benchmarks/cylinder.tnhpp` and
`sphere.tnhpp` scenes are intentionally loaded from the relocated
payload, not source fixtures.

The initial cross-platform pilot runs
`cmake --install build --config Release --prefix <runner-temp>/cmake-install`,
moves that disposable install to another directory, and invokes
`tests/integration/installation/run_installed_acceptance.py`.
It reuses `cli_process_repeatability.py --mode installed` for independent
native CLI processes, installed material/scene loading, bounded small
scientific traces, scripts, export files and fail-closed behavior. It then
reuses all three real GUI workflows via the same independent
GUI/CLI and analytical 64 kW assertions as Phase D, launched from
the relocated installed executables. The spawned CLI/GUI processes
do not inherit `LD_LIBRARY_PATH`, `DYLD_LIBRARY_PATH`,
`QT_PLUGIN_PATH`, `QT_QPA_PLATFORM_PLUGIN_PATH` and related SDK
loader overrides; Windows child `PATH` contains only relocated runtime
directories and normal Windows system DLL directories. Test evidence
and process logs are retained under `build/tests/installed_acceptance`.
All three platform pilots are deliberately **non-gating until verified**.
Inspect individual results, not just a green parent CI job.

This first gate uses the existing **BUILD_TESTING=ON** CMake installation
so the test-only GUI helper can exercise real dialogs. Such an installed
runtime is useful for packaging diagnosis, but does **not** establish that
a `BUILD_TESTING=OFF` production app, Qt IFW installer, MaintenanceTool
updates, signing, or notarization have passed. Those remain separate
Phase E release-payload gates. macOS requires `macdeployqt` for this
pilot; full app-bundle isolation (Frameworks/private libs/plugin RPATH)
must be assessed rather than inferred from CLI process success.

No frozen 500-million-ray benchmark is run or regenerated. All installed
work is confined to new external CI temporary directories, not a
developer/user installation.

### First relocated-install pilot evidence (run 43269447246)

All three ordinary platform CI jobs passed their existing build-tree
regressions and native GUI acceptance. Their **new Phase E pilots did not
pass**, despite green parent jobs because those pilots were non-gating:

- **Linux:** installed CLI scientific acceptance passed, including real
  installed scene and material-plugin loading. Relocated GUI exited 127
  before any scientific test: `libQt6OpenGL.so.6` was not found. The
  CMake install log shows that library present in installed `lib/`;
  this is consistent with ELF `RUNPATH` nontransitivity. The follow-up
  change gives each installed shared library a local `$ORIGIN` RUNPATH.
- **macOS:** the raw CMake-installed CLI aborted with
  `Library not loaded: @rpath/libTonatiuhppKernel.dylib`.
  `macdeployqt` alone does not consolidate Tonatiuh++ private
  libraries. The follow-up pilot reuses the release workflow's macOS
  app bundle consolidation, stages private Coin/SoQt and QtOpenGL
  dependencies, and loads installed examples from inside the bundle.
- **Windows:** the relocated CLI scientific tests passed, and the
  pinned CI Mesa software WGL runtime was staged. The installed GUI
  then exited `3221225781` (`0xC0000135`), indicating a missing
  DLL before scientific tests. In addition to `windeployqt`, the
  follow-up install includes the indirect Qt6OpenGL runtime DLLs needed
  by SoQt. Which DLL the loader lacked in the initial run is not
  proven by the exit code alone.

These are **first-pilot diagnoses and proposed remedies, not accepted
three-platform installed runtime results**. The separate second CI
run must prove each relocated app exercises the actual GUI science.
These tests do not execute the frozen benchmark and do not validate
`BUILD_TESTING=OFF` IFW installers or updater lifecycle.

### Phase E second-pilot results and consolidated validation (2026-11-12)

The latest completed Phase E exploratory run
[43272756323](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43272756323)
retained the ordinary three-OS CI regression and required Phase D native
GUI gates. Its *non-gating* relocated-install pilot actually **passed
on Linux and macOS**: installed CLI scientific repeatability, real installed
GUI photon export, seeded interception, and interactive flux analysis all
completed after installation and prefix relocation.

**Windows remained blocked:** the relocated CLI passed, the pinned Mesa
WGL deployment completed, and the installed GUI returned
`0xC0000135` before numerical acceptance. Independent inspection of the
actual Windows build artifact's PE import table found that
`TonatiuhppGuiPresentation.dll` imports `Qt6PrintSupport.dll` and
`SoQt1.dll` imports `Qt6OpenGL.dll`. The captured build runtime
contained `Qt6OpenGL.dll` but **did not contain
`Qt6PrintSupport.dll`**; copying Qt6OpenGL alone was therefore not
sufficient. The candidate installer fix now explicitly requires and
deploys Release `Qt6PrintSupport.dll`, `Qt6OpenGL.dll` and
`Qt6OpenGLWidgets.dll` and refuses missing DLLs during installed-GUI
preflight. Actual Windows installed-GUI success remains contingent on
the next run; a PE import audit identifies missing dependencies but
is not itself a successful runtime test.

**Additional release-like pilot:** CI now also reconfigures to
`BUILD_TESTING=OFF`, rebuilds and installs into a fresh disposable
prefix, and runs independent installed CLI scientific acceptance plus
a bounded actual native GUI process-start/liveness check. It does
**not** use Phase D's test-only GUI automation hooks and must not be
reported as equivalent dialog/flux-map acceptance.
The pilot is non-gating until its numeric and launch evidence is
inspected. Successful CMake install, app-bundle staging and production
smoke do **not** by themselves prove Qt IFW installer execution,
maintenance/update behavior, notarization, signing or operation with
a different operating-system dependency baseline.

The GUI-only DLL checks have negative/positive Python self-tests,
which must fail closed when PrintSupport is omitted. Both preliminary
and production runs keep the frozen 500-million-ray benchmark and
scientific kernel unchanged.

### Phase E production Qt IFW *payload* acceptance pilot

In addition to the relocated CMake install, the Phase E CI now
uses the existing `installer/prepare_ifw_payload.py` from a disposable
`BUILD_TESTING=OFF` CMake staging tree. Its generated Qt IFW
`installer/packages/com.tonatiuhpp.app/data` is copied to a separate
external runtime prefix, relocated again, and exercised with the
same production CLI scientific checks and real native GUI startup
smoke. The resulting manifest and per-process evidence are retained
under `build/tests/ifw_payload_acceptance`.

This is a **payload-content/runtime** check, not installation of a
`binarycreator`-generated Qt IFW executable, an unattended installer
transaction, an updater or uninstaller test, signing or notarization.
It remains **non-gating until all three OS pilots actually pass**.
Using source test scripts as comparators is permitted; the launched
binaries, plugins and scene files must originate from the copied IFW
payload, not the source/SDK/build tree. The 500-million-ray frozen
benchmark remains unchanged.

### Phase E three-platform pilot evidence and promotion to required CI

The completed [run 43304366649](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43304366649)
has independently verified all **three** non-gating Phase E checkpoints
on **Windows, Ubuntu 24.04 and macOS**. In every case, the individual
steps returned successful process exit status and generated their own
`INSTALLED_ACCEPTANCE_RESULT=PASS` structured evidence:

| Test contract | Windows | Ubuntu | macOS |
| --- | --- | --- | --- |
| Relocated `BUILD_TESTING=ON` installed CLI plus all three real GUI scientific modes | PASS | PASS | PASS |
| Relocated `BUILD_TESTING=OFF` production CLI scientific acceptance and real native GUI process startup | PASS | PASS | PASS |
| Relocated production Qt IFW `package data` CLI scientific acceptance and real native GUI process startup | PASS | PASS | PASS |

The test-enabled installed GUI photon export remains an **independent
GUI/CLI canonical multiset equality** check: 3184/3184 records on
Windows and Linux, 3197/3197 on macOS. GUI script interception is
63857.145538085264 W on Windows/Linux and 63889.13809597008 W on
macOS; Qt flux-map integrals are 63857.145538085264 W on
Windows/Linux and 64225.0599537606 W on macOS. These are evaluated
against a bounded analytically derived 64000 W reference; no claim of
exact cross-platform RNG identity is implied.

On the production binaries and packaged payload, GUI evidence is
**native GUI startup/liveness only** (seven seconds with actual
Windows/X11/Cocoa platform), not production GUI dialog automation
or full interactive flux comparison. Numerical scientific acceptance
there comes from independently launched *production CLI* processes.
Qt IFW package-data tests do **not** execute an installer/updater/
uninstaller, binary signing, notarization, or GPU-hardware qualification.

All three validated Phase E contracts are now candidates for
**required CI checks**, not `continue-on-error`, with a final CI run
necessary before Phase E approval. The macOS application consolidation
adds a loader-path audit to reject absolute external development/Qt
SDK references in its Mach-O binaries and framework/plugin contents.
This additional audit must itself pass the final required CI run.
Frozen 500-million-ray scientific benchmarks and the production
scientific kernel remain unmodified.

### Shared macOS release packaging implementation

The Release workflow now invokes the **same** bundle-consolidation
script used in Phase E's relocated installed-runtime acceptance,
`scripts/prepare_macos_installed_pilot.sh`, instead of maintaining
a separate inline dylib/Frameworks rewrite. The script accepts only
disposable GitHub Actions runner temporary install prefixes or the
release job's exact `${GITHUB_WORKSPACE}/dist` directory; it rejects
unrelated paths. The existing release `.app` architecture/payload
checks remain in the workflow.

This ensures the release packaging *procedure* shares its tested
implementation. It does **not** imply that a release-tag workflow,
installer GUI/update lifecycle, signing, notarization or published
distribution has actually passed. Those require separate execution
and evidence; Phase E's production CMake and IFW-payload runtime
checks establish only the scopes described above.

## Post-D2 native GUI scene-edit Undo/Redo regression (candidate)

The opt-in fourth GUI CTest, `gui.native_scene_undo_redo`, uses the actual `Fresnel.tnhpps` example through the GUI Script Editor. It performs Cut, Insert/Rename, nested Paste, Copy/Paste, and translation, then three complete 7-command Undo/Redo cycles. It verifies scene-tree node counts, retained Coin3D identities and restored/translated values at every step, including selection invalidation. Failures and evidence are retained alongside existing native GUI reports. This does not change the standard build-tree CTest inventory (203 Windows / 202 Linux and macOS), scientific inputs, RNG, or the frozen 500M reference. The extra mode is test-build-only, not production behavior. Validate it on Windows Release and the native Windows/Linux/macOS GUI CI matrix before acceptance.
