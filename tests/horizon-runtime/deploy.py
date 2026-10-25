#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Explicit PPC probe package; reuse the tested Hekate USB deployment mechanics.

Default refuses replacing a different PPC NRO. Can never select the old native
probe destination. Does not launch, unmount or change firmware/configuration.
"""
import argparse,importlib.util,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('leaf_package',Path(__file__).with_name('package.py'))
package=importlib.util.module_from_spec(spec);spec.loader.exec_module(package)
sys.path.insert(0,str(ROOT/'tools/switch'))
import deploy as usb

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--package',type=Path,required=True);ap.add_argument('--volume',type=Path)
    ap.add_argument('--replace-selected-ppc',action='store_true',help='Explicitly select replacing this PPC probe only; preserves a verified backup')
    a=ap.parse_args();payload,manifest=package.validate(a.package)
    result=usb.deploy_usb(usb.select(usb.discover(),a.volume),payload,application=package.APP,allow_replace=a.replace_selected_ppc)
    result['build_source_commit']=manifest['source_commit'];print(json.dumps(result,indent=2))
if __name__=='__main__':main()
