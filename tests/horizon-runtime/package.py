#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Package ONLY the verified original generated leaf NRO into a separate SD path."""
import argparse,importlib.util,json,re,shutil,subprocess,sys,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/switch'))
from common import sha,check_nro,regular_bytes
spec=importlib.util.spec_from_file_location('leaf_runner',Path(__file__).with_name('run.py'))
runner=importlib.util.module_from_spec(spec);spec.loader.exec_module(runner)
APP='brutal-legend-nx-ppc';NRO=APP+'.nro';RELATIVE='switch/'+APP+'/'+NRO
FILES=[NRO,'build-info.json','SHA256SUMS','README.txt','LICENSE.txt','THIRD_PARTY.txt']
def packaging_hashes():
    return {n:sha((ROOT/n).read_bytes()) for n in ['tests/horizon-runtime/package.py','tests/horizon-runtime/deploy.py','tests/horizon-runtime/NOTICE.txt','tools/switch/deploy.py','tools/switch/common.py','LICENSE']}
def validate(directory):
    directory=Path(directory).absolute()
    for p in [directory,directory/'switch',directory/'switch'/APP]:
        if p.is_symlink() or not p.is_dir():raise ValueError('unsafe package directory')
    app=directory/'switch'/APP;manifest=json.loads(regular_bytes(app/'build-info.json',65536))
    lock=json.loads((ROOT/'tests/switch-horizon/toolchain.json').read_text())
    pinned=json.loads((ROOT/'tests/horizon-runtime/lock.json').read_text())
    if not re.fullmatch('[0-9a-f]{40}',manifest.get('source_commit','')):raise ValueError('missing source revision')
    if manifest.get('generated_sha256')!=pinned['generated_sha256'] or manifest.get('input_sha256')!=runner.BINARY_SHA256:raise ValueError('wrong generated fixture')
    if manifest.get('schema')!=1 or manifest.get('nro_path')!=RELATIVE or manifest.get('application')!='Brutal Legend NX - PPC Runtime Probe':raise ValueError('wrong package destination')
    if manifest.get('build_sources_sha256')!=runner.inputs() or manifest.get('packaging_sources_sha256')!=packaging_hashes():raise ValueError('package source provenance changed')
    if manifest.get('image')!=lock['image'] or manifest.get('host_passed')!=177 or manifest.get('host_failed')!=0 or manifest.get('clean_builds')!=2 or manifest.get('negative_compile_refusals')!=8:raise ValueError('missing build/test evidence')
    if manifest.get('hardware_execution')!='UNKNOWN' or manifest.get('game_code_included') is not False:raise ValueError('invalid runtime claim')
    data=regular_bytes(app/NRO,16*1024*1024);check_nro(data)
    if sha(data)!=manifest.get('nro_sha256'):raise ValueError('NRO hash mismatch')
    return data,manifest

def archive(package,target):
    with zipfile.ZipFile(target,'x',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for leaf in sorted(FILES):
            name='switch/'+APP+'/'+leaf;info=zipfile.ZipInfo(name,(1980,1,1,0,0,0));info.create_system=3
            info.external_attr=0o100644<<16;info.compress_type=zipfile.ZIP_DEFLATED
            z.writestr(info,(package/name).read_bytes(),compresslevel=9)
    with zipfile.ZipFile(target) as z:
        if sorted(z.namelist())!=sorted('switch/'+APP+'/'+n for n in FILES) or z.testzip():raise ValueError('ZIP layout failure')
        for name in z.namelist():
            if z.read(name)!=(package/name).read_bytes():raise ValueError('ZIP data changed')

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--from-build',type=Path,required=True);ap.add_argument('--workdir',type=Path,required=True)
    a=ap.parse_args();work=a.workdir.resolve();build=a.from_build.resolve()
    if work==ROOT or ROOT in work.parents:ap.error('private output required')
    work.mkdir(parents=True,exist_ok=False);results=json.loads((build/'results.json').read_text())
    if results['build_sources_sha256']!=runner.inputs():raise ValueError('build inputs changed')
    if len(results['host'])!=3 or any(r['passed']!=59 or r['failed'] for r in results['host']):raise ValueError('host failure')
    if results['negative_compile_refusals']!=8 or not results['body_identical_to_pinned_default']:raise ValueError('generation/refusal evidence missing')
    if any(r['timeout'] or r['exit_code']!=r['expected_exit_code'] for r in results['records']):raise ValueError('unexpected process result')
    sw=results['switch']
    if sw['deterministic_builds']!=2 or sw['inspection']['unresolved_strong_symbols']!=0 or sw['inspection']['hbmenu']['icon_bytes']<=0:raise ValueError('link inspection missing')
    for name,digest in sw['sha256'].items():
        if sha((build/'switch-0'/name).read_bytes())!=digest or (build/'switch-0'/name).read_bytes()!=(build/'switch-1'/name).read_bytes():raise ValueError('build artifact changed')
    for name,digest in results['generated_sha256'].items():
        if sha((build/'generated'/name).read_bytes())!=digest:raise ValueError('generated source changed')
    package=work/'package';app=package/'switch'/APP;app.mkdir(parents=True)
    data=(build/'switch-0/probe.nro').read_bytes();(app/NRO).write_bytes(data)
    manifest={'schema':1,'application':'Brutal Legend NX - PPC Runtime Probe','nro_path':RELATIVE,
              'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True,timeout=10).strip(),
              'source_worktree_dirty':bool(subprocess.check_output(['git','status','--porcelain=v1'],cwd=ROOT,timeout=10)),
              'build_sources_sha256':runner.inputs(),'packaging_sources_sha256':packaging_hashes(),
              'image':json.loads((ROOT/'tests/switch-horizon/toolchain.json').read_text())['image'],
              'generator_cli_sha256':results['generator_cli_sha256'],'generated_sha256':results['generated_sha256'],
              'input_sha256':results['input_sha256'],'nro_sha256':sha(data),'elf_sha256':sw['sha256']['probe.elf'],
              'host_passed':177,'host_failed':0,'negative_compile_refusals':8,'clean_builds':2,
              'expected_hardware_result':'PASS=38 FAIL=0; entries=10 loads=4 stores=3',
              'hardware_execution':'UNKNOWN','game_code_included':False,'scope':'original generated four-instruction leaf only'}
    (app/'build-info.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
    (app/'SHA256SUMS').write_text(sha(data)+'  '+NRO+'\n')
    shutil.copyfile(ROOT/'LICENSE',app/'LICENSE.txt');shutil.copyfile(ROOT/'tests/horizon-runtime/NOTICE.txt',app/'THIRD_PARTY.txt')
    (app/'README.txt').write_text('BRUTAL LEGEND NX - PPC RUNTIME PROBE (ORIGINAL SYNTHETIC FIXTURE)\n\nCopy switch/ to SD root. New path: sdmc:/'+RELATIVE+'\nRetain the old working native probe in switch/brutal-legend-nx/.\nSafely eject before leaving Hekate USB storage; boot normally and launch the PPC Runtime Probe from hbmenu.\nExpected success: PASS=38 FAIL=0, generated entries=10 checked loads=4 stores=3.\nPlatform, guest memory, actual generated entry, expected value and cleanup each report real checks.\nWait for PLUS (+) to exit. Record the complete screen, launch mode and system/loader versions.\nBUILD VERIFIED; HARDWARE EXECUTION UNKNOWN. No game code, guest XEX loading, Xbox PCR/TLS or full ABI support.\nSource: https://github.com/Wilwous/brutal-legend-nx/tree/research/switch-horizon-feasibility\nExact source and generated-input hashes are in build-info.json.\n')
    validate(package)
    target=work/'brutal-legend-nx-ppc-runtime-probe.zip';archive(package,target)
    repeat=work/'repeat.zip';archive(package,repeat)
    if target.read_bytes()!=repeat.read_bytes():raise ValueError('ZIP reproducibility failed')
    repeat.unlink();row={'nro':str(app/NRO),'zip':str(target),'nro_sha256':sha(data),'zip_sha256':sha(target.read_bytes()),'hardware_execution':'UNKNOWN'}
    (work/'package-result.json').write_text(json.dumps(row,indent=2)+'\n');print(json.dumps(row,indent=2))
if __name__=='__main__':main()
