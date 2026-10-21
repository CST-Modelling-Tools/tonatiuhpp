# M3.3 — bounded native GUI / true-headless script parity (local gate)

**State (local Windows acceptance, October 21-22, 2026): PASSED.** Native GUI and true-headless executions of the same 2,048-ray script produced exactly matching six-row artifacts, with fixed seed `0`, 200x200 sun grid and power `63857.145538085264 W`. The two-process headless check is automated; the GUI comparison was performed and attested by the maintainer. Baseline: merged M3.2 PR #33. The closed/unmerged PR #34 was not integrated. GitHub integration, hosted CI and review are pending.

## Scientific contract under test

Both entry points run the *same* `tests/scripts/m3_3_gui_headless_parity.tnhpps` script, copied to a disposable build directory. The script constructs an in-memory planar `8 x 8` absorbing receiver, clears/inserts the document, sets sun azimuth `180` and elevation `90`, traces **2,048 rays** through `tn.FindInterception`, and writes `m3_3_parity_result.csv` using `DataObject`. The resulting artifact has six fixed metadata/scientific rows; the power value uses `Number.toPrecision(17)` to retain full double precision.

Automated `headless.m3_script_parity_baseline` executes the script twice in *independent native true-headless processes* from a different process working directory. It rejects failed scripts, wrong artifact location, malformed rows, nonnumeric power and any difference between the two complete artifacts. It saves `headless_expected.csv` for the manual GUI comparison. This automated gate proves repeatability **only for the headless execution path**.

The fixed-seed GUI acceptance uses the same script, with GUI ray-tracing settings **fixed seed `0`**, **`200 x 200` sun grid**, the same available plugins, no selected GUI insertion target (root insertion), and the same vacuum scene conditions. The GUI's legacy `FluxAnalysis` interception and the true-headless `SimulationSession` implementation remain distinct in M3.2; matching numerical results must be *measured*, not presumed. This fixture has no trackers, export, GUI-only drawing or large run. Historical M3.2 matched-input power on a similarly constructed fixture is prior supporting evidence only; no new fixed numerical reference has been frozen here.

## Windows procedure

1. Reconfigure/build the existing Release target after patch application. In the PowerShell session, restore the usual application/kernel/libraries/Qt/third-party DLL directories on `PATH`, as required by GoogleTest discovery.
2. Run `ctest --test-dir build -C Release -R '^headless[.]m3_script_parity_baseline$' --output-on-failure`. Do **not** rerun this test after the GUI comparison unless you intentionally regenerate the baseline.
3. Inspect the baseline at `build/tests/m3_script_parity/headless_expected.csv`.
4. **Save any open GUI scene before running the script.** In the native GUI, enable **fixed seed `0`** and **`200 x 200` sun grid** in the existing Ray Tracing configuration. Ensure root insertion is not redirected by scene-tree selection. Open `build/tests/m3_script_parity/script/m3_3_gui_headless_parity.tnhpps` in the native Script Editor and execute it. Verify that script execution finishes without errors and that the scene tree contains the `M3_3_Parity` receiver.
5. Compare `build/tests/m3_script_parity/script/m3_3_parity_result.csv` (GUI output) byte-for-byte after normalizing CRLF vs LF with `build/tests/m3_script_parity/headless_expected.csv` (headless baseline). On this bounded matched-input fixture, all six rows including the 17-digit power should match. If not, capture both files and the GUI configuration; do not edit physics to force equality.
6. Record compiler/configuration, exact test result, fixed seed, grid, script location, full-precision power, scene visibility and GUI responsiveness in the final local acceptance note.

## Completed local gates and scope

- **Planar scene + DataObject:** exact GUI/headless `6/6` CSV rows, 2,048 rays, fixed seed `0`, sun grid `200x200`, power `63857.145538085264 W`.
- **Tracker:** exact GUI/headless `12/12` CSV rows; sun azimuths `90` and `110` degrees, 2,048 rays per interception; see `docs/m3-script-tracker-parity-local.md`.
- **Scripting lifecycle:** bounded independent headless sequence `10/10` checks; same GUI Script Editor session showed correct file/line errors, recovered, and reproduced `11/11` stable DataObject artifact rows.
- **Scene save/reload:** Windows Release and `21/21` focused CTests passed; native GUI/headless artifacts matched exactly (`11/11` rows). Missing/corrupt file, overwrite and invalid-extension operations raised controlled errors and preserved the restored scene/power. GUI-saved files were independently read by the true-headless loader. Maintainer evidence: `build/tests/m3_3_scene_io_validation/run_20261021T182852739Z`.
- **Script-controlled cancellation/recovery:** Windows Release and `6/6` focused CTests passed; two native true-headless processes produced matching `7/7` stable CSV rows, and the GUI matched those rows. Two successful GUI recovery executions surrounded three controlled script errors, including unfinished trace cancellation and worker joining. The recovery fixture produced `0.071966489949341531` W per ray. Maintainer evidence: `build/tests/m3_3_script_cancel_validation/run_20261021T221901693Z`.

**Limits:** All native GUI acceptance is maintainer-attested Windows evidence, not hosted automated GUI testing. The `tn.startTrace` / `tn.cancelTrace` / `tn.finishTrace` API is a cooperative file-based **no-export** simulation-session facility; it cannot be used to infer arbitrary JavaScript interruption, photon-export parity, all existing-script runtime compatibility, universal scientific equivalence, or a GUI-independent build. Remaining M3 architectural exit gaps and M4/M5 work stay open. Hosted Windows/Linux/macOS CI, review, commit and merge are not yet complete. No 500-million-ray benchmark was rerun; its preserved references are unchanged.
