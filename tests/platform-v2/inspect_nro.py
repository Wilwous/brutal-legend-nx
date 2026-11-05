#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Static original NRO inspection; no Horizon execution or title data."""
import hashlib
import json
import re
import struct
import subprocess
from pathlib import Path
elf=Path('probe.elf').read_bytes();nro=Path('probe.nro').read_bytes()
assert elf[:6]==b'\x7fELF\x02\x01' and struct.unpack_from('<HH',elf,16)==(3,183)
assert nro[16:20]==b'NRO0';size=struct.unpack_from('<I',nro,24)[0];assert nro[size:size+4]==b'ASET'
assets=[struct.unpack_from('<QQ',nro,size+off) for off in (8,24,40)]
ranges=sorted((off,off+count) for off,count in assets);assert all(56<=lo<hi<=len(nro)-size for lo,hi in ranges)
assert all(ranges[i][1]<=ranges[i+1][0] for i in range(2)) and ranges[-1][1]==len(nro)-size
nacp=nro[size+assets[1][0]:size+sum(assets[1])];assert len(nacp)==0x4000
identity=tuple(nacp[start:end].split(b'\0',1)[0].decode() for start,end in [(0,0x200),(0x200,0x300),(0x3060,0x3070)])
assert identity==('Brutal NX - Synthetic 3D Demo V2','Wilwous','0.105.0')
romfs=nro[size+assets[2][0]:size+sum(assets[2])];header=struct.unpack_from('<10Q',romfs);assert header[0]==0x50
filemeta=romfs[header[7]:header[7]+header[8]];dirinfo=romfs[header[3]:header[3]+header[4]]
# Inspect every embedded RomFS file against original deterministic asset identities.
import sys
sys.path.insert(0,'/src/tests/platform-v2')
from assets import content
root=struct.unpack_from('<6I',dirinfo,0);assert root[2]!=0xffffffff and root[3]==0xffffffff
directory=struct.unpack_from('<6I',dirinfo,root[2]);assert dirinfo[root[2]+24:root[2]+24+directory[5]]==b'resources' and directory[1]==directory[2]==0xffffffff
expected=content();embedded={};cursor=directory[3];visited=set()
while cursor!=0xffffffff:
 assert cursor not in visited;visited.add(cursor);entry=struct.unpack_from('<IIQQII',filemeta,cursor)
 name=filemeta[cursor+32:cursor+32+entry[5]].decode();data=romfs[header[9]+entry[2]:header[9]+entry[2]+entry[3]]
 assert name in expected and data==expected[name] and name not in embedded;embedded[name]=hashlib.sha256(data).hexdigest();cursor=entry[1]
assert set(embedded)==set(expected)
content=b''.join(expected.values())
phoff=struct.unpack_from('<Q',elf,32)[0];phsize,phnum=struct.unpack_from('<HH',elf,54);loads=[];needed=[];load_bytes=[]
for i in range(phnum):
 kind,flags,offset,va,pa,filesize,memsize,alignment=struct.unpack_from('<IIQQQQQQ',elf,phoff+i*phsize)
 if kind==1:
  assert flags&3!=3;loads.append((flags,filesize,memsize));load_bytes.append((flags,offset,va,filesize,memsize))
 if kind==2:
  for off in range(offset,offset+filesize,16):
   tag,value=struct.unpack_from('<qQ',elf,off)
   if tag==1:needed.append(value)
assert loads and not needed
# elf2nro writes its header into the reserved text header, and pads each load
# to 4 KiB. Compare every other load byte, including actual executable code.
segments=[struct.unpack_from('<II',nro,off) for off in (32,40,48)]
assert len(load_bytes)==3 and [v[0] for v in load_bytes]==[5,4,6]
end=0
for index,((start,count),(flags,offset,va,filesize,memsize)) in enumerate(zip(segments,load_bytes)):
 assert start==end==va and start%4096==0 and count==(filesize+4095)&~4095
 assert start+count<=size and offset+filesize<=len(elf)
 skip=0x80 if index==0 else 0
 assert elf[offset:offset+min(16,skip)]==nro[start:start+min(16,skip)]
 assert elf[offset+skip:offset+filesize]==nro[start+skip:start+filesize]
 assert not any(nro[start+filesize:start+count])
 end=start+count
assert end==size
assert struct.unpack_from('<I',nro,56)[0]==((load_bytes[-1][4]+4095)&~4095)-segments[-1][1]
prefix='/opt/devkitpro/devkitA64/bin/aarch64-none-elf-'
def tool(name,args,log):
 out=subprocess.check_output([prefix+name,*args],text=True,timeout=10);Path(log).write_text(out);return out
symbols=tool('nm',['-C','probe.elf'],'symbols.txt');undefined=tool('nm',['-u','probe.elf'],'undefined.txt');assert not any(re.search(r'\bU\b',line) for line in undefined.splitlines())
# romfsInit/romfsExit are inline wrappers at the pinned libnx revision.
for name in ['main','eglInitialize','eglSwapBuffers','eglTerminate','audoutInitialize','audoutAppendAudioOutBuffer','romfsMountSelf','romfsUnmount']:
 assert re.search(r'\b'+name+r'$',symbols,re.M),name
for name in ['__imp__xstart','demo_functions']:
 assert re.search(r'\b'+name+r'$',symbols,re.M),name
for name in ['mmap','mprotect','shm_open','dlopen','socket','connect','svcMapPhysicalMemoryUnsafe','svcCreateCodeMemory','svcControlCodeMemory']:
 assert not re.search(r'\b'+name+r'$',symbols,re.M),name
assert not re.search(rb'/(?:Users|home)/[^\0\s/]+/',elf+nro)
tool('readelf',['-h','-l','-d','probe.elf'],'elf-headers.txt')
report={'architecture':'AArch64 ELF64 PIE / NRO0','identity':identity,'romfs_resource_bytes':len(content),'embedded_resources':embedded,'romfs_sha256':hashlib.sha256(romfs).hexdigest(),'elf_sha256':hashlib.sha256(elf).hexdigest(),'nro_sha256':hashlib.sha256(nro).hexdigest(),'nro_bytes':len(nro),'loads':loads,'NRO_load_bytes_match_ELF':True,'unresolved_strong':0,'DT_NEEDED':0,'W_and_X':False,'hardware_execution':'UNKNOWN'}
Path('inspection.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
