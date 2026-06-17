# Headless Benchmark

Tonatiuh++ headless mode runs through the existing executable without creating the GUI application or GUI windows.

## Command Inventory

```text
tonatiuhpp --headless --help
tonatiuhpp --headless validate-scene path/to/scene.tnhpp
tonatiuhpp --headless trace-scene path/to/scene.tnhpp --rays 10000 --seed 123456789 --no-export
tonatiuhpp --headless benchmark path/to/benchmark_config.json
tonatiuhpp --headless run-script path/to/script.tnhpps
```

Exit codes are:

- `0`: command completed successfully
- `1`: scene loading, tracing, benchmark, script execution, or file I/O failed
- `2`: command-line usage error

`trace-scene` currently supports no-export execution only. It prints key-value lines suitable for logs, including `scene_file`, `rays`, `seed`, `photon_export`, `export_path`, `rays_traced`, `elapsed_seconds`, `rays_per_second`, `worker_count`, `chunk_count`, and `chunk_size`.

Benchmark mode also runs without photon export and writes result JSON. Its console output includes `benchmark`, `scene_file`, `rays`, `seed`, `photon_export`, `export_path`, `output_file`, `rays_traced`, `elapsed_seconds`, `rays_per_second`, scheduling fields, and `result_file`.

## Headless Scripts

`run-script` evaluates a `.tnhpps` file through a true headless `QCoreApplication` path:

```text
tonatiuhpp --headless run-script script.tnhpps
```

This is intentionally separate from legacy `tonatiuhpp -i script.tnhpps`, which still uses the GUI script infrastructure for compatibility. `tonatiuhpp --headless -i script.tnhpps` is not a supported script command; use `--headless run-script` for non-GUI automation.

The first headless script API is deliberately small:

```js
print(value)
tn.writeJson(path, value)
tn.validateScene(path)
tn.runBenchmark(path)
tn.traceScene({ scene, rays, seed, noExport: true })
```

`tonatiuh` is also available as an alias for the same limited object. GUI-only APIs such as screenshot capture, scene-tree editing, dialogs, widget access, or GUI-compatible `MainWindow` methods are not available in headless scripts. Unknown or GUI-only API calls fail with a script error instead of being silently ignored.

`tn.traceScene` uses the same QCoreApplication-compatible `RayTraceRunner` path as the headless `trace-scene` command. It supports deterministic no-export tracing only:

- `scene`: required `.tnhpp` scene path
- `rays`: required positive integer
- `seed`: optional integer, default `0`
- `noExport`: required `true`

It returns a JavaScript object with fields such as `scene_file`, `rays`, `seed`, `no_export`, `photon_export`, `export_path`, `rays_traced`, `elapsed_seconds`, `rays_per_second`, `worker_count`, `chunk_count`, `chunk_size`, `sun_aperture_area`, `irradiance`, and `power_per_ray`. Photon export remains unsupported in headless scripts.

Example:

```js
print("starting benchmark");

if (!tn.validateScene("examples/benchmarks/cylinder.tnhpp")) {
  throw new Error("scene validation failed");
}

var trace = tn.traceScene({
  scene: "examples/benchmarks/cylinder.tnhpp",
  rays: 1000,
  seed: 123456789,
  noExport: true
});

tn.writeJson("run-metadata.json", {
  scene: "examples/benchmarks/cylinder.tnhpp",
  trace: trace,
  benchmark: "benchmark_config.json"
});

var exitCode = tn.runBenchmark("benchmark_config.json");
if (exitCode !== 0) {
  throw new Error("benchmark failed with exit code " + exitCode);
}
```

## Benchmark Command

Tonatiuh++ runs benchmark v1 through the existing executable:

```text
tonatiuhpp --headless benchmark benchmark_config.json
```

The benchmark config is a JSON object. The formal schema is `docs/benchmark_config_schema_v1.json`, and an example template is available at `examples/benchmarks/benchmark_config_v1.example.json`.

## Benchmark Dataset

The Tonatiuh++ benchmark v1 reference dataset is archived on Zenodo:

```text
https://doi.org/10.5281/zenodo.20395328
```

Use the dataset scene, config, reference JSON, and flux-grid reference files when reproducing the published benchmark baseline.

## Scheduling Fields

`worker_count` and `chunk_size` are optional RayTraceRunner tuning fields.

| Field | Type | Default | Notes |
| --- | --- | --- | --- |
| `worker_count` | positive integer | `QThread::idealThreadCount()` | Effective value is written to result JSON as `worker_count`. |
| `chunk_size` | positive integer | `10000` | Effective value is written to result JSON as `chunk_size`; `chunk_count` is also written. |

The random stream is deterministic for a fixed scene, ray count, seed, worker strategy, and chunk size. Changing `chunk_size` changes deterministic chunk seeds, so `flux_grid_sha256` is expected to change.

## Result Fields

Benchmark result JSON always includes the effective scheduling fields:

```json
{
  "worker_count": 20,
  "chunk_count": 1000,
  "chunk_size": 10000
}
```

Whole-file byte-for-byte equality is not expected across repeated runs because elapsed time and rays/s vary. Use `flux_grid_sha256` and the metric fields for deterministic comparisons.

## Flux Grid Files

Set `flux_grid_output_file` to write the computed flux grid as CSV:

```json
{
  "flux_grid_output_file": "benchmark_flux_grid.csv"
}
```

The CSV contains `target_grid.height` rows and `target_grid.width` columns. Values are MW/m2, written in the same row-major order used by `flux_grid_sha256`. The SHA256 is still computed from the deterministic little-endian float64 binary representation, not from the CSV text.

Set `flux_grid_binary_output_file` to write the same grid as raw binary:

```json
{
  "flux_grid_binary_output_file": "benchmark_flux_grid.bin"
}
```

Binary format:

- raw binary file
- no header
- no metadata
- row-major order
- dimensions from `target_grid.width` and `target_grid.height`
- each value is IEEE 754 binary64 / double
- little-endian byte order
- values are flux density in MW/m2
- byte size is `target_grid.width * target_grid.height * 8`

When a grid is written, result JSON includes both fields:

```json
{
  "flux_grid_output_file": "D:/bench/benchmark_flux_grid.csv",
  "flux_grid_binary_output_file": "D:/bench/benchmark_flux_grid.bin",
  "flux_grid_sha256": "..."
}
```

Read examples:

Python:

```python
grid = numpy.fromfile(path, dtype="<f8").reshape((height, width))
```

Mathematica:

```wolfram
data = BinaryReadList[path, "Real64", ByteOrdering -> +1]
grid = Partition[data, width]
```

C++:

```cpp
std::vector<double> grid(width * height);
// Read width * height IEEE 754 binary64 values from the file.
// Interpret each 8-byte value as little-endian and keep row-major order.
```

## Reference Grid Comparison

Reference JSON can compare scalar metrics, the flux-grid hash, and a separate grid CSV:

```json
{
  "total_power_mw": 42.0,
  "maximum_flux_mw_m2": 20.0,
  "flux_grid_sha256": "...",
  "flux_grid_file": "benchmark_flux_grid.csv",
  "flux_grid_binary_file": "benchmark_flux_grid.bin",
  "target_grid": {
    "width": 100,
    "height": 100
  },
  "tolerances": {
    "total_power_relative_percent": 0.1,
    "maximum_flux_relative_percent": 5.0
  }
}
```

`flux_grid_file` and `flux_grid_binary_file` paths in reference JSON are resolved relative to the reference JSON file. Config-level `reference_flux_grid_file` and `reference_flux_grid_binary_file` are also supported and are resolved relative to the benchmark config file. If both CSV and binary reference grid files are provided, Tonatiuh++ uses the binary file for exact comparison and ignores the CSV grid for grid-error calculations.

When a reference grid is provided, result JSON adds:

```json
{
  "flux_grid_hash_matches": true,
  "maximum_flux_grid_absolute_error_mw_m2": 0.0,
  "maximum_flux_grid_relative_error_percent": 0.0,
  "rms_flux_grid_error_mw_m2": 0.0
}
```
