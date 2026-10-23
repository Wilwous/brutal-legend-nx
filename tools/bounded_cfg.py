#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Small fail-closed PPC/CFG manifest builder for explicitly supplied ranges.

No discovery/whole-title engine. Bytes/PCs come from a separately identified image
range, never C++ ordinals. Detailed proprietary products belong outside Git.
"""
import hashlib,json,re,struct

REVISION='f5337cdc947ff6d4c4196737e2c807a48f2a1fc2'
EVENTS={'entry','block','branch','join','backedge','call','return','pre-import','post-import',
        'memory','stop','exit','refusal','long-path'}
def canonical(value): return json.dumps(value,sort_keys=True,separators=(',',':')).encode()
def sha(value): return hashlib.sha256(value).hexdigest()
def signed(value,bits): return value-(1<<bits) if value&(1<<(bits-1)) else value

def decode(pc,w):
 if pc&3 or not 0<=pc<=0xfffffffc or not 0<=w<=0xffffffff:raise ValueError('instruction identity')
 op=w>>26;rt=(w>>21)&31;ra=(w>>16)&31;rb=(w>>11)&31;imm=signed(w&65535,16)
 r={'pc':pc,'word':w,'kind':'linear','memory':None}
 if op in [18,16]:
  bits=26 if op==18 else 16;delta=signed(w&((1<<bits)-4),bits);target=(delta if w&2 else pc+delta)
  if not 0<=target<=0xfffffffc:raise ValueError('branch wrap')
  r.update(target=target,kind='call' if w&1 else ('branch' if op==18 else 'conditional'))
  if op==18:text=('bl' if w&1 else 'b')+f' 0x{target:x}'
  else:
   bo=(w>>21)&31;bi=(w>>16)&31
   if w&1 or bo not in [4,12] or bi not in [0,2]:raise ValueError('unsupported conditional form')
   text=({(12,2):'beq',(4,2):'bne',(12,0):'blt',(4,0):'bge'}[(bo,bi)])+f' 0x{target:x}'
   r.update(bo=bo,bi=bi)
 elif w in [0x4e800020,0x4e800420,0x4e800421]:
  r['kind']='return' if w==0x4e800020 else ('indirect_call' if w&1 else 'indirect_branch')
  text={0x4e800020:'blr',0x4e800420:'bctr',0x4e800421:'bctrl'}[w]
 elif op in [14,15]:
  text=(('li' if op==14 else 'lis')+f' r{rt},{imm}') if ra==0 else (('addi' if op==14 else 'addis')+f' r{rt},r{ra},{imm}')
  r.update(rt=rt,ra=ra,immediate=imm)
 elif op==24:
  text='nop' if w==0x60000000 else f'ori r{ra},r{rt},{w&65535}'
 elif op==11:
  if (w>>21)&1:raise ValueError('unsupported compare width')
  bf=(w>>23)&7;text='cmpwi '+(f'cr{bf},' if bf else '')+f'r{ra},{imm}'
 elif op in [32,36,37,58,62]:
  if op in [58,62]:
   if w&3:raise ValueError('unsupported DS update/sign form')
   imm=signed(w&0xfffc,16)
  text={32:'lwz',36:'stw',37:'stwu',58:'ld',62:'std'}[op]+f' r{rt},{imm}(r{ra})'
  r['memory']={'width':8 if op in [58,62] else 4,'write':op in [36,37,62], 'rt':rt,'ra':ra,'offset':imm,'update':op==37}
 elif op==31:
  xo=(w>>1)&1023;spr=((w>>16)&31)|((w>>6)&0x3e0)
  if xo==339 and spr==8:text=f'mflr r{rt}'
  elif xo==467 and spr in [8,9]:text=('mtlr' if spr==8 else 'mtctr')+f' r{rt}'
  elif xo==444 and rt==rb and not w&1:text=f'mr r{ra},r{rt}'
  else:raise ValueError('unsupported X-form')
 else:raise ValueError('unsupported instruction family')
 r['text']=text;return r

def decode_range(raw,start,end,origin):
 if start&3 or end&3 or start>=end or start<origin or end-origin>len(raw):raise ValueError('invalid range')
 return [decode(pc,struct.unpack_from('>I',raw,pc-origin)[0]) for pc in range(start,end,4)]

def reconcile(rows,body,*,complete=True,function_map=None):
 """Match independently addressed decoded instructions, with explicit anchors.

Only a verified contiguous range is accepted. Partial prefix matching is explicit;
there is no address calculation from comments. Branch/LR/label anchors must agree.
 """
 comments=[];fragments=[];current=[]
 for line in body.splitlines():
  m=re.match(r'^\t// (.+?)\s*$',line)
  if m:
   if comments:fragments.append('\n'.join(current))
   comments.append(m[1]);current=[]
  else:current.append(line)
 if comments:fragments.append('\n'.join(current))
 if len(comments)<len(rows) or complete and len(comments)!=len(rows):raise ValueError('instruction omission/extent mismatch')
 normal=lambda s:re.sub(r'\s+','',s).lower()
 for row,text,fragment in zip(rows,comments,fragments):
  if normal(row['text'])!=normal(text):raise ValueError('independent instruction mismatch')
  pc=row['pc'];kind=row['kind']
  if kind in ['call','indirect_call']:
   links=re.findall(r'ctx.lr\s*=\s*(0x[0-9A-Fa-f]+);',fragment)
   if links!=[f'0x{pc+4:X}']:raise ValueError('link/continuation anchor mismatch')
  if kind=='call' and function_map is not None:
   calls=re.findall(r'REX_CALL_FUNC\((\w+)\)|\b(\w+)\(ctx,\s*base\)',fragment)
   names=[a or b for a,b in calls]
   if len(names)!=1 or function_map.get(names[0])!=row['target']:raise ValueError('direct callee mapping mismatch')
  if kind=='indirect_call' and 'REX_CALL_INDIRECT_FUNC(ctx.ctr.u32)' not in re.sub(r'\s+','',fragment):raise ValueError('indirect source mismatch')
  if kind in ['branch','conditional']:
   targets=[int(v,16) for v in re.findall(r'goto loc_([0-9A-Fa-f]+);',fragment)]
   if targets!=[row['target']]:raise ValueError('emitted branch mismatch')
  if kind=='return' and 'return;' not in fragment:raise ValueError('return mismatch')
  if row['memory']:
   mem=row['memory'];macro='REX_'+('STORE' if mem['write'] else 'LOAD')+'_U'+str(mem['width']*8)
   if fragment.count(macro+'(')!=1:raise ValueError('memory width/direction mismatch')
 expected_pcs=[r['pc'] for r in rows];labels=[];position=0
 for line in body.splitlines():
  label=re.match(r'^loc_([0-9A-Fa-f]+):$',line)
  if label:labels.append(int(label[1],16))
  if re.match(r'^\t// ',line):
   if position<len(rows) and any(label!=expected_pcs[position] for label in labels):raise ValueError('label/PC mismatch')
   labels=[];position+=1
 return {'matched_instructions':len(rows),'explicit_link_anchors':sum(r['kind'] in ['call','indirect_call'] for r in rows),'branch_anchors':sum(r['kind'] in ['branch','conditional'] for r in rows)}

def reconcile_hooks(rows,body,expected):
 parts=re.split(r'^\t// .+$',body,flags=re.M)[1:]
 if len(parts)!=len(rows):raise ValueError('hook instruction map mismatch')
 found=[]
 for row,fragment in zip(rows,parts):
  matches=re.findall(r'^\tbrutal_cfg_([0-9A-F]+)\(\);$',fragment,re.M)
  if matches and matches!=[f'{row["pc"]:X}']:raise ValueError('misplaced/duplicate hook')
  if matches:found.append(row['pc'])
 wanted=sorted(set(expected)&{r['pc'] for r in rows})
 if sorted(found)!=wanted:raise ValueError('omitted/extra hook')
 return found

def coalesce(events):
 merged={}
 for event in events:
  e=dict(event);roles=set(e.pop('events'))
  if e.get('phase')!='before' or not roles<=EVENTS:raise ValueError('unsupported phase/event')
  pc=e['pc']
  if pc in merged:
   previous,previous_roles=merged[pc]
   if previous!=e:raise ValueError('incompatible duplicate PC')
   previous_roles.update(roles)
  else:merged[pc]=(e,roles)
 return [{**e,'events':sorted(roles),'index':i} for i,(e,roles) in enumerate(merged[pc] for pc in sorted(merged))]

def build(raw,origin,ranges,roots,imports,indirect,boundaries,bindings):
 """Explicit ranges and boundary actions are trusted policy inputs, not discovery."""
 ranges=sorted(ranges,key=lambda r:r['start']);roots=sorted(roots);imports=sorted(imports)
 if not ranges or len(ranges)>32 or len(set(roots))!=len(roots):raise ValueError('range/root policy')
 functions={r['start']:r for r in ranges};owner={};rows={}
 for f in ranges:
  if f['owner_start']&3 or f['owner_end']&3 or f['owner_start']<origin or f['owner_start']>=f['owner_end'] or f['owner_start']>f['start'] or f['end']>f['owner_end'] or f['owner_end']>origin+len(raw):raise ValueError('owner extent')
  for row in decode_range(raw,f['start'],f['end'],origin):
   pc=row['pc']
   if pc in rows:raise ValueError('overlapping/shared admitted ranges')
   rows[pc]=row;owner[pc]=f['start']
 if not set(roots)<=functions.keys() or set(imports)&rows.keys():raise ValueError('root/import collision')
 if not set(boundaries)<=rows.keys() or not set(indirect)<=rows.keys():raise ValueError('unknown policy site')
 sites=[];edges=set();return_targets={f:[] for f in functions}
 for pc,row in rows.items():
  action=boundaries.get(pc,'execute');kind=row['kind'];targets=[];events={'stop'}
  if action not in ['execute','exit','refuse']:raise ValueError('boundary action')
  if pc in functions:events.add('entry')
  if (pc-owner[pc])%32==0:events.add('long-path')
  if row['memory']:events.add('memory')
  if action!='execute':events.add('exit' if action=='exit' else 'refusal')
  elif kind in ['call','indirect_call','indirect_branch']:
   if kind=='indirect_branch':raise ValueError('computed tail unsupported; explicit refusal required')
   targets=[row['target']] if kind=='call' else sorted(indirect.get(pc,[]))
   if not targets or len(set(targets))!=len(targets):raise ValueError('unknown transfer without explicit boundary')
   if any(t not in functions and t not in imports for t in targets):raise ValueError('unauthorized target')
   events.add('call' if kind!='indirect_branch' else 'branch')
   if kind!='indirect_branch':
    if pc+4 not in rows or owner[pc+4]!=owner[pc]:raise ValueError('unverified continuation')
    for t in targets:
     if t in imports:edges.add((pc,pc+4));events.add('pre-import')
     else:edges.add((pc,t));return_targets[t].append(pc+4)
   else:
    for t in targets:edges.add((pc,t))
  elif kind in ['branch','conditional']:
   targets=[row['target']]+([pc+4] if kind=='conditional' else [])
   if any(t not in rows or owner[t]!=owner[pc] for t in targets):raise ValueError('branch escape/missing target')
   events.add('branch')
   if row['target']<=pc:events.add('backedge')
   edges.update((pc,t) for t in targets)
  elif kind=='return':events.add('return')
  else:
   if pc+4 not in rows or owner[pc+4]!=owner[pc]:raise ValueError('fallthrough escape')
   edges.add((pc,pc+4));targets=[pc+4]
  sites.append({'pc':pc,'owner':owner[pc],'phase':'before','kind':kind,'action':action,'condition':row.get('bo',0)*32+row.get('bi',0),'targets':targets,'events':events})
 for site in sites:
  if site['kind']=='return' and site['action']=='execute':
   site['targets']=sorted(set(return_targets[site['owner']]))
   if not site['targets'] and site['owner'] not in roots:raise ValueError('return lacks caller/root')
   edges.update((site['pc'],t) for t in site['targets'])
  if (site['pc']-4,site['pc']) in edges and any(s['pc']==site['pc']-4 and 'pre-import' in s['events'] for s in sites):site['events'].add('post-import')
 indegree={pc:0 for pc in rows}
 for a,b in edges:indegree[b]+=1
 for s in sites:
  if indegree[s['pc']]>1:s['events'].add('join')
 leaders=set(functions)|set(boundaries)
 for site in sites:
  if site['kind'] in ['branch','conditional','call','indirect_call','indirect_branch']:
   leaders.update(t for t in site['targets'] if t in rows)
   if site['pc']+4 in rows and owner[site['pc']+4]==site['owner']:leaders.add(site['pc']+4)
 blocks=[]
 for f in ranges:
  starts=sorted(pc for pc in leaders if f['start']<=pc<f['end'])
  for i,start in enumerate(starts):blocks.append({'start':start,'end':starts[i+1] if i+1<len(starts) else f['end'],'owner':f['start']})
 for site in sites:
  if site['pc'] in leaders:site['events'].add('block')
 sites=coalesce(sites)
 if len(sites)>256:raise ValueError('site limit')
 policy={'denied_transfers':['callback','exception','computed_tail','unlisted_target','unverified_continuation'],'functions':ranges,'blocks':blocks,'roots':roots,'imports':imports,'sites':sites,'edges':[list(e) for e in sorted(edges)]}
 if not {'xex_sha256','cli_revision','patch_lock_sha256','codegen_config_sha256'}<=bindings.keys() or bindings['cli_revision']!=REVISION:raise ValueError('missing/pinned binding')
 for key,h in bindings.items():
  if not isinstance(h,str) or not re.fullmatch('[0-9a-f]{40}' if key=='cli_revision' else '[0-9a-f]{64}',h):raise ValueError('hash/revision binding')
 policy_digest=sha(canonical(policy));namespace=sha(canonical({'bindings':bindings,'policy_sha256':policy_digest}))
 return {'schema':1,'bindings':dict(sorted(bindings.items())),'policy_sha256':policy_digest,'namespace':namespace,'policy':policy}

def validate(manifest,authority):
 # Independent authority is rebuilt from frozen bytes/ranges, not the supplied
 # document's self-reported digest. Rehashing a forged document cannot admit it.
 if canonical(manifest)!=canonical(authority):raise ValueError('manifest differs from independent authority')
 p=manifest['policy'];sites=p['sites']
 if [s['index'] for s in sites]!=list(range(len(sites))) or coalesce([{k:v for k,v in s.items() if k!='index'} for s in sites])!=sites:raise ValueError('site index/order mismatch')
 if sha(canonical(p))!=manifest['policy_sha256'] or sha(canonical({'bindings':manifest['bindings'],'policy_sha256':manifest['policy_sha256']}))!=manifest['namespace']:raise ValueError('namespace mismatch')
 return True

def stop_proof(manifest):
 p=manifest['policy'];sites={s['pc']:s for s in p['sites']};uncovered={pc for pc,s in sites.items() if 'stop' not in s['events']}
 # Removing all stop vertices must leave an acyclic graph. Bounded explicit DFS.
 graph={pc:[] for pc in uncovered}
 for a,b in p['edges']:
  if a in uncovered and b in uncovered:graph[a].append(b)
 active=set();done=set()
 def visit(pc):
  if pc in active:raise ValueError('cycle without stop')
  if pc in done:return
  active.add(pc)
  for target in graph[pc]:visit(target)
  active.remove(pc);done.add(pc)
 for pc in graph:visit(pc)
 required={'entry','backedge','long-path','pre-import','post-import','exit','refusal'}
 for site in sites.values():
  if set(site['events'])&required and 'stop' not in site['events']:raise ValueError('uncovered required boundary')
 return {'sites':len(sites),'stops':len(sites)-len(uncovered),'backedges':sum('backedge' in s['events'] for s in sites.values()),'uncovered_cycles':0}
