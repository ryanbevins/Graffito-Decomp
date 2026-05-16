#!/usr/bin/env python3
"""Binary diff VTX1 sections between original and exported BMD files."""

import struct
import sys

ORIG = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"
EXPORTED = r"C:\Users\ryana\Downloads\reconstruct_test.bmd"

def read_file(path):
    with open(path, "rb") as f:
        return f.read()

def find_section(data, tag):
    """Find a section by its 4-byte tag, return (offset, size)."""
    pos = 0
    while pos < len(data) - 8:
        sec_tag = data[pos:pos+4]
        if sec_tag == tag:
            sec_size = struct.unpack_from(">I", data, pos+4)[0]
            return pos, sec_size
        # Try next 4 bytes if not found, but normally sections are sequential
        # after the file header
        if pos == 0:
            # Skip J3D file header (0x20 bytes)
            pos = 0x20
        else:
            # Read current section size to jump to next
            try:
                cur_size = struct.unpack_from(">I", data, pos+4)[0]
                if cur_size > 0 and cur_size < len(data):
                    pos += cur_size
                else:
                    pos += 4
            except:
                pos += 4
    return None, None

def parse_vtx1(data, base):
    """Parse VTX1 section starting at base offset."""
    tag = data[base:base+4]
    size = struct.unpack_from(">I", data, base+4)[0]

    # VTX1 header:
    # +0x00: tag "VTX1"
    # +0x04: section size
    # +0x08: offset to arrayFormat table (relative to section start)
    # +0x0C: offsets to data pools (13 entries, one per GX attr)

    fmt_offset = struct.unpack_from(">I", data, base+0x08)[0]

    # 13 data pool offsets at +0x0C
    pool_offsets = []
    for i in range(13):
        off = struct.unpack_from(">I", data, base+0x0C + i*4)[0]
        pool_offsets.append(off)

    # Parse arrayFormat entries (each 16 bytes: arrayType(u32), compSize(u32), compType(u32), compShift(u8), pad[3])
    # Actually: GXAttr(u32), compCnt(u32), compType(u32), fracBits(u8), pad(u8), stride(u16)
    # Wait - let me check the standard J3D VTX1 arrayFormat:
    # Each entry is 16 bytes:
    #   u32 arrayType (GXAttr)
    #   u32 componentCount (GXCompCnt)
    #   u32 componentType (GXCompType)
    #   u8  decimalPoint (fraction bits)
    #   u8  pad
    #   u16 pad/stride? or just padding
    # Terminated by entry with arrayType == 0xFF (GX_VA_NULL)

    fmt_abs = base + fmt_offset
    entries = []
    i = 0
    while True:
        off = fmt_abs + i * 16
        if off + 16 > len(data):
            break
        raw = data[off:off+16]
        arrayType = struct.unpack_from(">I", raw, 0)[0]
        compCnt = struct.unpack_from(">I", raw, 4)[0]
        compType = struct.unpack_from(">I", raw, 8)[0]
        fracBits = raw[12]
        pad1 = raw[13]
        extra = struct.unpack_from(">H", raw, 14)[0]
        entries.append({
            "arrayType": arrayType,
            "compCnt": compCnt,
            "compType": compType,
            "fracBits": fracBits,
            "pad1": pad1,
            "extra": extra,
            "raw": raw,
        })
        if arrayType == 0xFF or arrayType == 0xFFFFFFFF:
            break
        i += 1

    return {
        "tag": tag,
        "size": size,
        "fmt_offset": fmt_offset,
        "pool_offsets": pool_offsets,
        "entries": entries,
        "base": base,
    }

GX_ATTR_NAMES = {
    0: "GX_VA_PNMTXIDX", 9: "GX_VA_POS", 10: "GX_VA_NRM",
    11: "GX_VA_CLR0", 12: "GX_VA_CLR1",
    13: "GX_VA_TEX0", 14: "GX_VA_TEX1", 15: "GX_VA_TEX2",
    16: "GX_VA_TEX3", 17: "GX_VA_TEX4", 18: "GX_VA_TEX5",
    19: "GX_VA_TEX6", 20: "GX_VA_TEX7",
    0xFF: "GX_VA_NULL", 0xFFFFFFFF: "GX_VA_NULL",
}

GX_COMP_CNT = {
    0: "POS_XY/NRM_XYZ/CLR_RGB/TEX_S",
    1: "POS_XYZ/NRM_NBT/CLR_RGBA/TEX_ST",
    2: "NRM_NBT3",
}

GX_COMP_TYPE = {
    0: "U8", 1: "S8", 2: "U16", 3: "S16", 4: "F32",
}

GX_CLR_TYPE = {
    0: "RGB565", 1: "RGB8", 2: "RGBX8", 3: "RGBA4", 4: "RGBA6", 5: "RGBA8",
}

def get_stride(entry):
    """Calculate stride for a VTX1 array format entry."""
    at = entry["arrayType"]
    cc = entry["compCnt"]
    ct = entry["compType"]

    if at == 11 or at == 12:  # Color
        # Color types have fixed sizes
        clr_sizes = {0: 2, 1: 3, 2: 4, 3: 2, 4: 3, 5: 4}
        return clr_sizes.get(ct, 4)

    # Position/Normal/TexCoord
    comp_sizes = {0: 1, 1: 1, 2: 2, 3: 2, 4: 4}
    comp_size = comp_sizes.get(ct, 4)

    if at == 9:  # Position
        num_comps = 2 if cc == 0 else 3
    elif at == 10:  # Normal
        num_comps = 3  # Always 3 for XYZ or NBT
        if cc == 1:  # NBT
            num_comps = 9
        elif cc == 2:  # NBT3
            num_comps = 9
    elif 13 <= at <= 20:  # TexCoord
        num_comps = 1 if cc == 0 else 2
    else:
        num_comps = 3

    return num_comps * comp_size

def read_positions_f32(data, base, pool_offset, count):
    """Read count f32 xyz positions from pool."""
    positions = []
    abs_off = base + pool_offset
    for i in range(count):
        x, y, z = struct.unpack_from(">fff", data, abs_off + i * 12)
        positions.append((x, y, z))
    return positions

def read_positions_s16(data, base, pool_offset, count, frac_bits):
    """Read count s16 xyz positions from pool."""
    positions = []
    abs_off = base + pool_offset
    scale = 1.0 / (1 << frac_bits) if frac_bits > 0 else 1.0
    for i in range(count):
        x, y, z = struct.unpack_from(">hhh", data, abs_off + i * 6)
        positions.append((x * scale, y * scale, z * scale))
    return positions

def parse_shp1(data, base):
    """Parse SHP1 section."""
    tag = data[base:base+4]
    size = struct.unpack_from(">I", data, base+4)[0]
    batch_count = struct.unpack_from(">H", data, base+8)[0]

    # Offsets per the task description
    batch_hdr_off = struct.unpack_from(">I", data, base+0x0C)[0]
    # +0x10: offset to index remap table (short[])
    remap_off = struct.unpack_from(">I", data, base+0x10)[0]
    # +0x14: unused/zero
    # +0x18: offset to attrib descriptors
    attrib_off = struct.unpack_from(">I", data, base+0x18)[0]
    # +0x1C: offset to matrix table
    mtx_tbl_off = struct.unpack_from(">I", data, base+0x1C)[0]
    # +0x20: offset to primitive data
    prim_off = struct.unpack_from(">I", data, base+0x20)[0]
    # +0x24: offset to matrix data (packet locations?)
    mtx_data_off = struct.unpack_from(">I", data, base+0x24)[0]
    # +0x28: offset to draw init data (packet count/offset per batch)
    draw_init_off = struct.unpack_from(">I", data, base+0x28)[0]

    return {
        "tag": tag, "size": size, "batch_count": batch_count,
        "batch_hdr_off": batch_hdr_off, "remap_off": remap_off,
        "attrib_off": attrib_off, "mtx_tbl_off": mtx_tbl_off,
        "prim_off": prim_off, "mtx_data_off": mtx_data_off,
        "draw_init_off": draw_init_off, "base": base,
    }

def get_batch_attribs(data, shp1_base, shp1, batch_idx):
    """Get attribute descriptors for a batch."""
    # Batch header is 0x28 bytes each
    batch_off = shp1_base + shp1["batch_hdr_off"] + batch_idx * 0x28
    # Batch header:
    # +0x00: u8 matrixType
    # +0x02: u16 firstPacketIndex (into packet location table)
    # +0x04: u16 packetCount
    # +0x06: u16 firstAttribIndex
    # ...more fields

    matrix_type = data[batch_off]
    first_packet = struct.unpack_from(">H", data, batch_off + 0x02)[0]
    packet_count = struct.unpack_from(">H", data, batch_off + 0x04)[0]
    first_attrib = struct.unpack_from(">H", data, batch_off + 0x06)[0]

    # first matrix index and matrix count
    first_mtx_data = struct.unpack_from(">H", data, batch_off + 0x08)[0]

    # Read attribs starting at first_attrib
    attrib_base = shp1_base + shp1["attrib_off"] + first_attrib * 4
    attribs = []
    while True:
        attr = struct.unpack_from(">I", data, attrib_base)[0]
        gx_attr = (attr >> 24) & 0xFF
        # Actually, attrib descriptors are pairs: u32 with GXAttr(u32) and GXAttrType(u32)
        # Let me re-read - it's two u16s or something else?
        # J3D SHP1 attrib: GXAttr (u32) + GXAttrType (u32) = 8 bytes each
        # Terminated by GX_VA_NULL (0xFF)
        break

    # Re-do: each attrib is 8 bytes
    attrib_base = shp1_base + shp1["attrib_off"] + first_attrib * 8
    attribs = []
    pos = attrib_base
    while pos + 8 <= len(data):
        gx_attr = struct.unpack_from(">I", data, pos)[0]
        gx_type = struct.unpack_from(">I", data, pos+4)[0]
        if gx_attr == 0xFF or gx_attr == 0xFFFFFFFF:
            break
        attribs.append((gx_attr, gx_type))
        pos += 8

    # Actually attribs might be u16 pairs?  Let me try that too
    # Standard J3D: GXAttr is u32, GXAttrType is u32 => 8 bytes per entry

    return {
        "matrix_type": matrix_type,
        "first_packet": first_packet,
        "packet_count": packet_count,
        "first_attrib": first_attrib,
        "first_mtx_data": first_mtx_data,
        "attribs": attribs,
    }

def get_prim_data_range(data, shp1_base, shp1, batch_info):
    """Get the primitive data offset and size for a batch's first packet."""
    # draw init data: each entry is 8 bytes (u32 offset, u32 size) per packet
    # Actually it might be the packet locations table
    # mtx_data_off points to packet matrix info
    # The primitive data for packet i is found via:
    #   draw_init_off has entries of (u32 prim_data_size, u32 prim_data_offset) per packet

    # Actually let me re-check: the packet location table
    # Each batch has packetCount packets starting at firstPacketIndex
    # The packet location table at draw_init_off has entries for each packet:
    #   u32 size, u32 offset (relative to prim_data section start)

    first_pkt = batch_info["first_packet"]
    pkt_loc_base = shp1_base + shp1["draw_init_off"] + first_pkt * 8
    pkt_size = struct.unpack_from(">I", data, pkt_loc_base)[0]
    pkt_offset = struct.unpack_from(">I", data, pkt_loc_base + 4)[0]

    abs_prim = shp1_base + shp1["prim_off"] + pkt_offset
    return abs_prim, pkt_size

def calc_vertex_size(attribs):
    """Calculate vertex size in bytes from attrib list."""
    total = 0
    for gx_attr, gx_type in attribs:
        if gx_type == 0:  # NONE
            continue
        elif gx_type == 1:  # DIRECT
            # Depends on attr
            if gx_attr == 0:  # PNMTXIDX
                total += 1
            elif gx_attr in (11, 12):  # Color
                total += 4  # guess RGBA8
            else:
                total += 2  # guess
        elif gx_type == 2:  # INDEX8
            total += 1
        elif gx_type == 3:  # INDEX16
            total += 2
    return total

def read_indices_from_prim(data, prim_off, prim_size, attribs, target_attr):
    """Read primitive data and extract indices for a specific attribute."""
    pos = prim_off
    end = prim_off + prim_size
    indices = []

    while pos < end:
        opcode = data[pos]
        pos += 1
        if opcode == 0:
            continue

        # GX primitive opcodes: 0x80=quads, 0x90=triangles, 0x98=tristrip, 0xA0=trifan, 0xA8=lines, 0xB0=linestrip, 0xB8=points
        if opcode & 0x80 == 0:
            continue

        vert_count = struct.unpack_from(">H", data, pos)[0]
        pos += 2

        for v in range(vert_count):
            for gx_attr, gx_type in attribs:
                if gx_type == 0:
                    continue

                if gx_type == 1:  # DIRECT
                    if gx_attr == 0:  # PNMTXIDX
                        val = data[pos]
                        pos += 1
                    else:
                        val = data[pos]
                        pos += 1
                elif gx_type == 2:  # INDEX8
                    val = data[pos]
                    pos += 1
                elif gx_type == 3:  # INDEX16
                    val = struct.unpack_from(">H", data, pos)[0]
                    pos += 2
                else:
                    val = 0

                if gx_attr == target_attr:
                    indices.append(val)

        if len(indices) > 50:
            break

    return indices


def main():
    orig_data = read_file(ORIG)
    exp_data = read_file(EXPORTED)

    print(f"Original file size: {len(orig_data)} bytes")
    print(f"Exported file size: {len(exp_data)} bytes")
    print()

    # ============================================================
    # 1. Parse VTX1 sections
    # ============================================================
    orig_vtx1_off, orig_vtx1_sz = find_section(orig_data, b"VTX1")
    exp_vtx1_off, exp_vtx1_sz = find_section(exp_data, b"VTX1")

    print(f"=== VTX1 Locations ===")
    print(f"Original: offset=0x{orig_vtx1_off:X}, size=0x{orig_vtx1_sz:X} ({orig_vtx1_sz} bytes)")
    print(f"Exported: offset=0x{exp_vtx1_off:X}, size=0x{exp_vtx1_sz:X} ({exp_vtx1_sz} bytes)")
    print(f"Size difference: {exp_vtx1_sz - orig_vtx1_sz} bytes")
    print()

    orig_vtx1 = parse_vtx1(orig_data, orig_vtx1_off)
    exp_vtx1 = parse_vtx1(exp_data, exp_vtx1_off)

    # ============================================================
    # 2. Compare arrayFormat entries
    # ============================================================
    print(f"=== ArrayFormat Entries ===")
    print(f"Original: {len(orig_vtx1['entries'])} entries (including terminator)")
    print(f"Exported: {len(exp_vtx1['entries'])} entries (including terminator)")
    print()

    max_entries = max(len(orig_vtx1['entries']), len(exp_vtx1['entries']))
    for i in range(max_entries):
        o = orig_vtx1['entries'][i] if i < len(orig_vtx1['entries']) else None
        e = exp_vtx1['entries'][i] if i < len(exp_vtx1['entries']) else None

        if o and e:
            name = GX_ATTR_NAMES.get(o['arrayType'], f"0x{o['arrayType']:X}")
            match = "MATCH" if o['raw'] == e['raw'] else "DIFFER"
            print(f"  Entry {i}: {name} [{match}]")
            print(f"    Original: type={o['arrayType']:3d} compCnt={o['compCnt']} compType={o['compType']} fracBits={o['fracBits']} extra={o['extra']}")
            print(f"    Exported: type={e['arrayType']:3d} compCnt={e['compCnt']} compType={e['compType']} fracBits={e['fracBits']} extra={e['extra']}")
            if o['raw'] != e['raw']:
                print(f"    Orig raw: {o['raw'].hex()}")
                print(f"    Exp  raw: {e['raw'].hex()}")
        elif o:
            print(f"  Entry {i}: ONLY IN ORIGINAL")
        elif e:
            print(f"  Entry {i}: ONLY IN EXPORTED")
        print()

    # ============================================================
    # 3. Compare data pool offsets
    # ============================================================
    print(f"=== Data Pool Offsets ===")
    for i in range(13):
        o_off = orig_vtx1['pool_offsets'][i]
        e_off = exp_vtx1['pool_offsets'][i]
        if o_off != 0 or e_off != 0:
            match = "MATCH" if o_off == e_off else "DIFFER"
            print(f"  Pool {i:2d}: orig=0x{o_off:08X} exp=0x{e_off:08X} [{match}]")
    print()

    # ============================================================
    # 4. Compare position pool
    # ============================================================
    print(f"=== Position Pool Comparison ===")
    # Find the position entry
    pos_entry_o = None
    pos_entry_e = None
    for ent in orig_vtx1['entries']:
        if ent['arrayType'] == 9:
            pos_entry_o = ent
            break
    for ent in exp_vtx1['entries']:
        if ent['arrayType'] == 9:
            pos_entry_e = ent
            break

    if pos_entry_o and pos_entry_e:
        # Position pool index: GX_VA_POS=9, pool index = 9 - 9 = 0?
        # Actually the pool offsets are indexed 0-12 mapping to specific attrs
        # Pool 0 = POS, Pool 1 = NRM, Pool 2-3 = CLR0/CLR1, Pool 4-11 = TEX0-TEX7
        # Wait - let me check. The 13 offsets at +0x0C correspond to:
        # [0]=pos, [1]=nrm, [2]=nbt(?), [3]=clr0, [4]=clr1, [5-12]=tex0-7
        # Actually it might be simpler: just indexed by (GXAttr - 9)
        # GX_VA_POS=9 => index 0, GX_VA_NRM=10 => index 1, GX_VA_CLR0=11 => index 2, etc.

        # Let me just find which pool offset is nonzero and makes sense for positions
        # Check pool 0 first
        o_pos_pool = orig_vtx1['pool_offsets'][0]
        e_pos_pool = exp_vtx1['pool_offsets'][0]

        stride_o = get_stride(pos_entry_o)
        stride_e = get_stride(pos_entry_e)

        print(f"  Position format: compCnt={pos_entry_o['compCnt']}, compType={pos_entry_o['compType']}, fracBits={pos_entry_o['fracBits']}")
        print(f"  Stride: orig={stride_o}, exp={stride_e}")
        print(f"  Pool offset: orig=0x{o_pos_pool:X}, exp=0x{e_pos_pool:X}")

        # Calculate pool sizes - find next nonzero pool offset or section end
        def calc_pool_size(vtx1, pool_idx):
            off = vtx1['pool_offsets'][pool_idx]
            if off == 0:
                return 0
            # Find next pool
            next_off = vtx1['size']  # default to section end
            for i in range(13):
                if i == pool_idx:
                    continue
                candidate = vtx1['pool_offsets'][i]
                if candidate > off and candidate < next_off:
                    next_off = candidate
            return next_off - off

        o_pos_size = calc_pool_size(orig_vtx1, 0)
        e_pos_size = calc_pool_size(exp_vtx1, 0)
        o_pos_count = o_pos_size // stride_o if stride_o > 0 else 0
        e_pos_count = e_pos_size // stride_e if stride_e > 0 else 0

        print(f"  Pool size: orig={o_pos_size} bytes, exp={e_pos_size} bytes")
        print(f"  Position count: orig={o_pos_count}, exp={e_pos_count}")
        print()

        # Read positions
        if pos_entry_o['compType'] == 4:  # F32
            o_positions = read_positions_f32(orig_data, orig_vtx1_off, o_pos_pool, min(o_pos_count, 99999))
            e_positions = read_positions_f32(exp_data, exp_vtx1_off, e_pos_pool, min(e_pos_count, 99999))
        elif pos_entry_o['compType'] == 3:  # S16
            o_positions = read_positions_s16(orig_data, orig_vtx1_off, o_pos_pool, min(o_pos_count, 99999), pos_entry_o['fracBits'])
            e_positions = read_positions_s16(exp_data, exp_vtx1_off, e_pos_pool, min(e_pos_count, 99999), pos_entry_e['fracBits'])
        else:
            o_positions = []
            e_positions = []
            print(f"  Unknown compType={pos_entry_o['compType']}, cannot read positions")

        if o_positions and e_positions:
            print(f"  First 10 positions:")
            for i in range(min(10, len(o_positions), len(e_positions))):
                ox, oy, oz = o_positions[i]
                ex, ey, ez = e_positions[i]
                match = "OK" if (abs(ox-ex) < 0.01 and abs(oy-ey) < 0.01 and abs(oz-ez) < 0.01) else "DIFF"
                print(f"    [{i:4d}] orig=({ox:10.4f}, {oy:10.4f}, {oz:10.4f})  exp=({ex:10.4f}, {ey:10.4f}, {ez:10.4f})  [{match}]")

            print(f"  Last 10 positions:")
            start_o = max(0, len(o_positions) - 10)
            start_e = max(0, len(e_positions) - 10)
            for i in range(10):
                oi = start_o + i
                ei = start_e + i
                if oi < len(o_positions) and ei < len(e_positions):
                    ox, oy, oz = o_positions[oi]
                    ex, ey, ez = e_positions[ei]
                    match = "OK" if (abs(ox-ex) < 0.01 and abs(oy-ey) < 0.01 and abs(oz-ez) < 0.01) else "DIFF"
                    print(f"    [o{oi:4d}/e{ei:4d}] orig=({ox:10.4f}, {oy:10.4f}, {oz:10.4f})  exp=({ex:10.4f}, {ey:10.4f}, {ez:10.4f})  [{match}]")
            print()

            # Count mismatches
            min_count = min(len(o_positions), len(e_positions))
            mismatches = 0
            first_mismatch = -1
            for i in range(min_count):
                ox, oy, oz = o_positions[i]
                ex, ey, ez = e_positions[i]
                if abs(ox-ex) > 0.01 or abs(oy-ey) > 0.01 or abs(oz-ez) > 0.01:
                    mismatches += 1
                    if first_mismatch < 0:
                        first_mismatch = i
            print(f"  Total mismatched positions (of {min_count} compared): {mismatches}")
            if first_mismatch >= 0:
                print(f"  First mismatch at index {first_mismatch}")
                ox, oy, oz = o_positions[first_mismatch]
                ex, ey, ez = e_positions[first_mismatch]
                print(f"    orig=({ox:.4f}, {oy:.4f}, {oz:.4f})")
                print(f"    exp =({ex:.4f}, {ey:.4f}, {ez:.4f})")
    print()

    # ============================================================
    # 5. Compare normal pool
    # ============================================================
    print(f"=== Normal Pool Comparison ===")
    nrm_entry_o = None
    nrm_entry_e = None
    for ent in orig_vtx1['entries']:
        if ent['arrayType'] == 10:
            nrm_entry_o = ent
            break
    for ent in exp_vtx1['entries']:
        if ent['arrayType'] == 10:
            nrm_entry_e = ent
            break

    if nrm_entry_o:
        o_nrm_pool = orig_vtx1['pool_offsets'][1]
        e_nrm_pool = exp_vtx1['pool_offsets'][1]
        nrm_stride_o = get_stride(nrm_entry_o)
        nrm_stride_e = get_stride(nrm_entry_e) if nrm_entry_e else 0

        o_nrm_size = calc_pool_size(orig_vtx1, 1)
        e_nrm_size = calc_pool_size(exp_vtx1, 1)
        o_nrm_count = o_nrm_size // nrm_stride_o if nrm_stride_o > 0 else 0
        e_nrm_count = e_nrm_size // nrm_stride_e if nrm_stride_e > 0 else 0

        print(f"  Normal format: compCnt={nrm_entry_o['compCnt']}, compType={nrm_entry_o['compType']}, fracBits={nrm_entry_o['fracBits']}")
        print(f"  Stride: orig={nrm_stride_o}, exp={nrm_stride_e}")
        print(f"  Pool size: orig={o_nrm_size}, exp={e_nrm_size}")
        print(f"  Normal count: orig={o_nrm_count}, exp={e_nrm_count}")
        print(f"  Size match: {'YES' if o_nrm_size == e_nrm_size else 'NO'}")
    print()

    # ============================================================
    # 6. Compare texcoord pools
    # ============================================================
    print(f"=== TexCoord Pool Comparison ===")
    for tex_i in range(8):
        pool_idx = tex_i + 5  # pools 5-12 = tex0-tex7 (after pos, nrm, nbt(?), clr0, clr1)
        # Actually the pool indexing: let me check by looking at which pools are nonzero
        pass

    # Let me just print all nonzero pools
    print(f"  All nonzero pool offsets:")
    for i in range(13):
        if orig_vtx1['pool_offsets'][i] != 0 or exp_vtx1['pool_offsets'][i] != 0:
            o_sz = calc_pool_size(orig_vtx1, i)
            e_sz = calc_pool_size(exp_vtx1, i)
            match = "MATCH" if o_sz == e_sz else "DIFFER"
            print(f"    Pool {i:2d}: orig offset=0x{orig_vtx1['pool_offsets'][i]:08X} size={o_sz:6d}  exp offset=0x{exp_vtx1['pool_offsets'][i]:08X} size={e_sz:6d}  [{match}]")
    print()

    # ============================================================
    # 7. SHP1 - Read batch 0 and batch 9 primitive data
    # ============================================================
    orig_shp1_off, orig_shp1_sz = find_section(orig_data, b"SHP1")
    exp_shp1_off, exp_shp1_sz = find_section(exp_data, b"SHP1")

    if orig_shp1_off is None or exp_shp1_off is None:
        print("ERROR: Could not find SHP1 section in one or both files")
        return

    print(f"=== SHP1 Section ===")
    print(f"Original: offset=0x{orig_shp1_off:X}, size=0x{orig_shp1_sz:X}")
    print(f"Exported: offset=0x{exp_shp1_off:X}, size=0x{exp_shp1_sz:X}")

    orig_shp1 = parse_shp1(orig_data, orig_shp1_off)
    exp_shp1 = parse_shp1(exp_data, exp_shp1_off)

    print(f"Batch count: orig={orig_shp1['batch_count']}, exp={exp_shp1['batch_count']}")
    print()

    for batch_idx in [0, 9]:
        print(f"=== Batch {batch_idx} Primitive Data ===")

        if batch_idx >= orig_shp1['batch_count'] or batch_idx >= exp_shp1['batch_count']:
            print(f"  Batch {batch_idx} does not exist in one or both files")
            continue

        o_batch = get_batch_attribs(orig_data, orig_shp1_off, orig_shp1, batch_idx)
        e_batch = get_batch_attribs(exp_data, exp_shp1_off, exp_shp1, batch_idx)

        print(f"  Original batch: matrixType={o_batch['matrix_type']}, packets={o_batch['packet_count']}, attribs={o_batch['attribs']}")
        print(f"  Exported batch: matrixType={e_batch['matrix_type']}, packets={e_batch['packet_count']}, attribs={e_batch['attribs']}")

        # Get primitive data
        o_prim_off, o_prim_sz = get_prim_data_range(orig_data, orig_shp1_off, orig_shp1, o_batch)
        e_prim_off, e_prim_sz = get_prim_data_range(exp_data, exp_shp1_off, exp_shp1, e_batch)

        print(f"  Prim data: orig offset=0x{o_prim_off:X} size={o_prim_sz}, exp offset=0x{e_prim_off:X} size={e_prim_sz}")

        # Read position indices (GX_VA_POS = 9)
        o_pos_indices = read_indices_from_prim(orig_data, o_prim_off, o_prim_sz, o_batch['attribs'], 9)
        e_pos_indices = read_indices_from_prim(exp_data, e_prim_off, e_prim_sz, e_batch['attribs'], 9)

        print(f"  Position indices read: orig={len(o_pos_indices)}, exp={len(e_pos_indices)}")

        # Look up actual positions
        n_show = min(20, len(o_pos_indices), len(e_pos_indices))
        if n_show > 0 and o_positions and e_positions:
            print(f"  First {n_show} vertex positions (by index lookup):")
            for i in range(n_show):
                oi = o_pos_indices[i]
                ei = e_pos_indices[i]
                if oi < len(o_positions) and ei < len(e_positions):
                    ox, oy, oz = o_positions[oi]
                    ex, ey, ez = e_positions[ei]
                    idx_match = "same" if oi == ei else f"DIFF idx"
                    val_match = "OK" if (abs(ox-ex) < 0.01 and abs(oy-ey) < 0.01 and abs(oz-ez) < 0.01) else "DIFF val"
                    print(f"    v{i:3d}: idx orig={oi:5d} exp={ei:5d} [{idx_match}]  orig=({ox:10.4f},{oy:10.4f},{oz:10.4f})  exp=({ex:10.4f},{ey:10.4f},{ez:10.4f})  [{val_match}]")
                else:
                    print(f"    v{i:3d}: idx orig={oi:5d} exp={ei:5d}  OUT OF RANGE (orig pool={len(o_positions)}, exp pool={len(e_positions)})")
        print()

    # ============================================================
    # 8. Raw byte comparison of VTX1 sections
    # ============================================================
    print(f"=== Raw VTX1 Byte Comparison ===")
    min_sz = min(orig_vtx1_sz, exp_vtx1_sz)
    diffs = 0
    first_diff = -1
    for i in range(min_sz):
        if orig_data[orig_vtx1_off + i] != exp_data[exp_vtx1_off + i]:
            diffs += 1
            if first_diff < 0:
                first_diff = i
    print(f"  Bytes compared: {min_sz}")
    print(f"  Bytes differing: {diffs}")
    if first_diff >= 0:
        print(f"  First diff at offset +0x{first_diff:X} within VTX1")
        # Show context
        ctx_start = max(0, first_diff - 4)
        ctx_end = min(min_sz, first_diff + 20)
        print(f"  Orig bytes: {orig_data[orig_vtx1_off+ctx_start:orig_vtx1_off+ctx_end].hex()}")
        print(f"  Exp  bytes: {exp_data[exp_vtx1_off+ctx_start:exp_vtx1_off+ctx_end].hex()}")
    if orig_vtx1_sz != exp_vtx1_sz:
        print(f"  Extra bytes in {'exported' if exp_vtx1_sz > orig_vtx1_sz else 'original'}: {abs(exp_vtx1_sz - orig_vtx1_sz)}")

if __name__ == "__main__":
    main()
