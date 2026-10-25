#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original encoding and mock-only deployment regression; never touches a device."""
import hashlib,importlib.util,json,os,sys,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/switch'))
import deploy as usb
from fixture import WORDS,write,BINARY_SHA256

class Fixture(unittest.TestCase):
    def test_words_independent_fields(self):
        load,add,store,ret=WORDS
        self.assertEqual((load>>26,(load>>21)&31,(load>>16)&31,load&65535),(32,3,4,0))
        self.assertEqual((add>>26,(add>>21)&31,(add>>16)&31,add&65535),(14,3,3,1))
        self.assertEqual((store>>26,(store>>21)&31,(store>>16)&31,store&65535),(36,3,4,0))
        self.assertEqual(ret,0x4e800020)
    def test_encoding_identity(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)/'input';write(p);self.assertEqual(hashlib.sha256((p/'leaf.bin').read_bytes()).hexdigest(),BINARY_SHA256)
    def test_patch_lock(self):
        lock=json.loads((ROOT/'tests/horizon-runtime/lock.json').read_text())
        self.assertEqual(hashlib.sha256((ROOT/'patches/rexglue-sdk/horizon-leaf.patch').read_bytes()).hexdigest(),lock['patch_sha256'])
    def test_missing_helpers_not_fallbacks(self):
        s=(ROOT/'tests/horizon-runtime/templates/test/ppc_config_h.inja').read_text()
        self.assertIn('LeafScope::load',s);self.assertIn('LeafScope::store',s)
        for name in ['REX_RAW_ADDR','REX_LOAD_U64','REX_CALL_INDIRECT_FUNC','REX_LOAD_VECTOR']:self.assertNotIn('#define '+name,s)

class Destination(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.root=Path(self.temp.name)
        for d in ['switch/brutal-legend-nx','bootloader','atmosphere']:(self.root/d).mkdir(parents=True,exist_ok=True)
        for n in ['bootloader/hekate_ipl.ini','atmosphere/package3','switch/brutal-legend-nx/brutal-legend-nx.nro']:(self.root/n).write_bytes(b'working original')
        self.app=self.root/'switch/brutal-legend-nx-ppc';self.nro=self.app/'brutal-legend-nx-ppc.nro'
    def tearDown(self):
        self.assertEqual((self.root/'switch/brutal-legend-nx/brutal-legend-nx.nro').read_bytes(),b'working original')
        for n in ['bootloader/hekate_ipl.ini','atmosphere/package3']:self.assertEqual((self.root/n).read_bytes(),b'working original')
    def deploy(self,payload=b'new original leaf',allow=False):
        fd=usb.directory(self.root)
        try:return usb.deploy_fd(fd,payload,lambda:None,application='brutal-legend-nx-ppc',allow_replace=allow)
        finally:os.close(fd)
    def test_separate_destination(self):
        self.assertEqual(self.deploy()['status'],'COPIED_VERIFIED');self.assertEqual(self.nro.read_bytes(),b'new original leaf')
    def test_unknown_destination_rejected(self):
        for name in ['../brutal-legend-nx','other','',None]:
            with self.assertRaises(usb.Refused):usb.deployment_names(name)
    def test_existing_different_refused(self):
        self.app.mkdir();self.nro.write_bytes(b'previous')
        with self.assertRaises(usb.Refused):self.deploy()
        self.assertEqual(self.nro.read_bytes(),b'previous');self.assertEqual(sorted(p.name for p in self.app.iterdir()),[self.nro.name])
    def test_explicit_replace_has_verified_backup(self):
        self.app.mkdir();self.nro.write_bytes(b'previous')
        row=self.deploy(allow=True);self.assertEqual((self.app/row['backup']).read_bytes(),b'previous')
        self.assertTrue(row['backup'].startswith('brutal-legend-nx-ppc.previous-'))
    def test_repeat_is_noop(self):
        self.deploy();self.assertEqual(self.deploy()['status'],'ALREADY_CURRENT_VERIFIED')
    def test_destination_symlink_refused(self):
        self.app.symlink_to(self.root/'switch/brutal-legend-nx',target_is_directory=True)
        with self.assertRaises(OSError):self.deploy()
    def test_destination_file_symlink_refused(self):
        self.app.mkdir();self.nro.symlink_to(self.root/'switch/brutal-legend-nx/brutal-legend-nx.nro')
        with self.assertRaises(OSError):self.deploy(allow=True)
    def test_flush_failure_retains_native(self):
        with patch.object(usb,'sync_file',side_effect=OSError('synthetic flush failure')):
            with self.assertRaises(OSError):self.deploy()
        self.assertFalse(self.nro.exists())
class Package(unittest.TestCase):
    def setUp(self):
        spec=importlib.util.spec_from_file_location('leaf_package',Path(__file__).with_name('package.py'))
        self.p=importlib.util.module_from_spec(spec);spec.loader.exec_module(self.p)
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.root=Path(self.temp.name)
        app=self.root/'switch'/self.p.APP;app.mkdir(parents=True);self.app=app
        import struct
        data=bytearray(184);data[16:20]=b'NRO0';struct.pack_into('<I',data,24,128);data[128:132]=b'ASET';self.data=bytes(data)
        (app/self.p.NRO).write_bytes(self.data)
        self.m={'schema':1,'application':'Brutal Legend NX - PPC Runtime Probe','nro_path':self.p.RELATIVE,
                'source_commit':'0'*40,'build_sources_sha256':self.p.runner.inputs(),'packaging_sources_sha256':self.p.packaging_hashes(),
                'image':json.loads((ROOT/'tests/switch-horizon/toolchain.json').read_text())['image'],
                'input_sha256':BINARY_SHA256,'generated_sha256':json.loads((ROOT/'tests/horizon-runtime/lock.json').read_text())['generated_sha256'],
                'host_passed':177,'host_failed':0,'clean_builds':2,'negative_compile_refusals':8,'hardware_execution':'UNKNOWN','game_code_included':False,
                'nro_sha256':hashlib.sha256(self.data).hexdigest()}
    def validate(self):
        (self.app/'build-info.json').write_text(json.dumps(self.m));return self.p.validate(self.root)
    def test_exact_package(self):self.assertEqual(self.validate()[0],self.data)
    def test_native_destination_refused(self):
        self.m['nro_path']='switch/brutal-legend-nx/brutal-legend-nx.nro'
        with self.assertRaises(ValueError):self.validate()
    def test_changed_build_sources_refused(self):
        self.m['build_sources_sha256']={}
        with self.assertRaises(ValueError):self.validate()
    def test_changed_packaging_refused(self):
        self.m['packaging_sources_sha256']={}
        with self.assertRaises(ValueError):self.validate()
    def test_changed_nro_refused(self):
        (self.app/self.p.NRO).write_bytes(self.data+b'changed')
        with self.assertRaises(ValueError):self.validate()
    def test_wrong_generated_fixture_refused(self):
        self.m['input_sha256']='0'*64
        with self.assertRaises(ValueError):self.validate()
    def test_failed_test_receipt_refused(self):
        self.m['host_failed']=1
        with self.assertRaises(ValueError):self.validate()
    def test_hardware_claim_refused(self):
        self.m['hardware_execution']='VERIFIED'
        with self.assertRaises(ValueError):self.validate()
    def test_archive_separate_layout(self):
        for name in self.p.FILES:
            if name not in {self.p.NRO,'build-info.json'}:(self.app/name).write_text('original synthetic metadata')
        self.validate();first=self.root/'a.zip';second=self.root/'b.zip'
        self.p.archive(self.root,first);self.p.archive(self.root,second);self.assertEqual(first.read_bytes(),second.read_bytes())

if __name__=='__main__':unittest.main()
