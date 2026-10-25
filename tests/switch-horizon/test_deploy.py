#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Mock filesystem only. Never discovers or writes a real SD volume."""
import errno,json,os,struct,sys,tempfile,unittest,zipfile
from pathlib import Path
from unittest.mock import patch,Mock
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/switch'))
import deploy,common
from package import make_zip

class Deployment(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.base=Path(self.temp.name);self.root=self.base/'card';self.root.mkdir()
        for p in ['switch','bootloader','atmosphere','Nintendo/save']:(self.root/p).mkdir(parents=True,exist_ok=True)
        for name in ['bootloader/hekate_ipl.ini','atmosphere/package3','switch/other.nro','Nintendo/save/keep']:(self.root/name).write_bytes(b'untouched')
        self.app=self.root/'switch'/common.APP;self.payload=b'original new test payload'
        self.before=self.outside()
    def outside(self):
        return {p.relative_to(self.root).as_posix():p.read_bytes() for p in self.root.rglob('*') if p.is_file() and common.APP not in p.relative_to(self.root).parts}
    def tearDown(self):self.assertEqual(self.outside(),self.before)
    def run_copy(self,validator=lambda:None):
        fd=deploy.directory(self.root)
        try:return deploy.deploy_fd(fd,self.payload,validator)
        finally:os.close(fd)
    def previous(self):self.app.mkdir();(self.app/common.NRO).write_bytes(b'previous original NRO')
    def assert_previous(self):self.assertEqual((self.app/common.NRO).read_bytes(),b'previous original NRO')
    def test_fresh_install(self):
        row=self.run_copy();self.assertEqual(row['status'],'COPIED_VERIFIED')
        self.assertEqual((self.app/common.NRO).read_bytes(),self.payload)
        self.assertEqual(list(self.app.iterdir()),[self.app/common.NRO])
    def test_backup_exact_bytes(self):
        self.previous();row=self.run_copy()
        self.assertEqual((self.app/row['backup']).read_bytes(),b'previous original NRO')
        self.assertEqual((self.app/common.NRO).read_bytes(),self.payload)
    def test_idempotent(self):
        self.run_copy();before={p.name:p.read_bytes() for p in self.app.iterdir()}
        self.assertEqual(self.run_copy()['status'],'ALREADY_CURRENT_VERIFIED')
        self.assertEqual(before,{p.name:p.read_bytes() for p in self.app.iterdir()})
    def test_unrelated_app_file(self):
        self.app.mkdir();(self.app/'notes.txt').write_text('keep');self.run_copy()
        self.assertEqual((self.app/'notes.txt').read_text(),'keep')
    def test_wrong_structure(self):
        (self.root/'atmosphere/package3').unlink();self.before=self.outside()
        with self.assertRaises(OSError):self.run_copy()
        self.assertFalse(self.app.exists())
    def test_symlink_marker(self):
        p=self.root/'bootloader/hekate_ipl.ini';p.unlink();p.symlink_to(self.root/'atmosphere/package3')
        with self.assertRaises(deploy.Refused):self.run_copy()
        self.assertFalse(self.app.exists())
    def test_symlink_app(self):
        other=self.base/'outside';other.mkdir();self.app.symlink_to(other,target_is_directory=True)
        with self.assertRaises(OSError):self.run_copy()
        self.assertEqual(list(other.iterdir()),[])
    def test_symlink_switch_parent(self):
        (self.root/'switch/other.nro').unlink();(self.root/'switch').rmdir()
        other=self.base/'outside';other.mkdir();(self.root/'switch').symlink_to(other,target_is_directory=True);self.before=self.outside()
        with self.assertRaises(OSError):self.run_copy()
        self.assertEqual(list(other.iterdir()),[])
    def test_symlink_target(self):
        self.app.mkdir();other=self.base/'outside.nro';other.write_bytes(b'keep')
        (self.app/common.NRO).symlink_to(other)
        with self.assertRaises(OSError):self.run_copy()
        self.assertEqual(other.read_bytes(),b'keep')
    def test_hardlink_target(self):
        self.previous();os.link(self.app/common.NRO,self.base/'outside.nro')
        with self.assertRaises(deploy.Refused):self.run_copy()
        self.assert_previous()
    def test_directory_target(self):
        self.app.mkdir();(self.app/common.NRO).mkdir()
        with self.assertRaises(deploy.Refused):self.run_copy()
    def test_existing_lock(self):
        self.app.mkdir();(self.app/'.deploy.lock').write_bytes(b'owned elsewhere')
        with self.assertRaises(FileExistsError):self.run_copy()
        self.assertEqual((self.app/'.deploy.lock').read_bytes(),b'owned elsewhere')
    def test_orphan_metadata_retained(self):
        metadata=self.root/'switch'/('._'+common.APP);metadata.write_bytes(b'user metadata');self.before=self.outside()
        with self.assertRaises(deploy.Refused):self.run_copy()
        self.assertFalse(self.app.exists());self.assertEqual(metadata.read_bytes(),b'user metadata')
    def test_new_file_metadata_not_overwritten(self):
        self.app.mkdir();(self.app/'._new.nro').write_bytes(b'user metadata')
        fd=deploy.directory(self.app)
        try:
            with self.assertRaises(deploy.Refused):deploy.write_new(fd,'new.nro',b'new')
        finally:os.close(fd)
        self.assertEqual((self.app/'._new.nro').read_bytes(),b'user metadata');self.assertFalse((self.app/'new.nro').exists())
    def test_existing_directory_metadata_untouched(self):
        self.previous();metadata=self.root/'switch'/('._'+common.APP)
        metadata.write_bytes(b'existing directory metadata');self.before=self.outside()
        self.run_copy();self.assertEqual(metadata.read_bytes(),b'existing directory metadata')
    def test_older_backups_are_not_pruned(self):
        self.previous();first=self.run_copy();backup=(self.app/first['backup']).read_bytes()
        self.payload=b'second original probe';second=self.run_copy()
        self.assertNotEqual(first['backup'],second['backup'])
        self.assertEqual((self.app/first['backup']).read_bytes(),backup)
        self.assertEqual((self.app/second['backup']).read_bytes(),b'original new test payload')
    def test_failed_write_retains_old(self):
        self.previous()
        with patch.object(deploy.os,'write',side_effect=OSError(errno.ENOSPC,'injected full disk')):
            with self.assertRaises(OSError):self.run_copy()
        self.assert_previous();self.assertEqual(len(list(self.app.iterdir())),1)
    def test_failed_flush_retains_old(self):
        self.previous()
        with patch.object(deploy,'sync_file',side_effect=OSError(errno.EIO,'injected flush failure')):
            with self.assertRaises(OSError):self.run_copy()
        self.assert_previous()
    def test_insufficient_space_retains_old(self):
        self.previous()
        with patch.object(deploy.os,'fstatvfs',return_value=Mock(f_bavail=0,f_frsize=4096)):
            with self.assertRaises(deploy.Refused):self.run_copy()
        self.assert_previous();self.assertEqual(len(list(self.app.iterdir())),1)
    def test_failed_verification_retains_old(self):
        self.previous()
        with patch.object(deploy,'verify_file',side_effect=deploy.Refused('injected mismatch')):
            with self.assertRaises(deploy.Refused):self.run_copy()
        self.assert_previous()
    def test_failed_replace_retains_old_and_backup(self):
        self.previous()
        with patch.object(deploy.os,'replace',side_effect=OSError(errno.EIO,'injected rename failure')):
            with self.assertRaises(OSError):self.run_copy()
        self.assert_previous()
        backups=list(self.app.glob('*.previous-*.nro'));self.assertEqual(len(backups),1)
        self.assertEqual(backups[0].read_bytes(),b'previous original NRO')
    def test_changed_volume_retains_old(self):
        self.previous();calls=[]
        def verify():
            calls.append(1)
            if len(calls)>1:raise deploy.Refused('volume changed')
        with self.assertRaises(deploy.Refused):self.run_copy(verify)
        self.assert_previous()
    def test_initial_validation_before_directory_creation(self):
        def refuse():raise deploy.Refused('unverified volume')
        with self.assertRaises(deploy.Refused):self.run_copy(refuse)
        self.assertFalse(self.app.exists())
    def test_new_target_race_no_overwrite(self):
        original=deploy.rename_new
        def race(fd,src,dst):
            (self.app/common.NRO).write_bytes(b'concurrent file')
            original(fd,src,dst)
        with patch.object(deploy,'rename_new',side_effect=race):
            with self.assertRaises(FileExistsError):self.run_copy()
        self.assertEqual((self.app/common.NRO).read_bytes(),b'concurrent file')
    def test_old_target_race_no_overwrite(self):
        self.previous();calls=[]
        def change():
            calls.append(1)
            if len(calls)==2:(self.app/common.NRO).write_bytes(b'concurrent file')
        with self.assertRaises(deploy.Refused):self.run_copy(change)
        self.assertEqual((self.app/common.NRO).read_bytes(),b'concurrent file')
    def test_invalid_payload(self):
        self.payload=b''
        with self.assertRaises(deploy.Refused):self.run_copy()
        self.assertFalse(self.app.exists())

class Selection(unittest.TestCase):
    def info(self):return {'MountPoint':'/Volumes/CARD','Internal':False,'BusProtocol':'USB','Removable':True,'Writable':True,'FilesystemType':'msdos','VolumeUUID':'unique','DeviceIdentifier':'disk4s1','VolumeName':'arbitrary'}
    def test_valid_metadata(self):self.assertTrue(deploy.metadata_ok(self.info()))
    def test_wrong_metadata(self):
        for key,value in [('Internal',True),('BusProtocol','SATA'),('Removable',False),('Writable',False),('FilesystemType','apfs'),('VolumeUUID',''),('MountPoint','/tmp/SWITCH'),('MountPoint','/Volumes/../escape')]:
            with self.subTest(key=key,value=value):
                info=self.info();info[key]=value;self.assertFalse(deploy.metadata_ok(info))
    def test_missing_metadata(self):self.assertFalse(deploy.metadata_ok({'VolumeName':'SWITCH'}))
    def test_no_candidates(self):
        with self.assertRaises(deploy.Refused):deploy.select([])
    def test_ambiguous(self):
        with self.assertRaises(deploy.Refused):deploy.select([self.info(),self.info()])
    def test_explicit_selection(self):self.assertEqual(deploy.select([self.info()],Path('/Volumes/CARD')),self.info())
    def test_path_traversal_and_wrong_selection(self):
        for name in ['/Volumes/CARD/../elsewhere','/tmp/CARD','/Volumes/OTHER']:
            with self.assertRaises(deploy.Refused):deploy.select([self.info()],Path(name))
    def test_nxlink_exact_options(self):
        with patch.object(deploy.subprocess,'run',return_value=Mock(stdout='--address --retries --path --server')):
            args=deploy.nxlink_command('/tool/nxlink','192.0.2.1',Path('/local/probe.nro'))
        self.assertEqual(args,['/tool/nxlink','-a','192.0.2.1','-r','2','-p','/'+common.RELATIVE,'/local/probe.nro'])
        self.assertNotIn('-s',args)
    def test_nxlink_invalid_address(self):
        with self.assertRaises(ValueError):deploy.nxlink_command('/tool','192.0.2.1;bad',Path('probe'))

class Package(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);self.app=self.root/'switch'/common.APP;self.app.mkdir(parents=True)
        data=bytearray(184);data[16:20]=b'NRO0';struct.pack_into('<I',data,24,128);data[128:132]=b'ASET'
        self.payload=bytes(data);(self.app/common.NRO).write_bytes(data)
        lock=json.loads((ROOT/'tests/switch-horizon/toolchain.json').read_text())
        self.manifest={'schema':1,'nro_path':common.RELATIVE,'source_commit':'1'*40,'image':lock['image'],'build_sources_sha256':common.source_hashes(),'packaging_sources_sha256':common.packaging_hashes(),'host_passed':81,'host_failed':0,'clean_builds':2,'nro_sha256':common.sha(data)}
        self.save()
        for n in ['README.txt','SHA256SUMS','LICENSE.txt','THIRD_PARTY.txt']:(self.app/n).write_text('original test')
    def save(self):(self.app/'build-info.json').write_text(json.dumps(self.manifest))
    def test_valid_local_integrity_receipt(self):self.assertEqual(common.validate_package(self.root)[0],self.payload)
    def test_corrupt_nro(self):
        (self.app/common.NRO).write_bytes(self.payload+b'corrupt')
        with self.assertRaises(ValueError):common.validate_package(self.root)
    def test_changed_source_provenance(self):
        self.manifest['build_sources_sha256']['runtime/checked_memory.h']='0'*64;self.save()
        with self.assertRaises(ValueError):common.validate_package(self.root)
    def test_changed_packaging_provenance(self):
        self.manifest['packaging_sources_sha256']['tools/switch/common.py']='0'*64;self.save()
        with self.assertRaises(ValueError):common.validate_package(self.root)
    def test_absolute_binary_path_refused_even_with_matching_hash(self):
        data=self.payload+b'/'+b'Users'+b'/private/example\0';(self.app/common.NRO).write_bytes(data)
        self.manifest['nro_sha256']=common.sha(data);self.save()
        with self.assertRaises(ValueError):common.validate_package(self.root)
    def test_wrong_image(self):
        self.manifest['image']='untrusted:latest';self.save()
        with self.assertRaises(ValueError):common.validate_package(self.root)
    def test_manifest_path_traversal(self):
        self.manifest['nro_path']='../../outside.nro';self.save()
        with self.assertRaises(ValueError):common.validate_package(self.root)
    def test_missing_build_pass(self):
        self.manifest['host_failed']=1;self.save()
        with self.assertRaises(ValueError):common.validate_package(self.root)
    def test_symlink_nro(self):
        (self.app/common.NRO).unlink();outside=self.root/'outside';outside.write_bytes(self.payload);(self.app/common.NRO).symlink_to(outside)
        with self.assertRaises(OSError):common.validate_package(self.root)
    def test_zip_exact_safe_layout_and_determinism(self):
        first=self.root/'one.zip';second=self.root/'two.zip';make_zip(self.root,first);make_zip(self.root,second)
        self.assertEqual(first.read_bytes(),second.read_bytes())
        with zipfile.ZipFile(first) as z:
            self.assertEqual(set(z.namelist()),{f'switch/{common.APP}/'+n for n in [common.NRO,'build-info.json','README.txt','SHA256SUMS','LICENSE.txt','THIRD_PARTY.txt']})
            self.assertEqual(z.read(common.RELATIVE),self.payload)
            self.assertFalse(any(n.startswith('/') or '..' in Path(n).parts for n in z.namelist()))
if __name__=='__main__':unittest.main()
