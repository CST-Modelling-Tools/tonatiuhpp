# Tonatiuh++ v0.1.8.29 — release notes

This release introduces shared scene-instance construction and scene-level BVH acceleration relative to v0.1.8.28.

## Scene-level ray-tracing acceleration

- Unify construction of the runtime scene-instance hierarchy across GUI and headless tracing, preserving separate occurrences of shared scene nodes.
- Introduce a world-space scene-level BVH over canonical shape-instance occurrences and replace recursive nearest-hit traversal of the authored hierarchy.
- Build the shared, immutable index during common GUI/headless trace preparation after transform/tracker updates. Preserve deterministic equal-distance precedence, instance identity, materials, shape intersections and random-number behavior.
- Reject invalid or nonconservative bounds during preparation rather than silently falling back to legacy traversal.
- **Breaking kernel API change:** remove `InstanceNode::findNearestHit()`. External kernel callers using the removed method must migrate to scene-level BVH intersection queries.

## Scientific validation

- Maintainer-reported **213/213 Windows Release CTests passed**, including the new BVH regressions and native GUI tests.
- [Feature CI](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45521725484) and [versioned post-merge CI](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45565454963) passed on Windows, Ubuntu 24.04 and macOS.
- A fresh, manually authorized **500,000,000-ray** Windows Release simulation (seed `123456789`, 16 workers, 50,000 chunks, 100 × 100 receiver grid) completed in **108.595 seconds**. An independent file audit verified all 10,000 finite nonnegative flux values, binary/CSV bitwise equality, SHA-256 and reconstructed flux/power statistics. Collected power was **42.20696058499865 MW**, peak flux **19.48813968933321 MW/m²**, and full binary-grid hash `c92f5e2bd4ef664707dd7b135e20e283db7fcd0067b01c63a8ec65b57989adab`, identical to the v0.1.8.28 candidate grid.
- The older frozen historical reference remains different because of physics changes already shipped in v0.1.8.28. Numerical historical power and peak differences satisfy the preexisting 0.1% and 5% tolerances; historical exact-grid equality is **not** claimed.

## Performance and release limits

- In 70M same-seed comparisons against pre-BVH `master`, observed median acceleration was approximately **15.1×** (paired elapsed-time effect −93.323%) with exact scientific output equality. The formal timing-stability verdict was **INCONCLUSIVE / UNSTABLE ENVIRONMENT**, not PASS.
- The new 500M elapsed time (108.595 s) compared with the earlier published release's recorded 500M time (1,574.168 s) yields a descriptive **14.496×** ratio across different sessions, not a controlled performance-verdict PASS.
- The maintainer accepted the timing-stability limitation without changing the inconclusive verdict; the reduced-ray experiment used pre-BVH `master`, not an independently built previous release tag.
- The [nonpublishing packaging rehearsal](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45571795316) succeeded on Windows x64, Linux x64 and macOS arm64. Rehearsal distribution checksums and IFW repository metadata were independently verified, and the maintainer reported successful Windows installation and operation. Native Linux/macOS installer execution was not independently exercised; live public update verification is a post-publication check. These limitations do not constitute native installation or update PASS results.

See [v0.1.8.29 release acceptance](release-acceptance-v0.1.8.29.md) and the [release policy](release-cycle-checklist.md).
