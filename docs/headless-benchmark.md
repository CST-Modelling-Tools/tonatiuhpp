# Headless Benchmark

Tonatiuh++ runs benchmark v1 through the existing executable:

```text
tonatiuhpp --headless benchmark benchmark_config.json
```

The benchmark config is a JSON object. The formal schema is `docs/benchmark_config_schema_v1.json`, and an example template is available at `examples/benchmarks/benchmark_config_v1.example.json`.

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

## Flux Grid CSV

Set `flux_grid_output_file` to write the computed flux grid as CSV:

```json
{
  "flux_grid_output_file": "benchmark_flux_grid.csv"
}
```

The CSV contains `target_grid.height` rows and `target_grid.width` columns. Values are MW/m2, written in the same row-major order used by `flux_grid_sha256`. The SHA256 is still computed from the deterministic little-endian float64 binary representation, not from the CSV text.

When a grid is written, result JSON includes both fields:

```json
{
  "flux_grid_output_file": "D:/bench/benchmark_flux_grid.csv",
  "flux_grid_sha256": "..."
}
```

## Reference Grid Comparison

Reference JSON can compare scalar metrics, the flux-grid hash, and a separate grid CSV:

```json
{
  "total_power_mw": 42.0,
  "maximum_flux_mw_m2": 20.0,
  "flux_grid_sha256": "...",
  "flux_grid_file": "benchmark_flux_grid.csv",
  "tolerances": {
    "total_power_relative_percent": 0.1,
    "maximum_flux_relative_percent": 5.0
  }
}
```

`flux_grid_file` paths in reference JSON are resolved relative to the reference JSON file. Config-level `reference_flux_grid_file` is also supported and is resolved relative to the benchmark config file.

When a reference grid is provided, result JSON adds:

```json
{
  "flux_grid_hash_matches": true,
  "maximum_flux_grid_absolute_error_mw_m2": 0.0,
  "maximum_flux_grid_relative_error_percent": 0.0,
  "rms_flux_grid_error_mw_m2": 0.0
}
```
