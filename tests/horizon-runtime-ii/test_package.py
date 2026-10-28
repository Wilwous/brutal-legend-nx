#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original package-integrity unit fixtures, not actual NRO execution evidence."""
import hashlib,importlib.util,json,struct,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
s=importlib.util.spec_from_file_location('integrated_packaging',Path(__file__).with_name('package.py'));p=importlib.util.module_from_spec(s);s.loader.exec_module(p)
class Integrity(unittest.TestCase):
 def setUp(self):
  t=tempfile.TemporaryDirectory();self.addCleanup(t.cleanup);self.root=Path(t.name);self.app=self.root/'switch'/p.APP;self.app.mkdir(parents=True)
  data=bytearray(184);data[16:20]=b'NRO0';struct.pack_into('<I',data,24,128);data[128:132]=b'ASET';self.payload=bytes(data);self.sha=hashlib.sha256(data).hexdigest();(self.app/(p.APP+'.nro')).write_bytes(data)
  for name in ['LICENSE.txt','THIRD_PARTY.txt','README.txt']:(self.app/name).write_text('original fixture')
  (self.app/'SHA256SUMS').write_text(self.sha+'  '+p.APP+'.nro\n')
  names={'runtime/checked_memory.h','runtime/horizon/xex.h','runtime/diagnostics.h','runtime/horizon/query.h'}|{f.relative_to(ROOT).as_posix() for f in (ROOT/'tests/horizon-runtime-ii').rglob('*') if f.is_file() and '__pycache__' not in str(f)}
  self.manifest={'application':'Brutal Legend NX - Integrated Runtime II','nro_path':p.RELATIVE,'source_commit':'0'*40,'nro_sha256':self.sha,'sources_sha256':{n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in names},'hardware_execution':'UNKNOWN','game_code_included':False,'host_failed':0,'host_passed':804,'expected_console_passed':269,'source_worktree_dirty':False,'build':{'deterministic_builds':2,'inspection':{'unresolved_strong_symbols':0,'nro_sha256':self.sha}}}
 def validate(self):
  (self.app/'build-info.json').write_text(json.dumps(self.manifest));return p.validate(self.root)
 def test_valid_integrity_fixture(self):self.assertEqual(self.validate()[0],self.payload)
 def test_zero_assertions(self):
  self.manifest['host_passed']=0
  with self.assertRaises(ValueError):self.validate()
 def test_inconsistent_console(self):
  self.manifest['expected_console_passed']=0
  with self.assertRaises(ValueError):self.validate()
 def test_wrong_app(self):
  self.manifest['nro_path']='switch/brutal-legend-nx-ppc/brutal-legend-nx-ppc.nro'
  with self.assertRaises(ValueError):self.validate()
 def test_hardware_claim(self):
  self.manifest['hardware_execution']='VERIFIED'
  with self.assertRaises(ValueError):self.validate()
 def test_missing_sources(self):
  del self.manifest['sources_sha256']['tests/horizon-runtime-ii/query.cpp']
  with self.assertRaises(ValueError):self.validate()
 def test_changed_sources(self):
  self.manifest['sources_sha256']['runtime/checked_memory.h']='0'*64
  with self.assertRaises(ValueError):self.validate()
 def test_payload_mismatch(self):
  self.manifest['nro_sha256']='0'*64
  with self.assertRaises(ValueError):self.validate()
 def test_inspection_mismatch(self):
  self.manifest['build']['inspection']['nro_sha256']='0'*64
  with self.assertRaises(ValueError):self.validate()
 def test_dirty_source(self):
  self.manifest['source_worktree_dirty']=True
  with self.assertRaises(ValueError):self.validate()
 def test_extra_artifact(self):
  (self.app/'extra.nro').write_bytes(b'foreign')
  with self.assertRaises(ValueError):self.validate()
 def test_checksum_file(self):
  (self.app/'SHA256SUMS').write_text('wrong')
  with self.assertRaises(ValueError):self.validate()
 def test_symlinked_payload(self):
  (self.app/(p.APP+'.nro')).unlink();(self.root/'outside').write_bytes(self.payload);(self.app/(p.APP+'.nro')).symlink_to(self.root/'outside')
  with self.assertRaises((ValueError,OSError)):self.validate()
if __name__=='__main__':unittest.main()
