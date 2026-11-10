# M5.1A — Separate compiled simulation and scripting services (local integration)

**State:** **M5.1A locally validated** on native Windows Release (2026-11-09): the two compiled services and the QCoreApplication consumer built, and **163/163** scientific/headless CTests passed with zero failures (170.65 s). The working tree remains on uncommitted branch `refactor/m5-compiled-service-boundaries`. Base: M4 closure `626ef4d66e019d0c9528b5b6cb7a52e11565de51`. This is direct source separation, not a fully GUI-free core; no GitHub PR/CI or M5 exit is implied.

## Architectural change

The existing, previously shared **simulation** translation units for `SimulationSession`, `RayTraceExecutor`, `TracePreparation`, `SceneInstanceBuilder`, `SceneLoader` and the two A0 scientific signatures move out of the GUI executable into a single separately compiled `TonatiuhSimulation` static library. The existing **script** host, scene facade, document, interception service and `NodeObject`/`DataObject` adapters move into a separately compiled `TonatiuhScripting` static library. The GUI executable links both libraries, so the same C++ definitions serve the existing interfaces; `Q_OBJECT` headers move with their implementation to avoid duplicate moc objects. No physics, seed, chunk size, serialization, plugin ABI or script API is edited.

The new `headless.m5_compiled_services_link` CTest constructs `SimulationSession` and evaluates JavaScript through `ScriptHost` using **QCoreApplication** and the compiled service targets. A CMake configuration-time guard refuses *direct* QtWidgets/SoQt links and direct widget header includes in the moved service translation units.

## Crucial remaining M5.1B dependency debt

This first extraction **does not yet produce a GUI-free native binary**. The `TonatiuhKernel` shared target still directly links `Qt6::Widgets` and `SoQt::SoQt` and includes GUI-bound code (notably `TrackerKit.cpp`/`TArrayKit.cpp`); `TonatiuhLibraries` still contains QWidget-based editors and QCustomPlot; `SunPath` and production plugin targets retain GUI dependencies. Kernel factory headers such as `TFactory.h` expose `QIcon`, so the new service targets temporarily use `Qt6::Gui` to compile against unchanged ABI headers. The library-link and header guard is **direct-only**; it does not claim to traverse imported/transitive link dependencies.

**M5.1B must** remove core/widget dependencies, separate plugin GUI factories without silently changing their ABI, provide a headless executable linked without QtWidgets/SoQt, and add a failing whole-graph dependency guard. Only after this can M5.2 run the required Windows/Ubuntu/macOS end-to-end parity and packaging checks.

## M5.1A local acceptance — passed

- Exact-commit source relocation, incremental Release GUI and compiled-service builds **passed**.
- `headless.m5_compiled_services_link` and the complete scientific/headless suite **passed 163/163** (141 scientific, 40 headless, 16 equivalence and 40 smoke; labels overlap).
- No frozen 500-million-ray benchmark rerun, no hosted CI and no Git commit/PR.
- GUI-free *transitive* link closure and three-platform parity are **not** claimed by this M5.1A acceptance.

All changes must remain uncommitted and local until the maintainer accepts the M5.1 integrated source.
