#!/usr/bin/env python3
"""Final analysis: identify the root cause of wrong weighted batch positions.

Key findings from previous analysis:
- Original batch 9 has 11 packets with a MIX of rigid and weighted DRW entries
- Original batch 10 has 2 packets with a MIX of rigid and weighted DRW entries
- Rigid batches (0-8) are 100% correct in export
- Weighted batches (9-10) have ~86-94% WRONG positions

This script verifies: the exporter's vert_materials "first wins" logic and
deduplication are causing the wrong transform to be applied.
"""
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
            dt = r32(data, fmt_abs + fi*16 + 8)
            dp = r8(data, fmt_abs + fi*16 + 12)
            if at == 9:
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
        fpl = r16(data, bo+8)

        attribs = []
        aoff = shp1_off + off_a + ao
        while True:
            at = r32(data, aoff)
            if at == 0xFF: break
            dt = r32(data, aoff+4)
            attribs.append((at, dt))
            aoff += 8
            if len(attribs) > 20: break

        packets = []
        for pi in range(pc):
            plo = shp1_off + off_p + (fpl+pi)*8
            packets.append((r32(data, plo), r32(data, plo+4)))

        has_mtxidx = any(a == 0 for a, d in attribs)
        batches.append({
            'idx': bi, 'attribs': attribs, 'packets': packets,
            'has_mtxidx': has_mtxidx,
        })
    return batches, off_d

def collect_pos_indices(data, shp1_off, off_d, batch):
    attribs = batch['attribs']
    stride = 0
    pos_off = -1
    for a, d in attribs:
        if a == 9: pos_off = stride
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
            if pt == 0: o += 1; continue
            nv = r16(data, o+1); o += 3
            for _ in range(nv):
                if o + stride > e: break
                indices.add(r16(data, o + pos_off))
                o += stride
    return indices

def collect_per_vertex_mtx(data, shp1_off, off_d, batch):
    """Collect (posIndex, matrixIndex_raw) pairs from batch primitives."""
    attribs = batch['attribs']
    stride = 0
    pos_off = -1
    mtx_off = -1
    for a, d in attribs:
        if a == 0: mtx_off = stride  # PNMTXIDX
        if a == 9: pos_off = stride
        stride += (2 if d == 3 else 1)

    results = []
    for ps, po in batch['packets']:
        s = shp1_off + off_d + po
        e = s + ps
        o = s
        while o < e:
            pt = r8(data, o)
            if pt == 0: o += 1; continue
            nv = r16(data, o+1); o += 3
            for _ in range(nv):
                if o + stride > e: break
                pi = r16(data, o + pos_off)
                mi = r8(data, o + mtx_off) if mtx_off >= 0 else -1
                results.append((pi, mi))
                o += stride
    return results

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
    print(f"Position pools: orig={len(o_pos)}, exp={len(e_pos)} (diff={len(o_pos)-len(e_pos)})")

    o_batches, o_off_d = parse_shp1(od, o_shp1)
    e_batches, e_off_d = parse_shp1(ed, e_shp1)

    # KEY METRIC: position pool size difference = 816 - 800 = 16
    # This means the export is DEDUPLICATING 16 positions that were intentionally
    # duplicated in the original (same world-space geometry, different coordinate space).

    # Verify: original has NO shared posIndices between batches 0-8 (rigid) and 9-10 (weighted)
    o_rigid_idx = set()
    o_weighted_idx = set()
    for b in o_batches:
        pis = collect_pos_indices(od, o_shp1, o_off_d, b)
        if b['has_mtxidx']:
            o_weighted_idx |= pis
        else:
            o_rigid_idx |= pis

    print(f"\nOriginal: rigid indices={len(o_rigid_idx)}, weighted indices={len(o_weighted_idx)}, shared={len(o_rigid_idx & o_weighted_idx)}")

    # But check: do the VALUES overlap?
    o_rigid_vals = {}
    for pi in o_rigid_idx:
        v = (round(o_pos[pi][0], 3), round(o_pos[pi][1], 3), round(o_pos[pi][2], 3))
        if v not in o_rigid_vals:
            o_rigid_vals[v] = []
        o_rigid_vals[v].append(pi)

    o_weighted_vals = {}
    for pi in o_weighted_idx:
        v = (round(o_pos[pi][0], 3), round(o_pos[pi][1], 3), round(o_pos[pi][2], 3))
        if v not in o_weighted_vals:
            o_weighted_vals[v] = []
        o_weighted_vals[v].append(pi)

    value_overlap = set(o_rigid_vals.keys()) & set(o_weighted_vals.keys())
    print(f"Original: rigid unique values={len(o_rigid_vals)}, weighted unique values={len(o_weighted_vals)}")
    print(f"VALUE overlap between rigid and weighted: {len(value_overlap)}")
    if value_overlap:
        print("  These are positions with IDENTICAL coordinates in both rigid and weighted batches:")
        for v in sorted(value_overlap)[:15]:
            r_idx = o_rigid_vals[v]
            w_idx = o_weighted_vals[v]
            print(f"    ({v[0]:9.3f},{v[1]:9.3f},{v[2]:9.3f}) rigid_idx={r_idx} weighted_idx={w_idx}")

    # Now check the EXPORT: does deduplication merge these?
    print(f"\n\n{'='*70}")
    print("  ROOT CAUSE ANALYSIS: Position Deduplication + Coordinate Space Conflict")
    print(f"{'='*70}")

    # In Blender, each vertex has ONE position (world space after import).
    # The importer transforms:
    #   - Rigid batch vertices: world_pos = bone_matrix @ bone_local_pos
    #   - Weighted batch vertices: world_pos = blend_matrix @ bind_pos
    # After import, ALL vertices are in Blender world space.
    #
    # On export, the code must REVERSE this:
    #   - For rigid batch verts: store bone_inv @ world_pos (bone-local)
    #   - For weighted batch verts: store world_pos directly (bind-pose space)
    #
    # The mat_bone_map classifies materials 0,1 as weighted (batches 9,10)
    # and materials 2-10 as rigid (batches 0-8).
    #
    # The vert_materials logic:
    #   for each vertex, collect ALL material indices it belongs to
    #   if ANY material is weighted -> leave in world space (no transform)
    #   if ALL materials are rigid -> apply bone inverse
    #
    # PROBLEM: vertices shared between Blender materials 0/1 (weighted) and 2+ (rigid)
    # get left in world space. The rigid batch then references a world-space position
    # where it expects bone-local. BUT we showed rigid batches are 100% correct...
    #
    # So the ACTUAL problem must be different. Let me check the export position pool
    # ordering more carefully.

    # Check: for export batch 9 (weighted), what positions does it reference?
    # And what are those positions' values vs original?
    e_b9_pis = sorted(collect_pos_indices(ed, e_shp1, e_off_d, e_batches[9]))
    o_b9_pis = sorted(collect_pos_indices(od, o_shp1, o_off_d, o_batches[9]))

    print(f"\nBatch 9 posIndex ranges: orig=[{min(o_b9_pis)},{max(o_b9_pis)}] exp=[{min(e_b9_pis)},{max(e_b9_pis)}]")

    # Check: are the export weighted positions bone-local-transformed versions
    # of the original weighted positions?
    # Original weighted positions should be in bind-pose space.
    # If exporter wrongly applied bone_inv, they'd be in bone-local.

    # Let me check if export weighted positions match ANY rigid batch positions from the original
    e_b9_val_set = set()
    for pi in e_b9_pis:
        v = (round(e_pos[pi][0], 3), round(e_pos[pi][1], 3), round(e_pos[pi][2], 3))
        e_b9_val_set.add(v)

    # How many export batch 9 values match original RIGID positions?
    match_rigid = len(e_b9_val_set & set(o_rigid_vals.keys()))
    match_weighted = len(e_b9_val_set & set(o_weighted_vals.keys()))
    print(f"\nExport batch 9 values matching original:")
    print(f"  Match original RIGID positions: {match_rigid}")
    print(f"  Match original WEIGHTED positions: {match_weighted}")
    print(f"  Match neither: {len(e_b9_val_set) - match_rigid - match_weighted + len(e_b9_val_set & set(o_rigid_vals.keys()) & set(o_weighted_vals.keys()))}")

    # CRITICAL: Check if some of the export batch 9 positions are bone-local
    # (i.e., they come from rigid batches 4-8 in the original)
    # This would mean the exporter stored bone-local positions for the weighted batch

    # Specifically: original batch 4 (rigid, bone 27) positions are bone-local
    # If export batch 9 has these same bone-local values, the transform was wrong
    for ob in o_batches:
        if ob['has_mtxidx']:
            continue
        ob_pis = collect_pos_indices(od, o_shp1, o_off_d, ob)
        ob_vals = set()
        for pi in ob_pis:
            ob_vals.add((round(o_pos[pi][0], 3), round(o_pos[pi][1], 3), round(o_pos[pi][2], 3)))
        overlap = ob_vals & e_b9_val_set
        if overlap:
            print(f"\n  Export batch 9 has {len(overlap)} positions matching ORIG RIGID batch {ob['idx']}:")
            for v in sorted(overlap)[:5]:
                print(f"    ({v[0]:9.3f},{v[1]:9.3f},{v[2]:9.3f})")

    # Now let me check the EXPORT deduplication: export batch 4 (rigid) and batch 9 (weighted)
    # share some indices in the export
    e_b4_pis = set(collect_pos_indices(ed, e_shp1, e_off_d, e_batches[4]))
    e_b9_pis_set = set(e_b9_pis)
    shared_exp = e_b4_pis & e_b9_pis_set
    print(f"\n  Export batch 4 (rigid) & batch 9 (weighted) shared indices: {len(shared_exp)}")
    if shared_exp:
        print(f"    Shared: {sorted(shared_exp)[:15]}")
        print("    These vertices have ONE stored position but need TWO coordinate spaces!")
        print("    Rigid batch expects bone-local, weighted batch expects world/bind-pose.")
        print("    --> The deduplication is MERGING positions that must remain separate.")

    # Check ALL rigid-weighted sharing in export
    e_rigid_idx = set()
    e_weighted_idx = set()
    for b in e_batches:
        pis = collect_pos_indices(ed, e_shp1, e_off_d, b)
        if b['has_mtxidx']:
            e_weighted_idx |= pis
        else:
            e_rigid_idx |= pis

    shared_all = e_rigid_idx & e_weighted_idx
    print(f"\n  Export total shared indices (rigid & weighted): {len(shared_all)}")
    if shared_all:
        print(f"    These are: {sorted(shared_all)[:30]}")
        print("\n    For each shared index, show which batches use it:")
        for pi in sorted(shared_all)[:10]:
            using = []
            for b in e_batches:
                pis = collect_pos_indices(ed, e_shp1, e_off_d, b)
                if pi in pis:
                    kind = "W" if b['has_mtxidx'] else "R"
                    using.append(f"batch{b['idx']}({kind})")
            print(f"      posIdx={pi}: used by {', '.join(using)} -> val=({e_pos[pi][0]:.3f},{e_pos[pi][1]:.3f},{e_pos[pi][2]:.3f})")
            # Also show what the orig has at this index
            if pi < len(o_pos):
                print(f"        orig[{pi}]=({o_pos[pi][0]:.3f},{o_pos[pi][1]:.3f},{o_pos[pi][2]:.3f})")

    # FINAL: What transform is being applied?
    print(f"\n\n{'='*70}")
    print("  DIAGNOSIS")
    print(f"{'='*70}")
    print(f"""
  Position pool: orig={len(o_pos)}, export={len(e_pos)} (LOST {len(o_pos)-len(e_pos)} entries)

  The exporter's Vtx1.FromBlenderMesh() deduplicates positions by XYZ value.
  In Blender, all vertices are in world space. The exporter decides per-vertex:
    - If vertex is in ANY weighted material -> leave in world space
    - If vertex is in ALL rigid materials -> apply bone_inv transform

  After transform, the XYZ values become different for rigid vs weighted.
  But the DEDUPLICATION happens AFTER the transform, based on final XYZ values.

  So deduplication itself shouldn't merge rigid/weighted positions (different values).

  The actual issue must be that the weighted batch positions are being
  transformed when they shouldn't be, or NOT transformed when they should be.

  Export batch 9 (weighted) shares {len(shared_all)} indices with rigid batches.
  This means the EXPORTER IS producing shared indices, which is wrong.

  The exporter stores ONE copy of each position. For shared vertices:
  - If stored in bone-local (for rigid) -> weighted batch reads wrong value
  - If stored in world-space (for weighted) -> rigid batch reads wrong value

  Since rigid batches are 100% correct, the shared positions are in bone-local space.
  Therefore the weighted batches are reading bone-local positions where they need world-space.

  ROOT CAUSE: The VTX1 position pool must have SEPARATE entries for the same
  geometric vertex when it appears in both rigid and weighted batches. The original
  BMD has {len(o_pos)} positions with 0 shared indices. The export has {len(e_pos)} positions
  with {len(shared_all)} shared indices. The {len(o_pos)-len(e_pos)} "missing" entries are exactly
  the world-space duplicates that got merged by deduplication.
""")


if __name__ == '__main__':
    main()
