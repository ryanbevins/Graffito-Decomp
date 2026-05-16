#!/usr/bin/env python3
"""Check what transform the exporter applies to weighted batch vertices.

The export weighted positions don't match original rigid OR weighted positions.
They must be some incorrect transform of the Blender world-space positions.

Hypothesis: the weighted batch vertices in Blender are being transformed by
a bone inverse when they shouldn't be (mat_bone_map classifies them as rigid
instead of weighted).

Let me check by:
1. Reading the original weighted positions (bind-pose space)
2. Reading EVP1 inverse bind matrices
3. Checking if export_weighted_pos ≈ inv_bind_mat @ orig_weighted_pos
"""
import struct
import math

def r8(d, o):  return struct.unpack_from('>B', d, o)[0]
def r16(d, o): return struct.unpack_from('>H', d, o)[0]
def rs16(d, o): return struct.unpack_from('>h', d, o)[0]
def r32(d, o): return struct.unpack_from('>I', d, o)[0]
def rf32(d, o): return struct.unpack_from('>f', d, o)[0]

def find_section(data, tag):
    off = 0x20
    while off < len(data) - 8:
        if data[off:off+4] == tag:
            return off, r32(data, off+4)
        off += r32(data, off+4)
    return None, None

def parse_positions(data, vtx1_off):
    fmt_off = r32(data, vtx1_off + 8)
    offsets = [r32(data, vtx1_off + 12 + i*4) for i in range(13)]
    fmt_abs = vtx1_off + fmt_off
    fi = 0
    for i in range(13):
        if offsets[i] != 0:
            at = r32(data, fmt_abs + fi*16)
            dt = r32(data, fmt_abs + fi*16 + 8)
            dp = r8(data, fmt_abs + fi*16 + 12)
            if at == 9:
                start = offsets[i]
                end = None
                for j in range(i+1, 13):
                    if offsets[j]: end = offsets[j]; break
                if end is None: end = r32(data, vtx1_off + 4)
                length = end - start
                a = vtx1_off + start
                pos = []
                if dt == 4:
                    for k in range(length // 12):
                        o = a + k*12
                        pos.append((rf32(data, o), rf32(data, o+4), rf32(data, o+8)))
                elif dt == 3:
                    sc = 1.0 / (2**dp)
                    for k in range(length // 6):
                        o = a + k*6
                        pos.append((rs16(data,o)*sc, rs16(data,o+2)*sc, rs16(data,o+4)*sc))
                return pos
            fi += 1
    return []

def parse_shp1(data, shp1_off):
    bc = r16(data, shp1_off + 8)
    off_b = r32(data, shp1_off + 0x0C)
    off_a = r32(data, shp1_off + 0x18)
    off_d = r32(data, shp1_off + 0x20)
    off_p = r32(data, shp1_off + 0x28)
    batches = []
    for bi in range(bc):
        bo = shp1_off + off_b + bi*40
        pc = r16(data, bo+2)
        ao = r16(data, bo+4)
        fpl = r16(data, bo+8)
        attribs = []
        aoff = shp1_off + off_a + ao
        while True:
            at = r32(data, aoff)
            if at == 0xFF: break
            attribs.append((at, r32(data, aoff+4)))
            aoff += 8
            if len(attribs) > 20: break
        packets = []
        for pi in range(pc):
            plo = shp1_off + off_p + (fpl+pi)*8
            packets.append((r32(data, plo), r32(data, plo+4)))
        batches.append({'idx': bi, 'attribs': attribs, 'packets': packets,
                        'has_mtxidx': any(a == 0 for a, d in attribs)})
    return batches, off_d

def collect_pos_indices(data, shp1_off, off_d, batch):
    attribs = batch['attribs']
    stride = 0; pos_off = -1
    for a, d in attribs:
        if a == 9: pos_off = stride
        stride += (2 if d == 3 else 1)
    if pos_off < 0: return set()
    indices = set()
    for ps, po in batch['packets']:
        s = shp1_off + off_d + po; e = s + ps; o = s
        while o < e:
            pt = r8(data, o)
            if pt == 0: o += 1; continue
            nv = r16(data, o+1); o += 3
            for _ in range(nv):
                if o + stride > e: break
                indices.add(r16(data, o + pos_off))
                o += stride
    return indices

def parse_evp1_matrices(data, evp1_off):
    """Parse EVP1 inverse bind matrices."""
    off_matrices = r32(data, evp1_off + 0x18)
    sec_size = r32(data, evp1_off + 4)
    mat_start = evp1_off + off_matrices
    mat_space = evp1_off + sec_size - mat_start
    num = mat_space // 48
    matrices = []
    for mi in range(num):
        m = []
        for r in range(3):
            row = []
            for c in range(4):
                row.append(rf32(data, mat_start + mi*48 + r*16 + c*4))
            m.append(tuple(row))
        matrices.append(m)
    return matrices

def parse_jnt1_matrices(data, jnt1_off):
    """Parse JNT1 to get bone world matrices.

    JNT1 stores per-bone transforms (scale, rotation, translation).
    But the actual world matrices are computed hierarchically.
    The EVP1 inverse bind matrices are more useful - invert them to get world matrices.
    """
    pass

def mat3x4_transform(m, p):
    """Apply 3x4 matrix to a point (x,y,z)."""
    x = m[0][0]*p[0] + m[0][1]*p[1] + m[0][2]*p[2] + m[0][3]
    y = m[1][0]*p[0] + m[1][1]*p[1] + m[1][2]*p[2] + m[1][3]
    z = m[2][0]*p[0] + m[2][1]*p[1] + m[2][2]*p[2] + m[2][3]
    return (x, y, z)

def invert_3x4(m):
    """Invert a 3x4 matrix (rotation part + translation)."""
    # Extract 3x3 rotation and translation
    r = [[m[i][j] for j in range(3)] for i in range(3)]
    t = [m[i][3] for i in range(3)]
    # Transpose rotation (assuming orthonormal)
    rt = [[r[j][i] for j in range(3)] for i in range(3)]
    # New translation = -R^T * t
    nt = [-(rt[i][0]*t[0] + rt[i][1]*t[1] + rt[i][2]*t[2]) for i in range(3)]
    return [
        (rt[0][0], rt[0][1], rt[0][2], nt[0]),
        (rt[1][0], rt[1][1], rt[1][2], nt[1]),
        (rt[2][0], rt[2][1], rt[2][2], nt[2]),
    ]

def dist(a, b):
    return math.sqrt(sum((ai-bi)**2 for ai, bi in zip(a, b)))

def main():
    orig_path = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"
    exp_path = r"C:\Users\ryana\Downloads\reconstruct_test.bmd"

    with open(orig_path, 'rb') as f:
        od = f.read()
    with open(exp_path, 'rb') as f:
        ed = f.read()

    o_vtx1, _ = find_section(od, b'VTX1')
    e_vtx1, _ = find_section(ed, b'VTX1')
    o_shp1, _ = find_section(od, b'SHP1')
    e_shp1, _ = find_section(ed, b'SHP1')
    evp1_off, _ = find_section(od, b'EVP1')

    o_pos = parse_positions(od, o_vtx1)
    e_pos = parse_positions(ed, e_vtx1)

    inv_bind = parse_evp1_matrices(od, evp1_off)
    # World matrices = invert(inverse_bind) for each bone
    world_mats = [invert_3x4(m) for m in inv_bind]

    o_batches, o_off_d = parse_shp1(od, o_shp1)
    e_batches, e_off_d = parse_shp1(ed, e_shp1)

    # Get original batch 9 positions (weighted, bind-pose space)
    o_b9_pis = sorted(collect_pos_indices(od, o_shp1, o_off_d, o_batches[9]))
    e_b9_pis = sorted(collect_pos_indices(ed, e_shp1, e_off_d, e_batches[9]))

    print(f"Batch 9: orig {len(o_b9_pis)} indices, exp {len(e_b9_pis)} indices")
    print(f"EVP1: {len(inv_bind)} inverse bind matrices")

    # For each original weighted position, check if applying any bone's
    # inverse bind matrix produces the export position
    print("\n--- Testing: export_pos ~= inv_bind[bone] @ orig_pos ? ---")
    print("(If true, exporter wrongly applied bone inverse to weighted verts)")

    # Build map of original weighted positions by sorted order
    # Export positions at same-ish indices should correspond
    # But indices are shifted (orig starts at 533, exp at 520)

    # Instead, match by order within the batch
    o_b9_sorted = sorted(o_b9_pis)
    e_b9_sorted = sorted(e_b9_pis)

    # Try matching the Nth position in each batch's index list
    # First check if same order within batch
    print(f"\nFirst 10 orig batch 9 positions:")
    for pi in o_b9_sorted[:10]:
        print(f"  [{pi}] ({o_pos[pi][0]:9.3f}, {o_pos[pi][1]:9.3f}, {o_pos[pi][2]:9.3f})")
    print(f"First 10 exp batch 9 positions:")
    for pi in e_b9_sorted[:10]:
        print(f"  [{pi}] ({e_pos[pi][0]:9.3f}, {e_pos[pi][1]:9.3f}, {e_pos[pi][2]:9.3f})")

    # For each MISMATCHED pair, try all 29 bones' inverse bind matrices
    print(f"\n--- For first 5 mismatched positions, try all bone transforms ---")
    for i in range(min(5, len(o_b9_sorted))):
        opi = o_b9_sorted[i]
        epi = e_b9_sorted[i]
        op = o_pos[opi]
        ep = e_pos[epi]

        if dist(op, ep) < 0.1:
            continue  # already matching

        print(f"\n  orig[{opi}]=({op[0]:9.3f},{op[1]:9.3f},{op[2]:9.3f})")
        print(f"  exp [{epi}]=({ep[0]:9.3f},{ep[1]:9.3f},{ep[2]:9.3f})")

        # Try: ep = inv_bind[bone] @ op ?  (exporter applied inv_bind when it shouldn't)
        best_bone = -1
        best_dist = 999
        for bi, ibm in enumerate(inv_bind):
            t = mat3x4_transform(ibm, op)
            d = dist(t, ep)
            if d < best_dist:
                best_dist = d
                best_bone = bi
        print(f"  Best match: inv_bind[{best_bone}] @ orig -> dist={best_dist:.3f}")
        if best_dist < 1.0:
            t = mat3x4_transform(inv_bind[best_bone], op)
            print(f"    result=({t[0]:9.3f},{t[1]:9.3f},{t[2]:9.3f})")

        # Try: ep = world_mat[bone] @ op ?  (exporter applied world_mat when it shouldn't)
        best_bone2 = -1
        best_dist2 = 999
        for bi, wm in enumerate(world_mats):
            t = mat3x4_transform(wm, op)
            d = dist(t, ep)
            if d < best_dist2:
                best_dist2 = d
                best_bone2 = bi
        print(f"  Best match: world_mat[{best_bone2}] @ orig -> dist={best_dist2:.3f}")
        if best_dist2 < 1.0:
            t = mat3x4_transform(world_mats[best_bone2], op)
            print(f"    result=({t[0]:9.3f},{t[1]:9.3f},{t[2]:9.3f})")

        # Try reverse: op = world_mat[bone] @ ep ? (export is bone-local, orig is world)
        best_bone3 = -1
        best_dist3 = 999
        for bi, wm in enumerate(world_mats):
            t = mat3x4_transform(wm, ep)
            d = dist(t, op)
            if d < best_dist3:
                best_dist3 = d
                best_bone3 = bi
        print(f"  Best match: world_mat[{best_bone3}] @ exp -> orig? dist={best_dist3:.3f}")

        # Try: op = inv_bind[bone] @ ep ?
        best_bone4 = -1
        best_dist4 = 999
        for bi, ibm in enumerate(inv_bind):
            t = mat3x4_transform(ibm, ep)
            d = dist(t, op)
            if d < best_dist4:
                best_dist4 = d
                best_bone4 = bi
        print(f"  Best match: inv_bind[{best_bone4}] @ exp -> orig? dist={best_dist4:.3f}")

    # Broader test: for ALL mismatched batch 9 positions, find which bone's inv_bind
    # transforms orig->exp
    print(f"\n\n--- Systematic: which bone transform explains each mismatch? ---")
    from collections import Counter
    bone_counter = Counter()
    good_matches = 0
    bad_matches = 0

    for i in range(len(o_b9_sorted)):
        opi = o_b9_sorted[i]
        epi = e_b9_sorted[i]
        op = o_pos[opi]
        ep = e_pos[epi]

        if dist(op, ep) < 0.5:
            good_matches += 1
            continue

        # Try inv_bind @ orig -> exp
        best = (-1, 999)
        for bi, ibm in enumerate(inv_bind):
            d = dist(mat3x4_transform(ibm, op), ep)
            if d < best[1]:
                best = (bi, d)

        if best[1] < 1.0:
            bone_counter[best[0]] += 1
        else:
            # Try world_mat @ orig -> exp
            for bi, wm in enumerate(world_mats):
                d = dist(mat3x4_transform(wm, op), ep)
                if d < best[1]:
                    best = (bi + 100, d)  # +100 to distinguish

            if best[1] < 1.0:
                bone_counter[best[0]] += 1
            else:
                bad_matches += 1

    print(f"  Already matching: {good_matches}")
    print(f"  No bone transform found: {bad_matches}")
    print(f"  Transform matches by bone:")
    for bone, count in bone_counter.most_common(20):
        if bone >= 100:
            print(f"    world_mat[{bone-100}]: {count} positions")
        else:
            print(f"    inv_bind[{bone}]: {count} positions")


if __name__ == '__main__':
    main()
