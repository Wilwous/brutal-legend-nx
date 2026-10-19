#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Compile the pinned SDK emitter in isolation; use a separate numeric oracle.

No game input, generated game source, runtime, network or Xbox keys are needed.
The oracle is IEEE binary16 round-toward-zero, NOT asserted Xbox hardware truth.
Known disagreements are reported explicitly and checked as characterization.
"""
import argparse
import bisect
import collections
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import subprocess

PIN = 'b3528ed4a7e0642e2f390cf3d745e18d6e496f14d1707afef6f7049bb84fb39d'
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def run(cmd, **kw):
    try:
        return subprocess.run(cmd, check=True, timeout=30, capture_output=True, text=True, **kw)
    except subprocess.CalledProcessError as exc:
        raise RuntimeError('Child failed with exit '+str(exc.returncode)+'\n'+exc.stderr) from exc


def half_value(h):
    # Numeric definition: no source-code sharing with the emitter's bit algorithm.
    exponent, fraction = divmod(h, 1024)
    return math.ldexp(fraction, -24) if exponent == 0 else math.ldexp(1 + fraction / 1024, exponent - 15)


FINITE = [half_value(h) for h in range(0x7c00)]


def reference(bits):
    sign = (bits >> 16) & 0x8000
    value = struct.unpack('>f', bits.to_bytes(4, 'big'))[0]
    if math.isnan(value):
        return sign | 0x7e00
    if math.isinf(value):
        return sign | 0x7c00
    # Saturating finite overflow is IEEE round-toward-zero, infinity is preserved.
    return sign | min(0x7bff, bisect.bisect_right(FINITE, abs(value)) - 1)


def category(bits):
    mag = bits & 0x7fffffff
    if mag > 0x7f800000:
        return 'nan_encoding'
    if mag == 0x7f800000:
        return 'infinity'
    if mag > 0x477fe000:
        return 'finite_overflow'
    return 'finite_in_range'


def expected_vector(src, initial, shift):
    h = list(map(reference, src))
    words = initial.copy()
    words[2-shift:4-shift] = [(h[0] << 16) | h[1], (h[2] << 16) | h[3]]
    return words


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--sdk-source', type=Path, required=True, help='Pinned open-source vector.cpp')
    ap.add_argument('--sdk-include', type=Path, required=True, help='SDK include directory with fmt headers')
    ap.add_argument('--workdir', type=Path, required=True, help='Private output directory outside checkout')
    ap.add_argument('--compiler', default='clang++')
    ap.add_argument('--optimization', choices=['0', '2'], default='0')
    ap.add_argument('--sanitize', action='store_true')
    args = ap.parse_args()
    work = args.workdir.resolve()
    if work == ROOT or ROOT in work.parents:
        ap.error('Output must stay outside the public checkout')
    work.mkdir(parents=True, exist_ok=True)
    raw = args.sdk_source.read_bytes()
    if hashlib.sha256(raw).hexdigest() != PIN:
        ap.error('SDK source hash mismatch: do not silently test another version')
    source = raw.decode()
    start = source.index('    case 5:  // float16_4', source.index('bool build_vpkd3d128'))
    end = source.index('    case 6:', start)
    (work/'emitter_case.inc').write_text(source[start:end])
    flags = ['-std=c++20', '-O'+args.optimization, '-Wall', '-Wextra']
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    cmd = [args.compiler, *flags, '-I'+str(args.sdk_include.resolve()), '-I'+str(work),
           str(HERE/'emit_fixture.cpp'), '-o', str(work/'emitter')]
    build = run(cmd)
    emitted = run([str(work/'emitter')]).stdout
    (work/'synthetic_subject.cpp').write_text(emitted)
    build_subject = run([args.compiler, *flags, str(work/'synthetic_subject.cpp'), '-o', str(work/'subject')])
    rng = random.Random(0x42524C4E58)
    directed = [0, 0x80000000, 0x3f800000, 0xbf800000, 0x3f801000,
                0x3f801001, 0x3f803000, 0x7f800000, 0xff800000,
                0x7fc00000, 0xffc00000, 0x7f800001, 0xffffffff,
                1, 0x80000001, 0x007fffff, 0x33800000, 0x33000000,
                0x387fc000, 0x38800000, 0x477fe000, 0xc77fe000,
                0x477fe001, 0x477ff000, 0x7f7fffff]
    # Exhaustive positive finite half values expanded exactly to binary32,
    # plus adjacent binary32 neighbors on both sides. Random bit patterns below include signs.
    boundary = []
    for h in range(1, 0x7c00):
        b = int.from_bytes(struct.pack('>f', FINITE[h]), 'big')
        boundary += [b-1, b, b+1]
    bits = directed + boundary + [rng.getrandbits(32) for _ in range(10000)]
    sentinel = [0x11223344, 0x55667788, 0x99aabbcc, 0xddeeff00]
    cases = []
    for b in bits:
        for mode in [0, 6]:
            cases.append((mode, [b]*4, sentinel, 'scalar'))
    vectors = [[0x3f800000, 0x40000000, 0x40400000, 0x40800000],
               [0, 0x80000000, 0x33800000, 0x477fe000]]
    vectors += [[int.from_bytes(struct.pack('>f', rng.uniform(-100, 100)), 'big') for _ in range(4)] for _ in range(128)]
    for v in vectors:
        for mode in range(12):
            cases.append((mode, v, sentinel, 'vector'))
    payload = ''.join(str(mode)+' '+' '.join(f'{w:08x}' for w in src+dst)+'\n' for mode,src,dst,_ in cases)
    observed = run([str(work/'subject')], input=payload)
    outputs = [[int(w,16) for w in line.split()] for line in observed.stdout.splitlines()]
    assert len(outputs) == len(cases), 'Truncated subject output'
    scalar = collections.Counter()
    vector = collections.Counter()
    examples = {}
    for (mode, src, dst, kind), actual in zip(cases, outputs):
        shift = (mode % 6)//2
        alias = mode % 2 == 1
        expect = expected_vector(src, src if alias else dst, shift)
        if kind == 'scalar':
            cat = category(src[0]); scalar[cat+'_cases'] += 1
            if actual != expect:
                scalar[cat+'_differences'] += 1
                examples.setdefault(cat, {'input':f'{src[0]:08x}', 'expected':[f'{x:08x}' for x in expect], 'observed':[f'{x:08x}' for x in actual]})
        else:
            key = ('alias' if alias else 'distinct')+'_shift'+str(shift)
            vector[key+'_cases'] += 1
            if actual != expect:
                vector[key+'_differences'] += 1
                examples.setdefault(key, {'input':[f'{x:08x}' for x in src], 'expected':[f'{x:08x}' for x in expect], 'observed':[f'{x:08x}' for x in actual]})
    # Independent oracle self-checks, not obtained from the implementation under test.
    assert reference(0x3f800000) == 0x3c00
    assert reference(0x33800000) == 1 and reference(0x33000000) == 0
    assert reference(0x80000000) == 0x8000
    assert reference(0x7f800000) == 0x7c00
    assert scalar['finite_in_range_differences'] == 0, examples
    assert scalar['infinity_differences'] == scalar['infinity_cases']
    assert scalar['finite_overflow_differences'] == scalar['finite_overflow_cases']
    for shift in range(3):
        assert vector['distinct_shift'+str(shift)+'_differences'] == 0, examples
    assert vector['alias_shift0_differences'] > 0, 'Known alias defect unexpectedly changed'
    # All scalar mask-2/mask-3 pairs must match exactly, including unverified cases.
    for i in range(0, 2*len(bits), 2):
        assert outputs[i] == outputs[i+1], 'Unexpected mask 2/3 divergence'
    result = {'status':'PASS characterization; hardware equivalence UNKNOWN',
              'compiler':run([args.compiler,'--version']).stdout.splitlines()[0],
              'optimization':args.optimization,'sanitizers':args.sanitize,
              'sdk_builder_sha256':PIN,'scalar_input_count':len(bits),
              'random_seed':'0x42524c4e58','random_bit_patterns':10000,
              'scalar':dict(scalar),'vectors':dict(vector),'examples':examples,
              'compiler_diagnostics':build.stderr+build_subject.stderr,
              'subject_diagnostics':observed.stderr}
    (work/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__ == '__main__':
    main()
