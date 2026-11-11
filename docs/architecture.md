# Current Tonatiuh++ architecture

## Purpose and entry points

Tonatiuh++ provides interactive concentrated-solar-power ray tracing, scientific scene plugins, scripting, photon File export, and nonvisual automation. The current implementation shares its scientific engine among the graphical application and headless clients.

The permanent application entry points are:

- `tonatiuhpp-gui` (Windows: `tonatiuhpp-gui.exe`): the Qt Widgets/SoQt interactive application.
- `tonatiuhpp-cli` (Windows: `tonatiuhpp-cli.exe`): the QCoreApplication-only command-line client; use `tonatiuhpp-cli --help`, `validate-scene`, `trace-scene`, `run-script`, or `benchmark` directly, without `--headless`.

Both reuse the same physical scientific kernel, compiled services, and official plugins. The retired public executable names have no compatibility aliases.

## Build targets and dependencies

| Target | Current responsibility |
| --- | --- |
| `TonatiuhppKernel` | Scientific and optical scene/ray-tracing types and algorithms |
| `TonatiuhppLibraries` | Nonvisual utility and numerical support |
| `TonatiuhSimulation` | Shared preparation, execution, and simulation-session services |
| `TonatiuhScriptRuntime` | GUI-independent script execution and shared runtime services |
| `TonatiuhScripting` | Shared scene/document and scripting services |
| `TonatiuhppGuiPresentation` | GUI-only widgets, visualization, and presentation support |

Build entry point: `source/CMakeLists.txt`; individual target declarations live in `source/kernel/CMakeLists.txt`, `source/libraries/CMakeLists.txt`, and `source/application/CMakeLists.txt`.

The nonvisual library and official plugin dependency boundaries prohibit `Qt6::Widgets`, `SoQt`, and `Qt6::PrintSupport` in the guarded core linkage. `Qt6::Gui` remains an intentional dependency for a few shared data types; *GUI-independent* does not mean *free of every Qt GUI module*.

## Simulation and scripting

GUI Run/Flux Analysis, headless scene tracing, benchmark execution, and scripts use shared preparation, ray propagation, and simulation services. `RayTraceExecutor` handles the tracing work. The architecture retains one scientific kernel and shared photon handling rather than duplicating calculation algorithms across interfaces.

`ScriptHost` supplies a shared `QJSEngine` and script-local file resolution. The graphical Script Editor and headless scripts provide different presentation capabilities while reusing scientific services. GUI-specific operations (windows, dialogs, screenshots, or widget access) are not available in true-headless mode. See [Headless commands and benchmark](headless-benchmark.md).

For reproducible simulation, preserve the current seed, scene, tracker policy, aperture/energy normalization, worker/chunk semantics, scientific plugin identity, and photon format unless a change has a separately reviewed scientific justification.

## Official scientific plugins

The currently built and installed official plugin targets are declared in `source/plugins/CMakeLists.txt`: `AirMirval`, `MaterialSpecular`, `PhotonsFile`, `ShapeElliptic`, `ShapeFunctionXYZ`, `ShapeFunctionZ`, `ShapeHyperbolic`, `ShapeMapN`, `ShapeMesh`, `SunBuie`, and `SunGaussian`.

Other historical plugin source directories may still exist in the repository but are **not** automatically part of the active build. Do not delete a plugin solely because its CMake subdirectory is disabled: first check scene-file compatibility, factory registration, installation behavior, and test/fixture use. The maintainer has confirmed that no separate legacy third-party binary plugins require compatibility certification.

## Scientific acceptance boundaries

The merged M5 scientific implementation previously passed native Windows and three-platform CI acceptance; the integrated P2 executable and packaging migration requires a new native and hosted validation run. The full 500-million-ray Windows heliostat benchmark also matched the unchanged frozen reference exactly on the M5 merge candidate. These are validations of the specified cases, not a claim that all possible optical systems have been exhaustively tested or that platform packages are ready for release.

See [Scientific validation](scientific-validation.md) for the exact reference and safe verification commands. Release packaging and installation validation are separate activities.
