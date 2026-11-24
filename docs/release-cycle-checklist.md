# Release-cycle acceptance and performance policy

This is the reusable checklist for **every future Tonatiuh++ release**. The already-published **v0.1.8.28** is the previous-release baseline for the next proposed release (tagged source `1bbef1ee6924b38106f4d16132303c2529db387a`); historical version-specific checklists remain historical. Re-evaluate this baseline after each new release rather than silently retaining old version numbers. This file does not authorize publication or tagging. The maintainer has approved a fresh, manual **500-million-ray candidate simulation** as a mandatory release gate; it does **not** authorize regenerating or altering the frozen historical 500M reference.

## Development cadence and release gate

- [ ] Alternate substantial **functionality** work with evidence-driven **architecture/coding-practice** work. Avoid cosmetic micro-refactor cycles.
- [ ] Finish the current cycle and publish a validated release **before** switching to the other development focus. If acceptance fails, keep the cycle open; never publish a failing release merely to advance the cadence.
- [ ] Identify the last *published* stable release, proposed version and exact baseline/candidate Git SHAs. Confirm clean/synchronized candidate `master` and review the actual diff and release notes.
- [ ] Confirm the version in `source/CMakeLists.txt`, `CITATION.cff`, IFW metadata, release notes and proposed tag is consistent; use `installer/sync_ifw_metadata.py` with the intended tag.
- [ ] Obtain maintainer authorization for the release tag and publication; do not assume a successful PR, build or branch merge authorizes release.

## Functional, scientific and cross-platform validation

- [ ] Validate the exact release candidate with Windows, Linux and macOS CI and appropriate native tests, Python automation/tooling, GUI/CLI acceptance and bounded scientific studies; record any platform-specific limitations.
- [ ] Verify installed GUI, CLI, official plugins and runtime dependencies from actual candidate installers; test required installer/update flows and SHA-256 checksums before publication.
- [ ] Preserve the historical frozen 500-million-ray scene, reference JSON and binary/CSV grids unchanged; run `tests/scientific/verify_reference_v2.py` before and after the candidate run. Never regenerate, replace, or silently redefine the historical reference.

## Mandatory full-scale 500-million-ray scientific release gate

- [ ] For each proposed release, **explicitly run a fresh 500,000,000-ray simulation** of the canonical frozen heliostat scene with seed `123456789`, 50,000 chunks of 10,000 rays, receiver side 1, 100 x 100 grid and original bounds. Run only as a deliberate manual release-acceptance task (not in routine CTest/CI). Use a complete Release build including the official material plugins, and write all new outputs into a unique external evidence directory.
- [ ] Record the source Git SHA, clean/dirty checkout, executable SHA-256, scene/input SHA-256, toolchain/runtime, machine and worker count, run command/configuration, native stdout/stderr, output JSON, binary and CSV grid, time and throughput. For the exact-historical mode, the existing `tests/cmake/run_heliostat_benchmark.cmake` and `tests/scientific/audit_500m_benchmark.py` require a matching historical grid hash.
- [ ] If physics is **unchanged**, retain the exact historical grid-hash agreement as a release requirement. If physics **intentionally changes**, obtain independent scientific justification for the changed model and run an explicitly documented candidate configuration without the old `reference_file` exact-hash gate. Independently validate benchmark metadata, exact ray count, generated binary SHA-256, binary/CSV cell equality, all finite nonnegative grid values, integrated power and reported flux statistics. Preserve the historical reference itself unchanged; do not relabel the existing exact-hash auditor as passing.
- [ ] Check total collected power against the existing 0.1% tolerance and maximum flux against the existing 5% tolerance where those physical comparisons are applicable. Calculate and archive full 100 x 100 signed/absolute difference matrices, mean absolute error, RMSE, percentile errors, meaningful-flux relative errors, Pearson correlation and integrated absolute error. These spatial metrics are **descriptive until prospective acceptance thresholds are scientifically approved**; do not introduce thresholds after inspecting a release's results.
- [ ] Record separate, unambiguous verdicts for (a) full-scale execution, (b) output integrity, (c) power/peak numerical tolerances, (d) historical exact-grid identity, (e) cellwise distribution review and (f) independent changed-physics acceptance. A 500M numerical pass alone does not establish universal material-model correctness or automatically authorize publication.


## Required previous-release performance comparison

- [ ] Compare the **candidate release against the previous published release** on the same physical machine using the same benchmark scene bytes, ray count, seed, worker settings and output options. Record input SHA-256, OS, CPU, power profile, compiler/toolchain, dependency versions, CMake options and executable hashes.
- [ ] Prefer **two isolated Release builds** from the previous release tag and exact candidate SHA using the same compiler, optimization flags, Qt and other dependencies; build and stage *all* official scientific plugins. If using installed release binaries instead, report installer-to-installer results separately and identify toolchain/runtime differences as confounders.
- [ ] Reuse `scripts/performance/Compare-TonatiuhPP-Performance.ps1` and the method in `docs/precommit-performance.md`. Use reduced-ray runs (the established example is 5,000,000 rays), at least two warm-up pairs, and repeated ABBA cross-over blocks; do not substitute one-off timing results.
- [ ] Require successful executions and matched workload metadata. Record elapsed time, throughput/rays per second, paired ABBA percentage effect and scientific outputs. For an unchanged physics/workload contract, compare power, flux statistics and grid hashes as specified by the harness; assess intentional physics changes with explicitly justified numerical tolerances instead of asserting byte equality.
- [ ] Apply the harness's existing stability checks (default drift/spread limit 3% and ABBA-effect agreement limit 2 percentage points). The nominal 2% slowdown threshold is an **investigation trigger**, not automatic proof of causation or equivalence. Do not adjust thresholds or remove outliers after seeing results to manufacture acceptance.
- [ ] Record a clear verdict: **PASS**, **CONFIRMED REGRESSION**, or **INCONCLUSIVE / UNSTABLE ENVIRONMENT**. Investigate and confirm potential regressions under controlled conditions. Do not treat an inconclusive result as PASS (the harness can return exit code 0 for inconclusive results).
- [ ] Preserve `metadata.json`, `report.json`, `report.md`, `runs.csv` and applicable environment-monitoring evidence outside immutable reference locations. Summarize release-to-release time/throughput changes and limitations in release acceptance notes.
- [ ] For a confirmed unacceptable regression, block publication until resolved or the maintainer explicitly approves a documented exception. For an inconclusive measurement, stabilize/repeat the experiment or obtain a documented maintainer decision acknowledging the unresolved risk; never silently claim performance acceptance.

## Release completion

- [ ] After candidate acceptance, follow the existing controlled release workflow and verify published installers, checksums, update repositories and application updates. Record exact release tag, Git SHA and release URLs.
- [ ] Synchronize remote and local `master`, archive the release evidence and performance verdict, and only then begin the next **opposite** development cycle.

See also `docs/performance-m9-acceptance.md` for the prior inconclusive measurement experience and `docs/headless-benchmark.md` for safe CLI benchmark usage.
