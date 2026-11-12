# Tonatiuh++ testing architecture

Baseline date: 2026-11-11. This policy applies to the two-executable, single-scientific-kernel architecture introduced before PR #41. It does not authorize release or changes to scientific references.

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
| GUI native acceptance | Actual Qt dialog and Script Editor interaction | `tests/integration/gui/` (driver is opt-in, not yet CTest registered) |
| Installed runtime | External install and plugin/runtime discovery | Planned `tests/integration/installation/`; release/installer workflow |
| Python tooling | Python stdlib `unittest` of auditors, comparators, CI classifier | `tests/ci/`, `tests/scientific/` and future `tests/tooling/` |

For A+B, `tests/cmake/`, `tests/scripts/` and `tests/scientific/fixtures/` remain in place to avoid changing scientific data and existing runtime contracts. Historical milestone strings in report schemas, environment variables, scripts and reference artifacts are not generic rename targets.

## Reproducible inventory

After a configured Release build, use the bundled inventory helper to capture all CTest registrations, labels, commands, configured timeouts and conservative inferred categories:

```powershell
python scripts/ctest_inventory.py --build-dir build --config Release --json-out build/test-inventory.json --markdown-out build/test-inventory.md --expect-count 180
```

Save the baseline JSON outside the source tree before changing registrations. To compare after a CMake restructure:

```powershell
python scripts/ctest_inventory.py --build-dir build --config Release --baseline build/test-inventory-before.json --json-out build/test-inventory-after.json --markdown-out build/test-inventory-after.md --expect-count 180
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
