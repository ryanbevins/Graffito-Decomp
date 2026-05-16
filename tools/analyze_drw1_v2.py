"""Analyze DRW1 matrix assignments and batch 9 vertex data in two BMD files. V2."""
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
    if off is None: return []
    count = read_u16(data, off + 8)
    is_weighted_off = read_u32(data, off + 12) + off
    data_off = read_u32(data, off + 16) + off
    return [(read_u8(data, is_weighted_off + i), read_u16(data, data_off + i * 2)) for i in range(count)]

def parse_shp1_info(data):
    off, size = find_section(data, b'SHP1')
    if off is None: return None
    return {
        'off': off, 'size': size,
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
    mat_type = read_u8(data, b_off)
    num_packets = read_u16(data, b_off + 2)
    attrib_offset = read_u16(data, b_off + 4)
    first_mtx_data = read_u16(data, b_off + 6)
    first_packet_loc = read_u16(data, b_off + 8)

    attribs = []
    a_off = shp1['attribs_off'] + attrib_offset * 4
    while True:
        attr = read_u16(data, a_off)
        dtype = read_u16(data, a_off + 2)
        if attr == 0xFFFF: break
        attribs.append((attr, dtype))
        a_off += 4

    packets = []
    for pkt_idx in range(num_packets):
        mi_off = shp1['mtx_init_off'] + (first_mtx_data + pkt_idx) * 8
        if mi_off + 8 > len(data):
            packets.append({'mtx_table': [], 'prim_start': 0, 'prim_size': 0})
            continue
        mtx_count = read_u16(data, mi_off)
        mtx_first = read_u32(data, mi_off + 4)
        mtx_table = []
        for m in range(mtx_count):
            t_off = shp1['mtx_table_off'] + (mtx_first + m) * 2
            mtx_table.append(read_u16(data, t_off) if t_off + 2 <= len(data) else 0xFFFF)

        di_off = shp1['draw_init_off'] + (first_packet_loc + pkt_idx) * 8
        if di_off + 8 > len(data):
            packets.append({'mtx_table': mtx_table, 'prim_start': 0, 'prim_size': 0})
            continue
        prim_size = read_u32(data, di_off)
        prim_offset = read_u32(data, di_off + 4)
        packets.append({
            'mtx_table': mtx_table,
            'prim_start': shp1['prim_data_off'] + prim_offset,
            'prim_size': prim_size,
        })
    return {'mat_type': mat_type, 'num_packets': num_packets, 'attribs': attribs, 'packets': packets}

def parse_vertices(data, packet, attribs):
    vertices = []
    off = packet['prim_start']
    end = min(off + packet['prim_size'], len(data))

    while off < end:
        if off >= len(data): break
        opcode = read_u8(data, off)
        off += 1
        if opcode == 0: continue
        if opcode < 0x80: continue
        if off + 2 > end: break
        vert_count = read_u16(data, off)
        off += 2
        if vert_count > 10000:  # sanity check
            print(f"    WARNING: huge vert_count={vert_count}, opcode=0x{opcode:02X}, stopping packet parse")
            break

        for v in range(vert_count):
            vert = {}
            for attr, dtype in attribs:
                if off + 2 > len(data): break
                if attr == 0:
                    vert['matrixIndex'] = read_u8(data, off); off += 1
                elif dtype == 2:
                    vert[f'attr_{attr}'] = read_u8(data, off); off += 1
                elif dtype == 3:
                    vert[f'attr_{attr}'] = read_u16(data, off); off += 2
                elif dtype == 1:
                    if attr in (11, 12):
                        if off + 4 > len(data): break
                        vert[f'attr_{attr}'] = read_u32(data, off); off += 4
                    else:
                        vert[f'attr_{attr}'] = read_u8(data, off); off += 1
            vertices.append(vert)
    return vertices

def analyze_batch(data, shp1, batch_idx, drw1):
    batch = parse_batch(data, shp1, batch_idx)
    all_verts = []
    drw1_usage = {}
    vert_num = 0
    for pkt_idx, pkt in enumerate(batch['packets']):
        if pkt['prim_size'] == 0: continue
        vertices = parse_vertices(data, pkt, batch['attribs'])
        for vert in vertices:
            mi = vert.get('matrixIndex', 0)
            slot = mi // 3
            drw1_idx = pkt['mtx_table'][slot] if slot < len(pkt['mtx_table']) else -1
            if 0 <= drw1_idx < len(drw1):
                is_w, dval = drw1[drw1_idx]
            else:
                is_w, dval = -1, -1
            all_verts.append((vert_num, mi, slot, drw1_idx, is_w, dval, pkt_idx))
            drw1_usage[drw1_idx] = drw1_usage.get(drw1_idx, 0) + 1
            vert_num += 1
    return batch, all_verts, drw1_usage

def main():
    orig_data = Path(r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd").read_bytes()
    export_data = Path(r"C:\Users\ryana\Downloads\reconstruct_test.bmd").read_bytes()

    # 1. DRW1
    print("=" * 80)
    print("1. DRW1 ENTRIES")
    print("=" * 80)
    orig_drw1 = parse_drw1(orig_data)
    export_drw1 = parse_drw1(export_data)
    print(f"Original: {len(orig_drw1)}, Export: {len(export_drw1)}")
    print(f"Identical: {orig_drw1 == export_drw1}")
    print(f"\n{'Idx':>4} {'Type':>8} {'Data':>6}")
    for i, (w, v) in enumerate(orig_drw1):
        print(f"{i:4d} {'WEIGHTED' if w else 'RIGID':>8} {v:6d}")

    # 2. SHP1 / Batch 9
    print("\n" + "=" * 80)
    print("2. BATCH 9 ANALYSIS")
    print("=" * 80)
    orig_shp1 = parse_shp1_info(orig_data)
    export_shp1 = parse_shp1_info(export_data)

    for i in range(orig_shp1['batch_count']):
        b_o = orig_shp1['batches_off'] + i * 0x28
        b_e = export_shp1['batches_off'] + i * 0x28
        mt_o, np_o = read_u8(orig_data, b_o), read_u16(orig_data, b_o+2)
        mt_e, np_e = read_u8(export_data, b_e), read_u16(export_data, b_e+2)
        m = " <-- TARGET" if i == 9 else ""
        print(f"  Batch {i:2d}: orig mt={mt_o} pkts={np_o} | exp mt={mt_e} pkts={np_e}{m}")

    orig_batch, orig_verts, orig_usage = analyze_batch(orig_data, orig_shp1, 9, orig_drw1)
    export_batch, export_verts, export_usage = analyze_batch(export_data, export_shp1, 9, export_drw1)

    print(f"\nBatch 9 orig: matType={orig_batch['mat_type']}, {orig_batch['num_packets']} pkts, {len(orig_verts)} verts")
    print(f"Batch 9 exp:  matType={export_batch['mat_type']}, {export_batch['num_packets']} pkts, {len(export_verts)} verts")
    print(f"Attribs orig:   {orig_batch['attribs']}")
    print(f"Attribs export: {export_batch['attribs']}")

    # Per-packet vertex counts
    print("\nPer-packet vertex counts:")
    for pkt_idx in range(orig_batch['num_packets']):
        ocount = sum(1 for v in orig_verts if v[6] == pkt_idx)
        print(f"  Orig pkt {pkt_idx}: {ocount} verts, mtx_table={orig_batch['packets'][pkt_idx]['mtx_table']}")
    for pkt_idx in range(export_batch['num_packets']):
        ecount = sum(1 for v in export_verts if v[6] == pkt_idx)
        print(f"  Exp  pkt {pkt_idx}: {ecount} verts, mtx_table={export_batch['packets'][pkt_idx]['mtx_table']}")

    # Packet matrix table comparison
    print("\nPacket Matrix Table Comparison:")
    for p in range(max(orig_batch['num_packets'], export_batch['num_packets'])):
        omt = orig_batch['packets'][p]['mtx_table'] if p < orig_batch['num_packets'] else ['N/A']
        emt = export_batch['packets'][p]['mtx_table'] if p < export_batch['num_packets'] else ['N/A']
        match = "MATCH" if omt == emt else "DIFFER"
        print(f"  Pkt {p:2d}: orig={omt}")
        if omt != emt:
            print(f"          exp ={emt}")
        print(f"          [{match}]")

    # 3. DRW1 usage
    print("\n" + "=" * 80)
    print("3. DRW1 USAGE DISTRIBUTION (Batch 9)")
    print("=" * 80)
    ok, ek = set(orig_usage), set(export_usage)
    only_orig = ok - ek
    only_exp = ek - ok
    shared = ok & ek

    if only_orig:
        print(f"\nIn ORIGINAL only: {sorted(only_orig)}")
        for idx in sorted(only_orig):
            w, v = orig_drw1[idx] if 0 <= idx < len(orig_drw1) else (-1, -1)
            print(f"  DRW1[{idx}]: {orig_usage[idx]} verts, {'WEIGHTED' if w else 'RIGID'}, data={v}")
    if only_exp:
        print(f"\nIn EXPORT only: {sorted(only_exp)}")
        for idx in sorted(only_exp):
            w, v = export_drw1[idx] if 0 <= idx < len(export_drw1) else (-1, -1)
            print(f"  DRW1[{idx}]: {export_usage[idx]} verts, {'WEIGHTED' if w else 'RIGID'}, data={v}")

    print(f"\nShared ({len(shared)}):")
    for idx in sorted(shared):
        w, v = orig_drw1[idx] if 0 <= idx < len(orig_drw1) else (-1, -1)
        print(f"  DRW1[{idx}]: orig={orig_usage[idx]} exp={export_usage[idx]} {'WEIGHTED' if w else 'RIGID'} data={v} {'MATCH' if orig_usage[idx]==export_usage[idx] else 'DIFFER'}")

    # 4. First 20 vertices
    print("\n" + "=" * 80)
    print("4. FIRST 20 VERTICES OF BATCH 9")
    print("=" * 80)
    for label, verts in [("ORIGINAL", orig_verts), ("EXPORT", export_verts)]:
        print(f"\n--- {label} ---")
        print(f"{'V#':>4} {'MtxI':>5} {'Slot':>5} {'DRW1':>5} {'Type':>8} {'Data':>5} {'Pkt':>4}")
        for i in range(min(20, len(verts))):
            vn, mi, sl, di, iw, dv, pk = verts[i]
            print(f"{vn:4d} {mi:5d} {sl:5d} {di:5d} {'WEIGHTED' if iw==1 else 'RIGID' if iw==0 else '???':>8} {dv:5d} {pk:4d}")

    # 5. Mismatch check
    print("\n" + "=" * 80)
    print("5. RIGID vs WEIGHTED MISMATCH CHECK (Batch 9)")
    print("=" * 80)
    n = min(len(orig_verts), len(export_verts))
    drw1_diff = 0
    type_diff = 0
    for i in range(n):
        _, _, _, od, ow, _, _ = orig_verts[i]
        _, _, _, ed, ew, _, _ = export_verts[i]
        if od != ed:
            drw1_diff += 1
            if ow != ew:
                type_diff += 1
                if type_diff <= 20:
                    ow_s = 'W' if ow == 1 else 'R'
                    ew_s = 'W' if ew == 1 else 'R'
                    _, ov = orig_drw1[od] if 0 <= od < len(orig_drw1) else (-1, -1)
                    _, ev = export_drw1[ed] if 0 <= ed < len(export_drw1) else (-1, -1)
                    print(f"  V{i}: orig DRW1[{od}]({ow_s},env={ov}) vs exp DRW1[{ed}]({ew_s},env={ev})")

    print(f"\nCompared: {n} verts")
    print(f"Different DRW1 index: {drw1_diff}")
    print(f"Rigid/Weighted type flip: {type_diff}")
    if len(orig_verts) != len(export_verts):
        print(f"VERTEX COUNT MISMATCH: orig={len(orig_verts)} exp={len(export_verts)}")

if __name__ == '__main__':
    main()
