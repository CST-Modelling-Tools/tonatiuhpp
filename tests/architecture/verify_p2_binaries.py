#!/usr/bin/env python3
"""Cross-platform P2 binary/entrypoint regression, no GUI event loop or rays."""
import argparse
from pathlib import Path
import subprocess
import sys


def fail(msg):
    raise SystemExit('P2 architecture check failed: ' + msg)


def main():
    parser=argparse.ArgumentParser()
    for name in ('gui','cli','kernel','libraries','presentation'):
        parser.add_argument('--'+name,required=True,type=Path)
    args=parser.parse_args()
    for name in ('gui','cli','kernel','libraries','presentation'):
        path=getattr(args,name)
        if not path.is_file():fail(f'{name} artifact not found: {path}')
        marker={'gui':'tonatiuhpp-gui','cli':'tonatiuhpp-cli',
                'kernel':'TonatiuhppKernel','libraries':'TonatiuhppLibraries',
                'presentation':'TonatiuhppGuiPresentation'}[name]
        if marker not in path.name:fail(f'{name}: unexpected binary name {path.name}')
    if args.gui.resolve()==args.cli.resolve():fail('GUI and CLI are same executable')
    # Important: this must return before constructing QApplication and SoQt,
    # so the regression runs in unprivileged headless Linux CI as well.
    gui=subprocess.run([str(args.gui),'--headless'],capture_output=True,text=True,
                       encoding='utf-8',errors='replace',timeout=15)
    if gui.returncode!=2 or 'tonatiuhpp-cli' not in (gui.stdout+gui.stderr):
        fail(f'GUI unexpectedly offers legacy headless dispatch: rc={gui.returncode}, err={gui.stderr[-200:]}')
    cli=subprocess.run([str(args.cli),'--help'],capture_output=True,text=True,
                       encoding='utf-8',errors='replace',timeout=15)
    if cli.returncode or 'tonatiuhpp-cli' not in cli.stdout:
        fail(f'CLI direct help failed: rc={cli.returncode}, output={cli.stdout[-200:]}')
    legacy=subprocess.run([str(args.cli),'--headless','--help'],capture_output=True,
                          text=True,encoding='utf-8',errors='replace',timeout=15)
    if legacy.returncode!=2:
        fail('CLI still accepts legacy --headless option')
    print('P2 architecture PASS: separate binaries/shared kernel, GUI rejects old mode, CLI direct commands')

if __name__=='__main__':main()
