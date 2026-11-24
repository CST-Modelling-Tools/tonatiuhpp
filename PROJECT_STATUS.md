# Tonatiuh++ — current project status

## Published release and development focus

- **Published stable release:** [v0.1.8.28](https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v0.1.8.28). It is the reference release for subsequent scientific and performance comparisons.
- **Development baseline:** `master` contains the unified instance-tree construction merged in [PR #70](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/70), merge commit `6b8cfc15bfc716bdcfb93fd23f72f7825933b983`.
- **Current development objective:** accelerate scene-level ray intersection using an automatic bounding-volume hierarchy (BVH). This work is **planned, not yet implemented**. It builds on the completed instance-tree unification.

## Current application and architecture

Tonatiuh++ is an optical ray tracer for concentrated-solar-thermal systems. It offers the interactive Qt/SoQt application (`tonatiuhpp-gui`), a nonvisual command-line application (`tonatiuhpp-cli`), scripting and Python automation interfaces, scientific plugins, photon export, and flux analysis. GUI and headless paths reuse the same scientific ray-propagation services.

**Unified instance-tree construction is implemented and merged.** `SceneInstanceBuilder::build(TSceneKit*)` constructs the `InstanceNode` scene/layout hierarchy, and `SceneInstanceBuilder::insertSubtree()` constructs inserted nested occurrences. The GUI's `SceneTreeModel` uses the shared builder while retaining Qt indexing, Coin-node occurrence lookup, editing, and undo/redo. Headless and GUI callers retain independent tree ownership where their lifetimes differ. Shared Coin3D nodes are represented by distinct instance occurrences with their own parentage and order.

The existing `InstanceNode` traversal tests bounding boxes in the authored scene hierarchy; shape-specific acceleration remains separate. **There is not yet an automatic scene-level BVH.** The unification changes tree construction, not optical physics or the ray-propagation algorithm, and is not itself a demonstrated performance improvement.

## Verified baseline

- **Local Windows Release:** maintainer-reported **209/209 CTests passed**, including four native display-backed GUI tests; 230.06 seconds elapsed. The regression suite covers nested/shared instances and GUI scene-edit/undo/redo behavior.
- **Cross-platform development CI:** [run #45439316050](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45439316050) succeeded on Windows, Ubuntu and macOS, including each platform's standard CTest suite and native GUI scientific acceptance. This verifies PR #70 before merge; it is not a new release qualification.
- **Current CTest inventory:** standard configuration: **205 Windows**, **204 Linux/macOS**; enabling the four native GUI acceptance tests raises the totals to **209 Windows**, **208 Linux/macOS**.
- **Published v0.1.8.28 scientific acceptance:** the release candidate's independent 500-million-ray output integrity and predefined power/peak numerical tolerances passed. Historical exact-grid identity did **not** pass for the candidate with documented physics differences. The controlled performance comparison remained **INCONCLUSIVE / UNSTABLE ENVIRONMENT**, not a verified speedup. See [release acceptance](docs/release-acceptance-v0.1.8.28.md).

## Next implementation milestone — scene-level BVH

**Goal:** reduce intersection-search work in complex optical scenes while preserving scientific results and the existing GUI/headless behavior.

1. **Establish the baseline.** Identify the authoritative intersection/traversal boundary and measure representative scenes against the published stable release under the existing performance protocol.
2. **Build an acceleration index.** Introduce a scene-wide BVH over actual shape-instance occurrences using their correct world-space bounds and transforms, without altering the authored Coin3D scene or the canonical `InstanceNode` ownership hierarchy.
3. **Integrate and maintain it.** Use the acceleration index in the shared tracing path; handle scene changes, tracking and transforms without stale geometry. Preserve material interactions, per-occurrence identity, nearest-hit semantics and reproducibility.
4. **Validate and measure.** Add focused tree/geometry/nearest-hit and GUI-edit regressions; run the local Windows Release and scientific equivalence suites before commits, then three-platform CI. Report performance only after stable, same-machine comparisons with scientific-output checks.

BVH design and implementation are the **next work**, not completed functionality. Keep them in a separate feature branch and milestone from PR #70.

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
