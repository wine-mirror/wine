#!/usr/bin/env python3
"""WineHQ bug 60383: long quoted cmd /c command must not overflow."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

p=argparse.ArgumentParser()
p.add_argument("--wine",required=True)
p.add_argument("--prefix",required=True)
a=p.parse_args()
prefix=Path(a.prefix).expanduser().resolve()
if prefix==Path.home()/".wine" or not prefix.is_relative_to(Path.home()):
    sys.exit("Unsafe Wine prefix")
if not (prefix/"system.reg").exists():
    sys.exit("Prefix is not initialized")
wine=Path(a.wine).resolve(strict=True)
env=os.environ.copy()
env.update(WINEPREFIX=str(prefix),WINEDEBUG="-all")
for count in (256, 1024, 4096, 8192):
    # The targeted bug was a stack overflow in command lookup for a
    # quoted /c command longer than MAX_PATH, not arbitrary output volume.
    command='"' + 'z' * count + '.exe"'
    run=subprocess.run([str(wine),"cmd","/c",command],env=env,
                       capture_output=True,timeout=20)
    print(f"length={count} exit={run.returncode} stdout={len(run.stdout)} stderr={len(run.stderr)}")
    if run.returncode in (-11, -6, 139, 134):
        sys.exit("CRASH regression for WineHQ 60383")
print("PASS: no process crash in quoted cmd lookup")
