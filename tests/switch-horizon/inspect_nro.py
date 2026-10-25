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
    assert 128<=size<=len(nro), 'NRO image size mismatch'
    assert nro[size:size+4]==b'ASET', 'missing hbmenu metadata'
    assert struct.unpack_from('<I',nro,size+4)[0]==0, 'unknown asset header version'
    assets=[struct.unpack_from('<QQ',nro,size+off) for off in (8,24,40)]
    ranges=sorted((off,off+length) for off,length in assets if length)
    assert ranges and all(56<=lo<hi<=len(nro)-size for lo,hi in ranges)
    assert all(ranges[i][1]<=ranges[i+1][0] for i in range(len(ranges)-1))
    assert ranges[-1][1]==len(nro)-size, 'unexpected trailing data'
    icon_off,icon_size=assets[0];nacp_off,nacp_size=assets[1]
    assert nacp_size==0x4000 and assets[2][1]==0, 'unexpected NACP/RomFS'
    nacp=nro[size+nacp_off:size+nacp_off+nacp_size]
    title=nacp[:0x200].split(b'\0',1)[0].decode()
    author=nacp[0x200:0x300].split(b'\0',1)[0].decode()
    version=nacp[0x3060:0x3070].split(b'\0',1)[0].decode()
    assert (title,author,version)==('Brutal Legend NX Probe','Wilwous','0.1.0')
    if icon_size:
        icon=nro[size+icon_off:size+icon_off+icon_size]
        assert icon.startswith(b'\xff\xd8') and icon.endswith(b'\xff\xd9'), 'expected JPEG icon'
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
    result['hbmenu']={'title':title,'author':author,'version':version,'icon_bytes':icon_size,'romfs_bytes':0}
    Path('inspection.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
if __name__=='__main__':inspect()
