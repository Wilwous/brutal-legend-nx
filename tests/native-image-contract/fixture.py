# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original NONE XEX fixture with an explicitly authored native zero-tail plan."""
from pathlib import Path
import importlib.util,struct,json
ROOT=Path(__file__).resolve().parents[2]
def create(destination):
 spec=importlib.util.spec_from_file_location('original',ROOT/'tests/xex-binding/fixture.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 p=Path(destination);m.create(p);f=p/'fixture.xex';b=bytearray(f.read_bytes())
 struct.pack_into('<I',b,0x1000+0x98+224+3*40+8,0x20000)
 struct.pack_into('<I',b,0x1000+0x98+56,0x50000);f.write_bytes(b)
 (p/'native-plan.json').write_text(json.dumps({'descriptor_extent':0x40000,'native_extent':0x50000,'authored_zero':[0x40000,0x50000],'source':'original synthetic author contract, not inferred Xbox policy'}))
 return f
