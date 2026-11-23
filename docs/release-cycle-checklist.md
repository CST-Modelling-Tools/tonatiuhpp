# Release-cycle acceptance and performance policy

This is the reusable checklist for **every future Tonatiuh++ release**. The already-published v0.1.8.27 is the previous-release baseline for the next proposed release; historical version-specific checklists remain historical. This file does not authorize publication, tagging or running the frozen benchmark.

## Development cadence and release gate

- [ ] Alternate substantial **functionality** work with evidence-driven **architecture/coding-practice** work. Avoid cosmetic micro-refactor cycles.
- [ ] Finish the current cycle and publish a validated release **before** switching to the other development focus. If acceptance fails, keep the cycle open; never publish a failing release merely to advance the cadence.
- [ ] Identify the last *published* stable release, proposed version and exact baseline/candidate Git SHAs. Confirm clean/synchronized candidate `master` and review the actual diff and release notes.
- [ ] Confirm the version in `source/CMakeLists.txt`, `CITATION.cff`, IFW metadata, release notes and proposed tag is consistent; use `installer/sync_ifw_metadata.py` with the intended tag.
- [ ] Obtain maintainer authorization for the release tag and publication; do not assume a successful PR, build or branch merge authorizes release.

## Functional, scientific and cross-platform validation

- [ ] Validate the exact release candidate with Windows, Linux and macOS CI and appropriate native tests, Python automation/tooling, GUI/CLI acceptance and bounded scientific studies; record any platform-specific limitations.
- [ ] Verify installed GUI, CLI, official plugins and runtime dependencies from actual candidate installers; test required installer/update flows and SHA-256 checksums before publication.
- [ ] Preserve scientific reference inputs and expected outputs. Do **not** execute, regenerate, replace or reinterpret the frozen 500-million-ray benchmark as a routine release gate.

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
