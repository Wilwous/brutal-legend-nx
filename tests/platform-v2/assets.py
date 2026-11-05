# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Deterministic original cube, checker and two integer triangle-wave PCM assets."""
import hashlib
import json
import struct
from pathlib import Path

def content():
    faces=[((-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)),((1,-1,-1),(-1,-1,-1),(-1,1,-1),(1,1,-1)),((-1,-1,-1),(-1,-1,1),(-1,1,1),(-1,1,-1)),((1,-1,1),(1,-1,-1),(1,1,-1),(1,1,1)),((-1,1,1),(1,1,1),(1,1,-1),(-1,1,-1)),((-1,-1,-1),(1,-1,-1),(1,-1,1),(-1,-1,1))]
    vertices=b''.join(struct.pack('>5f',x*.5,y*.5,z*.5,*uv) for face in faces for (x,y,z),uv in zip(face,[(0,0),(1,0),(1,1),(0,1)]))
    indices=b''.join(struct.pack('>H',face*4+i) for face in range(6) for i in [0,1,2,0,2,3])
    header=b'SCN2'+struct.pack('>7I',1,24,36,32,32+len(vertices),len(vertices)+len(indices)+32,4)
    texture=bytes(c for y in range(8) for x in range(8) for c in ((240,164,32,255) if (x//2+y//2)%2==0 else (32,80,208,255)))
    def tone(period,amplitude):
        out=bytearray()
        for n in range(4800):
            phase=n%period;value=(4*phase if phase<period//2 else 4*(period-phase))-period
            sample=value*amplitude//period;out+=struct.pack('<hh',sample,sample)
        return bytes(out)
    return {'scene.bin':header+vertices+indices,'checker.rgba':texture,'ambient.pcm':tone(240,1000),'action.pcm':tone(80,3000)}

def create(directory):
    directory=Path(directory);directory.mkdir(parents=True,exist_ok=False);items=content();metadata=[]
    for name,data in items.items():
        (directory/name).write_bytes(data);metadata.append({'path':name,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
    header='#pragma once\n#include "runtime/horizon/platform_resources.h"\ninline std::array<brutal::horizon::resources::Manifest,4> v2_manifest(){using namespace brutal::horizon::resources;return {{'
    header+=','.join('{"'+v['path']+'",'+str(v['bytes'])+',Digest{'+','.join(str(b) for b in bytes.fromhex(v['sha256']))+'}}' for v in metadata)
    header+='}};}\n';(directory.parent/'assets_contract.h').write_text(header)
    (directory.parent/'assets-manifest.json').write_text(json.dumps(metadata,indent=2)+'\n');return metadata
