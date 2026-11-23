# v0.1.8.28 — Release acceptance record (OPEN)

**Previous published release:** `v0.1.8.27`, tag SHA `e2914aca7843f64735d4490e6a09da297b1449ed` (published 2026-11-13).
**Candidate source checkpoint tested:** `7c2ccfff751d9e84eb86b45d8d5ca0e5e45db02e` (documentation and final release SHA may change subsequently).
**Version:** `0.1.8.28` candidate only. **Publication:** not authorized; no tag or GitHub release.
**Historical source preparation baseline:** `5291cbef080fd2db4a944b68c0919ab113f7a7d3`.

## Release gates

| Gate | State | Evidence and outstanding work |
| --- | --- | --- |
| Candidate version metadata | Prepared, final verification pending | Check CMake/CITATION/IFW/tag consistency, exact final diff and citation DOI provenance |
| Development CI on tested checkpoint | PASS | [Three-platform CI](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45200034440): Windows, Ubuntu 24.04 and macOS succeeded on `7c2ccfff751d9e84eb86b45d8d5ca0e5e45db02e` |
| Windows native CTest | PASS | Maintainer reports **207/207**, including 169 scientific-labeled, 37 headless-labeled, 4 GUI-labeled tests; labels overlap |
| Full 500M candidate simulation | PASS | 2026-11-23, canonical scene/seed/grid; 500M rays completed, native CLI exit 0 |
| Independent 500M output integrity | PASS | Maintainer ran read-only cross-check of 10,000 binary/CSV cells, SHA-256, finite flux, integrated power and reported statistics |
| 500M reference power / peak-flux tolerances | PASS | 0.023904% power deviation ≤ 0.1%; 0.189796% peak deviation ≤ 5% |
| Exact historical grid identity | DIFFERENT | Intentional material/ray algorithm change; the existing historical exact-grid auditor cannot be marked PASS |
| Full-grid spatial comparison | COMPLETE; descriptive | Correlation, absolute/relative error distribution measured below; no prospective spatial thresholds |
| Independent changed-physics signoff | Pending | Targeted material-contract tests exist; formal physics acceptance rationale/review remains to be recorded |
| Previous-release timing comparison | INCONCLUSIVE; investigation closed by maintainer | 5M/50M/75M ABBA comparisons suggest approximately 18–21% faster candidate but fail predefined timing-stability conditions; not PASS |
| Nonpublishing release rehearsal | Pending | Run `.github/workflows/release.yml` with `fake_tag=v0.1.8.28`; inspect cross-platform artifacts/checksums |
| Installed application and updater | Pending | Verify installed GUI/CLI/official plugins, dependencies, update flows |
| Final authorization to publish | Pending | Explicit maintainer tag/publication approval |

## Full-scale 500-million-ray provenance and outputs

The maintainer executed a **new candidate** trace, not a regeneration of the historical reference. The canonical frozen scene and receiver were used with seed `123456789`, 500,000,000 launched/traced rays, 50,000 chunks × 10,000 rays, receiver side 1, 100 × 100 grid and `[-2,2] × [-2,2]` bounds, 16 workers, photon export disabled. The candidate configuration omitted the old `reference_file` exact-grid comparator, and wrote new outputs outside the source tree.

- **Evidence directory:** `%USERPROFILE%\TonatiuhPP-Scientific-Acceptance\v0.1.8.28-500M-20261123-183703-983\` (`config.json`, `console.log`, `result.json`, `grid.bin`, `grid.csv`, `provenance.json`).
- **Candidate CLI SHA-256 (earlier validated comparison executable):** `aa34fae0b997f684d725bf155ac0436b4edc062eba98f804a7d9a5274c12631a`. Confirm the full-run executable matches `provenance.json` before final signoff; that manifest's contents were not supplied for independent review.
- **Benchmark scene SHA-256 (earlier comparison):** `fda26c7939649a8ad51537ccbc955ecdd800beb25a873fcadd6400433cc3a580`. Confirm against `provenance.json`.
- **Execution:** `elapsed_seconds = 1574.168`, `rays_per_second = 317628.1057676182`, Windows Release CLI exit code 0, 16 workers.
- **Candidate grid SHA-256:** `c92f5e2bd4ef664707dd7b135e20e283db7fcd0067b01c63a8ec65b57989adab`. Independent SHA-256 of `grid.bin` matched `result.json`.
- **Frozen reference grid SHA-256:** `36c7dc17cee8e7fdef820f08d4482e7c3dcdee3b5eed4748ea3949a6cf4af472` (unaltered historical identity; a final post-run integrity transcript has not been provided).

| Measure | Historical frozen 500M | New candidate 500M | Relative difference |
| --- | ---: | ---: | ---: |
| Collected power (MW) | 42.217052166502086 | 42.20696058499865 | -0.023904041% |
| Average flux (MW/m2) | 2.638565760406375 | 2.6379350365624195 | -0.023904041% |
| Peak flux (MW/m2) | 19.52519778288914 | 19.48813968933321 | -0.189796% |
| Minimum flux (MW/m2) | 0.0807424687410667 | 0.07779745468364178 | Approximately -3.65% (descriptive, not a separately specified tolerance) |

The independent read-only validation checked benchmark metadata, generated SHA, 10,000 finite/nonnegative binary values, bitwise-identical decoded binary/CSV values, output min/mean/max, and independently integrated total power. Both historically specified scalar tolerances passed. This is **full-scale numerical and output-integrity acceptance**, not a claim that the old `audit_500m_benchmark.py` passed: that audit additionally requires exact frozen flux-grid identity, which this candidate does not satisfy. Changed-physics correctness remains a distinct review gate.

## Cell-by-cell 100 × 100 flux distribution comparison

Maintainer-generated read-only output is preserved under `%USERPROFILE%\TonatiuhPP-Scientific-Acceptance\flux-grid-comparison-20261123-191654-316\`. The run generated a summary plus 10,000-cell difference tables and signed/absolute matrices; the following values are from the supplied summary.

| Descriptive measure | Observed value |
| --- | ---: |
| Signed integrated power difference | -0.023904041% |
| Integrated absolute cell error / frozen reference power | 0.768261% |
| Mean absolute cell error | 0.020271072 MW/m2 |
| RMSE | 0.030442402 MW/m2 |
| RMSE / historical peak flux | 0.15591341% |
| Largest absolute cell difference | 0.24615409 MW/m2 |
| 95th percentile absolute cell difference | 0.065281145 MW/m2 |
| Pearson correlation across all cells | 0.99996308 |
| Increased / decreased / unchanged cells | 4,847 / 5,096 / 57 |
| Cells with frozen flux ≥1% of frozen peak | 9,039 |
| Median / 95th percentile absolute relative error (included cells) | 0.94849322% / 4.5693108% |
| Included cells above 5% absolute relative difference | 349 of 9,039 (3.86%) |

The 1%-of-peak cutoff only avoids unstable relative errors near zero; it is **not** an acceptance limit. The high correlation and small integrated absolute discrepancy support close spatial agreement for this scene, but the metrics are descriptive, not a statistically established uncertainty bound or a newly approved spatial PASS criterion. Neither high correlation nor low integrated power error proves physical correctness of the revised material model.

## Previous published release performance comparison: INCONCLUSIVE

Maintainer executed same-machine reduced-ray comparisons versus the immutable `v0.1.8.27` build using the established harness and matching canonical scene SHA `fda26c7939649a8ad51537ccbc955ecdd800beb25a873fcadd6400433cc3a580`, seed `123456789`, two warmup pairs and two ABBA blocks. Independent scientific repeatability within each version was exact; changed physics produced different cross-version outputs.

| Workload | Paired candidate runtime change | Formal verdict | Stability limitation |
| --- | ---: | --- | --- |
| 5M rays | -17.962% | INCONCLUSIVE | Baseline drift 4.943%, baseline between-block spread 4.197%, ABBA effect disagreement 2.755 percentage points |
| 50M rays | -20.518342% | INCONCLUSIVE | Baseline block-2 endpoint drift -3.2424% exceeds 3% |
| 75M rays | -20.044618% | INCONCLUSIVE | Candidate between-block spread 3.3033% exceeds 3%; ABBA effect disagreement 4.1953 pp exceeds 2 pp |

The consistent observed advantage is **not** a performance harness PASS. On 2026-11-23, the maintainer decided to stop further timing investigation and proceed while accepting the unresolved measurement-stability risk; the prescribed thresholds were not changed or reinterpreted. Preserve complete `metadata.json`, `report.json`, `report.md`, `runs.csv`, `run-events.jsonl` and per-run artifacts in the existing external comparison directories, including `release-v0.1.8.28-50M-20261123-162005-253` and `release-v0.1.8.28-75M-20261123-172758-533`. This documented decision is not final publication approval.

## Next actions

1. Verify source/executable provenance and record the independent new-physics release acceptance rationale, including existing bounded material studies; preserve immutable files.
2. Run nonpublishing `release.yml` rehearsal with `fake_tag=v0.1.8.28` on the final candidate source; inspect all installer artifacts and checksums.
3. Validate installed GUI/CLI/official plugins and updater on supported platforms; resolve or explicitly document issues.
4. Confirm version/DOI and final Git SHA, then request **explicit maintainer tag/publication authorization**. Do not tag or publish automatically.
