#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
import struct,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/switch'))
from sanitize_elf import sanitize
def fixture():
    raw=bytearray(1024);raw[:6]=b'\x7fELF\x02\x01';struct.pack_into('<HH',raw,16,3,183)
    struct.pack_into('<Q',raw,32,64);struct.pack_into('<HH',raw,54,56,3)
    for i,(flags,off,length) in enumerate([(5,256,64),(4,320,512),(6,832,192)]):struct.pack_into('<IIQQQQQQ',raw,64+i*56,1,flags,off,off,off,length,length,4096)
    prefix=b'/'+b'home'+b'/builder/projects/devkitpro/pacman-packages/devkita64/newlib/src/newlib-4.6.0.20260123/'
    pos=320
    for name in [b'dtoa',b'mprec']:
        text=prefix+b'newlib/libc/stdlib/'+name+b'.c\0';raw[pos:pos+len(text)]=text;pos+=len(text)
    return bytes(raw)
class Sanitizer(unittest.TestCase):
    def test_exact_readonly_change(self):
        raw=fixture();data,row=sanitize(raw);self.assertEqual(row['diagnostic_prefixes'],2)
        self.assertEqual(len(raw),len(data));self.assertEqual(raw[:320],data[:320]);self.assertEqual(raw[832:],data[832:])
    def test_suffix_offsets_preserved(self):
        raw=fixture();data,_=sanitize(raw)
        for name in [b'newlib/libc/stdlib/dtoa.c\0',b'newlib/libc/stdlib/mprec.c\0']:self.assertEqual(raw.index(name),data.index(name))
    def test_paths_removed(self):self.assertNotIn(b'/'+b'home'+b'/',sanitize(fixture())[0])
    def test_wrong_format(self):
        with self.assertRaises(ValueError):sanitize(b'not ELF')
    def test_wrong_section(self):
        raw=bytearray(fixture());struct.pack_into('<I',raw,64+56+4,5)
        with self.assertRaises(ValueError):sanitize(bytes(raw))
    def test_version_drift(self):
        with self.assertRaises(ValueError):sanitize(fixture().replace(b'20260123',b'20260124'))
    def test_missing_diagnostic(self):
        with self.assertRaises(ValueError):sanitize(fixture().replace(b'mprec.c',b'other.c'))
    def test_unexpected_remaining_path(self):
        raw=bytearray(fixture());raw[900:908]=b'/'+b'Users'+b'/x'
        with self.assertRaises(ValueError):sanitize(bytes(raw))
if __name__=='__main__':unittest.main()
