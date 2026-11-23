# v0.1.8.28 — Release acceptance record (OPEN)

**Previous published version:** `v0.1.8.27` (published 2026-11-13).
**Candidate version:** `0.1.8.28` (draft only).
**Source baseline before version preparation:** `5291cbef080fd2db4a944b68c0919ab113f7a7d3`.
**Exact candidate SHA:** pending version-bump merge and subsequent stabilization.
**Publication authority:** not granted. Do not create or push the release tag. The existing `CITATION.cff` DOI is carried forward unchanged for candidate preparation only; verify whether it is a concept DOI or a previous-version DOI before publication.

## Gates and evidence

| Gate | Status | Required evidence |
| --- | --- | --- |
| Version metadata and release notes | Pending | CMake, `CITATION.cff`, IFW version check and exact candidate diff |
| Three-platform development CI | Pending | Windows/Ubuntu/macOS checks on exact candidate |
| Scientific behavior | Pending | Independent acceptance for new material roulette and energy-budget semantics; bounded scene tests |
| Performance relative to v0.1.8.27 | Pending | Same-machine ABBA report and raw logs, stable verdict or documented maintainer decision |
| Release rehearsal | Pending | Manual `.github/workflows/release.yml` run with `fake_tag=v0.1.8.28`; installer/checksum evidence |
| Installed runtime and updater | Pending | GUI, CLI, plugins, installation/update acceptance on supported platforms; limitations recorded |
| Maintainer publication approval | Pending | Explicit tag/publication authorization |

## Controlled performance comparison

Use the existing `scripts/performance/Compare-TonatiuhPP-Performance.ps1` harness and `docs/precommit-performance.md`, not a new timing implementation. Keep the released tag and candidate in isolated full Release builds with matching compilers, dependencies, scene bytes, seeds, ray counts, worker settings, and plugin availability. Record exact SHA-256 hashes of both executables and scene inputs. The established example uses **5,000,000 rays**, two warm-up pairs, and repeated ABBA crossover blocks with 3% drift/spread and 2-percentage-point block-agreement limits. A stable slowdown above the nominal 2% threshold triggers confirmation/investigation; it is not automatic proof of causation. The existing M9 measurement was **inconclusive due to timing instability**; never count a harness exit code of zero as a performance PASS without reading the report verdict.

The new material/termination semantics intentionally alter physics for some inputs. Compare *runtime* on a matched workload but validate scientifically changed results against the new independently justified contract; only demand old/new exact flux-grid identity on fixtures whose physical behavior is unchanged. Record any differences and the acceptance rationale explicitly. Do not rerun the frozen 500-million-ray reference.

**Evidence to fill after real execution:** baseline tag SHA; candidate SHA; compiler/Qt/Coin versions; CPU/OS/power state; test scene SHA; baseline and candidate executable hashes; measured ABBA time change; rays/s; scientific validation outcome; timing stability and final verdict; report/CI/rehearsal links; documented exceptions if any.

## Next actions

1. Review and merge the candidate metadata/notes PR after CI; do not tag.
2. Validate exact candidate on all three platforms and run controlled performance/scientific acceptance before any publishing decision.
3. Rehearse the release workflow with `fake_tag=v0.1.8.28`, verify packaged artifacts, and seek explicit maintainer approval before the real tag.
