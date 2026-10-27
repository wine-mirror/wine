#!/usr/bin/env python3
"""Static validation for Wine program IDs and supported/unsupported status."""
import json
from pathlib import Path
p=Path(__file__).with_name('catalog.json')
doc=json.loads(p.read_text(encoding='utf-8'))
assert doc['schema']==1
apps=doc['apps']
assert len(apps)>=10
for key,value in apps.items():
    assert key and all(c.isalnum() or c=='-' for c in key),key
    assert value['kind'] in {'wine-builtin','unsupported-msix','staged-unverified'},key
    assert value['display'],key
    if value['kind']=='wine-builtin':
        assert value['exe'].endswith('.exe') and '/' not in value['exe'] and '\\' not in value['exe'],key
    if value['kind']=='unsupported-msix':
        assert 'exe' not in value,key
assert apps['wordpad']['exe']=='write.exe'
assert apps['powershell7']['kind']=='staged-unverified'
print('APP_CATALOG_OK',len(apps),'items')
