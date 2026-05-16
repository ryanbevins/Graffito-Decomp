"""Compare triangle topology between original and exported BMD files."""
import struct

def read_u8(data, off): return struct.unpack_from('>B', data, off)[0]
def read_u16(data, off): return struct.unpack_from('>H', data, off)[0]
def read_u32(data, off): return struct.unpack_from('>I', data, off)[0]
def read_s16(data, off): return struct.unpack_from('>h', data, off)[0]
def read_f32(data, off): return struct.unpack_from('>f', data, off)[0]

# GX component types
GX_U8  = 0
GX_S8  = 1
GX_U16 = 2
GX_S16 = 3
GX_F32 = 4

def find_section(data, tag):
    off = 0x20
    while off < len(data):
        sec_tag = data[off:off+4]
        sec_size = read_u32(data, off+4)
        if sec_tag == tag:
            return off, sec_size
        off += sec_size
    return None, None

def parse_vtx1_positions(data):
    vtx1_off, vtx1_size = find_section(data, b'VTX1')
    if vtx1_off is None:
        raise ValueError("No VTX1 section found")

    fmt_off = vtx1_off + read_u32(data, vtx1_off + 0x08)

    pos_comp_cnt = None
    pos_comp_type = None
    pos_frac = None
    i = 0
    while True:
        attr = read_u32(data, fmt_off + i * 16)
        if attr == 0xFF:
            break
        if attr == 9:  # GX_VA_POS
            pos_comp_cnt = read_u32(data, fmt_off + i * 16 + 4)
            pos_comp_type = read_u32(data, fmt_off + i * 16 + 8)
            pos_frac = read_u8(data, fmt_off + i * 16 + 12)
            break
        i += 1

    pos_data_offset_rel = read_u32(data, vtx1_off + 0x0C)
    pos_data_off = vtx1_off + pos_data_offset_rel

    # Find end of position data
    offsets = []
    for j in range(13):
        o = read_u32(data, vtx1_off + 0x0C + j * 4)
        if o != 0:
            offsets.append(o)
    offsets.sort()
    idx = offsets.index(pos_data_offset_rel)
    end = vtx1_off + (offsets[idx + 1] if idx + 1 < len(offsets) else vtx1_size)

    positions = []
    off = pos_data_off

    if pos_comp_type == GX_S16:
        scale = 1.0 / (1 << pos_frac) if pos_frac else 1.0
        stride = 6  # 3 * s16
        while off + stride <= end:
            x = read_s16(data, off) * scale
            y = read_s16(data, off + 2) * scale
            z = read_s16(data, off + 4) * scale
            positions.append((x, y, z))
            off += stride
    elif pos_comp_type == GX_F32:
        stride = 12
        while off + stride <= end:
            x = read_f32(data, off)
            y = read_f32(data, off + 4)
            z = read_f32(data, off + 8)
            positions.append((x, y, z))
            off += stride
    elif pos_comp_type == GX_U16:
        scale = 1.0 / (1 << pos_frac) if pos_frac else 1.0
        stride = 6
        while off + stride <= end:
            x = read_u16(data, off) * scale
            y = read_u16(data, off + 2) * scale
            z = read_u16(data, off + 4) * scale
            positions.append((x, y, z))
            off += stride
    else:
        raise ValueError(f"Unsupported position compType={pos_comp_type}")

    print(f"  VTX1: {len(positions)} positions, compType={pos_comp_type} ({'s16' if pos_comp_type==3 else 'f32' if pos_comp_type==4 else '?'}), frac={pos_frac}")
    return positions

def parse_shp1(data, positions):
    shp1_off, shp1_size = find_section(data, b'SHP1')
    if shp1_off is None:
        raise ValueError("No SHP1 section found")

    batch_count = read_u16(data, shp1_off + 0x08)
    batch_off = shp1_off + read_u32(data, shp1_off + 0x0C)
    attrib_off = shp1_off + read_u32(data, shp1_off + 0x18)
    prim_data_off = shp1_off + read_u32(data, shp1_off + 0x20)
    draw_init_off = shp1_off + read_u32(data, shp1_off + 0x28)

    print(f"  SHP1: {batch_count} batches")

    all_batches = []

    for b in range(batch_count):
        be = batch_off + b * 0x28
        mat_type = read_u8(data, be + 0x00)
        attr_byte_start = read_u16(data, be + 0x04)
        first_pkt = read_u16(data, be + 0x08)

        # Parse attributes
        attrs = []
        a_off = attrib_off + attr_byte_start
        while True:
            a_type = read_u32(data, a_off)
            a_dt = read_u32(data, a_off + 4)
            if a_type == 0xFF:
                break
            attrs.append((a_type, a_dt))
            a_off += 8

        # Determine packet count
        if b + 1 < batch_count:
            next_first_pkt = read_u16(data, batch_off + (b + 1) * 0x28 + 0x08)
            pkt_count = next_first_pkt - first_pkt
        else:
            pkt_count = 100

        triangles = []

        for pkt in range(pkt_count):
            di_off = draw_init_off + (first_pkt + pkt) * 8
            if di_off + 8 > shp1_off + shp1_size:
                break
            prim_size = read_u32(data, di_off)
            prim_offset = read_u32(data, di_off + 4)

            if prim_size == 0:
                break

            prim_start = prim_data_off + prim_offset
            prim_end = prim_start + prim_size

            off = prim_start
            while off < prim_end:
                cmd = read_u8(data, off)
                off += 1
                if cmd == 0:
                    break
                if cmd not in (0x90, 0x98, 0xA0, 0xA8):
                    break

                vert_count = read_u16(data, off)
                off += 2
                if vert_count == 0:
                    break

                verts = []
                for v in range(vert_count):
                    pos_idx = -1
                    for ai, (a_type, a_dt) in enumerate(attrs):
                        if a_dt == 1:  # direct u8
                            val = read_u8(data, off)
                            off += 1
                        elif a_dt == 2:  # index8
                            val = read_u8(data, off)
                            off += 1
                        elif a_dt == 3:  # index16
                            val = read_u16(data, off)
                            off += 2
                        else:
                            continue
                        if a_type == 9:
                            pos_idx = val
                    verts.append(pos_idx)

                # Validate
                max_idx = max(verts) if verts else 0
                if max_idx >= len(positions):
                    # Skip this primitive
                    continue

                if cmd == 0x90:  # triangles
                    for i in range(0, len(verts) - 2, 3):
                        triangles.append((verts[i], verts[i+1], verts[i+2]))
                elif cmd == 0x98:  # tristrip
                    for i in range(len(verts) - 2):
                        if i % 2 == 0:
                            triangles.append((verts[i], verts[i+1], verts[i+2]))
                        else:
                            triangles.append((verts[i+1], verts[i], verts[i+2]))
                elif cmd == 0xA0:  # trifan
                    for i in range(1, len(verts) - 1):
                        triangles.append((verts[0], verts[i], verts[i+1]))

        all_batches.append({
            'batch_idx': b,
            'mat_type': mat_type,
            'attrs': attrs,
            'triangles': triangles,
        })

    return all_batches

def round_pos(pos, decimals=2):
    return tuple(round(v, decimals) for v in pos)

def tri_as_set(tri, positions):
    p0 = round_pos(positions[tri[0]])
    p1 = round_pos(positions[tri[1]])
    p2 = round_pos(positions[tri[2]])
    return frozenset([p0, p1, p2])

def compare_batch(orig_batch, exp_batch, orig_pos, exp_pos, batch_idx, verbose=True):
    orig_tris = orig_batch['triangles']
    exp_tris = exp_batch['triangles']

    print(f"\n  === Batch {batch_idx} (matType: orig={orig_batch['mat_type']}, exp={exp_batch['mat_type']}) ===")
    print(f"  Original: {len(orig_tris)} triangles, Export: {len(exp_tris)} triangles")

    orig_set = set()
    for t in orig_tris:
        try:
            orig_set.add(tri_as_set(t, orig_pos))
        except IndexError:
            pass
    exp_set = set()
    for t in exp_tris:
        try:
            exp_set.add(tri_as_set(t, exp_pos))
        except IndexError:
            pass

    shared = orig_set & exp_set
    only_orig = orig_set - exp_set
    only_exp = exp_set - orig_set

    match_pct = len(shared) / max(len(orig_set), 1) * 100
    print(f"  Unique triangles (by position): orig={len(orig_set)}, exp={len(exp_set)}")
    print(f"  Shared: {len(shared)} ({match_pct:.1f}%)")
    print(f"  Only in original: {len(only_orig)}")
    print(f"  Only in export: {len(only_exp)}")

    if verbose and only_orig:
        print(f"\n  First 5 triangles ONLY in original:")
        for i, t in enumerate(sorted(only_orig)[:5]):
            print(f"    {i}: {sorted(t)}")

    if verbose and only_exp:
        print(f"\n  First 5 triangles ONLY in export:")
        for i, t in enumerate(sorted(only_exp)[:5]):
            print(f"    {i}: {sorted(t)}")

    # Unique positions
    orig_posset = set()
    for t in orig_tris:
        for vi in t:
            if vi < len(orig_pos):
                orig_posset.add(round_pos(orig_pos[vi]))
    exp_posset = set()
    for t in exp_tris:
        for vi in t:
            if vi < len(exp_pos):
                exp_posset.add(round_pos(exp_pos[vi]))

    shared_pos = orig_posset & exp_posset
    print(f"\n  Unique positions referenced: orig={len(orig_posset)}, exp={len(exp_posset)}")
    print(f"  Shared positions: {len(shared_pos)}")
    print(f"  Only in orig: {len(orig_posset - exp_posset)}, Only in exp: {len(exp_posset - orig_posset)}")

    # By index
    orig_idx_set = set(frozenset(t) for t in orig_tris)
    exp_idx_set = set(frozenset(t) for t in exp_tris)
    shared_idx = orig_idx_set & exp_idx_set
    print(f"  By vertex INDEX: shared={len(shared_idx)}, only_orig={len(orig_idx_set - exp_idx_set)}, only_exp={len(exp_idx_set - orig_idx_set)}")

    return match_pct

def main():
    orig_path = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"
    exp_path = r"C:\Users\ryana\Downloads\reconstruct_test.bmd"

    print("=== Loading Original BMD ===")
    with open(orig_path, 'rb') as f:
        orig_data = f.read()
    orig_pos = parse_vtx1_positions(orig_data)
    orig_batches = parse_shp1(orig_data, orig_pos)

    print(f"\n=== Loading Exported BMD ===")
    with open(exp_path, 'rb') as f:
        exp_data = f.read()
    exp_pos = parse_vtx1_positions(exp_data)
    exp_batches = parse_shp1(exp_data, exp_pos)

    print(f"\n=== Batch Count: orig={len(orig_batches)}, exp={len(exp_batches)} ===")

    # Compare batch 9 (weighted body)
    print("\n" + "="*60)
    print("BATCH 9 (WEIGHTED BODY) COMPARISON")
    print("="*60)
    if 9 < len(orig_batches) and 9 < len(exp_batches):
        compare_batch(orig_batches[9], exp_batches[9], orig_pos, exp_pos, 9)

    # Compare batches 0-3 (rigid head)
    print("\n" + "="*60)
    print("BATCHES 0-3 (RIGID HEAD) COMPARISON")
    print("="*60)
    for b in range(min(4, len(orig_batches), len(exp_batches))):
        compare_batch(orig_batches[b], exp_batches[b], orig_pos, exp_pos, b)

    # Summary
    print("\n" + "="*60)
    print("ALL BATCHES SUMMARY")
    print("="*60)
    for b in range(min(len(orig_batches), len(exp_batches))):
        ob = orig_batches[b]
        eb = exp_batches[b]
        orig_set = set()
        for t in ob['triangles']:
            try: orig_set.add(tri_as_set(t, orig_pos))
            except IndexError: pass
        exp_set = set()
        for t in eb['triangles']:
            try: exp_set.add(tri_as_set(t, exp_pos))
            except IndexError: pass
        shared = orig_set & exp_set
        pct = len(shared) / max(len(orig_set), 1) * 100
        print(f"  Batch {b:2d}: matType=orig{ob['mat_type']}/exp{eb['mat_type']}, orig_tris={len(ob['triangles']):5d}, exp_tris={len(eb['triangles']):5d}, unique_orig={len(orig_set)}, unique_exp={len(exp_set)}, match={pct:.1f}%")

if __name__ == '__main__':
    main()
