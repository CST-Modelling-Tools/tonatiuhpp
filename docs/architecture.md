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

## Scope boundary: optical simulation versus solar-resource analysis

Tonatiuh++ keeps optical scene construction, ray tracing, sunshape modeling, the existing solar-position calculator, scripting and optical-result export in its active application. The calculator and scene services use `source/libraries/sun`, which is preserved. The historical `source/SunPath` library (solar-resource sampling and weighted sun directions) and disconnected `source/application/WeatherView` interface were removed from the active repository as out-of-scope for the focused optical simulator. Their implementations remain available through Git history; meteorological-year processing and annual energy-weighted performance workflows belong in specialized tools that drive the existing GUI/CLI/script simulation interfaces rather than in Tonatiuh++ itself.

## Simulation and scripting

GUI Run/Flux Analysis, headless scene tracing, benchmark execution, and scripts use shared preparation, ray propagation, and simulation services. `RayTraceExecutor` handles the tracing work. The architecture retains one scientific kernel and shared photon handling rather than duplicating calculation algorithms across interfaces.

`ScriptHost` supplies a shared `QJSEngine` and script-local file resolution. The graphical Script Editor and headless scripts provide different presentation capabilities while reusing scientific services. GUI-specific operations (windows, dialogs, screenshots, or widget access) are not available in true-headless mode. See [Headless commands and benchmark](headless-benchmark.md).

For reproducible simulation, preserve the current seed, scene, tracker policy, aperture/energy normalization, worker/chunk semantics, scientific plugin identity, and photon format unless a change has a separately reviewed scientific justification.

## Shared scene instance-tree construction

`SceneInstanceBuilder` in `source/application/core/` is the single authoritative algorithm for constructing the runtime `InstanceNode` hierarchy. `build(TSceneKit*)` constructs the scene and layout occurrences and recursively visits child groups using `insertSubtree(parent, node, row)`. Separate occurrences of a shared Coin3D `SoNode` receive distinct `InstanceNode` objects, preserving per-occurrence parentage and child order.

The headless `TracePreparation` path owns a builder-produced tree unless a caller explicitly borrows a pinned tree for synchronous scripted photon-export append. The GUI's `SceneTreeModel::setDocument()` also uses `SceneInstanceBuilder::build()`; `insertCoinNode()` and `Paste()` use `insertSubtree()` for nested groups. `SceneTreeModel` remains responsible for Qt indexes, the mapping from Coin nodes to their instances, interactive insert/delete/cut/paste and undo/redo. This unifies **tree construction**, not GUI/headless tree ownership or Qt editing responsibilities.

This refactor does **not** create a scene-wide bounding-volume hierarchy or change ray propagation. The existing `InstanceNode` world-space bounding boxes already cull traversal according to the scene's authored hierarchy; a future automatic scene-level BVH is a separate acceleration milestone. Shape-specific mesh BVHs remain separate.

## Official scientific plugins

The currently built and installed official plugin targets are declared in `source/plugins/CMakeLists.txt`: `AirMirval`, `MaterialSpecular`, `PhotonsFile`, `ShapeElliptic`, `ShapeFunctionXYZ`, `ShapeFunctionZ`, `ShapeHyperbolic`, `ShapeMapN`, `ShapeMesh`, `SunBuie`, and `SunGaussian`.

Other historical plugin source directories may still exist in the repository but are **not** automatically part of the active build. Do not delete a plugin solely because its CMake subdirectory is disabled: first check scene-file compatibility, factory registration, installation behavior, and test/fixture use. The maintainer has confirmed that no separate legacy third-party binary plugins require compatibility certification.

## Scientific acceptance boundaries

The published [v0.1.8.28 acceptance record](release-acceptance-v0.1.8.28.md) documents Windows Release and three-platform development checks plus a fresh 500-million-ray candidate. That run passed integrity and the predefined power/peak numerical tolerances, but its grid hash was **not identical** to the frozen historical reference because of intentional physics changes. Formal reduced-ray performance assessment was **INCONCLUSIVE / UNSTABLE ENVIRONMENT**, not a benchmark PASS. The frozen historical reference must not be regenerated or changed.

The shared instance-tree construction is now merged into `master` through [PR #70](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/70). Its Windows Release suite passed **209/209 CTests**, including four native GUI tests; Windows, Linux and macOS CI [run #45439316050](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45439316050) passed the standard suite and native GUI acceptance. Scene-level BVH acceleration remains planned and unimplemented, so this result does not establish a tracing speedup or qualify a future release. See [Scientific validation](scientific-validation.md), [Testing](testing.md) and the [release checklist](release-cycle-checklist.md).
