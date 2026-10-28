#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Package only the verified integrated synthetic runtime; no device writes."""
import argparse,hashlib,json,re,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/switch'));import stage_package,common
APP='brutal-legend-nx-runtime-ii';RELATIVE=f'switch/{APP}/{APP}.nro'
def digest(data):return hashlib.sha256(data).hexdigest()
def validate(directory):
 directory=Path(directory).absolute();app=directory/'switch'/APP
 for p in [directory,directory/'switch',app]:
  if p.is_symlink() or not p.is_dir():raise ValueError('missing or symlinked package directory')
 manifest=json.loads(common.regular_bytes(app/'build-info.json',65536));payload=common.regular_bytes(app/(APP+'.nro'),16*1024*1024);common.check_nro(payload)
 if set(p.name for p in app.iterdir())!={APP+'.nro','build-info.json','SHA256SUMS','LICENSE.txt','THIRD_PARTY.txt','README.txt'}:raise ValueError('unexpected app contents')
 if manifest.get('application')!='Brutal Legend NX - Integrated Runtime II' or manifest.get('nro_path')!=RELATIVE:raise ValueError('wrong app destination')
 if common.regular_bytes(app/'SHA256SUMS',256).decode()!=digest(payload)+'  '+APP+'.nro\n':raise ValueError('checksum manifest mismatch')
 if manifest.get('nro_sha256')!=digest(payload) or not re.fullmatch('[0-9a-f]{40}',manifest.get('source_commit','')):raise ValueError('payload/revision integrity')
 hashes=manifest.get('sources_sha256',{})
 required={'runtime/checked_memory.h','runtime/horizon/xex.h','runtime/diagnostics.h','runtime/horizon/query.h','tests/horizon-runtime-ii/query.cpp','tests/horizon-runtime-ii/fixture.py','tests/horizon-runtime-ii/run.py','tests/horizon-runtime-ii/query-lock.json'}
 required|={p.relative_to(ROOT).as_posix() for p in (ROOT/'tests/horizon-runtime-ii').rglob('*') if p.is_file() and '__pycache__' not in str(p)}
 if not required<=set(hashes):raise ValueError('incomplete source receipt')
 for name,value in hashes.items():
  path=ROOT/name
  if path.resolve().is_relative_to(ROOT) is False or digest(path.read_bytes())!=value:raise ValueError('source provenance changed')
 if manifest.get('source_worktree_dirty') is not False:raise ValueError('uncommitted package source')
 if manifest.get('hardware_execution')!='UNKNOWN' or manifest.get('game_code_included') is not False or manifest.get('host_failed')!=0:raise ValueError('invalid runtime/test claim')
 passed=manifest.get('host_passed',0)
 if not isinstance(passed,int) or passed<=0 or isinstance(passed,bool) or passed%3 or manifest.get('expected_console_passed')!=passed//3+1:raise ValueError('inconsistent assertion counts')
 build=manifest.get('build',{})
 if build.get('deterministic_builds')!=2 or build.get('inspection',{}).get('unresolved_strong_symbols')!=0:raise ValueError('invalid build proof')
 if build['inspection'].get('nro_sha256')!=digest(payload):raise ValueError('inspection payload mismatch')
 return payload,manifest

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for name in ['build','benchmark','workdir']:ap.add_argument('--'+name,type=Path,required=True)
 a=ap.parse_args();build=a.build.resolve();work=a.workdir.resolve();receipt=json.loads((build/'results.json').read_text());benchmark=json.loads((a.benchmark/'results.json').read_text())
 if work==ROOT or ROOT in work.parents:ap.error('private output required')
 if receipt.get('stage')!=11 or len(receipt.get('stress',[]))!=3 or any(row['failed'] for row in receipt['stress']):raise ValueError('passing full Stage 11 required')
 if {r['profile'] for r in receipt['host']}!={'Debug','Release','Sanitized'} or {r['profile'] for r in receipt['stress']}!={'Debug','Release','Sanitized'}:raise ValueError('three distinct profiles required')
 if any(not group['passed'] or group['failed'] for row in receipt['host'] for group in row['groups']):raise ValueError('missing milestone assertions')
 if benchmark.get('failed')!=0 or benchmark.get('host_only') is not True:raise ValueError('host benchmark proof required')
 if benchmark.get('build_fingerprint')!=receipt.get('build_fingerprint'):raise ValueError('benchmark belongs to another build')
 if subprocess.check_output(['git','status','--porcelain=v1'],cwd=ROOT,timeout=10):raise ValueError('commit verified sources before packaging')
 expected=receipt['host'][0]['passed'];result=stage_package.package(build,work,APP,'Brutal Legend NX - Integrated Runtime II',expected)
 validate(work/'package');print(json.dumps(result,indent=2))
if __name__=='__main__':main()
