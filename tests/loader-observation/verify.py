#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Finite source-backed observation contracts; reference phase limits stay explicit."""
REFUSED={'pe-overhang','none-short','no-read','overlap','bad-pe','rounded-owned'}
def verify(rows):
 passed=0;failed=[]
 def check(ok,label):
  nonlocal passed
  if ok:passed+=1
  else:failed.append(label)
 for row in rows:
  name=row['case'];a=row.get('project');b=row.get('reference_slice')
  check(a is not None and b is not None,name+': both observations')
  if a is None or b is None:continue
  accepted=name not in REFUSED;pages=4 if name=='owned-tail' else 3
  check(a['load_status']==(0 if accepted else 0xc0000001),name+': SDK load result')
  check(a['balanced'],name+': SDK full page rollback')
  check(all(p['query'] and p['state']==0 for p in a['pages_after']),name+': SDK pages free after cleanup')
  check(a['guest_invocations']==0 and b['guest_invocations']==0,name+': no guest entry')
  check(b['image_return']==0,name+': reference image materialization')
  check(b['pe_return']==(1 if name=='bad-pe' else 0),name+': reference PE-only decision')
  request=4*65536 if name=='owned-tail' else 2*65536 if name=='none-short' else 3*65536+(name=='rounded-owned')
  check(b['allocation_request']==request,name+': source allocation request')
  check(b['full_xenia_load']=='UNOBSERVED',name+': no whole-reference claim')
  if accepted:
   check(a['active_unload_refused'],name+': SDK active lease refuses unload')
   check(a['marker_copy']==0 and a['marker']==0,name+': SDK entry marker unchanged')
   check(a['content_copy']==0 and a['content']==(0 if name.startswith('basic-') else 0xa5),name+': SDK known initialization')
   check([p['state'] for p in a['pages_during']]==[3]*pages+[0]*(5-pages),name+': SDK guest commitment')
   check([p['protect'] for p in a['pages_during']]==[1,1]+[3]*(pages-2)+[0]*(5-pages),name+': SDK descriptor protection')
   check(a['pages_during'][0]['allocation_size']==pages*65536,name+': SDK reservation size')
  else:check(all(p['state']==0 for p in a['pages_during']),name+': refused SDK module owns no pages')
  if b['marker_available']:check(b['marker']==0,name+': reference marker unchanged')
  if name in ('basic-zero','basic-gap'):check(b['content']==0,name+': BASIC owned allocation zero initialized')
  if name in ('raw-zero','raw-short'):check(b['content']==0xa5,name+': raw declaration does not replace XEX payload')
  if name=='none-short':check(any('outside-adapter' in s for s in b['trace']),name+': original loop ignores failed Protect result')
  if name=='rounded-owned':check(b['tail']==0x7b,name+': known extra payload byte, no inferred padding content')
 return passed,failed
