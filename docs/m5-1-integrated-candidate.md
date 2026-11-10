# M5.1 consolidated local integration candidate — nonvisual compile/link closure

**State: UNVALIDATED on native Windows.** This is a single consolidated integration on top of the maintainer-accepted M5.1A uncommitted branch. The prior M5.1A extraction and scientific/headless suite passed **163/163** (2026-11-09). This package must not be called M5 complete, merged, three-platform validated or release-ready until its local and hosted acceptance gates are met.

## One integrated source change

- Preserve the existing GUI application, `TonatiuhKernel`, `TonatiuhLibraries`, plugin interfaces and shipped plugin implementations. In particular, do not remove or reorder the virtual `TFactory::icon()` or `PhotonsFactory::createWidget()` ABI slots.
- Isolate `ScriptHost` and `DataObject` into one `TonatiuhScriptRuntime` library linked to QtCore/QtQml only. Do not compile either class twice in a GUI process. Add a direct and imported transitive CMake link guard and a real `QCoreApplication` JavaScript/CSV test.
- Compile headless *variants* of the legacy kernel/library sources, without `PhotonsWidget`, QWidget editors, QtPrintSupport or SoQt. Keep the GUI builds completely unchanged. Headless variants replace only presentation-only editor creation with `nullptr` and GUI-only missing-file dialogs with `qWarning` (the latter still stops the operation). No optics, physics, RNG, photon record or disk format is modified.
- Compile a separately linked `TonatiuhHeadlessSimulation` and `TonatiuhHeadlessScripting` from the same `.cpp` sources already used by the GUI-oriented `TonatiuhSimulation` and `TonatiuhScripting` targets. The *source-level scientific implementations* are identical; no new tracing algorithm is introduced.
- Build `tonatiuhpp_m5_core_headless`, whose entry point is `QCoreApplication` and `HeadlessCommandRunner`. The built-in type/scripting smoke invokes genuine Coin initialization, the built-in factory registry, scene document, shared simulation session and shared JS host, with no GUI instance. Plugin loading remains intentionally fail-closed for this new variant; the original production GUI/CLI continues to handle its legacy plugins. CMake checks the complete headless library dependency-graph paths to reject QtWidgets/SoQt/QtPrintSupport or legacy GUI/kernel targets.

## Compatibility and remaining exit gate

**Important:** This is a nonvisual **QtWidgets/SoQt-free target graph**, not yet a guaranteed QtGui-free runtime. `QIcon` is an exported C++ plugin factory ABI type, and the existing `SunAperture` rendering operation uses QtGui. QtGui remains linked in the headless kernel under this compatibility strategy. Calling this a completely GUI-library-free binary would be incorrect. The GUI plugin DLLs remain built and linked as before. To prevent cross-DLL ABI mixing, the new standalone headless variant deliberately disables external scene-plugin loading and rejects File-export plugins in direct CLI and scripted `tn.Run`. It can exercise built-in scene types and no-export execution only, while the original GUI/CLI binary retains all existing plugin functionality unchanged. The build smoke exercises built-in scene types without external plugins. Full plugin isolation, an independently packaged true-headless runtime with every supported scene/plugin, and equivalent native GUI/CLI physics are **not** yet established by this candidate. Do not merge or close M5.1 on the strength of the smoke alone.

This design makes the ABI tradeoff explicit rather than silently removing virtual functions or changing existing installed plugin linkage. It is a consolidated, substantial local integration candidate, with a hard stop if native builds or the CMake dependency graph fail. Completing the remaining plugin/QtGui ABI separation will require inspecting actual platform compiler and loader results after the integrated local test; no additional micro-PRs or intermediate commits are proposed.

## Acceptance requested in a single Windows run

1. Guard-check unchanged M5.1A source/files and exact base commit; never discard user changes or reset Git.
2. Configure CMake with nonvisual and runtime link closure guards, build GUI, both existing service targets, new headless variants/executable, both new smokes, and the native scientific test executable, incrementally in Release.
3. Run both independent nonvisual smokes and the **full selected scientific/headless suite once** (expected **165** uniquely selected tests; verify the actual test count rather than assume).
4. Inspect the native Windows link dependencies of the new headless executable and all referenced runtime DLLs for QtWidgets/SoQt/PrintSupport. QtGui is still intentionally present and explicitly documented.
5. The frozen 500-million-ray evidence and scientific reference data remain untouched. No commit/push/PR/CI.

**Not yet validated:** The independent Windows nonvisual kernel build and new runtime. If the new executable links a forbidden GUI library at runtime, fails to build on MSVC, or the 165-test suite differs, treat that as a blocker and keep the local branch; do not describe M5.1 as passed.
