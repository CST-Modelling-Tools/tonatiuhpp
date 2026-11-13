"""Bounded, non-network CLI updater regression checks against the real executable.

The tests deliberately never invoke an actual update check or updater.
Real IFW updater transactions are release-acceptance tests, not unit tests.
"""
import argparse
from pathlib import Path
import subprocess
import unittest

parser = argparse.ArgumentParser(add_help=False)
parser.add_argument('--cli', type=Path, required=True)
args, rest = parser.parse_known_args()


class UpdateCommandTests(unittest.TestCase):
    def call(self, *arguments):
        return subprocess.run([str(args.cli.resolve()), *arguments], text=True,
                              encoding='utf-8', errors='replace', capture_output=True,
                              timeout=20, check=False)

    def test_help_advertises_both_commands(self):
        p = self.call('--help')
        self.assertEqual(p.returncode, 0, p.stderr)
        self.assertIn('tonatiuhpp-cli check-updates', p.stdout)
        self.assertIn('tonatiuhpp-cli update', p.stdout)

    def test_checks_reject_extra_arguments_without_network(self):
        for command in ('check-updates', 'update'):
            with self.subTest(command=command):
                p = self.call(command, '--unexpected')
                self.assertEqual(p.returncode, 2, (p.stdout, p.stderr))
                self.assertIn('takes no arguments', p.stderr)

    def test_never_conflates_scene_tracker_flag_with_software_updates(self):
        p = self.call('--help')
        self.assertIn('--update-trackers', p.stdout)
        self.assertIn('check-updates', p.stdout)


if __name__ == '__main__':
    unittest.main(argv=['test_update_commands.py', *rest], verbosity=2)
