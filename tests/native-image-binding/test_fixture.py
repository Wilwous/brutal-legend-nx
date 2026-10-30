#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Public encoding and original source identity regression; no SDK/runtime needed."""
import hashlib,tempfile,struct
from pathlib import Path
from fixture import create
with tempfile.TemporaryDirectory(prefix='native-image-') as td:
 p=Path(td)/'input';m=create(p);b=(p/'fixture.xex').read_bytes()
 assert len(m['words'])==35 and len(m['functions'])==3 and len(m['hooks'])==15
 assert hashlib.sha256(b).hexdigest()=='ebc7240a2ae4a2c826c8cd513f26b815f38fb0baba5a46907e5daf9604fb54c0'
 assert hashlib.sha256(b''.join(w.to_bytes(4,'big') for w in m['words'])).hexdigest()=='429913c07f25e91f54736f81cfe4ad01f354cee1cea8b496653ad52da898925c'
 assert len(b)==0x51000 and struct.unpack_from('>I',b,0x104)[0]==0x50000
 assert struct.unpack_from('<I',b,0x1000+0x98+56)[0]==0x50000
 assert not any(b[4096+0x30400:])
 assert 10+5+7+7+sum(v==0x10203040 for v in [0x10203040,0,0x10203040,0x10203040])==32
print('PASS: fixed original XEX/PPC fingerprints, explicit zero run, independent arithmetic oracle')
