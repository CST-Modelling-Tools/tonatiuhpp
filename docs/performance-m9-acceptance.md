# M9 — Lambda-elimination post-merge scientific and performance closeout

**Checkpoint date:** 2026-11-16. **Scope:** Windows Release, same-machine, same-workload reduced-ray comparison of the production C++ lambda cleanup against its pre-cleanup parent. This is a developer-run diagnostic, not a product release gate or a new frozen scientific reference.

## Decision

- **Reduced-ray scientific equality: PASS.** Both full comparisons finished; the monitored run's 34 executions matched exactly on total power, minimum/average/maximum flux, and `flux_grid_sha256`.
- **Performance acceptance: INCONCLUSIVE / UNSTABLE ENVIRONMENT.** Both full comparisons exceeded stability thresholds. Neither demonstrates a reproducible regression beyond the nominal 2% decision threshold, but neither proves equivalence within ±2%.
- **Action:** Preserve evidence and defer further timing experiments until the machine's measurement environment can be controlled. Do not relax acceptance thresholds, discard outliers post hoc, alter production C++, or regenerate the immutable 500-million-ray scientific reference to force a conclusion.

## Compared revisions and provenance

| Item | Baseline | Lambda-free candidate |
|---|---|---|
| Git revision | `d4889f7083d1f5d4c134c2dce2d11a60a57fc759` | `0e5bde8614b6b547990b8561b53b5f81872606b0` |
| Relationship | First parent of PR #50 merge | Merged `master` (PR #50) |
| CLI executable SHA-256 | `45f99ed6399419d15207288fda9274a7990bae687a9884d182c9af08a47e57f2` | `db50719ab9f4aa1ead0dd27b999732d79a8291cb52e4fb326f6e147ca81a0cb6` |

- Repository: `CST-Modelling-Tools/tonatiuhpp`; [merged PR #50](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/50).
- Platform recorded by harness: `Windows-11-10.0.26300-SP0`; Python 3.13.7; 16 logical processors. Both were separate Windows Release builds; the local build manifest records preparatory configuration checks but is not stored in Git.
- Scene: `examples/benchmarks/benchmark_heliostat_field_v1.tnhpp` in both revisions, exact input SHA-256 `fda26c7939649a8ad51537ccbc955ecdd800beb25a873fcadd6400433cc3a580`.
- Workload: 5,000,000 rays per execution, seed `123456789`, 500 chunks of 10,000 rays, 16 reported workers; five baseline/candidate warm-up pairs followed by six measured ABBA blocks (34 executions, 24 measured) per full experiment.
- Implementation: existing `scripts/performance/Compare-TonatiuhPP-Performance.ps1` and `compare_working_tree.py`; the second full run used `Start-TonatiuhPP-MonitoredComparison.ps1` and `analyze_monitor_coverage.py`.
- The frozen 500-million-ray benchmark and reference artifacts were **not** executed, rewritten, or used as reduced-ray comparison targets.

## Results of the two full experiments

| Metric | First full run (without hardware monitor) | Second full run (with hardware monitor) |
|---|---:|---:|
| Captured run directory (under `comparisons/`) | `full-20261116-024627-102/` | `monitored-20261116-031042-254/` |
| Paired median ABBA candidate time change | +0.418% | +1.129% |
| Overall candidate median time change | +0.756% | +1.270% |
| Baseline / candidate median, seconds | Not recorded in this document | 18.862 / 19.102 |
| Maximum absolute baseline endpoint drift | 4.17% | 13.39% |
| Maximum absolute candidate pair drift | Not separately available in supplied first-run console | 7.24% |
| Candidate block-mean spread | 3.51% | 7.36% |
| ABBA effect disagreement | 4.56 pp | 6.71 pp |
| Reduced-ray scientific comparison | MATCH | MATCH |
| Harness verdict | INCONCLUSIVE / UNSTABLE ENVIRONMENT | INCONCLUSIVE / UNSTABLE ENVIRONMENT |

Positive time change means the lambda-free candidate was slower. The first-run values above were taken from the maintainer's terminal transcript; its complete report was not part of the evidence bundle reviewed for this document. The second-run values and executable hashes come from `report.json` / `metadata.json` provided by the maintainer.

For the second run, the six ABBA effects were: +0.669%, +0.951%, +1.547%, +1.306%, +1.637%, -5.076%. The last block was particularly unstable: baseline endpoints were 21.061 s and 18.241 s, producing -13.39% drift. Do not remove or reinterpret it after observing the outcome.

**Decision rule:** The harness rejects timing acceptance if within-block baseline/candidate drift or across-block mean spread exceeds 3%, or block effects disagree by more than 2 percentage points. A stable result within the nominal 2% effect threshold would still not be formal proof of equivalence or causation. Both runs failed stability checks, so no performance-acceptance claim is made.

## Scientific result from the 5-million-ray comparison

Every one of the 34 executions in the uploaded second-run report has these exact values:

| Field | Exact observed value |
|---|---|
| `total_power_mw` | `42.22718222952255` |
| `minimum_flux_mw_m2` | `0` |
| `average_flux_mw_m2` | `2.6391988893451517` |
| `maximum_flux_mw_m2` | `21.130475862023847` |
| `flux_grid_sha256` | `b21e56e1b2530be896c51a53904c8ca2be47265915c196c6eb835033e9d540c9` |

These establish **exact reduced-ray output equality** between the compared binaries for the tested scene and seed. They do **not** independently establish physical correctness for other scenes, universal cross-platform bitwise equivalence, or a new 500-million-ray acceptance event.

## Monitoring investigation and limits

| Diagnostic | Counter configuration | Coverage and outcome |
|---|---|---|
| Monitored full comparison | Seven configured Windows counter paths; six wildcard `_Total` + per-processor expansions, 109 CSV data columns | 248 samples in approximately 12 minutes; 71 intervals over 3 s; maximum interval 14.505 s; 34/34 benchmark process windows marked sparse. |
| Reduced-counter sampling probe | Five explicit machine-wide counters, no wildcards | 33 samples across four full-size benchmark executions, 10 gaps over 3 s, maximum gap 12.350 s; 4/4 windows sparse. Probe had no warm-ups and one ABBA block; its timing difference has **no acceptance meaning**. |
| Idle counter check | Five machine-wide counters, no benchmark | 41 samples over approximately 40 s; all 40 sample intervals within 0.980–1.021 s; no gap over 3 s. |

The collector reported no errors, and exact process boundaries were recorded. Under heavy CPU workload, counter collection was irregular; the idle collector behaved regularly. Reducing the number of counters did not fix the gaps. This supports an association between heavy load and unreliable sampling, **not** a definitive explanation of the benchmark's timing drift. The first full comparison was also unstable **without** hardware monitoring. The sampled processor frequency and queue length do not prove thermal throttling, power-limit effects, or competing-process causation.

## Build/run preparation issue resolved

The initial M9 baseline warm-up failed to load the heliostat scene with `Coin read error: Node type "MaterialSpecular" not recognized`. The temporary preparation wrapper had built only `tonatiuhpp-cli`, not all separately loaded scientific plugin targets. Completing **both full Release builds**, including their plugins, resolved the runtime failure. The subsequent 100,000-ray smoke comparison completed with exact scientific output equality; no performance conclusion was drawn from its subsecond timings.

For future isolated baseline builds, ensure `MaterialSpecular.dll` and other scientific plugins are built and available beneath the Release executable's `plugins/` directory before attempting the benchmark. This was a **build/runtime deployment issue**, not a physics or lambda-elimination failure. No benchmark framework or production C++ change is required.

## Evidence retained locally (not committed into the repository)

Under `%USERPROFILE%\TonatiuhPP-Performance-M9\`:

- `M9-build-manifest.json` — preflight/build identities and paths (maintainer workstation).
- `comparisons/full-20261116-024627-102/` — first full-run output; terminal summary supplied.
- `comparisons/monitored-20261116-031042-254/` — second run: `metadata.json`, `report.json`, `report.md`, `runs.csv`, `run-events.jsonl` and per-process outputs.
- `comparisons/monitored-20261116-031042-254-windows-monitor/` — `monitor-metadata.json`, `sample-coverage.json`, `hardware-counters_000001.csv`, counter log records.
- `comparisons/sampling-probe-20261116-075344-852/` plus sibling `-windows-monitor/` — reduced-counter probe and coverage.
- `monitor-idle-*/idle_000001.csv` — independent idle collector validation.

This markdown document is a reproducible **evidence summary**, not a replacement for the raw local artifacts. Keep original run folders intact and use a new output directory for any later experiment. No source, scientific reference, or Git history needs to change as part of closing this checkpoint.
