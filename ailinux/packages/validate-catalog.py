#!/usr/bin/env python3
"""Validate the Windows package capability matrix and block misleading claims."""
import json
from pathlib import Path
j=json.loads(Path(__file__).with_name('catalog.json').read_text())
assert j['schema']==1
p=j['policy']
assert p['require_sha256'] and p['require_snapshot']
assert not p['allow_msix_appx'] and not p['allow_system_wine_change']
entries={r['id']:r['status'] for r in j['capabilities']}
assert entries['winget']=='unsupported'
assert entries['msix-appx']=='unsupported'
assert entries['windows-store']=='unsupported'
assert entries['windows-msi']=='supported-with-verification'
assert entries['powershell-modules']=='linux-control-plane'
print('WINDOWS_PACKAGE_CAPABILITY_MATRIX_OK',len(entries))
