#!/usr/bin/env python3
"""Deeper analysis: compare position pools and identify the reordering/transform issue."""
import struct

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
            if at == 9:
                dt = r32(data, fmt_abs + fi*16 + 8)
                dp = r8(data, fmt_abs + fi*16 + 12)
                start = offsets[i]
                end = None
                for j in range(i+1, 13):
                    if offsets[j]:
                        end = offsets[j]
                        break
                if end is None:
                    end = r32(data, vtx1_off + 4)
                length = end - start
                abs_s = vtx1_off + start
                pos = []
                if dt == 4:
                    for k in range(length // 12):
                        o = abs_s + k*12
                        pos.append((rf32(data, o), rf32(data, o+4), rf32(data, o+8)))
                elif dt == 3:
                    sc = 1.0 / (2**dp)
                    for k in range(length // 6):
                        o = abs_s + k*6
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
        mt = r16(data, bo)
        pc = r16(data, bo+2)
        ao = r16(data, bo+4)
        fmd = r16(data, bo+6)
        fpl = r16(data, bo+8)

        attribs = []
        aoff = shp1_off + off_a + ao
        while True:
            at = r32(data, aoff)
            dt = r32(data, aoff+4)
            if at == 0xFF:
                break
            attribs.append((at, dt))
            aoff += 8
            if len(attribs) > 20:
                break

        packets = []
        for pi in range(pc):
            plo = shp1_off + off_p + (fpl+pi)*8
            packets.append((r32(data, plo), r32(data, plo+4)))

        has_mtxidx = any(a == 0 for a, d in attribs)
        batches.append({
            'idx': bi, 'mt': mt, 'pc': pc, 'attribs': attribs,
            'packets': packets, 'has_mtxidx': has_mtxidx,
        })
    return batches, off_d

def collect_pos_indices(data, shp1_off, off_d, batch):
    attribs = batch['attribs']
    stride = 0
    pos_off = -1
    for a, d in attribs:
        if a == 9:
            pos_off = stride
        stride += (2 if d == 3 else 1)
    if pos_off < 0:
        return set()

    indices = set()
    for ps, po in batch['packets']:
        s = shp1_off + off_d + po
        e = s + ps
        o = s
        while o < e:
            pt = r8(data, o)
            if pt == 0:
                o += 1
                continue
            nv = r16(data, o+1)
            o += 3
            for _ in range(nv):
                if o + stride > e:
                    break
                indices.add(r16(data, o + pos_off))
                o += stride
    return indices

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

    o_pos = parse_positions(od, o_vtx1)
    e_pos = parse_positions(ed, e_vtx1)
    print(f"Position pool: orig={len(o_pos)}, exp={len(e_pos)}")

    o_batches, o_off_d = parse_shp1(od, o_shp1)
    e_batches, e_off_d = parse_shp1(ed, e_shp1)

    # The real weighted/rigid distinction: has PNMTXIDX (attr 0) or not
    print("\nBatch classification (by PNMTXIDX attribute):")
    for b in o_batches:
        kind = "weighted" if b['has_mtxidx'] else "RIGID"
        pis = collect_pos_indices(od, o_shp1, o_off_d, b)
        print(f"  Orig batch {b['idx']:2d}: {kind:8s} mt=0x{b['mt']:04X} posIdx range=[{min(pis)},{max(pis)}] count={len(pis)}")
    print()
    for b in e_batches:
        kind = "weighted" if b['has_mtxidx'] else "RIGID"
        pis = collect_pos_indices(ed, e_shp1, e_off_d, b)
        print(f"  Exp  batch {b['idx']:2d}: {kind:8s} mt=0x{b['mt']:04X} posIdx range=[{min(pis)},{max(pis)}] count={len(pis)}")

    # The first 201 positions (batch 0-3, rigid) should match
    print("\n--- Position comparison for RIGID batches (0-3) ---")
    # Batches 0-3 share indices 0-200
    rigid_match = 0
    rigid_mismatch = 0
    for pi in range(201):
        if pi < len(o_pos) and pi < len(e_pos):
            ox, oy, oz = o_pos[pi]
            ex, ey, ez = e_pos[pi]
            if abs(ox-ex) < 0.01 and abs(oy-ey) < 0.01 and abs(oz-ez) < 0.01:
                rigid_match += 1
            else:
                rigid_mismatch += 1
                if rigid_mismatch <= 5:
                    print(f"  MISMATCH posIdx={pi}: orig=({ox:.3f},{oy:.3f},{oz:.3f}) exp=({ex:.3f},{ey:.3f},{ez:.3f})")
    print(f"  Rigid range [0,200]: {rigid_match} match, {rigid_mismatch} mismatch")

    # Now check: the POSITION VALUES from orig batches 4-10 vs export batches 4-10
    # The issue is that the export has DIFFERENT values because vertices were transformed wrong
    # Key question: are the export positions in WORLD space instead of BONE-LOCAL space?

    # Let's build value->index maps to understand the reordering
    print("\n--- Checking if exported positions are just REORDERED ---")
    o_val_set = set()
    for x, y, z in o_pos:
        o_val_set.add((round(x, 3), round(y, 3), round(z, 3)))
    e_val_set = set()
    for x, y, z in e_pos:
        e_val_set.add((round(x, 3), round(y, 3), round(z, 3)))

    print(f"  Unique position values: orig={len(o_val_set)}, exp={len(e_val_set)}")
    print(f"  In both: {len(o_val_set & e_val_set)}")
    print(f"  Orig only: {len(o_val_set - e_val_set)}")
    print(f"  Exp only: {len(e_val_set - o_val_set)}")

    # Are the exported ones just shifted/reordered?
    # Check: for each batch range in original, see if the VALUES exist somewhere in export
    print("\n--- Per-batch value matching ---")
    for b in o_batches:
        pis = collect_pos_indices(od, o_shp1, o_off_d, b)
        kind = "weighted" if b['has_mtxidx'] else "RIGID"
        vals = set()
        for pi in pis:
            x, y, z = o_pos[pi]
            vals.add((round(x, 3), round(y, 3), round(z, 3)))
        found_in_exp = len(vals & e_val_set)
        print(f"  Orig batch {b['idx']:2d} ({kind:8s}): {len(vals)} values, {found_in_exp} found in export pool ({found_in_exp*100//len(vals)}%)")

    # Same for export
    for b in e_batches:
        pis = collect_pos_indices(ed, e_shp1, e_off_d, b)
        kind = "weighted" if b['has_mtxidx'] else "RIGID"
        vals = set()
        for pi in pis:
            x, y, z = e_pos[pi]
            vals.add((round(x, 3), round(y, 3), round(z, 3)))
        found_in_orig = len(vals & o_val_set)
        print(f"  Exp  batch {b['idx']:2d} ({kind:8s}): {len(vals)} values, {found_in_orig} found in orig pool ({found_in_orig*100//len(vals)}%)")

    # Now the CRITICAL analysis: check what the export positions look like for rigid batches 4-8
    # These store positions in BONE-LOCAL space. The original has them bone-local.
    # Did the export accidentally apply the inverse transform, or leave them in world space?

    # Let's compare orig batch 4 range [201-288] with export batch 4 range
    print("\n--- Detailed: Orig batch 4 (RIGID) vs Export batch 4 ---")
    o_b4_pis = sorted(collect_pos_indices(od, o_shp1, o_off_d, o_batches[4]))
    e_b4_pis = sorted(collect_pos_indices(ed, e_shp1, e_off_d, e_batches[4]))

    print(f"  Orig batch 4 posIdx: {o_b4_pis[:10]}...{o_b4_pis[-3:]}")
    print(f"  Exp  batch 4 posIdx: {e_b4_pis[:10]}...{e_b4_pis[-3:]}")

    # Show first 10 values from each
    print("  Orig batch 4 first 10 positions:")
    for pi in o_b4_pis[:10]:
        x, y, z = o_pos[pi]
        print(f"    [{pi}] ({x:9.3f}, {y:9.3f}, {z:9.3f})")

    print("  Exp  batch 4 first 10 positions (at their indices):")
    for pi in e_b4_pis[:10]:
        x, y, z = e_pos[pi]
        print(f"    [{pi}] ({x:9.3f}, {y:9.3f}, {z:9.3f})")

    # Check: do the orig batch 4 VALUES appear in export, just at different indices?
    o_b4_vals = [(round(o_pos[pi][0],3), round(o_pos[pi][1],3), round(o_pos[pi][2],3)) for pi in o_b4_pis]
    e_b4_vals = [(round(e_pos[pi][0],3), round(e_pos[pi][1],3), round(e_pos[pi][2],3)) for pi in e_b4_pis]

    o_b4_set = set(o_b4_vals)
    e_b4_set = set(e_b4_vals)
    print(f"\n  Batch 4 value overlap: {len(o_b4_set & e_b4_set)} of orig {len(o_b4_set)}, exp {len(e_b4_set)}")

    # Check batch 9 (truly weighted, has PNMTXIDX)
    print("\n--- Detailed: batch 9 (WEIGHTED) ---")
    o_b9_pis = sorted(collect_pos_indices(od, o_shp1, o_off_d, o_batches[9]))
    e_b9_pis = sorted(collect_pos_indices(ed, e_shp1, e_off_d, e_batches[9]))

    o_b9_vals = set((round(o_pos[pi][0],3), round(o_pos[pi][1],3), round(o_pos[pi][2],3)) for pi in o_b9_pis)
    e_b9_vals = set((round(e_pos[pi][0],3), round(e_pos[pi][1],3), round(e_pos[pi][2],3)) for pi in e_b9_pis)

    print(f"  Orig batch 9: {len(o_b9_vals)} unique values")
    print(f"  Exp  batch 9: {len(e_b9_vals)} unique values")
    print(f"  Overlap: {len(o_b9_vals & e_b9_vals)}")
    print(f"  Orig only: {len(o_b9_vals - e_b9_vals)}")
    print(f"  Exp only:  {len(e_b9_vals - o_b9_vals)}")

    # Show some orig-only and exp-only to see the transform pattern
    print("\n  First 15 positions in ORIG batch 9 only:")
    for v in sorted(o_b9_vals - e_b9_vals)[:15]:
        print(f"    ({v[0]:9.3f}, {v[1]:9.3f}, {v[2]:9.3f})")
    print("  First 15 positions in EXP batch 9 only:")
    for v in sorted(e_b9_vals - o_b9_vals)[:15]:
        print(f"    ({v[0]:9.3f}, {v[1]:9.3f}, {v[2]:9.3f})")

    # The vert_materials "first wins" question:
    # In Vtx1.py FromBlenderMesh, vert_materials collects ALL material indices per vertex.
    # A vertex is left in world space if ANY of its materials is weighted (in_weighted check).
    # This means: shared vertices between rigid batch 4 and weighted batch 9 get left in world space.
    # But rigid batch 4 expects bone-local space!
    #
    # Let's check: how many vertices are shared between rigid and weighted batches?
    print("\n--- Vertex sharing between rigid/weighted batches ---")
    rigid_pis = set()
    weighted_pis = set()
    for b in o_batches:
        pis = collect_pos_indices(od, o_shp1, o_off_d, b)
        if b['has_mtxidx']:
            weighted_pis |= pis
        else:
            rigid_pis |= pis

    shared = rigid_pis & weighted_pis
    print(f"  Rigid-only indices: {len(rigid_pis - weighted_pis)}")
    print(f"  Weighted-only indices: {len(weighted_pis - rigid_pis)}")
    print(f"  SHARED between rigid & weighted: {len(shared)}")
    if shared:
        print(f"    Shared indices: {sorted(shared)[:30]}")

    # Same for export
    e_rigid_pis = set()
    e_weighted_pis = set()
    for b in e_batches:
        pis = collect_pos_indices(ed, e_shp1, e_off_d, b)
        if b['has_mtxidx']:
            e_weighted_pis |= pis
        else:
            e_rigid_pis |= pis

    e_shared = e_rigid_pis & e_weighted_pis
    print(f"\n  Export rigid-only: {len(e_rigid_pis - e_weighted_pis)}")
    print(f"  Export weighted-only: {len(e_weighted_pis - e_rigid_pis)}")
    print(f"  Export SHARED: {len(e_shared)}")
    if e_shared:
        print(f"    Shared indices: {sorted(e_shared)[:30]}")

    # KEY QUESTION: In the ORIGINAL, are there shared posIndices between rigid and weighted?
    # If yes, those positions must be stored in the coordinate space expected by BOTH.
    # But that's impossible if rigid=bone-local and weighted=world-space!
    # The answer: the original BMD uses DIFFERENT posIndex values for the same geometric vertex.
    # One copy in bone-local space (for rigid batch), one in world space (for weighted batch).
    # But during export, Blender only has ONE vertex with ONE position.
    # The exporter must DUPLICATE vertices that appear in both rigid and weighted batches.

    # Let's verify: in the original, check if shared posIndices point to same or different values
    if shared:
        print("\n  Values at shared posIndices (should be same for both batches!):")
        for pi in sorted(shared)[:10]:
            print(f"    posIdx={pi}: ({o_pos[pi][0]:.3f}, {o_pos[pi][1]:.3f}, {o_pos[pi][2]:.3f})")

    # Pool-level: check which positions in the orig pool DON'T exist in the export pool
    # This tells us what vertices got lost or transformed wrong
    print("\n--- Full pool value analysis ---")
    o_all = set()
    for x, y, z in o_pos:
        o_all.add((round(x, 2), round(y, 2), round(z, 2)))
    e_all = set()
    for x, y, z in e_pos:
        e_all.add((round(x, 2), round(y, 2), round(z, 2)))

    print(f"  Orig pool unique (2dp): {len(o_all)}")
    print(f"  Exp  pool unique (2dp): {len(e_all)}")
    print(f"  Missing from export: {len(o_all - e_all)}")
    print(f"  Extra in export: {len(e_all - o_all)}")


if __name__ == '__main__':
    main()
