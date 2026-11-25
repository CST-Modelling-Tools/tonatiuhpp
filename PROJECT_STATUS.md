# Tonatiuh++ — current project status

## Published release and development focus

- **Latest published stable release:** [v0.1.8.28](https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v0.1.8.28), the baseline for the proposed v0.1.8.29.
- **Current merged development baseline:** `master` at `34f01640ce638976137c87c74acfe6c7cdd03a36` includes the canonical instance-tree refactor ([PR #70](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/70)) and the BVH-only acceleration ([PR #72](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/72)).
- **Current milestone:** prepare and qualify **v0.1.8.29**. Version metadata and release evidence are being prepared for review; no tag has been created and no release has been published.

## Current application and architecture

Tonatiuh++ is an optical ray tracer for concentrated-solar-thermal systems with a Qt/SoQt GUI (`tonatiuhpp-gui`), headless CLI (`tonatiuhpp-cli`), shared scripting services, official scientific plugins, photon export and flux analysis. GUI and headless tracing use the same scientific engine.

`SceneInstanceBuilder` constructs canonical scene/layout `InstanceNode` occurrences. `SceneBvh` indexes shape occurrences after transforms and tracker updates, shares a read-only spatial index among workers and retains original shape-intersection physics, deterministic equal-distance precedence and occurrence identity. The old recursive production `InstanceNode::findNearestHit()` search has been removed; this is a kernel API breaking change for external callers.

## Verification and scientific results

- **Local Windows Release:** maintainer-reported **213/213 CTests passed** (224.11 s), including four BVH regressions and native GUI acceptance; zero production-lambda findings across 447 files; Python CI-classifier 15/15 passed and tooling 55 passed with three Windows symlink-privilege skips.
- **Cross-platform feature CI:** [PR #72 run #45521725484](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45521725484) succeeded on Windows, Ubuntu 24.04 and macOS. [Post-merge `master` CI #45524303701](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45524303701) requires its own final-status review.
- **Scientific parity and performance:** controlled fixed-seed 5M/70M comparisons with pre-BVH `master` yielded identical outputs and complete flux-grid hashes. At 70M, observed median speedup was approximately **15.1×** with **93.323% paired elapsed-time reduction**. The formal timing-stability verdict remains **INCONCLUSIVE / UNSTABLE ENVIRONMENT**, not a performance PASS.
- **Fresh 500M release candidate:** the maintainer ran the canonical scene on Windows Release at feature commit `46ddd5e92e679cefa7aa1046f0c06ed183e04916`, seed `123456789`, 16 workers and 50,000 chunks of 10,000 rays. It completed in **108.595 s**. An independent read-only audit checked every one of the 10,000 binary/CSV cells, their SHA-256 and recomputed power/flux statistics. The full grid hash `c92f5e2bd4ef664707dd7b135e20e283db7fcd0067b01c63a8ec65b57989adab`, collected power **42.20696058499865 MW** and peak flux **19.48813968933321 MW/m²** are identical to the published v0.1.8.28 candidate's 500M results.
- **Immutable historical reference:** the older frozen 500M grid hash differs because of earlier, already released material/physics changes. Power and peak differences satisfy the existing 0.1% and 5% tolerances; the old exact-grid auditor is not a PASS for the changed-physics baseline and the historical files must not be modified.

## Current release gates and limitations

Complete the post-merge CI review, validate the proposed release version and its installer metadata, conduct a **nonpublishing** three-platform packaging rehearsal, verify real installer contents and installed GUI/CLI/plugins, check candidate checksums and update behavior, and obtain separate explicit approval before tagging or publishing. The 500M elapsed-time ratio versus the previously recorded v0.1.8.28 run (approximately 14.50×) is descriptive across different sessions, not a controlled performance acceptance. The fresh 500M execution was on the feature source revision; the final versioned release executable and installers have not been separately qualified.

Preserve all original frozen 500M reference files and all fresh candidate evidence outside immutable reference locations. The merge does not authorize a release.

## Maintained references

- [Architecture](docs/architecture.md), [Testing](docs/testing.md), and [Scientific validation](docs/scientific-validation.md).
- [Proposed v0.1.8.29 release notes](docs/release-notes-v0.1.8.29.md) and [acceptance record](docs/release-acceptance-v0.1.8.29.md).
- [Published v0.1.8.28 acceptance](docs/release-acceptance-v0.1.8.28.md), [release policy](docs/release-cycle-checklist.md), and [AGENT.md](AGENT.md).
