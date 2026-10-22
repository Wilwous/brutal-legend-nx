#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Check source registration/provenance for the reviewed 33 import contracts.

This is a source census, not executable API conformance or an admission policy.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[2]
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--source',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 output=a.output.resolve()
 if output==ROOT or ROOT in output.parents:ap.error('private diagnostics required')
 rows=json.loads((ROOT/'tests/image-runtime/import-contracts.json').read_text())
 previous=(ROOT/'docs/research/guest-memory-entry-readiness.md').read_text().split('## Import and error contracts')[1].split('## ABI readiness')[0]
 expected=set(re.findall(r'^\| (\w+) \|',previous,re.M))-{'Import'}
 if len(rows)!=33 or {row['name'] for row in rows}!=expected:raise RuntimeError('entry dependency contract census drift')
 sources={p:p.read_text() for p in (a.source/'src/kernel').rglob('*.cpp')}
 records=[]
 for row in rows:
  if any(not row.get(key) for key in ['arguments','result','errors','outputs','ownership','thread_context','status','evidence']):raise RuntimeError('incomplete contract')
  found=[]
  for p,s in sources.items():
   if re.search(r'(?:REX_EXPORT\w*|REX_HOOK_RAW)\(\s*__imp__'+re.escape(row['name'])+r'\s*[,)]',s):
    found.append({'source':str(p.relative_to(a.source)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
  if len(found)!=1:raise RuntimeError('ambiguous/missing export '+row['name'])
  records.append({'name':row['name'],'status':row['status'],'registration':found[0]})
 output.parent.mkdir(parents=True,exist_ok=True)
 output.write_text(json.dumps({'reviewed_imports':33,'conformance_claim':False,'records':records},indent=2)+'\n')
 print('33 reviewed imports: unique source registrations verified; ABI conformance not inferred')
if __name__=='__main__':main()
