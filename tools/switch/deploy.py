#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Conservative macOS Hekate USB-SD deployment; never a firmware/device runner."""
import argparse,ctypes,errno,fcntl,ipaddress,json,os,plistlib,shutil,stat,subprocess,sys,uuid
from pathlib import Path
from common import APP,NRO,RELATIVE,validate_package,sha

LIMIT=16*1024*1024
class Refused(RuntimeError):pass
def directory(name,parent=None):
    return os.open(name,os.O_RDONLY|os.O_DIRECTORY|os.O_NOFOLLOW,dir_fd=parent)
def token(info):return (info.st_dev,info.st_ino,info.st_size,info.st_mtime_ns)
def read_fd(fd):
    info=os.fstat(fd)
    if not stat.S_ISREG(info.st_mode) or info.st_nlink!=1 or info.st_size>LIMIT:raise Refused('unsafe application file type/size/link count')
    os.lseek(fd,0,os.SEEK_SET);data=b''
    while True:
        block=os.read(fd,65536)
        if not block:break
        data+=block
        if len(data)>LIMIT:raise Refused('application file grew beyond limit')
    if token(info)!=token(os.fstat(fd)):raise Refused('application file changed while reading')
    return data
def structure(root):
    for folder,leaf in [('bootloader','hekate_ipl.ini'),('atmosphere','package3')]:
        fd=directory(folder,root)
        try:
            info=os.stat(leaf,dir_fd=fd,follow_symlinks=False)
            if not stat.S_ISREG(info.st_mode):raise Refused('required Switch marker is not a regular file')
        finally:os.close(fd)
    fd=directory('switch',root);os.close(fd)
def metadata_ok(info):
    root=Path(info.get('MountPoint',''))
    return (root.is_absolute() and len(root.parts)==3 and root.parts[1]=='Volumes' and root.name not in {'.','..'}
            and info.get('Internal') is False and info.get('BusProtocol')=='USB'
            and (info.get('Removable') is True or info.get('RemovableMedia') is True)
            and info.get('Writable') is True and info.get('FilesystemType') in {'msdos','exfat'}
            and bool(info.get('VolumeUUID')) and bool(info.get('DeviceIdentifier')))
def disk_info(path):
    r=subprocess.run(['/usr/sbin/diskutil','info','-plist',str(path)],capture_output=True,timeout=10,check=True)
    return plistlib.loads(r.stdout)
def discover():
    if sys.platform!='darwin':raise Refused('USB discovery requires macOS diskutil')
    r=subprocess.run(['/usr/sbin/diskutil','list','-plist'],capture_output=True,timeout=10,check=True)
    volumes=[]
    for name in plistlib.loads(r.stdout).get('VolumesFromDisks',[]):
        # diskutil, not a user-supplied name, establishes the actual mountpoint.
        try:
            info=disk_info('/Volumes/'+name)
            if not metadata_ok(info) or not os.path.ismount(info['MountPoint']):continue
            fd=directory(info['MountPoint'])
            try:
                if os.fstat(fd).st_dev==os.stat('/Volumes').st_dev:continue
                structure(fd)
            finally:os.close(fd)
            volumes.append(info)
        except (OSError,ValueError,Refused,subprocess.SubprocessError):continue
    return volumes
def select(volumes,chosen=None):
    if chosen is not None:
        matches=[v for v in volumes if v['MountPoint']==str(chosen)]
        if len(matches)!=1:raise Refused('selected path is not a verified Switch USB volume')
        return matches[0]
    if len(volumes)!=1:raise Refused('no unique verified Switch USB volume; inspect --list and select --volume if ambiguous')
    return volumes[0]
def sync_file(fd):
    os.fsync(fd)
    if sys.platform=='darwin':
        try:fcntl.fcntl(fd,51);return 'fsync + F_FULLFSYNC'
        except OSError as e:
            if e.errno not in {errno.EINVAL,errno.ENOTSUP,errno.ENOTTY}:raise
    return 'fsync (full device-cache flush unavailable)'
def absent(parent,name):
    try:os.stat(name,dir_fd=parent,follow_symlinks=False);return False
    except FileNotFoundError:return True
def write_new(parent,name,data):
    if not absent(parent,'._'+name):raise Refused('pre-existing metadata companion; no overwrite')
    fd=os.open(name,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o644,dir_fd=parent)
    try:
        remaining=memoryview(data)
        while remaining:
            count=os.write(fd,remaining)
            if count<=0:raise OSError('short write')
            remaining=remaining[count:]
        return sync_file(fd)
    finally:os.close(fd)
def verify_file(parent,name,expected):
    fd=os.open(name,os.O_RDONLY|os.O_NOFOLLOW,dir_fd=parent)
    try:
        if read_fd(fd)!=expected:raise Refused('copied bytes failed verification')
        return token(os.fstat(fd))
    finally:os.close(fd)
def rename_new(parent,source,dest):
    # An initial install must not overwrite a concurrently created file.
    if sys.platform=='darwin':
        libc=ctypes.CDLL(None,use_errno=True)
        fn=libc.renameatx_np;fn.argtypes=[ctypes.c_int,ctypes.c_char_p,ctypes.c_int,ctypes.c_char_p,ctypes.c_uint];fn.restype=ctypes.c_int
        if fn(parent,os.fsencode(source),parent,os.fsencode(dest),4):
            value=ctypes.get_errno();raise OSError(value,os.strerror(value))
    else:
        # Used for host mock-filesystem regressions, never macOS FAT fallback.
        os.link(source,dest,src_dir_fd=parent,dst_dir_fd=parent,follow_symlinks=False)
        os.unlink(source,dir_fd=parent)
def deploy_fd(root,payload,revalidate):
    if not payload or len(payload)>LIMIT:raise Refused('invalid deployment payload')
    revalidate();structure(root)
    switch=directory('switch',root)
    try:
        if absent(switch,APP):
            if not absent(switch,'._'+APP):raise Refused('pre-existing app metadata without directory; retained')
            try:os.mkdir(APP,0o755,dir_fd=switch)
            except FileExistsError:pass
        app=directory(APP,switch)
    finally:os.close(switch)
    lock=None;tmp=None
    try:
        lock=os.open('.deploy.lock',os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_NOFOLLOW,0o600,dir_fd=app)
        old=None;old_token=None
        try:fd=os.open(NRO,os.O_RDONLY|os.O_NOFOLLOW,dir_fd=app)
        except FileNotFoundError:pass
        else:
            try:old=read_fd(fd);old_token=token(os.fstat(fd))
            finally:os.close(fd)
        if old==payload:return {'status':'ALREADY_CURRENT_VERIFIED','sha256':sha(payload),'backup':None}
        free=os.fstatvfs(app)
        if free.f_bavail*free.f_frsize<len(payload)+len(old or b'')+1024*1024:raise Refused('insufficient free space; original retained')
        tmp='.brutal-legend-nx-'+uuid.uuid4().hex+'.tmp'
        flush=write_new(app,tmp,payload);verify_file(app,tmp,payload)
        backup=None
        if old is not None:
            backup='brutal-legend-nx.previous-'+sha(old)[:12]+'-'+uuid.uuid4().hex[:12]+'.nro'
            write_new(app,backup,old);verify_file(app,backup,old)
        revalidate();structure(root)
        # An opened directory pins its identity, but reject renamed/replaced parents.
        switch=directory('switch',root)
        try:
            current=directory(APP,switch)
            try:
                if token(os.fstat(current))[:2]!=token(os.fstat(app))[:2]:raise Refused('application directory changed')
            finally:os.close(current)
        finally:os.close(switch)
        if old is None:rename_new(app,tmp,NRO)
        else:
            if token(os.stat(NRO,dir_fd=app,follow_symlinks=False))!=old_token:raise Refused('previous application changed; refusing replacement')
            os.replace(tmp,NRO,src_dir_fd=app,dst_dir_fd=app)
        tmp=None
        verify_file(app,NRO,payload)
        directory_sync='fsync'
        try:os.fsync(app)
        except OSError as e:
            if e.errno not in {errno.EINVAL,errno.ENOTSUP}:raise
            directory_sync='unsupported; safe eject required, power-loss durability unverified'
        return {'status':'COPIED_VERIFIED','sha256':sha(payload),'backup':backup,'file_flush':flush,'directory_flush':directory_sync}
    finally:
        if tmp:
            try:os.unlink(tmp,dir_fd=app)
            except FileNotFoundError:pass
        if lock is not None:
            try:
                if token(os.stat('.deploy.lock',dir_fd=app,follow_symlinks=False))==token(os.fstat(lock)):os.unlink('.deploy.lock',dir_fd=app)
            finally:os.close(lock)
        os.close(app)
def deploy_usb(info,payload):
    root=directory(info['MountPoint'])
    identity=os.fstat(root)
    def revalidate():
        latest=disk_info(info['MountPoint'])
        if not metadata_ok(latest) or not os.path.ismount(info['MountPoint']):raise Refused('volume is no longer a valid mounted USB target')
        for key in ['VolumeUUID','DeviceIdentifier','MountPoint']:
            if latest[key]!=info[key]:raise Refused('volume identity changed')
        if token(os.stat(info['MountPoint'],follow_symlinks=False))[:2]!=token(identity)[:2]:raise Refused('volume root changed')
        if identity.st_dev==os.stat('/Volumes').st_dev:raise Refused('destination is on the host filesystem')
    try:
        result=deploy_fd(root,payload,revalidate)
        # Flush directory creation/rename/lock cleanup too, after app handles close.
        subprocess.run(['/bin/sync'],check=True,timeout=15)
        revalidate();structure(root)
        switch=directory('switch',root)
        try:
            app=directory(APP,switch)
            try:verify_file(app,NRO,payload)
            finally:os.close(app)
        finally:os.close(switch)
        result['macos_sync']='completed; final bytes reverified'
    finally:os.close(root)
    result.update(destination=info['MountPoint']+'/'+RELATIVE,device=info['DeviceIdentifier'],safe_eject_required=True,hardware_execution='UNKNOWN')
    return result
def nxlink_command(executable,address,nro):
    ipaddress.IPv4Address(address)
    help_text=subprocess.run([executable,'--help'],capture_output=True,text=True,timeout=5).stdout
    if not all(s in help_text for s in ['--address','--retries','--path']):raise Refused('unsupported nxlink CLI')
    return [executable,'-a',address,'-r','2','-p','/'+RELATIVE,str(nro)]
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--transport',choices=['usb','nxlink'],default='usb')
    ap.add_argument('--package',type=Path,default=Path.home()/'.local/share/brutal-legend-nx-switch-usb/ready/package')
    ap.add_argument('--volume',type=Path);ap.add_argument('--list',action='store_true')
    ap.add_argument('--address');ap.add_argument('--nxlink',default=shutil.which('nxlink'))
    a=ap.parse_args()
    if a.list:
        print(json.dumps([{k:v[k] for k in ['MountPoint','DeviceIdentifier','FilesystemType','VolumeName']} for v in discover()],indent=2));return
    payload,manifest=validate_package(a.package)
    if a.transport=='usb':result=deploy_usb(select(discover(),a.volume),payload)
    else:
        if discover():raise Refused('Switch USB SD storage is still mounted; leave Hekate before NetLoader')
        if not a.address or not a.nxlink:raise Refused('nxlink requires an installed native executable and explicit --address')
        command=nxlink_command(a.nxlink,a.address,a.package/RELATIVE)
        subprocess.run(command,check=True,timeout=30)
        result={'status':'NXLINK_TOOL_EXITED_ZERO','hardware_execution':'UNKNOWN','runtime_logging':'not enabled'}
    result['build_source_commit']=manifest['source_commit'];print(json.dumps(result,indent=2))
if __name__=='__main__':
    try:main()
    except (OSError,ValueError,KeyError,Refused,subprocess.SubprocessError) as e:print('REFUSED: '+str(e),file=sys.stderr);sys.exit(1)
