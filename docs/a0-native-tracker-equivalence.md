# Architectural A0 — native one-axis tracker scientific parity (PR #17)

**Status:** acceptance pending. This milestone extends the exact-hit signature diagnostic from merged PR #16 to the existing one-axis tracker fixture. Production optical physics, RNG, ray scheduling, photon formats, fixtures and the frozen 500M reference remain unchanged.

## Existing evidence

PR #15 added the explicit headless `--update-trackers` flag and established GUI-lifecycle-equivalent *test-harness* results. PR #16 merged as master `c80d33da` after actual native GUI/headless cylinder signatures matched 10,914 exact hit events (20,001 rays; seed 123456789); Windows, Ubuntu and macOS CI passed. Native GUI tracker parity was not yet demonstrated.

## Automated real-process acceptance

The new `headless.trace_tracker_exact_signatures` CTest runs the real headless executable as four independent processes on `tests/scientific/fixtures/one-axis-tracker.tnhpp`: two with `--update-trackers` and two with the legacy default. Each run uses 20,001 rays, seed 123456789, the default 200 × 200 sun grid and `--no-export`. It asserts successful completion, correct flag reporting, signature files with nonempty events, valid SHA-256 digests, and *all scientific signature fields* identical on independent repeat runs of each policy.

The tracked and legacy runs must share scene-file SHA-256, seed, ray count, grid and irradiance, but their exact scientific hit SHA-256 values must **differ**. Their aperture areas, hit counts and power-per-ray can legitimately differ when tracker rotation changes geometry. This covers the actual CLI and plugin loading, not native GUI automation.

```powershell
cmake --build build --config Release --parallel 16
ctest --test-dir build -C Release -R '^scientific[.]|^headless[.]trace_tracker_' --output-on-failure
```

## Manual acceptance — actual native tracker GUI versus headless

Run from `C:\OpenSource\tonatiuhpp` on the PR #17 branch, after successful compilation. Do **not** edit or save the committed fixture.

```powershell
$exe = (Resolve-Path 'build/application/Release/tonatiuhpp.exe').Path
$scene = (Resolve-Path 'tests/scientific/fixtures/one-axis-tracker.tnhpp').Path
Remove-Item build/a0-tracker-gui-1.json, build/a0-tracker-gui-2.json, build/a0-tracker-headless.json -ErrorAction SilentlyContinue

$env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE = "$PWD\build\a0-tracker-gui-1.json"
& $exe $scene
```

In the *real GUI*, choose Ray tracing > Parameters, **20,001 rays**, **200 × 200 sun grid**, **fixed seed 123456789**, and **No export**. Run the trace; look for `a0_native_gui_capture: enabled` followed by `a0_native_gui_signature:` in the console. Close the GUI. Confirm the tracking mirror, target and plugin-backed `MaterialSpecular` material loaded correctly. If the program reports a scene-modification or signature-export error, retain the message rather than overwriting the fixture.

Repeat a second GUI run in an independent process with exactly the same settings:

```powershell
$env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE = "$PWD\build\a0-tracker-gui-2.json"
& $exe $scene
```

Then run tracked headless execution independently:

```powershell
$env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE = "$PWD\build\a0-tracker-headless.json"
& $exe --headless trace-scene $scene --rays 20001 --seed 123456789 --no-export --update-trackers
Remove-Item Env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE

Test-Path build/a0-tracker-gui-1.json
Test-Path build/a0-tracker-gui-2.json
Test-Path build/a0-tracker-headless.json

python tests/scientific/compare_native_trace_signatures.py build/a0-tracker-gui-1.json build/a0-tracker-gui-2.json
python tests/scientific/compare_native_trace_signatures.py build/a0-tracker-gui-1.json build/a0-tracker-headless.json
```

All three files must exist and both comparators must exit zero. The comparator checks scene identity, exact seed/ray/grid and energy-normalization metadata, front/total event counts and a sorted exact surface URL/side/binary64-position hit digest. Compare bitwise only **within the same OS/toolchain**. Headless default/legacy is intentionally not the reference for GUI-tracker parity.

## Deferred A0 gates

Native photon-export-file reproducibility, complete ray-history equivalence, the frozen full 500M benchmark acceptance and final A0 closure remain separate tasks. No new numerical reference is introduced by this milestone.
