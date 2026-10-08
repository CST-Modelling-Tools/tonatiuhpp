# Scientific equivalence — Milestone 0A

Status: **Milestone 0A Windows/MSVC Release validation passed**. The developer
compiled the test target, confirmed both repeatability tests, and verified
both scientific comparisons when they were opt-in diagnostics. After enabling
all four tests in CTest, the developer reported **4/4 passing (2.49 s total)**
on 2026-10-08. Cross-platform and full GUI integration validation remain
pending. The precise cause of an earlier test-harness crash was not established
by a native debugger.

## Purpose

This test-only milestone captures reproducibility in the two existing trace-preparation
paths before any refactoring of the optical kernel, RNG or executor. It is not yet a
GUI-end-to-end test: the GUI-style path uses a borrowed `SceneInstanceBuilder`
tree and calculates sun bounds from its `InstanceNode` hierarchy, matching
the headless bounds method for controlled comparison. It does **not** exercise
`MainWindow::UpdateLightSize()` or its Coin scene-graph bounding-box traversal,
and creates no `QApplication`, `SceneTreeModel`, or window.

## Coverage

- `ScientificTraceBaseline.HeadlessIsRepeatableAcrossIndependentScenes`
- `ScientificTraceBaseline.GuiStylePreparationIsRepeatableWithoutWidgets`

Each test loads the plugin-free cylinder fixture into independent scenes, uses seed
`123456789`, a `200 x 200` sun grid, and exercises both one-chunk and multichunk
runs (`1,024` and `20,001` rays). Assertions cover resolved sun area, irradiance,
power per ray, scheduling metadata, front/total hit counts and a coarse,
order-independent three-dimensional hit histogram. No photon files are written.

Two further tests are now **enabled by default**, following successful
manual execution on Windows:

- `ScientificTraceDiagnostic.GuiStyleMatchesHeadless` compares the scientific
  signatures of borrowed and owned preparation with the **same geometric
  sun-sizing method**. This is not a full UI-vs-CLI comparison.
- `ScientificTraceDiagnostic.PhotonRecordingPreservesScientificHits`
  compares the hit signature with in-memory photon recording on/off,
  without writing any photon files.

These tests now fail normal CTest if this behavior changes. The scope is limited
to the cylinder fixture and the configured trace modes; the same results have
not yet been confirmed on other platforms.

## Suggested local validation

Use the existing developer build environment and CMake Tools/normal build path;
do not change the build configuration just to run the test. With a configured
`BUILD_TESTING=ON` tree:

1. Build target `tonatiuhpp_scientific_equivalence_tests` (and required dependencies).
2. Run `ctest --test-dir <build-dir> -C Release -R "^scientific\." --output-on-failure`
   for a Release build. CTest should now run **four enabled tests**.
3. On Windows, ensure the normal runtime DLL locations are available to the
   test process, as for other tests using Coin3D and TonatiuhKernel.

The final Windows/Release CTest run passed all four enabled tests (4/4,
2.49 seconds total as reported by the developer). No performance or official
benchmark reference results are claimed by this milestone.

The first Windows/Release CTest run passed headless repeatability but the
GUI-style test crashed with access violation `0xc0000005`. That test previously
called `SunKit::setBox(TSceneKit*)` without GUI initialization. After the
borrowed-context test was changed to use computed ray-tracing instance bounds,
both repeatability tests passed. Both disabled diagnostics were also run
explicitly and passed. The change removed a suspected crashing path from the
harness; without a debugger stack trace it does not establish the root cause
or validate production GUI sun sizing. A later real-GUI bounding-box regression
test is still required.

## Boundaries and known limitations

- The fixture is a single simple absorber scene; it does not cover reflections,
  non-vacuum air, trackers, mesh plugins, surface selection, or GPU display.
- The borrowed preparation path does not launch `MainWindow` or invoke its
  scene-graph sun-sizing operation; a later real-GUI integration test must do both.
- GUI/headless equivalence using an identical sun-box calculation does not prove
  that the actual GUI scene-graph bounding-box result matches the headless bounds.
- Raw photon recording is exercised only through the GUI-style preparation
  contract, since headless preparation currently does not accept a photon buffer.
- Photon-file formats, buffer flushing and cross-worker file ordering are not
  covered by this milestone.
- The histogram is a test fingerprint, not a substitute for the official
  benchmark reference or a full physical flux-grid comparison.
- Do not update reference hashes to silence discrepancies.

Follow-up: investigate potential vacuum handling discrepancies in FluxAnalysis,
sun-aperture sizing differences, then the duplicated RayTracer propagation loops.
