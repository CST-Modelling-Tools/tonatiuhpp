# Scientific equivalence — Milestone 0C: unified analytical sun aperture

**Status:** development branch, unvalidated. Implements the project-approved policy to derive sun aperture size from ray-tracing instance geometry for GUI preview, GUI tracing (including flux analysis), and headless tracing. This branch is based on the **unmerged** M0B work; merge and validate M0B first. No historical benchmark baselines have been changed.

## Scientific behavior

The previous graphical sun-sizing route used Coin3D preview mesh bounding boxes. For a planar two-surface Fresnel fixture, these generated aperture areas of 1.1908791977337683 (GUI) versus 1.1965395657699263 (headless) after the graphical sensor queue was processed, a relative difference of approximately 0.473%. M0C makes the analytical `InstanceNode::updateTree()` bounds authoritative instead.

This is a **scientific behavior change** for GUI simulations whose graphical and analytical bounds previously differed. For the same irradiance and ray count, power per ray is proportional to aperture area. Prior GUI photon/flux normalizations may therefore change. Do not silently regenerate or overwrite historical references; retain before/after measurements and a release note.

## Implementation

- `TracePreparation::prepareGuiTrace()` now synchronizes and updates the borrowed instance tree, checks analytical bounding validity, sets the sun box, and only then samples aperture cells. The existing headless preparation follows the same order.
- `MainWindow::UpdateLightSize()` rebuilds a temporary scene instance tree to size GUI preview sun bounds from analytical geometry, not the Coin graphical mesh.
- `MainWindow::Run()` avoids an unnecessary pre-tracing preview sizing step. All tracing calls independently refresh sun sizing during preparation.
- The legacy `sizeSunFromScene` GUI preparation toggle was removed; `FluxAnalysis` now uses the same analytical preparation as standard GUI ray tracing.
- A scientific test on both existing fixtures deliberately sets invalid legacy-style GUI sun bounds and checks that preparation replaces them, then compares GUI/headless aperture area, sampled cell indices, and `PreparedTraceContext::powerPerRay()`. The existing 8 M0B tests remain.

## Validation checklist (not yet executed)

From an existing Windows/MSVC Release setup, check out the new branch **after fetching it**:

```powershell
git fetch origin
git switch --track origin/feat/unified-analytical-sun-aperture-m0c-20261008
cmake --build "C:\OpenSource\tonatiuhpp\build" --config Release --target tonatiuhpp tonatiuhpp_scientific_equivalence_tests -j 16
ctest --test-dir "C:\OpenSource\tonatiuhpp\build" -C Release -R '^scientific\.' --output-on-failure
```

Expected test discovery: **nine** scientific tests (eight M0B + one M0C). Confirm that the new test and all old tests pass. Check CI on Windows, Linux and macOS before proposing a merge. Run a representative GUI/CLI flux or photon-export reference comparison before claiming complete scientific equivalence.

## Limits and follow-ups

M0C parity is scoped to the same analytical geometry used in the test scenes, including a curved cylinder and flat Fresnel elements. It is not a proof of physical correctness across all shape plugins, disabled surfaces, trackers, appended photon histories, real widget interactions, or export files. The diagnostic from M0B that calls `SunKit::setBox(TSceneKit*)` directly still deliberately illustrates the historical Coin-vs-analytical discrepancy; that legacy method remains available, but is no longer called from main GUI preparation.
