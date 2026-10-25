#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Read-only bounded instrumentation checks. No guest execution/admission API.

Detailed products (including addresses, policies and compiler IR) must stay private.
An offline PASS never grants a live module, stack owner, lease or title startup.
"""
import copy,json,re,struct
import bounded_cfg as cfg

def layout(base,size,sections,references=()):
 if not base or not size or base+size>1<<32 or not sections:raise ValueError('image owner')
 ordered=sorted(sections,key=lambda s:s['address']);end=base
 for s in ordered:
  a,n=s['address'],s['size']
  if n<=0 or a<base or a+n>base+size:raise ValueError('out-of-owner section')
  if a<end:raise ValueError('overlapping section')
  end=a+n
 for address,width,access in references:
  matches=[s for s in sections if s['address']<=address and address+width<=s['address']+s['size']]
  if width<=0 or address<base or address+width>base+size or len(matches)!=1:raise ValueError('reference ownership')
  flags=matches[0]['flags'];required=0x40000000|(0x80000000 if access=='write' else 0x20000000 if access=='code' else 0)
  if access not in ['read','write','code'] or flags&required!=required:raise ValueError('reference flags')
 return True

def identities(authority,observed):
 if observed!=authority:raise ValueError('input/code/owner identity mismatch')
 return True

def context(expected,actual):
 # Original fixture snapshot checks only. Neither snapshot is title provenance.
 if expected!=actual or not expected.get('stack_owner') or not expected.get('image_lease'):raise ValueError('context/owner mismatch')
 return True

def llvm_policy(ir,expected):
 payload=r'((?:\\[0-9A-Fa-f]{2}|[^"\\])*)'
 line=next((x for x in ir.splitlines() if x.startswith('@entry_policy =')), '')
 m=re.search(r'constant \[(\d+) x i8\] c"'+payload+'"',line)
 if not m:
  # Clang ASan wraps globals with a declared redzone. Validate its exact shape;
  # never disable instrumentation or accept a different policy payload.
  wrapped=re.search(r'constant \{ \[(\d+) x i8\], \[(\d+) x i8\] \} \{ \[(\d+) x i8\] c"'+payload+r'", \[(\d+) x i8\] zeroinitializer \}',line)
  if not wrapped or wrapped[1]!=wrapped[3] or wrapped[2]!=wrapped[5] or not 0<int(wrapped[2])<=65536:raise ValueError('missing/malformed compiled policy constant')
  m=(None,wrapped[1],wrapped[4])
 text=m[2];raw=bytearray();i=0
 while i<len(text):
  if text[i]=='\\':raw.append(int(text[i+1:i+3],16));i+=3
  else:raw.append(ord(text[i]));i+=1
 if len(raw)!=int(m[1]) or bytes(raw)!=cfg.canonical(expected):raise ValueError('compiled policy mismatch')
 return True

def policy_source(manifest):
 raw=cfg.canonical(manifest)
 return '// Original data-only policy serialization, never a guest function.\nextern "C" {\n[[gnu::used]] extern const unsigned char entry_policy['+str(len(raw))+'] = {'+','.join(str(v) for v in raw)+'};\n}\n'

def audit(authority,candidate,metadata,bodies,function_map,hook_table,macros):
 cfg.validate(candidate,authority);proof=cfg.stop_proof(authority);p=authority['policy'];raw=bytes(metadata['code']);origin=metadata['code_base']
 if cfg.sha(raw)!=authority['bindings']['image_code_sha256']:raise ValueError('instruction/image code identity')
 points=[s['pc'] for s in p['sites']];compiled=[(int(a),int(b)) for a,b in re.findall(r'BoundedCfg::point\((\d+),(\d+)\)',hook_table)]
 if compiled!=[(s['index'],s['pc']) for s in p['sites']]:raise ValueError('compiled/generated site table mismatch')
 observed=[];stores=[];call_anchors=0
 for r in p['functions']:
  rows=cfg.decode_range(raw,r['start'],r['end'],origin);body=bodies[r['start']]
  comments=list(re.finditer(r'^\t// .+$',body,re.M))
  if len(comments)<len(rows):raise ValueError('missing instruction')
  complete=len(comments)==len(rows)
  if not complete and next(s for s in p['sites'] if s['pc']==rows[-1]['pc'])['action']!='exit':raise ValueError('unbounded partial owner')
  checked=body[:comments[len(rows)].start()] if not complete else body # Read-only view; emitted source is never rewritten.
  agreement=cfg.reconcile(rows,checked,complete=True,function_map=function_map);call_anchors+=agreement['explicit_link_anchors'];observed+=cfg.reconcile_hooks(rows,checked,points)
  if re.search(r'base\s*\+|TranslateVirtual|reinterpret_cast|memcpy|ppc_(?:trap|setjmp|longjmp)|REX_(?:RAW|LOAD|MM_)',checked):raise ValueError('unrecognized/raw memory or exceptional path')
  if re.search(r'[*&]|->|\[|\]',checked):raise ValueError('explicit pointer/alias construct outside scalar grammar')
  without_calls=re.sub(r'\b\w+\(ctx,\s*base\)', '', checked)
  if re.search(r'\bbase\b',without_calls):raise ValueError('unrecognized base/alias path')
  allowed_calls={'REX_FUNC_PROLOGUE','REX_STORE_U32','REX_STORE_U64','REX_CALL_FUNC'}|set(function_map)|{'brutal_cfg_'+format(pc,'X') for pc in points}
  if set(re.findall(r'\b([A-Za-z_]\w*)\s*\(',checked))-allowed_calls:raise ValueError('unrecognized native call/construct')
  expected32=sum(bool(x['memory']) and x['memory']['write'] and x['memory']['width']==4 for x in rows if x['pc']!=rows[-1]['pc'] or p['sites'][points.index(x['pc'])]['action']=='execute')
  expected64=sum(bool(x['memory']) and x['memory']['write'] and x['memory']['width']==8 for x in rows if x['pc']!=rows[-1]['pc'] or p['sites'][points.index(x['pc'])]['action']=='execute')
  # Boundary words in these fixtures are calls, not stores. Refuse any unknown memory form.
  tokens=re.findall(r'\b(REX_STORE_\w+)\(',checked)
  if tokens.count('REX_STORE_U32')!=expected32 or tokens.count('REX_STORE_U64')!=expected64 or len(tokens)!=expected32+expected64:raise ValueError('store census')
  stores += [x['pc'] for x in rows if x['memory']]
 if sorted(observed)!=points:raise ValueError('site omission/duplicate')
 for name in ['REX_STORE_U32','REX_STORE_U64']:
  definition=next((s for s in macros.splitlines() if s.startswith('#define '+name+'(')), '')
  if 'brutal::instrumentation::store<' not in definition or 'base +' in definition:raise ValueError('unchecked expansion')
 return dict(static_sites=len(points),emitted_sites=len(observed),store_sites=len(stores),normal_edges=len(p['edges']),call_anchors=call_anchors,proof=proof,runtime_observed=False)

def artifacts(directory):
 from pathlib import Path
 d=Path(directory);metadata=json.loads((d/'input/cfg.json').read_text());manifest=json.loads((d/'input/region.json').read_text());generated=d/'input/generated';source=''.join(f.read_text() for f in sorted(generated.glob('fixture_recomp.*.cpp')));definitions=list(re.finditer(r'DEFINE_REX_FUNC\((\w+)\)\s*\{',source));bodies={}
 for i,m in enumerate(definitions):
  pc=metadata['entry'] if m[1]=='xstart' else int(m[1].removeprefix('sub_'),16);bodies[pc]=source[m.end():definitions[i+1].start() if i+1<len(definitions) else len(source)]
 mappings={}
 for f in generated.glob('*.cpp'):
  for address,name in re.findall(r'\{\s*(0x[0-9A-Fa-f]+),\s*(\w+)\s*\}',f.read_text()):mappings[name]=int(address,16)
 return manifest,metadata,bodies,mappings,(d/'cfg-data.h').read_text(),(d/'macros.txt').read_text()
