# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Extract only exact pinned open-source SDK declarations and query leaf."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def function(text,signature):
 start=text.index(signature);end=text.index('{',start)+1;depth=1
 while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]+'\n'
def pieces(sdk):
 flags=(sdk/'include/rex/system/xmemory.h').read_text();flags=flags[flags.index('enum MemoryAllocationFlag'):flags.index('// Describes a single page')]
 types=(sdk/'include/rex/system/xtypes.h').read_text();statuses='\n'.join(line for line in types.splitlines() if line.startswith(('typedef uint32_t X_STATUS;','#define X_STATUS_SUCCESS ','#define X_STATUS_INVALID_PARAMETER ')))+'\n'
 enums=types[types.index('enum X_MEM :'):types.index('// Sockets/networking')]
 text=(sdk/'src/kernel/xboxkrnl/xboxkrnl_memory.cpp').read_text();struct=text[text.index('struct X_MEMORY_BASIC_INFORMATION {'):text.index('\nu32 NtQueryVirtualMemory_entry')]
 definitions='namespace rex {\n'+statuses+enums+'namespace memory {\n'+flags+'}\n}\n'
 leaf=function(text,'uint32_t ToXdkProtectFlags(')+struct+'\n'+function(text,'u32 NtQueryVirtualMemory_entry(')
 return definitions,leaf

def extract(sdk,out):
 lock=json.loads((ROOT/'tests/horizon-runtime-ii/query-lock.json').read_text())
 for name,digest in lock['files'].items():
  if hashlib.sha256((sdk/name).read_bytes()).hexdigest()!=digest:raise ValueError('SDK query evidence changed: '+name)
 definitions,leaf=pieces(sdk)
 if hashlib.sha256(definitions.encode()).hexdigest()!=lock['types_sha256'] or hashlib.sha256(leaf.encode()).hexdigest()!=lock['leaf_sha256']:raise ValueError('SDK query extraction mismatch')
 (out/'sdk-query-types.inc').write_text(definitions);(out/'sdk-query.inc').write_text(leaf)
