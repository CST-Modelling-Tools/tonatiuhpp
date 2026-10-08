# Scientific equivalence — Milestone 0B

Status: development branch. **Eight of eight automatic scientific tests passed on Windows/MSVC Release**, according to the developer's 2026-10-08 build and CTest output (4.47 seconds total). The optional native GUI diagnostic compiled, but both attempts terminated with Windows access violation `0xC0000005` before printing aperture results. This is **not a validated GUI/headless aperture comparison**; the cause and offending call remain unknown. The diagnostic now has flushed phase checkpoints for a targeted rerun. Do not merge until its scope and the crash have been reviewed.

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

### Investigating the native crash

The developer reported `$LASTEXITCODE = -1073741819` (`0xC0000005`) after running the GUI diagnostic twice on Windows. The initial build emitted no area measurements. The diagnostic now prints flushed `[gui-sun]` checkpoints before and after Qt/SoQt/Coin initialization, scene loading, `SunKit::setBox(TSceneKit*)`, instance-tree creation, aperture texture generation, and area reading.

After pulling the new branch commit, rebuild only `tonatiuhpp_gui_sun_diagnostic` and rerun in the configured Windows DLL environment. Report the last printed checkpoint and exit code; if the first checkpoint never appears, investigate startup/loading with a native debugger. If a checkpoint narrows the crash to a call, obtain a native debugger call stack before editing production code. No runtime fix has yet been demonstrated.

## Boundaries

A passing borrowed-path comparison does **not** establish end-to-end MainWindow equivalence, and tracking atmospheric transmission calls does not independently establish numerical atmospheric correctness. The Fresnel example provides a reflected/refracted multi-surface path, not a mirror-only specular reference. Reflections, surface-side assignment, and photon histogram bins are characterized by consistency checks, not external truth data. More geometry, tracker, mesh-plugin, full GUI automation, numerical reference data, and cross-platform validation remain future work.
