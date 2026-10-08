# Scientific equivalence — Milestone 0B

Status: development branch; **8/8 scientific CTest tests passed on Windows Release before the Coin action-dispatch change** (4.47 s total, reported by developer). The native GUI diagnostic now completes without crashing after the Coin fix. Explicitly processing Coin's pending sensor queue largely reconciles GUI/headless aperture areas: the cylinder displays 0.147387 for each, while the Fresnel two-surface fixture displays 1.19088 GUI vs 1.19654 headless. Exit code 1 still reports numerical differences. Further full-precision area and cell-mask diagnostics are pending. Do not merge yet; scientific equivalence is not fully established.

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

For the native graphical diagnostic, opt in explicitly at configuration time (a separate build configuration is recommended). It has now been run on Windows; the initial crash is gone, but the area comparison still fails:

```powershell
cmake -S source -B build -DTONATIUHPP_BUILD_GUI_SUN_DIAGNOSTIC=ON
cmake --build build --config Release --target tonatiuhpp_gui_sun_diagnostic
& "C:\OpenSource\tonatiuhpp\build\tests\scientific\Release\tonatiuhpp_gui_sun_diagnostic.exe"
```

The native tool needs a working Qt GUI environment and matching SoQt DLLs. It intentionally is **not** registered as an automatic CTest test, because the original M0A QCoreApplication harness crashed when invoking the GUI scene-sizing code. A mismatch or native crash requires investigation, not automatic benchmark adjustment.

**After the action-dispatch fix:** update the branch, rebuild `tonatiuhpp_gui_sun_diagnostic` (which also rebuilds `TonatiuhKernel`), and rerun it. Check that both fixtures print finite, positive GUI and headless areas, and inspect the equality/mismatch result instead of assuming the numerical values must agree. Then rebuild `tonatiuhpp_scientific_equivalence_tests` and rerun all eight scientific CTest cases to confirm no regression.

### Investigating differing aperture areas

With the corrected Coin action call, the native diagnostic completes both fixtures without an access violation, but returns 1 due to area mismatches (cylinder: 0.0279707 vs 0.147387; Fresnel two-surface: 1.12908 vs 1.19654, GUI vs headless). These are standalone diagnostic results, **not** end-to-end MainWindow measurements.

The new Windows diagnostic confirmed that pending Coin field sensors leave preview geometry stale unless processed. In the cylinder fixture, Coin bounds changed from approximately `(-0.1, 0, 0.9) .. (0.1, 0, 1.1)` to `(-0.2, -0.1, 0.8) .. (0.2, 0.1, 1.2)`, matching analytical ray-tracing bounds to the reported precision. The cylinder areas subsequently *display* the same six-digit value, but the strict comparison still fails, so an exact match has not been demonstrated.

The Fresnel fixture has a separate remaining geometrical distinction: after the sensor flush, its Coin preview box is `(-0.5, -1, 0) .. (2, 1, 2)`, while the analytical ray-tracing box is `(-0.5, -1, -0.01) .. (2, 1, 2.02)`. The base `ShapeRT::getBox()` expands a planar surface's normal-direction bounds by 1% of its profile width; the Coin planar rendering mesh is flat. This explains why those two bounds are not identical, though the exact area effect still needs characterization.

The updated diagnostic prints each aperture area using `max_digits10`, the number and indices of valid sampled cells, absolute area difference, relative difference, and the **unchanged** original tolerance. We are characterizing the discrepancy without adjusting production optics, GUI sun-sizing, ray count normalization, or scientific reference values. Processing Coin's sensor queue in this **standalone diagnostic** is not proof that `MainWindow` always processes sensors before computing its aperture.

After pulling, rebuild and rerun `tonatiuhpp_gui_sun_diagnostic`. Report the full-precision areas, valid-cell counts, whether cell indices match, absolute/relative differences, and exit code. Also rerun the eight scientific CTest regressions, because their prior pass predates the production Coin action-dispatch correction. Investigate remaining differences rather than suppressing them.

### Investigating the native crash

The developer reported `$LASTEXITCODE = -1073741819` (`0xC0000005`) after running the GUI diagnostic twice on Windows. The initial build emitted no area measurements. The diagnostic now prints flushed `[gui-sun]` checkpoints before and after Qt/SoQt/Coin initialization, scene loading, `SunKit::setBox(TSceneKit*)`, instance-tree creation, aperture texture generation, and area reading.

A checkpointed native run originally reached `[gui-sun] before SunKit::setBox(TSceneKit*)` and then crashed with `0xC0000005`. The production method previously called `separatorKit->getBoundingBox(action)` directly and was changed to use `action.apply(separatorKit)`. Both fixtures now complete this method and the remainder of the native comparison without crashing on Windows. The precise former faulting instruction was not debugger-confirmed. The remaining exit code 1 is from the numerical aperture comparison, not a crash.

After pulling, rebuild and rerun the diagnostic in the configured Windows DLL environment. Report the full-precision aperture values, sampled cell counts, and exit code. Any remaining scientific mismatch must be investigated rather than suppressed.

## Boundaries

A passing borrowed-path comparison does **not** establish end-to-end MainWindow equivalence, and tracking atmospheric transmission calls does not independently establish numerical atmospheric correctness. The Fresnel example provides a reflected/refracted multi-surface path, not a mirror-only specular reference. Reflections, surface-side assignment, and photon histogram bins are characterized by consistency checks, not external truth data. More geometry, tracker, mesh-plugin, full GUI automation, numerical reference data, and cross-platform validation remain future work.
