#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Real decoder/builder/template, original synthetic execution, bounded children."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run
spec=importlib.util.spec_from_file_location('audit',ROOT/'tools/audit_instrumentation.py');audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--sdk',type=Path,required=True);ap.add_argument('--source',type=Path,required=True);ap.add_argument('--workdir',type=Path,required=True);ap.add_argument('--sanitize',action='store_true');a=ap.parse_args()
    sdk=a.sdk.resolve();source=a.source.resolve();work=a.workdir.resolve()
    if ROOT==work or ROOT in work.parents:ap.error('private output required')
    meta=json.loads((sdk/'fixture-sdk.json').read_text())
    if hashlib.sha256((sdk/'lib'/meta['runtime_filename']).read_bytes()).hexdigest()!=meta['runtime_sha256']:raise RuntimeError('runtime hash mismatch')
    if (meta['config']=='Sanitized')!=a.sanitize:raise RuntimeError('sanitizer mismatch')
    work.mkdir(parents=True,exist_ok=False)
    for name in list(os.environ):
        if name.startswith(('DYLD_','REXSDK','VK_','VULKAN_')):del os.environ[name]
    os.environ['UBSAN_OPTIONS']='print_stacktrace=1'
    flags=['-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0' if meta['config']=='Debug' else '-O2','-g','-I'+str(sdk/'include'),'-iquote',str(ROOT)]
    if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    libraries=['-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib')];records=[]
    def execute(cmd,label):
        rec=run(cmd,work,label,30,True);records.append(rec)
        if rec['timeout'] or rec['exit_code']:raise RuntimeError(label+' failed; inspect private diagnostics')
        return rec
    suffix='d' if meta['config']=='Debug' else ''
    emitter=work/'emitter'
    execute(['clang++',*flags,'-I'+str(source/'src/codegen/builders'),'-I'+str(source/'thirdparty/disasm'),str(ROOT/'tests/instrumentation/emitter.cpp'),*libraries,'-lrexcodegen'+suffix,'-ldisasm'+suffix,'-lfmt'+suffix,'-lxxhash'+suffix,'-o',str(emitter)],'build-emitter')
    execute([str(emitter),str(work)],'emit')
    coverage=audit.fixture_gate((work/'synthetic.inc').read_text())
    rows=[];sizes={}
    define=['-DREX_CHECKED_ACCESS_HEADER="runtime/generated_access.h"']
    for mode,extra in [('default',[]),('checked',define)]:
        binary=work/mode;common=['clang++',*flags,'-I'+str(work),*extra]
        execute([*common,str(ROOT/'tests/instrumentation/contracts.cpp'),*libraries,'-o',str(binary)],'build-'+mode)
        execute([str(binary),str(work/(mode+'-runtime'))],mode)
        observed=[json.loads(line) for line in (work/(mode+'.stdout')).read_text().splitlines() if line.startswith('{')]
        if len(observed)!=1 or observed[0].get('result')!='PASS':raise RuntimeError('missing fixture result')
        rows+=observed
        # Same original functions only: object-size comparison excludes harness code.
        execute([*common,'-x','c++','-c',str(work/'synthetic.inc'),'-o',str(work/(mode+'.o'))],'build-size-'+mode)
        execute(['size','-m',str(work/(mode+'.o'))],'size-'+mode)
        text=(work/('size-'+mode+'.stdout')).read_text()
        sections=[int(n) for n in re.findall(r'Section \(__TEXT, __text\): (\d+)' ,text)]
        if len(sections)!=1:raise RuntimeError('missing object text size')
        execute([*common,'-c',str(ROOT/'tests/instrumentation/test-template.cpp'),'-o',str(work/('test-template-'+mode+'.o'))],'build-test-template-'+mode)
        sizes[mode]={'object_bytes':(work/(mode+'.o')).stat().st_size,'text_bytes':sections[0],'binary_bytes':binary.stat().st_size}

    # Macro definitions after actual PCH expansion must select checked functions.
    execute(['clang++',*flags,'-I'+str(work),*define,'-dM','-E','-x','c++',str(work/'synthetic_pch.h'),'-o',str(work/'macros.txt')],'preprocess-file')
    definitions=(work/'macros.txt').read_text();selected=[line for line in definitions.splitlines() if re.match(r'#define REX_(?:(?:MM_)?(?:LOAD|STORE)_|RAW_ADDR|CALL_FUNC|CALL_INDIRECT_FUNC|LOOKUP_FUNC|PARTIAL_VECTOR_GUARD)',line)]
    if len(selected)!=24 or any('base +' in line for line in selected):raise RuntimeError('checked macro coverage drift')
    for line in selected:
        if not any(token in line for token in ['brutal::','REX_LOAD_U','REX_STORE_U','brutal_reject_partial_vector']):raise RuntimeError('unknown checked macro')
    for vector_case in range(3):
        label='typed-vector-negative-'+str(vector_case)
        negative=run(['clang++',*flags,'-DVECTOR_CASE='+str(vector_case),'-c',str(ROOT/'tests/instrumentation/vector-negative.cpp'),'-o',str(work/('vector'+str(vector_case)+'.o'))],work,label,30);records.append(negative)
        diagnostic=(work/(label+'.stderr')).read_text()
        if not negative['exit_code'] or negative['timeout'] or 'ContextFunction' not in diagnostic:raise RuntimeError('typed ABI refusal changed')
    result={'sdk':meta,'codegen_sha256':hashlib.sha256((sdk/'lib'/('librexcodegen'+suffix+'.a')).read_bytes()).hexdigest(),'records':records,'passed_groups':len(rows),'checks':rows,'coverage':coverage,'checked_macro_definitions':len(selected),'sizes':sizes,'generated_source_bytes':(work/'synthetic.inc').stat().st_size,'typed_vector':'BLOCKED explicit compile refusal','unexpected_failures':0}
    (work/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['sdk','records']},indent=2))
if __name__=='__main__':
    try:main()
    except (OSError,RuntimeError,ValueError) as error:print(str(error),file=sys.stderr);sys.exit(1)
