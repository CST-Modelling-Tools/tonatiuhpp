# Scientific optical results in Python studies — local milestone

This layer extends `run_study` without modifying Tonatiuh++ scientific C++ code.
It delegates optical calculations to existing **headless** scene operations:

- `tn.FindInterception(surface_url, rays)` calculates **front-side intercepted power (W)** using the existing C++ interception service. The existing script method does not accept a seed; the headless service defaults to master seed **0**.
- `tn.RunFluxAnalysis(surface_url, side, rays, height_bins, width_bins, path, false, seed)` uses the existing seeded C++ flux service and writes the established tab-separated grid in **W/m²**. It updates trackers and limits recorded rays to **200,000 per call** and grid cells to **1,000,000**.
- `tn.writeJson(...)` writes the scalar measurement and exact configuration. Python verifies the native outputs, records the grid SHA-256 and calculates min/max/mean of *native grid cells*. The mean is descriptive, **not a replacement for integrated intercepted power**.

These are **two separate native simulations in addition to** `trace-scene`. Their ray counts and seeds are documented independently; do not treat the three results as one shared photon history. Flux uses the study case seed. Interception currently uses seed 0. Both native scientific services update trackers. `StudyCase.update_trackers=False` controls only the **separate CLI trace**, not these optical analyses.

## Example

```python
from tonatiuhpp_automation import OpticalRequest, StudyCase, TonatiuhClient, run_study

client = TonatiuhClient(r"build\bin\Release\tonatiuhpp-cli.exe", timeout=120)
request = OpticalRequest(
    surface_url="//Node/M3_Flux_Parity/Receiver/Shape",  # URL must exist in YOUR scene
    interception_rays=2048,
    flux_rays=2048,
    height_bins=8,
    width_bins=8,
    surface_side="front",
)
results = run_study(client, "my_scene.tnhpp", [
    StudyCase("noon", 180, 75, 2048, 12345, optical=request),
], "my_new_output_directory")
for result in results:
    print(result.optical.intercepted_power_watts)
    print(result.optical.flux_grid, result.optical.flux_mean_w_m2)
```

Per case, the resulting directory retains `measure_optics.tnhpps`, `optical_measurements.json`, `flux_grid_w_m2.tsv`, and `optical_summary.json`, in addition to the existing scene, trace logs and metadata. Files use the original native flux numeric formatting; the file hash is for **same-machine repeatability**, not an assertion of universal bit-identical floating-point results across operating systems.

## Failure and reproducibility checks

The package validates optical requests **before creating the study directory**, including the backend's flux limits, grid shape, and the surface side. It rejects nonfinite/non-numeric power and flux values and wrong native output dimensions. Native failures propagate; work remains in a fresh case directory for diagnosis. Existing directories are never overwritten. The unit suite injects bogus output files to test these guardrails without faking scientific calculations. The bounded native test constructs the same planar receiver style used by Tonatiuh++'s existing GUI/headless flux-parity fixture, repeats two identical conditions and verifies that power and raw grid bytes match on the **same machine**.

On Windows run the supplied installer script, which will execute focused unit tests, the existing 2 x 1024-ray study smoke, and the new bounded 6 x 512-ray optical smoke. It will not run the frozen 500-million-ray benchmark, build C++, commit, push, or start CI.
