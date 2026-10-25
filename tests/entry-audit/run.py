#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Audit actual original production artifacts, not title code. Private outputs."""
import argparse,copy,json,hashlib,os,re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'));import audit_entry_instrumentation as a
sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for n in ['sdk','input-suite','workdir']:ap.add_argument('--'+n,type=Path,required=True)
 ap.add_argument('--sanitize',action='store_true');args=ap.parse_args();sdk=args.sdk.resolve();inp=args.input_suite.resolve();work=args.workdir.resolve()
 if ROOT==work or ROOT in work.parents:ap.error('private output required')
 work.mkdir(parents=True,exist_ok=False);meta=json.loads((sdk/'fixture-sdk.json').read_text());assert (meta['config']=='Sanitized')==args.sanitize
 sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();assert sha(sdk/'lib'/meta['runtime_filename'])==meta['runtime_sha256'];previous=json.loads((inp/'results.json').read_text());assert previous['sdk']==meta and previous['passed_groups']==41 and not previous['failed_groups']
 for f,h in previous['fixture_sources_sha256'].items():assert sha(ROOT/f)==h
 for f,h in previous['generated'].items():assert sha(inp/'input/generated'/f)==h
 # A second independently authored construction remains the authority.
 import importlib.util
 spec=importlib.util.spec_from_file_location('original_startup',ROOT/'tests/entry-contracts/fixture.py');fixture=importlib.util.module_from_spec(spec);spec.loader.exec_module(fixture);_,authority=fixture.create(work/'authority')
 manifest,metadata,bodies,mappings,table,macros=a.artifacts(inp);a.cfg.validate(manifest,authority);assert a.cfg.sha((inp/'input/fixture.xex').read_bytes())==authority['bindings']['xex_sha256']
 positive=a.audit(authority,manifest,metadata,bodies,mappings,table,macros);assert positive['static_sites']==24 and positive['store_sites']==10 and positive['normal_edges']==21
 flags=['-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0' if meta['config']=='Debug' else '-O2','-g','-I'+str(sdk/'include')]
 if args.sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
 for k in list(os.environ):
  if k.startswith(('DYLD_','REXSDK','VK_','VULKAN_')):del os.environ[k]
 os.environ['UBSAN_OPTIONS']='print_stacktrace=1';records=[];checks=[dict(group='production_source_census',result='PASS')];failed=[]
 def execute(cmd,label):
  r=run(cmd,work,label,30,True);records.append(r)
  if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed')
  return r
 source=work/'policy.cpp';source.write_text(a.policy_source(authority));execute(['clang++',*flags,'-S','-emit-llvm',str(source),'-o',str(work/'policy.ll')],'compile-policy-ir');execute(['clang++',*flags,'-c',str(source),'-o',str(work/'policy.o')],'compile-policy-object');ir=(work/'policy.ll').read_text();a.llvm_policy(ir,authority);checks.append(dict(group='compiled_policy_identity',result='PASS'))
 base=metadata['base'];sections=[dict(address=base+i*65536,size=65536,flags=f) for i,f in enumerate([0x40000040,0x60000020,0x40000040,0xc0000040])];original_context=dict(stack_owner='original-owner',image_lease='original-lease',r1=0x10000);identity=dict(image=authority['bindings']['xex_sha256'],code=authority['bindings']['image_code_sha256'],owner_size=0x40000)
 entry=metadata['entry'];store=metadata['stores'][0];owner=next(s['owner'] for s in manifest['policy']['sites'] if s['pc']==store);order=[s for s in manifest['policy']['sites'] if s['owner']==owner];caller_name=next(n for n,pc in mappings.items() if pc==metadata['named']['save'])
 for variant in ['omitted-store-hook','duplicate-pc','wrong-word','wrong-image-hash','namespace','helper','return','stack-owner','startup-register','out-image','oversized-owner-claim','overlap','exception','missing-stop','compiled-table','raw-alias','raw-pointer']:
  m=copy.deepcopy(manifest);data=copy.deepcopy(metadata);body=dict(bodies);mapping=dict(mappings);t=table;denied=False
  try:
   if variant=='omitted-store-hook':body[owner]=body[owner].replace('\tbrutal_cfg_'+format(store,'X')+'();','',1)
   if variant=='duplicate-pc':m['policy']['sites'][1]['pc']=m['policy']['sites'][0]['pc']
   if variant=='wrong-word':data['code'][0]^=1
   if variant=='wrong-image-hash':a.identities(identity,dict(identity,image='0'*64))
   if variant=='namespace':m['namespace']='0'*64
   if variant=='helper':mapping[caller_name]^=4
   if variant=='return':body[entry]=body[entry].replace('ctx.lr = 0x'+format(entry+8,'X')+';','ctx.lr = 0x'+format(entry+12,'X')+';',1)
   if variant=='stack-owner':a.context(original_context,dict(original_context,stack_owner='foreign'))
   if variant=='startup-register':a.context(original_context,dict(original_context,r1=0))
   if variant=='out-image':a.layout(base,0x40000,sections,[(base+0x40000,4,'write')])
   if variant=='oversized-owner-claim':a.identities(identity,dict(identity,owner_size=0x60000))
   if variant=='overlap':changed=copy.deepcopy(sections);changed[-1]['address']=changed[-2]['address'];a.layout(base,0x40000,changed)
   if variant=='exception':m['policy']['denied_transfers'].remove('exception')
   if variant=='missing-stop':body[entry]=body[entry].replace('\tbrutal_cfg_'+format(entry,'X')+'();','',1)
   if variant=='compiled-table':t=t.replace('BoundedCfg::point(0,','BoundedCfg::point(1,',1)
   if variant=='raw-pointer':body[owner]=body[owner].replace('\t// ','\t*(unsigned*)0x1234=7;\n\t// ',1)
   if variant=='raw-alias':body[owner]=body[owner].replace('\t// ', '\tauto* alias=base; alias[0]=0;\n\t// ',1)
   a.audit(authority,m,data,body,mapping,t,macros)
  except (ValueError,KeyError):denied=True
  if not denied:raise ValueError('mutation accepted: '+variant)
  checks.append(dict(group=variant,result='PASS',outcome='static refusal; no mutated candidate executed'))
 try:a.llvm_policy(ir,dict(authority,namespace='0'*64))
 except ValueError:checks.append(dict(group='compiled-namespace',result='PASS'))
 else:raise ValueError('compiled namespace mismatch accepted')
 libs=['-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib')]
 for name,path,argv in [('context','tests/entry-audit/context_probe.cpp',[str(inp/'input'),str(work/'context-runtime')]),('loader','tools/entry-audit/load_probe.cpp',[str(inp/'input'),'fixture.xex',str(work/'loader-runtime')])]:
  binary=work/name;execute(['clang++',*flags,str(ROOT/path),*libs,'-o',str(binary)],'build-'+name)
  rec=run([str(binary),*argv],work,name,30,True);records.append(rec);rows=[json.loads(line) for line in (work/(name+'.stdout')).read_text().splitlines() if line.startswith('{')]
  valid=not rec['timeout'] and rec['exit_code']==0 and len(rows)==1
  if valid and name=='context':valid=rows[0]['result']=='PASS' and not rows[0]['title_contract_verified']
  if valid and name=='loader':valid=rows[0]['load_success'] and rows[0]['image_virtual_pages_reclaimed'] and rows[0]['guest_functions_invoked']==0
  if valid:checks.append(dict(group=name,result='PASS',observation=rows[0]))
  else:failed.append(name) # Preserve the failed strict oracle; do not relabel as success.
 # Independent representation diagnostic, not a passing startup-value oracle.
 representation=work/'context-initialization';execute(['clang++',*flags,str(ROOT/'tests/entry-audit/context_initialization.cpp'),*libs,'-o',str(representation)],'build-initialization');execute([str(representation)],'initialization')
 representation_rows=[json.loads(line) for line in (work/'initialization.stdout').read_text().splitlines() if line.startswith('{')];assert len(representation_rows)==1
 findings=[]
 for rec in records:
  for suffix in ['.stdout','.stderr']:
   if re.search(r'ERROR: AddressSanitizer|SUMMARY:.*Sanitizer|runtime error:',(work/(rec['label']+suffix)).read_text(errors='replace')):findings.append(rec['label']+suffix)
 inputs=['tools/audit_entry_instrumentation.py','tools/entry-audit/load_probe.cpp','tests/entry-audit/run.py','tests/entry-audit/context_probe.cpp','tests/entry-audit/test_audit.py','tests/entry-audit/context_initialization.cpp','tests/entry-audit/README.md']
 result=dict(sdk=meta,records=records,checks=checks,passed_groups=len(checks),failed_groups=failed+(['sanitizer'] if findings else []),sanitizer_findings=findings,static_census=positive,policy_object_sha256=sha(work/'policy.o'),source_sha256=sha(source),fixture_sources_sha256={f:sha(ROOT/f) for f in inputs},context_representation=representation_rows[0],private_code_executed=False)
 (work/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['sdk','records','checks','fixture_sources_sha256']},indent=2));return bool(failed or findings)
if __name__=='__main__':
 try:sys.exit(main())
 except (RuntimeError,ValueError,AssertionError,OSError) as e:print(str(e),file=sys.stderr);sys.exit(1)
