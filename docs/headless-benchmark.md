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
