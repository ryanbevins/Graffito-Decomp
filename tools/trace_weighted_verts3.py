#!/usr/bin/env python3
"""Check if weighted positions have a bone inverse transform applied.

The hypothesis: Vtx1.py's FromBlenderMesh is applying bone inverse transforms
to vertices in weighted batches when it shouldn't. Weighted batch positions
should be in WORLD (bind-pose) space, not bone-local space.

Let's verify by checking if orig_weighted_pos ≈ bone_matrix @ exp_weighted_pos
or if exp_weighted_pos ≈ bone_inv @ orig_weighted_pos.
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
                return pos
    return []

def parse_jnt1_matrices(data, jnt1_off):
    """Parse JNT1 bone world matrices."""
    # JNT1 header: tag(4) + size(4) + count(2) + pad(2) + offsets...
    count = r16(data, jnt1_off + 8)
    # JNT1 layout: tag(4) size(4) count(2) pad(2)
    # +0x0C: offset to joint data
    # +0x10: offset to remap table
    # +0x14: offset to string table
    off_joints = r32(data, jnt1_off + 0x0C)

    # Each joint entry is 0x40 bytes (64 bytes):
    # u16 unk0, u8 unk1, u8 pad, f32 sx, f32 sy, f32 sz,
    # s16 rx, s16 ry, s16 rz, s16 pad,
    # f32 tx, f32 ty, f32 tz,
    # f32 bbMin[3], f32 bbMax[3]
    # Total = 2+1+1 + 12 + 8 + 12 + 24 = 60 ... hmm, let me just check the actual structure

    print(f"  JNT1 at 0x{jnt1_off:X}, {count} bones")
    print(f"    offJoints=0x{off_joints:X}")

    # Actually, the joint data format varies. Let me just read the transforms
    # from the EVP1 inverse bind matrices instead, which are guaranteed 3x4 matrices.
    return count

def parse_evp1(data, evp1_off):
    """Parse EVP1 section - inverse bind matrices."""
    count = r16(data, evp1_off + 8)
    # EVP1: tag(4) size(4) count(2) pad(2)
    # +0x0C: offset to counts array (u8 per entry)
    # +0x10: offset to indices array
    # +0x14: offset to weights array
    # +0x18: offset to inverse bind matrices (3x4 float each = 48 bytes)
    off_counts = r32(data, evp1_off + 0x0C)
    off_indices = r32(data, evp1_off + 0x10)
    off_weights = r32(data, evp1_off + 0x14)
    off_matrices = r32(data, evp1_off + 0x18)

    print(f"  EVP1 at 0x{evp1_off:X}, {count} entries")
    print(f"    offMatrices=0x{off_matrices:X}")

    # Read inverse bind matrices (one per bone that has weighted verts)
    # But we need the bone count from somewhere... count here is envelope count
    # Let me read the matrices - they start at off_matrices
    # Each matrix is 3x4 floats = 48 bytes

    # First, figure out how many matrices there are
    sec_size = r32(data, evp1_off + 4)
    mat_start = evp1_off + off_matrices
    mat_space = evp1_off + sec_size - mat_start
    num_matrices = mat_space // 48

    matrices = []
    for mi in range(num_matrices):
        m = []
        for r in range(3):
            row = []
            for c in range(4):
                row.append(rf32(data, mat_start + mi*48 + r*16 + c*4))
            m.append(row)
        matrices.append(m)

    return count, matrices

def parse_drw1(data, drw1_off):
    """Parse DRW1 section."""
    count = r16(data, drw1_off + 8)
    off_isWeighted = r32(data, drw1_off + 0x0C)
    off_data = r32(data, drw1_off + 0x10)

    isWeighted = []
    for i in range(count):
        isWeighted.append(r8(data, drw1_off + off_isWeighted + i))

    data_arr = []
    for i in range(count):
        data_arr.append(r16(data, drw1_off + off_data + i*2))

    print(f"  DRW1 at 0x{drw1_off:X}, {count} entries")
    for i in range(count):
        kind = "weighted" if isWeighted[i] else "rigid"
        print(f"    [{i:2d}] {kind:8s} data={data_arr[i]}")

    return isWeighted, data_arr

def parse_shp1_mtx_info(data, shp1_off):
    """Parse SHP1 matrix table and matrix data for each batch."""
    bc = r16(data, shp1_off + 8)
    off_b = r32(data, shp1_off + 0x0C)
    off_a = r32(data, shp1_off + 0x18)
    off_mt = r32(data, shp1_off + 0x1C)
    off_d = r32(data, shp1_off + 0x20)
    off_md = r32(data, shp1_off + 0x24)
    off_pl = r32(data, shp1_off + 0x28)

    batches = []
    for bi in range(bc):
        bo = shp1_off + off_b + bi*40
        mt = r16(data, bo)
        pc = r16(data, bo+2)
        ao = r16(data, bo+4)
        fmd = r16(data, bo+6)
        fpl = r16(data, bo+8)

        # Read attribs
        attribs = []
        aoff = shp1_off + off_a + ao
        while True:
            at = r32(data, aoff)
            if at == 0xFF: break
            dt = r32(data, aoff+4)
            attribs.append((at, dt))
            aoff += 8
            if len(attribs) > 20: break

        # Read matrix data for each packet
        pkt_mtx_data = []
        for pi in range(pc):
            mdo = shp1_off + off_md + (fmd + pi)*8
            unk1 = r16(data, mdo)
            cnt = r16(data, mdo+2)
            first_idx = r32(data, mdo+4)
            # Read matrix table entries
            mtx_entries = []
            for mi in range(cnt):
                entry = r16(data, shp1_off + off_mt + (first_idx + mi)*2)
                mtx_entries.append(entry)
            pkt_mtx_data.append({'unk1': unk1, 'count': cnt, 'firstIdx': first_idx, 'entries': mtx_entries})

        has_mtxidx = any(a == 0 for a, d in attribs)
        batches.append({
            'idx': bi, 'mt': mt, 'pc': pc, 'attribs': attribs,
            'has_mtxidx': has_mtxidx, 'pkt_mtx': pkt_mtx_data,
            'fmd': fmd,
        })

    return batches

def main():
    orig_path = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"

    with open(orig_path, 'rb') as f:
        data = f.read()

    vtx1_off, _ = find_section(data, b'VTX1')
    shp1_off, _ = find_section(data, b'SHP1')
    evp1_off, _ = find_section(data, b'EVP1')
    drw1_off, _ = find_section(data, b'DRW1')
    jnt1_off, _ = find_section(data, b'JNT1')

    positions = parse_positions(data, vtx1_off)
    print(f"Position pool: {len(positions)} entries")

    jnt_count = parse_jnt1_matrices(data, jnt1_off)
    evp_count, inv_bind_mats = parse_evp1(data, evp1_off)
    isWeighted, drw_data = parse_drw1(data, drw1_off)
    batches = parse_shp1_mtx_info(data, shp1_off)

    print(f"\n  EVP1 inverse bind matrices: {len(inv_bind_mats)}")
    for i, m in enumerate(inv_bind_mats):
        print(f"    Matrix {i}: [{m[0][0]:8.4f} {m[0][1]:8.4f} {m[0][2]:8.4f} {m[0][3]:8.4f}]")
        print(f"               [{m[1][0]:8.4f} {m[1][1]:8.4f} {m[1][2]:8.4f} {m[1][3]:8.4f}]")
        print(f"               [{m[2][0]:8.4f} {m[2][1]:8.4f} {m[2][2]:8.4f} {m[2][3]:8.4f}]")

    # Check weighted batches: what DRW1 entries do they reference through the matrix table?
    print("\n  Weighted batch matrix table entries -> DRW1 mapping:")
    for b in batches:
        if not b['has_mtxidx']:
            continue
        print(f"\n  Batch {b['idx']} (weighted), {b['pc']} packets:")
        for pi, pkt in enumerate(b['pkt_mtx']):
            print(f"    Packet {pi}: {pkt['count']} mtx entries starting at {pkt['firstIdx']}")
            for mi, entry in enumerate(pkt['entries']):
                if entry < len(isWeighted):
                    kind = "weighted" if isWeighted[entry] else "rigid"
                    val = drw_data[entry]
                    print(f"      mtxTable[{pkt['firstIdx']+mi}] = drw[{entry}] -> {kind}, data={val}")
                else:
                    if entry == 0xFFFF:
                        print(f"      mtxTable[{pkt['firstIdx']+mi}] = 0xFFFF (reuse previous)")
                    else:
                        print(f"      mtxTable[{pkt['firstIdx']+mi}] = {entry} (out of range!)")

    # For rigid batches, show their single matrix table entry
    print("\n  Rigid batch matrix table entries:")
    for b in batches:
        if b['has_mtxidx']:
            continue
        for pi, pkt in enumerate(b['pkt_mtx']):
            entries_str = ", ".join(str(e) for e in pkt['entries'])
            drw_info = []
            for e in pkt['entries']:
                if e < len(isWeighted):
                    kind = "W" if isWeighted[e] else "R"
                    drw_info.append(f"drw[{e}]={kind}:{drw_data[e]}")
            print(f"    Batch {b['idx']} pkt{pi}: mtx=[{entries_str}] -> [{', '.join(drw_info)}]")

    # KEY ANALYSIS: what the mat_bone_map logic in the exporter would produce
    # batch_order = [9, 7, 10, 8, 6, 5, 4, 2, 3, 0, 1]
    # Materials 0 and 1 are weighted (batches 9 and 10).
    # The code checks: for each material, are all verts single-bone?
    # If yes -> rigid, apply bone inverse transform
    # If no -> weighted, leave in world space

    # But the PROBLEM is vert_materials "first wins":
    # A vertex shared between material 0 (weighted) and material 4 (rigid)
    # gets classified as "in weighted material" and left in world space.
    # But batch 4 (rigid) expects bone-local space!

    # Wait, the data shows NO shared posIndices between rigid and weighted batches.
    # So the issue must be different...

    # Let me check: the exporter deduplicates vertices into a shared position pool.
    # Position index 201 in the original is specific to rigid batch 4.
    # In the export, that same geometric position might get index 7 (shared with batch 0).
    # But batch 0 is also rigid, so no conflict there.

    # The REAL question: why are weighted batch positions wrong?
    # Let me check what transform the exporter applies to weighted verts.
    # In FromBlenderMesh:
    #   if vi in vert_bone_inv_matrix: gc_pos = inv @ gc_pos_world
    #   else: gc_pos = gc_pos_world  (world space, correct for weighted)
    #
    # The mat_bone_map for weighted materials returns (False, None, None)
    # So in_weighted = True for those verts -> skip -> left in world space
    #
    # BUT WAIT: what if vert_materials assigns a vertex to a RIGID material
    # even though it's also in a WEIGHTED material? The code says:
    #   in_weighted = any(not mat_bone_map.get(mi, (False,))[0] for mi in mats)
    #   if in_weighted: continue  # leave in world space
    #
    # This should be correct... unless a vertex is ONLY in a rigid material
    # but its geometric position happens to be reused by the weighted batch too
    # (through index deduplication).

    # Actually - THE PROBLEM IS DEDUPLICATION!
    # Original BMD has SEPARATE position entries for rigid and weighted batches.
    # Position 201 (bone-local) is different from position 533 (world-space)
    # even if they represent the same geometric vertex.
    # But the exporter deduplicates by position VALUE, creating ONE entry.
    # If the vertex is transformed to bone-local (for the rigid batch),
    # the weighted batch will reference the WRONG (bone-local) value!
    # If left in world space (for the weighted batch),
    # the rigid batch gets the wrong value!

    # VERIFICATION: check if any positions in the original's rigid range
    # would be the world-space version of positions in the weighted range
    print("\n\n=== CHECKING DEDUPLICATION HYPOTHESIS ===")
    print("If true: some orig rigid positions == orig weighted positions when transformed")
    print("Let's just check: do any rigid-range values == weighted-range values?")

    rigid_vals = set()
    for pi in range(0, 533):  # rigid range
        v = positions[pi]
        rigid_vals.add((round(v[0], 2), round(v[1], 2), round(v[2], 2)))

    weighted_vals = set()
    for pi in range(533, len(positions)):
        v = positions[pi]
        weighted_vals.add((round(v[0], 2), round(v[1], 2), round(v[2], 2)))

    overlap = rigid_vals & weighted_vals
    print(f"  Rigid values: {len(rigid_vals)}")
    print(f"  Weighted values: {len(weighted_vals)}")
    print(f"  Overlapping values: {len(overlap)}")
    if overlap:
        for v in sorted(overlap)[:10]:
            print(f"    ({v[0]:8.2f}, {v[1]:8.2f}, {v[2]:8.2f})")


if __name__ == '__main__':
    main()
