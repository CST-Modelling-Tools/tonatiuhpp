# M4 — Authentic heliostat benchmark scene

The original, frozen `examples/benchmarks/benchmark_heliostat_field_v1.tnhpp` scene has been copied byte-for-byte from the Tonatiuh++ benchmark-v2 reference deposit hosted by CERN Zenodo.

- **Primary source:** https://zenodo.org/records/21325848
- **File URL:** https://zenodo.org/records/21325848/files/benchmark_heliostat_field_v1.tnhpp?download=1
- **Version:** Original scene named by `benchmark_reference_500M_v2.json` and `benchmark_config_v2.example.json`.
- **Published file size:** 9,413,273 bytes.
- **Published MD5:** `e42985c98b965f084a958226851ed626`.
- **Git blob SHA-1 (the imported file):** `8b8ded3e4f97e49201e158f53251a46d1a99c1d8`.

The original file bytes are not edited, regenerated, or simplified. The temporary checksum-gated GitHub importer was removed after its successful verified import.

The cross-platform `tests/scientific/verify_reference_v2.py` now checks the frozen scene size/MD5 in addition to the reference JSON, binary-grid SHA-256, CSV identity and flux summary metrics. Use:

```powershell
python tests/scientific/verify_reference_v2.py
```

The repository benchmark-v2 example already references this scene by filename, so the reference and config scientific data are preserved.

**Scope:** This is dataset preservation and integrity validation, **not** a 500,000,000-ray numerical repeatability test. Routine CI does not run the enormous full benchmark. Validate headless scene loading and a medium ray run on the intended installed Windows build before the release-grade 500M scientific rerun. Confirm the dataset's reuse/redistribution terms with the depositor as part of the publication review; the provenance/checksum are not themselves a license grant.
