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
