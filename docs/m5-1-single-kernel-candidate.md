# M5.1 — single-kernel integration (Windows native acceptance passed 2026-11-09)

## Contract

The experiment that passed 165/165 CTests (2026-11-09) was based on TWO compiled
kernel/library sets. That experiment is superseded; it is not a merged design.

This integration must deliver exactly ONE `TonatiuhKernel` shared library,
ONE `TonatiuhLibraries` and ONE set of `TonatiuhSimulation`,
`TonatiuhScripting`, and `TonatiuhScriptRuntime` services. Both the GUI
`tonatiuhpp` and the independent `tonatiuhpp_m5_core_headless` executable must
link these *same* targets. No `TonatiuhHeadlessKernel`, `TonatiuhHeadlessLibraries`,
`TonatiuhHeadlessSimulation`, or `TonatiuhHeadlessScripting` target is built.

Optional GUI presentation code (Qt Widgets, PrintSupport, QCustomPlot, Coin
viewer callbacks, editor widgets, and photon-export settings widgets) belongs
to **TonatiuhGuiPresentation**, which is not linked into the standalone
headless executable, physics kernel, scientific plugins or scripting services.
The core still uses QtGui for QIcon factory ABI, QPolygonF geometry support,
and QImage/QPainter; M5.1 enforces *no QtWidgets/SoQt/PrintSupport* in the
scientific target closure, not absence of every QtGui component.

## Plugins and compatibility

- Each official scientific plugin (including PhotonsFile) has ONE actual
  scientific DLL, linked against the same TonatiuhKernel and TonatiuhLibraries.
- PhotonsFile's GUI-only PhotonsFileWidget is moved to the GUI presentation
  library; its File-export class/format/algorithm is NOT duplicated or changed.
- Scene factories and the File exporter retain the existing factory virtual
  slots and historical Qt plugin IIDs. The optional GUI widget creation
  remains reachable via a callback registered in the GUI main entrypoint.
- Certified in-tree plugins declare `TonatiuhppHeadlessABI=1` in Qt's plugin
  metadata. GUI mode still permits legacy plugin discovery; headless mode
  does not instantiate plugins missing compatibility metadata.
- Moving `PhotonsWidget`, `FieldEditor`, the editor widgets and QCustomPlot
  to a separate DLL changes the **binary** home of their exported symbols.
  Existing binary-only third-party GUI plugins can require recompilation.
  Source-level virtual layout is preserved, but strict old-binary plugin ABI
  compatibility is NOT claimed. This is an explicit M5.1 acceptance blocker
  requiring an owner decision or migration strategy before release.
- Plugin metadata is a declared compatibility level, NOT a digital signature
  or sandbox. Do not load untrusted code merely because its JSON claims ABI 1.

## Acceptance requirements

1. Preflight verifies the accepted 165-test candidate on the original uncommitted
   branch before mutation. No reset, checkout, commit, push, or PR.
2. CMake configures and builds GUI, standalone QCoreApplication CLI, exactly
   one kernel/library/scientific plugin set, and all scientific tests.
3. CMake recursively rejects QtWidgets, SoQt and PrintSupport from core,
   simulation, scripting, standalone CLI and each certified plugin.
4. `headless.m5_nonvisual_core_link`, runtime and compiled-service smokes, and
   `headless.m5_unified_file_plugin` must all pass. The latter loads an actual
   first-party external scene plugin and calls the same File-export plugin.
5. Execute equivalent seeded standalone and historical `tonatiuhpp --headless`
   traces with and without File export. Compare file SHA-256 and metadata;
   no 500M benchmark rerun. Run complete scientific/headless CTest suite once.
6. Verify runtime DLL imports, GUI interactive launch, plugin menu/Flux dialog,
   packaging and three-platform CI before formal M5.1 closure.

The native build, binaries, photon/flux parity and cross-platform checks are
UNVALIDATED at the moment this candidate is distributed. Stop on the FIRST
failure, preserve the full worktree and share the error log. Do not describe
this package as accepted or complete before native evidence exists.

## 2026-11-09 Windows M5.1 acceptance (later evidence)

The package's above pre-distribution native-unvalidated warning is historical.
Actual user-run MSVC Release acceptance subsequently reported **166/166 unique
CTests passed** (16 equivalence, 43 headless, 144 scientific; labels overlap),
with exact seeded GUI-binary/standalone File export, compiled one-kernel
smokes and Windows PE imports confirming no QtWidgets/SoQt/PrintSupport
for the inspected standalone/kernel/libraries/File/Material binaries.
No Git commit/push/PR or GitHub CI was triggered. This is Windows **local**
acceptance only, not installed-runtime, third-party binary ABI or three-OS
certification. M5.2 follow-on: `docs/m5-2-end-to-end-acceptance.md`.
