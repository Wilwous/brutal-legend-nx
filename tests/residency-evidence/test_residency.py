#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original hash-chain consistency fixtures; no console signature or execution."""
import hashlib,struct,sys,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import audit_xex_residency as a
BASE=0x90000000;PAGE=4096;SEC=0x100;TABLE=0x98+224
def put(b,at,v,n=4,big=True):b[at:at+n]=v.to_bytes(n,'big' if big else 'little')
def chain(x,image):
 # Original fixture authoring, reverse construction; verifier walks forwards.
 link=bytes(20)
 for i in [2,1,0]:
  at=SEC+0x184+24*i;x[at+4:at+24]=link
  link=hashlib.sha1(image[i*PAGE:(i+1)*PAGE]+x[at:at+24]).digest()
 x[SEC+0x114:SEC+0x128]=link
def fixture():
 x=bytearray(0x400);image=bytearray(3*PAGE)
 for at,v in [(0,0x58455832),(8,len(x)),(16,SEC),(20,3),(24,0x10201),(28,BASE),(32,0x2ff),(36,0x300),(40,0x3ff),(44,0x330),(SEC+4,len(image)),(SEC+0x10c,0x10000000),(SEC+0x110,BASE),(SEC+0x180,3),(0x300,20),(0x30c,BASE+0x2020),(0x310,32),(0x330,8)]:put(x,at,v)
 x[0x304:0x30c]=b'ORIGINAL'
 for i,kind in enumerate([3,1,2]):put(x,SEC+0x184+24*i,0x10+kind)
 for at,v,n in [(0,0x905a4d,4),(60,0x80,4),(0x80,0x4550,4),(0x84,0x1f2,2),(0x86,3,2),(0x94,224,2),(0x98,0x10b,2),(0x98+28,BASE,4),(0x98+32,PAGE,4),(0x98+36,512,4),(0x98+56,len(image),4)]:put(image,at,v,n,False)
 for i,flags in enumerate([0x40000040,0x60000020,0xc0000080]):
  at=TABLE+40*i;image[at:at+8]=('part'+str(i)).encode().ljust(8,b'\0')
  for off,v in [(8,PAGE),(12,i*PAGE),(16,0 if i==2 else PAGE),(20,i*PAGE),(36,flags)]:put(image,at+off,v,4,False)
 image[0x2020:0x2040]=bytes(range(32));chain(x,image);return x,image
class Residency(unittest.TestCase):
 def setUp(self):self.x,self.image=fixture()
 def change_section(self,off,value):put(self.image,TABLE+80+off,value,4,False);chain(self.x,self.image)
 def test_full_pages(self):
  r=a.audit(self.x,self.image);self.assertEqual(r['chain']['verified_descriptors'],3);self.assertTrue(all(c['full_descriptor_coverage'] for c in r['claims']));self.assertFalse(r['live_owner_proven']);self.assertFalse(r['chain']['rsa_signature_verified'])
 def test_pe_extent_no_owner(self):
  put(self.image,0x98+56,0x9000,4,False);chain(self.x,self.image);self.assertEqual(a.audit(self.x,self.image)['inventory']['end'],BASE+3*PAGE)
 def test_uncovered(self):
  self.change_section(8,2*PAGE);self.assertEqual(a.audit(self.x,self.image)['claims'][-1]['uncovered_bytes'],PAGE)
 def test_discardable_not_authority(self):
  self.change_section(8,2*PAGE);self.change_section(36,0x42000040);r=a.audit(self.x,self.image);self.assertFalse(r['claims'][-1]['full_descriptor_coverage']);self.assertEqual(r['outside_zero_fill_contract'],'UNKNOWN')
 def test_raw_offset_not_virtual(self):
  self.change_section(8,2*PAGE);self.change_section(20,0x200);self.assertEqual(a.audit(self.x,self.image)['claims'][-1]['uncovered_bytes'],PAGE)
 def test_resource_intersection(self):self.assertEqual(a.audit(self.x,self.image)['claims'][-1]['resource_intersections'],[0])
 def test_resource_outside(self):
  put(self.x,0x30c,BASE+3*PAGE)
  with self.assertRaises(ValueError):a.audit(self.x,self.image)
 def test_page_byte(self):
  self.image[0x2100]^=1
  with self.assertRaises(ValueError):a.audit(self.x,self.image)
 def test_root_hash(self):
  self.x[SEC+0x114]^=1
  with self.assertRaises(ValueError):a.audit(self.x,self.image)
 def test_descriptor_link(self):
  self.x[SEC+0x188]^=1
  with self.assertRaises(ValueError):a.audit(self.x,self.image)
 def test_zero_descriptor(self):
  put(self.x,SEC+0x184,3)
  with self.assertRaises(ValueError):a.audit(self.x,self.image)
 def test_descriptor_kind(self):
  for kind in [0,4,7,8,15]:
   with self.subTest(kind=kind):
    put(self.x,SEC+0x184,0x10+kind)
    with self.assertRaises(ValueError):a.audit(self.x,self.image)
 def test_base_conflict(self):
  put(self.x,28,BASE+PAGE)
  with self.assertRaises(ValueError):a.audit(self.x,self.image)
 def test_base_alignment(self):
  put(self.x,28,BASE+1);put(self.x,SEC+0x110,BASE+1)
  with self.assertRaises(ValueError):a.audit(self.x,self.image)
 def test_missing_image(self):
  with self.assertRaises(ValueError):a.audit(self.x,self.image[:-1])
 def test_extra_image(self):
  with self.assertRaises(ValueError):a.audit(self.x,self.image+b'\0')
 def test_section_overlap(self):
  self.change_section(12,PAGE)
  with self.assertRaises(ValueError):a.audit(self.x,self.image)
 def test_address_wrap(self):
  self.change_section(12,0xfffff000)
  with self.assertRaises(ValueError):a.audit(self.x,self.image)
 def test_truncated_header(self):
  with self.assertRaises(ValueError):a.audit(self.x[:100],self.image)
 def test_wrong_pe(self):
  self.image[0x80]=0;chain(self.x,self.image)
  with self.assertRaises(ValueError):a.audit(self.x,self.image)
if __name__=='__main__':unittest.main()
