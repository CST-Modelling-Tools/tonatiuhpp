# M3.3 — Script parity, first bounded regression package

## Scope and baseline

Based on merged M3.2 PR #33 (`745a0047`), whose final branch commit `7600b6d5` passed hosted Windows, Ubuntu and macOS CI. M3.3 is **in progress**, not accepted. This initial package adds a real-process repeatability gate around the unchanged `tests/scripts/m3_2_interception_bounded.tnhpps` fixture. It does not change physics, seed selection, tracker defaults, exporters, or the frozen benchmark.

## Automated gate

`headless.m3_script_interception_repeatability` launches the existing true-headless script in **two separate native processes** and requires success, one finite positive printed interception value per process, and exact equality of the printed numerical result. The fixture creates an in-memory 8 x 8 planar absorbing shape, clears/inserts the document, sets sun position and requests 2,048 rays; current true-headless `tn.FindInterception` uses seed 0. CTest runs this alongside the original bounded smoke/error gate.

Execute on a built application using `ctest --test-dir build -C Release -R 'headless[.]m3_script_interception_repeatability' --output-on-failure` (adapt build directory and executable runtime setup to the platform). The build/test result must be recorded, not inferred from successful source edits.

## Independent native GUI parity gate — pending

1. In GUI Ray Tracing settings, enable **fixed master seed 0** and **200 x 200 sun grid**. Avoid an automatically resolved seed.
2. Run exactly `tests/scripts/m3_2_interception_bounded.tnhpps` in the real GUI Script Editor with no selection affecting root insertion; ensure the scripted `tn.Clear()` will not destroy unsaved work.
3. Run the same script via `tonatiuhpp --headless run-script <absolute-script-path>`, with the same installed plugins and effective vacuum/scene state.
4. Record full printed numerical interception power from both interfaces, the fixed seed, grid, platform, executable/source commit and native log. Compare exact printed values on this controlled fixture, and confirm GUI responsiveness and root visibility.
5. A mismatch requires inspecting effective inputs and caller-specific semantics before attributing it to optical propagation.

Historical Windows M3.2 evidence found `63857.145538085264 W` in both GUI and true-headless at reported precision. This is prior evidence, **not** a new M3.3 validation result.

## Remaining M3.3 gates

Expand regression coverage to scene/document state and save/reload, trackers and effective policies, short repeated traces, `DataObject` relative-path I/O, missing inputs/plugins, source-location errors, consecutive script hosts, and cancellation. Document unsupported presentation-only APIs separately. Do not claim general GUI/headless equivalence from two headless runs; do not rerun the frozen 500-million-ray benchmark without explicit permission.
