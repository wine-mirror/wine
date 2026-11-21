#!/usr/bin/env python3
"""Summarize Wine VC++ Burn logs without declaring installation success."""
import argparse
from pathlib import Path
import re
import sys

parser = argparse.ArgumentParser()
parser.add_argument('log', nargs='+', type=Path)
a = parser.parse_args()
issues = 0
for path in a.log:
    content = path.read_text(errors='replace')
    print(f'LOG {path.name}')
    for pattern in (r'Error 0x[0-9a-fA-F]+: Failed to create window',
                    r'Error 0x[0-9a-fA-F]+: Failed to load theme controls',
                    r'Error 0x[0-9a-fA-F]+: Failed to create main window',
                    r'Shutting down, exit code: 0x[0-9a-fA-F]+'):
        values = re.findall(pattern, content)
        for item in values: print(' ', item)
        issues += bool(values and ('Error ' in values[0] or not values[-1].endswith('0x0')))
if issues:
    print('BURN_INSTALL_NOT_VERIFIED')
    sys.exit(1)
print('NO_KNOWN_BURN_ERRORS; further runtime verification required')
