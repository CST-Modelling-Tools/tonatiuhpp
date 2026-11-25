# Tonatiuh++ — current project status

## Published release and development focus

- **Published stable release:** [v0.1.8.28](https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v0.1.8.28). It is the reference release for subsequent scientific and performance comparisons.
- **Development baseline:** `master` at `272df55d72dbcb4e572da945261d374785181e7e` (merged documentation PR #71) includes canonical instance-tree construction from [PR #70](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/70), merge commit `6b8cfc15bfc716bdcfb93fd23f72f7825933b983`.
- **Current development objective:** complete cross-platform CI verification of the locally validated **BVH-only scene-level acceleration** on `feat/scene-level-bvh`. The legacy recursive nearest-hit traversal is removed; this feature is not yet merged or released.

## Current application and architecture

Tonatiuh++ is an optical ray tracer for concentrated-solar-thermal systems. It offers the interactive Qt/SoQt application (`tonatiuhpp-gui`), a nonvisual command-line application (`tonatiuhpp-cli`), scripting and Python automation interfaces, scientific plugins, photon export, and flux analysis. GUI and headless paths reuse the same scientific ray-propagation services.

**Unified instance-tree construction is implemented and merged.** `SceneInstanceBuilder::build(TSceneKit*)` constructs the `InstanceNode` scene/layout hierarchy, and `SceneInstanceBuilder::insertSubtree()` constructs inserted nested occurrences. The GUI's `SceneTreeModel` uses the shared builder while retaining Qt indexing, Coin-node occurrence lookup, editing, and undo/redo. Headless and GUI callers retain independent tree ownership where their lifetimes differ. Shared Coin3D nodes are represented by distinct instance occurrences with their own parentage and order.

The verified `master` baseline uses `InstanceNode` bounding-box traversal following the authored hierarchy; shape-specific acceleration remains separate. The unmerged **BVH-only candidate** indexes canonical shape occurrences after updating world transforms, shares the immutable index among tracing workers, and retains the original per-shape intersection physics and first-occurrence tie precedence. Its Windows Release tests and reduced-ray scientific equality checks pass; cross-platform CI has not yet run on this branch.

## Verified baseline

- **Local Windows Release:** maintainer-reported **209/209 CTests passed**, including four native display-backed GUI tests; 230.06 seconds elapsed. The regression suite covers nested/shared instances and GUI scene-edit/undo/redo behavior.
- **Cross-platform development CI:** [PR #70 run #45439316050](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45439316050) passed on Windows, Ubuntu and macOS, including standard CTest and native GUI acceptance. The documentation-only [post-merge run #45451446747](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45451446747) succeeded for current `master`; it is not a new scientific acceptance run or release qualification.
- **BVH-only Windows Release:** maintainer-reported **213/213 CTests passed** (224.11 seconds), including four native GUI tests and four expanded BVH regressions. The production-lambda audit found zero violations across 447 files; CI classifier tests passed 15/15, and tooling tests passed 55 with three privilege-related skips. Expected standard candidate CTest inventory: **209 Windows / 208 Linux/macOS**, or **213 / 212** with native GUI acceptance. This does not establish Linux/macOS validation.
- **BVH-versus-original comparison:** 5M and 70M-ray heliostat-field runs yielded exact same-seed scientific-output and full flux-grid hash equality. After removing the legacy traversal, the 70M-ray comparison again matched exactly, with a **93.323% paired elapsed-time reduction** (15.1x median speedup). The timing harness reported **INCONCLUSIVE / UNSTABLE ENVIRONMENT** for stability; this is recorded as a limitation, not a prerequisite for feature-branch CI. No new release-level scientific acceptance is claimed.
- **Published v0.1.8.28 scientific acceptance:** the release candidate's independent 500-million-ray output integrity and predefined power/peak numerical tolerances passed. Historical exact-grid identity did **not** pass for the candidate with documented physics differences. The controlled performance comparison remained **INCONCLUSIVE / UNSTABLE ENVIRONMENT**, not a verified speedup. See [release acceptance](docs/release-acceptance-v0.1.8.28.md).

## Current milestone — cross-platform BVH verification

**Goal:** reduce intersection-search work in complex optical scenes while preserving scientific results and the existing GUI/headless behavior.

1. **Preserve an independent baseline.** The separate clean `master` worktree at `272df55d` retains the authored-tree traversal for cross-version comparisons without reintroducing it into production.
2. **Build and integrate the acceleration index.** The candidate constructs a median-split, world-space BVH over actual shape occurrences, preserving canonical `InstanceNode` ownership and Coin3D structure. The shared GUI/headless ray tracer uses the same index per prepared trace; it is rebuilt on each preparation after tracker/transform updates.
3. **Preserve scientific behavior.** The authored recursive nearest-hit path has been removed. Shape intersection, deterministic first-occurrence ties, materials, RNG, photon export and instance identity are preserved; invalid BVH bounds fail preparation explicitly.
4. **Complete cross-platform integration.** Run the existing Windows, Ubuntu and macOS CI matrix, including scientific regressions and native GUI acceptance, before a merge decision. Release qualification is a separate approval process.

The BVH-only cleanup has passed **213/213 Windows Release CTests** and an exact same-seed 70M-ray scientific-output comparison with the preserved baseline. The measured performance improvement is substantial; the formal timing-stability verdict remains inconclusive. Cross-platform CI and review are the remaining feature-integration steps.

## Scientific, delivery and documentation boundaries

- Preserve the frozen historical 500-million-ray reference data. A fresh independent 500-million-ray candidate run is required only for an explicitly authorized **future release**, not routine development or CI.
- Preserve optical and RNG semantics, input/output compatibility, plugin behavior, and GUI/headless consistency unless a change is deliberately reviewed and scientifically justified.
- Follow the local-first build/test/commit workflow, and keep implementation, tests and maintained documentation synchronized within each merged change. See [AGENT.md](AGENT.md).
- A merge does not authorize tagging or publication. Follow the [release acceptance policy](docs/release-cycle-checklist.md) and obtain separate maintainer approval for any release.

## Maintained references

- [Architecture](docs/architecture.md) — current components and responsibilities.
- [Testing](docs/testing.md) — test inventory, commands and acceptance boundaries.
- [Release acceptance](docs/release-acceptance-v0.1.8.28.md) — published baseline evidence and limitations.
- [Release-cycle policy](docs/release-cycle-checklist.md) — mandatory future-release gates.
- [AGENT.md](AGENT.md) — permanent development and review rules.

This file describes the **current project state and next work**, not the history of changes. Completed discussions and past checkpoints remain available in Git and merged pull requests.
