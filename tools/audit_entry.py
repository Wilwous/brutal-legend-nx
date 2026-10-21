#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Read private generated output statically; emit counts/import names, never bodies.

Literal direct-call closure only: branches are not evaluated, indirect targets,
callbacks and table-driven exception edges are not recovered. No compilation or
execution occurs. Output must remain outside the public checkout.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
DEFINITION=re.compile(r'DEFINE_REX_FUNC\((\w+)\)\s*\{')
CALL=re.compile(r'\b(\w+)\(ctx,\s*base\)')

def analyze(directory,entry):
    functions={}; fingerprints=[]
    for path in sorted(directory.rglob('*recomp*.cpp')):
        raw=path.read_bytes();source=raw.decode('utf-8')
        fingerprints.append(hashlib.sha256(raw).hexdigest())
        matches=list(DEFINITION.finditer(source))
        for i,match in enumerate(matches):
            name=match[1]
            if name in functions:raise ValueError('duplicate emitted function')
            body=source[match.end():matches[i+1].start() if i+1<len(matches) else len(source)]
            functions[name]=re.sub(r'//[^\n]*|/\*.*?\*/','',body,flags=re.S)
    if entry not in functions:raise ValueError('entry symbol absent')
    seen=set();pending=[entry]
    while pending:
        name=pending.pop()
        if name in seen:continue
        seen.add(name);pending.extend(CALL.findall(functions.get(name,'')))
    unknown=sorted(name for name in seen if name not in functions and not name.startswith('__imp__'))
    def count(names):
        text='\n'.join(functions[n] for n in names if n in functions)
        return {'load_sites':len(re.findall(r'\bREX_LOAD_\w+\(',text)),
                'store_sites':len(re.findall(r'\bREX_STORE_\w+\(',text)),
                'indirect_call_sites':len(re.findall(r'\bREX_CALL_INDIRECT_FUNC\(',text))}
    return {'classification':'PARTIAL','method':'literal direct calls; no branch feasibility or dynamic closure',
            'generated_partitions':len(fingerprints),'generated_functions':len(functions),
            'input_content_digest':hashlib.sha256(''.join(fingerprints).encode()).hexdigest(),
            'closure_nodes':len(seen),'closure_generated_functions':len(seen & functions.keys()),
            'direct_imports':sorted(name.removeprefix('__imp__') for name in seen if name.startswith('__imp__')),
            'unresolved_literal_symbols':unknown,'all_access_sites':count(functions),'closure_access_sites':count(seen),
            'mediated_generated_sites':0}

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--generated',required=True,type=Path)
    ap.add_argument('--entry',default='xstart')
    ap.add_argument('--output',required=True,type=Path)
    a=ap.parse_args();output=a.output.resolve()
    if output==ROOT or ROOT in output.parents:ap.error('private output required')
    result=analyze(a.generated,a.entry)
    output.parent.mkdir(parents=True,exist_ok=True);output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
if __name__=='__main__':main()
