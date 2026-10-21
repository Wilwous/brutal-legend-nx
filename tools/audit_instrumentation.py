#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Lexical coverage census only: no code execution or source rewriting.

Reports counts, never private function names/addresses/snippets. A fixture gate
checks known emitted seams, not arbitrary C++ sandbox security or a full access
proof. Unknown SDK helpers/inline assembly still require manual review.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
MACRO=re.compile(r'\b(REX_(?:(?:MM_)?(?:LOAD|STORE)_\w+|RAW_ADDR|PARTIAL_VECTOR_GUARD|CALL_FUNC|CALL_INDIRECT_FUNC))\s*\(')
DEFINITION=re.compile(r'DEFINE_REX_FUNC\((\w+)\)\s*\{')
CALL=re.compile(r'\b(\w+)\(ctx,\s*base\)')
def census(source):
    text=re.sub(r'//[^\n]*|/\*.*?\*/','',source,flags=re.S)
    macros=Counter(MACRO.findall(text))
    return {'macros':dict(sorted(macros.items())),
            'literal_calls':len(CALL.findall(text)),
            'direct_base_pointer_patterns':len(re.findall(r'\bbase\s*(?:\+|\[)|\b(?:GuestPtr|TranslateVirtual|TranslateRelative)\s*[<(]',text)),
            'exception_or_nonlocal_helpers':len(re.findall(r'\b(?:ppc_setjmp|ppc_longjmp|ppc_trap)\s*\(|\bcatch\s*\(',text))}
def combine(rows):
    macros=Counter();out={'literal_calls':0,'direct_base_pointer_patterns':0,'exception_or_nonlocal_helpers':0}
    for row in rows:
        macros.update(row['macros'])
        for key in out:out[key]+=row[key]
    return {'macros':dict(sorted(macros.items())),**out}
def assess(directory,entry='xstart'):
    summaries={};edges={};digests=[]
    for path in sorted(directory.rglob('*recomp*.cpp')):
        raw=path.read_bytes();digests.append(hashlib.sha256(raw).hexdigest());source=raw.decode()
        matches=list(DEFINITION.finditer(source))
        for i,match in enumerate(matches):
            name=match[1]
            if name in summaries:raise ValueError('duplicate function')
            body=source[match.end():matches[i+1].start() if i+1<len(matches) else len(source)]
            summaries[name]=census(body)
            stripped=re.sub(r'//[^\n]*|/\*.*?\*/','',body,flags=re.S)
            edges[name]=CALL.findall(stripped)
    if entry not in summaries:raise ValueError('entry absent')
    seen=set();todo=[entry]
    while todo:
        name=todo.pop()
        if name in seen:continue
        seen.add(name);todo+=edges.get(name,[])
    return {'classification':'PARTIAL','body_partitions':len(digests),'functions':len(summaries),
            'input_content_digest':hashlib.sha256(''.join(digests).encode()).hexdigest(),
            'all':combine(summaries.values()),'direct_entry_closure':combine(summaries[n] for n in seen if n in summaries),
            'actually_instrumented_private_sites':0,
            'limits':'lexical direct closure only; no branch feasibility, pointer alias or indirect target proof'}
def fixture_gate(source):
    result=census(source)
    if result['literal_calls'] or result['direct_base_pointer_patterns'] or result['exception_or_nonlocal_helpers']:
        raise ValueError('unreviewed fixture bypass/helper')
    macros=result['macros']
    if macros.get('REX_LOAD_VECTOR')!=1 or macros.get('REX_STORE_VECTOR')!=1 or macros.get('REX_PARTIAL_VECTOR_GUARD')!=4 or macros.get('REX_CALL_FUNC')!=1 or macros.get('REX_CALL_INDIRECT_FUNC')!=1:
        raise ValueError('generator seam drift')
    return result
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--generated',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();output=a.output.resolve()
    if ROOT==output or ROOT in output.parents:ap.error('private output required')
    result=assess(a.generated);output.parent.mkdir(parents=True,exist_ok=True);output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if __name__=='__main__':main()
