#!/usr/bin/env python3
"""Deeper investigation of VTX1 differences."""

import struct

ORIG = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"
EXPORTED = r"C:\Users\ryana\Downloads\reconstruct_test.bmd"

def read_file(path):
    with open(path, "rb") as f:
        return f.read()

def find_section(data, tag):
    pos = 0x20
    while pos < len(data) - 8:
        sec_tag = data[pos:pos+4]
        if sec_tag == tag:
            sec_size = struct.unpack_from(">I", data, pos+4)[0]
            return pos, sec_size
        cur_size = struct.unpack_from(">I", data, pos+4)[0]
        if cur_size > 0 and cur_size < len(data):
            pos += cur_size
        else:
            pos += 4
    return None, None

def main():
    orig = read_file(ORIG)
    exp = read_file(EXPORTED)

    # Find VTX1
    o_vtx_off, o_vtx_sz = find_section(orig, b"VTX1")
    e_vtx_off, e_vtx_sz = find_section(exp, b"VTX1")

    # Position pool starts at offset 0xA0 in both, stride=6 (s16 xyz)
    o_pos_start = o_vtx_off + 0xA0
    e_pos_start = e_vtx_off + 0xA0

    # Original has 816 positions, exported has 800
    # Find where they diverge
    print("=== Position Pool Divergence Analysis ===")
    print(f"Original: 816 positions (4896 bytes), Exported: 800 positions (4800 bytes)")
    print(f"Difference: 16 positions = 96 bytes")
    print()

    # Read all positions as raw s16 triples
    def read_pos(data, start, count):
        positions = []
        for i in range(count):
            x, y, z = struct.unpack_from(">hhh", data, start + i*6)
            positions.append((x, y, z))
        return positions

    o_pos = read_pos(orig, o_pos_start, 816)
    e_pos = read_pos(exp, e_pos_start, 800)

    # Find first divergence
    for i in range(min(816, 800)):
        if o_pos[i] != e_pos[i]:
            print(f"First position divergence at index {i}")
            for j in range(max(0, i-3), min(len(o_pos), len(e_pos), i+5)):
                match = "OK" if o_pos[j] == e_pos[j] else "DIFF"
                print(f"  [{j:4d}] orig={o_pos[j]}  exp={e_pos[j]}  [{match}]")
            break
    print()

    # Check if exported is a subset or reordered version of original
    o_set = set(o_pos)
    e_set = set(e_pos)
    only_orig = o_set - e_set
    only_exp = e_set - o_set
    print(f"Unique positions in original: {len(o_set)}")
    print(f"Unique positions in exported: {len(e_set)}")
    print(f"Positions only in original: {len(only_orig)}")
    print(f"Positions only in exported: {len(only_exp)}")
    print()

    if only_orig:
        print("Sample positions only in original (first 10):")
        for p in list(only_orig)[:10]:
            scale = 1.0/256
            print(f"  raw=({p[0]:6d},{p[1]:6d},{p[2]:6d})  float=({p[0]*scale:10.4f},{p[1]*scale:10.4f},{p[2]*scale:10.4f})")
    if only_exp:
        print("Sample positions only in exported (first 10):")
        for p in list(only_exp)[:10]:
            scale = 1.0/256
            print(f"  raw=({p[0]:6d},{p[1]:6d},{p[2]:6d})  float=({p[0]*scale:10.4f},{p[1]*scale:10.4f},{p[2]*scale:10.4f})")
    print()

    # Check for duplicates in each pool
    o_dupes = len(o_pos) - len(o_set)
    e_dupes = len(e_pos) - len(e_set)
    print(f"Duplicate positions in original: {o_dupes}")
    print(f"Duplicate positions in exported: {e_dupes}")
    print()

    # Are there duplicate positions in original that got deduplicated in export?
    if o_dupes > 0:
        from collections import Counter
        o_counts = Counter(o_pos)
        duped = [(p, c) for p, c in o_counts.items() if c > 1]
        print(f"Positions that appear more than once in original: {len(duped)}")
        for p, c in duped[:10]:
            scale = 1.0/256
            in_exp = p in e_set
            print(f"  ({p[0]*scale:10.4f},{p[1]*scale:10.4f},{p[2]*scale:10.4f}) x{c} {'(also in exp)' if in_exp else '(NOT in exp)'}")
    print()

    # ============================================================
    # Batch 9 deeper look - it's a weighted batch (matrixType=3)
    # ============================================================
    print("=== Batch 9 Detailed Analysis ===")
    o_shp_off, _ = find_section(orig, b"SHP1")
    e_shp_off, _ = find_section(exp, b"SHP1")

    # Read batch 9 header (each batch header is 0x28 bytes)
    def read_batch(data, shp_base, idx):
        batch_hdr_off = struct.unpack_from(">I", data, shp_base+0x0C)[0]
        b = shp_base + batch_hdr_off + idx * 0x28
        return {
            "matrixType": data[b],
            "pad": data[b+1],
            "firstPacket": struct.unpack_from(">H", data, b+0x02)[0],
            "packetCount": struct.unpack_from(">H", data, b+0x04)[0],
            "firstAttrib": struct.unpack_from(">H", data, b+0x06)[0],
            "firstMtxData": struct.unpack_from(">H", data, b+0x08)[0],
            "firstMtxGrp": struct.unpack_from(">H", data, b+0x0A)[0],
            "raw": data[b:b+0x28].hex(),
        }

    for idx in [0, 9]:
        ob = read_batch(orig, o_shp_off, idx)
        eb = read_batch(exp, e_shp_off, idx)
        print(f"  Batch {idx} orig: {ob}")
        print(f"  Batch {idx} exp:  {eb}")
        print()

    # Read attrib descriptors for batch 9
    def read_attribs(data, shp_base, first_attrib):
        attrib_off = struct.unpack_from(">I", data, shp_base+0x18)[0]
        base = shp_base + attrib_off + first_attrib * 8
        attribs = []
        while True:
            attr = struct.unpack_from(">I", data, base)[0]
            typ = struct.unpack_from(">I", data, base+4)[0]
            if attr == 0xFF or attr == 0xFFFFFFFF:
                break
            attribs.append((attr, typ))
            base += 8
        return attribs

    print("Batch 9 attribs (original):", read_attribs(orig, o_shp_off, read_batch(orig, o_shp_off, 9)["firstAttrib"]))
    print("Batch 9 attribs (exported):", read_attribs(exp, e_shp_off, read_batch(exp, e_shp_off, 9)["firstAttrib"]))
    print()

    # For batch 9, read the actual prim data raw bytes
    def get_prim_range(data, shp_base, batch_idx):
        b = read_batch(data, shp_base, batch_idx)
        draw_init_off = struct.unpack_from(">I", data, shp_base+0x28)[0]
        prim_off = struct.unpack_from(">I", data, shp_base+0x20)[0]

        first_pkt = b["firstPacket"]
        loc = shp_base + draw_init_off + first_pkt * 8
        pkt_size = struct.unpack_from(">I", data, loc)[0]
        pkt_offset = struct.unpack_from(">I", data, loc+4)[0]
        return shp_base + prim_off + pkt_offset, pkt_size

    o_prim, o_sz = get_prim_range(orig, o_shp_off, 9)
    e_prim, e_sz = get_prim_range(exp, e_shp_off, 9)
    print(f"Batch 9 prim data: orig at 0x{o_prim:X} size={o_sz}, exp at 0x{e_prim:X} size={e_sz}")
    print(f"First 64 bytes orig: {orig[o_prim:o_prim+64].hex()}")
    print(f"First 64 bytes exp:  {exp[e_prim:e_prim+64].hex()}")
    print()

    # Parse batch 9 prim primitives manually
    # Attribs are (13,3),(14,3),(15,3) = TEX0 INDEX16, TEX1 INDEX16, TEX2 INDEX16
    # Each vertex = 6 bytes (3x u16)
    print("Batch 9 primitive parsing (first packet):")
    for label, data_src, prim_start, prim_sz in [("orig", orig, o_prim, o_sz), ("exp", exp, e_prim, e_sz)]:
        pos = prim_start
        end = prim_start + prim_sz
        prim_count = 0
        total_verts = 0
        while pos < end:
            op = data_src[pos]
            pos += 1
            if op == 0:
                continue
            if op & 0x80 == 0:
                print(f"  [{label}] Unknown opcode 0x{op:02X} at offset {pos-1-prim_start}")
                break
            vc = struct.unpack_from(">H", data_src, pos)[0]
            pos += 2
            prim_count += 1
            total_verts += vc
            # Skip vertex data: each vertex = 6 bytes for 3x INDEX16
            pos += vc * 6
        print(f"  [{label}] primitives={prim_count}, total_verts={total_verts}")

    # ============================================================
    # Normal pool: 832 vs 880
    # ============================================================
    print()
    print("=== Normal Pool Analysis ===")
    o_nrm_off = o_vtx_off + 0x13C0  # from pool offset
    e_nrm_off = e_vtx_off + 0x1360
    o_nrm_count = 832
    e_nrm_count = 880
    print(f"Original normals: {o_nrm_count}, Exported normals: {e_nrm_count}")
    print(f"Difference: {e_nrm_count - o_nrm_count} normals ({(e_nrm_count - o_nrm_count)*6} bytes)")

    # Check first few normals
    def read_nrm(data, start, count):
        norms = []
        for i in range(count):
            x, y, z = struct.unpack_from(">hhh", data, start + i*6)
            norms.append((x, y, z))
        return norms

    o_nrm = read_nrm(orig, o_nrm_off, o_nrm_count)
    e_nrm = read_nrm(exp, e_nrm_off, e_nrm_count)

    # Find divergence
    for i in range(min(o_nrm_count, e_nrm_count)):
        if o_nrm[i] != e_nrm[i]:
            print(f"First normal divergence at index {i}")
            break
    else:
        print(f"First {min(o_nrm_count, e_nrm_count)} normals match!")

    o_nset = set(o_nrm)
    e_nset = set(e_nrm)
    print(f"Unique normals: orig={len(o_nset)}, exp={len(e_nset)}")
    print(f"Only in original: {len(o_nset - e_nset)}, Only in exported: {len(e_nset - o_nset)}")

    # ============================================================
    # Texcoord pools - compare first few values
    # ============================================================
    print()
    print("=== TexCoord Pool Analysis ===")
    # TEX0 at pool 5: orig=0x2740, exp=0x2800, size=6688 each (f32 st = 8 bytes per entry)
    # 6688 / 8 = 836 entries
    o_tex0 = o_vtx_off + 0x2740
    e_tex0 = e_vtx_off + 0x2800
    tex0_count = 6688 // 8
    print(f"TEX0: {tex0_count} entries each, size=6688 bytes each")

    # Compare raw bytes
    o_tex0_data = orig[o_tex0:o_tex0+6688]
    e_tex0_data = exp[e_tex0:e_tex0+6688]
    if o_tex0_data == e_tex0_data:
        print("  TEX0 pools are IDENTICAL")
    else:
        diffs = sum(1 for a, b in zip(o_tex0_data, e_tex0_data) if a != b)
        print(f"  TEX0 pools differ in {diffs} bytes")
        # Find first diff
        for i in range(len(o_tex0_data)):
            if o_tex0_data[i] != e_tex0_data[i]:
                print(f"  First diff at byte {i} (entry {i//8})")
                break

    # TEX1 at pool 6
    o_tex1 = o_vtx_off + 0x4160
    e_tex1 = e_vtx_off + 0x4220
    tex1_count = 7648 // 8
    o_tex1_data = orig[o_tex1:o_tex1+7648]
    e_tex1_data = exp[e_tex1:e_tex1+7648]
    print(f"TEX1: {tex1_count} entries each, size=7648 bytes each")
    if o_tex1_data == e_tex1_data:
        print("  TEX1 pools are IDENTICAL")
    else:
        diffs = sum(1 for a, b in zip(o_tex1_data, e_tex1_data) if a != b)
        print(f"  TEX1 pools differ in {diffs} bytes")

    # TEX2 at pool 7
    o_tex2 = o_vtx_off + 0x5F40
    e_tex2 = e_vtx_off + 0x6000
    tex2_count = 7808 // 8
    o_tex2_data = orig[o_tex2:o_tex2+7808]
    e_tex2_data = exp[e_tex2:e_tex2+7808]
    print(f"TEX2: {tex2_count} entries each, size=7808 bytes each")
    if o_tex2_data == e_tex2_data:
        print("  TEX2 pools are IDENTICAL")
    else:
        diffs = sum(1 for a, b in zip(o_tex2_data, e_tex2_data) if a != b)
        print(f"  TEX2 pools differ in {diffs} bytes")

if __name__ == "__main__":
    main()
