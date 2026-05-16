"""Analyze DRW1 matrix assignments and batch 9 vertex data in two BMD files."""
import struct
from pathlib import Path

def read_u8(data, off): return struct.unpack_from('>B', data, off)[0]
def read_u16(data, off): return struct.unpack_from('>H', data, off)[0]
def read_u32(data, off): return struct.unpack_from('>I', data, off)[0]

def find_section(data, tag):
    off = 0x20
    while off < len(data):
        sec_tag = data[off:off+4]
        sec_size = read_u32(data, off+4)
        if sec_tag == tag:
            return off, sec_size
        off += sec_size
    return None, None

def parse_drw1(data):
    off, size = find_section(data, b'DRW1')
    if off is None:
        return []
    count = read_u16(data, off + 8)
    is_weighted_off = read_u32(data, off + 12) + off
    data_off = read_u32(data, off + 16) + off
    entries = []
    for i in range(count):
        is_weighted = read_u8(data, is_weighted_off + i)
        val = read_u16(data, data_off + i * 2)
        entries.append((is_weighted, val))
    return entries

def parse_shp1_info(data):
    off, size = find_section(data, b'SHP1')
    if off is None:
        return None
    return {
        'off': off,
        'size': size,
        'batch_count': read_u16(data, off + 8),
        'batches_off': read_u32(data, off + 0x0C) + off,
        'attribs_off': read_u32(data, off + 0x18) + off,
        'mtx_table_off': read_u32(data, off + 0x1C) + off,
        'prim_data_off': read_u32(data, off + 0x20) + off,
        'mtx_init_off': read_u32(data, off + 0x24) + off,
        'draw_init_off': read_u32(data, off + 0x28) + off,
    }

def parse_batch(data, shp1, batch_idx):
    b_off = shp1['batches_off'] + batch_idx * 0x28
    mat_type = read_u8(data, b_off + 0x00)
    num_packets = read_u16(data, b_off + 0x02)
    attrib_offset = read_u16(data, b_off + 0x04)
    first_mtx_data = read_u16(data, b_off + 0x06)
    first_packet_loc = read_u16(data, b_off + 0x08)

    # Parse attribs
    attribs = []
    a_off = shp1['attribs_off'] + attrib_offset * 4
    while True:
        attr = read_u16(data, a_off)
        dtype = read_u16(data, a_off + 2)
        if attr == 0xFFFF:
            break
        attribs.append((attr, dtype))
        a_off += 4

    # Parse packets
    packets = []
    for pkt_idx in range(num_packets):
        # Matrix init: each entry is 8 bytes: u16 count, u16 pad, u32 firstIndex
        mi_off = shp1['mtx_init_off'] + (first_mtx_data + pkt_idx) * 8
        if mi_off + 8 > len(data):
            print(f"  WARNING: mtx_init out of bounds at packet {pkt_idx}, batch {batch_idx}")
            packets.append({'mtx_table': [], 'prim_start': 0, 'prim_size': 0})
            continue
        mtx_count = read_u16(data, mi_off)
        mtx_first = read_u32(data, mi_off + 4)

        mtx_table = []
        for m in range(mtx_count):
            t_off = shp1['mtx_table_off'] + (mtx_first + m) * 2
            if t_off + 2 > len(data):
                mtx_table.append(0xFFFF)
            else:
                mtx_table.append(read_u16(data, t_off))

        # Draw init: each entry is 8 bytes: u32 size, u32 offset
        di_off = shp1['draw_init_off'] + (first_packet_loc + pkt_idx) * 8
        if di_off + 8 > len(data):
            print(f"  WARNING: draw_init out of bounds at packet {pkt_idx}, batch {batch_idx}")
            packets.append({'mtx_table': mtx_table, 'prim_start': 0, 'prim_size': 0})
            continue
        prim_size = read_u32(data, di_off)
        prim_offset = read_u32(data, di_off + 4)

        packets.append({
            'mtx_table': mtx_table,
            'prim_start': shp1['prim_data_off'] + prim_offset,
            'prim_size': prim_size,
        })

    return {
        'mat_type': mat_type,
        'num_packets': num_packets,
        'attribs': attribs,
        'packets': packets,
    }

def parse_vertices(data, packet, attribs):
    """Parse all vertices from a packet's primitive data."""
    vertices = []
    off = packet['prim_start']
    end = min(off + packet['prim_size'], len(data))

    while off < end:
        opcode = read_u8(data, off)
        off += 1
        if opcode == 0:
            continue
        if opcode < 0x80:
            continue
        if off + 2 > end:
            break

        vert_count = read_u16(data, off)
        off += 2

        for v in range(vert_count):
            vert = {}
            for attr, dtype in attribs:
                if off >= len(data):
                    break
                if attr == 0:  # PNMTXIDX
                    vert['matrixIndex'] = read_u8(data, off)
                    off += 1
                elif dtype == 2:  # INDEX8
                    vert[f'attr_{attr}'] = read_u8(data, off)
                    off += 1
                elif dtype == 3:  # INDEX16
                    if off + 2 > len(data): break
                    vert[f'attr_{attr}'] = read_u16(data, off)
                    off += 2
                elif dtype == 1:  # DIRECT
                    if attr in (11, 12):
                        if off + 4 > len(data): break
                        vert[f'attr_{attr}'] = read_u32(data, off)
                        off += 4
                    else:
                        vert[f'attr_{attr}'] = read_u8(data, off)
                        off += 1
            vertices.append(vert)

    return vertices

def analyze_batch(data, shp1, batch_idx, drw1):
    batch = parse_batch(data, shp1, batch_idx)
    all_verts = []
    drw1_usage = {}
    vert_num = 0

    for pkt_idx, pkt in enumerate(batch['packets']):
        mtx_table = pkt['mtx_table']
        if pkt['prim_size'] == 0:
            continue
        vertices = parse_vertices(data, pkt, batch['attribs'])

        for vert in vertices:
            mi = vert.get('matrixIndex', 0)
            slot = mi // 3

            drw1_idx = mtx_table[slot] if slot < len(mtx_table) else -1

            if 0 <= drw1_idx < len(drw1):
                is_w, dval = drw1[drw1_idx]
            else:
                is_w, dval = -1, -1

            all_verts.append((vert_num, mi, slot, drw1_idx, is_w, dval))
            drw1_usage[drw1_idx] = drw1_usage.get(drw1_idx, 0) + 1
            vert_num += 1

    return batch, all_verts, drw1_usage

def main():
    orig_path = Path(r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd")
    export_path = Path(r"C:\Users\ryana\Downloads\reconstruct_test.bmd")

    orig_data = orig_path.read_bytes()
    export_data = export_path.read_bytes()

    # 1. DRW1
    print("=" * 80)
    print("1. DRW1 ENTRIES")
    print("=" * 80)

    orig_drw1 = parse_drw1(orig_data)
    export_drw1 = parse_drw1(export_data)

    print(f"Original: {len(orig_drw1)} entries, Export: {len(export_drw1)} entries")
    if orig_drw1 == export_drw1:
        print("DRW1 entries are IDENTICAL.")
    else:
        print("DRW1 entries DIFFER!")
        for i in range(max(len(orig_drw1), len(export_drw1))):
            o = orig_drw1[i] if i < len(orig_drw1) else None
            e = export_drw1[i] if i < len(export_drw1) else None
            if o != e:
                print(f"  [{i}] orig={o} export={e}")

    print(f"\n{'Idx':>4} {'isWeighted':>10} {'Data':>6}")
    print("-" * 24)
    for i, (is_w, val) in enumerate(orig_drw1):
        w_str = "WEIGHTED" if is_w else "RIGID"
        print(f"{i:4d} {w_str:>10} {val:6d}")

    # 2. SHP1 batch overview
    print("\n" + "=" * 80)
    print("2. BATCH 9 ANALYSIS")
    print("=" * 80)

    orig_shp1 = parse_shp1_info(orig_data)
    export_shp1 = parse_shp1_info(export_data)

    print(f"Original: {orig_shp1['batch_count']} batches, Export: {export_shp1['batch_count']} batches")

    # Quick batch overview - only parse batch 9 and neighbors
    for i in range(min(orig_shp1['batch_count'], export_shp1['batch_count'])):
        b_off_o = orig_shp1['batches_off'] + i * 0x28
        b_off_e = export_shp1['batches_off'] + i * 0x28
        mt_o = read_u8(orig_data, b_off_o)
        mt_e = read_u8(export_data, b_off_e)
        np_o = read_u16(orig_data, b_off_o + 2)
        np_e = read_u16(export_data, b_off_e + 2)
        marker = " <-- TARGET" if i == 9 else ""
        print(f"  Batch {i:2d}: orig matType={mt_o} pkts={np_o} | export matType={mt_e} pkts={np_e}{marker}")

    # Analyze batch 9
    print("\n--- Batch 9 Detail ---")
    orig_batch, orig_verts, orig_usage = analyze_batch(orig_data, orig_shp1, 9, orig_drw1)
    export_batch, export_verts, export_usage = analyze_batch(export_data, export_shp1, 9, export_drw1)

    print(f"Original: matType={orig_batch['mat_type']}, {orig_batch['num_packets']} packets, {len(orig_verts)} verts")
    print(f"Export:   matType={export_batch['mat_type']}, {export_batch['num_packets']} packets, {len(export_verts)} verts")
    print(f"Attribs orig:   {orig_batch['attribs']}")
    print(f"Attribs export: {export_batch['attribs']}")

    # Packet matrix tables
    print("\nPacket Matrix Tables:")
    max_pkts = max(orig_batch['num_packets'], export_batch['num_packets'])
    for p in range(max_pkts):
        omt = orig_batch['packets'][p]['mtx_table'] if p < orig_batch['num_packets'] else []
        emt = export_batch['packets'][p]['mtx_table'] if p < export_batch['num_packets'] else []
        match = "MATCH" if omt == emt else "DIFFER"
        print(f"  Pkt {p}: orig={omt}")
        if omt != emt:
            print(f"         exp ={emt}")
        print(f"         [{match}]")

    # 3. DRW1 usage comparison
    print("\n" + "=" * 80)
    print("3. DRW1 USAGE DISTRIBUTION (Batch 9)")
    print("=" * 80)

    orig_keys = set(orig_usage.keys())
    export_keys = set(export_usage.keys())
    only_orig = orig_keys - export_keys
    only_export = export_keys - orig_keys
    shared = orig_keys & export_keys

    if only_orig:
        print(f"\nDRW1 indices in ORIGINAL only: {sorted(only_orig)}")
        for idx in sorted(only_orig):
            is_w, val = orig_drw1[idx] if 0 <= idx < len(orig_drw1) else (-1, -1)
            w = "WEIGHTED" if is_w else "RIGID"
            print(f"  DRW1[{idx}]: {orig_usage[idx]} verts, {w}, data={val}")

    if only_export:
        print(f"\nDRW1 indices in EXPORT only: {sorted(only_export)}")
        for idx in sorted(only_export):
            is_w, val = export_drw1[idx] if 0 <= idx < len(export_drw1) else (-1, -1)
            w = "WEIGHTED" if is_w else "RIGID"
            print(f"  DRW1[{idx}]: {export_usage[idx]} verts, {w}, data={val}")

    if not only_orig:
        print("\nNo DRW1 indices unique to original.")
    if not only_export:
        print("No DRW1 indices unique to export.")

    print(f"\nShared DRW1 indices ({len(shared)}):")
    print(f"{'DRW1':>5} {'Orig#':>7} {'Exp#':>7} {'Type':>8} {'Data':>6} {'CountMatch':>10}")
    for idx in sorted(shared):
        is_w, val = orig_drw1[idx] if 0 <= idx < len(orig_drw1) else (-1, -1)
        w = "WEIGHTED" if is_w else "RIGID"
        oc, ec = orig_usage[idx], export_usage[idx]
        print(f"{idx:5d} {oc:7d} {ec:7d} {w:>8} {val:6d} {'YES' if oc == ec else 'NO':>10}")

    # 4. First 20 vertices
    print("\n" + "=" * 80)
    print("4. FIRST 20 VERTICES OF BATCH 9")
    print("=" * 80)

    for label, verts in [("ORIGINAL", orig_verts), ("EXPORT", export_verts)]:
        print(f"\n--- {label} ---")
        print(f"{'Vert':>5} {'MtxIdx':>7} {'Slot':>5} {'DRW1':>5} {'Type':>8} {'Data':>6}")
        print("-" * 42)
        for i in range(min(20, len(verts))):
            vn, mi, slot, drw1_idx, is_w, dval = verts[i]
            w = "WEIGHTED" if is_w == 1 else ("RIGID" if is_w == 0 else "???")
            print(f"{vn:5d} {mi:7d} {slot:5d} {drw1_idx:5d} {w:>8} {dval:6d}")

    # 5. Rigid vs Weighted mismatch
    print("\n" + "=" * 80)
    print("5. RIGID vs WEIGHTED MISMATCH CHECK (Batch 9)")
    print("=" * 80)

    n = min(len(orig_verts), len(export_verts))
    type_mismatches = 0
    drw1_mismatches = 0
    for i in range(n):
        _, _, _, o_d, o_w, o_v = orig_verts[i]
        _, _, _, e_d, e_w, e_v = export_verts[i]
        if o_d != e_d:
            drw1_mismatches += 1
            if o_w != e_w:
                type_mismatches += 1
                if type_mismatches <= 30:
                    ow = "WEIGHTED" if o_w else "RIGID"
                    ew = "WEIGHTED" if e_w else "RIGID"
                    print(f"  Vert {i}: orig DRW1[{o_d}] {ow} data={o_v} | export DRW1[{e_d}] {ew} data={e_v}")

    print(f"\nTotal vertices compared: {n}")
    print(f"Vertices with different DRW1 index: {drw1_mismatches}")
    print(f"Vertices with RIGID/WEIGHTED type mismatch: {type_mismatches}")

    if drw1_mismatches == 0 and type_mismatches == 0:
        print("All vertex DRW1 assignments match between original and export.")

    # Extra: if vertex counts differ
    if len(orig_verts) != len(export_verts):
        print(f"\nWARNING: Vertex count mismatch! Original={len(orig_verts)}, Export={len(export_verts)}")

if __name__ == '__main__':
    main()
