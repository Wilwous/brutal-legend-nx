#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Strict local build-receipt validation. This is integrity, not signed attestation."""
import hashlib,json,os,re,stat,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
APP='brutal-legend-nx'
NRO='brutal-legend-nx.nro'
RELATIVE=f'switch/{APP}/{NRO}'
BUILD_SOURCES=['tools/switch/sanitize_elf.py']+['runtime/'+n for n in ['checked_memory.h','checked_tls.h','cooperative_stop.h']]+['tests/switch-horizon/'+n for n in ['probe.h','probe.cpp','main.cpp','result_ui.h','ui.cpp','host.cpp','Makefile','toolchain.json','run.py','inspect_nro.py']]
def sha(data):return hashlib.sha256(data).hexdigest()
def regular_bytes(path,limit):
    fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW)
    try:
        info=os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size>limit:raise ValueError('unexpected file type/size')
        data=b''
        while True:
            chunk=os.read(fd,65536)
            if not chunk:break
            data+=chunk
            if len(data)>limit:raise ValueError('file grew beyond allowed size')
        return data
    finally:os.close(fd)
def source_hashes():return {n:sha((ROOT/n).read_bytes()) for n in BUILD_SOURCES}
def packaging_hashes():return {n:sha((ROOT/n).read_bytes()) for n in ['tools/switch/'+f for f in ['common.py','package.py','icon.py','requirements.txt','NOTICE.txt']]+['LICENSE']}
def check_nro(data):
    if len(data)<128 or data[16:20]!=b'NRO0':raise ValueError('not an NRO')
    size=struct.unpack_from('<I',data,24)[0]
    if size<128 or size+56>len(data) or data[size:size+4]!=b'ASET':raise ValueError('missing NRO assets')
    if b'/home/' in data or b'/Users/' in data:raise ValueError('NRO contains an absolute build/user path')
def validate_package(directory):
    directory=Path(os.path.abspath(directory))
    for p in [directory,directory/'switch',directory/'switch'/APP]:
        if p.is_symlink() or not p.is_dir():raise ValueError('package directory missing or symlinked')
    app=directory/'switch'/APP
    manifest=json.loads(regular_bytes(app/'build-info.json',65536))
    lock=json.loads((ROOT/'tests/switch-horizon/toolchain.json').read_text())
    if manifest.get('schema')!=1 or manifest.get('nro_path')!=RELATIVE:raise ValueError('unexpected package path/schema')
    if not re.fullmatch('[0-9a-f]{40}',manifest.get('source_commit','')):raise ValueError('missing source revision')
    if manifest.get('image')!=lock['image'] or manifest.get('build_sources_sha256')!=source_hashes():raise ValueError('build provenance does not match this public source/toolchain')
    if manifest.get('packaging_sources_sha256')!=packaging_hashes():raise ValueError('packaging provenance does not match this source')
    if manifest.get('host_passed')!=81 or manifest.get('host_failed')!=0 or manifest.get('clean_builds')!=2:raise ValueError('missing passing build/test receipt')
    data=regular_bytes(app/NRO,16*1024*1024);check_nro(data)
    if sha(data)!=manifest.get('nro_sha256'):raise ValueError('NRO checksum mismatch')
    return data,manifest
