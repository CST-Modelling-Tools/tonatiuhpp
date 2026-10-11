# Architectural A0 — one-axis tracker photon File-export equivalence (PR #20)

**Status:** Draft; native Windows Release, three-platform CI and actual GUI/File-export acceptance pending. Based on PR #19 merge `f1bd0e6e`. This is a test-and-documentation-only milestone.

## Scope and earlier evidence

PR #17 established actual Windows native GUI/headless exact-hit parity for the unchanged `tests/scientific/fixtures/one-axis-tracker.tnhpp` fixture: **36,650 hits**, 20,001 rays, fixed seed 123456789, 200 x 200 sun grid, with `--update-trackers` on headless. Legacy headless tracking intentionally differs.

PR #18 established GUI/headless **File-plugin photon-record parity for the cylinder**, not for a tracker. This change extends that existing scientific regression to the one-axis tracker without changing production tracing, RNG, scheduling, native photon-file formats, or frozen benchmarks.

## Automated regression

`headless.photon_export_tracker_equivalence` runs the real headless executable in **three separate processes** with 20,001 rays, fixed seed 123456789 and the default 200 x 200 sun grid:

1. Tracked File export to a new `first` directory.
2. Independent tracked File export to a new `second` directory.
3. Tracked `--no-export` reference, testing whether recording affects scientific hits.

All three must pass `--update-trackers` and report `tracker_update: enabled`. The existing exact-hit signature comparator checks scientific scene identity, seed/grid, exact sun-aperture and energy-normalization bits, hit counts and sorted raw-position-bit surface/side event digest for exported repeatability and exported versus non-recording parity.

The existing binary photon File-export comparator also requires that `PhotonMap.dat` and `PhotonMap_parameters.txt` from both exported runs decode to the **same physical photon-record multiset** (exact global coordinate bits, side, mapped surface URL), count and serialized power. Photon file record order and global sequential IDs are not scientifically stable across parallel worker runs.

The existing `headless.trace_tracker_exact_signatures` separately verifies that opt-in tracked and legacy paths differ scientifically and remain repeatable. The existing cylinder `headless.photon_export_scientific_equivalence` still omits `--update-trackers`; its execution is unchanged.

### Reproduce on Windows Release

```powershell
cd C:\OpenSource\tonatiuhpp
git switch test/a0-tracker-photon-export-equivalence-20261011
cmake --build build --config Release --parallel 16
ctest --test-dir build -C Release -R '^scientific[.]|^headless[.]trace_tracker_|^headless[.]photon_export_' --output-on-failure
```

## Manual actual native GUI/File-export acceptance (separate gate; pending)

The automated CTest runs **headless** processes and cannot prove that the GUI File exporter was selected or that GUI photon files match. Perform this independent GUI check on Windows with the unchanged saved scene:

```powershell
$exe = (Resolve-Path 'build/application/Release/tonatiuhpp.exe').Path
$scene = (Resolve-Path 'tests/scientific/fixtures/one-axis-tracker.tnhpp').Path
New-Item -ItemType Directory -Force build/a0-tracker-photon-gui | Out-Null
Remove-Item build/a0-tracker-photon-gui.json -ErrorAction SilentlyContinue
$env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE = "$PWD\build\a0-tracker-photon-gui.json"
& $exe $scene
```

In **Ray tracing > Parameters**, use **20,001 rays**, **200 x 200** sun grid, fixed seed **123456789**; select **File** output, directory `C:\OpenSource\tonatiuhpp\build\a0-tracker-photon-gui`, file basename **PhotonMap** and **Photons per file unchecked**. Enable global coordinates, surface ID and surface side; disable previous/next photon IDs and buffer append, select all surfaces. Run the GUI trace once. Confirm no scene-modification or export error and exit the GUI.

Separately run the opt-in tracked headless File export and compare:

```powershell
$env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE = "$PWD\build\a0-tracker-photon-headless.json"
& $exe --headless trace-scene $scene --rays 20001 --seed 123456789 --photon-export-dir "$PWD\build\a0-tracker-photon-headless" --update-trackers
Remove-Item Env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE -ErrorAction SilentlyContinue

Test-Path build/a0-tracker-photon-gui.json
Test-Path build/a0-tracker-photon-headless.json
Test-Path build/a0-tracker-photon-gui/PhotonMap.dat
Test-Path build/a0-tracker-photon-gui/PhotonMap_parameters.txt
Test-Path build/a0-tracker-photon-headless/PhotonMap.dat
Test-Path build/a0-tracker-photon-headless/PhotonMap_parameters.txt

python tests/scientific/compare_native_trace_signatures.py build/a0-tracker-photon-gui.json build/a0-tracker-photon-headless.json
python tests/scientific/compare_photon_exports.py build/a0-tracker-photon-gui build/a0-tracker-photon-headless
```

All six paths must exist and both comparators must exit zero. The prior **36,650-event** figure is evidence from hit-only tracker validation; **do not infer a photon record count or accept GUI File-export parity without the actual export checks**.

## Deferred A0 gates

This test only establishes within-platform canonical photon-record multiset parity for one axis. It does not assert raw byte-for-byte file equality, complete per-ray histories, parity across all tracker types, cross-platform exact bit identity or full 500-million-ray benchmark acceptance.
