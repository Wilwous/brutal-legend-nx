# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Extract only the exact pinned open-source fill leaf into private output."""
import hashlib,importlib.util,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def extract(sdk,out):
 lock=json.loads((Path(__file__).with_name('fill-lock.json')).read_text());source=(sdk/lock['file']).read_bytes()
 if hashlib.sha256(source).hexdigest()!=lock['source_sha256']:raise ValueError('fill SDK source mismatch')
 s=importlib.util.spec_from_file_location('leaf_extract',ROOT/'tests/horizon-runtime-ii/sdk_query.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m);body=m.function(source.decode(),'void RtlFillMemoryUlong_entry(')
 if hashlib.sha256(body.encode()).hexdigest()!=lock['function_sha256']:raise ValueError('fill body mismatch')
 if 'XE_EXPORT(xboxkrnl, 0x00000126, RtlFillMemoryUlong,' not in (sdk/'src/kernel/xboxkrnl/export_table.inc').read_text():raise ValueError('fill ordinal mismatch')
 (out/'sdk-fill.inc').write_text(body)
