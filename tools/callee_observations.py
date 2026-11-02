#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Finite nonexecuting callee observations; every unknown call is a barrier.

No discovery, runtime admission, concrete memory reads, code generation or guest
execution. Input ranges and hashes require an independent authority. Results for
proprietary inputs must remain private. CFG membership is not runtime reachability.
"""
import copy
import hashlib
from dataclasses import asdict, dataclass
from bounded_cfg import decode as old_decode, signed


def pdata_entry(raw, base, rva, size, target):
    """Read one CE packed entry from an externally verified Exception directory.

    Absolute BE entry addresses and packed lengths are the selected format, not
    guessed nearest-symbol bounds. This metadata does not grant execution rights.
    """
    if (base < 0 or base+len(raw) > 2**32 or rva < 0 or rva % 4 or size <= 0 or size % 8 or size//8 > 65536
            or rva+size > len(raw) or target % 4 or not base <= target < base+len(raw)):
        raise ValueError('invalid explicit exception directory/target')
    matches = []
    for off in range(rva, rva+size, 8):
        start = int.from_bytes(raw[off:off+4], 'big')
        if start != target:
            continue
        packed = int.from_bytes(raw[off+4:off+8], 'big')
        words, prolog = (packed >> 8) & 0x3fffff, packed & 255
        if not (packed >> 30) & 1 or packed >> 31:
            raise ValueError('different instruction format or exceptional extent needs separate authority')
        if not words or prolog > words or target+words*4 > base+len(raw):
            raise ValueError('invalid matching packed function extent')
        matches.append({'entry': start, 'end': start+words*4, 'instructions': words,
                        'prolog_words': prolog, 'exception_flag': bool(packed >> 31),
                        'instruction_flag': bool((packed >> 30) & 1),
                        'source_offset': off, 'packed': packed})
    if len(matches) != 1:
        raise ValueError('missing or ambiguous exact entry metadata')
    return matches[0]


def decode(pc, w):
    if pc & 3 or not 0 <= pc <= 0xfffffffc or not 0 <= w <= 0xffffffff:
        raise ValueError('invalid instruction identity')
    op, rt, ra, rb = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
    r = {'pc': pc, 'word': w, 'kind': 'linear', 'memory': None}
    if op == 16:
        bo, bi = rt, ra
        if w & 1 or bo not in (4, 12, 16) or (bo == 16 and bi) or (bo != 16 and bi % 4 not in (0, 2)):
            raise ValueError('unsupported conditional branch form')
        target = signed(w & 0xfffc, 16) + (0 if w & 2 else pc)
        if not 0 <= target <= 0xfffffffc:
            raise ValueError('branch wrap')
        r.update(kind='conditional', target=target, bo=bo, bi=bi,
                 text=('bdnz' if bo == 16 else ('bt' if bo == 12 else 'bf')+f' crbit{bi}')+f' 0x{target:x}')
    elif op in (10, 11):
        if w & (3 << 21):
            raise ValueError('unsupported compare width/reserved bit')
        r.update(compare={'field': (w >> 23) & 7, 'reg': ra,
                          'unsigned': op == 10, 'immediate': w & 65535 if op == 10 else signed(w & 65535, 16)},
                 text=('cmplwi' if op == 10 else 'cmpwi')+f' cr{(w>>23)&7},r{ra},'+str(w & 65535 if op == 10 else signed(w & 65535, 16)))
    elif op == 62 and w & 3 == 1:
        if not ra:
            raise ValueError('invalid update base')
        r.update(memory={'width': 8, 'write': True, 'rt': rt, 'ra': ra,
                         'offset': signed(w & 0xfffc, 16), 'update': True},
                 text=f'stdu r{rt},{signed(w&0xfffc,16)}(r{ra})')
    elif op == 12:
        r.update(rt=rt, ra=ra, immediate=signed(w & 65535, 16),
                 text=f'addic r{rt},r{ra},{signed(w&65535,16)}')
    elif op == 31 and (w >> 1) & 1023 == 136 and not w & 1:
        r.update(rt=rt, ra=ra, rb=rb, text=f'subfe r{rt},r{ra},r{rb}')
    else:
        r = old_decode(pc, w)
    reads, writes = set(), set()
    m = r['memory']
    if m:
        if m['ra']:
            reads.add(f'r{m["ra"]}')
        if m['write']:
            reads.add(f'r{m["rt"]}')
        else:
            writes.add(f'r{m["rt"]}')
        if m['update']:
            if not m['ra']:
                raise ValueError('invalid update base')
            writes.add(f'r{m["ra"]}')
    elif 'compare' in r:
        reads.update((f'r{ra}', 'XER.SO'));writes.add(f'CR{r["compare"]["field"]}')
    elif op in (14, 15, 12):
        if ra or op == 12:
            reads.add(f'r{ra}')
        writes.add(f'r{rt}')
        if op == 12:
            writes.add('XER.CA')
    elif op == 24 and w != 0x60000000:
        reads.add(f'r{rt}');writes.add(f'r{ra}')
    elif op == 31:
        xo = (w >> 1) & 1023
        if xo == 339:
            reads.add('LR');writes.add(f'r{rt}')
        elif xo == 467:
            reads.add(f'r{rt}');writes.add('LR' if r['text'].startswith('mtlr') else 'CTR')
        elif xo == 444:
            reads.add(f'r{rt}');writes.add(f'r{ra}')
        elif xo == 136:
            reads.update((f'r{ra}', f'r{rb}', 'XER.CA'));writes.update((f'r{rt}', 'XER.CA'))
    if r['kind'] == 'conditional':
        if r['bo'] == 16:
            reads.add('CTR');writes.add('CTR')
        else:
            reads.add(f'CR{r["bi"]//4}.{r["bi"]%4}')
    elif r['kind'] == 'call':
        writes.add('LR')
    elif r['kind'] == 'return':
        reads.add('LR')
    elif r['kind'] in ('indirect_call', 'indirect_branch'):
        reads.add('CTR')
    return {**r, 'reads': sorted(reads), 'writes': sorted(writes)}


@dataclass(frozen=True)
class Value:
    symbol: str = ''
    offset: int = 0
    bits: int = 64

    def plus(self, delta):
        return Value(self.symbol, self.offset+delta if self.symbol else (self.offset+delta) % (1 << self.bits), self.bits)

    def low(self, bits):
        if not self.symbol:
            return Value(offset=self.offset % (1 << bits), bits=bits)
        if bits >= self.bits:
            return self
        return Value(f'low{bits}({self.symbol}+{self.offset})', bits=bits)


def inspect(raw, base, entry, end, expected_sha256, *, initial_lr=None,
            site_budget=96, step_budget=128, path_budget=16):
    """Decode an exact metadata-bound range, then propagate only call-free paths.

    Loads are symbolic obligations, never values read from a saved image. A direct
    call records arguments and stops before effects; no ABI clobber summary or
    guessed fallthrough is supplied. Branch alternatives are POSSIBLE under stated
    guards and successful memory operations, never proven hardware reachability.
    Integer stack offsets require external no-wrap/nonalias/live-memory proofs.
    """
    if hashlib.sha256(raw).hexdigest() != expected_sha256:
        raise ValueError('untrusted source identity')
    if (base < 0 or base+len(raw) > 2**32 or entry % 4 or end % 4 or entry < base
            or end > base+len(raw) or entry >= end or not 1 <= site_budget <= 256
            or (end-entry)//4 > site_budget or not 1 <= step_budget <= 512
            or not 1 <= path_budget <= 64):
        raise ValueError('range/finite budget')
    rows = [decode(pc, int.from_bytes(raw[pc-base:pc-base+4], 'big')) for pc in range(entry, end, 4)]
    bypc = {r['pc']: r for r in rows};edges=[]
    for r in rows:
        kind, pc = r['kind'], r['pc']
        targets = [r['target']] if kind == 'branch' else [pc+4, r['target']] if kind == 'conditional' else [] if kind in ('return', 'indirect_call', 'indirect_branch') else [pc+4]
        for target in targets:
            if target not in bypc:
                raise ValueError('local CFG escape/incomplete metadata range')
            edges.append({'from': pc, 'to': target,
                          'requires_unproved_call_return': kind == 'call'})
    values = {f'r{i}': Value(f'r{i}') for i in range(32)}
    values.update({'LR': Value('LR'), 'CTR': Value('CTR'), 'XER.CA': Value('XER.CA'), 'XER.SO': Value('XER.SO')})
    if initial_lr is not None:
        if initial_lr % 4 or not 0 <= initial_lr <= 0xfffffffc:
            raise ValueError('invalid externally established incoming LR')
        values['LR'] = Value(offset=initial_lr)
    initial={'pc':entry, 'values':values, 'defined':set(), 'cr':{}, 'shadow':{},
             'route':[], 'memory':[], 'guards':[], 'inputs':set()}
    todo=[initial];frontiers=[];seen=set();transitions=0
    while todo:
        s=todo.pop()
        if len(s['route']) >= step_budget:
            raise ValueError('path step budget; no completion claim')
        r=bypc[s['pc']];pc=r['pc'];w=r['word'];op=w>>26
        s['route'].append(pc);seen.add(pc);transitions+=1
        if transitions > step_budget*path_budget:
            raise ValueError('total analysis budget')
        def read(name):
            if name not in s['defined']:
                s['inputs'].add(name)
            return s['values'].get(name, Value(name))
        def write(name, value):
            s['values'][name]=value;s['defined'].add(name)
        def finish(reason):
            frontiers.append({'reason':reason, 'pc':pc, 'route':s['route'], 'guards':s['guards'],
                              'read_before_definition':sorted(s['inputs']), 'memory':s['memory'],
                              'registers':{k:asdict(v) for k,v in s['values'].items()},
                              'target':r.get('target'), 'call_continuation':pc+4 if r['kind']=='call' else None})
        if r['kind'] in ('call', 'indirect_call', 'indirect_branch'):
            finish('unproved call barrier' if r['kind']=='call' else 'unsupported computed transfer')
            continue
        if r['kind']=='return':
            read('LR');finish('conditional local return; external memory/context obligations remain')
            continue
        next_pc=pc+4;m=r['memory'];rt=(w>>21)&31;ra=(w>>16)&31
        if m:
            address=(read(f'r{m["ra"]}') if m['ra'] else Value()).plus(m['offset'])
            address=Value(address.symbol,address.offset % 2**32 if not address.symbol else address.offset,32)
            key=(address.symbol,address.offset,m['width'])
            if m['write']:
                value=read(f'r{m["rt"]}')
                # Unknown-address stores can alias any prior cell. Even a fixed
                # address may alias a symbolic stack until nonaliasing is proved.
                for old in list(s['shadow']):
                    if old[0]!=key[0] or old[1]<key[1]+key[2] and key[1]<old[1]+old[2]:
                        del s['shadow'][old]
                s['shadow'][key]=value.low(m['width']*8)
            else:
                value=s['shadow'].get(key,Value(f'memory{m["width"]*8}@{pc:x}',bits=m['width']*8))
                write(f'r{m["rt"]}',value)
            s['memory'].append({'pc':pc,'write':m['write'],'width':m['width'],
                                'address':asdict(address),'value':asdict(value),
                                'obligation':'successful current owned/committed permitted access; source snapshots are not live providers'})
            if m['update']:
                write(f'r{m["ra"]}',address)
        elif 'compare' in r:
            c=r['compare'];s['cr'][c['field']]=(read(f'r{c["reg"]}'),c['immediate'],c['unsigned'])
            read('XER.SO')
        elif op in (14,15):
            value=read(f'r{ra}') if ra else Value()
            write(f'r{rt}',value.plus(signed(w&65535,16)*(65536 if op==15 else 1)))
        elif op==24 and w!=0x60000000:
            value=read(f'r{rt}');imm=w&65535
            write(f'r{ra}',Value(offset=value.offset|imm,bits=value.bits) if not value.symbol else value if not imm else Value(f'ori@{pc:x}'))
        elif op==31:
            xo=(w>>1)&1023
            if xo==339:write(f'r{rt}',read('LR'))
            elif xo==467:write('LR' if r['text'].startswith('mtlr') else 'CTR',read(f'r{rt}'))
            elif xo==444:write(f'r{ra}',read(f'r{rt}'))
            else:
                for name in r['reads']:read(name)
                for name in r['writes']:write(name,Value(f'opaque_{name}@{pc:x}'))
        elif op==12:
            read(f'r{ra}');write(f'r{rt}',Value(f'addic@{pc:x}'));write('XER.CA',Value(f'carry@{pc:x}'))
        if r['kind']=='branch':next_pc=r['target']
        elif r['kind']=='conditional':
            result=None
            if r['bo']==16:
                count=read('CTR');count=count.plus(-1);write('CTR',count)
                if not count.symbol:result=bool(count.offset & 0xffffffff)
                guard='decremented CTR.low32 != 0'
            else:
                comp=s['cr'].get(r['bi']//4);guard=f'CR{r["bi"]//4}.{r["bi"]%4} == {int(r["bo"]==12)}'
                if comp:
                    value,imm,unsigned=comp;guard=f'{asdict(value)} '+('== ' if r['bi']%4==2 else '< ')+str(imm)
                    if not value.symbol:
                        number=value.offset & 0xffffffff
                        if not unsigned:number=signed(number,32)
                        bit=number==imm if r['bi']%4==2 else number<imm
                        result=bit if r['bo']==12 else not bit
                # SO/other fields are not guessed; missing compare forks remain
                # explicit incoming condition dependencies.
                if comp is None:s['inputs'].add(f'CR{r["bi"]//4}.{r["bi"]%4}')
            if result is None:
                if len(todo)+len(frontiers)+2>path_budget:
                    raise ValueError('path budget')
                branch=copy.deepcopy(s);branch['pc']=r['target'];branch['guards'].append({'condition':guard,'branch_taken':True,'bo':r['bo'],'predicate_value':None if r['bo']==16 else r['bo']==12});todo.append(branch)
                s['guards'].append({'condition':guard,'branch_taken':False,'bo':r['bo'],'predicate_value':None if r['bo']==16 else r['bo']!=12})
            elif result:next_pc=r['target']
        s['pc']=next_pc;todo.append(s)
    return {'source_sha256':expected_sha256,'entry':entry,'end':end,'decoded_sites':rows,
            'local_CFG':edges,'call_free_prefix_sites':sorted(seen),'frontiers':frontiers,
            'abstract_transitions':transitions,'budgets':{'sites':site_budget,'steps_per_path':step_budget,'paths':path_budget},
            'structural_site_count':len(rows),'prefix_site_count':len(seen),
            'reachability':'potential call-free paths with symbolic loads/guards and successful-access preconditions; NOT actual runtime reachability',
            'call_summaries_assumed':False,'execution_admitted':False,'guest_instructions_executed':0}
