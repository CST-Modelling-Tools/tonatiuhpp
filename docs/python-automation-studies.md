# Multi-condition Python optical studies (local development milestone)

This proposed extension to `tonatiuhpp-automation` uses the production
`tonatiuhpp-cli` executable and the **existing** headless script operations.
It does not embed Python, modify the C++ scientific kernel, or replace the
existing JavaScript editor. Nothing in this bundle changes `.github/workflows`.

## What it supports

- Reuse a single `.tnhpp` source scene for multiple independent runs.
- Change each run's sun azimuth and elevation in degrees via the existing
  `tn.ChangeSunPosition(azimuth, elevation)` function.
- Optionally edit scene fields via `tn.SetValue(node_url, field, value)`.
- Save a newly prepared `.tnhpp` scene without modifying the source scene.
- Trace with explicit `rays`, `seed` and `--update-trackers` (enabled by default).
- Optionally export native photon Files into a new, empty per-run folder.
- Preserve preparation scripts, scene files, metadata, stdout and stderr.
- Produce machine-readable **execution metadata** in `trace_result.json`.
  **This metadata is not a flux map or intercepted-power calculation.** Photon
  export remains the supported optical output until a separate scientifically
  verified result contract is added.

Example (from the repository root, with Python package importable):

```python
from tonatiuhpp_automation import StudyCase, TonatiuhClient, run_study

client = TonatiuhClient(r"build\bin\Release\tonatiuhpp-cli.exe", timeout=120)
results = run_study(
    client,
    "examples/benchmarks/cylinder.tnhpp",
    (
        StudyCase("morning", azimuth_deg=90, elevation_deg=45, rays=2048, seed=12345),
        StudyCase("afternoon", azimuth_deg=270, elevation_deg=45, rays=2048, seed=12345),
    ),
    "local_study_output",  # MUST NOT exist already
)
for result in results:
    print(result.case.name, result.execution_stats, result.result_json)
```

`run_study` never overwrites an existing output directory. It creates a
separate subdirectory per case and retains useful artifacts if a run fails.
The caller is responsible for sufficient disk capacity and choosing a bounded
ray count. Native ray tracing remains in C++.

## Important scientific and operational details

1. `tn.loadScene` initializes a scene and updates its trackers according to
   its *previous* sun position. `tn.ChangeSunPosition` then edits the sun
   field. The independent CLI tracing step receives `--update-trackers`, which
   updates tracking transforms for the **new** sun position. Disabling tracker
   updates is an explicit opt-out for studies intentionally preserving saved
   transforms.
2. `FieldEdit` uses existing Coin/scene URL semantics and requires real
   scene URLs; it does not invent a new geometry schema.
3. The adapter validates sun elevation in [-90, 90] degrees and portable
   32-bit ray/seed ranges; it does **not** constrain azimuth because the native
   scripting function accepts any finite degree value.
4. CLI trace output is text. This module parses only known execution fields
   and checks the reported ray count. For optical science, read native photon
   File output or develop a separately validated structured flux/interception
   API; do not mistake execution counters for physical simulation outputs.
5. Every study case launches a headless scene-preparation process followed by
   an independent native trace process. The overhead is real; benchmark and
   optimize it only after functional validation.

## Local Windows tests — no CI or commits

Keep the existing uncommitted `test_client.py` normalization fix. Run:

```powershell
python -m unittest discover -s tests/python_automation -p "test_*.py" -v
python tests/python_automation/native_study_smoke.py `
  --cli .\build\bin\Release\tonatiuhpp-cli.exe `
  --scene .\examples\benchmarks\cylinder.tnhpp
git diff --check
git status -sb
```

The native smoke performs **2 x 1,024 rays** without photon export. It uses a
new temporary output directory and does not touch the frozen 500-million-ray
benchmark artifacts. Do not commit or push until local Windows results have
been reviewed.
