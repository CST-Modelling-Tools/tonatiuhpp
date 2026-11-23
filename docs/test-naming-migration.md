# Test-name migration (G0 Windows validated; CI pending)

This document records an explicit, behavior-based replacement for the remaining
tracked m3/m4/m5 **test source filenames** after merged PR #59 (`81b21b30c2f8`).
Renaming is not scientific implementation work. No photon, RNG, model, GUI
production, or reference data may be changed. Git rename detection is expected
when these files are staged for review.

## Exact file renames

| Prior tracked test file | New, behavior-oriented file |
| --- | --- |
| `tests/cmake/run_heliostat_benchmark_m5.cmake` | `tests/cmake/run_heliostat_benchmark.cmake` |
| `tests/cmake/run_m3_exit_phase3a_field_grid.cmake` | `tests/cmake/run_script_field_grid_csv.cmake` |
| `tests/cmake/run_m3_exit_phase3b2b1_file.cmake` | `tests/cmake/run_script_file_export_safety.cmake` |
| `tests/cmake/run_m3_exit_phase3b2b2a_append.cmake` | `tests/cmake/run_script_file_export_append_integrity.cmake` |
| `tests/cmake/run_m3_script_cancel_smoke.cmake` | `tests/cmake/run_script_cancellation_recovery.cmake` |
| `tests/cmake/run_m3_script_relative_io.cmake` | `tests/cmake/run_script_relative_file_io.cmake` |
| `tests/cmake/run_m3_shared_run_parity.cmake` | `tests/cmake/run_scripted_run_repeatability.cmake` |
| `tests/cmake/run_m4_cli_file_export_parity.cmake` | `tests/cmake/run_file_export_cli_script_equivalence.cmake` |
| `tests/cmake/run_m4_extended_file_export.cmake` | `tests/cmake/run_segmented_file_export_equivalence.cmake` |
| `tests/scientific/compare_m4_extended_exports.py` | `tests/scientific/compare_segmented_photon_exports.py` |
| `tests/scientific/verify_m3_exit_phase3b2b2_append.py` | `tests/scientific/verify_file_export_append.py` |
| `tests/scripts/m3_2_interception_bounded.tnhpps` | `tests/scripts/script_interception_bounded.tnhpps` |
| `tests/scripts/m3_2_interception_error.tnhpps` | `tests/scripts/script_interception_invalid_surface.tnhpps` |
| `tests/scripts/m3_3_cancel_inactive.tnhpps` | `tests/scripts/script_cancel_without_active_trace.tnhpps` |
| `tests/scripts/m3_3_cancel_invalid.tnhpps` | `tests/scripts/script_trace_invalid_output_policy.tnhpps` |
| `tests/scripts/m3_3_cancel_recover.tnhpps` | `tests/scripts/script_cancellation_recovery.tnhpps` |
| `tests/scripts/m3_3_cancel_unfinished.tnhpps` | `tests/scripts/script_unfinished_trace_cleanup.tnhpps` |
| `tests/scripts/m3_3_gui_headless_parity.tnhpps` | `tests/scripts/script_gui_headless_interception.tnhpps` |
| `tests/scripts/m3_3_tracker_parity.tnhpps` | `tests/scripts/script_tracker_preparation_repeatability.tnhpps` |
| `tests/scripts/m3_exit_phase2_sun_smoke.tnhpps` | `tests/scripts/script_sun_position.tnhpps` |
| `tests/scripts/m3_exit_phase3a_field_grid_smoke.tnhpps` | `tests/scripts/script_field_grid_csv.tnhpps` |
| `tests/scripts/m3_exit_phase3b2b1_run_file.tnhpps.in` | `tests/scripts/script_file_export_safety.tnhpps.in` |
| `tests/scripts/m3_exit_phase3b2b2_append.tnhpps.in` | `tests/scripts/script_file_export_append.tnhpps.in` |
| `tests/scripts/m3_flux_shared_parity.tnhpps` | `tests/scripts/script_flux_shared_backend.tnhpps` |
| `tests/scripts/m3_shared_run_parity.tnhpps.in` | `tests/scripts/scripted_run_repeatability.tnhpps.in` |
| `tests/scripts/m4_cli_file_export_parity.tnhpps.in` | `tests/scripts/file_export_cli_script_equivalence.tnhpps.in` |
| `tests/scripts/m4_extended_file_export.tnhpps.in` | `tests/scripts/segmented_file_export_equivalence.tnhpps.in` |

## Intentionally retained evidence and interfaces

The script updates all references to renamed files, generated test script
**filenames**, fixture selection keys, the matching stdout success markers,
and renamed CMake/Python local helper identifiers. Where build-tree output leaf
names change, their literal cleanup-guard allowlist arguments change with them.
Never disable or weaken `PrepareTestOutput.cmake` guards to make tests pass.

Historical `m3`/`m4`/`m5` strings may still appear in intentionally unchanged
**serialized CSV/JSON schema identifiers**, scientific result column names,
scene-node names, output data filenames, native GUI environment variables,
and the historical `PROJECT_STATUS.md` archive. Those are not test identities;
changing them could silently alter the scientific fixtures, external consumer
contracts or baseline comparisons. Do not bulk-replace them.

The two new `tests/tooling/test_test_names.py` checks prevent reintroduction of
milestone-based tracked test filenames and literal CTest/GoogleTest names.
They increase the separate Python tooling inventory by two tests; the CTest
inventory must retain every existing name and registered count.

## Windows Release validation (maintainer-reported PASS, 2026-11-22)

- 207/207 CTests passed (0 failures), 212.58 seconds; all four native
  GUI acceptance CTests were included.
- Python tooling: 42 tests executed, 39 passed, 3 skipped because Windows
  directory-symlink privileges were unavailable.
- Both new behavior-based test-name guards passed.
- No hosted cross-platform CI, PR, merge, installed-runtime or release
  acceptance is established by this result.

## Local reproduction commands

```powershell
cmake -S source -B build -DBUILD_TESTING=ON -DTONATIUHPP_ENABLE_NATIVE_GUI_ACCEPTANCE=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release -N
ctest --test-dir build -C Release --output-on-failure
python -m unittest discover -s tests/tooling -p "test_*.py" -v
```

Expected **Windows**: 207 CTests unchanged, 42 Python tooling tests
(39 passes/3 existing Windows directory-symlink skips if privileges unchanged).
Linux/macOS registration expectation must be confirmed independently.
Review the exact rename/call-site diff and require three-platform CI
before a maintainer-authorized merge. The G0 guard covers test filenames
and registered CTest/GoogleTest identities, not all test-owned evidence
payloads, scene URLs, schemas, environment keys or diagnostic labels.
Coordinate any later changes with their producers and consumers.
Do not run/regenerate the frozen 500-million-ray reference or claim
release or performance acceptance.

## G1 test-owned evidence and scene-name migration (candidate)

This second stage is NOT yet Windows/CI validated. G0 remains Windows validated.

| Before | After |
| --- | --- |
| `M3_2_Headless_Document` | `ScriptSceneDocument` |
| `M3_2_Insert` | `ScriptInsertedNode` |
| `M3_2_Interception` | `ScriptInterceptionFixture` |
| `M3_3_Parity` | `ScriptInterception` |
| `M3_3_Saved` | `ScriptSavedNode` |
| `M3_3_Tracking` | `ScriptTracking` |
| `M3_Edit` | `ScriptEditedNode` |
| `M3_Flux_Parity` | `ScriptFlux` |
| `M3_Phase3A` | `ScriptFieldGrid` |
| `m3_3_cancel_recovery.csv` | `script_cancellation_result.csv` |
| `m3_3_parity_result.csv` | `script_interception_result.csv` |
| `m3_3_tracker_result.csv` | `script_tracker_result.csv` |
| `m3_exit_phase3a_result.csv` | `script_field_grid_result.csv` |
| `m3_flux_back.txt` | `script_flux_back.txt` |
| `m3_flux_front.txt` | `script_flux_front.txt` |

Selected script-only diagnostics and synthetic filenames are also renamed.
Versioned evidence schemas, test-hook environment variable names,
historical records, geometry, scientific data, RNG and frozen 500M reference
are intentionally unchanged. The additional Python guard protects
G1's test-owned artifact and scene identifiers. After application,
rerun all 207 Windows CTests and the 43 Python tooling tests; verify
hosted Windows/Linux/macOS CI before considering the PR merge.

## G1.1 follow-up — Windows validation blocker and correction

The G0+G1 Windows Release native CTest suite passed 207/207 (228.64 seconds).
Python tooling reported 43 tests: 39 passed, one failure in the new content
naming guard, and three expected Windows directory-symlink skips. The
failure named exactly two surviving CTest-owned scratch-directory prefixes
and the existing 500-million-ray audit output directory contract.

The G1.1 correction coordinates both path producers and cleanup allowlists:

| Previous disposable output leaf | Updated output leaf |
| --- | --- |
| `m3_exit_phase3a_field_grid` | `script_field_grid_csv` |
| `m3_script_relative_io` | `script_relative_file_io` |

For the frozen 500-million-ray **audit only**, `m5_full_<10 hex>` is an
established provenance/path validation contract, also exercised in
`test_audit_500m_benchmark.py` using a synthetic directory. The naming
rule excludes only these exact values inside those two files; every other
milestone-named test-owned scene/result identifier remains rejected. It
would be incorrect to rename this historic output format during the test
naming cleanup, or to run the frozen benchmark. The correction has not yet
passed Windows tests or hosted CI. Rerun the full 207 CTests and 43 tooling
tests before accepting it.

## G2 — active M3/M5/A0 test interfaces (candidate)

G2 follows the G0 filename migration and G1/G1.1 scene/artifact and scratch
path migration. These remaining identifiers are *active test interfaces*, not
scientific baselines. Each producer/consumer pair changes in the same patch:

| Prior identifier | Behavior-oriented replacement |
| --- | --- |
| `TONATIUHPP_M5_NATIVE_GUI_TEST` | `TONATIUHPP_NATIVE_GUI_TEST` |
| `TONATIUHPP_M5_GUI_NATIVE_*` | `TONATIUHPP_NATIVE_GUI_*` |
| `TONATIUHPP_M3_TEST_FILE_DATA_FAULT` | `TONATIUHPP_TEST_PHOTON_FILE_DATA_FAULT` |
| `TONATIUHPP_A0_TRACE_SIGNATURE_FILE` | `TONATIUHPP_TEST_TRACE_SIGNATURE_FILE` |
| `TONATIUHPP_A0_RAY_HISTORY_FILE` | `TONATIUHPP_TEST_RAY_HISTORY_FILE` |
| `tonatiuhpp.m5.native-gui-presenter.v1` | `tonatiuhpp.native-gui-acceptance.v1` |
| `tonatiuhpp.m5.2.cli-process.v1` | `tonatiuhpp.cli-process-acceptance.v1` |
| `tonatiuhpp.phase-d.native-gui.v1` | `tonatiuhpp.native-gui-summary.v1` |
| `m3.3-script-parity-v1` | `script-interception-v1` |
| `m3.3-tracker-parity-v1` | `script-tracker-v1` |
| `m3.3-script-cancel-v1` | `script-cancellation-v1` |
| `m3-exit-phase3a-v1` | `script-field-grid-v1` |

Other internal CMake functions/variables and in-test helper identifiers lose
milestone prefixes. The former P2 architecture test runner becomes
`tests/architecture/verify_runtime_binaries.py`, and the generated
`p2-artifacts-<CONFIG>.txt` build inventory becomes
`runtime-artifacts-<CONFIG>.txt`. The *explicit* format/metadata schema change means old
non-frozen acceptance reports are not automatically interpreted by the new
validators; this is a documented test-evidence migration, not a silent
backwards-compatible alias. The historical frozen 500-million-ray benchmark
and its `m5_full_*` audit identity are preserved. No actual 500M rays are run.

After application obtain a new Windows Release configure/build/207 CTest run,
44 Python tooling checks, and hosted three-platform PR CI before merging.

## G2.1 - retained full-benchmark directory identity

The first post-G2 Python tooling run (44 tests) had one failure: the G1
scene/artifact naming guard flagged `m5_full_` in the CMake benchmark runner.
This was a **guard omission**, not a new obsolete test fixture. G2 kept
`m5_full_<10 hex>` when `MODE=full` specifically to preserve the unchanged
independent 500-million-ray audit contract, and uses `heliostat_<mode>_<id>`
for routine outputs.

The naming guard now permits this exact identifier in three files only:
`tests/scientific/audit_500m_benchmark.py`, its synthetic audit unit test,
and `tests/cmake/run_heliostat_benchmark.cmake`. For the CMake runner it also
verifies the exact full-mode/routine-mode output selection and rejects any
additional `m5_full_` uses. Other milestone-coded active interfaces and
fixture names remain forbidden. No CMake producer, auditor, frozen evidence,
scientific calculations or ray-tracing behavior is changed.

Full post-G2.1 Python and Windows-native validation, hosted three-platform
CI and merge remain pending.

## Windows local acceptance — G0 through G2.1 (2026-11-23)

After G2.1 the maintainer reported **207/207 Windows Release CTests passing**
(zero failures, 226.68 s), including **four real native GUI tests**. Python
tooling reported **44 tests: 41 passed, three expected Windows symlink
privilege skips**; **all four** registered-test, test-filename, test-owned
artifact/scene, and active-interface naming safeguards passed. Earlier G1
and G2 naming-guard failures in this document are historical, not current.

Only the historical full-run `m5_full_<10 hex>` directory contract remains
intentionally milestone-shaped, in the full-mode generator, unchanged
independent frozen-benchmark auditor, and its synthetic auditor test. The
full 500-million-ray benchmark and frozen scientific references were neither
changed nor rerun. The combined migration is locally Windows-validated but
**not committed, pushed, opened as a PR, CI-validated, or merged**. Next:
review and stage changes, obtain explicit authorization for Git remote
operations, and require hosted Windows/Linux/macOS CI before merge.
