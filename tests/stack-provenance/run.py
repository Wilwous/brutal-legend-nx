#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original nested-frame PPC through pinned production ReXGlue; no title input."""
import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from fixture import create, MODES, cfg

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'tests/ppc'))
from watchdog import run


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--workdir', type=Path, required=True)
    ap.add_argument('--toolchain', type=Path, required=True)
    a = ap.parse_args();w, tc = a.workdir.resolve(), a.toolchain.resolve()
    if w == ROOT or ROOT in w.parents:
        ap.error('fresh external output required')
    w.mkdir(parents=True, exist_ok=False)
    records = []
    def sha(p):
        return hashlib.sha256(p.read_bytes()).hexdigest()
    def execute(cmd, label):
        record = run(list(map(str, cmd)), w, label, 30);records.append(record)
        if record['timeout'] or record['exit_code']:
            raise RuntimeError(label+' failed; diagnostics preserved')
        return (w/(label+'.stdout')).read_text()
    names = ['runtime/horizon/'+n for n in ('space.h', 'guest.h', 'generated.h')]
    names += ['tests/stack-provenance/'+n for n in ('fixture.py', 'model.py', 'contract.h',
              'probe.cpp', 'run.py', 'verify.py', 'test_model.py',
              'templates/test/ppc_config_h.inja', 'templates/test/ppc_test_functions_cpp.inja')]
    before = {n: sha(ROOT/n) for n in names}
    pin = json.loads((tc/'prepared.json').read_text())
    assert pin['sdk_revision'] == 'f5337cdc947ff6d4c4196737e2c807a48f2a1fc2'
    assert sha(tc/'rexglue') == pin['cli_sha256'] == '6f40ec768bba9ea985719f6df543c5e0bd099b19ceaec88aee9f9c7f7e6cc15c'
    meta = create(w/'input')
    generated, baseline = w/'generated', w/'baseline'
    generated.mkdir();baseline.mkdir()
    cli = [tc/'rexglue', 'recompile-tests', '--bin-dir', w/'input', '--asm-dir', w/'input']
    execute([*cli, '--output', generated, '--templates', ROOT/'tests/stack-provenance/templates'], 'generation')
    execute([*cli, '--output', baseline], 'default-generation')
    hashes = {p.name: sha(p) for p in generated.iterdir()}
    execute([*cli, '--output', generated, '--templates', ROOT/'tests/stack-provenance/templates'], 'determinism')
    assert hashes == {p.name: sha(p) for p in generated.iterdir()}
    cpp = (generated/'ppc_test_functions.cpp').read_text()
    default = (baseline/'ppc_test_functions.cpp').read_text()
    assert cpp[cpp.index('DEFINE_REX_FUNC('):] == default[default.index('DEFINE_REX_FUNC('):]
    parts = re.split(r'DEFINE_REX_FUNC\((\w+)\)\s*\{', cpp)
    bodies = {parts[i]: parts[i+1] for i in range(1, len(parts), 2)}
    correspondence = []
    for p in meta:
        starts = [p['entry'], p['helper'], p['leaf']]
        mapping = {f'{p["mode"]}_{pc:X}': pc for pc in starts}
        for start, end in zip(starts, starts[1:]+[p['entry']+p['instructions']*4]):
            rows = [r for r in p['rows'] if start <= r['pc'] < end]
            name = f'{p["mode"]}_{start:X}'
            correspondence.append({'function': name, **cfg.reconcile(rows, bodies[name], function_map=mapping)})
    header = '#pragma once\n#include "runtime/horizon/generated.h"\n'
    for p in meta:
        for pc in (p['entry'], p['helper'], p['leaf']):
            header += f'REX_EXTERN({p["mode"]}_{pc:X});\n'
    header += 'namespace provenance_test {\nusing namespace brutal::horizon;\nenum class Mode {'+','.join(MODES)+'};\n'
    header += 'struct Program {Mode mode;uint32_t entry,helper,leaf,root_return,helper_return;uint64_t module,source;std::array<Function,3> functions;bool relocatable;};\n'
    header += 'inline const std::array<Program,7> programs{{\n'
    for i, p in enumerate(meta):
        fs = ','.join('{'+str(pc)+','+p['mode']+f'_{pc:X},"'+p['mode']+f'_{pc:X}'+'"}' for pc in (p['entry'], p['helper'], p['leaf']))
        header += '{Mode::'+p['mode']+','+','.join(str(p[k]) for k in ('entry', 'helper', 'leaf', 'root_return', 'helper_return'))+','+str(0x50524f5600+i)+',0x'+p['sha256'][:16]+',{{'+fs+'}},'+str(p['relocatable']).lower()+'},\n'
    header += '}};\n}\n'
    (w/'provenance-data.h').write_text(header)
    tests = []
    for profile, flags in [('Debug', ['-O0']), ('Release', ['-O2']),
                           ('Sanitized', ['-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer'])]:
        binary = w/('probe-'+profile)
        execute(['clang++', '-std=c++23', '-pthread', '-Wall', '-Wextra', '-Werror', *flags,
                 '-isystem', tc/'include', '-iquote', ROOT, '-I', ROOT/'tests/stack-provenance',
                 '-I', w, '-I', generated, ROOT/'tests/stack-provenance/probe.cpp',
                 generated/'ppc_test_functions.cpp', '-o', binary], 'compile-'+profile)
        snapshot = w/('observations-'+profile+'.bin')
        result = json.loads(execute([binary, snapshot], 'run-'+profile))
        assert result['refusals'] == result['zero_entry_checks']
        assert (result['completed_runs'], result['equivalent_pairs'], result['counterexample_pairs']) == (36, 12, 22)
        assert result['allocations'] == result['leases'] == 0
        independent = json.loads(execute(['python3', ROOT/'tests/stack-provenance/verify.py', snapshot], 'independent-'+profile))
        assert independent['equivalent_pairs'] == result['equivalent_pairs']
        assert independent['counterexample_pairs'] == result['counterexample_pairs']
        tests.append({'profile': profile, **result, 'independent': independent})
    assert before == {n: sha(ROOT/n) for n in names}
    assert hashes == {p.name: sha(p) for p in generated.iterdir()}
    assert len({t['independent']['snapshot_sha256'] for t in tests}) == 1
    output = {'tests': tests, 'records': records, 'fixtures': meta, 'static_correspondence': correspondence,
              'generated_sha256': hashes, 'sources': before, 'generated_unedited': True,
              'game_executed_generated_compiled': False, 'hardware_execution': 'NOT TESTED'}
    with (w/'results.json').open('x') as f:
        json.dump(output, f, indent=2);f.write('\n')
    print(json.dumps([{k: v for k, v in t.items() if k != 'independent'} for t in tests]))


if __name__ == '__main__':
    main()
