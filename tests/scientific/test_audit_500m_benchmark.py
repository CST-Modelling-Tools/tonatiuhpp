#!/usr/bin/env python3
"""Synthetic artifacts test only the *auditor*, never a 500M native trace."""

import hashlib
import json
import struct
import tempfile
import unittest
from pathlib import Path

from audit_500m_benchmark import AuditError, RAYS, SEED, audit_run


class FrozenBenchmarkAuditTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.frozen = root / "frozen"
        self.frozen.mkdir()
        self.run = root / "m5_full_1234567890"
        self.run.mkdir()
        (self.frozen / "benchmark_heliostat_field_v1.tnhpp").write_text(
            "synthetic scene; not a scientific reference", encoding="utf-8")
        values = [2.0] * 10000
        binary = struct.pack("<10000d", *values)
        digest = hashlib.sha256(binary).hexdigest()
        (self.frozen / "benchmark_reference_flux_grid_500M_v2.bin").write_bytes(binary)
        self.reference = {
            "flux_grid_sha256": digest,
            "total_power_mw": 32.0,
            "minimum_flux_mw_m2": 2.0,
            "average_flux_mw_m2": 2.0,
            "maximum_flux_mw_m2": 2.0,
        }
        (self.frozen / "benchmark_reference_500M_v2.json").write_text(
            json.dumps(self.reference), encoding="utf-8")
        (self.run / "grid_1.bin").write_bytes(binary)
        (self.run / "grid_1.csv").write_text(
            (",".join(["2.0"] * 100) + "\n") * 100, encoding="utf-8")
        (self.run / "stdout_1.log").write_text(
            "benchmark: benchmark_v1\nrays: 500000000\nseed: 123456789\n"
            "photon_export: false\nexport_path: none\nBenchmark completed.\n"
            "rays_traced: 500000000\nchunk_count: 50000\nchunk_size: 10000\n"
            "benchmark_pass: true\n"
            "flux_grid_hash_matches: true, flux_grid_hash_pass: true\n",
            encoding="utf-8")
        self.config = {
            "benchmark": "benchmark_v1", "rays": RAYS, "seed": SEED,
            "target_side_id": 1,
            "target_bounds": {"x_min": -2, "x_max": 2, "y_min": -2, "y_max": 2},
            "target_grid": {"width": 100, "height": 100},
            "photon_export": False,
            "scene_file": str(self.frozen / "benchmark_heliostat_field_v1.tnhpp"),
            "reference_file": str(self.frozen / "benchmark_reference_500M_v2.json"),
            "output_file": str(self.run / "result_1.json"),
            "flux_grid_binary_output_file": str(self.run / "grid_1.bin"),
            "flux_grid_output_file": str(self.run / "grid_1.csv"),
        }
        self.result = {
            "schema_version": 1, "benchmark": "benchmark_v1",
            "scene_file": self.config["scene_file"],
            "rays": RAYS, "seed": SEED, "target_side_id": 1,
            "target_bounds": self.config["target_bounds"],
            "target_grid": self.config["target_grid"],
            "chunk_count": 50000, "chunk_size": 10000,
            "worker_count": 8, "elapsed_seconds": 800.0,
            "rays_per_second": 625000.0,
            "total_power_mw": 32.0,
            "minimum_flux_mw_m2": 2.0,
            "average_flux_mw_m2": 2.0,
            "maximum_flux_mw_m2": 2.0,
            "flux_grid_sha256": digest,
            "flux_grid_output_file": self.config["flux_grid_output_file"],
            "flux_grid_binary_output_file": self.config["flux_grid_binary_output_file"],
            "reference_flux_grid_binary_file": str(
                self.frozen / "benchmark_reference_flux_grid_500M_v2.bin"),
            "total_power_error_percent": 0.0,
            "maximum_flux_error_percent": 0.0,
            "benchmark_pass": True,
            "total_power_pass": True,
            "maximum_flux_pass": True,
            "flux_grid_hash_matches": True,
            "flux_grid_hash_pass": True,
            "maximum_flux_grid_absolute_error_mw_m2": 0.0,
            "maximum_flux_grid_relative_error_percent": 0.0,
            "rms_flux_grid_error_mw_m2": 0.0,
        }
        self.save()

    def save(self):
        (self.run / "config_1.json").write_text(json.dumps(self.config), encoding="utf-8")
        (self.run / "result_1.json").write_text(json.dumps(self.result), encoding="utf-8")

    def audit(self):
        return audit_run(self.run, self.frozen, self.reference)

    def test_synthetic_fixture_accepted_by_auditor(self):
        self.assertEqual(self.audit()["ray_count"], RAYS)

    def test_wrong_ray_count_rejected(self):
        self.result["rays"] = 499999999
        self.save()
        with self.assertRaisesRegex(AuditError, "rays"):
            self.audit()

    def test_non_boolean_pass_flag_rejected(self):
        self.result["benchmark_pass"] = "true"
        self.save()
        with self.assertRaisesRegex(AuditError, "benchmark_pass"):
            self.audit()

    def test_mismatching_grid_bytes_rejected(self):
        path = self.run / "grid_1.bin"
        raw = bytearray(path.read_bytes())
        raw[-1] ^= 1
        path.write_bytes(raw)
        with self.assertRaisesRegex(AuditError, "CSV and binary|SHA-256"):
            self.audit()

    def test_missing_native_completion_log_rejected(self):
        (self.run / "stdout_1.log").write_text("Benchmark completed.\n", encoding="utf-8")
        with self.assertRaisesRegex(AuditError, "Native benchmark stdout log"):
            self.audit()

    def test_false_native_hash_flag_rejected_even_if_json_passes(self):
        log = self.run / "stdout_1.log"
        log.write_text(log.read_text(encoding="utf-8").replace(
            "flux_grid_hash_pass: true", "flux_grid_hash_pass: false"),
            encoding="utf-8")
        with self.assertRaisesRegex(AuditError, "combined successful flux-grid hash flags"):
            self.audit()

    def test_missing_combined_hash_line_rejected(self):
        log = self.run / "stdout_1.log"
        log.write_text(log.read_text(encoding="utf-8").replace(
            "flux_grid_hash_matches: true, flux_grid_hash_pass: true",
            "flux_grid_hash_matches: true"),
            encoding="utf-8")
        with self.assertRaisesRegex(AuditError, "combined successful flux-grid hash flags"):
            self.audit()

    def test_mismatching_csv_rejected(self):
        path = self.run / "grid_1.csv"
        path.write_text(path.read_text().replace("2.0", "3.0", 1), encoding="utf-8")
        with self.assertRaisesRegex(AuditError, "CSV and binary"):
            self.audit()

    def test_replaced_reference_path_rejected(self):
        self.config["reference_file"] = str(self.run / "result_1.json")
        self.save()
        with self.assertRaisesRegex(AuditError, "config.reference_file"):
            self.audit()

    def test_wrong_power_rejected_despite_true_flags(self):
        self.result["total_power_mw"] = 33.0
        self.save()
        with self.assertRaisesRegex(AuditError, "total_power_mw"):
            self.audit()

    def test_missing_output_rejected(self):
        (self.run / "grid_1.bin").unlink()
        with self.assertRaises(FileNotFoundError):
            self.audit()

    def test_wrong_chunk_size_rejected(self):
        self.result["chunk_size"] = 20000
        self.save()
        with self.assertRaisesRegex(AuditError, "chunk_size"):
            self.audit()

    def test_worker_count_ignored_for_scientific_identity(self):
        self.result["worker_count"] = 1
        self.save()
        self.assertEqual(self.audit()["ray_count"], RAYS)


if __name__ == "__main__":
    unittest.main()
