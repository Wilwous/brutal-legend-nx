#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Only the new integrated app; preserve old probes and refuse automatic replacement."""
import argparse,importlib.util,json,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
s=importlib.util.spec_from_file_location('integrated_package',Path(__file__).with_name('package.py'));package=importlib.util.module_from_spec(s);s.loader.exec_module(package)
sys.path.insert(0,str(ROOT/'tools/switch'));import deploy as usb

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--package',type=Path,required=True);ap.add_argument('--replace-selected-runtime',action='store_true');a=ap.parse_args()
 payload,manifest=package.validate(a.package);volumes=usb.discover();matches=[v for v in volumes if v['MountPoint']=='/Volumes/ULTRASWITCH']
 if len(matches)!=1:print(json.dumps({'deployment':'WAITING_FOR_VERIFIED_ULTRASWITCH','device_files_changed':False,'hardware_execution':'UNKNOWN'}));return
 info=usb.select(matches,Path('/Volumes/ULTRASWITCH'));result=usb.deploy_usb(info,payload,application=package.APP,allow_replace=a.replace_selected_runtime)
 eject=subprocess.run(['/usr/sbin/diskutil','eject',info['MountPoint']],capture_output=True,text=True,timeout=15)
 result['safe_eject']='SUCCESS' if eject.returncode==0 else 'BLOCKED; do not force unmount';result['source_commit']=manifest['source_commit'];result['eject_diagnostics']=(eject.stdout+eject.stderr).strip();print(json.dumps(result,indent=2))
if __name__=='__main__':main()
