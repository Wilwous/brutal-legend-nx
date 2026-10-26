#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Package verified original synthetic stages without changing the prior probes.

No device writes. Source receipts are checked against the current checkout.
"""
import argparse,hashlib,json,shutil,subprocess,sys,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def package(build,work,app,title,expected):
 work.mkdir(parents=True,exist_ok=False);r=json.loads((build/'results.json').read_text())
 for n,digest in r['sources_sha256'].items():
  if sha(ROOT/n)!=digest:raise ValueError('build source changed: '+n)
 if len(r['host'])!=3 or any(h['failed'] or h['passed']!=expected for h in r['host']):raise ValueError('host tests did not pass')
 if any(p['timeout'] or p['exit_code']!=p['expected_exit_code'] for p in r['records']):raise ValueError('unexpected process outcome')
 if r['switch']['deterministic_builds']!=2:raise ValueError('two builds required')
 for n,digest in r['switch']['sha256'].items():
  if sha(build/'switch-0'/n)!=digest or (build/'switch-0'/n).read_bytes()!=(build/'switch-1'/n).read_bytes():raise ValueError('artifact mismatch')
 if r['switch']['inspection']['unresolved_strong_symbols'] or r['hardware_execution']!='UNKNOWN':raise ValueError('invalid link/runtime receipt')
 appdir=work/'package/switch'/app;appdir.mkdir(parents=True);nro=app+'.nro';shutil.copyfile(build/'switch-0/probe.nro',appdir/nro)
 if b'/Users/' in (appdir/nro).read_bytes() or b'/home/' in (appdir/nro).read_bytes():raise ValueError('private path in NRO')
 revision=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True,timeout=10).strip()
 manifest={'application':title,'nro_path':'switch/'+app+'/'+nro,'nro_sha256':sha(appdir/nro),'source_commit':revision,
           'source_worktree_dirty':bool(subprocess.check_output(['git','status','--porcelain=v1'],cwd=ROOT,timeout=10)),
           'sources_sha256':r['sources_sha256'],'host_passed':expected*3,'host_failed':0,'expected_console_passed':expected+1,
           'hardware_execution':'UNKNOWN','game_code_included':False,'build':r['switch']}
 (appdir/'build-info.json').write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n');(appdir/'SHA256SUMS').write_text(sha(appdir/nro)+'  '+nro+'\n')
 shutil.copyfile(ROOT/'LICENSE',appdir/'LICENSE.txt');shutil.copyfile(ROOT/'tests/horizon-runtime/NOTICE.txt',appdir/'THIRD_PARTY.txt')
 (appdir/'README.txt').write_text(title+'\nOriginal synthetic fixture. NOT the game.\nCopy switch/ to SD root. Keep all earlier probes.\nBUILD VERIFIED; HARDWARE EXECUTION UNKNOWN.\nExpected successful console summary: PASS='+str(expected+1)+' FAIL=0. This has not yet been observed.\nLaunch using the existing homebrew setup; prefer Title Override. Photograph the complete screen, launch mode and platform versions. Press PLUS to exit.\nSafely eject SD storage before leaving Hekate USB Mass Storage.\nSource: https://github.com/Wilwous/brutal-legend-nx/tree/research/switch-horizon-feasibility\n')
 names=sorted(str(p.relative_to(work/'package')) for p in appdir.iterdir())
 def archive(target):
  with zipfile.ZipFile(target,'x',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
   for name in names:
    info=zipfile.ZipInfo(name,(1980,1,1,0,0,0));info.create_system=3;info.external_attr=0o100644<<16;info.compress_type=zipfile.ZIP_DEFLATED
    z.writestr(info,(work/'package'/name).read_bytes(),compresslevel=9)
  with zipfile.ZipFile(target) as z:
   if z.testzip() or sorted(z.namelist())!=names:raise ValueError('ZIP integrity')
   for name in names:
    if z.read(name)!=(work/'package'/name).read_bytes():raise ValueError('ZIP bytes mismatch')
 target=work/(app+'.zip');repeat=work/'repeat.zip';archive(target);archive(repeat)
 if sha(target)!=sha(repeat):raise ValueError('non-deterministic ZIP')
 repeat.unlink();row={'nro':str(appdir/nro),'zip':str(target),'nro_sha256':sha(appdir/nro),'zip_sha256':sha(target),'hardware_execution':'UNKNOWN'}
 (work/'package-result.json').write_text(json.dumps(row,indent=2)+'\n');return row
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--build',type=Path,required=True);ap.add_argument('--workdir',type=Path,required=True)
 ap.add_argument('--application',choices=['foundation','xex'],required=True);a=ap.parse_args()
 if a.workdir.resolve()==ROOT or ROOT in a.workdir.resolve().parents:ap.error('private output required')
 if a.application=='foundation':app,title,count='brutal-legend-nx-foundation','Brutal Legend NX - Horizon Runtime Foundation',108
 else:app,title,count='brutal-legend-nx-xex','Brutal Legend NX - Synthetic XEX Runtime',118
 print(json.dumps(package(a.build.resolve(),a.workdir.resolve(),app,title,count),indent=2))
