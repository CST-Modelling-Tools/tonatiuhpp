# Architectural A0 — opt-in photon file export scientific equivalence

**Status:** PR #18 merged as master `3e6c03ad` (2026-10-11). Corrected Windows Release 24/24 selected CTests, three-platform CI and manual native GUI/headless photon File-export parity passed for the cylinder fixture; other A0 gates remain open.

## Purpose and limitation

PRs #16–17 established real native GUI/headless exact-hit parity for the cylinder and one-axis tracker fixtures. The existing CLI `--headless trace-scene` was deliberately `--no-export` only. This change adds a **limited opt-in A0 diagnostic** with `--photon-export-dir PATH`, requiring the existing `TONATIUHPP_A0_TRACE_SIGNATURE_FILE` environment variable, and no more than 200,000 rays. The default headless CLI path and benchmark workflows remain unchanged.

The diagnostic dynamically loads the existing `PhotonsFile` plugin. It uses the same `PhotonsBuffer` and production `RayTracer` photon-recording branch as the GUI, with global coordinates, surface URL/ID and side enabled. It emits `PhotonMap.dat` and `PhotonMap_parameters.txt` in a caller-selected directory. No new photon binary format, RNG operation, optics, worker scheduling or benchmark reference is introduced.

## Initial integration failure and corrective implementation

The initial draft commit `058e242e` added CLI help text for `--photon-export-dir` but accidentally omitted the corresponding `HeadlessCommandRunner.cpp` parser and export implementation. The Windows build succeeded, 23 existing scientific tests passed, but `headless.photon_export_scientific_equivalence` returned `Unknown trace-scene option: --photon-export-dir.` before tracing or exporting photons. Initial GitHub Actions Test steps failed on all three platforms.

Corrective commit `a3b1057f` installed the option parser, opt-in File-plugin loader, `PhotonsBuffer`-backed recording and export completion handling. The first failure was **not** a scientific mismatch; corrected CI and native GUI/File-export acceptance subsequently passed.

## Scientific test contract

`headless.photon_export_scientific_equivalence` launches **three independent real headless executable processes**, each with 20,001 rays and fixed seed 123456789 on the cylinder fixture: two with photon-file export to separate build-tree directories, one with legacy `--no-export`. All produce PR #16 exact hit-event signature JSON files.

The test asserts that the two file-export runs and the non-recording run produce the **same exact sorted hit multiset** and energy normalization, then decodes both real `PhotonsFile` binary and parameter files and verifies their physical photon records are identical as multisets, including exact position bits, front/back flags and mapped surface instance URLs. Global sequential photon IDs and file-record ordering are **not** treated as scientific quantities: parallel workers can complete chunks in different orders. Photon power metadata must match exactly after normal decimal serialization; invalid files, absent surfaces and truncated records fail the test.

For direct local reproduction:

```powershell
cmake --build build --config Release --parallel 16
ctest --test-dir build -C Release -R '^scientific[.]|^headless[.]photon_export_scientific_equivalence' --output-on-failure
```

## Manual native GUI photon export acceptance

The automated gate verifies the **actual existing file exporter plugin** through native headless processes. The GUI File-export dialog and resulting files were verified separately by a successful manual Windows acceptance test. For a native GUI check, open `examples/benchmarks/cylinder.tnhpp`, select 20,001 rays, fixed seed 123456789, 200 × 200 grid, `File` output, a new empty export directory, and the global-coordinate/surface-ID/surface-side fields. Leave previous/next IDs disabled and use one output file. Run the trace with the A0 signature environment variable pointing at a distinct signature JSON.

Independently run the new headless export diagnostic for the same fixture, seed, ray count and grid:

```powershell
$exe = (Resolve-Path 'build/application/Release/tonatiuhpp.exe').Path
$scene = (Resolve-Path 'examples/benchmarks/cylinder.tnhpp').Path
$env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE = "$PWD\build\a0-photon-headless.json"
& $exe --headless trace-scene $scene --rays 20001 --seed 123456789 --photon-export-dir "$PWD\build\a0-photon-headless"
Remove-Item Env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE
```

Compare native GUI versus headless scientific signature using the existing `compare_native_trace_signatures.py`, and decoded real photon records via `compare_photon_exports.py`. Preserve the original GUI export-directory contents if debugging a mismatch, and do not reset the frozen reference.

## Verified native photon File-export scientific acceptance (Windows, 2026-10-11)

- Corrected commit `a3b1057f` passed a Windows Release build and **24/24 selected CTests**, including the native three-process File-export regression.
- [GitHub Actions run #163](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/38095905833) passed on **Windows, Ubuntu and macOS**.
- Actual native GUI and independently launched headless File-export execution on unchanged `examples/benchmarks/cylinder.tnhpp` matched with **20,001 rays**, seed **123456789**, and matching **200 × 200** sun grid and normalization metadata: `A0 scientific hit signatures match (10914 events, seed 123456789).`
- GUI files `PhotonMap.dat` (**1,483,920 bytes**), `PhotonMap_parameters.txt` (**127 bytes**) and its signature JSON were generated. The exact canonical record comparator returned `Photon export canonical records match (30915 photons; power=0.007369; surfaces=2/2).`
- Comparison covers order-independent physical photon records (exact global-coordinate bits, front/back side, mapped surface instance URL) and serialized photon power; **not** raw-file byte identity, ordered global IDs, complete ray histories, all tracker/scene exports or the frozen full 500M benchmark.
- Earlier missing GUI output artifacts were resolved by setting the GUI File exporter directory, basename `PhotonMap`, single-file mode, global coordinates, and surface ID/side; this was an export configuration issue rather than a scientific mismatch.
## Out of scope

Complete per-ray history matching, cross-platform bitwise identity, tracker export-file acceptance, GUI-export automatic automation and the full 500M benchmark reference validation remain separate milestones. No claim of completed A0 closeout is made here.
