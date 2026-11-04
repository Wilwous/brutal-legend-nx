#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Conservative trace-envelope validation, never capture completeness certification."""
import argparse
import hashlib
import json
import re
import sys
COVERAGE = {'guest_scalar', 'guest_vector_atomic', 'indirect_alias', 'loader_host', 'hle_host', 'apc_callbacks', 'tls_pcr', 'threads_exceptions', 'other_agents'}
CLASSES = {'HARDWARE_OBSERVATION', 'INDEPENDENT_EMULATOR_OBSERVATION', 'IMPLEMENTATION_SPECIFIC_TRACE', 'STATIC_SOURCE_CONTRACT', 'SYNTHETIC_ONLY', 'UNQUALIFIED'}

def digest(trace):
    return hashlib.sha256(json.dumps(trace, sort_keys=True, separators=(',', ':')).encode()).hexdigest()

def require(ok, reason):
    if not ok:
        raise ValueError(reason)

def validate(t, expected_sha256):
    require(digest(t) == expected_sha256, 'corrupt/untrusted trace digest')
    h = t['capture']
    events = t['events']
    require(t['schema'] == 1 and h['evidence_class'] in CLASSES, 'trace schema/class')
    require(0 < len(events) <= 256 and h['record_count'] == len(events) and (h['dropped_events'] == 0), 'missing/dropped event records')
    require(re.fullmatch('[0-9a-f]{64}', h['module_sha256']) and h['tool'] and h['revision'] and re.fullmatch('[0-9a-f]{64}', h['configuration_sha256']) and re.fullmatch('[0-9a-f]{64}', h['collector_sha256']), 'capture identity')
    require(set(h['coverage']) == COVERAGE and all((v['status'] == 'complete' and v['evidence'] for v in h['coverage'].values())), 'incomplete/unqualified path coverage')
    require(events[0]['kind'] == 'window_start' and events[0]['phase'] == 'before_materialization' and (events[-1]['kind'] == 'window_end'), 'missing early capture boundary')
    require(events[-1]['producer_count'] == len(events), 'producer/collector count mismatch')
    target = h['target']
    require(target['size'] == 8 and 0 < target['address'] < 2 ** 32 - 7, 'target window')
    known = {}
    allocs = {}
    threads = {}
    callbacks = {}
    enqueued = {}
    delivered = {}
    memory = []
    seen = set()
    last_stop = None
    for (i, e) in enumerate(events):
        require(e['seq'] == i and e['module'] == h['module_sha256'], 'event sequence/module identity')
        parents = e['after']
        require(all((p in known for p in parents)), 'invalid causal dependency')
        ancestors = set(parents)
        for p in parents:
            ancestors |= known[p]
        require(i == 0 or 0 in ancestors, 'event is outside the capture boundary')
        known[i] = ancestors
        k = e['kind']
        seen.add(k)
        require(k in {'window_start', 'window_end', 'alloc', 'commit', 'protect', 'free', 'materialize', 'imports', 'tls', 'pcr', 'thread_start', 'apc_enqueue', 'apc_deliver', 'callback_enter', 'callback_exit', 'startup', 'read', 'write', 'exception', 'stop'}, 'unknown event kind')
        if k == 'thread_start':
            require(e['thread'] not in threads and e['thread'], 'thread identity/reuse')
            threads[e['thread']] = i
        if k in {'alloc', 'commit', 'protect', 'free', 'read', 'write'}:
            token = e['allocation']
            require(e['owner'] and e['generation'] > 0, 'missing allocation provenance')
            if k == 'alloc':
                require(token not in allocs and e['size'] > 0 and (0 < e['address'] < 2 ** 32) and (e['address'] + e['size'] <= 2 ** 32), 'allocation extent/reuse')
                require(not any((a['live'] and e['address'] < a['base'] + a['size'] and (a['base'] < e['address'] + e['size']) for a in allocs.values())), 'overlapping logical owners')
                allocs[token] = {'owner': e['owner'], 'generation': e['generation'], 'base': e['address'], 'size': e['size'], 'live': True, 'committed': False, 'permissions': 'none', 'last': i}
            else:
                require(token in allocs, 'unknown allocation')
                a = allocs[token]
                require(a['live'] and a['owner'] == e['owner'] and (a['generation'] == e['generation']), 'invalid/stale owner transition')
                require(a['last'] in ancestors, 'ownership ordering is not causal')
                if k in {'commit', 'protect'}:
                    require(all(old['seq'] in ancestors for old in memory if old['allocation'] == token), 'permission transition lacks causal access boundary')
                    require(k == 'commit' or a['committed'], 'protect before commitment')
                    require(e['permissions'] in {'none', 'r', 'rw'}, 'unsupported permission')
                    a['committed'] = True
                    a['permissions'] = e['permissions']
                    a['last'] = i
                elif k == 'free':
                    require(not callbacks, 'free during unresolved callback')
                    require(all((old['seq'] in ancestors for old in memory if old['allocation'] == token)), 'free lacks causal end of accesses')
                    a['live'] = False
                    a['last'] = i
                else:
                    require(a['committed'] and a['permissions'] in ({'r', 'rw'} if k == 'read' else {'rw'}), 'uncommitted/protected access')
                    require(e['width'] > 0 and a['base'] <= e['address'] and (e['address'] + e['width'] <= a['base'] + a['size']), 'memory extent')
                    require(e['byte_order'] == 'big' and bool(e['pc_or_origin']), 'missing byte ordering/event location')
                    require(re.fullmatch('[0-9a-f]{64}', e['value_sha256']) and e['origin'] in {'guest', 'host_loader', 'hle', 'device'}, 'memory value/origin')
                    if e['origin'] == 'guest':
                        require(e.get('thread') in threads and threads[e['thread']] in ancestors, 'missing guest thread/causal start')
                    for old in memory:
                        overlap = e['address'] < old['address'] + old['width'] and old['address'] < e['address'] + e['width']
                        require(not overlap or k == old['kind'] == 'read' or old['seq'] in ancestors, 'unordered conflicting access; timestamps are insufficient')
                    memory.append(e)
        if k in {'apc_enqueue', 'apc_deliver', 'callback_enter', 'callback_exit'}:
            require(threads.get(e.get('thread')) in ancestors, 'callback without causal thread start')
            key = e['callback']
            require(key in h['known_callbacks'] and e.get('thread') in threads, 'unknown callback/thread behavior')
            if k == 'apc_enqueue':
                enqueued[key] = i
            elif k == 'apc_deliver':
                require(key in enqueued and enqueued[key] in ancestors, 'APC delivery lacks causal enqueue')
                delivered[key] = i
            elif k == 'callback_enter':
                require(key in delivered and delivered[key] in ancestors and (key not in callbacks), 'callback entry lacks delivery')
                callbacks[key] = i
            else:
                require(key in callbacks and callbacks[key] in ancestors, 'callback exit mismatch')
                del callbacks[key]
        if k in {'startup', 'stop'}:
            require(e.get('thread') in threads and threads[e['thread']] in ancestors, 'missing boundary thread')
        if k == 'startup':
            require(not callbacks and {'materialize', 'imports', 'tls', 'pcr'} <= seen, 'missing initialization/callback boundary')
            require(all((any((events[p]['kind'] == needed for p in ancestors)) for needed in ['materialize', 'imports', 'tls', 'pcr'])), 'noncausal startup ordering')
        if k == 'stop':
            require(any((events[p]['kind'] == 'startup' for p in ancestors)), 'stop lacks causal startup')
            last_stop = i
        if k == 'window_end':
            require(last_stop in ancestors and (not callbacks), 'missing stop or outstanding callback')
    require({'startup', 'stop', 'read', 'write', 'thread_start'} <= seen, 'missing required observation kinds')
    pair = h['pair_writes']
    require(len(pair) == 2 and all((isinstance(n, int) and 0 <= n < len(events) for n in pair)), 'missing store anchors')
    (first, second) = (events[n] for n in pair)
    require(first['kind'] == second['kind'] == 'write' and first['width'] == second['width'] == 4 and (first['address'] == target['address']) and (second['address'] == target['address'] + 4) and (pair[0] in known[pair[1]]), 'unproved target pair ordering/extent')
    observer = h['observation']
    require(isinstance(observer, int) and 0 <= observer < len(events) and (events[observer]['kind'] in {'read', 'stop'}) and (pair[1] in known[observer]) and (observer in known[len(events) - 1]), 'missing post-pair observation boundary')
    early = [e['seq'] for e in memory if e['kind'] == 'read' and e['address'] < target['address'] + 8 and (target['address'] < e['address'] + e['width']) and (pair[1] not in known[e['seq']])]
    return {'earlier_or_unordered_target_read_records': early, 'envelope': 'CONSISTENT', 'evidence_class': h['evidence_class'], 'qualification': 'EXTERNAL_REVIEW_REQUIRED', 'gate_impact': 'NONE', 'events': len(events)}
if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('trace')
    ap.add_argument('--sha256', required=True)
    a = ap.parse_args()
    try:
        with open(a.trace) as f:
            t = json.load(f)
        print(json.dumps(validate(t, a.sha256)))
    except (ValueError, KeyError, TypeError, IndexError) as exc:
        print(json.dumps({'envelope': 'REFUSED', 'reason': str(exc), 'gate_impact': 'NONE'}))
        sys.exit(1)
