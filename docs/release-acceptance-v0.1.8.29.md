# Tonatiuh++ v0.1.8.29 — release acceptance record (PUBLISHED)

**Published version:** [`v0.1.8.29`](https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v0.1.8.29), 2026-11-25 at 11:17:47 UTC. **Tagged source:** `732e7dc767fe52c885d74573cc586221b58fb15c` ([PR #74](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/74)). **Previous published stable:** [`v0.1.8.28`](https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v0.1.8.28). **Scientific candidate source:** `46ddd5e92e679cefa7aa1046f0c06ed183e04916`. **Merged BVH source:** `34f01640ce638976137c87c74acfe6c7cdd03a36` ([PR #72](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/72)). **Versioned rehearsal source:** `a3f4406cf2d71e5006ef6100aea5aaaaa37fe16e` ([PR #73](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/73)). This record documents completed qualification, authorized publication and the limitations of the available validation evidence.

## Acceptance checklist

| Gate | Verdict | Evidence / outstanding work |
| --- | --- | --- |
| BVH Windows Release CTests | PASS, maintainer-reported | 213/213 passed (224.11 s), including native GUI and BVH regressions |
| Three-platform feature CI | PASS | [PR #72 CI #45521725484](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45521725484): Windows, Ubuntu, macOS success |
| Post-merge `master` CI | PASS | [BVH merge #45524303701](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45524303701) and [versioned merge #45565454963](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45565454963) passed on all three platforms; [final documentation-merge CI #45591770284](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45591770284) passed classifier and tooling checks, without repeating the native suite |
| Fresh 500M candidate execution | PASS for execution corroboration | Local Windows Release source commit `46ddd5e`, 500M ray count and native stdout completion verified |
| Independent scientific output integrity | PASS | All 10,000 flux cells finite and nonnegative; CSV values bitwise match binary; SHA, flux and power independently recomputed |
| Exact 500M grid identity vs previous published candidate | PASS | Full binary SHA-256 and reported power/peak agree exactly with v0.1.8.28 candidate |
| Historical frozen reference integrity | REPORTED PASS | Original immutable files verified before and after the candidate run by the maintainer; original grid hash differs for earlier physics reasons |
| Historical numerical power/peak tolerances | PASS | −0.023904041% power (limit 0.1%), −0.189796252% peak (limit 5%) |
| Controlled timing stability | INCONCLUSIVE; limitation accepted | 70M pre-BVH `master` paired comparison exceeded stability limits; maintainer accepted Option B (no repeat experiment); 500M cross-session ratio is descriptive |
| Versioned release rehearsal | PASS | [Run #45571795316](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45571795316): three platform builds succeeded; publish skipped |
| Rehearsal distribution and IFW repository integrity | PASS | Six ZIP digests and CRCs, five distribution SHA-256 checksums, ten IFW payload SHA-1 checksums and three repository versions verified |
| Native Windows installer acceptance | PASS, maintainer-reported | Maintainer installed the candidate and reported successful operation; individual GUI/CLI/plugin/Maintenance Tool checks were not separately reported |
| Native Linux/macOS installer acceptance | NOT EXECUTED; limitation accepted | Cross-platform CI runtime/payload checks and rehearsal packaging checks passed; native end-user installation not independently exercised |
| Native public update behavior | NOT VERIFIED | No native end-user update transaction established; successful Pages deployment is recorded separately |
| Final tag and publication | PASS | Maintainer-authorized production [run #45594857955](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45594857955), attempt 3, succeeded at `732e7dc767fe52c885d74573cc586221b58fb15c` |
| Published asset inventory and Pages deployment | PASS | All five distribution assets and five checksum files are present; Pages deployment succeeded |
| Independent published-file checksums | NOT VERIFIED | Rehearsal checksums apply only to rehearsal assets; published asset digests are reported by GitHub |

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

## Versioned packaging and installer acceptance

The [manual Release rehearsal #45571795316](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45571795316) ran on 2026-11-25 from 07:54:24 to 08:17:57 UTC, at `a3f4406cf2d71e5006ef6100aea5aaaaa37fe16e`. Windows x64, Linux x64 and macOS arm64 build/package jobs succeeded and the publication job was skipped. The project version and all three IFW `Updates.xml` component versions (`com.tonatiuhpp.app`) were `0.1.8.29`. Independent read-only inspection of the downloaded artifacts recomputed all six GitHub ZIP SHA-256 digests, verified all ZIP CRCs, recomputed all five distribution SHA-256 checksums and all ten supplied IFW payload SHA-1 checksums. Linux and macOS portable tar archives and the macOS installer tar archive were readable; the release workflow's Linux bundled-runtime and macOS bundle checks passed.

| Rehearsal distribution asset | Independently recomputed SHA-256 |
| --- | --- |
| `TonatiuhPP-0.1.8.29-windows-x64.exe` | `5ba516e576abd6e4582f3d19d091ac6390c0a2cfe35dcec1626d443993faa061` |
| `TonatiuhPP-0.1.8.29-linux-x64.run` | `4cad6672df1953c82db460874a104ff1fb5f5e5db657074d19321dc67a9a9042` |
| `tonatiuhpp-Linux.tar.gz` | `d6aa0a2a9c8b11369318f5df0bb5d7d59338d2fb09e30f62fa68f38843b35d73` |
| `TonatiuhPP-0.1.8.29-macos-arm64-installer.tar.gz` | `09b5fd2b1df217790623617dc0c4421ec41243c23b256e117df4514ba004e0a4` |
| `tonatiuhpp-macos-arm64.tar.gz` | `d97726ad4f624e71d43659623a0a3c2bc80130cc19d5d873144201de744c296f` |

The maintainer reported successful installation and operation on Windows. Native Linux and macOS end-user installer execution was not independently performed; this limitation was documented and accepted for publication. Successful CI includes native GUI scientific acceptance, relocated installed-runtime acceptance and IFW production-payload validation on all three platforms, but does not replace actual installer execution or update testing. An end-to-end native v0.1.8.28-to-v0.1.8.29 update transaction was not established.

## Production publication

The maintainer authorized the tag at `732e7dc767fe52c885d74573cc586221b58fb15c`. [Production run #45594857955](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45594857955) completed successfully on its third attempt, with successful packaging on Windows x64, Linux x64 and macOS arm64. The first two publication attempts encountered Pages artifact-discovery failures after upload; the third deployed artifact `15016032396` from attempt 2 and reported successful deployment. The [public release](https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v0.1.8.29) was published at 11:17:47 UTC on 2026-11-25 with five distribution assets and five SHA-256 files. This establishes successful publication, not independent verification of all published bytes or native end-user updates.

## Maintainer performance decision and validation limitations

On 2026-11-25 the maintainer explicitly selected Option B: accept the unresolved timing-stability limitation, preserve the **INCONCLUSIVE / UNSTABLE ENVIRONMENT** verdict and do not repeat the timing experiment for this release. The observed approximately 15.1-fold median acceleration and 93.323% paired time reduction are reported as observations, not as a formal timing PASS. The controlled experiment used pre-BVH `master`, rather than an independently built v0.1.8.28 tag; comparison with the previous published candidate is exact for scientific grid output, while its 500M elapsed-time comparison is cross-session and descriptive. This limitation must not be represented as a completed controlled previous-tag performance comparison.

The complete scientific evidence remains external and the frozen reference remains unchanged. The fresh full-scale execution preceded the version metadata change; the rehearsal installer was not itself subjected to another 500M simulation. The tag-triggered production workflow rebuilt the distributions; rehearsal checksums remain evidence for the rehearsal artifacts only. The authorized release has been published successfully, with the limitations above retained. Follow [release-cycle checklist](release-cycle-checklist.md) and preserve the [v0.1.8.28 acceptance record](release-acceptance-v0.1.8.28.md).
