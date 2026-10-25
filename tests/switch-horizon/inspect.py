#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Inspect original build products only; this does not emulate Horizon."""
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

def inspect():
    elf=Path('probe.elf').read_bytes();nro=Path('probe.nro').read_bytes()
    assert elf[:6]==b'\x7fELF\x02\x01', 'expected ELF64 little endian'
    assert struct.unpack_from('<HH',elf,16)==(3,183), 'expected AArch64 PIE'
    assert nro[16:20]==b'NRO0', 'missing NRO header'
    size=struct.unpack_from('<I',nro,24)[0]
    assert size==len(nro), 'unexpected appended data or size mismatch'
    segments=[struct.unpack_from('<II',nro,off) for off in (32,40,48)]
    assert all(start%4096==0 and length%4096==0 and start+length<=size for start,length in segments)
    assert all(segments[i][0]+segments[i][1]<=segments[i+1][0] for i in (0,1))
    phoff=struct.unpack_from('<Q',elf,32)[0]
    phsize,phnum=struct.unpack_from('<HH',elf,54)
    loads=[];needed=[];file_loads=[]
    for i in range(phnum):
        kind,flags,offset,va,pa,filesize,memsize,align=struct.unpack_from('<IIQQQQQQ',elf,phoff+i*phsize)
        if kind==1:
            assert flags&3!=3, 'writable executable ELF mapping'
            loads.append({'flags':flags,'file_bytes':filesize,'memory_bytes':memsize,'alignment':align})
            file_loads.append((va,offset,filesize))
        if kind==2:
            for j in range(offset,offset+filesize,16):
                tag,value=struct.unpack_from('<qQ',elf,j)
                if tag==1:needed.append(value)
    assert loads and not needed, 'unexpected dynamic library dependency'
    prefix='/opt/devkitpro/devkitA64/bin/aarch64-none-elf-'
    def command(tool,args,out):
        text=subprocess.check_output([prefix+tool,*args],text=True,timeout=10)
        Path(out).write_text(text);return text
    symbols=command('nm',['-C','probe.elf'],'symbols.txt')
    sized=command('nm',['-S','probe.elf'],'symbol-sizes.txt')
    heap=re.search(r'^([0-9a-f]+)\s+([0-9a-f]+)\s+D\s+__nx_heap_size$',sized,re.M)
    assert heap and int(heap[2],16)==8, 'libnx heap override must be size_t'
    heap_va=int(heap[1],16)
    locations=[offset+heap_va-va for va,offset,count in file_loads if va<=heap_va and heap_va+8<=va+count]
    assert len(locations)==1 and struct.unpack_from('<Q',elf,locations[0])[0]==16*1024*1024
    undefined=command('nm',['-u','probe.elf'],'undefined.txt')
    assert not any(re.search(r'\bU\b',line) for line in undefined.splitlines()), 'unresolved strong symbol'
    forbidden=['mmap','mprotect','shm_open','dlopen','pthread_cancel','svcMapPhysicalMemoryUnsafe',
               'svcCreateCodeMemory','svcControlCodeMemory','socket','connect']
    for name in forbidden:
        assert not re.search(r'\b'+name+r'$',symbols,re.M), 'unexpected runtime dependency: '+name
    for name in ['main','threadCreate','threadStart','threadWaitForExit','threadClose']:
        assert re.search(r'\b'+name+r'$',symbols,re.M), 'missing static runtime symbol: '+name
    disassembly=command('objdump',['-d','probe.elf'],'disassembly.txt')
    svcs=sorted({int(n,16) for n in re.findall(r'\bsvc\s+#?0x([0-9a-f]+)',disassembly)})
    command('readelf',['-h','-l','-d','probe.elf'],'elf-headers.txt')
    result={'format_checks':'PASS','architecture':'AArch64 ELF64 PIE / NRO0',
            'elf_bytes':len(elf),'nro_bytes':len(nro),'nro_segments':segments,
            'nro_bss_bytes':struct.unpack_from('<I',nro,56)[0],
            'load_segments':loads,'dynamic_needed_count':len(needed),
            'unresolved_strong_symbols':0,'svc_ids':[hex(n) for n in svcs],
            'privileged_svc_wrapper_present':['0x52'] if 0x52 in svcs else [],
            'heap_request_bytes':16*1024*1024,'main_stack_runtime_bytes':'UNKNOWN (loader-owned)',
            'native_worker_stack_request_bytes':64*1024,
            'runtime':'UNKNOWN','hardware_svc_permission':'UNKNOWN',
            'nro_sha256':hashlib.sha256(nro).hexdigest()}
    Path('inspection.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
if __name__=='__main__':inspect()
