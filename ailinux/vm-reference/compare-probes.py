#!/usr/bin/env python3
"""Compare Win32 export availability on a Windows VM and local Wine."""
import argparse
import json
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('--windows',required=True)
p.add_argument('--wine',required=True)
a=p.parse_args()
w=json.loads(Path(a.windows).read_text())
v=json.loads(Path(a.wine).read_text())
if w.get('schema')!=1 or v.get('schema')!=1 or not isinstance(w.get('api'),dict) or not isinstance(v.get('api'),dict):
    raise ValueError("Invalid probe format")
rows=[{'export':k,'windows':w['api'].get(k),'wine':v['api'].get(k),
       'matches':w['api'].get(k)==v['api'].get(k)}
      for k in sorted(set(w['api'])|set(v['api']))]
print(json.dumps({'schema':1,'arch_match':w.get('arch_bits')==v.get('arch_bits'),
  'export_matches':sum(x['matches'] for x in rows),'total':len(rows),
  'note':'Export presence is not behavioral compatibility.',
  'results':rows},indent=2))
