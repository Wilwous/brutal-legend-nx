#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Exact offline code slices, not an image loader or execution capability.

Caller supplies a separately frozen authority and exact bytes. Reports and all
input/output code remain private. Production generation is done by SDK APIs.
"""
import copy,re,struct
import bounded_cfg as cfg
class OfflineOnly(ValueError):pass
def execution_admission(*args,**kwargs):raise OfflineOnly('offline artifacts never authorize guest execution')
def prepare(authority,candidate,slices,*,expected_slices):
 cfg.validate(candidate,authority);p=authority['policy'];cfg.stop_proof(authority)
 if cfg.canonical(slices)!=cfg.canonical(expected_slices):raise ValueError('bytes differ from independent frozen extraction')
 if p['imports'] or len(p['roots'])!=1:raise ValueError('unsupported offline imports/roots')
 functions=p['functions'];ordered=sorted(slices,key=lambda s:s['pc'])
 if len(ordered)!=len(functions):raise ValueError('missing or extra instruction ranges')
 rows=[]
 for f,s in zip(functions,ordered):
  raw=bytes(s['bytes'])
  if s['pc']!=f['start'] or len(raw)!=f['end']-f['start']:raise ValueError('function boundary identity')
  decoded=cfg.decode_range(raw,f['start'],f['end'],f['start'])
  if cfg.sha(raw)!=s['sha256']:raise ValueError('corrupt instruction words')
  rows+=decoded
 if len(rows)!=len(p['sites']):raise ValueError('instruction/site count')
 bypc={r['pc']:r for r in rows};sites={s['pc']:s for s in p['sites']}
 # Reconstruct normal targets/returns from independently decoded words. Never
 # infer targets from emitted source, function names or comment ordinals.
 returns={f['start']:[] for f in functions};calls=[]
 for s in p['sites']:
  r=bypc[s['pc']]
  if r['kind']!=s['kind']:raise ValueError('instruction kind differs from frozen policy')
  if s['phase']!='before':raise ValueError('unsupported hook phase')
  if s['action']!='execute':
   if s['action']!='exit' or r['kind']!='call' or s['targets']:raise ValueError('unsupported offline boundary')
  elif r['kind']=='call':
   if s['targets']!=[r['target']] or r['target'] not in returns:raise ValueError('helper identity')
   returns[r['target']].append(r['pc']+4);calls.append(dict(owner=s['owner'],pc=r['pc'],target=r['target']))
  elif r['kind'] not in ['linear','return']:raise ValueError('unsupported offline transfer')
  elif r['kind']=='linear' and r['word']>>26 not in [14,15,31,36,37,62] and r['word']!=0x60000000:raise ValueError('unsupported offline scalar form')
 for s in p['sites']:
  if s['kind']=='return' and s['targets']!=sorted(set(returns[s['owner']])):raise ValueError('return identity')
 hooks=[dict(pc=s['pc'],name='brutal_cfg_'+format(s['pc'],'X'),exit=s['action']=='exit') for s in p['sites']]
 return dict(offline_only=True,runtime_owner_verified=False,manifest=copy.deepcopy(authority),entry=p['roots'][0],slices=ordered,functions=functions,blocks=p['blocks'],calls=calls,hooks=hooks,rows=rows)
def audit(bundle,source,macros):
 manifest=bundle['manifest'];p=manifest['policy'];cfg.validate(manifest,manifest)
 declarations=re.findall(r'^extern void (brutal_cfg_[0-9A-F]+)\(\);$',source,re.M)
 if sorted(declarations)!=sorted(h['name'] for h in bundle['hooks']):raise ValueError('hook declaration identities')
 matches=list(re.finditer(r'DEFINE_REX_FUNC\((\w+)\)\s*\{',source));names={('xstart' if f['start']==bundle['entry'] else 'offline_'+str(f['start'])):f['start'] for f in p['functions']}
 if len(matches)!=len(names) or {m[1] for m in matches}!=set(names):raise ValueError('generated function identity')
 if re.sub(r'^extern void brutal_cfg_[0-9A-F]+\(\);\s*','',source[:matches[0].start()],flags=re.M).strip():raise ValueError('unexpected translation-unit prelude')
 bodies={names[m[1]]:source[m.end():matches[i+1].start() if i+1<len(matches) else len(source)] for i,m in enumerate(matches)}
 mapping=names;stores=[];hooks=[];unreachable=[]
 for f in p['functions']:
  rows=[r for r in bundle['rows'] if f['start']<=r['pc']<f['end']];body=bodies[f['start']]
  diagnostics=re.findall(r'^\t// ERROR:.*$',body,re.M)
  if diagnostics and not any(s['owner']==f['start'] and s['action']=='exit' for s in p['sites']):raise ValueError('unexpected emitter diagnostic')
  view=re.sub(r'^\t// ERROR:.*\n','',body,flags=re.M) # Read-only audit view; never rewrite output.
  pieces=re.split(r'^\t// .+$',view,flags=re.M);parts=pieces[1:]
  prologue=re.sub(r'\s+','',pieces[0])
  if prologue not in ['REX_FUNC_PROLOGUE();','REX_FUNC_PROLOGUE();uint32_tea{};']:raise ValueError('unexpected prologue')
  if len(parts)!=len(rows):raise ValueError('generated instruction count')
  cfg.reconcile_hooks(rows,view,[s['pc'] for s in p['sites']]);hooks += [r['pc'] for r in rows]
  for r,fragment in zip(rows,parts):
   s=next(s for s in p['sites'] if s['pc']==r['pc']);prefix='\tbrutal_cfg_'+format(r['pc'],'X')+'();'
   if not fragment.lstrip('\n').startswith(prefix):raise ValueError('hook must precede every operation')
   if s['action']=='exit':
    if not re.match(r'\s*'+re.escape(prefix)+r'\s*return;',fragment):raise ValueError('boundary lacks unconditional return')
    # Production emitter may retain an unreachable unknown-call diagnostic.
    tail=fragment.split('return;',1)[1]
    if len(diagnostics)!=1 or 'REX_FATAL(' not in tail or 'REX_CALL_' in tail or 'REX_STORE_' in tail:raise ValueError('unexpected boundary residue')
    unreachable.append(r['pc']);continue
   cfg.reconcile([r],'\t// '+r['text']+'\n'+fragment,complete=True,function_map=mapping)
   if any(t in fragment for t in ['base +','REX_FATAL','REX_UNIMPLEMENTED','reinterpret_cast','REX_RAW_','TranslateVirtual','ppc_trap','ppc_setjmp']):raise ValueError('unsafe generated path')
   if r['memory']:
    m=r['memory'];macro='REX_STORE_U'+str(m['width']*8)
    if not m['write'] or fragment.count(macro+'(')!=1:raise ValueError('store mediation')
    # Effective addresses must retain base register, signed displacement and
    # truncation to guest u32. Value casts differ for u32/u64; inspect both.
    flat=re.sub(r'\s+','',fragment);base='ctx.r'+str(m['ra'])+'.u32';value='ctx.r'+str(m['rt'])+'.u'+str(m['width']*8)
    expected=(f'ea={m["offset"]}+{base};{macro}(ea,{value});{base}=ea;' if m['update'] else f'{macro}({base}+{m["offset"]},{value});')
    if expected not in flat:raise ValueError('store operand identity')
    stores.append(dict(pc=r['pc'],**m))
   else:
    w=r['word'];op=w>>26;rt=(w>>21)&31;ra=(w>>16)&31;imm=cfg.signed(w&65535,16)
    if r['kind']=='call':expected=f'ctx.lr=0x{r["pc"]+4:X};REX_CALL_FUNC('+next(n for n,pc in mapping.items() if pc==r['target'])+');'
    elif r['kind']=='return':expected='return;'
    elif w==0x60000000:expected=''
    elif op in [14,15]:
     value=imm*(65536 if op==15 else 1);expected=f'ctx.r{rt}.s64='+((f'ctx.r{ra}.s64+' if ra else '')+str(value))+';'
    elif r['text'].startswith('mflr '):expected=f'ctx.r{rt}.u64=ctx.lr;'
    elif r['text'].startswith('mr '):expected=f'ctx.r{ra}.u64=ctx.r{rt}.u64;'
    else:raise ValueError('unsupported scalar semantics')
   clean=re.sub(r'^extern void brutal_cfg_[0-9A-F]+\(\);\s*','',fragment,flags=re.M)
   clean=re.sub(r'\s+','',clean).rstrip('}')
   if clean!=re.sub(r'\s+','',prefix)+expected:raise ValueError('extra/mismatched scalar or memory operation')
 for name in ['REX_STORE_U32','REX_STORE_U64']:
  line=next((l for l in macros.splitlines() if l.startswith('#define '+name+'(')), '')
  if 'brutal::instrumentation::store<' not in line or 'base +' in line:raise ValueError('unchecked macro')
 for name,target in [('REX_CALL_FUNC','BoundedCfg::direct'),('REX_CALL_INDIRECT_FUNC','BoundedCfg::indirect')]:
  line=next((l for l in macros.splitlines() if l.startswith('#define '+name+'(')), '')
  if target not in line:raise ValueError('unverified call fallback')
 if len(re.findall(r'REX_STORE_U(?:32|64)\(',source))!=len(stores):raise ValueError('extra/missing store')
 if sorted(hooks)!=[s['pc'] for s in p['sites']]:raise ValueError('hook census')
 return dict(stores=stores,requested_hooks=len(p['sites']),emitted_hooks=len(hooks),verified_hook_references=len(hooks),boundary_unreachable_residue=unreachable,offline_only=True,runtime_owner_verified=False,runtime_observed=False,execution_authorized=False)
