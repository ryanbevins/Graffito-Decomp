"""Analyze DRW1 + Batch 9 with correct u32-pair attrib parsing. V4."""
import struct
from pathlib import Path

def r8(d,o): return struct.unpack_from('>B',d,o)[0]
def r16(d,o): return struct.unpack_from('>H',d,o)[0]
def r32(d,o): return struct.unpack_from('>I',d,o)[0]

def find_section(data, tag):
    off = 0x20
    while off < len(data):
        if data[off:off+4] == tag:
            return off
        off += r32(data, off+4)
    return None

def parse_drw1(data):
    off = find_section(data, b'DRW1')
    if off is None: return []
    count = r16(data, off + 8)
    wo = r32(data, off + 12) + off
    do = r32(data, off + 16) + off
    return [(r8(data, wo+i), r16(data, do+i*2)) for i in range(count)]

def parse_shp1(data):
    off = find_section(data, b'SHP1')
    if off is None: return None
    return {
        'off': off,
        'batch_count': r16(data, off+8),
        'batches': r32(data, off+0x0C) + off,
        'attribs': r32(data, off+0x18) + off,
        'mtx_table': r32(data, off+0x1C) + off,
        'prim_data': r32(data, off+0x20) + off,
        'mtx_init': r32(data, off+0x24) + off,
        'draw_init': r32(data, off+0x28) + off,
    }

GX_ATTR_NAMES = {
    0: 'PNMTXIDX', 9: 'POS', 10: 'NRM', 11: 'CLR0', 12: 'CLR1',
    13: 'TEX0', 14: 'TEX1', 15: 'TEX2', 16: 'TEX3', 17: 'TEX4',
    0xFF: 'NULL'
}
GX_TYPE_NAMES = {0: 'NONE', 1: 'DIRECT', 2: 'INDEX8', 3: 'INDEX16'}

def parse_attribs(data, shp1, attrib_byte_offset):
    """Parse attrib list from SHP1. Each entry is (u32 attr, u32 type), 8 bytes."""
    base = shp1['attribs'] + attrib_byte_offset
    attribs = []
    off = base
    while True:
        attr = r32(data, off)
        dtype = r32(data, off + 4)
        off += 8
        if attr == 0xFF or attr == 0xFFFFFFFF:
            break
        attribs.append((attr, dtype))
    return attribs

def parse_batch(data, shp1, idx):
    b = shp1['batches'] + idx * 0x28
    mat_type = r8(data, b)
    num_pkts = r16(data, b + 2)
    attrib_off = r16(data, b + 4)  # byte offset into attrib table
    first_mtx = r16(data, b + 6)
    first_pkt = r16(data, b + 8)

    attribs = parse_attribs(data, shp1, attrib_off)

    packets = []
    for p in range(num_pkts):
        mi = shp1['mtx_init'] + (first_mtx + p) * 8
        mtx_count = r16(data, mi)
        mtx_first = r32(data, mi + 4)
        mtx_table = []
        for m in range(mtx_count):
            t_off = shp1['mtx_table'] + (mtx_first + m) * 2
            if t_off + 2 > len(data):
                break
            mtx_table.append(r16(data, t_off))

        di = shp1['draw_init'] + (first_pkt + p) * 8
        prim_size = r32(data, di)
        prim_off = r32(data, di + 4)
        packets.append({
            'mtx_table': mtx_table,
            'prim_start': shp1['prim_data'] + prim_off,
            'prim_size': prim_size,
        })
    return {'mat_type': mat_type, 'num_packets': num_pkts, 'attribs': attribs, 'packets': packets}

def vertex_stride(attribs):
    """Calculate bytes per vertex."""
    s = 0
    for attr, dtype in attribs:
        if attr == 0:  # PNMTXIDX always 1 byte
            s += 1
        elif dtype == 1:  # DIRECT
            if attr in (11, 12): s += 4  # color RGBA
            else: s += 1
        elif dtype == 2: s += 1  # INDEX8
        elif dtype == 3: s += 2  # INDEX16
    return s

def parse_vertices(data, packet, attribs):
    verts = []
    off = packet['prim_start']
    end = min(off + packet['prim_size'], len(data))
    stride = vertex_stride(attribs)

    while off < end:
        opcode = r8(data, off)
        off += 1
        if opcode == 0: continue
        if opcode < 0x80:
            # Unknown opcode - skip
            continue
        if off + 2 > end: break
        count = r16(data, off)
        off += 2
        # Sanity: count * stride should fit in remaining data
        needed = count * stride
        if off + needed > len(data) or count > 65000:
            break

        for v in range(count):
            vert = {}
            for attr, dtype in attribs:
                if attr == 0:
                    vert['mtxIdx'] = r8(data, off); off += 1
                elif dtype == 2:
                    vert[f'a{attr}'] = r8(data, off); off += 1
                elif dtype == 3:
                    vert[f'a{attr}'] = r16(data, off); off += 2
                elif dtype == 1:
                    if attr in (11, 12):
                        vert[f'a{attr}'] = r32(data, off); off += 4
                    else:
                        vert[f'a{attr}'] = r8(data, off); off += 1
            verts.append(vert)
    return verts

def analyze_batch_drw1(data, shp1, batch_idx, drw1):
    batch = parse_batch(data, shp1, batch_idx)
    all_verts = []
    drw1_usage = {}
    vn = 0
    for pi, pkt in enumerate(batch['packets']):
        if pkt['prim_size'] == 0: continue
        verts = parse_vertices(data, pkt, batch['attribs'])
        for v in verts:
            mi = v.get('mtxIdx', 0)
            slot = mi // 3
            di = pkt['mtx_table'][slot] if slot < len(pkt['mtx_table']) else -1
            if 0 <= di < len(drw1):
                iw, dv = drw1[di]
            else:
                iw, dv = -1, -1
            all_verts.append((vn, mi, slot, di, iw, dv, pi))
            drw1_usage[di] = drw1_usage.get(di, 0) + 1
            vn += 1
    return batch, all_verts, drw1_usage

def main():
    orig = Path(r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd").read_bytes()
    exp = Path(r"C:\Users\ryana\Downloads\reconstruct_test.bmd").read_bytes()

    # 1. DRW1
    print("=" * 80)
    print("1. DRW1 ENTRIES")
    print("=" * 80)
    od = parse_drw1(orig)
    ed = parse_drw1(exp)
    print(f"Count: orig={len(od)} exp={len(ed)}, Identical={od==ed}")
    print(f"{'Idx':>4} {'Type':>8} {'Data':>6}")
    for i,(w,v) in enumerate(od):
        print(f"{i:4d} {'W' if w else 'R':>8} {v:6d}")

    # 2. Batch overview + batch 9
    print("\n" + "=" * 80)
    print("2. BATCH 9 ANALYSIS")
    print("=" * 80)
    os = parse_shp1(orig)
    es = parse_shp1(exp)

    for i in range(os['batch_count']):
        ob = parse_batch(orig, os, i)
        eb = parse_batch(exp, es, i) if i < es['batch_count'] else None
        a_str = ','.join(GX_ATTR_NAMES.get(a, f'?{a}') + ':' + GX_TYPE_NAMES.get(t, f'?{t}') for a,t in ob['attribs'])
        m = " <--" if i == 9 else ""
        ep = eb['num_packets'] if eb else '?'
        print(f"  Batch {i:2d}: mt={ob['mat_type']} pkts={ob['num_packets']}/{ep} attribs=[{a_str}]{m}")

    ob9, ov, ou = analyze_batch_drw1(orig, os, 9, od)
    eb9, ev, eu = analyze_batch_drw1(exp, es, 9, ed)

    print(f"\nBatch 9 orig: {ob9['num_packets']} pkts, {len(ov)} verts")
    print(f"Batch 9 exp:  {eb9['num_packets']} pkts, {len(ev)} verts")

    # Attribs
    for label, b in [("orig", ob9), ("exp", eb9)]:
        a_str = ', '.join(f"{GX_ATTR_NAMES.get(a,'?')}:{GX_TYPE_NAMES.get(t,'?')}" for a,t in b['attribs'])
        print(f"  {label} attribs: [{a_str}]")

    # Per-packet
    print("\nPer-packet detail:")
    for pi in range(ob9['num_packets']):
        pkt = ob9['packets'][pi]
        vc = sum(1 for v in ov if v[6] == pi)
        # Only show first 10 mtx_table entries
        mt = pkt['mtx_table']
        mt_s = str(mt[:15]) + ('...' if len(mt) > 15 else '')
        print(f"  Orig pkt {pi:2d}: {vc:5d} verts, mtx[{len(mt)}]={mt_s}")
    for pi in range(eb9['num_packets']):
        pkt = eb9['packets'][pi]
        vc = sum(1 for v in ev if v[6] == pi)
        mt = pkt['mtx_table']
        print(f"  Exp  pkt {pi:2d}: {vc:5d} verts, mtx[{len(mt)}]={mt}")

    # 3. DRW1 usage comparison
    print("\n" + "=" * 80)
    print("3. DRW1 USAGE DISTRIBUTION (Batch 9)")
    print("=" * 80)
    ok, ek = set(ou), set(eu)
    only_o = ok - ek
    only_e = ek - ok
    shared = ok & ek

    if only_o:
        print(f"\nOriginal-only DRW1 indices: {sorted(only_o)}")
        for idx in sorted(only_o):
            w,v = od[idx] if 0<=idx<len(od) else (-1,-1)
            print(f"  [{idx}] {ou[idx]} verts, {'W' if w else 'R'}, evp_data={v}")
    else:
        print("\nNo original-only DRW1 indices.")
    if only_e:
        print(f"\nExport-only DRW1 indices: {sorted(only_e)}")
        for idx in sorted(only_e):
            w,v = ed[idx] if 0<=idx<len(ed) else (-1,-1)
            print(f"  [{idx}] {eu[idx]} verts, {'W' if w else 'R'}, evp_data={v}")
    else:
        print("\nNo export-only DRW1 indices.")

    print(f"\nShared ({len(shared)}):")
    for idx in sorted(shared):
        w,v = od[idx] if 0<=idx<len(od) else (-1,-1)
        t = 'W' if w else 'R'
        m = 'MATCH' if ou[idx]==eu[idx] else f'DIFFER({ou[idx]} vs {eu[idx]})'
        print(f"  [{idx}] {t} evp={v}: orig={ou[idx]} exp={eu[idx]} {m}")

    # 4. First 20 vertices
    print("\n" + "=" * 80)
    print("4. FIRST 20 VERTICES OF BATCH 9")
    print("=" * 80)
    for label, verts in [("ORIGINAL", ov), ("EXPORT", ev)]:
        print(f"\n--- {label} ---")
        print(f"{'V#':>4} {'MtxI':>5} {'Slot':>5} {'DRW1':>5} {'W?':>3} {'Data':>5} {'Pkt':>4}")
        for i in range(min(20, len(verts))):
            vn,mi,sl,di,iw,dv,pk = verts[i]
            print(f"{vn:4d} {mi:5d} {sl:5d} {di:5d} {'W' if iw==1 else 'R' if iw==0 else '?':>3} {dv:5d} {pk:4d}")

    # 5. Mismatch check
    print("\n" + "=" * 80)
    print("5. RIGID vs WEIGHTED MISMATCH CHECK")
    print("=" * 80)
    n = min(len(ov), len(ev))
    dd = 0
    td = 0
    for i in range(n):
        _,_,_,o_di,o_w,_,_ = ov[i]
        _,_,_,e_di,e_w,_,_ = ev[i]
        if o_di != e_di:
            dd += 1
            if o_w != e_w:
                td += 1
                if td <= 20:
                    o_t = 'W' if o_w else 'R'
                    e_t = 'W' if e_w else 'R'
                    print(f"  V{i}: orig DRW1[{o_di}]({o_t}) vs exp DRW1[{e_di}]({e_t})")
    print(f"\nCompared {n} verts, DRW1 index diffs: {dd}, type flips: {td}")
    if len(ov) != len(ev):
        print(f"VERTEX COUNT: orig={len(ov)} exp={len(ev)}")

if __name__ == '__main__':
    main()
