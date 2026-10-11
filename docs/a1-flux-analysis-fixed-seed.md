# Architectural A1 — GUI Flux Analysis fixed-seed characterization

**Status:** Bounded Windows GUI export repeatability **PASSED (maintainer-confirmed, 2026-10-11)**; PR #26 code review and merge still pending. Base: merged `master` `d614a635`. This work does **not** revise the frozen 500M benchmark reference, optics, ray scheduling, flux-area calculations, binning or exported file format.

## Motivation and narrowly scoped fix

The regular GUI tracing dialog already stores an optional fixed master seed. Its `MainWindow::Run()` path honors that option. In contrast, `MainWindow::RunFluxAnalysisDialog()` previously constructed Flux Analysis with a newly resolved **automatic** seed, even if the user had selected a fixed seed. The A1 change routes Flux Analysis through the existing `GuiTraceSeed::resolve` policy. Unselected seeds remain automatic.

Each Flux Analysis run now logs `flux_analysis_master_seed: <integer>`. The existing per-run seed increment remains unchanged, including on the **Append** path. The logging is informational and should be retained as acceptance evidence, not used as a numerical reference.

## What the new comparator proves

`tests/scientific/compare_gui_flux_exports.py` validates and compares two independent legacy **Data (*.txt)** GUI Flux Analysis exports. It requires the same supplied U/V cell dimensions, strictly finite nonnegative flux values, at least one positive cell and exact exported-file byte equality. It prints the file SHA-256 and summary. Native `.txt` output uses W/m2 and includes a trailing tab per row; native text formatting limits comparison to **saved-file precision**. Reject mismatches rather than changing the accepted scientific tolerance.

Synthetic positive and negative tests are included in normal three-platform CI. They do not run an actual GUI; native acceptance additionally requires the independent real-GUI test documented below.

## Manual native Windows acceptance

1. From PR branch on a clean Windows Release build, open the **unchanged** `examples/benchmarks/cylinder.tnhpp` in the real GUI. Do not save or edit the committed fixture.
2. In the ordinary **Ray Tracing** dialog, enable the existing **Fixed seed** option with **123456789** and set the sun grid to **200 × 200**. **Accepting this dialog immediately starts an ordinary ray-tracing run; it is not settings-only.** Choose a small ray count and no photon-file export for that preparatory run, then let it finish before opening Flux Analysis.
3. Open **Flux Analysis** from the GUI. Select the cylinder receiver surface, **front** side, **20,001** rays, **20 × 20** cells, and ensure **Append** is disabled. Click **Run**, and retain the console line `flux_analysis_master_seed: 123456789`. Export **Data (*.txt)** to `build/a1-flux-gui-run1.txt`.
4. **Close the entire application and relaunch it** to establish a second independent GUI process. Repeat the identical fixture and fixed-seed setup (including the preparatory ordinary ray trace), reopen Flux Analysis with Append disabled, and export separately to `build/a1-flux-gui-run2.txt`. Confirm the second log also reports **123456789**.
5. Compare the **two real generated** exports (no synthetic copied files):

```powershell
python -m unittest discover -s tests/scientific -p test_compare_gui_flux_exports.py -v
python tests/scientific/compare_gui_flux_exports.py build/a1-flux-gui-run1.txt build/a1-flux-gui-run2.txt --rows 20 --cols 20
```

The comparator must report **PASS**, and both console logs must identify the fixed seed. Retain executable SHA-256, exact commit, working-tree status and original exported files. Optionally repeat with **Append** enabled and separately document its expected incremented seed; **do not** compare appended output to fresh single-run output as though it had the same sample set.

## Recorded acceptance (2026-10-11)

- Maintainer Windows Release build: **PASS**, with pre-existing FluxAnalysis.cpp MSVC warnings C4834 and C4805.
- Standard-library comparator regressions: **17/17 PASS**; native scientific CTests: **23/23 PASS**.
- [GitHub Actions run #38144165974](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/38144165974): **PASS** on Windows, Ubuntu and macOS.
- The maintainer confirmed **two separate GUI application executions**, each reporting `flux_analysis_master_seed: 123456789`. The two original saved 20x20 `Data (*.txt)` exports were compared locally with the script above: **PASS**, 200 strictly positive cells, maximum **1161.08 W/m2**, and matching saved-file SHA-256 `f79e6f8d85328aae86ea34e54b6183313285423c0ae8b6e39ea3eb9996b6d68e`.
- These are user-reported native acceptance results, not an independent replay or direct inspection of both application console logs. Preserve those original logs/exports and executable provenance with the local validation transcript where possible. Only this bounded repeatability contract is closed; the broader exclusions below remain.

## Explicit exclusions and follow-up

This is a **within-platform two-GUI-process exported-grid repeatability** check for the cylinder fixture. It does **not** establish equality with the headless benchmark (which has a different receiver contract), bitwise unrounded in-memory flux results, correctness of the GUI's flux statistics, other geometry/material/air models, cancellation, or cross-platform scientific parity.

Existing Flux Analysis `UpdateAnalysis`, `fillBins` and `write` numerical formulas remain untouched. In particular, comments in `FluxAnalysisDialog.cpp` already call out a suspicious average-flux calculation. Changing those formulas requires a separate characterization against trustworthy analytical geometry and explicit scientific review.
