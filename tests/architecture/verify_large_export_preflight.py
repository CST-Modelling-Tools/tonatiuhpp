#!/usr/bin/env python3
"""Runtime preflight for File exports above 10M rays; executes ZERO large ray loops.

Direct CLI: missing scene is rejected only AFTER the large ray count parses.
Scripted tn.Run: invalid File destination is rejected after large-count validation
and simulation preparation, but BEFORE a ray-tracing session can start.
All files are created only in a freshly allocated build-tree temporary folder.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def execute(argv: list[str], *, cwd: Path, timeout: int = 75) -> subprocess.CompletedProcess[str]:
    return subprocess.run(argv, cwd=cwd, capture_output=True,
                          text=True, errors='replace', timeout=timeout, check=False)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cli', required=True, type=Path)
    parser.add_argument('--scene', required=True, type=Path)
    parser.add_argument('--build-test-dir', required=True, type=Path)
    args = parser.parse_args()

    cli = args.cli.resolve()
    scene = args.scene.resolve()
    workroot = args.build_test_dir.resolve()
    require(cli.is_file(), f'CLI executable not found: {cli}')
    require(scene.is_file(), f'Scene fixture missing: {scene}')
    require(workroot.is_dir(), f'Build test directory does not exist: {workroot}')

    with tempfile.TemporaryDirectory(prefix='tonatiuhpp-large-export-policy-', dir=workroot) as td:
        work = Path(td)
        empty_dir = work / 'empty-output'
        empty_dir.mkdir()
        nonexistent_scene = work / 'scene-does-not-exist.tnhpp'
        require(not nonexistent_scene.exists(), 'Refusing test: nonexistent scene exists')

        # Crucial: neither test can enter a ray loop because scene loading fails
        # before the file exporter is created or the trace session starts.
        for rays in (10_000_001, 500_000_000):
            result = execute([str(cli), 'trace-scene', str(nonexistent_scene),
                              '--rays', str(rays), '--seed', '0',
                              '--photon-export-dir', str(empty_dir)], cwd=work)
            output = result.stdout + '\n' + result.stderr
            require(result.returncode == 1 and 'Scene load failed:' in output,
                    f'{rays} rays did not pass CLI argument validation: '
                    f'code={result.returncode}\n{output}')
            require(not any(empty_dir.iterdir()),
                    'Unexpected exporter artifacts after missing-scene failure')

        # A scripted File run may pass 10M but must still reject an invalid
        # destination before session.start(), with no export or photon files.
        missing_output = work / 'export-destination-does-not-exist'
        require(not missing_output.exists(), 'Refusing test: expected absent output')
        script = work / 'large-preflight.tnhpps'
        src = f'''tn.loadScene({json.dumps(scene.as_posix())});
            tn.SetRaysNumber(10000001);
            tn.SetRaysGrid(200, 200);
            tn.SetExportPhotonMapType('File');
            tn.SetExportTypeParameterValue('ExportDirectory', {json.dumps(missing_output.as_posix())});
            tn.SetExportTypeParameterValue('ExportFile', 'PhotonMap');
            tn.SetExportTypeParameterValue('FileSize', '-1');
            var expectedRejected = false;
            try {{
                tn.Run(0);
            }} catch (error) {{
                if (String(error).indexOf('File export requires an existing absolute output directory') < 0)
                    throw error;
                expectedRejected = true;
            }}
            if (!expectedRejected)
                throw new Error('Large scripted File run unexpectedly bypassed directory validation');
            print('LARGE_EXPORT_SCRIPT_PREFLIGHT_PASSED');
'''
        script.write_text(src, encoding='utf-8')
        result = execute([str(cli), 'run-script', str(script)], cwd=work)
        output = result.stdout + '\n' + result.stderr
        require(result.returncode == 0 and 'LARGE_EXPORT_SCRIPT_PREFLIGHT_PASSED' in output,
                f'Large scripted Run did not reach File directory validation: '
                f'code={result.returncode}\n{output}')
        require(not missing_output.exists(), 'Script unexpectedly created invalid output directory')

    print('PASS: 10,000,001 and 500,000,000 CLI ray arguments accepted; '
          'large scripted File Run reached safe destination rejection. '
          'NO high-ray-count simulation was executed.')


if __name__ == '__main__':
    try:
        main()
    except Exception as exc:
        print(f'LARGE EXPORT POLICY PREFLIGHT FAILED: {exc}', file=sys.stderr)
        sys.exit(1)
