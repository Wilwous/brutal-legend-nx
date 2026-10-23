#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Cross-check reviewed identities and declarations against the pinned source.

This proves source consistency, not the actual Xbox contract or ABI conformance.
"""
import hashlib,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def audit(source=None):
 rows=json.loads((ROOT/'tests/image-runtime/import-contracts.json').read_text())
 if len(rows)!=33 or len({r['name'] for r in rows})!=33:raise RuntimeError('matrix size/duplicate')
 catalog=(ROOT/'tests/tls-import/catalog.h').read_text();policy=(ROOT/'runtime/import_admission.h').read_text()
 records=[];admitted=0;pointer=0
 for row in rows:
  identity=row['export_identity'];table=source/identity['table'] if source else None;text=table.read_text() if table else ''
  module='xam' if identity['library']=='xam.xex' else 'xboxkrnl'
  entries=re.findall(r'XE_EXPORT\(\s*'+module+r',\s*(0x[0-9a-fA-F]+),\s*'+re.escape(row['name'])+r',\s*(\w+)\)',text)
  if source and (len(entries)!=1 or int(entries[0][0],16)!=identity['ordinal'] or entries[0][1]!=identity['type']):raise RuntimeError('export drift '+row['name'])
  if '{"'+identity['library']+'",'+hex(identity['ordinal'])+'}, // '+row['name'] not in catalog:raise RuntimeError('fixture identity drift '+row['name'])
  evidence=row['signature_evidence'];src=source/evidence['source'] if source else None;body=src.read_text() if src else '';signature=row['sdk_signature']
  if source and signature.get('declaration') and signature['declaration'] not in ' '.join(body.split()):raise RuntimeError('declaration drift '+row['name'])
  if evidence['sdk_revision']!='f5337cdc947ff6d4c4196737e2c807a48f2a1fc2':raise RuntimeError('unreviewed revision')
  for key in ['arguments','result','register_stack','ownership','errors','outputs','thread_context','status','evidence','admission']:
   if not row.get(key):raise RuntimeError('missing contract '+row['name'])
  token='"'+row['name']+'",'+hex(identity['ordinal'])+',Signature::'+row['admission']['signature']
  if row['admission']['policy']=='ADMIT_SCOPED':
   admitted+=1
   if token not in policy:raise RuntimeError('missing scoped policy '+row['name'])
  elif row['admission']['policy']=='ADMIT_POINTER_SCOPED':
   pointer+=1
   if token not in policy or not row.get('pointer_contract'):raise RuntimeError('missing pointer policy '+row['name'])
  elif '"'+row['name']+'",' in policy:raise RuntimeError('denied import in policy')
  records.append({'name':row['name'],'source_sha256':hashlib.sha256(src.read_bytes()).hexdigest() if src else None,'policy':row['admission']['policy']})
 if admitted!=6 or pointer!=3 or len(re.findall(r'\{"(?:xboxkrnl.exe|xam.xex)",',policy))!=9:raise RuntimeError('policy size drift')
 return {'matrix_entries':33,'admitted_scoped':6,'admitted_pointer_scoped':3,'denied':24,'xbox_contract_certified':False,'records':records}

if __name__=='__main__':
 import argparse
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--source',type=Path);ap.add_argument('--matrix-only',action='store_true');a=ap.parse_args()
 if not a.source and not a.matrix_only:ap.error('select --source or --matrix-only')
 result=audit(a.source);print(json.dumps({k:v for k,v in result.items() if k!='records'},indent=2))
