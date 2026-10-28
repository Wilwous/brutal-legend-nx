#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Read-only XEX2/decoded-image census. Never maps or admits guest memory.

Page-chain equation: SHA1(page bytes || current descriptor), rooted in security
info, then expected hash from current descriptor. Independently authored from
the published landaire/acceleration page-descriptor research. Not RSA validation.
"""
import argparse,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
LIMIT=64*1024*1024
def span(data,offset,size):
 if offset<0 or size<0 or offset>len(data) or size>len(data)-offset:raise ValueError('truncated metadata/backing')
 return data[offset:offset+size]
def word(data,offset,endian='>'):return struct.unpack(endian+'I',span(data,offset,4))[0]
def half(data,offset):return struct.unpack('<H',span(data,offset,2))[0]
def xex_inventory(xex):
 if len(xex)>LIMIT or span(xex,0,4)!=b'XEX2':raise ValueError('unsupported XEX2 input')
 header=word(xex,8);security=word(xex,16);count=word(xex,20)
 if header>len(xex) or header<24 or count>(header-24)//8 or security%4:raise ValueError('invalid header bounds')
 span(xex,security,0x184)
 if security+0x184>header:raise ValueError('security outside header')
 options={}
 for i in range(count):
  key,value=word(xex,24+8*i),word(xex,28+8*i)
  if key in options:raise ValueError('duplicate optional key')
  options[key]=value
 base=options.get(0x10201,word(xex,security+0x110));security_base=word(xex,security+0x110)
 if base!=security_base:raise ValueError('conflicting base claims')
 flags=word(xex,security+0x10c);page=4096 if flags&0x10000000 else 65536
 n=word(xex,security+0x180)
 if not n or n>4096 or n>(header-security-0x184)//24 or base%page:raise ValueError('descriptor/alignment bounds')
 regions=[];cursor=base
 for i in range(n):
  at=security+0x184+24*i;raw=span(xex,at,24);value=int.from_bytes(raw[:4],'big');pages=value>>4;kind=value&15
  if not pages or kind not in (1,2,3) or pages*page>LIMIT or cursor+pages*page>2**32:raise ValueError('invalid descriptor claim')
  regions.append(dict(start=cursor,end=cursor+pages*page,kind=kind,descriptor_offset=at));cursor+=pages*page
 if cursor-base>LIMIT:raise ValueError('unsupported extent budget')
 resources=[]
 if 0x2ff in options:
  at=options[0x2ff];size=word(xex,at)
  if size<4 or (size-4)%16 or at+size>header:raise ValueError('resource table bounds')
  for pos in range(at+4,at+size,16):
   address,length=word(xex,pos+8),word(xex,pos+12)
   if not length or address<base or address+length>cursor:raise ValueError('resource outside descriptors')
   resources.append(dict(name=span(xex,pos,8).hex(),start=address,end=address+length))
 return dict(base=base,end=cursor,page_size=page,security=security,security_size=word(xex,security+4),regions=regions,resources=resources,header_size=header)
def page_chain(xex,image,inventory):
 if len(image)!=inventory['end']-inventory['base']:raise ValueError('decoded extent mismatch')
 sec=inventory['security'];expected=span(xex,sec+0x114,20);offset=0
 for r in inventory['regions']:
  length=r['end']-r['start'];descriptor=span(xex,r['descriptor_offset'],24)
  if hashlib.sha1(span(image,offset,length)+descriptor).digest()!=expected:raise ValueError('page chain mismatch')
  expected=descriptor[4:];offset+=length
 return dict(verified_descriptors=len(inventory['regions']),verified_bytes=offset,rsa_signature_verified=False)
def pe_inventory(image):
 if span(image,0,2)!=b'MZ':raise ValueError('DOS signature')
 nt=word(image,60,'<')
 if nt%4 or span(image,nt,4)!=b'PE\0\0':raise ValueError('PE signature/alignment')
 if half(image,nt+4)!=0x1f2 or half(image,nt+20)!=224:raise ValueError('unsupported PE headers')
 opt=nt+24
 if half(image,opt)!=0x10b:raise ValueError('unsupported PE32')
 count=half(image,nt+6);span(image,opt,224);span(image,opt+224,count*40);sections=[]
 for i in range(count):
  at=opt+224+i*40;size,rva=word(image,at+8,'<'),word(image,at+12,'<');base=word(image,opt+28,'<')
  if not size or base+rva+size>2**32:raise ValueError('section address overflow/empty')
  sections.append(dict(name=span(image,at,8).hex(),start=base+rva,end=base+rva+size,raw_offset=word(image,at+20,'<'),raw_size=word(image,at+16,'<'),flags=word(image,at+36,'<')))
 ordered=sorted(sections,key=lambda s:s['start'])
 if any(a['end']>b['start'] for a,b in zip(ordered,ordered[1:])):raise ValueError('overlapping PE sections')
 return dict(base=word(image,opt+28,'<'),size_of_image=word(image,opt+56,'<'),section_alignment=word(image,opt+32,'<'),file_alignment=word(image,opt+36,'<'),sections=sections)
def audit(xex,image):
 inv=xex_inventory(xex);pe=pe_inventory(image)
 if pe['base']!=inv['base'] or len(image)!=inv['end']-inv['base'] or inv['security_size']!=len(image):raise ValueError('inconsistent image extents/base')
 chain=page_chain(xex,image,inv);claims=[]
 for section in pe['sections']:
  low=max(inv['base'],section['start']);high=min(inv['end'],section['end']);backed=max(0,high-low);whole=backed==section['end']-section['start']
  prefix=span(image,low-inv['base'],backed) if backed else b''
  claims.append(dict(**section,descriptor_backed_bytes=backed,uncovered_bytes=section['end']-section['start']-backed,full_descriptor_coverage=whole,nonzero_backed_bytes=sum(b!=0 for b in prefix),readable=bool(section['flags']&0x40000000),resource_intersections=[i for i,r in enumerate(inv['resources']) if max(section['start'],r['start'])<min(section['end'],r['end'])],classification='A descriptor-covered candidate; live owner still required' if whole else 'D unsupported full-section claim; E console residency rule unknown'))
 return dict(status='STATIC ONLY; never execution admission',xex_sha256=hashlib.sha256(xex).hexdigest(),image_sha256=hashlib.sha256(image).hexdigest(),inventory=inv,pe=pe,chain=chain,claims=claims,additional_allocation_contract='UNKNOWN',outside_zero_fill_contract='UNKNOWN',live_owner_proven=False,guest_functions_invoked=0)
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for n in ['xex','image','report']:ap.add_argument('--'+n,type=Path,required=True)
 args=ap.parse_args();target=args.report.resolve()
 if target==ROOT or ROOT in target.parents:ap.error('report must remain private outside repository')
 for f in [args.xex,args.image]:
  if f.stat().st_size>LIMIT:ap.error('input budget exceeded')
 result=audit(args.xex.read_bytes(),args.image.read_bytes())
 with target.open('x') as f:json.dump(result,f,indent=2);f.write('\n')
 print(json.dumps(dict(status=result['status'],descriptors=result['chain']['verified_descriptors'],sections=len(result['claims']),uncovered_section_claims=sum(not c['full_descriptor_coverage'] for c in result['claims']))))
if __name__=='__main__':main()
