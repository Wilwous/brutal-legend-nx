#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Real generator -> unchanged caller TU -> checked dispatch -> actual SDK."""
import argparse,hashlib,json,os,re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run
from fixture import create
MEMORY=['REX_LOAD_U32','REX_STORE_U32','REX_LOAD_U64','REX_STORE_U64','REX_LOAD_VECTOR','REX_STORE_VECTOR']

def census(body,layout):
 parts=re.split(r'^DEFINE_REX_FUNC\(([^)]+)\) \{',body,flags=re.M)
 bodies=dict(zip(parts[1::2],parts[2::2]))
 names={n:f'caller_{layout["analysis_base"]+o:X}' for n,o in layout['functions'].items()}
 if set(bodies)!=set(names.values()):raise RuntimeError('generated definitions drift')
 if len(re.findall(r'^\t// ',body,re.M))!=layout['instructions']:raise RuntimeError('instruction count drift')
 reachable=''.join(bodies[names[n]] for n in ['entry','echo'])
 for n in ['compare','critical']:
  if re.findall(r'^\t// (.*)$',bodies[names[n]],re.M)!=['blr ']:raise RuntimeError('gate body drift')
 # Every generated instruction is classified: scalar/full-vector guest-memory
 # instructions must have exactly one checked macro; register SIMD accesses
 # address PPCContext only. Reject any new guest helper until reviewed.
 comments=re.findall(r'^\t// (\S+)',reachable,re.M)
 guest_ops=[s for s in comments if s in ['stwu','stw','std','lwz','ld','lvx','stvx']]
 allowed={'stwu','stw','std','lwz','ld','lvx','stvx','mflr','mr','addi','vaddubm','bl','mtctr','bctrl','cmpwi','bne','li','mtlr','blr','xor'}
 if set(comments)-allowed:raise RuntimeError('unreviewed reachable opcode')
 counts={n:len(re.findall(r'\b'+n+r'\(',reachable)) for n in MEMORY}
 if list(counts.values())!=[15,9,2,2,1,1] or len(guest_ops)!=30:raise RuntimeError('reachable memory census drift')
 for forbidden in ['REX_RAW_ADDR(', 'REX_PARTIAL_VECTOR_GUARD(', 'base +','REX_FATAL(','REX_UNIMPLEMENTED(','ppc_trap(','ppc_setjmp(','ppc_longjmp(','GuestPtr','volatile ', 'reinterpret_cast']:
  if forbidden in reachable:raise RuntimeError('unmediated reachable helper: '+forbidden)
 calls=re.findall(r'REX_CALL_FUNC\(([^)]+)\)',reachable)
 if calls!=[names['echo'],names['compare'],names['critical']] or reachable.count('REX_CALL_INDIRECT_FUNC(')!=1:raise RuntimeError('call graph drift')
 return {'scope':['entry','echo'],'static_memory_operations':30,'instrumented':30,'unsupported':0,'unchecked':0,'percent':100,'macro_counts':counts,'direct_edges':calls,'indirect_edge':'explicit NtQueryVirtualMemory thunk only; other targets denied','excluded':'separate negative roots, SDK internals, host loader, private game output'}

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for n in ['sdk','rexglue','workdir']:ap.add_argument('--'+n,type=Path,required=True)
 ap.add_argument('--sanitize',action='store_true');a=ap.parse_args();sdk=a.sdk.resolve();cli=a.rexglue.resolve();work=a.workdir.resolve()
 if work==ROOT or ROOT in work.parents:ap.error('private output required')
 meta=json.loads((sdk/'fixture-sdk.json').read_text())
 if meta['sdk_revision']!='f5337cdc947ff6d4c4196737e2c807a48f2a1fc2' or (meta['config']=='Sanitized')!=a.sanitize:raise RuntimeError('SDK mismatch')
 if hashlib.sha256((sdk/'lib'/meta['runtime_filename']).read_bytes()).hexdigest()!=meta['runtime_sha256']:raise RuntimeError('runtime mismatch')
 for k in list(os.environ):
  if k.startswith(('DYLD_','REXSDK','VK_','VULKAN_')):del os.environ[k]
 os.environ['UBSAN_OPTIONS']='print_stacktrace=1';work.mkdir(parents=True,exist_ok=False);layout=create(work/'input');out=work/'generated';out.mkdir();records=[]
 def execute(cmd,label):
  r=run(cmd,work,label,30,True);records.append(r)
  if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed; private diagnostics retained')
  return r
 execute([str(cli),'recompile-tests','--bin-dir',str(work/'input'),'--asm-dir',str(work/'input'),'--output',str(out)],'generation')
 generated=out/'ppc_test_functions.cpp';coverage=census(generated.read_text(),layout);digest=hashlib.sha256(generated.read_bytes()).hexdigest()
 if digest!='b0cb27072f3681e1927412ac758ac28d54f24aca75495d36837ea5ad94061e82':raise RuntimeError('reviewed generated TU fingerprint drift')
 flags=['-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0' if meta['config']=='Debug' else '-O2','-g','-I'+str(sdk/'include'),'-I'+str(out),'-I'+str(work/'input'),'-iquote',str(ROOT),'-DREX_CHECKED_ACCESS_HEADER="runtime/generated_imports.h"']
 if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
 binary=work/'caller';execute(['clang++',*flags,str(ROOT/'tests/generated-dispatch/contracts.cpp'),str(generated),'-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib'),'-o',str(binary)],'build-caller')
 checks=[];failures=[]
 for mode in ['valid','invalid','errors','policy','permissions','freed','uncommitted','checked','unsupported','gates','lifetime','concurrent','repeat']:
  r=run([str(binary),str(work/('runtime-'+mode)),str(work/'input'),mode],work,mode,30,True);records.append(r)
  rows=[json.loads(s) for s in (work/(mode+'.stdout')).read_text().splitlines() if s.startswith('{')];r['checks']=rows
  if r['timeout'] or r['exit_code'] or len(rows)!=1 or rows[0].get('result')!='PASS':failures.append(mode)
  else:checks+=rows
 execute(['clang++',*flags,'-dM','-E','-x','c++',str(out/'ppc_config.h'),'-o',str(work/'macros.txt')],'preprocess-caller')
 macros=(work/'macros.txt').read_text()
 for name in MEMORY+['REX_RAW_ADDR','REX_PARTIAL_VECTOR_GUARD','REX_CALL_FUNC','REX_CALL_INDIRECT_FUNC']:
  line=next((s for s in macros.splitlines() if s.startswith('#define '+name+'(')),'')
  if not line or 'base +' in line or not any(v in line for v in ['brutal::','brutal_reject']):raise RuntimeError('unchecked macro '+name)
 if hashlib.sha256(generated.read_bytes()).hexdigest()!=digest:raise RuntimeError('generated code changed')
 findings=[]
 for rec in records:
  for suffix in ['.stdout','.stderr']:
   log=work/(rec['label']+suffix)
   if re.search(r'ERROR: AddressSanitizer|SUMMARY:.*Sanitizer|runtime error:',log.read_text(errors='replace')):findings.append(rec['label']+suffix)
 if findings:failures.append('sanitizer-findings')
 paths=['runtime/generated_imports.h','runtime/generated_access.h','runtime/import_admission.h','runtime/pointer_imports.h','runtime/sdk_checked_memory.h','tests/tls-import/fixture.h','tests/tls-import/catalog.h','tests/image-runtime/xex-fixture.h']+[str(p.relative_to(ROOT)) for p in (ROOT/'tests/generated-dispatch').glob('*') if p.is_file()]
 result={'sdk':meta,'rexglue_sha256':hashlib.sha256(cli.read_bytes()).hexdigest(),'generated_sha256':digest,'program_sha256':hashlib.sha256((work/'input/caller.bin').read_bytes()).hexdigest(),'instructions':layout['instructions'],'functions':len(layout['functions']),'coverage':coverage,'fixture_sources_sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths},'checks':checks,'records':records,'passed_groups':len(checks),'failed_groups':failures,'game_loaded':False,'sanitizer_findings':findings,'timeouts':sum(r['timeout'] for r in records),'unexpected_signals':sum(r['exit_code']<0 for r in records),'expected_native_faults':0}
 (work/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['sdk','records','fixture_sources_sha256']},indent=2));return bool(failures)
if __name__=='__main__':
 try:sys.exit(main())
 except (RuntimeError,ValueError,OSError) as e:print(str(e),file=sys.stderr);sys.exit(1)
