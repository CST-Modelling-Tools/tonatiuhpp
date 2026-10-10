# Architectural A0 — opt-in photon file export scientific equivalence

**Status:** draft PR #18; awaiting local Windows build and three-platform CI.

## Purpose and limitation

PRs #16–17 established real native GUI/headless exact-hit parity for the cylinder and one-axis tracker fixtures. The existing CLI `--headless trace-scene` was deliberately `--no-export` only. This change adds a **limited opt-in A0 diagnostic** with `--photon-export-dir PATH`, requiring the existing `TONATIUHPP_A0_TRACE_SIGNATURE_FILE` environment variable, and no more than 200,000 rays. The default headless CLI path and benchmark workflows remain unchanged.

The diagnostic dynamically loads the existing `PhotonsFile` plugin. It uses the same `PhotonsBuffer` and production `RayTracer` photon-recording branch as the GUI, with global coordinates, surface URL/ID and side enabled. It emits `PhotonMap.dat` and `PhotonMap_parameters.txt` in a caller-selected directory. No new photon binary format, RNG operation, optics, worker scheduling or benchmark reference is introduced.

## Scientific test contract

`headless.photon_export_scientific_equivalence` launches **three independent real headless executable processes**, each with 20,001 rays and fixed seed 123456789 on the cylinder fixture: two with photon-file export to separate build-tree directories, one with legacy `--no-export`. All produce PR #16 exact hit-event signature JSON files.

The test asserts that the two file-export runs and the non-recording run produce the **same exact sorted hit multiset** and energy normalization, then decodes both real `PhotonsFile` binary and parameter files and verifies their physical photon records are identical as multisets, including exact position bits, front/back flags and mapped surface instance URLs. Global sequential photon IDs and file-record ordering are **not** treated as scientific quantities: parallel workers can complete chunks in different orders. Photon power metadata must match exactly after normal decimal serialization; invalid files, absent surfaces and truncated records fail the test.

For direct local reproduction:

```powershell
cmake --build build --config Release --parallel 16
ctest --test-dir build -C Release -R '^scientific[.]|^headless[.]photon_export_scientific_equivalence' --output-on-failure
```

## Manual native GUI photon export acceptance

For this PR, the automated gate verifies the **actual existing file exporter plugin** through native headless processes; the GUI's real plugin dialog/export path has not been automatically exercised. For a native GUI check, open `examples/benchmarks/cylinder.tnhpp`, select 20,001 rays, fixed seed 123456789, 200 × 200 grid, `File` output, a new empty export directory, and the global-coordinate/surface-ID/surface-side fields. Leave previous/next IDs disabled and use one output file. Run the trace with the A0 signature environment variable pointing at a distinct signature JSON.

Independently run the new headless export diagnostic for the same fixture, seed, ray count and grid:

```powershell
$exe = (Resolve-Path 'build/application/Release/tonatiuhpp.exe').Path
$scene = (Resolve-Path 'examples/benchmarks/cylinder.tnhpp').Path
$env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE = "$PWD\build\a0-photon-headless.json"
& $exe --headless trace-scene $scene --rays 20001 --seed 123456789 --photon-export-dir "$PWD\build\a0-photon-headless"
Remove-Item Env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE
```

Compare native GUI versus headless scientific signature using the existing `compare_native_trace_signatures.py`, and decoded real photon records via `compare_photon_exports.py`. Preserve the original GUI export-directory contents if debugging a mismatch, and do not reset the frozen reference.

## Out of scope

Complete per-ray history matching, cross-platform bitwise identity, tracker export-file acceptance, GUI-export automatic automation and the full 500M benchmark reference validation remain separate milestones. No claim of completed A0 closeout is made here.
