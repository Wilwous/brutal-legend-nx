# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Pure interval provenance accounting, never image admission or guest mapping."""
import hashlib,json
KINDS={'FILE-BACKED','DERIVED','EXPLICIT ZERO-FILL','UNBACKED','UNKNOWN'}
def canonical(value):return json.dumps(value,sort_keys=True,separators=(',',':')).encode()
def fingerprint(value):return hashlib.sha256(canonical(value)).hexdigest()
def extent(start,end):
 if type(start)!=int or type(end)!=int or not 0<=start<end<=1<<32:raise ValueError('invalid nonempty 32-bit interval')
def ledger(start,end,sources,claims=(),boundaries=()):
 extent(start,end)
 if len(sources)>4096 or len(claims)>4096 or len(boundaries)>16384:raise ValueError('interval budget')
 points={start,end};ordered=sorted(sources,key=lambda r:r['start'])
 for i,s in enumerate(ordered):
  extent(s['start'],s['end'])
  if s['start']<start or s['end']>end:raise ValueError('source outside audit domain')
  if i and ordered[i-1]['end']>s['start']:raise ValueError('overlapping content authority')
  if s['kind'] not in KINDS or s['kind'] in {'UNBACKED','UNKNOWN'}:raise ValueError('invalid content authority')
  if not s.get('evidence'):raise ValueError('missing source evidence')
  if s['kind']=='EXPLICIT ZERO-FILL':
   if not s.get('zero_rule'):raise ValueError('zero provenance absent')
  elif type(s.get('offset'))!=int or type(s.get('source_size'))!=int or s['offset']<0 or s['source_size']<0 or s['offset']+(s['end']-s['start'])>s['source_size'] or not isinstance(s.get('sha256'),str) or len(s['sha256'])!=64 or any(c not in '0123456789abcdef' for c in s['sha256']):raise ValueError('source offset/hash bounds')
  points.update((s['start'],s['end']))
 for claim in claims:
  extent(claim['start'],claim['end'])
  if claim['start']<start or claim['end']>end:raise ValueError('claim outside audit domain')
  points.update((claim['start'],claim['end']))
 for p in boundaries:
  if type(p)!=int or not start<=p<=end:raise ValueError('invalid boundary')
  points.add(p)
 points=sorted(points);rows=[]
 for lo,hi in zip(points,points[1:]):
  source=next((s for s in ordered if s['start']<=lo and hi<=s['end']),None);labels=[c.get('id') for c in claims if c['start']<=lo and hi<=c['end']]
  row={'start':lo,'end':hi,'size':hi-lo,'claims':labels,'kind':source['kind'] if source else ('UNBACKED' if labels else 'UNKNOWN'),'live_owner':False,'execution_admitted':False}
  if source:
   row['evidence']=source['evidence'];row['source_sha256']=source.get('sha256');row['source_offset']=source.get('offset',0)+lo-source['start'];row['zero_rule']=source.get('zero_rule')
  rows.append(row)
 totals={k:sum(r['size'] for r in rows if r['kind']==k) for k in sorted(KINDS)}
 if sum(totals.values())!=end-start:raise ValueError('accounting failure')
 return {'domain_bytes':end-start,'regions':rows,'totals':totals,'conflicts':0,'admission':'NOT AN EXECUTION ADMISSION','fingerprint':fingerprint(rows)}
def interval_identity(data,offset,length):
 if type(offset)!=int or type(length)!=int or offset<0 or length<=0 or offset+length>len(data):raise ValueError('interval source bounds')
 return hashlib.sha256(memoryview(data)[offset:offset+length]).hexdigest()
def audit_draft(candidate):
 """Non-admitting completeness report; caller must supply independently checked evidence."""
 reasons=[];rows=candidate['ledger']['regions'];total=candidate['ledger']['totals']
 if candidate.get('schema')!='native-image-audit-draft-v1':reasons.append('unsupported draft schema')
 if total['UNKNOWN'] or total['UNBACKED']:reasons.append('unresolved image intervals')
 if candidate.get('native_origin')!='original_synthetic':reasons.append('current native schema does not admit title provenance')
 if candidate.get('relocation_complete') is not True:reasons.append('relocation policy incomplete')
 if candidate.get('startup_complete') is not True:reasons.append('startup contract incomplete')
 if candidate.get('imports_complete') is not True:reasons.append('import contracts incomplete')
 if candidate.get('code_identity_complete') is not True:reasons.append('code/binding contract incomplete')
 return {'structure':'AUDIT DRAFT ONLY','proven_bytes':sum(r['size'] for r in rows if r['kind'] not in {'UNKNOWN','UNBACKED'}),'unresolved_bytes':total['UNKNOWN']+total['UNBACKED'],'full_title_admission':False,'reasons':reasons,'fingerprint':fingerprint(candidate)}
