# Tonatiuh++ — current project status

## Published release and development focus

- **Latest published stable release:** [v0.1.8.29](https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v0.1.8.29), published 2026-11-25 at 11:17:47 UTC.
- **Published source and next-release baseline:** `732e7dc767fe52c885d74573cc586221b58fb15c` ([PR #74](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/74)); includes canonical instance-tree construction (PR #70), BVH-only acceleration (PR #72) and version metadata (PR #73). [Production release #45594857955](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45594857955) succeeded.
- **Current milestone:** synchronize the documentation with the published v0.1.8.29 release; then return to a user-facing functionality milestone.

## Current application and architecture

Tonatiuh++ is an optical ray tracer for concentrated-solar-thermal systems with a Qt/SoQt GUI (`tonatiuhpp-gui`), headless CLI (`tonatiuhpp-cli`), shared scripting services, official scientific plugins, photon export and flux analysis. GUI and headless tracing use the same scientific engine.

`SceneInstanceBuilder` constructs canonical scene/layout `InstanceNode` occurrences. `SceneBvh` indexes shape occurrences after transforms and tracker updates, shares a read-only spatial index among workers and retains original shape-intersection physics, deterministic equal-distance precedence and occurrence identity. The old recursive production `InstanceNode::findNearestHit()` search has been removed; this is a kernel API breaking change for external callers.

## Verification and scientific results

- **Local Windows Release:** maintainer-reported **213/213 CTests passed** (224.11 s), including four BVH regressions and native GUI acceptance; zero production-lambda findings across 447 files; Python CI-classifier 15/15 passed and tooling 55 passed with three Windows symlink-privilege skips.
- **Cross-platform CI:** [PR #72 run #45521725484](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45521725484), [BVH post-merge run #45524303701](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45524303701) and [versioned post-merge run #45565454963](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45565454963) passed on Windows, Ubuntu 24.04 and macOS; the latter included native GUI, relocated runtime and IFW payload acceptance. [Final documentation-merge CI #45591770284](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45591770284) passed classifier and tooling checks on all three platforms; the full native suite was not repeated.
- **Scientific parity and performance:** controlled fixed-seed 5M/70M comparisons with pre-BVH `master` yielded identical outputs and complete flux-grid hashes. At 70M, observed median speedup was approximately **15.1×** with **93.323% paired elapsed-time reduction**. The formal timing-stability verdict remains **INCONCLUSIVE / UNSTABLE ENVIRONMENT**, not a performance PASS.
- **Fresh 500M release candidate:** the maintainer ran the canonical scene on Windows Release at feature commit `46ddd5e92e679cefa7aa1046f0c06ed183e04916`, seed `123456789`, 16 workers and 50,000 chunks of 10,000 rays. It completed in **108.595 s**. An independent read-only audit checked every one of the 10,000 binary/CSV cells, their SHA-256 and recomputed power/flux statistics. The full grid hash `c92f5e2bd4ef664707dd7b135e20e283db7fcd0067b01c63a8ec65b57989adab`, collected power **42.20696058499865 MW** and peak flux **19.48813968933321 MW/m²** are identical to the published v0.1.8.28 candidate's 500M results.
- **Immutable historical reference:** the older frozen 500M grid hash differs because of earlier, already released material/physics changes. Power and peak differences satisfy the existing 0.1% and 5% tolerances; the old exact-grid auditor is not a PASS for the changed-physics baseline and the historical files must not be modified.

## Published release and validation limitations

The [nonpublishing rehearsal #45571795316](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45571795316) passed for Windows x64, Linux x64 and macOS arm64 at the verified versioned candidate SHA. Independent checks passed for all six artifact ZIP digests and CRCs, five distribution SHA-256 checksums, ten IFW payload SHA-1 checksums and three IFW repository versions. Native Windows installation and operation succeeded according to the maintainer; specific individual installer/runtime subchecks were not separately reported. Native Linux/macOS installer execution was not independently verified. The maintainer accepted proceeding with these platform limitations documented. The authorized production workflow succeeded on attempt 3, including GitHub Pages deployment and publication of all ten release assets. Rehearsal artifact integrity checks apply to rehearsal artifacts only; independent public-download verification and native end-user updates were not established.

The maintainer explicitly accepted the inconclusive timing-stability limitation for this release without changing the formal verdict or requiring a repeat experiment. The 70M paired experiment compared against pre-BVH `master`, not a separately built published tag; the 500M cross-session ratio (approximately 14.50×) is descriptive. The fresh 500M simulation was on the feature source revision, not the packaged executable. Preserve all frozen reference files and external evidence unchanged. The published release is the accepted baseline for the next development cycle.

## Maintained references

- [Architecture](docs/architecture.md), [Testing](docs/testing.md), and [Scientific validation](docs/scientific-validation.md).
- [Published v0.1.8.29 release notes](docs/release-notes-v0.1.8.29.md) and [acceptance record](docs/release-acceptance-v0.1.8.29.md).
- [Published v0.1.8.28 acceptance](docs/release-acceptance-v0.1.8.28.md), [release policy](docs/release-cycle-checklist.md), and [AGENT.md](AGENT.md).
