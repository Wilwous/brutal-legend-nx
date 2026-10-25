#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Build original probe twice and produce an exact deterministic SD-layout ZIP."""
import argparse,json,os,shutil,subprocess,sys,zipfile
from pathlib import Path
from common import ROOT,APP,NRO,RELATIVE,source_hashes,packaging_hashes,sha,validate_package

def make_zip(package,destination):
    names=[f'switch/{APP}/'+n for n in [NRO,'build-info.json','README.txt','SHA256SUMS','LICENSE.txt','THIRD_PARTY.txt']]
    with zipfile.ZipFile(destination,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
        for name in sorted(names):
            info=zipfile.ZipInfo(name,(1980,1,1,0,0,0));info.create_system=3
            info.external_attr=0o100644<<16;info.compress_type=zipfile.ZIP_DEFLATED
            archive.writestr(info,(package/name).read_bytes(),compresslevel=9)
    with zipfile.ZipFile(destination) as archive:
        if sorted(archive.namelist())!=sorted(names) or archive.testzip():raise ValueError('invalid package archive')
        for name in names:
            if archive.read(name)!=(package/name).read_bytes():raise ValueError('archive bytes differ')

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--workdir',required=True,type=Path)
    ap.add_argument('--from-build',type=Path,help='Repackage an already verified unchanged build; no recompilation')
    a=ap.parse_args();work=a.workdir.resolve()
    if work==ROOT or ROOT in work.parents:ap.error('private output required')
    work.mkdir(parents=True,exist_ok=False)
    if a.from_build:build=a.from_build.resolve()
    else:
        from icon import create
        create(work/'icon.jpg');build=work/'build'
        subprocess.run([sys.executable,str(ROOT/'tests/switch-horizon/run.py'),'--workdir',str(build),'--switch-build','--icon',str(work/'icon.jpg')],check=True)
    results=json.loads((build/'results.json').read_text())
    if results['build_sources_sha256']!=source_hashes():raise ValueError('build inputs changed; rebuild required')
    for key,count in [('host',21),('ui_host',6)]:
        if len(results[key])!=3 or any(row['passed']!=count or row['failed'] for row in results[key]):raise ValueError('host contracts did not pass')
    if any(row['exit_code'] or row['timeout'] for row in results['records']):raise ValueError('failed build process receipt')
    switch=results['switch']
    if switch['deterministic_builds']!=2 or switch['inspection']['hbmenu']['icon_bytes']<=0:raise ValueError('missing deterministic/icon inspection')
    for name in ['probe.nro','probe.elf']:
        first=(build/'switch-0'/name).read_bytes();second=(build/'switch-1'/name).read_bytes()
        if first!=second or sha(first)!=switch['sha256'][name]:raise ValueError('build artifact changed')
    package=work/'package';app=package/'switch'/APP;app.mkdir(parents=True)
    data=(build/'switch-0/probe.nro').read_bytes();(app/NRO).write_bytes(data)
    lock=json.loads((ROOT/'tests/switch-horizon/toolchain.json').read_text())
    commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True,timeout=10).strip()
    dirty=bool(subprocess.check_output(['git','status','--porcelain=v1'],cwd=ROOT,timeout=10))
    manifest={'schema':1,'application':'Brutal Legend NX Probe','version':'0.1.0','nro_path':RELATIVE,
              'source_commit':commit,'source_worktree_dirty':dirty,'build_sources_sha256':source_hashes(),
              'packaging_sources_sha256':packaging_hashes(),
              'image':lock['image'],'packages':lock['packages'],'nro_sha256':sha(data),
              'elf_sha256':switch['sha256']['probe.elf'],'host_passed':81,'host_failed':0,
              'clean_builds':2,'expected_hardware_result':'PASS=23 FAIL=0','hardware_execution':'UNKNOWN',
              'game_code_included':False,'rexglue_integration':'BLOCKED'}
    (app/'build-info.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
    (app/'SHA256SUMS').write_text(sha(data)+'  '+NRO+'\n')
    shutil.copyfile(ROOT/'LICENSE',app/'LICENSE.txt')
    shutil.copyfile(ROOT/'tools/switch/NOTICE.txt',app/'THIRD_PARTY.txt')
    (app/'README.txt').write_text('BRUTAL LEGEND NX PROBE - ORIGINAL SYNTHETIC TEST, NOT THE GAME\n\nCopy the switch/ directory to the SD root. Path: sdmc:/'+RELATIVE+'\nSafely eject from macOS before leaving Hekate USB storage. Boot normally, open hbmenu, launch Brutal Legend NX Probe.\nExpected success: PASS=23 FAIL=0. Record results/launch mode. Press PLUS (+) to exit; applet shutdown also exits.\nNo hardware pass, game boot or ReXGlue integration is claimed. Do not run while Hekate exposes the SD card.\nSource and GPL-3.0 license: https://github.com/Wilwous/brutal-legend-nx/tree/'+commit+'\nExact build-input hashes are in build-info.json; check source_worktree_dirty when reproducing.\n')
    validate_package(package)
    archive=work/'brutal-legend-nx-switch-probe.zip';make_zip(package,archive)
    repeat=work/'repeat.zip';make_zip(package,repeat)
    if archive.read_bytes()!=repeat.read_bytes():raise ValueError('ZIP is not deterministic')
    repeat.unlink()
    record={'nro':str(app/NRO),'nro_sha256':sha(data),'zip':str(archive),'zip_sha256':sha(archive.read_bytes()),'package':'VERIFIED','hardware':'UNKNOWN'}
    (work/'package-result.json').write_text(json.dumps(record,indent=2)+'\n')
    print(json.dumps(record,indent=2))
if __name__=='__main__':
    try:main()
    except (OSError,ValueError,KeyError,subprocess.SubprocessError) as e:print(str(e),file=sys.stderr);sys.exit(1)
