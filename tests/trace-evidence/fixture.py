# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
import hashlib
COVERAGE = ['guest_scalar', 'guest_vector_atomic', 'indirect_alias', 'loader_host', 'hle_host', 'apc_callbacks', 'tls_pcr', 'threads_exceptions', 'other_agents']

def fixture():
    module = '1' * 64
    events = []

    def add(kind, **fields):
        n = len(events)
        events.append({'seq': n, 'kind': kind, 'module': module, 'after': [n - 1] if n else [], **fields})
    owner = {'allocation': 'original-region-1', 'owner': 'module-instance-1', 'generation': 1}
    add('window_start', phase='before_materialization')
    add('alloc', address=1073741824, size=8, **owner)
    add('commit', permissions='rw', **owner)
    add('materialize')
    add('imports')
    add('tls')
    add('pcr')
    add('thread_start', thread='guest-1')
    add('thread_start', thread='guest-2')
    add('apc_enqueue', callback='original-callback', thread='guest-1')
    add('apc_deliver', callback='original-callback', thread='guest-1')
    add('callback_enter', callback='original-callback', thread='guest-1')
    add('read', address=1073741824, width=4, byte_order='big', pc_or_origin='original-fixture-site', value_sha256=hashlib.sha256(bytes(4)).hexdigest(), origin='guest', thread='guest-1', addressing='indirect', **owner)
    add('callback_exit', callback='original-callback', thread='guest-1')
    add('startup', thread='guest-1')
    add('write', address=1073741824, width=4, byte_order='big', pc_or_origin='original-fixture-site', value_sha256=hashlib.sha256(bytes([1, 2, 3, 4])).hexdigest(), origin='guest', thread='guest-1', **owner)
    add('write', address=1073741828, width=4, byte_order='big', pc_or_origin='original-fixture-site', value_sha256=hashlib.sha256(bytes([5, 6, 7, 8])).hexdigest(), origin='guest', thread='guest-1', **owner)
    add('read', address=1073741824, width=8, byte_order='big', pc_or_origin='original-fixture-site', value_sha256=hashlib.sha256(bytes([1, 2, 3, 4, 5, 6, 7, 8])).hexdigest(), origin='guest', thread='guest-2', **owner)
    add('stop', thread='guest-1')
    add('free', **owner)
    add('window_end', producer_count=21)
    return {'schema': 1, 'capture': {'evidence_class': 'SYNTHETIC_ONLY', 'tool': 'original-data-fixture', 'revision': 'fixture-v1', 'collector_sha256': '3' * 64, 'configuration_sha256': '2' * 64, 'target': {'address': 1073741824, 'size': 8}, 'pair_writes': [15, 16], 'observation': 17, 'module_sha256': module, 'record_count': len(events), 'dropped_events': 0, 'known_callbacks': ['original-callback'], 'coverage': {k: {'status': 'complete', 'evidence': 'authored fixture declaration; not real capture qualification'} for k in COVERAGE}}, 'events': events}
