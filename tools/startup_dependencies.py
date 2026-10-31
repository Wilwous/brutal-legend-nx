#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Nonexecuting, bounded scalar PPC dependency analysis. Never an admission API.

Input bytes/PCs require an external trusted identity. No machine code is invoked,
generated or mapped. Only a single closed normal route is supported; conditional,
unknown computed, repeated and exceptional paths are refused, not guessed.
Effective addresses are low-32-bit expressions. Affine intervals require a separate
no-wrap/range/ownership/permission contract; they are not resident capabilities.
"""
import hashlib
from dataclasses import asdict, dataclass
from bounded_cfg import decode, signed

MASK = (1 << 64) - 1


@dataclass(frozen=True)
class Value:
    kind: str
    base: str = ''
    amount: int = 0
    bits: int = 64
    dependencies: tuple = ()

    def plus(self, amount):
        if self.kind == 'constant':
            return constant((self.amount + amount) & ((1 << self.bits) - 1), self.bits)
        if self.kind == 'affine':
            return Value('affine', self.base, self.amount + amount, self.bits, self.dependencies)
        return self


def constant(value, bits=64):
    return Value('constant', amount=value & ((1 << bits) - 1), bits=bits)


def incoming(name):
    return Value('affine', name, dependencies=(name,))


def describe(value):
    # Public proof representation must survive JSON storage/reload exactly.
    return {**asdict(value), 'dependencies': list(value.dependencies)}


def intervals(spans):
    """Exact union, preserving holes; spans are half-open symbolic offsets."""
    result = []
    for lo, hi in sorted(spans):
        if hi <= lo:
            raise ValueError('invalid footprint interval')
        if result and lo <= result[-1][1]:
            result[-1][1] = max(result[-1][1], hi)
        else:
            result.append([lo, hi])
    return result


def covered(spans, begin, end):
    if begin >= end:
        raise ValueError('empty/reversed declared window')
    return all(begin <= lo and hi <= end for lo, hi in spans)


def analyze(raw, base, entry, normal_pcs, stop, expected_sha256):
    """Static abstract propagation, not guest execution or reachability proof.

    normal_pcs are an explicit allowlist, not a discovered function extent. Stop
    bytes are fingerprinted but their instruction is excluded from propagation.
    The returned dependency values/PCs must stay private for proprietary inputs.
    """
    digest = hashlib.sha256(raw).hexdigest()
    if digest != expected_sha256:
        raise ValueError('untrusted source identity')
    if not 0 <= base <= 0xffffffff or base + len(raw) > 0x100000000:
        raise ValueError('source coordinate overflow')
    pcs = list(normal_pcs)
    if not pcs or len(pcs) > 128 or len(set(pcs)) != len(pcs) or stop in pcs or entry not in pcs:
        raise ValueError('invalid normal/stop allowlist')

    def word(pc):
        if pc & 3 or pc < base or pc + 4 > base + len(raw):
            raise ValueError('PC outside exact source')
        return int.from_bytes(raw[pc-base:pc-base+4], 'big')

    stop_word = word(stop)
    decoded = {pc: decode(pc, word(pc)) for pc in pcs}
    values = {f'r{i}': incoming(f'r{i}') for i in range(32)}
    values['LR'] = incoming('LR')
    defined, observed, effect_inputs = set(), set(), set()
    route, memory, calls, returns, call_stack = [], [], [], [], []
    pc = entry
    while pc != stop:
        if pc not in decoded or any(r['pc'] == pc for r in route):
            raise ValueError('escape/repeated site in bounded normal route')
        d = decoded[pc]
        w = d['word']; op = w >> 26
        rt, ra, rb = (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
        reads, writes, before = [], [], {}
        defined_before = set(defined)

        def read(name):
            reads.append(name); before[name] = describe(values[name])
            if name not in defined:
                observed.add(name)
            return values[name]

        def write(name, value):
            writes.append(name); defined.add(name); values[name] = value

        next_pc = pc + 4
        m = d['memory']
        if m:
            if op not in (32, 36, 37, 58, 62) or (m['update'] and not ra):
                raise ValueError('unsupported/invalid memory form')
            address = (read(f'r{ra}') if ra else constant(0)).plus(m['offset'])
            address = Value(address.kind, address.base, address.amount & 0xffffffff if address.kind == 'constant' else address.amount, 32, address.dependencies)
            value = read(f'r{rt}') if m['write'] else None
            item = {'pc': pc, 'direction': 'write' if m['write'] else 'read', 'width': m['width'],
                    'base_register': ra, 'displacement': m['offset'], 'address': describe(address),
                    'payload': describe(value) if value else None, 'base_update_after_access': m['update'],
                    'natural_alignment_obligation': m['width'], 'alignment_proven_at_runtime': False,
                    'no_wrap_bounds_owner_permissions_lifetime': 'REQUIRED; not established by this analysis'}
            memory.append(item); effect_inputs.update(address.dependencies)
            if value:
                effect_inputs.update(value.dependencies)
            else:
                write(f'r{rt}', Value('unknown', dependencies=tuple(sorted(set(address.dependencies) | {f'memory@{pc:x}'}))))
            if m['update']:
                # Pinned emitter updates u32 after the store; upper bits are not
                # proven zero. Track low32 only, not an invented full GPR value.
                write(f'r{ra}', address)
        elif op in (14, 15):
            source = read(f'r{ra}') if ra else constant(0)
            write(f'r{rt}', source.plus(signed(w & 65535, 16) * (65536 if op == 15 else 1)))
        elif w == 0x60000000:
            pass  # Canonical nop has no observable context input or output.
        elif op == 24:
            source = read(f'r{rt}'); imm = w & 65535
            result = constant(source.amount | imm, source.bits) if source.kind == 'constant' else source if not imm else Value('unknown', dependencies=source.dependencies)
            write(f'r{ra}', result)
        elif op == 31:
            xo = (w >> 1) & 1023
            if w & 1:
                raise ValueError('record form not supported')
            if xo == 339 and d['text'].startswith('mflr '):
                write(f'r{rt}', read('LR'))
            elif xo == 444 and rt == rb:
                write(f'r{ra}', read(f'r{rt}'))
            else:
                raise ValueError('unsupported system/register operation')
        elif op == 18:
            next_pc = d['target']
            if w & 1:
                write('LR', constant(pc+4))
                call_stack.append(pc+4); calls.append({'pc': pc, 'target': next_pc, 'continuation': pc+4})
        elif w == 0x4e800020:
            lr = read('LR')
            if not call_stack or lr.kind != 'constant' or (lr.amount & 0xfffffffc) != call_stack[-1]:
                raise ValueError('unproven return target')
            next_pc = call_stack.pop(); returns.append({'pc': pc, 'target': next_pc, 'matched': True})
        else:
            raise ValueError('conditional/computed/unsupported path requires explicit refusal')
        route.append({'pc': pc, 'source_offset': pc-base, 'word': w, 'instruction': d['text'],
                      'instruction_sha256': hashlib.sha256(raw[pc-base:pc-base+4]).hexdigest(),
                      'reads': sorted(set(reads)), 'read_before_definition': sorted(set(reads)-defined_before),
                      'writes': sorted(set(writes)), 'incoming_values_at_site': before,
                      'written_values': {n: describe(values[n]) for n in writes}, 'next_pc': next_pc,
                      'exception_policy': 'No exceptional successor inferred; external fail-stop proof required'})
        pc = next_pc
    if call_stack or {r['pc'] for r in route} != set(pcs):
        raise ValueError('unfinished call or unused normal site')
    stack = intervals([(m['address']['amount'], m['address']['amount']+m['width']) for m in memory
                       if m['address']['kind'] == 'affine' and m['address']['base'] == 'r1'])
    return {'schema': 'static-startup-dependencies-v1', 'source_sha256': digest, 'base': base, 'entry': entry,
            'sites': route, 'memory': memory, 'calls': calls, 'returns': returns,
            'read_before_definition': sorted(observed), 'memory_effect_input_symbols': sorted(effect_inputs),
            'final_written_registers': {n: describe(values[n]) for n in sorted(defined)},
            'stack_intervals': stack, 'stack_distinct_bytes': sum(hi-lo for lo, hi in stack),
            'stack_envelope': [stack[0][0], stack[-1][1]] if stack else None,
            'excluded_stop': {'pc': stop, 'word': stop_word, 'instruction_sha256': hashlib.sha256(raw[stop-base:stop-base+4]).hexdigest(), 'executed': False},
            'alignment_and_no_wrap': 'obligations only; no incoming pointer or live mapping supplied',
            'whole_title_reachability': False, 'runtime_hooks_observed': False, 'execution_admitted': False}


def verify(report, raw, base, entry, pcs, stop, trusted_sha256):
    """Recompute against externally pinned bytes/coordinates, never a self-hash."""
    if report != analyze(raw, base, entry, pcs, stop, trusted_sha256):
        raise ValueError('changed static proof/binding')
    return 'STATIC CORRESPONDENCE ONLY; NO EXECUTION ADMISSION'
