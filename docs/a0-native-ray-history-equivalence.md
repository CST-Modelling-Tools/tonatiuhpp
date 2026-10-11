# Architectural A0 — native complete ordered ray-history equivalence (PR #21)

**Status:** Draft implementation based on merged PR #20 master `7657954e`; Windows Release, three-platform CI and actual GUI acceptance pending.

## Why full histories need a separate diagnostic

PRs #16–#20 proved exact **hit-event multiset** and **photon File-export multiset** equivalence for cylinder and one-axis tracker fixtures. Neither representation associates successive interactions with their originating ray, preserving reflection order and ray termination. The new, test-only `TONATIUHPP_A0_RAY_HISTORY_FILE` diagnostic captures these details without changing production photon-file formats or the default tracing path.

Every launched ray receives a **stable ordinal** derived from the deterministic chunk index and index within the chunk; 10,000-ray chunks retain existing RNG derivation and worker scheduling. Each history stores the sun ray origin/direction, its ordered interactions (surface instance URL, front/back side, hit point, incoming ray origin/direction, outgoing ray origin/direction, continued-vs-terminal), and a classified termination (surface, air-loss or escape) with terminal ray state and sampled air-segment distance where applicable. The diagnostic hashes all binary64 field bits in order per ray, **sorts rays by ordinal** to discard worker completion order, checks every ray was captured exactly once, and writes one compact JSON summary. It does not leak the complete per-ray data into the normal photon File exporter.

The history callback is wired into both the non-recording and recording RayTracer branches; on normal traces it is absent. GUI and headless share the same history writer and metadata: on-disk scene SHA256, seed, ray count, sun grid, exact aperture area, irradiance and power-per-ray IEEE bits. A diagnostic run is bounded to 200,000 rays. Missing, duplicated, non-finite or incomplete histories are rejected.

## Automated native process regressions

Three new CTests: `headless.ray_history_cylinder_equivalence`, `headless.ray_history_specular_equivalence` and `headless.ray_history_tracker_equivalence`. The tracker variant opts into `--update-trackers` for all subprocesses; the other two preserve unchanged scene behavior. Every test launches three independent real native CLI traces with 20,001 rays, seed 123456789, default 200 x 200 sun grid:

1. Recording and real photon File export, first independent process.
2. Recording and real photon File export, second independent process.
3. No-photon-recording execution with `--no-export`, third independent process.

Both full-history comparisons must match all scientific metadata, counts, per-ray ordered history SHA-256 digest and termination summaries. Existing exact-hit comparators independently verify recording/non-recording parity, and the existing canonical photon-file comparator checks physical exported records. No reference digest is hardcoded or frozen.

```powershell
cd C:\OpenSource\tonatiuhpp
git switch test/a0-native-ray-history-equivalence-20261011
cmake --build build --config Release --parallel 16
ctest --test-dir build -C Release -R '^scientific[.]|^headless[.]ray_history_|^headless[.]photon_export_' --output-on-failure
```

## Native GUI history acceptance — separate pending gate

Use the actual GUI on the saved `examples/benchmarks/cylinder.tnhpp` scene, **20,001** rays, **200 x 200** solar grid, **fixed seed 123456789**, File output for all surfaces and a new directory (or No export). Do not edit/save the committed scene. From PowerShell:

```powershell
$exe = (Resolve-Path 'build/application/Release/tonatiuhpp.exe').Path
$scene = (Resolve-Path 'examples/benchmarks/cylinder.tnhpp').Path
$env:TONATIUHPP_A0_RAY_HISTORY_FILE = "$PWD\build\a0-history-gui.json"
$env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE = "$PWD\build\a0-history-gui-hits.json"
& $exe $scene
```

Check console shows `gui_master_seed: 123456789 (fixed)`, `a0_native_gui_ray_history:` and `a0_native_gui_signature:`. Exit the GUI. Run a separate headless trace:

```powershell
$env:TONATIUHPP_A0_RAY_HISTORY_FILE = "$PWD\build\a0-history-headless.json"
$env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE = "$PWD\build\a0-history-headless-hits.json"
& $exe --headless trace-scene $scene --rays 20001 --seed 123456789 --no-export
Remove-Item Env:TONATIUHPP_A0_RAY_HISTORY_FILE -ErrorAction SilentlyContinue
Remove-Item Env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE -ErrorAction SilentlyContinue
python tests/scientific/compare_native_ray_histories.py build/a0-history-gui.json build/a0-history-headless.json
python tests/scientific/compare_native_trace_signatures.py build/a0-history-gui-hits.json build/a0-history-headless-hits.json
```

Both comparators must exit zero; the console must show 123456789 **fixed**, not an automatic seed. Repeat the GUI acceptance for the unchanged `one-axis-tracker.tnhpp` fixture with the headless flag `--update-trackers`. Do not claim actual GUI parity solely from headless CI.

## Scope boundaries

The ordered ray-history signature proves equality of **observed per-ray propagation geometry, interaction sequence and termination reason** for the tested within-platform/toolchain scene and seed. It does not prove unobserved intermediate RNG draws, universal physics equivalence, cross-platform bitwise identity or full 500-million-ray benchmark closure. Frozen reference data, baseline optics, scheduler, native photon formats and CLI defaults remain untouched.
