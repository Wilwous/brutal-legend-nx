#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Read-only bounded transcriptions of pinned loader branches, not loaders.

Input is previously verified metadata plus a separately parsed PE inventory.
Predictions never establish Xbox semantics, guest ownership or load success.
"""
import argparse,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def compare(metadata,independent_pe,stored_payload_bytes):
 inv=metadata['inventory'];base=inv['base'];extent=inv['end']-base
 if not 0<extent<=64*1024*1024 or inv['security_size']!=extent or sum(r['end']-r['start'] for r in inv['regions'])!=extent:raise ValueError('inconsistent recorded extent')
 if not 0<=stored_payload_bytes<=64*1024*1024:raise ValueError('stored payload budget')
 sections=metadata['pe']['sections'];other=independent_pe['sections']
 if len(sections)!=len(other) or not sections:raise ValueError('independent section count')
 for s,t in zip(sections,other):
  if (s['start'],s['end']-s['start'],s['raw_offset'],s['raw_size'],s['flags'])!=(base+t['rva'],t['vsize'],t['raw_offset'],t['raw_size'],int(t['flags'],16)):raise ValueError('independent PE disagreement')
 claims=[];ida_extent=max(stored_payload_bytes,inv['security_size'])
 for i,s in enumerate(sections):
  start=s['start']-base;size=s['end']-s['start']
  if start<0 or size<=0 or s['end']>2**32:raise ValueError('section bounds')
  claims.append(dict(index=i,descriptor_coverage=max(0,min(size,extent-start)),sdk_exact_owner_accepts=start+size<=extent and bool(s['flags']&0x40000000),xenia_records_declared_span=True,idaxex_declared_segment_bytes=size,idaxex_available_copy_bytes=max(0,min(size,ida_extent-start)),console_tail_owner='UNKNOWN'))
 return dict(static_only=True,comparison_loader_executed=False,common_ancestry='SDK XEX loader derives from Xenia; agreement is not independent Xbox proof',sdk=dict(coordinate_extent=extent,allocation='descriptor-owned reserve/commit; no PE extension',permissions='descriptor page permissions AND readable PE section byte permissions for checked copying',initialization='zero and decode inside owned extent only',reclamation='transaction owner tokens and image leases'),xenia=dict(coordinate_extent=extent,allocation='descriptor image_size reserve+commit; no PE-tail allocation inferred',permissions='XEX page types, not PE byte checks',initialization='memset extent then LZX',reclamation='Release image base in Unload'),idaxex=dict(host_vector_extent=ida_extent,guest_allocation=None,permissions='PE flags as analysis database attributes',initialization='available decoded bytes copied; wider database segment is not RAM',reclamation='analysis database lifetime; no Xbox owner proof'),claims=claims,original_layout='UNSUPPORTED BY CURRENT SDK' if any(not c['sdk_exact_owner_accepts'] for c in claims) else 'STATIC CANDIDATE ONLY',console_residency='UNKNOWN')
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for n in ['metadata','independent-pe','report']:ap.add_argument('--'+n,type=Path,required=True)
 ap.add_argument('--stored-payload-bytes',type=int,required=True);a=ap.parse_args();report=a.report.resolve()
 if report==ROOT or ROOT in report.parents:ap.error('private report required')
 if any(f.stat().st_size>1048576 for f in [a.metadata,a.independent_pe]):ap.error('metadata budget')
 result=compare(json.loads(a.metadata.read_text()),json.loads(a.independent_pe.read_text()),a.stored_payload_bytes)
 with report.open('x') as f:json.dump(result,f,indent=2);f.write('\n')
 print(json.dumps(dict(static_only=True,sections=len(result['claims']),unsupported=sum(not c['sdk_exact_owner_accepts'] for c in result['claims']),console_residency='UNKNOWN')))
if __name__=='__main__':main()
