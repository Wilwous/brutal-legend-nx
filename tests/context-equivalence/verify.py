#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Second implementation: check actual synthetic snapshots without generator data."""
import argparse,hashlib,itertools,json,struct
from pathlib import Path
from model import normalize_stack

def verify(path):
    raw=path.read_bytes()
    if len(raw)>1024*1024 or raw[:8]!=b'CEQ1\0\0\0\x18':raise ValueError('snapshot header/budget')
    at=8;rows={};byte_checks=0
    for _ in range(24):
        if at+40>len(raw):raise ValueError('truncated record')
        mode,seed,pages,base,sp,pcr,tls,result,cell,size=struct.unpack_from('>10I',raw,at);at+=40
        if (mode,seed,pages) in rows or mode not in range(4) or seed not in (0,1) or pages not in (0,1,3) or size!=16384 or at+size>len(raw):raise ValueError('invalid record identity/extent')
        data=raw[at:at+size];at+=size
        assert base==0x10001000+pages*4096 and sp==base+0x3f00
        assert pcr==base+16384 and tls==pcr+4096
        values=(0x1021324354657687^seed,0x10004f10,0xfedcba9876543210^seed,0x01030507090b0d0f)
        tls_value=0x55667788 if seed else 0x11223344
        expected=bytearray(b'\xa5'*16384)
        # Hand-authored byte oracle; no fixture.py, decoded words, generated C++
        # or C++ normalizer/table imported. Python serialization differs as well.
        for offset,width,value in [(-40,8,values[0]),(-32,8,values[1]),(-24,8,values[2]),(-16,8,values[3]),(-8,4,tls_value if mode==1 else 0x8201fff0),(-496,4,sp)]:
            expected[sp-base+offset:sp-base+offset+width]=value.to_bytes(width,'big')
        assert data==expected;byte_checks+=size
        assert result==(int(sp==0x10004f00) if mode==2 else tls_value if mode==1 else 9)
        assert cell==(sp if mode==3 else 0xdeadbeef)
        rows[(mode,seed,pages)]={'normalized':normalize_stack(data,base,sp),'raw':data,'result':result,'cell':cell,'sp':sp}
    if at!=len(raw):raise ValueError('unexpected snapshot trailer')
    equal=bad=raw_diff=nonrefuting_fixed=0;witnesses=[]
    for mode,seed in itertools.product(range(4),range(2)):
        for a,b in itertools.combinations((0,1,3),2):
            left,right=rows[mode,seed,a],rows[mode,seed,b]
            assert left['raw']!=right['raw'];raw_diff+=1
            same=all(left[k]==right[k] for k in ('normalized','result','cell'))
            if mode<2:assert same;equal+=1
            elif mode==3 or a==0:
                assert not same;bad+=1;witnesses.append({'mode':mode,'seed':seed,'stack_pointers':[left['sp'],right['sp']],'result':[left['result'],right['result']],'persistent':[left['cell'],right['cell']]})
            else:assert same;nonrefuting_fixed+=1
    assert equal==12 and bad==10 and raw_diff==24 and nonrefuting_fixed==2
    return {'records':24,'independent_oracle_bytes':byte_checks,'equivalent_pairs':equal,'counterexample_pairs':bad,'raw_stack_differences':raw_diff,'nonrefuting_fixed_pairs':nonrefuting_fixed,'witnesses':witnesses,'snapshot_sha256':hashlib.sha256(raw).hexdigest(),'game_input':False}

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('snapshot',type=Path);args=ap.parse_args();print(json.dumps(verify(args.snapshot)))
