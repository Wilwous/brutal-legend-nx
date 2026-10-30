# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Bounded DATA-only initialization evidence helpers; never native admission.

PE declarations, saved bytes and provider bindings are separate authorities.
No helper maps memory, applies relocations, decodes instructions or creates code.
"""
import struct
import zlib


def span(offset, size, limit):
    if any(type(v) is not int for v in (offset, size, limit)) or not 0 <= offset <= limit or not 0 <= size <= limit-offset:
        raise ValueError('source span')


def guest_to_image(address, base, size, extent):
    """Flat decoded-image translation only, never a compressed file offset."""
    if any(type(v) is not int for v in (address, base, size, extent)) or not 0 <= base <= address < 1 << 32 or base+extent > 1 << 32:
        raise ValueError('guest/image coordinate')
    span(address-base, size, extent)
    return address-base


def compression_accounting(kind, descriptor_sizes, security_size, decoded_size, basic=()):
    if type(kind) is not int or kind not in (0, 1, 2) or not descriptor_sizes or len(descriptor_sizes) > 4096:
        raise ValueError('compression metadata')
    if any(type(n) is not int or n <= 0 or n > 0xffffffff for n in descriptor_sizes):
        raise ValueError('descriptor extent')
    total = sum(descriptor_sizes)
    if total > 0xffffffff or total != security_size or total != decoded_size:
        raise ValueError('descriptor/security/decoded disagreement')
    zeros, cursor = [], 0
    if kind == 1:
        if len(basic) > 4096:
            raise ValueError('basic budget')
        for data_size, zero_size in basic:
            span(data_size, zero_size, 0xffffffff)
            span(cursor, data_size+zero_size, total)
            if zero_size:
                zeros.append((cursor+data_size, cursor+data_size+zero_size))
            cursor += data_size+zero_size
    elif basic:
        raise ValueError('basic rules do not apply to this compression mode')
    return dict(decoded_bytes=total, explicit_basic_zero_ranges=zeros,
                unclassified_basic_remainder=total-cursor if kind == 1 else 0,
                outside_extent_zero_authority=False)


def xdbf_entries(data):
    """Validate container/table/entry extents, not every payload's own grammar."""
    span(0, 24, len(data))
    magic, version, count, used, free_count, free_used = struct.unpack_from('>6I', data)
    if magic != 0x58444246 or version != 0x10000 or used > count or free_used > free_count:
        raise ValueError('XDBF header')
    span(24, count*18+free_count*8, len(data))
    content = 24+count*18+free_count*8
    rows = []
    for i in range(used):
        namespace, identity, offset, size = struct.unpack_from('>HQII', data, 24+i*18)
        span(offset, size, len(data)-content)
        rows.append(dict(namespace=namespace, identity=identity,
                         start=content+offset, end=content+offset+size))
    return dict(content_offset=content, entries=rows, payload_grammar_verified=False)


def relocation_shape(data, offset, size, image_size, endian='<'):
    """Conventional PE shape only; no Xbox applicability or fixup authority."""
    if endian not in ('<', '>') or size > 64*1024*1024:
        raise ValueError('relocation input policy')
    span(offset, size, len(data))
    end, at, entries, blocks, types = offset+size, offset, 0, 0, {}
    while at < end:
        span(at, 8, end)
        page, length = struct.unpack_from(endian+'II', data, at)
        if at % 4 or page % 4096 or page >= image_size or length < 8 or length % 2:
            raise ValueError('relocation block shape')
        span(at, length, end)
        for pos in range(at+8, at+length, 2):
            word = struct.unpack_from(endian+'H', data, pos)[0]
            kind, displacement = word >> 12, word & 4095
            if kind and page+displacement >= image_size:
                raise ValueError('relocation target extent')
            types[kind] = types.get(kind, 0)+1
            entries += 1
        blocks += 1
        at += length
    return dict(blocks=blocks, encoded_words=entries, type_counts=types,
                fixups_applied=0, applicability_proven=False)


def png_chunks(data):
    """Selected PNG structure/CRC checks; no pixel decode or asset extraction."""
    if len(data) > 64*1024*1024 or data[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError('PNG signature')
    at, rows, idat_seen, idat_ended = 8, [], False, False
    while at < len(data):
        if len(rows) >= 4096:
            raise ValueError('PNG chunk budget')
        span(at, 12, len(data))
        size = int.from_bytes(data[at:at+4], 'big')
        span(at, size+12, len(data))
        kind = data[at+4:at+8]
        if size > 0x7fffffff or not all(65 <= c <= 90 or 97 <= c <= 122 for c in kind):
            raise ValueError('PNG chunk identity')
        expected = int.from_bytes(data[at+8+size:at+12+size], 'big')
        if zlib.crc32(data[at+4:at+8+size]) & 0xffffffff != expected:
            raise ValueError('PNG CRC')
        if not rows and (kind != b'IHDR' or size != 13):
            raise ValueError('PNG first chunk')
        if kind == b'IHDR':
            if rows or size != 13:
                raise ValueError('PNG IHDR count/size')
            width, height, depth, color, compression, filtering, interlace = struct.unpack_from('>IIBBBBB', data, at+8)
            depths = {0: (1, 2, 4, 8, 16), 2: (8, 16), 3: (1, 2, 4, 8), 4: (8, 16), 6: (8, 16)}
            if not 0 < width <= 0x7fffffff or not 0 < height <= 0x7fffffff or depth not in depths.get(color, ()) or compression or filtering or interlace not in (0, 1):
                raise ValueError('PNG IHDR fields')
        if kind == b'IDAT':
            if idat_ended:
                raise ValueError('PNG nonconsecutive IDAT')
            idat_seen = True
        elif idat_seen:
            idat_ended = True
        rows.append(dict(kind=kind.decode('ascii'), start=at, end=at+12+size,
                         data_start=at+8, data_end=at+8+size))
        at += size+12
        if kind == b'IEND':
            if size or at != len(data) or not idat_seen:
                raise ValueError('PNG end extent')
            return rows
    raise ValueError('PNG missing end')


def binding_identity(before, after, slots):
    if len(before) > 64*1024*1024 or len(before) != len(after) or len(slots) > 4096:
        raise ValueError('binding input extent')
    covered, rows = set(), []
    for slot in slots:
        offset, ordinal = slot['offset'], slot['ordinal']
        span(offset, 4, len(before))
        if offset % 4 or type(ordinal) is not int or not 0 <= ordinal <= 65535:
            raise ValueError('binding identity')
        if covered.intersection(range(offset, offset+4)):
            raise ValueError('binding overlap')
        token = int.from_bytes(before[offset:offset+4], 'big')
        if token >> 24 or token & 65535 != ordinal or slot.get('variable_only') is not True:
            raise ValueError('not a verified variable-only token')
        covered.update(range(offset, offset+4))
        rows.append(dict(offset=offset, ordinal=ordinal,
                         changed=before[offset:offset+4] != after[offset:offset+4],
                         native_rebinding_admitted=False))
    different, unexplained = 0, 0
    for i, (a, b) in enumerate(zip(before, after)):
        if a != b:
            different += 1
            unexplained += i not in covered
    return dict(slots=rows, different_bytes=different,
                unexplained_bytes=unexplained,
                historical_values_reusable=False)


def demand_intervals(start, end, page_size, accesses):
    if type(start) is not int or type(end) is not int or not 0 <= start < end <= 1 << 32:
        raise ValueError('guest interval')
    if type(page_size) is not int or page_size <= 0 or page_size & (page_size-1):
        raise ValueError('page size')
    if len(accesses) > 4096:
        raise ValueError('access budget')
    for lo, hi in accesses:
        if type(lo) is not int or type(hi) is not int or not 0 <= lo < hi <= 1 << 32:
            raise ValueError('access interval')
    if (end-1)//page_size-start//page_size+1 > 16384:
        raise ValueError('page budget')
    cuts = [start] + list(range((start//page_size+1)*page_size, end, page_size)) + [end]
    return [dict(start=lo, end=hi, bytes=hi-lo,
                 normal_slice_references=sum(a < hi and lo < b for a, b in accesses),
                 initialization='UNKNOWN', proven_unused=False, execution_admitted=False)
            for lo, hi in zip(cuts, cuts[1:])]


def selected_pages(accesses, page_size, source_start, source_end):
    """Finite listed accesses only; absence is never whole-program proof."""
    # Reuse bounds/page validation, including each nonempty access.
    demand_intervals(source_start, source_end, page_size, accesses)
    pages = set()
    for lo, hi in accesses:
        for page in range(lo//page_size*page_size, (hi-1)//page_size*page_size+1, page_size):
            if not source_start <= page < page+page_size <= source_end:
                raise ValueError('selected page lacks complete source')
            pages.add(page)
    return sorted(pages)


def permission_slices(page, page_size, sections):
    """Keep logical section holes distinct from known source/page backing."""
    demand_intervals(page, page+page_size, page_size, [])
    if page % page_size or len(sections) > 4096:
        raise ValueError('permission input policy')
    cuts, selected = {page, page+page_size}, []
    for s in sections:
        lo, hi = s['start'], s['end']
        if type(lo) is not int or type(hi) is not int or not 0 <= lo < hi <= 1 << 32:
            raise ValueError('section extent')
        if lo < page+page_size and page < hi:
            a, b = max(lo, page), min(hi, page+page_size)
            cuts.update((a, b)); selected.append((a, b, s['flags']))
    cuts = sorted(cuts); rows = []
    for lo, hi in zip(cuts, cuts[1:]):
        flags = [flags for a, b, flags in selected if a <= lo and hi <= b]
        if len(flags) > 1:
            raise ValueError('overlapping permission declarations')
        rows.append(dict(start=lo, end=hi, flags=flags[0] if flags else None,
                         access_granted=False))
    return dict(slices=rows, single_section_covers_page=any(a == page and b == page+page_size for a, b, _ in selected),
                undeclared_bytes=sum(r['end']-r['start'] for r in rows if r['flags'] is None),
                execution_admitted=False)
