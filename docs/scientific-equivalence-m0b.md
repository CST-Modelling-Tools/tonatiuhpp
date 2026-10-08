# Scientific equivalence — Milestone 0B

Status: **8/8 scientific CTest regressions passed on Windows/MSVC Release after the Coin action-traversal fix** (3.45 seconds total, 2026-10-08 developer run). The opt-in native graphical diagnostic no longer crashes and confirms identical sampled sun-cell indices, but **GUI and headless aperture areas differ at full precision**; it returns exit code 1 as designed. The results below characterize the residual differences without changing production optics or test tolerances. Full MainWindow equivalence remains unproven; keep PR #5 as a draft pending scientific geometry-policy review.

## Scope

- Keep all four M0A regression tests in the normal CTest suite.
- Add a checked-in two-surface Fresnel/absorber fixture from the project's existing refractive example. It exercises built-in scene types only.
- Compare deterministic, fixed-seed GUI-style borrowed and headless-owned preparation on the additional fixture at 4,096 and 20,001 rays, including repeatability and order-independent hit signatures.
- Replace vacuum with an instrumented `AirExponential` node to assert that atmospheric transmission is actually called after the first interaction; compare tracing with and without in-memory photon buffering.
- Exercise a correctly applied `SoGetBoundingBoxAction` on both fixtures, separate from the production GUI's `SunKit::setBox(TSceneKit*)` method.
- Add an **opt-in** native QApplication/SoQt executable that directly invokes that exact production GUI sun-sizing method, then compares sampled aperture area with separately prepared optical bounds. It is not a MainWindow automation test.

The GUI `SunKit::setBox(TSceneKit*)` implementation now contains a targeted action-traversal correction. This is a production **GUI aperture-sizing** change whose numerical effect must be assessed before merging. No tracer propagation, scheduling, RNG, photon exporter, or benchmark reference values were changed.

## Windows Release validation (2026-10-08)

The developer rebuilt `tonatiuhpp_scientific_equivalence_tests` **after** the production fix to the GUI Coin action. CTest reported **8/8 passing tests, zero failures, 3.45 seconds total**. This extends the M0A baseline with deterministic Fresnel, non-vacuum atmospheric, photon-buffer and bounding-action checks; it does not establish absolute physical accuracy.

The optional `tonatiuhpp_gui_sun_diagnostic` also rebuilt and completed both fixtures without the earlier Windows `0xC0000005` access violation. It exits **1**, not from a crash, but because the GUI and analytical sun areas remain different at the original comparison tolerance.

| Fixture | GUI Coin sun area | Headless analytical sun area | Absolute difference | Relative difference | Sampled cell indices |
| --- | ---: | ---: | ---: | ---: | --- |
| Cylinder | 0.14738736997989096 | 0.14738737141625147 | 1.4363605027600812e-9 | 9.7454787948114726e-9 | 60/60 identical |
| Fresnel two-surface | 1.1908791977337683 | 1.1965395657699263 | 0.0056603680361579922 | 0.0047306150152383528 | 323/323 identical |

The unchanged absolute tolerance is `1e-9 * max(1, GUI area, headless area)`: approximately 1.0e-9 for the cylinder and 1.19654e-9 for Fresnel. Both exceed the tolerance, so the diagnostic correctly reports an area mismatch despite identical sampled cell masks.

### Why the bounding methods differ

The diagnostic explicitly processes the pending Coin field-sensor queue between two graphical bounding-box measurements. Before processing, the cylinder's Coin preview bounds are approximately `(-0.1, 0, 0.9) .. (0.1, 0, 1.1)`. After processing they become `(-0.2, -0.1, 0.8) .. (0.2, 0.1, 1.2)`, closely matching the analytical ray-tracing bounds `(-0.20000000298, -0.100000040233, 0.799999955297) .. (0.20000000298, 0.100000040233, 1.2000000447)`. The residual cylinder area difference is extremely small and consistent with float-precision Coin graphical bounds versus double-precision analytical values.

For the Fresnel fixture, the post-sensor Coin preview bounds are `(-0.5, -1, 0) .. (2, 1, 2)`, whereas the ray-tracing instance bounds are `(-0.5, -1, -0.01) .. (2, 1, 2.02)`. The base `ShapeRT::getBox()` pads planar optical geometry in its normal direction by 1% of the larger profile extent; the Coin display mesh is a zero-thickness plane. This explains their differing Z bounds and is a plausible source of the corresponding ~0.473% aperture-area difference. Since ray-tracing power per ray depends on sampled aperture area, the difference may affect energy normalization under equal irradiance and ray count, even when cell indices match.

**Scientific decision still required:** determine whether the GUI and headless modes should both use analytical ray-tracing bounds to size the sun, changing existing GUI numerical results for affected geometries, or intentionally retain distinct sizing semantics. The existing M0B regression tests do not resolve that choice. Do not relax the diagnostic tolerance, alter `ShapeRT::getBox()`, or reset benchmark references simply to obtain equality.

The native diagnostic reproduces `SunKit::setBox(TSceneKit*)`, but is **not** a full `MainWindow` test. Its explicit Coin sensor-queue processing does not establish that the actual GUI has refreshed its mesh before all sizing operations.

### Local commands

From the existing Windows/Release build environment, with the development branch checked out:

```powershell
git pull --ff-only
cmake --build "C:\OpenSource\tonatiuhpp\build" --config Release --target tonatiuhpp_scientific_equivalence_tests -j 16
ctest --test-dir "C:\OpenSource\tonatiuhpp\build" -C Release -R '^scientific\.' --output-on-failure
```

The opt-in graphical diagnostic can be run with the existing `TONATIUHPP_BUILD_GUI_SUN_DIAGNOSTIC=ON` configuration:

```powershell
cmake --build "C:\OpenSource\tonatiuhpp\build" --config Release --target tonatiuhpp_gui_sun_diagnostic -j 16
& "C:\OpenSource\tonatiuhpp\build\tests\scientific\Release\tonatiuhpp_gui_sun_diagnostic.exe"
Write-Host "Exit code: $LASTEXITCODE"
```

The native diagnostic is deliberately not registered as an automatic CTest check; its observed exit code 1 is evidence of an unresolved scientific-area difference, not of a build failure. Cross-platform CI results for the latest branch commit must also be reviewed before merging.

## Boundaries

A passing borrowed-path comparison does **not** establish end-to-end MainWindow equivalence, and tracking atmospheric transmission calls does not independently establish numerical atmospheric correctness. The Fresnel example provides a reflected/refracted multi-surface path, not a mirror-only specular reference. Reflections, surface-side assignment, and photon histogram bins are characterized by consistency checks, not external truth data. More geometry, tracker, mesh-plugin, full GUI automation, numerical reference data, and cross-platform validation remain future work.
