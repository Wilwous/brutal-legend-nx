#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original loop XEX -> production midasm hooks -> checked SDK execution."""
import argparse,hashlib,json,os,re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run
from fixture import create
from audit import audit
MODES=['natural','finite-stop','infinite-stop','nested-stop','memory-stop','long-stop','entry-stop','import-stop','adapter-stop','unload-import','concurrent','repeat','latency','error','callback','initialization','policy','benchmark','request-race']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for n in ['sdk','rexglue','workdir']:ap.add_argument('--'+n,type=Path,required=True)
 ap.add_argument('--sanitize',action='store_true');a=ap.parse_args();sdk=a.sdk.resolve();cli=a.rexglue.resolve();work=a.workdir.resolve()
 if work==ROOT or ROOT in work.parents:ap.error('private output required')
 meta=json.loads((sdk/'fixture-sdk.json').read_text())
 if meta['sdk_revision']!='f5337cdc947ff6d4c4196737e2c807a48f2a1fc2' or (meta['config']=='Sanitized')!=a.sanitize or sha(sdk/'lib'/meta['runtime_filename'])!=meta['runtime_sha256']:raise RuntimeError('SDK mismatch')
 for k in list(os.environ):
  if k.startswith(('DYLD_','REXSDK','VK_','VULKAN_')):del os.environ[k]
 os.environ['UBSAN_OPTIONS']='print_stacktrace=1';work.mkdir(parents=True,exist_ok=False);records=[]
 def execute(cmd,label):
  r=run(cmd,work,label,30,True);records.append(r)
  if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed; private diagnostics retained')
 execute([str(cli),'--version'],'version-rexglue');execute(['clang++','--version'],'version-compiler')
 input=work/'input';input.mkdir();artifacts={};coverage={};objects=[];header='#pragma once\n#include <array>\n'
 common=['-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0' if meta['config']=='Debug' else '-O2','-g','-I'+str(sdk/'include'),'-iquote',str(ROOT),'-DREX_CHECKED_ACCESS_HEADER="runtime/generated_stop.h"']
 if a.sanitize:common+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
 for name,base,instrument in [('a',0x82000000,True),('b',0x83000000,False)]:
  p=input/name;layout=create(p,base,instrument);execute([str(cli),'codegen',str(p/'fixture_manifest.toml')],'generation-'+name);out=p/'generated'
  emitted=sorted([*out.glob('*.cpp'),*out.glob('*.h')]);before={f.name:sha(f) for f in emitted}
  execute([str(cli),'codegen',str(p/'fixture_manifest.toml'),'--ignore-stamp'],'regeneration-'+name)
  if {f.name:sha(f) for f in emitted}!=before:raise RuntimeError('non-reproducible generation')
  body=''.join(f.read_text() for f in out.glob('fixture_recomp.*.cpp'));coverage[name]=audit(layout,body)
  header+=f'extern const rex::PPCImageInfo image_{name}_info;\n'
  def array(ctype,key,data):
   nonlocal header
   header+=f'inline constexpr std::array<{ctype},{len(data)}> {name}_{key}{{'+','.join(str(v) for v in data)+'};\n'
  array('unsigned char','sha',list(bytes.fromhex(sha(p/'fixture.xex'))));array('uint8_t','code',layout['code']);array('uint32_t','functions',layout['functions']);array('uint32_t','sites',list(map(int,layout['sites'])));array('uint32_t','backedges',layout['backedges'])
  for key,pc in layout['named'].items():header+=f'inline constexpr uint32_t {name}_{key}={pc};\n'
  header+=f'inline auto description_{name}(){{return brutal::synthetic::ImageDescription{{{base},{layout["entry"]},0x40000,{layout["code_base"]},{name}_code,{name}_functions}};}}\n'
  if instrument:
   for pc in map(int,layout['sites']):header+=f'void brutal_point_{pc:X}(){{brutal::synthetic::GeneratedStop::poll({pc});}}\n'
  flags=common+['-I'+str(out),'-DPPCImageConfig=image_'+name+'_info','-DPPCFuncMappings=image_'+name+'_mappings','-Dxstart=image_'+name+'_entry','-D__imp__xstart=image_'+name+'_imp_entry','-Dfixture_RegisterFunctions=image_'+name+'_register']
  for src in sorted(out.glob('*.cpp')):
   obj=work/(name+'-'+src.stem+'.o');execute(['clang++',*flags,'-c',str(src),'-o',str(obj)],'build-'+name+'-'+src.stem);objects.append(obj)
  execute(['clang++',*flags,'-dM','-E','-x','c++',str(out/'fixture_pch.h'),'-o',str(work/('macros-'+name+'.txt'))],'preprocess-'+name)
  macros=(work/('macros-'+name+'.txt')).read_text()
  for macro in ['REX_LOAD_U32','REX_STORE_U32','REX_LOAD_U64','REX_STORE_U64','REX_LOAD_VECTOR','REX_STORE_VECTOR','REX_CALL_FUNC','REX_CALL_INDIRECT_FUNC']:
   if 'brutal::' not in next(s for s in macros.splitlines() if s.startswith('#define '+macro+'(')):raise RuntimeError('unchecked seam')
  if {f.name:sha(f) for f in emitted}!=before:raise RuntimeError('emitted files modified')
  artifacts[name]={'image_sha256':sha(p/'fixture.xex'),'manifest_sha256':sha(p/'fixture_manifest.toml'),'layout_sha256':sha(p/'loop.json'),'generated':before,'objects':{o.name:sha(o) for o in objects if o.name.startswith(name+'-')}}
 (work/'binding-data.h').write_text(header);binary=work/'loops'
 execute(['clang++',*common,'-I'+str(work),str(ROOT/'tests/generated-loop-stop/contracts.cpp'),*[str(o) for o in objects],'-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib'),'-o',str(binary)],'build-loops')
 checks=[];failures=[]
 for mode in MODES:
  r=run([str(binary),str(work/('runtime-'+mode)),str(input),mode],work,mode,30,True);records.append(r);rows=[json.loads(s) for s in (work/(mode+'.stdout')).read_text().splitlines() if s.startswith('{')];r['checks']=rows
  if r['timeout'] or r['exit_code'] or len(rows)!=1 or rows[0].get('result')!='PASS':failures.append(mode)
  else:checks+=rows
 r=run([str(binary),str(work/'runtime-join-failure'),str(input),'join-failure'],work,'join-failure',30,True);records.append(r)
 if r['timeout'] or r['exit_code']!=70 or 'JOIN_TIMEOUT:' not in (work/'join-failure.stderr').read_text():failures.append('join-failure')
 else:checks.append({'group':'loop_join_failure','result':'PASS','expected_fail_stop_exit':70})
 findings=[]
 for rec in records:
  for suffix in ['.stdout','.stderr']:
   if re.search(r'ERROR: AddressSanitizer|SUMMARY:.*Sanitizer|runtime error:',(work/(rec['label']+suffix)).read_text(errors='replace')):findings.append(rec['label']+suffix)
 if findings:failures.append('sanitizer')
 hits=[sum(c.get('backedge_hits',[0]*6)[i] for c in checks) for i in range(6)]
 if not all(hits):failures.append('runtime-loop-coverage')
 sources=[str(p.relative_to(ROOT)) for d in ['tests/generated-loop-stop','runtime'] for p in (ROOT/d).glob('*') if p.is_file()]+['tests/xex-binding/fixture.py','tests/generated-dispatch/fixture.py','tests/xex-binding/workflow.py']
 result={'sdk':meta,'rexglue_sha256':sha(cli),'coverage':coverage,'runtime_backedge_hits':hits,'artifacts':artifacts,'binary_sha256':sha(binary),'records':records,'checks':checks,'passed_groups':len(checks),'failed_groups':failures,'sanitizer_findings':findings,'game_loaded':False,'fixture_sources_sha256':{f:sha(ROOT/f) for f in sources}}
 (work/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['sdk','records','artifacts','fixture_sources_sha256','checks']},indent=2));return bool(failures)
if __name__=='__main__':
 try:sys.exit(main())
 except (RuntimeError,OSError,ValueError) as e:print(str(e),file=sys.stderr);sys.exit(1)
