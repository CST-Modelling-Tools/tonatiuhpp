# Architectural A0 — native GUI/headless trace-signature characterization

**Status: Draft.** This is a diagnostic, not a new physical reference or a claim of completed full GUI/CLI parity.

## Opt-in contract

Set the same environment variable before starting each real process:

`TONATIUHPP_A0_TRACE_SIGNATURE_FILE` = absolute, writable output path to a JSON file.

Only when the variable is set, each trace captures its actual `RayTracerHit` callback events on the same production execution path. Each run sorts a multiset of exact world position IEEE-754 bit patterns, surface instance URLs and front/back flags and computes SHA-256. Inputs in the JSON include SHA-256 of the original scene-file bytes, master seed, ray count, solar grid width and height, and exact solar aperture area, irradiance and power-per-ray bits. A tiny Python comparator rejects differing inputs or signatures. Callback ordering and thread scheduling do not affect the sorted event multiset.

The diagnostic is capped at 200,000 rays to avoid surprising memory overhead; normal runs have no callback or new signature output. Incomplete, export-failed or canceled native GUI runs do not publish a new signature. The GUI still uses its normal Ray tracing dialog and fixed-seed checkbox; headless `trace-scene` uses the same `--seed` and can opt into `--update-trackers`. No photon output formats, RNG draws, tracing branches or benchmark reference files are changed.

**Caveat:** The GUI uses a photon-buffer/export path, whereas `--headless trace-scene --no-export` uses the no-photon path. The existing scientific tests check callback parity under recording changes but a full native export-file comparison remains separate. Bitwise comparison here is **within the same OS/toolchain**, not across platforms. This does not assess missed rays or complete per-ray history.

## Manual acceptance on Windows

Use a new terminal in VS Code; adjust the executable path if using a different build generator:

```powershell
cd C:\OpenSource\tonatiuhpp
$exe = (Resolve-Path 'build/application/Release/tonatiuhpp.exe').Path
$scene = (Resolve-Path 'examples/benchmarks/cylinder.tnhpp').Path
$env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE = "$PWD\build\a0-gui.json"
& $exe $scene
```

In the **actual GUI** choose Ray tracing > Parameters, 20,001 rays, grid **200 × 200**, No export, and fixed seed **123456789**. Start the trace and wait for it to complete. Confirm the GUI terminal says `a0_native_gui_signature:`. Close the GUI. Then in the same PowerShell terminal:

```powershell
$env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE = "$PWD\build\a0-headless.json"
& $exe --headless trace-scene $scene --rays 20001 --seed 123456789 --no-export
Remove-Item Env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE
python tests/scientific/compare_native_trace_signatures.py build/a0-gui.json build/a0-headless.json
```

For the tracker fixture, choose the same ray count/grid/seed; add `--update-trackers` to headless invocation. Verify the GUI loaded `MaterialSpecular` in the real application and the receiver is hit. Compare two independent native GUI runs as well as GUI-versus-headless when performing manual release sign-off.

Failure means a difference in either the input geometry/energy normalization or the observed callback events. Do **not** alter the frozen 500M reference to accommodate a mismatch.

## Automated validation

The test-only `ScientificNativeTraceSignature` checks order-independent bitwise signatures, sensitivity to sub-bin hit changes, and rejecting empty/invalid diagnostics. A small Python comparison check verifies matching and mismatching JSON independently. No CI test substitutes a borrowed-tree GUI-style trace for actual MainWindow automation. Native GUI interaction remains a **manual** release gate in this increment; headless output and the comparison parser can run unattended.

```powershell
cmake --build build --config Release --parallel 16
ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
```
