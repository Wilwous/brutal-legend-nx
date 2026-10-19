#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Compare a pinned Canary software helper with the synthetic SDK subject.

Software evidence only. Neither result is a hardware oracle.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

PIN='2e61856a7901842d494ddef4f0bc09f1fd246533d3ae56d205b179d9d5be9a0e'
ROOT=Path(__file__).resolve().parents[2]
VALUES=[0,0x80000000,0x3f800000,0x3f801001,0x33800000,0x477fe000,
        0x477fe001,0x47800000,0x47ffe000,0x48000000,0x7f800000,0xff800000,0x7fc00000]

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--canary-math',type=Path,required=True)
    ap.add_argument('--sdk-subject',type=Path,required=True)
    ap.add_argument('--workdir',type=Path,required=True)
    args=ap.parse_args();work=args.workdir.resolve()
    if work==ROOT or ROOT in work.parents:ap.error('Outputs must remain outside the checkout')
    raw=args.canary_math.read_bytes()
    if hashlib.sha256(raw).hexdigest()!=PIN:ap.error('Canary source hash mismatch')
    source=raw.decode();start=source.index('inline uint16_t float_to_xenos_half');a=source.index('{',start);b=a+1;depth=1
    while depth:
        if source[b]=='{':depth+=1
        if source[b]=='}':depth-=1
        b+=1
    work.mkdir(parents=True,exist_ok=True)
    body='#include <algorithm>\n#include <cstdint>\n#include <cstring>\n#include <iostream>\n'+source[start:b]+'''
int main(){uint32_t bits;while(std::cin>>std::hex>>bits){float value;std::memcpy(&value,&bits,4);
std::cout<<std::hex<<float_to_xenos_half(value,false,true)<<'\\n';}}
'''
    (work/'canary_subject.cpp').write_text(body)
    subprocess.run(['clang++','-std=c++20','-O0',str(work/'canary_subject.cpp'),'-o',str(work/'canary-subject')],check=True,timeout=30)
    data=''.join(f'{x:08x}\n' for x in VALUES)
    r=subprocess.run([str(work/'canary-subject')],input=data,text=True,capture_output=True,check=True,timeout=10)
    canary=[int(l,16) for l in r.stdout.splitlines()]
    data=''.join('0 '+' '.join([f'{x:08x}']*4+['00000000']*4)+'\n' for x in VALUES)
    r=subprocess.run([str(args.sdk_subject.resolve())],input=data,text=True,capture_output=True,check=True,timeout=10)
    sdk=[int(l.split()[3],16)&0xffff for l in r.stdout.splitlines()]
    assert len(sdk)==len(canary)==len(VALUES)
    rows=[{'binary32':f'{x:08x}','canary':f'{c:04x}','sdk':f'{s:04x}'} for x,c,s in zip(VALUES,canary,sdk)]
    # Characterization of two distinct implementations, not Xbox expectations.
    assert canary[7]==0x7c00 and sdk[7]==0x7fff
    assert canary[10]==sdk[10]==0x7fff
    assert canary[4]==0 and sdk[4]==1
    assert canary[3]!=sdk[3]
    result={'classification':'INDEPENDENT EMULATOR EVIDENCE; VERIFIED HARDWARE BEHAVIOR unavailable','rows':rows}
    (work/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
