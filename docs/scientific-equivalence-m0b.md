# Scientific equivalence — Milestone 0B

Status: **8/8 scientific regressions passed on Windows/MSVC Release prior to the sun-sizing correction** (4.47 seconds total, developer reported). The corrected GUI Coin traversal no longer crashes on the two standalone diagnostic fixtures, but their aperture areas differ: cylinder GUI 0.0279707 vs headless 0.147387 and Fresnel GUI 1.12908 vs headless 1.19654. Exit code 1 is the diagnostic's expected mismatch signal. The current hypothesis is that pending Coin field sensors leave preview geometry stale in a standalone executable without a GUI event loop. A diagnostic sensor-queue comparison is pending. The production GUI's numerical equivalence is **unproven**. Do not merge yet.

## Scope

- Keep all four M0A regression tests in the normal CTest suite.
- Add a checked-in two-surface Fresnel/absorber fixture from the project's existing refractive example. It exercises built-in scene types only.
- Compare deterministic, fixed-seed GUI-style borrowed and headless-owned preparation on the additional fixture at 4,096 and 20,001 rays, including repeatability and order-independent hit signatures.
- Replace vacuum with an instrumented `AirExponential` node to assert that atmospheric transmission is actually called after the first interaction; compare tracing with and without in-memory photon buffering.
- Exercise a correctly applied `SoGetBoundingBoxAction` on both fixtures, separate from the production GUI's `SunKit::setBox(TSceneKit*)` method.
- Add an **opt-in** native QApplication/SoQt executable that directly invokes that exact production GUI sun-sizing method, then compares sampled aperture area with separately prepared optical bounds. It is not a MainWindow automation test.

The GUI `SunKit::setBox(TSceneKit*)` implementation now contains a targeted action-traversal correction. This is a production **GUI aperture-sizing** change whose numerical effect must be assessed before merging. No tracer propagation, scheduling, RNG, photon exporter, or benchmark reference values were changed.

## Local validation

In the established Windows development environment, first select the development branch and rebuild the existing scientific test target:

```powershell
git fetch origin
git switch --track origin/test/scientific-equivalence-m0b-20261008
cmake --build "C:\OpenSource\tonatiuhpp\build" --config Release --target tonatiuhpp_scientific_equivalence_tests -j 16
ctest --test-dir "C:\OpenSource\tonatiuhpp\build" -C Release -R '^scientific\.' --output-on-failure
```

The scientific suite should discover **eight enabled tests**. Use the PowerShell terminal with previously established Qt and dependent DLL paths. If it fails, report the raw CTest output before any production changes.

For the native graphical diagnostic, opt in explicitly at configuration time (a separate build configuration is recommended). It has not yet been run:

```powershell
cmake -S source -B build -DTONATIUHPP_BUILD_GUI_SUN_DIAGNOSTIC=ON
cmake --build build --config Release --target tonatiuhpp_gui_sun_diagnostic
& "C:\OpenSource\tonatiuhpp\build\tests\scientific\Release\tonatiuhpp_gui_sun_diagnostic.exe"
```

The native tool needs a working Qt GUI environment and matching SoQt DLLs. It intentionally is **not** registered as an automatic CTest test, because the original M0A QCoreApplication harness crashed when invoking the GUI scene-sizing code. A mismatch or native crash requires investigation, not automatic benchmark adjustment.

**After the action-dispatch fix:** update the branch, rebuild `tonatiuhpp_gui_sun_diagnostic` (which also rebuilds `TonatiuhKernel`), and rerun it. Check that both fixtures print finite, positive GUI and headless areas, and inspect the equality/mismatch result instead of assuming the numerical values must agree. Then rebuild `tonatiuhpp_scientific_equivalence_tests` and rerun all eight scientific CTest cases to confirm no regression.

### Investigating differing aperture areas

With the corrected Coin action call, the native diagnostic completes both fixtures without an access violation, but returns 1 due to area mismatches (cylinder: 0.0279707 vs 0.147387; Fresnel two-surface: 1.12908 vs 1.19654, GUI vs headless). These are standalone diagnostic results, **not** end-to-end MainWindow measurements.

The GUI box is based on Coin preview geometry; the headless box comes from `ShapeRT::getBox()` through `InstanceNode::updateTree()`. `TShapeKit` regenerates its preview meshes through delayed field sensors. Since the diagnostic never enters a GUI event loop, pending sensor updates might explain these differences; genuine graphical/analytical bound differences are another possibility. The next run logs Coin bounds before and after processing the pending sensor queue, and then optical bounds. **This does not change production tracing or reference data.**

After pulling, rebuild `tonatiuhpp_gui_sun_diagnostic`, rerun it, and report those three bounds and the aperture values. Also rerun the eight scientific CTest regressions, because their prior pass predates the sun-sizing production fix. Investigate remaining differences rather than forcing a match.

### Investigating the native crash

The developer reported `$LASTEXITCODE = -1073741819` (`0xC0000005`) after running the GUI diagnostic twice on Windows. The initial build emitted no area measurements. The diagnostic now prints flushed `[gui-sun]` checkpoints before and after Qt/SoQt/Coin initialization, scene loading, `SunKit::setBox(TSceneKit*)`, instance-tree creation, aperture texture generation, and area reading.

A checkpointed native run reached `[gui-sun] before SunKit::setBox(TSceneKit*)` and then crashed with `0xC0000005`. This localizes the failure to that method, but does not provide a debugger call stack proving the exact instruction. The method originally called `separatorKit->getBoundingBox(action)` directly; the regression test successfully traverses the same fixture by calling `action.apply(separatorKit)`. The production method has now been updated to use the latter correctly initialized action. **This is a suspected fix, not a verified resolution**.

After pulling the new branch commit, rebuild and rerun the diagnostic in the configured Windows DLL environment. Report the area measurements, the last printed checkpoint, and the exit code. If the fix does not resolve the crash, obtain a native debugger call stack before further production changes. Any scientific area mismatch must be investigated rather than suppressed.

## Boundaries

A passing borrowed-path comparison does **not** establish end-to-end MainWindow equivalence, and tracking atmospheric transmission calls does not independently establish numerical atmospheric correctness. The Fresnel example provides a reflected/refracted multi-surface path, not a mirror-only specular reference. Reflections, surface-side assignment, and photon histogram bins are characterized by consistency checks, not external truth data. More geometry, tracker, mesh-plugin, full GUI automation, numerical reference data, and cross-platform validation remain future work.
