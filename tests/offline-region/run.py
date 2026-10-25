#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original offline SDK-API generation/compile checks. Never invokes generated code."""
import argparse,copy,hashlib,importlib.util,json,os,re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'));import offline_region as off
sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def build_generator(sdk,source,work,meta,sanitize,execute):
 flags=['-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0' if meta['config']=='Debug' else '-O2','-g','-I'+str(sdk/'include'),'-I'+str(source/'thirdparty/inja/third_party/include'),'-iquote',str(ROOT)]
 if sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
 libs=['-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib')];suffix='d' if meta['config']=='Debug' else ''
 binary=work/'generator';execute(['clang++',*flags,str(ROOT/'tests/offline-region/generator.cpp'),*libs,'-lrexcodegen'+suffix,'-ldisasm'+suffix,'-lfmt'+suffix,'-lxxhash'+suffix,'-o',str(binary)],'build-generator');return binary,flags

def generate(bundle,generator,work,flags,execute):
 input=work/'input.json';input.write_text(json.dumps(bundle));input.chmod(0o400);digest=sha(input)
 for label in ['a','b']:execute([str(generator),str(input),str(work/label)],'generate-'+label)
 assert sha(input)==digest
 for name in ['region.inc','ppc_config.h']:
  if (work/'a'/name).read_bytes()!=(work/'b'/name).read_bytes():raise ValueError('nondeterministic production output')
 names=['xstart' if f['start']==bundle['entry'] else 'offline_'+str(f['start']) for f in bundle['functions']]
 wrapper='#ifndef BRUTAL_OFFLINE_COMPILE_ONLY\n#error offline compile-only contract required\n#endif\n#include "ppc_config.h"\n'
 wrapper+='\n'.join('DECLARE_REX_FUNC('+name+');' for name in names)+'\n#include "region.inc"\n'
 (work/'a/offline.cpp').write_text(wrapper)
 checked=['-DREX_CHECKED_ACCESS_HEADER="runtime/bounded_cfg.h"','-DBRUTAL_OFFLINE_COMPILE_ONLY=1','-I'+str(work/'a')]
 execute(['clang++',*flags,*checked,'-dM','-E',str(work/'a/offline.cpp'),'-o',str(work/'macros.txt')],'preprocess')
 audited=off.audit(bundle,(work/'a/region.inc').read_text(),(work/'macros.txt').read_text())
 execute(['clang++',*flags,*checked,'-c',str(work/'a/offline.cpp'),'-o',str(work/'region.o')],'compile-only')
 execute(['nm','-u',str(work/'region.o')],'undefined-symbols')
 execute(['nm',str(work/'region.o')],'object-symbols')
 symbols=(work/'undefined-symbols.stdout').read_text()
 if re.search('GuestToHost|FindPPCFunc|LookupFunction',symbols):raise ValueError('unexpected native dispatch dependency')
 hooks=re.findall(r'brutal_cfg_[0-9A-F]+',symbols)
 if sorted(hooks)!=sorted(h['name'] for h in bundle['hooks']):raise ValueError('compiled hook reference mismatch')
 # Hooks intentionally remain unresolved. No hook implementation, native link,
 # image registry, startup state or execution capability is manufactured.
 audited['compiled_hook_references']=len(hooks);audited['linked']=False;audited['compile_only']=True
 audited['generated_sha256']={n:sha(work/'a'/n) for n in ['region.inc','ppc_config.h','offline.cpp']};audited['object_sha256']=sha(work/'region.o')
 return audited

def fixture(work):
 spec=importlib.util.spec_from_file_location('original_startup',ROOT/'tests/entry-contracts/fixture.py');f=importlib.util.module_from_spec(spec);spec.loader.exec_module(f);layout,_=f.create(work/'fixture')
 # Independently authored no-op in this original analogue; no game words used.
 at=layout['named']['startup']+12-layout['code_base'];layout['code'][at:at+4]=list(bytes.fromhex('60000000'));image=bytearray((work/'fixture/fixture.xex').read_bytes());image[0x11000+at:0x11004+at]=bytes.fromhex('60000000');(work/'fixture/fixture.xex').write_bytes(image)
 ranges=[v for v in layout['ranges'] if v['start'] in [layout['named']['save'],layout['named']['startup']]]
 authority=off.cfg.build(bytes(layout['code']),layout['code_base'],ranges,[layout['entry']],[],{},{layout['boundary']:'exit'},dict(xex_sha256=off.cfg.sha((work/'fixture/fixture.xex').read_bytes()),cli_revision=off.cfg.REVISION,patch_lock_sha256=sha(ROOT/'tools/sdk/residency-lock.json'),codegen_config_sha256='a'*64))
 slices=[]
 for v in sorted(ranges,key=lambda v:v['start']):
  raw=bytes(layout['code'])[v['start']-layout['code_base']:v['end']-layout['code_base']];slices.append(dict(pc=v['start'],bytes=list(raw),sha256=off.cfg.sha(raw)))
 return authority,slices

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for n in ['sdk','source','workdir']:ap.add_argument('--'+n,type=Path,required=True)
 ap.add_argument('--sanitize',action='store_true');a=ap.parse_args();sdk=a.sdk.resolve();source=a.source.resolve();work=a.workdir.resolve()
 if work==ROOT or ROOT in work.parents:ap.error('private output required')
 work.mkdir(parents=True,exist_ok=False);meta=json.loads((sdk/'fixture-sdk.json').read_text());assert sha(sdk/'lib'/meta['runtime_filename'])==meta['runtime_sha256'] and (meta['config']=='Sanitized')==a.sanitize
 for k in list(os.environ):
  if k.startswith(('DYLD_','REXSDK','VK_','VULKAN_')):del os.environ[k]
 os.environ['UBSAN_OPTIONS']='print_stacktrace=1';records=[]
 def execute(cmd,label):
  rec=run(cmd,work,label,30,True);records.append(rec)
  if rec['timeout'] or rec['exit_code']:raise RuntimeError(label+' failed; private diagnostics retained')
 generator,flags=build_generator(sdk,source,work,meta,a.sanitize,execute);authority,slices=fixture(work);bundle=off.prepare(authority,authority,slices,expected_slices=slices);audit=generate(bundle,generator,work,flags,execute)
 checks=[dict(group='offline-production-generation',result='PASS'),dict(group='repeat-generation-identity',result='PASS'),dict(group='checked-stores',result='PASS'),dict(group='compiled-hook-references',result='PASS')]
 for mode in ['missing-range','corrupt-word','rehashed-word','wrong-pc','wrong-boundary','helper','return','hook-conflict','manifest','unsupported-word','ownership-absent','execution-admission','nop-form']:
  m=copy.deepcopy(authority);s=copy.deepcopy(slices);refused=False
  try:
   if mode=='missing-range':s.pop()
   if mode in ['corrupt-word','rehashed-word','unsupported-word']:
    s[0]['bytes'][0]^=1
    if mode=='unsupported-word':s[0]['bytes'][:4]=[0,0,0,0]
    if mode!='corrupt-word':s[0]['sha256']=off.cfg.sha(bytes(s[0]['bytes']))
   if mode=='wrong-pc':s[0]['pc']+=4
   if mode=='wrong-boundary':m['policy']['functions'][0]['end']+=4
   if mode=='helper':next(v for v in m['policy']['sites'] if v['kind']=='call' and v['action']=='execute')['targets'][0]+=4
   if mode=='return':next(v for v in m['policy']['sites'] if v['kind']=='return')['targets'][0]+=4
   if mode=='hook-conflict':m['policy']['sites'][1]['pc']=m['policy']['sites'][0]['pc']
   if mode=='manifest':m['namespace']='0'*64
   if mode=='ownership-absent':assert bundle['runtime_owner_verified'] is False;off.execution_admission(bundle)
   if mode=='nop-form':off.cfg.decode(0x40000000,0x60000000);off.execution_admission(bundle)
   if mode=='execution-admission':off.execution_admission(dict(bundle,runtime_owner_verified=True,loaded=True))
   off.prepare(authority,m,s,expected_slices=s if mode=='unsupported-word' else slices)
  except ValueError:refused=True
  if not refused:raise ValueError('negative accepted: '+mode)
  checks.append(dict(group=mode,result='PASS'))
 for mode in ['missing-store','duplicate-hook','missing-hook','duplicate-hook-call','missing-hook-call','wrong-ea','wrong-scalar','extra-prologue','raw-pointer']:
  text=(work/'a/region.inc').read_text();macros=(work/'macros.txt').read_text()
  if mode=='missing-store':text=text.replace('REX_STORE_U64(','REX_LOAD_U64(',1)
  if mode=='duplicate-hook':text=text.replace(bundle['hooks'][0]['name']+'();',bundle['hooks'][0]['name']+'();\n\t'+bundle['hooks'][0]['name']+'();',1)
  if mode=='missing-hook':text=text.replace(bundle['hooks'][0]['name']+'();','',1)
  if mode=='duplicate-hook-call':text=text.replace('\t'+bundle['hooks'][0]['name']+'();','\t'+bundle['hooks'][0]['name']+'();\n\t'+bundle['hooks'][0]['name']+'();',1)
  if mode=='missing-hook-call':text=text.replace('\t'+bundle['hooks'][0]['name']+'();','',1)
  if mode=='wrong-ea':text=text.replace('ctx.r1.u32 + -56','ctx.r1.u32 + 56',1)
  if mode=='wrong-scalar':text=text.replace('ctx.r7.u64 = ctx.lr;','ctx.r7.u64 = 0;',1)
  if mode=='extra-prologue':text=text.replace('REX_FUNC_PROLOGUE();','REX_FUNC_PROLOGUE();\n\t*(base)=1;',1)
  if mode=='raw-pointer':text=text.replace('REX_STORE_U32(', 'raw_pointer(',1)
  try:off.audit(bundle,text,macros)
  except ValueError:checks.append(dict(group=mode,result='PASS'))
  else:raise ValueError('source mutation accepted: '+mode)
 findings=[rec['label'] for rec in records if any(re.search(r'ERROR: AddressSanitizer|SUMMARY:.*Sanitizer|runtime error:',(work/(rec['label']+suffix)).read_text(errors='replace')) for suffix in ['.stdout','.stderr'])]
 result=dict(sdk=meta,passed_groups=len(checks),failed_groups=findings,checks=checks,records=records,audit=audit,sanitizer_findings=findings,fixture_sources_sha256={n:sha(ROOT/n) for n in ['tools/offline_region.py','tests/offline-region/generator.cpp','tests/offline-region/run.py']},game_code_executed=False)
 (work/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(passed_groups=len(checks),failed_groups=findings,stores=len(audit['stores']),hooks=audit['emitted_hooks'],guest_invocations=0)));return bool(findings)
if __name__=='__main__':sys.exit(main())
