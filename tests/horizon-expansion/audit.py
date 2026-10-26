# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Independent instruction-index to emitted-hook/memory census; no generator metadata trust."""
import re

def audit(meta,cpp):
 blocks=re.split(r'^\s*// .*$',cpp,flags=re.M)[1:]
 if len(blocks)!=len(meta['words']):raise ValueError('instruction blocks differ')
 memory=[];branches=[];calls=[];returns=[];seen=[]
 for i,(w,block) in enumerate(zip(meta['words'],blocks)):
  pc=meta['code_base']+i*4;op=w>>26
  expected={32:'REX_LOAD_U32',36:'REX_STORE_U32',37:'REX_STORE_U32',58:'REX_LOAD_U64',62:'REX_STORE_U64'}.get(op)
  operations=re.findall(r'\b(REX_(?:LOAD|STORE)_\w+)\(',block)
  if operations!=([expected] if expected else []):raise ValueError('memory site mismatch '+hex(pc))
  if expected:memory.append(pc)
  hook=[int(s,16) for s in re.findall(r'^\s*horizon_point_([0-9A-F]+)\(\);',block,re.M)]
  transfer=op in (16,18) or w in (0x4e800020,0x4e800421)
  if hook!=([pc] if expected or transfer else []):raise ValueError('hook identity mismatch '+hex(pc))
  seen+=hook
  if op in (16,18):
   bits,mask=(16,0xfffc) if op==16 else (26,0x3fffffc);delta=w&mask
   if delta&(1<<(bits-1)):delta-=1<<bits
   target=pc+delta
   if op==18 and w&1:calls.append((pc,target))
   else:
    if not(meta['code_base']<=target<meta['code_base']+4*len(meta['words'])) or target&3:raise ValueError('branch target outside instruction image')
    branches.append((pc,target))
  elif w==0x4e800421:calls.append((pc,None))
  elif w==0x4e800020:returns.append(pc)
 if seen!=meta['hooks'] or memory!=meta['memory_sites']:raise ValueError('manifest is not independent census equivalent')
 backward=[pc for pc,target in branches if target<=pc]
 if backward!=meta['backedges']:raise ValueError('backedge drift')
 return {'instructions':len(blocks),'memory_static':len(memory),'memory_emitted':len(memory),'hook_static':len(seen),'hook_emitted':len(seen),'backedges_static':len(backward),'backedges_emitted':len(backward),'calls':len(calls),'returns':len(returns),'branch_edges':len(branches)}
