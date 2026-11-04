#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Validate evidence-scoped partial orders; UNKNOWN never means no observer."""
import argparse,json

def validate(graph):
 nodes=graph['nodes'];scopes=graph['scopes'];edges=graph['edges']
 if not nodes or len(nodes)!=len(set(nodes)) or not scopes or len(scopes)!=len(set(scopes)):raise ValueError('duplicate/empty event or scope')
 if len(nodes)>256 or len(edges)>1024:raise ValueError('finite graph budget exceeded')
 for e in edges:
  if e['before'] not in nodes or e['after'] not in nodes or e['before']==e['after'] or e['scope'] not in scopes:raise ValueError('invalid event edge')
  if e['status'] not in ['PROVEN','PARTIAL','UNKNOWN']:raise ValueError('invalid evidence classification')
  if e['status']=='PROVEN' and not e.get('evidence'):raise ValueError('missing edge evidence')
  if e.get('condition') is not None and not isinstance(e['condition'],str):raise ValueError('invalid branch condition')
 for scope in scopes:
  pending=set(nodes);done=set()
  while pending:
   ready={n for n in pending if not any(e['scope']==scope and e['status']=='PROVEN' and e['after']==n and e['before'] not in done for e in edges)}
   if not ready:raise ValueError('contradictory ordering cycle')
   pending-=ready;done|=ready
 return graph

def relation(graph,before,after,scope,assumptions=()):
 validate(graph)
 if before not in graph['nodes'] or after not in graph['nodes'] or scope not in graph['scopes']:raise ValueError('unknown query event/scope')
 if before==after:return 'SAME'
 def reaches(start,target):
  seen={start};pending=[start]
  while pending:
   n=pending.pop()
   for e in graph['edges']:
    if e['scope']==scope and e['status']=='PROVEN' and e.get('condition') in [None,*assumptions] and e['before']==n and e['after'] not in seen:
     if e['after']==target:return True
     seen.add(e['after']);pending.append(e['after'])
  return False
 if reaches(before,after):return 'BEFORE'
 if reaches(after,before):return 'AFTER'
 return 'UNKNOWN'

if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('graph');a=ap.parse_args()
 with open(a.graph) as f:g=json.load(f)
 validate(g);print(json.dumps([relation(g,**q) for q in g.get('queries',[])]))
