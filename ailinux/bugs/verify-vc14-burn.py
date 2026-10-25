#!/usr/bin/env python3
"""Verify Microsoft Burn execution and both minimum/additional MSI results."""
import argparse
from pathlib import Path
import re
import sys
p=argparse.ArgumentParser()
p.add_argument('logs', nargs='+', type=Path)
a=p.parse_args()
if not a.logs: sys.exit(2)
failed=False
for path in a.logs:
    raw=path.read_text(encoding='utf-8',errors='replace')
    arch='x64' if 'amd64' in path.name else 'x86' if 'x86' in path.name else 'unknown'
    checks={
        'minimum': bool(re.search(r'Applied execute package: vcRuntimeMinimum_'+arch+r', result: 0x0',raw)),
        'additional': bool(re.search(r'Applied execute package: vcRuntimeAdditional_'+arch+r', result: 0x0',raw)),
        'apply_complete': bool(re.search(r'Apply complete, result: 0x0',raw)),
        'exit_zero': bool(re.search(r'(?:Shutting down, exit code:|Exit code:) 0x0',raw)),
    }
    good=all(checks.values()) and 'Failed to load theme controls' not in raw
    print(path.name, 'PASS' if good else 'FAIL', checks)
    failed|=not good
sys.exit(1 if failed else 0)
