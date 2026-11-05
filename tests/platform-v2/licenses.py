#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Reproduce pinned dependency attribution text without extracting/executing sources."""
import argparse
import hashlib
import io
import re
import tarfile
import time
import urllib.request
from pathlib import Path
SOURCES=[('mesa.archive','https://archive.mesa3d.org/older-versions/20.x/mesa-20.1.0-rc3.tar.xz','c90b75ea34302ebde9b81b87c5642fa864c40fe9c4ad34ce0793170c1413168d'),('glad.archive','https://github.com/devkitPro/switch-glad/releases/download/v0.1.27/switch-glad-src-0.1.27.tar.bz2','b59cf95c542b19947e0595daef7da9b5b541f4e8ce3cb6995ea3470298b7ccb9'),('drm.archive','https://github.com/devkitPro/libdrm_nouveau/archive/v1.0.1/libdrm_nouveau-src-1.0.1.tar.gz','b069bf8029abb120294bd2dd988dc4d6ce2be3f1016c88c26c73c87b6f312136'),('glad-upstream-LICENSE.txt','https://raw.githubusercontent.com/Dav1dde/glad/v0.1.27/LICENSE','a2a88399d4ef1cab60abcd6ddbd301b4535d10cf051e940edf35d8665749fb45')]
EXPECTED='988671793105429c99afaa0568a20f06a40430c55dde62850f88d6116909909e'
def create(cache):
 cache=Path(cache);cache.mkdir(parents=True,exist_ok=True)
 for name,url,digest in SOURCES:
  path=cache/name
  if not path.exists():
   start=time.monotonic();data=bytearray()
   with urllib.request.urlopen(url,timeout=10) as stream:
    while True:
     block=stream.read(1024*1024)
     if not block:break
     data.extend(block)
     if len(data)>40*1024*1024 or time.monotonic()-start>30:raise ValueError('bounded source download refused')
   if hashlib.sha256(data).hexdigest()!=digest:raise ValueError('source fingerprint mismatch')
   path.write_bytes(data)
  if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest()!=digest:raise ValueError('cached source mismatch')
 b=(cache/'glad-upstream-LICENSE.txt').read_bytes();url=SOURCES[-1][1]
 parts=['Open-source dependency attribution inventory for the local T105 synthetic NRO.\nOriginal project GPL-3.0-only; upstream terms apply to their components.\nFull SDK/SIMDe/libnx notices also accompany this package.\n\nGLAD upstream v0.1.27 LICENSE: '+url+'\n'+b.decode()]
 for name in ['mesa','drm']:
  seen=set()
  with tarfile.open(fileobj=io.BytesIO((cache/(name+'.archive')).read_bytes())) as archive:
   for item in archive.getmembers():
    if not item.isfile() or item.size>1024*1024:continue
    path=Path(item.name)
    if path.name.lower().startswith(('license','copying','copyright')) or item.name.endswith('docs/license.html'):text=archive.extractfile(item).read().decode(errors='replace')
    elif path.suffix in ['.c','.h','.cpp','.cc','.hpp']:
     text=archive.extractfile(item).read().decode(errors='replace');block=re.match(r'\s*((?:(?:/\*.*?\*/|//[^\n]*\n)\s*)+)',text,re.S)
     if not block:continue
     text=block[1]
     if not re.search(r'copyright|permission|licen[cs]e',text,re.I):continue
    else:continue
    digest=hashlib.sha256(text.encode()).hexdigest()
    if digest not in seen:seen.add(digest);parts.append('\n\nSOURCE '+item.name+'\n'+text)
 result='\n'.join(parts).encode();assert hashlib.sha256(result).hexdigest()==EXPECTED
 return result
if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('--cache',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);a=parser.parse_args()
 if a.output.exists():parser.error('new external output required')
 a.output.write_bytes(create(a.cache));print(EXPECTED)
