#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original data only; independent literal byte oracle; no runtime or title input."""
import hashlib
import struct
import sys
import unittest
from dataclasses import replace
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2]/'tools'))
from xex_header_contract import Snapshot, draft_provider, selected_field

# Independent literal XEX-like header: two optional records; selected word at36.
BLOB = bytes.fromhex('584558320000000000000028000000000000000000000002'
                     '00010000000000070002040112345678')
DIGEST = hashlib.sha256(BLOB).hexdigest()


def fixture():
    # Unusual asymmetric addresses expose little-endian and collapsed-pointer bugs.
    a, b, c, h = 0x10203040, 0x20304050, 0x30405060, 0x40506070
    obj = bytearray(92); obj[88:92] = h.to_bytes(4, 'big')
    return [Snapshot(a, b.to_bytes(4, 'big'), 'm', 1, 'm'),
            Snapshot(b, c.to_bytes(4, 'big'), 'm', 1, 'm'),
            Snapshot(c, bytes(obj), 'm', 1, 'm'), Snapshot(h, BLOB, 'm', 1, 'm')]


def inspect(records, **kw):
    args = dict(module='m', generation=1, thread_module='m',
                expected_header_sha256=DIGEST, leased=True)
    args.update(kw)
    return draft_provider(*records, **args)


class HeaderTests(unittest.TestCase):
    def test_literal_oracle(self):
        x = selected_field(BLOB)
        self.assertEqual((x['offset'], x['width'], x['value']), (36, 4, 0x12345678))
        # A separate struct parser knows neither the production iterator nor layout helper.
        rows = list(struct.iter_unpack('>II', BLOB[24:40]))
        self.assertEqual(rows, [(0x10000, 7), (0x20401, 0x12345678)])
        self.assertEqual(BLOB[x['offset']:x['offset']+4], bytes.fromhex('12345678'))
        self.assertFalse(x['execution_admitted'])

    def test_return_offset_not_scalar(self):
        self.assertNotEqual(selected_field(BLOB)['offset'], 0x12345678)

    def test_all_truncations(self):
        for i in range(len(BLOB)):
            with self.subTest(i=i), self.assertRaises(ValueError): selected_field(BLOB[:i])

    def test_unsupported_selector(self):
        with self.assertRaisesRegex(ValueError, 'unsupported selector'): selected_field(BLOB, 0x10000)

    def test_wrong_magic(self):
        with self.assertRaisesRegex(ValueError, 'header magic'): selected_field(b'PE00'+BLOB[4:])

    def test_count_overflow(self):
        b = bytearray(BLOB); b[20:24] = b'\xff'*4
        with self.assertRaisesRegex(ValueError, 'directory extent'): selected_field(bytes(b))

    def test_extent_overflow(self):
        b = bytearray(BLOB); b[8:12] = b'\xff'*4
        with self.assertRaisesRegex(ValueError, 'header extent'): selected_field(bytes(b))

    def test_misaligned_extent(self):
        b = bytearray(BLOB); b[8:12] = (39).to_bytes(4, 'big')
        with self.assertRaisesRegex(ValueError, 'header extent'): selected_field(bytes(b))

    def test_missing(self):
        b = bytearray(BLOB); b[32:36] = bytes(4)
        with self.assertRaisesRegex(ValueError, 'missing or duplicate'): selected_field(bytes(b))

    def test_duplicate(self):
        b = bytearray(BLOB); b[24:28] = b[32:36]
        with self.assertRaisesRegex(ValueError, 'missing or duplicate'): selected_field(bytes(b))

    def test_zero_payload_still_has_address(self):
        b = BLOB[:36]+bytes(4)
        x = selected_field(b)
        self.assertEqual((x['offset'], x['value']), (36, 0))

    def test_trailing_file_data_not_part_of_header(self):
        self.assertEqual(selected_field(BLOB+b'nonheader')['source_sha256'], DIGEST)


class DraftTests(unittest.TestCase):
    def refuse_record(self, index, reason, **kw):
        r = fixture(); r[index] = replace(r[index], **kw)
        with self.assertRaisesRegex(ValueError, reason): inspect(r)

    def test_consistent_not_admitted(self):
        x = inspect(fixture())
        self.assertEqual(x['field_guest_coordinate'], 0x40506094)
        self.assertEqual(x['value'], 0x12345678)
        self.assertFalse(x['execution_admitted']); self.assertFalse(x['runtime_provider_installed'])
        self.assertEqual(x['mutations'], 0)

    def test_null_unbound(self): self.refuse_record(0, 'pointer depth', data=bytes(4))
    def test_collapsed_depth(self): self.refuse_record(0, 'pointer depth', data=(0x30405060).to_bytes(4, 'big'))
    def test_little_endian(self): self.refuse_record(1, 'pointer depth', data=(0x30405060).to_bytes(4, 'little'))
    def test_misaligned(self): self.refuse_record(2, 'alignment', base=0x30405061)
    def test_address_wrap(self): self.refuse_record(3, 'guest extent', base=0xfffffff0)
    def test_foreign_module(self): self.refuse_record(2, 'stale or foreign', module='other')
    def test_stale_generation(self): self.refuse_record(2, 'stale or foreign', generation=2)
    def test_wrong_owner(self): self.refuse_record(2, 'invalid owner', owner='other')
    def test_uncommitted(self): self.refuse_record(3, 'unbacked', committed=False)
    def test_unreserved(self): self.refuse_record(3, 'unbacked', reserved=False)
    def test_unreadable(self): self.refuse_record(3, 'permissions', permissions='')
    def test_guest_writable(self): self.refuse_record(3, 'permissions', permissions='RW')
    def test_executable(self): self.refuse_record(2, 'permissions', permissions='RX')
    def test_destroyed(self): self.refuse_record(1, 'stale or foreign', alive=False)
    def test_header_mismatch(self): self.refuse_record(2, 'module/header mismatch', data=bytes(92))
    def test_short_object(self): self.refuse_record(2, 'module member extent', data=bytes(88))
    def test_wide_cell(self): self.refuse_record(1, 'pointer cell extent', data=bytes(8))
    def test_overlapping(self): self.refuse_record(3, 'overlapping', base=0x30405060)
    def test_source_changed(self): self.refuse_record(3, 'header provenance', data=BLOB[:39]+b'X')
    def test_extra_backing(self): self.refuse_record(3, 'noncanonical', data=BLOB+b'X')

    def test_thread_mismatch(self):
        with self.assertRaisesRegex(ValueError, 'thread/module'): inspect(fixture(), thread_module='other')

    def test_missing_lease(self):
        with self.assertRaisesRegex(ValueError, 'missing lease'): inspect(fixture(), leased=False)

    def test_truthy_metadata_does_not_prove_boolean_claims(self):
        for kw, reason in [({'reserved': 'yes'}, 'unbacked'),
                           ({'committed': 'yes'}, 'unbacked'),
                           ({'alive': 1}, 'stale or foreign'),
                           ({'generation': True}, 'stale or foreign')]:
            with self.subTest(kw=kw): self.refuse_record(2, reason, **kw)
        with self.assertRaisesRegex(ValueError, 'module identity'): inspect(fixture(), module=True)

    def test_rebind_requires_new_generation(self):
        r = [replace(x, generation=2) for x in fixture()]
        with self.assertRaisesRegex(ValueError, 'stale or foreign'): inspect(r)
        self.assertFalse(inspect(r, generation=2)['execution_admitted'])

    def test_refusal_and_repeated_snapshots_are_immutable(self):
        r = fixture(); before = tuple(r)
        for _ in range(32):
            self.assertFalse(inspect(r)['execution_admitted'])
            with self.assertRaises(ValueError): inspect(r, leased=False)
        self.assertEqual(tuple(r), before)
        # Snapshot consistency is NOT proof of actual allocator cleanup/rollback.


if __name__ == '__main__': unittest.main()
