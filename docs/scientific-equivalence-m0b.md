# Scientific equivalence — Milestone 0B

Status: development branch, **unvalidated**. All claims below are test intentions, not passed results. Milestone 0A is already merged and remains the baseline.

## Scope

- Keep all four M0A regression tests in the normal CTest suite.
- Add a checked-in two-surface Fresnel/absorber fixture from the project's existing refractive example. It exercises built-in scene types only.
- Compare deterministic, fixed-seed GUI-style borrowed and headless-owned preparation on the additional fixture at 4,096 and 20,001 rays, including repeatability and order-independent hit signatures.
- Replace vacuum with an instrumented `AirExponential` node to assert that atmospheric transmission is actually called after the first interaction; compare tracing with and without in-memory photon buffering.
- Exercise a correctly applied `SoGetBoundingBoxAction` on both fixtures, separate from the production GUI's `SunKit::setBox(TSceneKit*)` method.
- Add an **opt-in** native QApplication/SoQt executable that directly invokes that exact production GUI sun-sizing method, then compares sampled aperture area with separately prepared optical bounds. It is not a MainWindow automation test.

No optical kernel, scheduler, RNG, photon-exporter, GUI production source, or scientific benchmark references are modified.

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

## Boundaries

A passing borrowed-path comparison does **not** establish end-to-end MainWindow equivalence, and tracking atmospheric transmission calls does not independently establish numerical atmospheric correctness. The Fresnel example provides a reflected/refracted multi-surface path, not a mirror-only specular reference. Reflections, surface-side assignment, and photon histogram bins are characterized by consistency checks, not external truth data. More geometry, tracker, mesh-plugin, full GUI automation, numerical reference data, and cross-platform validation remain future work.
