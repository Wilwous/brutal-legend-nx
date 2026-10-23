# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Fail-closed census of original PPC and actual emitted hooks/backedges."""
import re,struct

def audit(meta,body):
 words=struct.unpack('>'+str(len(meta['code'])//4)+'I',bytes(meta['code']));origin=meta['code_base'];expected={};unsupported=[]
 for address,length in meta['extents'].items():
  start=int(address)
  for pc in range(start,start+4*length,4):
   w=words[(pc-origin)//4];op=w>>26
   if op in [16,18] and not w&1:
    bits=16 if op==16 else 26;delta=w&(0xfffc if op==16 else 0x3fffffc);delta-=1<<bits if delta&(1<<(bits-1)) else 0
    target=delta if w&2 else pc+delta
    if not start<=target<start+4*length:raise ValueError("unscoped direct branch")
    if target<=pc:expected[pc]=target
   if op==19 and w not in [0x4e800020,0x4e800421]:unsupported.append(pc)
 if unsupported:raise ValueError('unsupported branch path')
 seen=[];hooks={};backward={};pc=None;count=0
 for line in body.splitlines():
  f=re.match(r'DEFINE_REX_FUNC\(([^)]+)\)',line)
  if f:
   pc=meta['entry'] if f[1]=='xstart' else int(f[1].removeprefix('sub_'),16);seen.append(pc);continue
  if line.startswith('\t// '):
   if pc is None:raise ValueError('instruction outside function')
   current=pc;pc+=4;count+=1
  h=re.match(r'\tbrutal_point_([0-9A-F]+)\(\);$',line)
  if h:
   at=int(h[1],16)
   if at!=current or at in hooks:raise ValueError('misplaced/duplicate safe point')
   hooks[at]=True
  g=re.search(r'goto loc_([0-9A-F]+);',line)
  if g and int(g[1],16)<=current:
   target=int(g[1],16);backward[current]=target
   if meta['instrumented'] and current not in hooks:raise ValueError('uncovered emitted backedge')
 if set(seen)!=set(meta['functions']) or len(seen)!=len(meta['functions']) or count!=meta['instructions']:raise ValueError('generated instruction/function drift')
 if backward!=expected or set(expected)!=set(meta['backedges']):raise ValueError('PPC/emitted backedge drift')
 wanted=set(map(int,meta['sites'])) if meta['instrumented'] else set()
 if set(hooks)!=wanted:raise ValueError('safe-point placement drift')
 for forbidden in ['REX_RAW_ADDR(','REX_PARTIAL_VECTOR_GUARD(','REX_UNIMPLEMENTED(','REX_FATAL(','base +','ppc_trap(','ppc_setjmp(','ppc_longjmp(','GuestPtr']:
  if forbidden in body:raise ValueError('unsupported generated path '+forbidden)
 direct=re.findall(r'REX_CALL_FUNC\(([^)]+)\)',body)
 wanted_calls={f'sub_{origin:X}',f'sub_{origin+184:X}','__imp__RtlCompareMemoryUlong','__imp__RtlInitializeCriticalSection'}
 if meta['instructions']>1 and (len(direct)!=5 or set(direct)!=wanted_calls or body.count('REX_CALL_INDIRECT_FUNC(')!=1):raise ValueError('unreviewed generated call graph')
 memory=len(re.findall(r'\bREX_(?:LOAD|STORE)_(?:U\d+|VECTOR)\(',body))
 if 'memory_sites' in meta and memory!=len(meta['memory_sites']):raise ValueError('memory census drift')
 return {'functions':len(seen),'instructions':count,'backedges':len(expected),'instrumented_backedges':len(expected) if meta['instrumented'] else 0,'unsupported_backedges':0,'safe_point_sites':len(hooks),'memory_operations':len(re.findall(r'\bREX_(?:LOAD|STORE)_(?:U\d+|VECTOR)\(',body))}
