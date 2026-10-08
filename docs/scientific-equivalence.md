# Scientific equivalence — Milestone 0A

Status: Windows/MSVC Release build succeeded. First CTest run passed the headless
baseline, while the initial GUI-style test crashed with SEH `0xc0000005`.
This revision removes a GUI scene-graph bounding-box call from the
`QCoreApplication` test harness; it **still needs a local rebuild and retest**.
The precise crash location was not established by a native debugger.

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

The following tests are **disabled by default** because they characterize
potentially pre-existing differences rather than defining accepted new behavior:

- `ScientificTraceDiagnostic.DISABLED_GuiStyleMatchesHeadless` compares the
  scientific signature of GUI-style and headless-style preparation.
- `ScientificTraceDiagnostic.DISABLED_PhotonRecordingPreservesScientificHits`
  compares the scientific hit signature with in-memory photon buffering on/off.

Disabled diagnostics must not be enabled in normal CI until the current
behavior has been reviewed and any discrepancies are understood.

## Suggested local validation

Use the existing developer build environment and CMake Tools/normal build path;
do not change the build configuration just to run the test. With a configured
`BUILD_TESTING=ON` tree:

1. Build target `tonatiuhpp_scientific_equivalence_tests` (and required dependencies).
2. Run `ctest --test-dir <build-dir> -R "^scientific\." --output-on-failure`.
3. To execute opt-in diagnostics, run the test executable directly with
   `--gtest_also_run_disabled_tests --gtest_filter="ScientificTraceDiagnostic.*"`.
4. On Windows, ensure the normal runtime DLL locations are available to the
   test process, as for other tests using Coin3D and TonatiuhKernel.

A developer's first Windows/Release validation compiled this target and ran
CTest: headless repeatability passed; GUI-style preparation crashed with Windows
access violation `0xc0000005`. At that time the test called
`SunKit::setBox(TSceneKit*)` without GUI initialization. The test harness now
uses computed ray-tracing instance bounds for this borrowed-context test.
That removes the suspect code path from the test; it does not establish the
root cause of the crash or validate production GUI behavior. Rebuild/retest
and a later real-GUI bounding-box regression test are required.

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
