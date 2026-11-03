#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Nonexecuting DRAFT audit of a selected XEX field and provider snapshots.

No guest allocation, binding, PPC invocation, host-pointer translation or runtime
admission. Caller-supplied snapshot claims are NOT live memory capabilities.
"""
from dataclasses import dataclass
import hashlib

DEFAULT_HEAP_SIZE = 0x00020401
HEADER_MEMBER = 0x58  # selected open-source layout; not a full Xbox object proof


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def lookup_selected(blob, selector=DEFAULT_HEAP_SIZE):
    """Classify PRESENT/ABSENT separately from invalid input; never admit a provider.

    Only the single demonstrated inline-word selector is supported. Strict
    malformed/duplicate refusal is native policy, not an Xbox status.
    """
    require(selector == DEFAULT_HEAP_SIZE, 'unsupported selector')
    require(isinstance(blob, bytes) and len(blob) >= 24, 'truncated header')
    require(blob[:4] == b'XEX2', 'header magic')
    u32 = lambda off: int.from_bytes(blob[off:off+4], 'big')
    size, count = u32(8), u32(20)
    require(24 <= size <= len(blob) and size % 4 == 0, 'header extent')
    require(count <= (size-24)//8, 'directory extent')
    matches = [24+8*i+4 for i in range(count) if u32(24+8*i) == selector]
    require(len(matches) <= 1, 'missing or duplicate selector')
    offset = matches[0] if matches else None
    return {'state': 'PRESENT' if matches else 'ABSENT',
            'offset': offset, 'width': 4 if matches else 0,
            'value': u32(offset) if matches else None,
            'header_size': size, 'source_sha256': hashlib.sha256(blob[:size]).hexdigest(),
            'kind': 'inline-word-address' if matches else 'absent', 'execution_admitted': False}


def selected_field(blob, selector=DEFAULT_HEAP_SIZE):
    """Retain T14's strict policy: an absent field cannot install a provider."""
    field = lookup_selected(blob, selector)
    require(field['state'] == 'PRESENT', 'missing or duplicate selector')
    return {k: v for k, v in field.items() if k != 'state'}


@dataclass(frozen=True)
class Snapshot:
    """Inert bytes plus untrusted declarations, not ownership enforcement."""
    base: int
    data: bytes
    module: str
    generation: int
    owner: str
    reserved: bool = True
    committed: bool = True
    permissions: str = 'R'
    alive: bool = True


def draft_provider(slot, cell, obj, header, *, module, generation, thread_module,
                   expected_header_sha256, leased):
    """Check immutable synthetic snapshot consistency. Never publish a provider.

    Module and generation must come from a future trusted runtime registry.
    Thread association is a conservative native policy, not proved Xbox TLS ABI.
    """
    require(isinstance(module, str) and bool(module) and type(generation) is int
            and generation > 0, 'module identity')
    require(thread_module == module, 'thread/module mismatch')
    require(leased is True, 'missing lease')
    records = (slot, cell, obj, header)
    for r in records:
        require(isinstance(r, Snapshot), 'snapshot type')
        require(type(r.base) is int and 0 < r.base < 2**32 and r.base % 4 == 0,
                'guest alignment/range')
        require(isinstance(r.data, bytes) and 0 < len(r.data) <= 2**32-r.base, 'guest extent')
        require(r.module == module and type(r.generation) is int
                and r.generation == generation and r.alive is True, 'stale or foreign module')
        require(r.owner == module, 'invalid owner')
        require(r.reserved is True and r.committed is True, 'unbacked snapshot')
        require(r.permissions == 'R', 'snapshot permissions')
    for i, a in enumerate(records):
        for b in records[i+1:]:
            require(a.base+len(a.data) <= b.base or b.base+len(b.data) <= a.base,
                    'overlapping snapshots')
    require(len(slot.data) == len(cell.data) == 4, 'pointer cell extent')
    require(len(obj.data) >= HEADER_MEMBER+4, 'module member extent')
    load = lambda r, off=0: int.from_bytes(r.data[off:off+4], 'big')
    require(load(slot) == cell.base and load(cell) == obj.base, 'pointer depth/binding')
    require(load(obj, HEADER_MEMBER) == header.base, 'module/header mismatch')
    field = selected_field(header.data)
    require(len(header.data) == field['header_size'], 'noncanonical header backing')
    require(field['source_sha256'] == expected_header_sha256, 'header provenance')
    # Python wide arithmetic plus checked object extent prevent ptr32 wrapping.
    result = header.base+field['offset']
    require(result+4 <= header.base+len(header.data) <= 2**32, 'field guest extent')
    return {'status': 'DRAFT snapshot consistent', 'field_guest_coordinate': result,
            'value': field['value'], 'execution_admitted': False,
            'runtime_provider_installed': False, 'mutations': 0}
