# Tonatiuh++ v0.1.8.29 — release acceptance record (OPEN; NOT PUBLISHED)

**Proposed version:** `v0.1.8.29`. **Previous published stable:** [`v0.1.8.28`](https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v0.1.8.28). **Scientific candidate source:** `46ddd5e92e679cefa7aa1046f0c06ed183e04916`. **Merged `master` BVH source:** `34f01640ce638976137c87c74acfe6c7cdd03a36` ([PR #72](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/72)). This is an open release assessment, not permission to tag or publish.

## Acceptance checklist

| Gate | Verdict | Evidence / outstanding work |
| --- | --- | --- |
| BVH Windows Release CTests | PASS, maintainer-reported | 213/213 passed (224.11 s), including native GUI and BVH regressions |
| Three-platform feature CI | PASS | [PR #72 CI #45521725484](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45521725484): Windows, Ubuntu, macOS success |
| Post-merge `master` CI | TO VERIFY | Review [run #45524303701](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45524303701) final result separately |
| Fresh 500M candidate execution | PASS for execution corroboration | Local Windows Release source commit `46ddd5e`, 500M ray count and native stdout completion verified |
| Independent scientific output integrity | PASS | All 10,000 flux cells finite and nonnegative; CSV values bitwise match binary; SHA, flux and power independently recomputed |
| Exact 500M grid identity vs previous published candidate | PASS | Full binary SHA-256 and reported power/peak agree exactly with v0.1.8.28 candidate |
| Historical frozen reference integrity | REPORTED PASS | Original immutable files verified before and after the candidate run by the maintainer; original grid hash differs for earlier physics reasons |
| Historical numerical power/peak tolerances | PASS | −0.023904041% power (limit 0.1%), −0.189796252% peak (limit 5%) |
| Previous-release controlled timing stability | INCONCLUSIVE | 70M pre-BVH `master` paired comparison exceeded stability limits; 500M cross-session speed ratio is descriptive |
| Versioned release rehearsal and installer checks | PENDING | Execute nonpublishing `fake_tag=v0.1.8.29` rehearsal, verify Windows/Linux/macOS artifacts and checksums |
| Installed GUI/CLI/plugins and update compatibility | PENDING | Check real candidate installers; record platform-specific limitations and approved exceptions |
| Final tag and publication | NOT AUTHORIZED | Requires separate explicit maintainer release decision |

## Fresh 500-million-ray scientific evidence

On 2026-11-25 the maintainer ran one Windows Release benchmark with the canonical heliostat scene, seed `123456789`, 500,000,000 rays, 50,000 × 10,000-ray chunks, 16 workers, photon export disabled, receiver side 1, 100 × 100 grid and `[-2, 2] × [-2, 2]` bounds. Execution lasted **108.595 s** (reported throughput **4,604,263.548 rays/s**), and the native stdout confirms trace completion. New files were written under `%USERPROFILE%\TonatiuhPP-Scientific-Acceptance\BVH-500M-20261125-020521-342\`; frozen historical reference files were never overwritten.

| Metric | BVH 500M value | v0.1.8.28 candidate |
| --- | ---: | ---: |
| Collected power (MW) | 42.20696058499865 | 42.20696058499865 |
| Mean flux (MW/m²) | 2.6379350365624195 | 2.6379350365624195 |
| Minimum flux (MW/m²) | 0.07779745468364178 | 0.07779745468364178 |
| Peak flux (MW/m²) | 19.48813968933321 | 19.48813968933321 |
| Full 100 × 100 binary-grid SHA-256 | `c92f5e2bd4ef664707dd7b135e20e283db7fcd0067b01c63a8ec65b57989adab` | identical |

An independent read-only audit used `stdout.log`, `result.json`, `grid.bin`, `grid.csv` and `provenance.json` and reported **27/27 checks passing**. It verified 80,000 binary bytes, all 10,000 finite nonnegative values, bitwise-identical CSV decoding, matching SHA-256, minimum/mean/maximum statistics and independently integrated **42.20696058499864 MW** (floating-point rounding from the reported value). The provenance records executable SHA-256 `c3b72f2b3ca52c4119f2cb8b3a15e8b575243c070815f2bebe96e6a8eded1157`, canonical scene SHA-256 `fda26c7939649a8ad51537ccbc955ecdd800beb25a873fcadd6400433cc3a580`, and configuration SHA-256 `80a0cae2a90313cf7e9bf651f6869b038c110339159123f851f562589d24e42cb`. Because the actual executable, scene bytes and `config.json` were not uploaded to the auditor, those recorded source hashes were not independently recomputed; binary-to-execution provenance is not cryptographically proven by the manifests alone.

The historical frozen 500M grid hash remains `36c7dc17cee8e7fdef820f08d4482e7c3dcdee3b5eed4748ea3949a6cf4af472`. It differs from the v0.1.8.28 candidate because of intentional material/physics changes shipped previously; do not regenerate it or report the old exact-hash auditor as passing. The candidate satisfies the historical **0.1% power** and **5% peak** numerical comparison thresholds. The 70M ABBA measurement against pre-BVH `master` observed 93.323% paired elapsed-time improvement but was formally **INCONCLUSIVE / UNSTABLE ENVIRONMENT**. Comparison of the separate release-session 500M elapsed times (**1,574.168 s** previously, **108.595 s** now) yields an approximately **14.496× descriptive ratio**, not a formal controlled performance PASS.

## Pending release qualification

The complete evidence is retained externally; this summary does not import or alter the 500M reference. The locally tested BVH feature SHA preceded the release-version metadata change. Validate final candidate CI, perform the nonpublishing release workflow rehearsal, review built installers and runtime checks on each platform and obtain **explicit separate authorization** before creating a v0.1.8.29 tag or publishing. Follow [release-cycle checklist](release-cycle-checklist.md) and preserve the [v0.1.8.28 acceptance record](release-acceptance-v0.1.8.28.md).
