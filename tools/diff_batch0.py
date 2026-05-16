"""Exhaustive byte-level comparison of SHP1 batch 0 between two BMD files."""
import struct
import sys

def read_file(path):
    with open(path, "rb") as f:
        return f.read()

def find_section(data, tag):
    """Find a section by its 4-byte tag, return offset to section start."""
    pos = 0
    while pos < len(data) - 8:
        if data[pos:pos+4] == tag:
            return pos
        pos += 1
    return None

def read_u8(data, off): return struct.unpack_from(">B", data, off)[0]
def read_u16(data, off): return struct.unpack_from(">H", data, off)[0]
def read_u32(data, off): return struct.unpack_from(">I", data, off)[0]
def read_s16(data, off): return struct.unpack_from(">h", data, off)[0]
def read_f32(data, off): return struct.unpack_from(">f", data, off)[0]

def parse_shp1(data):
    off = find_section(data, b"SHP1")
    if off is None:
        raise ValueError("SHP1 not found")

    sec_size = read_u32(data, off + 4)
    batch_count = read_u16(data, off + 8)
    # pad at +0x0A
    batches_off   = read_u32(data, off + 0x0C)  # offset to batch array
    # +0x10 = index remap table offset
    # +0x14 = unknown/zero
    attribs_off   = read_u32(data, off + 0x18)  # attrib descriptors
    mtx_table_off = read_u32(data, off + 0x1C)  # matrix table (u16 indices)
    prim_data_off = read_u32(data, off + 0x20)  # primitive data
    mtx_init_off  = read_u32(data, off + 0x24)  # matrix init data (MatrixData)
    draw_init_off = read_u32(data, off + 0x28)  # draw init data (PacketLocation)

    return {
        "shp1_off": off,
        "sec_size": sec_size,
        "batch_count": batch_count,
        "batches_off": off + batches_off,
        "attribs_off": off + attribs_off,
        "mtx_table_off": off + mtx_table_off,
        "prim_data_off": off + prim_data_off,
        "mtx_init_off": off + mtx_init_off,
        "draw_init_off": off + draw_init_off,
    }

def parse_batch_header(data, off):
    """Parse 40-byte batch header."""
    return {
        "matType":         read_u8(data, off + 0x00),
        "useMtxType":      read_u8(data, off + 0x01),  # hasNormals?
        "packetCount":     read_u16(data, off + 0x02),
        "attribOffset":    read_u16(data, off + 0x04),  # offset into attrib desc array (in entries)
        "firstMatrixData": read_u16(data, off + 0x06),
        "firstDrawInit":   read_u16(data, off + 0x08),
        "pad0A":           read_u16(data, off + 0x0A),
        "bbox_minX":       read_f32(data, off + 0x0C),
        "bbox_minY":       read_f32(data, off + 0x10),
        "bbox_minZ":       read_f32(data, off + 0x14),
        "bbox_maxX":       read_f32(data, off + 0x18),
        "bbox_maxY":       read_f32(data, off + 0x1C),
        "bbox_maxZ":       read_f32(data, off + 0x20),
        "raw": data[off:off+0x28],
    }

def parse_attrib_descriptors(data, off):
    """Parse attrib descriptors starting at off, until terminator (0xFF, 0x00000000)."""
    attribs = []
    pos = off
    while True:
        attr_type = read_u32(data, pos)
        data_type = read_u32(data, pos + 4)
        if attr_type == 0xFF:  # GX_VA_NULL terminator
            break
        attribs.append({"attr": attr_type, "dataType": data_type})
        pos += 8
    return attribs

def parse_matrix_init(data, off):
    """Parse MatrixData entry (8 bytes): useMtxIndex(u16), count(u16), firstIndex(u32)."""
    return {
        "useMtxIndex": read_u16(data, off + 0),
        "count":       read_u16(data, off + 2),
        "firstIndex":  read_u32(data, off + 4),
    }

def parse_draw_init(data, off):
    """Parse PacketLocation entry (8 bytes): size(u32), offset(u32)."""
    return {
        "displayListSize":   read_u32(data, off + 0),
        "displayListOffset": read_u32(data, off + 4),
    }

def hex_dump(data, count=None):
    if count is not None:
        data = data[:count]
    return " ".join(f"{b:02X}" for b in data)

def compare_field(name, val_a, val_b):
    if val_a != val_b:
        print(f"  *** DIFF {name}: orig={val_a} export={val_b}")
        return 1
    else:
        print(f"      {name}: {val_a}")
        return 0

def compare_field_hex(name, val_a, val_b):
    if val_a != val_b:
        print(f"  *** DIFF {name}: orig=0x{val_a:X} export=0x{val_b:X}")
        return 1
    else:
        print(f"      {name}: 0x{val_a:X}")
        return 0

def compare_field_float(name, val_a, val_b):
    if val_a != val_b:
        print(f"  *** DIFF {name}: orig={val_a:.6f} export={val_b:.6f}")
        return 1
    else:
        print(f"      {name}: {val_a:.6f}")
        return 0

def main():
    orig_path = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"
    export_path = r"C:\Users\ryana\Downloads\reconstruct_test.bmd"

    orig = read_file(orig_path)
    export = read_file(export_path)

    shp_orig = parse_shp1(orig)
    shp_export = parse_shp1(export)

    print("=" * 70)
    print("SHP1 HEADER COMPARISON")
    print("=" * 70)
    print(f"  Original  SHP1 at 0x{shp_orig['shp1_off']:X}, size={shp_orig['sec_size']}")
    print(f"  Export    SHP1 at 0x{shp_export['shp1_off']:X}, size={shp_export['sec_size']}")
    print(f"  Batch count: orig={shp_orig['batch_count']}, export={shp_export['batch_count']}")

    diffs = 0

    # ---- 1. BATCH 0 HEADER (40 bytes) ----
    print()
    print("=" * 70)
    print("BATCH 0 HEADER (40 bytes)")
    print("=" * 70)

    b0_orig = parse_batch_header(orig, shp_orig["batches_off"])
    b0_export = parse_batch_header(export, shp_export["batches_off"])

    for key in ["matType", "useMtxType", "packetCount", "attribOffset",
                "firstMatrixData", "firstDrawInit", "pad0A"]:
        diffs += compare_field(key, b0_orig[key], b0_export[key])

    for key in ["bbox_minX", "bbox_minY", "bbox_minZ", "bbox_maxX", "bbox_maxY", "bbox_maxZ"]:
        diffs += compare_field_float(key, b0_orig[key], b0_export[key])

    # Raw hex of batch header
    print(f"\n  Raw batch header (orig):   {hex_dump(b0_orig['raw'])}")
    print(f"  Raw batch header (export): {hex_dump(b0_export['raw'])}")
    if b0_orig["raw"] != b0_export["raw"]:
        print("  *** RAW BATCH HEADER DIFFERS")
        for i in range(len(b0_orig["raw"])):
            if b0_orig["raw"][i] != b0_export["raw"][i]:
                print(f"      byte 0x{i:02X}: orig=0x{b0_orig['raw'][i]:02X} export=0x{b0_export['raw'][i]:02X}")

    # ---- 2. ATTRIB DESCRIPTORS ----
    print()
    print("=" * 70)
    print("ATTRIB DESCRIPTORS FOR BATCH 0")
    print("=" * 70)

    # attribOffset is index into attrib array; each entry is 8 bytes
    attr_off_orig = shp_orig["attribs_off"] + b0_orig["attribOffset"] * 8
    attr_off_export = shp_export["attribs_off"] + b0_export["attribOffset"] * 8

    attribs_orig = parse_attrib_descriptors(orig, attr_off_orig)
    attribs_export = parse_attrib_descriptors(export, attr_off_export)

    print(f"  Orig attribs ({len(attribs_orig)} entries) at file offset 0x{attr_off_orig:X}:")
    for i, a in enumerate(attribs_orig):
        print(f"    [{i}] attr=0x{a['attr']:02X} dataType=0x{a['dataType']:02X}")

    print(f"  Export attribs ({len(attribs_export)} entries) at file offset 0x{attr_off_export:X}:")
    for i, a in enumerate(attribs_export):
        print(f"    [{i}] attr=0x{a['attr']:02X} dataType=0x{a['dataType']:02X}")

    if attribs_orig != attribs_export:
        print("  *** ATTRIB DESCRIPTORS DIFFER!")
        diffs += 1
    else:
        print("  Attrib descriptors MATCH.")

    # Also dump raw bytes of attrib region for both
    attr_raw_len = (len(attribs_orig) + 1) * 8  # +1 for terminator
    print(f"\n  Raw attrib bytes (orig):   {hex_dump(orig[attr_off_orig:attr_off_orig+attr_raw_len])}")
    attr_raw_len_e = (len(attribs_export) + 1) * 8
    print(f"  Raw attrib bytes (export): {hex_dump(export[attr_off_export:attr_off_export+attr_raw_len_e])}")

    # ---- 3. MATRIX INIT DATA ----
    print()
    print("=" * 70)
    print("MATRIX INIT DATA FOR BATCH 0")
    print("=" * 70)

    pkt_count = b0_orig["packetCount"]
    pkt_count_e = b0_export["packetCount"]

    for p in range(max(pkt_count, pkt_count_e)):
        mi_off_orig = shp_orig["mtx_init_off"] + (b0_orig["firstMatrixData"] + p) * 8
        mi_off_export = shp_export["mtx_init_off"] + (b0_export["firstMatrixData"] + p) * 8

        print(f"\n  Packet {p}:")
        if p < pkt_count:
            mi_orig = parse_matrix_init(orig, mi_off_orig)
            print(f"    Orig:   useMtxIndex={mi_orig['useMtxIndex']}, count={mi_orig['count']}, firstIndex={mi_orig['firstIndex']}")
            print(f"    Raw:    {hex_dump(orig[mi_off_orig:mi_off_orig+8])}")
        else:
            print(f"    Orig:   (no packet {p})")

        if p < pkt_count_e:
            mi_export = parse_matrix_init(export, mi_off_export)
            print(f"    Export: useMtxIndex={mi_export['useMtxIndex']}, count={mi_export['count']}, firstIndex={mi_export['firstIndex']}")
            print(f"    Raw:    {hex_dump(export[mi_off_export:mi_off_export+8])}")
        else:
            print(f"    Export: (no packet {p})")

        if p < pkt_count and p < pkt_count_e:
            if mi_orig != mi_export:
                print(f"    *** MATRIX INIT DIFFERS for packet {p}")
                diffs += 1

            # Dump matrix table entries
            mt_off_orig = shp_orig["mtx_table_off"] + mi_orig["firstIndex"] * 2
            mt_off_export = shp_export["mtx_table_off"] + mi_export["firstIndex"] * 2

            mt_count = mi_orig["count"]
            mt_count_e = mi_export["count"]

            mt_vals_orig = [read_u16(orig, mt_off_orig + i*2) for i in range(mt_count)]
            mt_vals_export = [read_u16(export, mt_off_export + i*2) for i in range(mt_count_e)]

            print(f"    Mtx table (orig,  count={mt_count}): {mt_vals_orig}")
            print(f"    Mtx table (export, count={mt_count_e}): {mt_vals_export}")
            if mt_vals_orig != mt_vals_export:
                print(f"    *** MATRIX TABLE ENTRIES DIFFER")
                diffs += 1

    # ---- 4. DRAW INIT DATA + PRIMITIVE DATA ----
    print()
    print("=" * 70)
    print("DRAW INIT DATA + PRIMITIVE DATA FOR BATCH 0")
    print("=" * 70)

    for p in range(max(pkt_count, pkt_count_e)):
        di_off_orig = shp_orig["draw_init_off"] + (b0_orig["firstDrawInit"] + p) * 8
        di_off_export = shp_export["draw_init_off"] + (b0_export["firstDrawInit"] + p) * 8

        print(f"\n  Packet {p} draw init:")

        di_orig = parse_draw_init(orig, di_off_orig) if p < pkt_count else None
        di_export = parse_draw_init(export, di_off_export) if p < pkt_count_e else None

        if di_orig:
            print(f"    Orig:   size={di_orig['displayListSize']}, offset=0x{di_orig['displayListOffset']:X}")
        if di_export:
            print(f"    Export: size={di_export['displayListSize']}, offset=0x{di_export['displayListOffset']:X}")

        if di_orig and di_export:
            if di_orig["displayListSize"] != di_export["displayListSize"]:
                print(f"    *** DISPLAY LIST SIZE DIFFERS: orig={di_orig['displayListSize']} export={di_export['displayListSize']}")
                diffs += 1
            if di_orig["displayListOffset"] != di_export["displayListOffset"]:
                print(f"    *** DISPLAY LIST OFFSET DIFFERS")
                diffs += 1

            # Read primitive data
            prim_off_orig = shp_orig["prim_data_off"] + di_orig["displayListOffset"]
            prim_off_export = shp_export["prim_data_off"] + di_export["displayListOffset"]
            prim_size_orig = di_orig["displayListSize"]
            prim_size_export = di_export["displayListSize"]

            prim_orig = orig[prim_off_orig:prim_off_orig + prim_size_orig]
            prim_export = export[prim_off_export:prim_off_export + prim_size_export]

            print(f"\n    Primitive data comparison (packet {p}):")
            print(f"    Orig  size: {prim_size_orig} bytes, file offset 0x{prim_off_orig:X}")
            print(f"    Export size: {prim_size_export} bytes, file offset 0x{prim_off_export:X}")

            # Parse primitive commands
            for label, pdata, psize in [("ORIG", prim_orig, prim_size_orig),
                                         ("EXPORT", prim_export, prim_size_export)]:
                print(f"\n    {label} primitive commands:")
                pos = 0
                cmd_idx = 0
                while pos < psize:
                    cmd_byte = pdata[pos]
                    if cmd_byte == 0x00:
                        # NOP padding - count consecutive zeros
                        nop_start = pos
                        while pos < psize and pdata[pos] == 0x00:
                            pos += 1
                        print(f"      [{cmd_idx}] NOP padding: {pos - nop_start} bytes")
                        cmd_idx += 1
                        continue

                    prim_type = cmd_byte & 0xF8
                    vat_idx = cmd_byte & 0x07

                    if pos + 2 >= psize:
                        print(f"      [{cmd_idx}] Truncated at pos {pos}: byte=0x{cmd_byte:02X}")
                        break

                    vtx_count = read_u16(pdata, pos + 1)

                    prim_names = {
                        0x80: "GX_QUADS",
                        0x90: "GX_TRIANGLES",
                        0x98: "GX_TRIANGLESTRIP",
                        0xA0: "GX_TRIANGLEFAN",
                        0xA8: "GX_LINES",
                        0xB0: "GX_LINESTRIP",
                        0xB8: "GX_POINTS",
                    }
                    pname = prim_names.get(prim_type, f"0x{prim_type:02X}")

                    # Calculate vertex stride from attribs
                    attribs = attribs_orig if label == "ORIG" else attribs_export
                    stride = 0
                    for a in attribs:
                        dt = a["dataType"]
                        if dt == 1:    # direct u8 / index8
                            stride += 1
                        elif dt == 2:  # direct u16 / index16
                            stride += 2
                        elif dt == 3:  # direct s8
                            stride += 1
                        elif dt == 4:  # direct s16
                            stride += 2
                        elif dt == 5:  # direct f32
                            stride += 4
                        else:
                            stride += 2  # fallback assume index16

                    vtx_data_size = vtx_count * stride
                    total_cmd_size = 3 + vtx_data_size

                    print(f"      [{cmd_idx}] {pname} vat={vat_idx} vtxCount={vtx_count} stride={stride} dataSize={vtx_data_size}")

                    # Dump first 30 bytes of vertex data
                    vtx_start = pos + 3
                    dump_len = min(30, vtx_data_size)
                    if vtx_start + dump_len <= psize:
                        print(f"        First {dump_len} bytes: {hex_dump(pdata[vtx_start:vtx_start+dump_len])}")

                    # Also dump first few complete vertices
                    if stride > 0:
                        n_show = min(3, vtx_count)
                        for vi in range(n_show):
                            v_off = vtx_start + vi * stride
                            if v_off + stride <= psize:
                                print(f"        vtx[{vi}]: {hex_dump(pdata[v_off:v_off+stride])}")

                    pos += total_cmd_size
                    cmd_idx += 1

            # Byte-by-byte diff of primitive data
            min_size = min(prim_size_orig, prim_size_export)
            diff_count = 0
            first_diffs = []
            for i in range(min_size):
                if prim_orig[i] != prim_export[i]:
                    diff_count += 1
                    if len(first_diffs) < 30:
                        first_diffs.append((i, prim_orig[i], prim_export[i]))

            if prim_size_orig != prim_size_export:
                diff_count += abs(prim_size_orig - prim_size_export)

            print(f"\n    Byte diff summary: {diff_count} differing bytes out of {min_size} compared")
            if first_diffs:
                print(f"    First {len(first_diffs)} differences:")
                for off, a, b in first_diffs:
                    print(f"      offset 0x{off:04X}: orig=0x{a:02X} export=0x{b:02X}")
                diffs += 1

            if prim_orig == prim_export:
                print("    Primitive data is IDENTICAL.")

    # ---- 5. BOUNDING BOX ANALYSIS ----
    print()
    print("=" * 70)
    print("BOUNDING BOX ANALYSIS")
    print("=" * 70)

    print(f"  Orig  bbox: min=({b0_orig['bbox_minX']:.2f}, {b0_orig['bbox_minY']:.2f}, {b0_orig['bbox_minZ']:.2f}) "
          f"max=({b0_orig['bbox_maxX']:.2f}, {b0_orig['bbox_maxY']:.2f}, {b0_orig['bbox_maxZ']:.2f})")
    print(f"  Export bbox: min=({b0_export['bbox_minX']:.2f}, {b0_export['bbox_minY']:.2f}, {b0_export['bbox_minZ']:.2f}) "
          f"max=({b0_export['bbox_maxX']:.2f}, {b0_export['bbox_maxY']:.2f}, {b0_export['bbox_maxZ']:.2f})")

    # Center point
    cx_o = (b0_orig['bbox_minX'] + b0_orig['bbox_maxX']) / 2
    cy_o = (b0_orig['bbox_minY'] + b0_orig['bbox_maxY']) / 2
    cz_o = (b0_orig['bbox_minZ'] + b0_orig['bbox_maxZ']) / 2
    cx_e = (b0_export['bbox_minX'] + b0_export['bbox_maxX']) / 2
    cy_e = (b0_export['bbox_minY'] + b0_export['bbox_maxY']) / 2
    cz_e = (b0_export['bbox_minZ'] + b0_export['bbox_maxZ']) / 2

    print(f"  Orig  center: ({cx_o:.2f}, {cy_o:.2f}, {cz_o:.2f})")
    print(f"  Export center: ({cx_e:.2f}, {cy_e:.2f}, {cz_e:.2f})")

    if (b0_orig['bbox_minY'] != b0_export['bbox_minY'] or
        b0_orig['bbox_maxY'] != b0_export['bbox_maxY']):
        print(f"\n  *** WARNING: Y-axis bbox differs!")
        print(f"      Orig  Y range: {b0_orig['bbox_minY']:.2f} to {b0_orig['bbox_maxY']:.2f} (height {b0_orig['bbox_maxY']-b0_orig['bbox_minY']:.2f})")
        print(f"      Export Y range: {b0_export['bbox_minY']:.2f} to {b0_export['bbox_maxY']:.2f} (height {b0_export['bbox_maxY']-b0_export['bbox_minY']:.2f})")
        print(f"      If head bbox Y is near 0 instead of ~97, game may render head at wrong position!")

    # ---- 6. SUMMARY ----
    print()
    print("=" * 70)
    print(f"TOTAL DIFFERENCES FOUND: {diffs}")
    print("=" * 70)

if __name__ == "__main__":
    main()
