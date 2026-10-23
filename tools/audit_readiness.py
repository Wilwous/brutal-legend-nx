#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Read-only lexical dependency/access audit. Detailed products MUST stay private.

No C++ parsing, branch feasibility, constant propagation, address reconstruction,
PPC execution or code rewriting. Instruction ordinals are diagnostic locations,
NOT guest PCs. Unknown operations/targets stay unknown. No output on stdout.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
DEFINITION = re.compile(r'DEFINE_REX_FUNC\((\w+)\)\s*\{')
CALL = re.compile(r'\b(\w+)\(ctx,\s*base\)')
MACRO = re.compile(r'\b(REX_(?:(?:MM_)?(?:LOAD|STORE)_\w+|RAW_ADDR|PARTIAL_VECTOR_GUARD))\s*\(')
INSTRUCTION = re.compile(r'^\t// ([a-z][a-z0-9.]*)(?=\s|$)')
ATOMIC = {'lwarx', 'ldarx', 'stwcx.', 'stdcx.'}
FULL_VECTOR = {'lvx', 'lvx128', 'stvx', 'stvx128'}
PARTIAL_VECTOR = {'lvlx', 'lvlx128', 'lvrx', 'lvrx128', 'stvlx', 'stvlx128',
                  'stvrx', 'stvrx128', 'lvewx', 'lvewx128', 'lvehx', 'lvebx',
                  'stvewx', 'stvewx128', 'stvehx', 'stvebx'}
HELPERS = re.compile(r'\b(ppc_trap|ppc_setjmp|ppc_longjmp|REX_UNIMPLEMENTED|REX_FATAL)\s*\(')


def operation(macro, mnemonic):
    """Eligibility is a codegen-seam candidate, never proof of mediation."""
    direction = 'write' if 'STORE' in macro else 'read'
    width = re.search(r'_U(8|16|32|64)$', macro)
    if width:
        return {'family': 'scalar', 'width_bytes': int(width[1]) // 8,
                'direction': direction, 'seam': 'eligible_after_regeneration',
                'alignment': 'effective address; runtime alignment unknown',
                'atomicity': 'ordinary', 'vector': 'none',
                'mmio': 'hint' if '_MM_' in macro else 'unknown_address'}
    if macro in {'REX_LOAD_VECTOR', 'REX_STORE_VECTOR'} or mnemonic in FULL_VECTOR:
        return {'family': 'full_vector', 'width_bytes': 16,
                'direction': 'write' if mnemonic.startswith('st') else direction,
                'seam': 'eligible_after_regeneration', 'alignment': 'PPC aligned-down full-vector semantics',
                'atomicity': 'ordinary', 'vector': 'full', 'mmio': 'unknown_address'}
    if mnemonic in ATOMIC:
        family = 'atomic'; width = 8 if mnemonic in {'ldarx', 'stdcx.'} else 4
        direction = 'conditional_write' if mnemonic.startswith('st') else 'reservation_read'
    elif mnemonic in PARTIAL_VECTOR or macro == 'REX_PARTIAL_VECTOR_GUARD':
        family = 'partial_vector'; width = None; direction = 'write' if mnemonic.startswith('st') else 'read'
    elif mnemonic in {'dcbz', 'dcbzl'}:
        family = 'cache_zero'; width = None; direction = 'write'
    else:
        family = 'unknown_raw'; width = None; direction = 'unknown'
    return {'family': family, 'width_bytes': width, 'direction': direction,
            'seam': 'reject_or_unverified', 'alignment': 'operation-specific; not certified',
            'atomicity': 'reservation/conditional' if family == 'atomic' else 'unknown',
            'vector': 'partial' if family == 'partial_vector' else 'none', 'mmio': 'unknown_address'}


def body_inventory(body):
    body = re.sub(r'/\*.*?\*/', '', body, flags=re.S)
    sites = []; helpers = []; indirect = []; ordinal = -1; mnemonic = ''; instructions = 0
    mnemonics = Counter()
    for line in body.splitlines():
        match = INSTRUCTION.match(line)
        if match:
            ordinal += 1; instructions += 1; mnemonic = match[1]; mnemonics[mnemonic] += 1
        # Comments cannot create access/call/helper observations.
        code = re.sub(r'//.*', '', line)
        for macro in MACRO.findall(code):
            sites.append({'instruction_ordinal': ordinal, 'mnemonic': mnemonic, 'macro': macro,
                          **operation(macro, mnemonic)})
        helpers.extend({'instruction_ordinal': ordinal, 'helper': h, 'mnemonic': mnemonic}
                       for h in HELPERS.findall(code))
        for target in re.findall(r'\bREX_CALL_INDIRECT_FUNC\(\s*([^)]*)\)', code):
            # Literal syntax alone does not prove an authorized mapping/signature.
            literal = bool(re.fullmatch(r'0x[0-9a-fA-F]+|[0-9]+', target.strip()))
            indirect.append({'instruction_ordinal': ordinal, 'literal_target': target.strip() if literal else None,
                             'resolution': 'literal_unadmitted' if literal else 'unresolved'})
    stripped = re.sub(r'//[^\n]*|/\*.*?\*/', '', body, flags=re.S)
    return {'calls': CALL.findall(stripped), 'instructions': instructions, 'sites': sites,
            'helpers': helpers, 'indirect': indirect, 'mnemonics': dict(sorted(mnemonics.items())),
            'direct_base_patterns': len(re.findall(r'\bbase\s*(?:\+|\[)|\b(?:GuestPtr|TranslateVirtual|TranslateRelative)\s*[<(]', stripped)),
            'labels': sorted(set(re.findall(r'\bloc_([0-9A-Fa-f]+):', stripped))),
            'gotos': re.findall(r'\bgoto loc_([0-9A-Fa-f]+);', stripped)}


def aggregate(rows):
    fields = ['macro', 'family', 'width_bytes', 'direction', 'alignment', 'atomicity', 'vector', 'mmio', 'seam']
    counts = {key: Counter() for key in fields}; helpers = Counter(); mnemonics = Counter()
    out = {key: 0 for key in ['macro_sites', 'instruction_comments', 'indirect_sites',
                              'literal_indirect_targets', 'direct_base_patterns', 'label_identities', 'goto_sites']}
    for row in rows:
        for site in row['sites']:
            out['macro_sites'] += 1
            for key in fields: counts[key][str(site[key])] += 1
        out['instruction_comments'] += row['instructions']
        out['indirect_sites'] += len(row['indirect'])
        out['literal_indirect_targets'] += sum(x['literal_target'] is not None for x in row['indirect'])
        out['direct_base_patterns'] += row['direct_base_patterns']
        out['label_identities'] += len(row['labels']); out['goto_sites'] += len(row['gotos'])
        helpers.update(x['helper'] for x in row['helpers']); mnemonics.update(row['mnemonics'])
    for key, count in counts.items():
        out['by_'+('width_bytes' if key == 'width_bytes' else key)] = dict(sorted(count.items()))
    out['helper_sites'] = dict(sorted(helpers.items()))
    out['mnemonic_comments'] = dict(sorted(mnemonics.items()))
    return out


def analyze(directory, entry='xstart'):
    edges = {}; digests = []
    def bodies(fingerprint=False):
        for path in sorted(directory.rglob('*recomp*.cpp')):
            raw = path.read_bytes()
            if fingerprint: digests.append(hashlib.sha256(raw).hexdigest())
            source = raw.decode(); matches = list(DEFINITION.finditer(source))
            for i, match in enumerate(matches):
                yield match[1], source[match.end():matches[i+1].start() if i+1<len(matches) else len(source)]
    def rows():
        for name, body in bodies(True):
            if name in edges: raise ValueError('duplicate emitted function')
            row = body_inventory(body); edges[name] = row['calls']; yield row
    whole = aggregate(rows())
    if entry not in edges: raise ValueError('entry absent')
    seen = set(); todo = [entry]
    while todo:
        name = todo.pop()
        if name in seen: continue
        seen.add(name); todo.extend(edges.get(name, []))
    names = sorted(seen & edges.keys())
    closure = {name: body_inventory(body) for name, body in bodies() if name in seen}
    summary = {'classification': 'PARTIAL', 'partitions': len(digests), 'functions': len(edges),
               'input_content_digest': hashlib.sha256(''.join(digests).encode()).hexdigest(),
               'closure_functions': len(names),
               'closure_imports': sorted(n.removeprefix('__imp__') for n in seen if n.startswith('__imp__')),
               'missing_literal_symbols': sorted(seen - edges.keys() - {n for n in seen if n.startswith('__imp__')}),
               'all': whole, 'closure': aggregate(closure.values()),
               'mediation': 'not established by lexical analysis',
               'limits': 'Lexical closure only; no proven execution, CFG completeness, pointer values or guest PCs reconstructed.'}
    return {'summary': summary, 'entry_symbol': entry, 'closure': dict(sorted(closure.items()))}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--generated', required=True, type=Path)
    ap.add_argument('--entry', default='xstart')
    ap.add_argument('--output', required=True, type=Path)
    a = ap.parse_args(); output = a.output.resolve()
    if output == ROOT or ROOT in output.parents: ap.error('private output required')
    result = analyze(a.generated, a.entry)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n')


if __name__ == '__main__': main()
