#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""One private source -> XEX -> production codegen -> compile -> SDK execution workflow."""
import argparse,hashlib,json,os,re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run
from fixture import create
MEMORY=['REX_LOAD_U32','REX_STORE_U32','REX_LOAD_U64','REX_STORE_U64','REX_LOAD_VECTOR','REX_STORE_VECTOR']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main(shutdown=False):
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
  if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed; inspect private diagnostics')
  return r
 execute([str(cli),'--version'],'version-rexglue');execute(['clang++','--version'],'version-compiler')
 input=work/'input';input.mkdir();descriptions={};artifacts={};coverage={};objects=[];header='#pragma once\n#include <array>\n'
 common=['-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0' if meta['config']=='Debug' else '-O2','-g','-I'+str(sdk/'include'),'-iquote',str(ROOT),'-DREX_CHECKED_ACCESS_HEADER="runtime/generated_imports.h"']
 if a.sanitize:common+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
 for name,base in [('a',0x82000000),('b',0x83000000)]:
  p=input/name;layout=create(p,base);descriptions[name]=layout
  execute([str(cli),'codegen',str(p/'fixture_manifest.toml')],'generation-'+name)
  out=p/'generated';emitted=sorted([*out.glob('*.cpp'),*out.glob('*.h')]);before={f.name:sha(f) for f in emitted}
  execute([str(cli),'codegen',str(p/'fixture_manifest.toml'),'--ignore-stamp'],'regeneration-'+name)
  if {f.name:sha(f) for f in emitted}!=before:raise RuntimeError('non-reproducible generated sources')
  body=''.join(f.read_text() for f in out.glob('fixture_recomp.*.cpp'))
  declarations=re.findall(r'^DEFINE_REX_FUNC\(([^)]+)\)',body,re.M)
  if len(declarations)!=3 or len(re.findall(r'^\t// ',body,re.M))!=63:raise RuntimeError('unexpected function/instruction scope')
  counts={m:len(re.findall(r'\b'+m+r'\(',body)) for m in MEMORY}
  if list(counts.values())!=[17,12,2,2,1,1]:raise RuntimeError('memory census drift: '+str(counts))
  for forbidden in ['REX_RAW_ADDR(','REX_PARTIAL_VECTOR_GUARD(','REX_UNIMPLEMENTED(','REX_FATAL(','base +','ppc_trap(','ppc_setjmp(','ppc_longjmp(','GuestPtr']:
   if forbidden in body:raise RuntimeError('unsupported generated path '+forbidden)
  calls=re.findall(r'REX_CALL_FUNC\(([^)]+)\)',body)
  if set(calls)!={f'sub_{base+0x10000:X}',f'sub_{base+0x100b8:X}','__imp__RtlCompareMemoryUlong','__imp__RtlInitializeCriticalSection'} or len(calls)!=4 or body.count('REX_CALL_INDIRECT_FUNC(')!=1:raise RuntimeError('unreviewed call graph')
  coverage[name]={'functions':3,'instructions':63,'memory_operations':35,'instrumented':35,'unsupported':0,'unchecked':0,'macro_counts':counts}
  header+=f'extern const rex::PPCImageInfo image_{name}_info;\n'
  header+=f'inline constexpr std::array<unsigned char,32> image_{name}_sha{{'+','.join(str(v) for v in bytes.fromhex(sha(p/'fixture.xex')))+'};\n'
  header+=f'inline constexpr std::array<uint8_t,{len(layout["code"])}> image_{name}_code{{'+','.join(str(v) for v in layout['code'])+'};\n'
  header+=f'inline auto description_{name}(){{return brutal::synthetic::ImageDescription{{0x{base:X},0x{layout["entry"]:X},0x40000,0x{layout["code_base"]:X},image_{name}_code}};}}\n'
  flags=common+['-I'+str(out),'-DPPCImageConfig=image_'+name+'_info','-DPPCFuncMappings=image_'+name+'_mappings','-Dxstart=image_'+name+'_entry','-D__imp__xstart=image_'+name+'_imp_entry','-Dfixture_RegisterFunctions=image_'+name+'_register']
  # Only namespace global linkage identities; generated files remain byte-identical.
  for src in sorted(out.glob('*.cpp')):
   obj=work/(name+'-'+src.stem+'.o');execute(['clang++',*flags,'-c',str(src),'-o',str(obj)],'build-'+name+'-'+src.stem);objects.append(obj)
  execute(['clang++',*flags,'-dM','-E','-x','c++',str(out/'fixture_pch.h'),'-o',str(work/('macros-'+name+'.txt'))],'preprocess-'+name)
  macros=(work/('macros-'+name+'.txt')).read_text()
  for macro in MEMORY+['REX_CALL_FUNC','REX_CALL_INDIRECT_FUNC']:
   line=next((s for s in macros.splitlines() if s.startswith('#define '+macro+'(')),'')
   if 'brutal::' not in line:raise RuntimeError('unmediated macro '+macro)
  if {f.name:sha(f) for f in emitted}!=before:raise RuntimeError('generated files changed')
  artifacts[name]={'image_sha256':sha(p/'fixture.xex'),'manifest_sha256':sha(p/'fixture_manifest.toml'),'generated':before,'object_sha256':{o.name:sha(o) for o in objects if o.name.startswith(name+'-')}}
 (work/'binding-data.h').write_text(header)
 binary=work/'binding';execute(['clang++',*common,'-I'+str(work),str(ROOT/('tests/image-pointer-shutdown/contracts.cpp' if shutdown else 'tests/xex-binding/contracts.cpp')),*[str(o) for o in objects],'-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib'),'-o',str(binary)],'build-binding')
 checks=[];failures=[]
 modes=['valid','boundaries','permissions','rollback','reentrant','stale','stop-import','stop-adapter','unload-active','repeat','concurrent','initialization','callback'] if shutdown else ['valid','mismatch','negative','permissions','pointer','target','sdk-error','repeat','concurrent','benchmark','copy-benchmark']
 for mode in modes:
  rec=run([str(binary),str(work/('runtime-'+mode)),str(input),mode],work,mode,30,True);records.append(rec)
  rows=[json.loads(s) for s in (work/(mode+'.stdout')).read_text().splitlines() if s.startswith('{')];rec['checks']=rows
  if rec['timeout'] or rec['exit_code'] or len(rows)!=1 or rows[0].get('result')!='PASS':failures.append(mode)
  else:checks+=rows
 rec=run([str(binary),str(work/'runtime-join-failure'),str(input),'join-failure'],work,'join-failure',30,True);records.append(rec)
 if rec['timeout'] or rec['exit_code']!=70 or 'JOIN_TIMEOUT:' not in (work/'join-failure.stderr').read_text(errors='replace'):failures.append('join-failure')
 else:checks.append({'group':'xex_join_failure','result':'PASS','expected_fail_stop_exit':70})
 findings=[]
 for rec in records:
  for suffix in ['.stdout','.stderr']:
   if re.search(r'ERROR: AddressSanitizer|SUMMARY:.*Sanitizer|runtime error:',(work/(rec['label']+suffix)).read_text(errors='replace')):findings.append(rec['label']+suffix)
 if findings:failures.append('sanitizer-findings')
 sources=['runtime/synthetic_xex_binding.h','runtime/generated_access.h','runtime/generated_imports.h','runtime/import_admission.h','runtime/pointer_imports.h','runtime/sdk_checked_memory.h','tests/generated-dispatch/fixture.py']+[str(p.relative_to(ROOT)) for p in (ROOT/'tests/xex-binding').glob('*') if p.is_file()]
 if shutdown:sources+=['runtime/cooperative_stop.h']+[str(p.relative_to(ROOT)) for p in (ROOT/'tests/image-pointer-shutdown').glob('*') if p.is_file()]
 result={'sdk':meta,'rexglue_sha256':sha(cli),'coverage':coverage,'artifacts':artifacts,'binary_sha256':sha(binary),'records':records,'checks':checks,'passed_groups':len(checks),'failed_groups':failures,'sanitizer_findings':findings,'game_loaded':False,'fixture_sources_sha256':{f:sha(ROOT/f) for f in sources}}
 (work/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['sdk','records','artifacts','fixture_sources_sha256']},indent=2));return bool(failures)
if __name__=='__main__':
 try:sys.exit(main())
 except (RuntimeError,OSError,ValueError) as e:print(str(e),file=sys.stderr);sys.exit(1)
