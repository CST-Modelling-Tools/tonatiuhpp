# Architectural A0 — one-axis tracker photon File-export equivalence (PR #20)

**Status:** Native Windows Release **28/28 selected tests passed**; three-platform CI passed; actual Windows native GUI/headless File-export parity **passed** on 2026-10-11. Based on PR #19 master `f1bd0e6e`. PR #20 final review and merge remain pending. This is a test-and-documentation-only milestone.

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

## Manual actual native GUI/File-export acceptance (separate gate; verified)

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

All six artifact paths are required, and both comparators must exit zero. This separate GUI File-export acceptance **was completed successfully** on Windows on 2026-10-11. Recorded outputs and limitations follow.

## Verified scientific acceptance — 2026-10-11

- Maintainer's Windows Release build succeeded; the selected scientific/tracker/photon-export CTests **28/28 passed**, including new `headless.photon_export_tracker_equivalence`, the existing cylinder File-export regression and the tracked-versus-legacy exact-signature regression.
- [GitHub Actions run #38100894385](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/38100894385) completed successfully on **Windows, Ubuntu and macOS** for PR head `5aafd878`.
- A real native Windows GUI File-export trace and an independent native headless `--photon-export-dir ... --update-trackers` trace of unchanged `tests/scientific/fixtures/one-axis-tracker.tnhpp` used **20,001 rays**, fixed seed **123456789** and equal sun-grid/scientific-normalization metadata. Exact-hit comparator: `A0 scientific hit signatures match (36650 events, seed 123456789).`
- Both true File-export outputs passed the canonical photon-record comparator: `Photon export canonical records match (56651 photons; power=0.2057; surfaces=3/3).` This matches **56,651** decoded physical photon records (exact global position bits, side and mapped surface instance URL) plus serialized power, without depending on file-record ordering or global sequential photon IDs.
- The first manual GUI attempt unintentionally used **automatic seed 53** while headless used 123456789; the comparators correctly rejected different hit and photon signatures. A corrected **fixed-seed GUI run** then matched both headless scientific-hit signatures and canonical File records. The initial mismatch did **not** establish any physics/export defect.
- This is actual **within-Windows / within-toolchain one-axis tracker** File-export parity, **not** whole ray-history equivalence, all tracker/scene coverage, raw photon-file byte identity, cross-platform exact-bit equality or frozen full 500-million-ray benchmark acceptance.
## Deferred A0 gates

This test only establishes within-platform canonical photon-record multiset parity for one axis. It does not assert raw byte-for-byte file equality, complete per-ray histories, parity across all tracker types, cross-platform exact bit identity or full 500-million-ray benchmark acceptance.
