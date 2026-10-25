#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Neutralize two pinned newlib diagnostic source prefixes, never code or layout.

Prebuilt newlib retains __FILE__ paths outside consumer -ffile-prefix-map.
Preserve all offsets and the relative filename suffix, including suffix merging.
Unexpected paths/layouts refuse; the unmodified linked ELF stays private.
"""
import hashlib,json,re,struct,sys
from pathlib import Path
PATTERN=re.compile(rb'/'+rb'home'+rb'/[^/\x00]+/projects/devkitpro/pacman-packages/devkita64/newlib/src/newlib-4\.6\.0\.20260123/(newlib/libc/stdlib/(?:dtoa|mprec)\.c)\x00')
def sanitize(raw):
    if len(raw)<64 or raw[:6]!=b'\x7fELF\x02\x01' or struct.unpack_from('<HH',raw,16)!=(3,183):raise ValueError('expected AArch64 PIE')
    phoff=struct.unpack_from('<Q',raw,32)[0];size,count=struct.unpack_from('<HH',raw,54)
    if size!=56 or not 0<count<=32 or phoff+size*count>len(raw):raise ValueError('invalid program headers')
    readonly=[];loaded=[]
    for i in range(count):
        kind,flags,off,va,pa,length,mem,align=struct.unpack_from('<IIQQQQQQ',raw,phoff+i*size)
        if off+length>len(raw):raise ValueError('segment out of bounds')
        if kind==1:loaded.append((off,off+length))
        if kind==1 and flags==4:readonly.append((off,off+length))
    matches=[m for m in PATTERN.finditer(raw) if any(lo<=m.start() and m.end()<=hi for lo,hi in readonly)]
    if len(matches)!=2 or {m[1] for m in matches}!={b'newlib/libc/stdlib/dtoa.c',b'newlib/libc/stdlib/mprec.c'}:raise ValueError('pinned newlib diagnostic census changed')
    result=bytearray(raw);changed=[]
    for m in matches:
        start=m.start();end=m.start(1)
        if not any(lo<=start and m.end()<=hi for lo,hi in readonly):raise ValueError('diagnostic is not wholly in read-only non-executable memory')
        label=b'[newlib]'
        if end-start<len(label):raise ValueError('prefix too short')
        result[start:end]=label+b' '*(end-start-len(label))
        changed.append((start,end))
    result=bytes(result)
    if len(result)!=len(raw):raise ValueError('ELF size changed')
    if any(a!=b and not any(lo<=i<hi for lo,hi in changed) for i,(a,b) in enumerate(zip(raw,result))):raise ValueError('unexpected ELF mutation')
    if any(b'/home/' in result[lo:hi] or b'/Users/' in result[lo:hi] for lo,hi in loaded):raise ValueError('unexpected remaining loaded private path')
    return result,{'diagnostic_prefixes':2,'executable_bytes_changed':0,'layout_changed':False,'linked_sha256':hashlib.sha256(raw).hexdigest(),'sanitized_sha256':hashlib.sha256(result).hexdigest()}
def main():
    if len(sys.argv)!=2:raise ValueError('expected one freshly linked ELF')
    p=Path(sys.argv[1]);raw=p.read_bytes();data,record=sanitize(raw)
    p.with_suffix('.linked.elf').write_bytes(raw)
    p.write_bytes(data);p.with_suffix('.diagnostics.json').write_text(json.dumps(record,indent=2)+'\n')
if __name__=='__main__':main()
