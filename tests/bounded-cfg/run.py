#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Production XEX codegen, independent CFG reconciliation and bounded execution."""
import argparse,hashlib,json,os,re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run
from fixture import create,cfg
MODES=['valid','zero','prefix','escape','unknown','callback','return','policy','stop-loop','stop-forward','imports','stop-import','long','stop-long','memory','hash','exception']
def sha(p):return cfg.sha(p.read_bytes())
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for n in ['sdk','rexglue','workdir']:ap.add_argument('--'+n,type=Path,required=True)
 ap.add_argument('--sanitize',action='store_true');a=ap.parse_args();sdk=a.sdk.resolve();cli=a.rexglue.resolve();work=a.workdir.resolve()
 if ROOT==work or ROOT in work.parents:ap.error('private output required')
 meta=json.loads((sdk/'fixture-sdk.json').read_text());assert meta['sdk_revision']==cfg.REVISION and (meta['config']=='Sanitized')==a.sanitize and sha(sdk/'lib'/meta['runtime_filename'])==meta['runtime_sha256']
 for k in list(os.environ):
  if k.startswith(('DYLD_','REXSDK','VK_','VULKAN_')):del os.environ[k]
 os.environ['UBSAN_OPTIONS']='print_stacktrace=1';work.mkdir(parents=True,exist_ok=False);records=[]
 def execute(cmd,label):
  rec=run(cmd,work,label,30,True);records.append(rec)
  if rec['timeout'] or rec['exit_code']:raise RuntimeError(label+' failed; private diagnostics retained')
  return rec
 input=work/'input';layout,manifest=create(input);execute([str(cli),'codegen',str(input/'fixture_manifest.toml')],'generation');generated=input/'generated';emitted=sorted([*generated.glob('*.cpp'),*generated.glob('*.h')]);before={f.name:sha(f) for f in emitted}
 execute([str(cli),'codegen',str(input/'fixture_manifest.toml'),'--ignore-stamp'],'regeneration')
 if before!={f.name:sha(f) for f in emitted}:raise ValueError('codegen nondeterminism')
 fresh=work/'repeat';repeat,authority=create(fresh);cfg.validate(manifest,authority);assert (fresh/'fixture.xex').read_bytes()==(input/'fixture.xex').read_bytes()
 source=''.join(f.read_text() for f in generated.glob('fixture_recomp.*.cpp'));definitions=list(re.finditer(r'DEFINE_REX_FUNC\((\w+)\)\s*\{',source));bodies={}
 for i,m in enumerate(definitions):
  pc=layout['entry'] if m[1]=='xstart' else int(m[1].removeprefix('sub_'),16)
  if pc in bodies:raise ValueError('duplicate generated function')
  bodies[pc]=source[m.end():definitions[i+1].start() if i+1<len(definitions) else len(source)]
 if set(bodies)!=set(layout['functions']):raise ValueError('function discovery drift')
 function_map={}
 for f in generated.glob('*.cpp'):
  for addr,name in re.findall(r'\{\s*(0x[0-9A-Fa-f]+),\s*(\w+)\s*\}',f.read_text()):function_map[name]=int(addr,16)
 for pc,size in layout['extents'].items():cfg.reconcile(cfg.decode_range(bytes(layout['code']),int(pc),int(pc)+size,layout['code_base']),bodies[int(pc)],function_map=function_map)
 hooks=[]
 for pc,body in bodies.items():
  rows=cfg.decode_range(bytes(layout['code']),pc,pc+layout['extents'][pc],layout['code_base']);hooks+=cfg.reconcile_hooks(rows,body,[s['pc'] for s in manifest['policy']['sites']])
 points=[s['pc'] for s in manifest['policy']['sites']]
 if sorted(hooks)!=points:raise ValueError('omitted or extra emitted stop site')
 proof=cfg.stop_proof(manifest)
 header='#pragma once\n#include <array>\nextern const rex::PPCImageInfo cfg_image_info;\n'
 def array(ctype,name,items):
  nonlocal header
  header+=f'inline constexpr std::array<{ctype},{len(items)}> {name}{{'+','.join(str(i) for i in items)+'};\n'
 array('unsigned char','image_sha',bytes.fromhex(sha(input/'fixture.xex')));array('unsigned char','manifest_sha',bytes.fromhex(sha(input/'region.json')));array('uint8_t','image_code',layout['code']);array('uint32_t','image_functions',layout['functions']);array('uint32_t','cfg_points',points);array('uint32_t','cfg_roots',manifest['policy']['roots']);array('uint32_t','cfg_imports',manifest['policy']['imports'])
 for n,pc in layout['named'].items():header+=f'inline constexpr uint32_t pc_{n}={pc};\n'
 for n in ['indirect','import_site']:header+=f'inline constexpr uint32_t pc_{n}={layout[n]};\n'
 header+=f'inline auto description(){{return brutal::synthetic::ImageDescription{{{layout["base"]},{layout["entry"]},0x40000,{layout["code_base"]},image_code,image_functions}};}}\n'
 for s in manifest['policy']['sites']:
  array('uint32_t','targets_'+str(s['index']),s['targets']);header+=f'void brutal_cfg_{s["pc"]:X}(){{brutal::synthetic::BoundedCfg::point({s["index"]},{s["pc"]});}}\n'
 kinds={'linear':'linear','branch':'branch','conditional':'conditional','call':'call','indirect_call':'indirect_call','indirect_branch':'indirect_branch','return':'ret'}
 header+=f'inline const std::array<brutal::synthetic::BoundedCfg::Site,{len(points)}> cfg_sites{{{{\n'
 for s in manifest['policy']['sites']:header+='{'+f'{s["index"]},{s["pc"]},{s["owner"]},brutal::synthetic::BoundedCfg::Kind::{kinds[s["kind"]]},brutal::synthetic::BoundedCfg::Action::{s["action"]},{s["condition"]},targets_{s["index"]}'+'},\n'
 header+='}};\n';edges=manifest['policy']['edges'];header+=f'inline const std::array<std::array<uint32_t,2>,{len(edges)}> cfg_edges{{{{'+','.join('std::array<uint32_t,2>{'+str(a)+','+str(b)+'}' for a,b in edges)+'}};\n';(work/'cfg-data.h').write_text(header)
 flags=['-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0' if meta['config']=='Debug' else '-O2','-g','-I'+str(sdk/'include'),'-iquote',str(ROOT),'-DREX_CHECKED_ACCESS_HEADER="runtime/bounded_cfg.h"']
 if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
 names=['-DPPCImageConfig=cfg_image_info','-DPPCFuncMappings=cfg_maps','-Dxstart=cfg_entry','-D__imp__xstart=cfg_imp_entry','-Dfixture_RegisterFunctions=cfg_register'];objects=[]
 for src in sorted(generated.glob('*.cpp')):
  obj=work/(src.stem+'.o');execute(['clang++',*flags,*names,'-I'+str(generated),'-c',str(src),'-o',str(obj)],'build-'+src.stem);objects.append(obj)
 execute(['clang++',*flags,*names,'-I'+str(generated),'-dM','-E','-x','c++',str(generated/'fixture_pch.h'),'-o',str(work/'macros.txt')],'preprocess')
 macros=(work/'macros.txt').read_text()
 for macro in ['REX_LOAD_U32','REX_STORE_U32','REX_LOAD_U64','REX_STORE_U64','REX_RAW_ADDR','REX_CALL_FUNC','REX_CALL_INDIRECT_FUNC']:
  if 'brutal::' not in next(s for s in macros.splitlines() if s.startswith('#define '+macro+'(')):raise ValueError('unchecked macro')
 binary=work/'cfg';execute(['clang++',*flags,'-I'+str(work),str(ROOT/'tests/bounded-cfg/contracts.cpp'),*[str(o) for o in objects],'-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib'),'-o',str(binary)],'build-contracts')
 checks=[];failed=[]
 for mode in MODES:
  rec=run([str(binary),str(input),str(work/('runtime-'+mode)),mode],work,mode,30,True);records.append(rec);observed=[json.loads(line) for line in (work/(mode+'.stdout')).read_text().splitlines() if line.startswith('{')];rec['checks']=observed
  if rec['timeout'] or rec['exit_code'] or len(observed)!=1 or observed[0].get('result')!='PASS':failed.append(mode)
  else:checks+=observed
 findings=[]
 for rec in records:
  for suffix in ['.stdout','.stderr']:
   if re.search(r'ERROR: AddressSanitizer|SUMMARY:.*Sanitizer|runtime error:',(work/(rec['label']+suffix)).read_text(errors='replace')):findings.append(rec['label']+suffix)
 if findings:failed.append('sanitizer')
 observed_edges=sorted({tuple(e[:2]) for c in checks for e in c.get('edges',[])})
 if set(observed_edges)!=set(map(tuple,manifest['policy']['edges'])):failed.append('runtime-edge-coverage')
 for c in checks:
  expected=0 if c['group'] in ['cfg_hash','cfg_policy'] else len(points)
  if len(c.get('stop_hits',[]))!=expected:raise ValueError('missing per-site stop diagnostics')
 stop_hits=[sum(c['stop_hits'][i] for c in checks if c.get('stop_hits')) for i in range(len(points))]
 if not all(stop_hits):failed.append('unobserved-stop-site')
 backedges=[s['pc'] for s in manifest['policy']['sites'] if 'backedge' in s['events']]
 if not all(any(a==pc for a,b in observed_edges) for pc in backedges):failed.append('unobserved-backedge')
 if before!={f.name:sha(f) for f in emitted}:failed.append('generated-files-changed')
 sources=[str(f.relative_to(ROOT)) for folder in ['tests/bounded-cfg','runtime'] for f in (ROOT/folder).glob('*') if f.is_file()]+['tools/bounded_cfg.py','tests/xex-binding/fixture.py','tests/generated-dispatch/fixture.py']
 result={'sdk':meta,'rexglue_sha256':sha(cli),'manifest_sha256':sha(input/'region.json'),'namespace':manifest['namespace'],'codegen_config_sha256':sha(input/'fixture_manifest.toml'),'generated':before,'binary_sha256':sha(binary),'object_sha256':{f.name:sha(f) for f in objects},'proof':proof,'emitted_sites':len(hooks),'runtime_stop_hits':stop_hits,'runtime_edges':observed_edges,'declared_edges':len(manifest['policy']['edges']),'records':records,'checks':checks,'passed_groups':len(checks),'failed_groups':failed,'sanitizer_findings':findings,'fixture_sources_sha256':{f:sha(ROOT/f) for f in sources},'game_code_executed':False}
 (work/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['sdk','generated','object_sha256','records','checks','fixture_sources_sha256','runtime_edges']},indent=2));return bool(failed)
if __name__=='__main__':
 try:sys.exit(main())
 except (RuntimeError,OSError,ValueError,AssertionError) as e:print(str(e),file=sys.stderr);sys.exit(1)
