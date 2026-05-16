#!/usr/bin/env python3
"""Check BMD binary structure for issues: section sizes, padding, alignment."""

import struct
import sys

def read_u8(data, pos): return data[pos], pos + 1
def read_u16(data, pos): return struct.unpack_from('>H', data, pos)[0], pos + 2
def read_u32(data, pos): return struct.unpack_from('>I', data, pos)[0], pos + 4
def read_f32(data, pos): return struct.unpack_from('>f', data, pos)[0], pos + 4

def check_bmd(filepath, label):
    with open(filepath, 'rb') as f:
        data = f.read()

    print(f"\n{'='*60}")
    print(f"  {label}: {filepath}")
    print(f"  File size: {len(data)} bytes")
    print(f"{'='*60}")

    # Header
    magic = data[0:8].decode('ascii')
    file_size = struct.unpack_from('>I', data, 8)[0]
    section_count = struct.unpack_from('>I', data, 12)[0]
    print(f"  Magic: {magic}")
    print(f"  File size (header): {file_size} (actual: {len(data)})")
    print(f"  Section count: {section_count}")

    # Walk sections
    pos = 0x20
    sections = []
    for i in range(section_count):
        if pos + 8 > len(data):
            print(f"  ERROR: Section {i} starts at {pos:#x} but only {len(data)} bytes in file")
            break
        tag = data[pos:pos+4].decode('ascii', errors='replace')
        size = struct.unpack_from('>I', data, pos + 4)[0]
        aligned = (pos % 32 == 0 or pos == 0x20)
        sections.append((tag, pos, size))
        print(f"  Section {i}: {tag} at {pos:#08x}, size={size:#x} ({size}), end={pos+size:#08x}, "
              f"aligned32={'yes' if pos % 32 == 0 else 'NO (offset=%d)' % (pos % 32)}")
        if pos + size > len(data):
            print(f"    WARNING: Section extends past file end!")
        pos += size

    # Check SHP1 in detail
    for tag, base, size in sections:
        if tag != 'SHP1':
            continue
        print(f"\n  --- SHP1 Detailed Check ---")
        p = base
        _, p = read_u32(data, p)  # tag already read
        _, p = read_u32(data, p)  # size
        batch_count, p = read_u16(data, p)
        pad, p = read_u16(data, p)
        off_batches, p = read_u32(data, p)
        off_unknown, p = read_u32(data, p)
        off_zero, p = read_u32(data, p)
        off_attribs, p = read_u32(data, p)
        off_mtx_table, p = read_u32(data, p)
        off_data, p = read_u32(data, p)
        off_mtx_data, p = read_u32(data, p)
        off_pkt_locs, p = read_u32(data, p)

        print(f"    batch_count: {batch_count}")
        print(f"    pad: {pad:#06x}")
        print(f"    off_batches: {off_batches:#x}")
        print(f"    off_unknown: {off_unknown:#x}")
        print(f"    off_zero: {off_zero:#x}")
        print(f"    off_attribs: {off_attribs:#x}")
        print(f"    off_mtx_table: {off_mtx_table:#x}")
        print(f"    off_data: {off_data:#x}")
        print(f"    off_mtx_data: {off_mtx_data:#x}")
        print(f"    off_pkt_locs: {off_pkt_locs:#x}")

        # Check that data section primitives are correctly aligned
        print(f"\n    --- Packet data alignment check ---")
        for bi in range(batch_count):
            bd_off = base + off_batches + bi * 40
            bd_unknown, _ = read_u16(data, bd_off)
            bd_pkt_count, _ = read_u16(data, bd_off + 2)
            bd_attribs_off, _ = read_u16(data, bd_off + 4)
            bd_first_mtx, _ = read_u16(data, bd_off + 6)
            bd_first_pkt, _ = read_u16(data, bd_off + 8)

            for pi in range(bd_pkt_count):
                pl_off = base + off_pkt_locs + (bd_first_pkt + pi) * 8
                pkt_size, _ = read_u32(data, pl_off)
                pkt_offset, _ = read_u32(data, pl_off + 4)

                abs_data_off = base + off_data + pkt_offset
                if pkt_size % 32 != 0:
                    print(f"    Batch {bi} Pkt {pi}: size={pkt_size:#x} NOT 32-aligned! "
                          f"(remainder={pkt_size % 32})")
                # Check data offset alignment
                if pkt_offset % 32 != 0 and pi > 0:
                    pass  # First packet offset doesn't need alignment

        # Check matrix table values - look for bogus entries
        print(f"\n    --- Matrix table scan ---")
        mtx_table_start = base + off_mtx_table
        # We need to know total mtx table size from mtx_data
        # Count total entries
        total_mtx_entries = 0
        for bi in range(batch_count):
            bd_off = base + off_batches + bi * 40
            bd_pkt_count, _ = read_u16(data, bd_off + 2)
            bd_first_mtx, _ = read_u16(data, bd_off + 6)
            for pi in range(bd_pkt_count):
                md_off = base + off_mtx_data + (bd_first_mtx + pi) * 8
                md_count, _ = read_u16(data, md_off + 2)
                md_first, _ = read_u32(data, md_off + 4)
                total_mtx_entries = max(total_mtx_entries, md_first + md_count)

        print(f"    Total matrix table entries: {total_mtx_entries}")

        # Read and print matrix table for weighted batches
        for bi in range(batch_count):
            bd_off = base + off_batches + bi * 40
            bd_unknown, _ = read_u16(data, bd_off)
            bd_pkt_count, _ = read_u16(data, bd_off + 2)
            bd_first_mtx, _ = read_u16(data, bd_off + 6)

            # Check if weighted (mattype check)
            attribs_off_val, _ = read_u16(data, bd_off + 4)
            attr_abs = base + off_attribs + attribs_off_val
            first_attr, _ = read_u32(data, attr_abs)
            is_weighted = (first_attr == 0)  # attribute 0 = matrix index

            if not is_weighted:
                continue

            print(f"\n    Batch {bi} (weighted, {bd_pkt_count} packets):")
            for pi in range(bd_pkt_count):
                md_off = base + off_mtx_data + (bd_first_mtx + pi) * 8
                md_unk1, _ = read_u16(data, md_off)
                md_count, _ = read_u16(data, md_off + 2)
                md_first, _ = read_u32(data, md_off + 4)

                entries = []
                for ei in range(md_count):
                    val, _ = read_u16(data, mtx_table_start + (md_first + ei) * 2)
                    entries.append(val)

                ffff_count = sum(1 for v in entries if v == 0xFFFF)
                print(f"      Pkt {pi}: useMtx={md_unk1:#06x}, count={md_count}, "
                      f"firstIdx={md_first}, entries={entries}")

    # Check VTX1
    for tag, base, size in sections:
        if tag != 'VTX1':
            continue
        print(f"\n  --- VTX1 Check ---")
        p = base + 8  # skip tag + size
        fmt_offset, p = read_u32(data, p)
        offsets = []
        for _ in range(13):
            v, p = read_u32(data, p)
            offsets.append(v)

        active = [(i, offsets[i]) for i in range(13) if offsets[i] != 0]
        print(f"    Active arrays: {len(active)}")
        for i, off in active:
            # Read format
            fmt_idx = sum(1 for j in range(i) if offsets[j] != 0)
            fmt_pos = base + fmt_offset + fmt_idx * 16
            arr_type, _ = read_u32(data, fmt_pos)
            comp_count, _ = read_u32(data, fmt_pos + 4)
            data_type, _ = read_u32(data, fmt_pos + 8)
            dec_pt, _ = read_u8(data, fmt_pos + 12)

            # Compute length
            next_off = size
            for j in range(i + 1, 13):
                if offsets[j] != 0:
                    next_off = offsets[j]
                    break
            length = next_off - off

            type_names = {9: 'pos', 0xa: 'nrm', 0xb: 'clr0', 0xc: 'clr1',
                         0xd: 'tex0', 0xe: 'tex1', 0xf: 'tex2'}
            dtype_names = {3: 's16', 4: 'f32', 5: 'rgba8'}
            print(f"    Slot {i}: type={type_names.get(arr_type, hex(arr_type))}, "
                  f"comp={comp_count}, dtype={dtype_names.get(data_type, hex(data_type))}, "
                  f"decPt={dec_pt}, offset={off:#x}, len={length}, "
                  f"abs_offset={base + off:#x}")

    return data, sections

print("ORIGINAL BMD:")
orig_data, orig_sections = check_bmd(
    r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd", "ORIGINAL")

print("\n\nEXPORTED BMD:")
exp_data, exp_sections = check_bmd(
    r"C:\Users\ryana\Downloads\reconstruct_test.bmd", "EXPORTED")

# Binary comparison of raw section data for EVP1, DRW1, JNT1
print(f"\n{'='*60}")
print("RAW SECTION COMPARISON")
print(f"{'='*60}")

def find_section(sections, tag):
    for t, off, sz in sections:
        if t == tag:
            return off, sz
    return None, None

for tag in ['EVP1', 'DRW1', 'JNT1']:
    o_off, o_sz = find_section(orig_sections, tag)
    e_off, e_sz = find_section(exp_sections, tag)
    if o_off is None or e_off is None:
        print(f"  {tag}: MISSING in one file")
        continue
    o_data = orig_data[o_off:o_off+o_sz]
    e_data = exp_data[e_off:e_off+e_sz]
    if o_data == e_data:
        print(f"  {tag}: IDENTICAL ({o_sz} bytes)")
    else:
        print(f"  {tag}: DIFFERENT (orig={o_sz}, exp={e_sz})")
        # Find first difference
        for i in range(min(len(o_data), len(e_data))):
            if o_data[i] != e_data[i]:
                print(f"    First diff at section offset {i:#x}: "
                      f"orig={o_data[i]:#04x} exp={e_data[i]:#04x}")
                break
