# External Python automation

This is an initial, **optional external Python adapter** for the existing
`tonatiuhpp-cli` executable. Tonatiuh++ remains the C++ optical simulator;
Python does not run inside the GUI or replace its JavaScript engine.

The package has **no runtime dependencies beyond Python 3.10+ standard library**.
Install from the repository root:

```sh
python -m pip install -e .
```

Build Tonatiuh++ first or use an installed `tonatiuhpp-cli`. Then:

```python
from tonatiuhpp_automation import TonatiuhClient
client = TonatiuhClient("build/application/tonatiuhpp-cli", timeout=120)
client.validate_scene("examples/benchmarks/cylinder.tnhpp")
trace = client.trace_scene("examples/benchmarks/cylinder.tnhpp",
                           rays=2048, seed=123456789)
print(trace.stdout)
```

On Windows the usual build-tree executable is
`build/bin/Release/tonatiuhpp-cli.exe`. Use that path instead.
The adapter requires the compiled executable, native runtime libraries and
official scientific plugins. Paths supplied for scenes and scripts are made
absolute; a configurable `cwd` and `env` can be supplied.

`trace_scene` defaults to `--no-export`. To export photon Files, first
create an **empty directory**, then supply `photon_export_dir` plus optional
`photon_file_name`, `photon_file_size` and `photon_surfaces`. The adapter
does not create, overwrite, or clean the export directory. `update_trackers`
is an explicit optional flag, matching the CLI.

`run_script(path)` executes an existing Tonatiuh++ **JavaScript**
`.tnhpps` script via the CLI. It does not interpret Python inside Tonatiuh++.
Successful operations return a `CliResult` containing the command, exit
code, stdout and stderr; failures raise `CliError` with the failed result
attached when available. The CLI's progress and elapsed-time messages are not
a structured scientific-output API: consume documented photon exports or
script-written JSON. The 500-million-ray benchmark is deliberately not wrapped.

## Validation

```sh
python -m unittest discover -s tests/python_automation -p 'test_*.py' -v
python tests/python_automation/native_smoke.py --cli /path/to/tonatiuhpp-cli --scene examples/benchmarks/cylinder.tnhpp
```

The second command runs a **bounded 2048-ray** trace and never executes or
regenerates frozen benchmark references. CI exercises the smoke against the
production executable on Windows, Ubuntu and macOS.

Future Python integrations should improve documented scene configuration and
machine-readable results only after inspecting actual CLI/script contracts.
Optimization, weather data and annual-performance workflows belong in
specialized downstream Python tools, not in Tonatiuh++.
