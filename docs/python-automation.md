# External Python automation for Tonatiuh++

The optional `tonatiuhpp_automation` package (Python 3.10+, standard library only)
operates the existing `tonatiuhpp-cli` process. Python is **not** embedded into
Tonatiuh++; its GUI's JavaScript editor and C++ ray-tracing/scientific services
remain unchanged. No supplied Python workflow invokes the frozen 500-million-ray
benchmark.

## Installation and CLI path

From the repository root, with the native GUI/CLI already built:

```powershell
python -m pip install -e .
```

On Windows a typical executable is `build/bin/Release/tonatiuhpp-cli.exe`.
On Unix-like development builds it is commonly `build/application/tonatiuhpp-cli`.
Pass the actual existing executable path to `TonatiuhClient`. C++ plugins and
runtime dependencies must be available to that executable.

## Basic subprocess operations

```python
from tonatiuhpp_automation import TonatiuhClient

client = TonatiuhClient(r"build\bin\Release\tonatiuhpp-cli.exe", timeout=120)
client.validate_scene("examples/benchmarks/cylinder.tnhpp")
trace = client.trace_scene("examples/benchmarks/cylinder.tnhpp",
                           rays=2048, seed=123456789,
                           update_trackers=True)
print(trace.stdout)
```

`trace_scene` defaults to no photon export. For native photon File export,
create a **new empty directory** and pass `photon_export_dir` (and optional
file name, segment size and surface URLs). `run_script(path)` executes an
existing `.tnhpps` JavaScript file. `CliResult` captures the exit code,
stdout and stderr; nonzero process exits raise `CliError` with `.result`.
A timeout raises `CliError` without a completed result. Console counters
and timings are not scientific power or flux measurements.

## Multiple sun conditions and optical results

`run_study` creates a **new** output directory (never overwrites an existing
study); each case starts from the same source `.tnhpp` scene. It uses native
headless JavaScript to load/change/save a separate scene, then the real CLI
to trace it, optionally export photons, and optionally obtain scientific results.

```python
from tonatiuhpp_automation import (
    FieldEdit, OpticalRequest, StudyCase, TonatiuhClient, run_study,
)

client = TonatiuhClient(r"build\bin\Release\tonatiuhpp-cli.exe", timeout=120)
receiver = OpticalRequest(
    surface_url="//Node/MyField/Receiver/Shape",  # replace with a real shape URL
    interception_rays=2048, flux_rays=2048,
    height_bins=8, width_bins=8, surface_side="front",
)
results = run_study(
    client, "my_scene.tnhpp",
    (
        StudyCase("morning", 90, 35, 2048, 12345, optical=receiver),
        StudyCase("afternoon", 245, 60, 2048, 12345, optical=receiver),
    ),
    "my_new_output_directory",  # must not already exist
)
for result in results:
    print(result.case.name, result.execution_stats)
    print(result.optical.intercepted_power_watts, result.optical.flux_grid)
    print(result.status_json)
```

Use `FieldEdit(node_url, field_name, value)` in `StudyCase.field_edits` to edit
an existing Coin scene field using native `tn.SetValue`. Sun azimuth and
elevation are in **degrees**, following `tn.ChangeSunPosition`. The CLI's
`update_trackers` option defaults to true **in studies** and false in direct
`trace_scene` calls. Native `FindInterception` and `RunFluxAnalysis` always
update trackers independently; `StudyCase.update_trackers=False` controls only
the CLI trace, not those scientific analyses.

The optional scientific measurements are **separate C++ traces**:

- `tn.FindInterception` returns **front-side intercepted power in W** using its
  current fixed default seed **0** (not the study case seed).
- `tn.RunFluxAnalysis` uses the study case seed and produces the legacy
  tab-separated grid in **W/m²**, with up to 200,000 rays and 1,000,000 cells.
  The requested `surface_side` can be front or back. The grid mean is a cell
  statistic, **not** total intercepted power.
- The CLI execution trace uses the case's explicit rays/seed; its counters
  describe a third independent native trace. Do not combine the three traces
  as if they shared a photon history.

The native flux TSV is retained unmodified, along with `optical_summary.json`
containing a SHA-256 of its bytes. Same-machine fixed-seed repeatability is
tested; byte-identical results across operating systems are **not** guaranteed.

## Files and failures

Each case directory contains `prepare_scene.tnhpps`, `prepared_scene.tnhpp`,
`scene_preparation.json`, `trace_stdout.txt`, `trace_stderr.txt`,
`trace_result.json` and `case_status.json`. Optical cases also contain
`measure_optics.tnhpps`, `optical_measurements.json`, `flux_grid_w_m2.tsv` and
`optical_summary.json`. Optional native photon Files are in `photons/`.

`case_status.json` indicates `running`, `completed`, or `failed` and identifies
the failing stage (`prepare`, `trace`, `optics`). On a nonzero native exit, the
captured per-stage stdout/stderr are retained (`prepare_stderr.txt`, etc.).
The original exception is raised, and successful outputs from earlier stages
remain available for investigation. **Do not interpret `trace_result.json` as
proof of whole-case completion:** check `case_status.json` for `completed`.
The source scene is not overwritten. Existing output directories are rejected.

## Local validation and project scope

From the repository root:

```powershell
python -m unittest discover -s tests/python_automation -p "test_*.py" -v
python tests/python_automation/native_smoke.py --cli .\build\bin\Release\tonatiuhpp-cli.exe --scene .\examples\benchmarks\cylinder.tnhpp
python tests/python_automation/native_study_smoke.py --cli .\build\bin\Release\tonatiuhpp-cli.exe --scene .\examples\benchmarks\cylinder.tnhpp
python tests/python_automation/native_optics_smoke.py --cli .\build\bin\Release\tonatiuhpp-cli.exe
```

These are bounded tests; the frozen 500-million-ray benchmark is never run or
changed. Following local validation, cross-platform CI is the final PR gate.
High-level weather processing, annual performance, and optimization should be
built downstream against this interface rather than inside the optical kernel.
