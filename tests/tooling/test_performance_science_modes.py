#!/usr/bin/env python3
"""Regression tests for strict and physics-change performance-harness science gates."""

import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / 'scripts' / 'performance' / 'compare_working_tree.py'
spec = importlib.util.spec_from_file_location('compare_working_tree', SCRIPT)
performance = importlib.util.module_from_spec(spec)
spec.loader.exec_module(performance)


def run(version, index, *, power=10.0, grid='baseline-grid'):
    return {
        'version': version, 'index': index,
        'scientific': {
            'total_power_mw': power, 'minimum_flux_mw_m2': 0.0,
            'average_flux_mw_m2': power / 10, 'maximum_flux_mw_m2': power / 2,
            'flux_grid_sha256': grid,
        },
    }


class ScienceComparisonTests(unittest.TestCase):
    def setUp(self):
        self.identical = [run('baseline', 1), run('candidate', 2),
                          run('candidate', 3), run('baseline', 4)]
        self.changed = [run('baseline', 1), run('candidate', 2, power=11.0, grid='new-grid'),
                        run('candidate', 3, power=11.0, grid='new-grid'), run('baseline', 4)]

    def test_default_requires_cross_version_equality(self):
        self.assertTrue(performance.compare_science(self.identical)['cross_version_matches'])
        with self.assertRaisesRegex(RuntimeError, 'Scientific mismatch'):
            performance.compare_science(self.changed)

    def test_physics_change_mode_preserves_cross_version_differences(self):
        result = performance.compare_science(self.changed, allow_cross_version_differences=True)
        self.assertTrue(result['within_version_matches'])
        self.assertFalse(result['cross_version_matches'])
        self.assertTrue(any('flux_grid_sha256' in x for x in result['cross_version_mismatches']))
        self.assertEqual(result['baseline']['total_power_mw'], 10.0)
        self.assertEqual(result['candidate']['total_power_mw'], 11.0)

    def test_physics_change_mode_rejects_candidate_nondeterminism(self):
        entries = self.changed[:]
        entries[2] = run('candidate', 3, power=12.0, grid='new-grid')
        with self.assertRaisesRegex(RuntimeError, 'Within-version scientific reproducibility FAILED'):
            performance.compare_science(entries, allow_cross_version_differences=True)

    def test_physics_change_mode_rejects_baseline_nondeterminism(self):
        entries = self.changed[:]
        entries[3] = run('baseline', 4, power=10.5)
        with self.assertRaisesRegex(RuntimeError, 'Within-version scientific reproducibility FAILED'):
            performance.compare_science(entries, allow_cross_version_differences=True)

    def test_warmups_are_subject_to_reproducibility(self):
        entries = self.changed[:]
        entries[0] = dict(entries[0], warmup=True)
        entries.insert(1, dict(run('baseline', 5, power=9.0), warmup=False))
        with self.assertRaisesRegex(RuntimeError, 'Within-version scientific reproducibility FAILED'):
            performance.compare_science(entries, allow_cross_version_differences=True)

    def test_missing_version_fails_closed(self):
        with self.assertRaisesRegex(ValueError, 'both baseline and candidate'):
            performance.compare_science([run('baseline', 1)], allow_cross_version_differences=True)

    def test_argument_parsing_accepts_mode_and_rationale(self):
        args = performance.parse_args([
            '--baseline-exe', 'a.exe', '--candidate-exe', 'b.exe',
            '--baseline-scene', 'a.tnhpp', '--candidate-scene', 'b.tnhpp',
            '--allow-scientific-differences',
            '--scientific-change-rationale', 'documented change to material physics',
            '--output-dir', 'fresh-folder',
        ])
        self.assertTrue(args.allow_scientific_differences)
        self.assertEqual(args.scientific_change_rationale, 'documented change to material physics')


if __name__ == '__main__':
    unittest.main()
