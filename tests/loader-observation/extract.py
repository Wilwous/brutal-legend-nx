#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Extract byte-identical public Xenia methods, never title data or a patched loader."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def extract(reference,out,defined_offset=False):
 lock=json.loads((ROOT/'tests/loader-observation/reference-lock.json').read_text())
 for row in lock['files']:
  if hashlib.sha256((reference/row['path']).read_bytes()).hexdigest()!=row['sha256']:raise ValueError('pinned reference changed: '+row['path'])
 s=(reference/'src/xenia/cpu/xex_module.cc').read_text();chunks=[];receipt=[]
 def retain(start,end):
  body=s[start:end];receipt.append({'first_line':s.count('\n',0,start)+1,'last_line':s.count('\n',0,end)+1,'sha256':hashlib.sha256(body.encode()).hexdigest()});return body
 for name in ['ReadImageUncompressed','ReadImageBasicCompressed','ReadPEHeaders']:
  start=s.index('int XexModule::'+name+'(');end=s.index('\n}\n',start)+2;chunks.append(retain(start,end))
 start=s.index('  // Setup memory protection.',s.index('bool XexModule::LoadContinue()'))
 end=s.index('\n  return true;',start)
 chunks.append('void XexModule::ObserveProtection(){\n auto* heap=memory()->LookupHeap(base_address_);auto page_size=heap->page_size();auto* sec_header=xex_security_info();\n'+retain(start,end)+'\n}')
 # Wrapper/adapter is independent; these retained byte spans are never rewritten.
 text='\n\n'.join(chunks)+'\n'
 (out/'reference-methods-original.inc').write_text(text)
 if defined_offset:
  old='#define offsetof1(type, member) ((std::size_t) & (((type*)0)->member))'
  new='#define offsetof1(type, member) offsetof(type, member)'
  patch=(ROOT/'tests/loader-observation/defined-offset.patch').read_text()
  if text.count(old)!=1 or '-'+old not in patch or '+'+new not in patch:raise ValueError('offset-only patch mismatch')
  text=text.replace(old,new)
 (out/'reference-methods.inc').write_text(text);(out/'extraction.json').write_text(json.dumps({'revision':lock['revision'],'unmodified_spans':receipt,'defined_offset_variant':defined_offset,'full_xenia_load':False,'native_memory_adapter':True},indent=2)+'\n')
 return receipt
