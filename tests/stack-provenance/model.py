# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Explicit object renaming, not an ISA engine or title admission policy."""
from dataclasses import dataclass


@dataclass(frozen=True)
class Object:
    base: int
    size: int
    epoch: int = 1
    live: bool = True
    writable: bool = True


class Relation:
    def __init__(self, left, right):
        if left.keys() != right.keys() or not left:
            raise ValueError('explicit object correspondence required')
        for objects in (left, right):
            intervals = []
            for obj in objects.values():
                if (not obj.live or not obj.writable or obj.base % 16 or obj.size <= 0
                        or obj.base < 0 or obj.base + obj.size > 2**32):
                    raise ValueError('invalid live owned object declaration')
                intervals.append((obj.base, obj.base + obj.size))
            intervals.sort()
            if any(a[1] > b[0] for a, b in zip(intervals, intervals[1:])):
                raise ValueError('overlapping object roles')
        for name, obj in left.items():
            if (obj.size, obj.epoch) != (right[name].size, right[name].epoch):
                raise ValueError('extent/lifetime correspondence missing')
        self.left, self.right = left, right

    def pointer(self, name, offset, *, width=0):
        if name not in self.left or offset < 0 or width < 0:
            raise ValueError('unknown object/negative extent')
        a, b = self.left[name], self.right[name]
        if offset + width > a.size or (width and offset == a.size):
            raise ValueError('pointer outside declared object')
        return a.base + offset, b.base + offset

    def alias(self, a, b):
        x, y = self.pointer(*a), self.pointer(*b)
        return (x[0] == y[0]) == (x[1] == y[1])

    def ordered(self, a, b):
        # Ordering across unrelated objects is an address observation, not a
        # property of an object graph. Equalities remain alias-preserving.
        if a[0] != b[0]:
            raise ValueError('cross-object order requires a separate proof')
        x, y = self.pointer(*a), self.pointer(*b)
        return (x[0] < y[0]) == (x[1] < y[1])

    def raw_integer_equal(self, name, offset):
        a, b = self.pointer(name, offset)
        return a == b


def normalize(raw, base, fields):
    """Rename ONLY externally typed pointer fields after checking actual bytes.

    fields = (field offset, target object-relative offset). The sole object here
    is the independently declared stack allocation. Opaque words stay opaque.
    This does not prove that a future provider uses this observer.
    """
    if base < 0 or base % 16 or not raw or base + len(raw) > 2**32:
        raise ValueError('invalid declared stack representation')
    out = bytearray(raw)
    used = set()
    for field, target in fields:
        if field < 0 or field + 4 > len(raw) or not 0 <= target <= len(raw):
            raise ValueError('invalid typed graph field')
        if used.intersection(range(field, field + 4)):
            raise ValueError('overlapping typed fields')
        used.update(range(field, field + 4))
        if int.from_bytes(raw[field:field+4], 'big') != base + target:
            raise ValueError('actual pointer does not match declared edge')
        out[field:field+4] = target.to_bytes(4, 'big')
    return bytes(out)


def nested_fields(sp_offset):
    return ((sp_offset-64, sp_offset),
            (sp_offset-160, sp_offset-64),
            (sp_offset-192, sp_offset-160),
            (sp_offset-188, sp_offset-160))


def frame_access(pointer, begin, end, active, width=4):
    if not active or width <= 0 or pointer < begin or pointer + width > end:
        raise ValueError('dead frame or out-of-frame access')
    return True
