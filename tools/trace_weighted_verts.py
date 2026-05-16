#!/usr/bin/env python3
"""Trace exactly which vertices are wrong in exported weighted batches.

Parses SHP1 batches from both original and exported BMD, finds weighted ones
(matType=3), collects posIndices, looks up XYZ in VTX1, compares values.
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

def parse_vtx1_positions(data, vtx1_off):
    """Parse VTX1 position pool -> list of (x,y,z)."""
    fmt_off = r32(data, vtx1_off + 8)  # arrayFormatOffset
    offsets = [r32(data, vtx1_off + 12 + i*4) for i in range(13)]

    # Read formats for active slots
    fmt_abs = vtx1_off + fmt_off
    fmt_idx = 0
    pos_data_type = None
    pos_decimal = 0
    pos_slot = None
    for i in range(13):
        if offsets[i] != 0:
            arr_type = r32(data, fmt_abs + fmt_idx*16)
            if arr_type == 9:
                pos_data_type = r32(data, fmt_abs + fmt_idx*16 + 8)
                pos_decimal = r8(data, fmt_abs + fmt_idx*16 + 12)
                pos_slot = i
                break
            fmt_idx += 1

    if pos_slot is None:
        return []

    pos_start = offsets[pos_slot]
    pos_end = None
    for j in range(pos_slot + 1, 13):
        if offsets[j] != 0:
            pos_end = offsets[j]
            break
    if pos_end is None:
        pos_end = r32(data, vtx1_off + 4)  # sectionSize

    length = pos_end - pos_start
    abs_start = vtx1_off + pos_start
    positions = []
    if pos_data_type == 4:  # f32
        for i in range(length // 12):
            o = abs_start + i*12
            positions.append((rf32(data, o), rf32(data, o+4), rf32(data, o+8)))
    elif pos_data_type == 3:  # s16
        scale = 1.0 / (2 ** pos_decimal)
        for i in range(length // 6):
            o = abs_start + i*6
            positions.append((rs16(data, o)*scale, rs16(data, o+2)*scale, rs16(data, o+4)*scale))
    return positions


def parse_shp1_full(data, shp1_off):
    """Parse SHP1 using the actual BMD structure."""
    # Header: tag(4) size(4) batchCount(2) pad(2) then 8 u32 offsets
    batch_count = r16(data, shp1_off + 8)
    off_batches = r32(data, shp1_off + 0x0C)
    off_remap   = r32(data, shp1_off + 0x10)
    off_zero    = r32(data, shp1_off + 0x14)
    off_attribs = r32(data, shp1_off + 0x18)
    off_mtx_tbl = r32(data, shp1_off + 0x1C)
    off_data    = r32(data, shp1_off + 0x20)
    off_mtx_dat = r32(data, shp1_off + 0x24)
    off_pkt_loc = r32(data, shp1_off + 0x28)

    print(f"  SHP1 at 0x{shp1_off:X}, {batch_count} batches")
    print(f"    offBatches=0x{off_batches:X} offAttribs=0x{off_attribs:X} offData=0x{off_data:X} offPktLoc=0x{off_pkt_loc:X}")

    batches = []
    BATCH_DESC_SIZE = 40  # 6 u16 + 7 f32
    for bi in range(batch_count):
        bo = shp1_off + off_batches + bi * BATCH_DESC_SIZE
        mat_type = r16(data, bo + 0)  # "unknown" field - actually matType (0x00FF pattern)
        pkt_count = r16(data, bo + 2)
        attrib_off = r16(data, bo + 4)  # byte offset into attrib table
        first_mtx_data = r16(data, bo + 6)
        first_pkt_loc = r16(data, bo + 8)
        pad = r16(data, bo + 10)
        # 7 floats follow (bounding box etc)

        # Read attribs for this batch
        attribs = []
        aoff = shp1_off + off_attribs + attrib_off
        while True:
            attr = r32(data, aoff)
            dtype = r32(data, aoff + 4)
            if attr == 0xFF or attr == 0x000000FF:
                break
            attribs.append((attr, dtype))
            aoff += 8
            if len(attribs) > 20:
                break

        # Read packet locations
        packets = []
        for pi in range(pkt_count):
            plo = shp1_off + off_pkt_loc + (first_pkt_loc + pi) * 8
            pkt_size = r32(data, plo)
            pkt_offset = r32(data, plo + 4)
            packets.append((pkt_size, pkt_offset))

        batches.append({
            'index': bi,
            'matType': mat_type & 0xFF,  # low byte
            'matTypeFull': mat_type,
            'pktCount': pkt_count,
            'attribs': attribs,
            'packets': packets,
            'firstMtxData': first_mtx_data,
        })

    return batches, off_data, off_mtx_tbl, off_mtx_dat


def collect_pos_indices(data, shp1_off, off_data, batch):
    """Collect all posIndex values from a batch's primitive data."""
    attribs = batch['attribs']

    # Calculate vertex stride and posIndex offset
    stride = 0
    pos_offset = -1
    pos_size = 0
    for attr, dtype in attribs:
        if attr == 9:  # POS
            pos_offset = stride
        # dtype: 1=direct(u8), 2=u8index, 3=u16index
        if dtype == 1:
            stride += 1
        elif dtype == 2:
            stride += 1
        elif dtype == 3:
            stride += 2

        if attr == 9:
            pos_size = 2 if dtype == 3 else 1

    if pos_offset < 0:
        return set()

    indices = set()
    for pkt_size, pkt_offset in batch['packets']:
        abs_start = shp1_off + off_data + pkt_offset
        abs_end = abs_start + pkt_size
        off = abs_start

        while off < abs_end:
            prim_type = r8(data, off)
            if prim_type == 0:
                off += 1
                continue
            num_verts = r16(data, off + 1)
            off += 3
            for _ in range(num_verts):
                if off + stride > abs_end:
                    break
                if pos_size == 2:
                    pi = r16(data, off + pos_offset)
                else:
                    pi = r8(data, off + pos_offset)
                indices.add(pi)
                off += stride

    return indices


def collect_mtx_indices(data, shp1_off, off_data, off_mtx_tbl, batch):
    """Collect all matrixIndex values from a batch's primitive data,
    and resolve them through the matrix table."""
    attribs = batch['attribs']

    stride = 0
    mtx_offset = -1
    mtx_size = 0
    pos_offset = -1
    pos_size = 0
    for attr, dtype in attribs:
        if attr == 0:  # PNMTXIDX
            mtx_offset = stride
        if attr == 9:
            pos_offset = stride
        if dtype == 1:
            s = 1
        elif dtype == 2:
            s = 1
        elif dtype == 3:
            s = 2
        else:
            s = 0
        if attr == 0:
            mtx_size = s
        if attr == 9:
            pos_size = 2 if dtype == 3 else 1
        stride += s

    results = []  # (posIndex, matrixIndex_raw, matrixIndex_resolved)
    for pkt_size, pkt_offset in batch['packets']:
        abs_start = shp1_off + off_data + pkt_offset
        abs_end = abs_start + pkt_size
        off = abs_start
        while off < abs_end:
            prim_type = r8(data, off)
            if prim_type == 0:
                off += 1
                continue
            num_verts = r16(data, off + 1)
            off += 3
            for _ in range(num_verts):
                if off + stride > abs_end:
                    break
                pi = r16(data, off + pos_offset) if pos_size == 2 else r8(data, off + pos_offset)
                mi = r8(data, off + mtx_offset) if mtx_offset >= 0 else -1
                results.append((pi, mi))
                off += stride
    return results


def analyze(filepath, label):
    with open(filepath, 'rb') as f:
        data = f.read()

    print(f"\n{'='*70}")
    print(f"  {label}")
    print(f"  File: {filepath} ({len(data)} bytes)")
    print(f"{'='*70}")

    vtx1_off, vtx1_size = find_section(data, b'VTX1')
    shp1_off, shp1_size = find_section(data, b'SHP1')
    print(f"  VTX1: 0x{vtx1_off:X} (0x{vtx1_size:X})")
    print(f"  SHP1: 0x{shp1_off:X} (0x{shp1_size:X})")

    positions = parse_vtx1_positions(data, vtx1_off)
    print(f"  Position pool: {len(positions)} entries")

    batches, off_data, off_mtx_tbl, off_mtx_dat = parse_shp1_full(data, shp1_off)

    print(f"\n  Batch summary:")
    for b in batches:
        wt = "WEIGHTED" if (b['matType'] & 0x03) == 3 else "rigid"
        print(f"    [{b['index']:2d}] matType=0x{b['matTypeFull']:04X} ({wt:8s}) pkts={b['pktCount']} attribs={[(hex(a),d) for a,d in b['attribs']]}")

    weighted = [b for b in batches if (b['matType'] & 0x03) == 3]
    print(f"\n  Weighted batches: {[b['index'] for b in weighted]}")

    all_pos_idx = set()
    batch_pos = {}
    for b in weighted:
        pis = collect_pos_indices(data, shp1_off, off_data, b)
        batch_pos[b['index']] = pis
        all_pos_idx |= pis
        print(f"    Batch {b['index']}: {len(pis)} unique posIndices, range [{min(pis)},{max(pis)}]")

    pos_values = {}
    for pi in sorted(all_pos_idx):
        if pi < len(positions):
            pos_values[pi] = positions[pi]
        else:
            print(f"    WARNING: posIdx {pi} out of range!")

    return {
        'data': data,
        'positions': positions,
        'batches': batches,
        'weighted': weighted,
        'batch_pos': batch_pos,
        'all_pos_idx': all_pos_idx,
        'pos_values': pos_values,
        'shp1_off': shp1_off,
        'off_data': off_data,
        'off_mtx_tbl': off_mtx_tbl,
        'off_mtx_dat': off_mtx_dat,
    }


def compare(orig, exp):
    print(f"\n{'='*70}")
    print(f"  COMPARISON: Weighted Batch Vertices")
    print(f"{'='*70}")

    # Value-level comparison (round to 2 decimals)
    def val_set(pv):
        return {(round(x,2), round(y,2), round(z,2)) for x,y,z in pv.values()}

    ov = val_set(orig['pos_values'])
    ev = val_set(exp['pos_values'])

    print(f"\n  Unique position VALUES in weighted batches:")
    print(f"    Original: {len(ov)}")
    print(f"    Exported: {len(ev)}")
    print(f"    In both:  {len(ov & ev)}")
    print(f"    Original only: {len(ov - ev)}")
    print(f"    Export only:   {len(ev - ov)}")

    # Index-level
    oi = orig['all_pos_idx']
    ei = exp['all_pos_idx']
    print(f"\n  PosIndex sets:")
    print(f"    Original: {len(oi)} indices, range [{min(oi)},{max(oi)}]")
    print(f"    Exported: {len(ei)} indices, range [{min(ei)},{max(ei)}]")
    print(f"    Shared:   {len(oi & ei)}")

    # Same-index value mismatches
    shared = oi & ei
    mismatches = []
    for pi in sorted(shared):
        if pi < len(orig['positions']) and pi < len(exp['positions']):
            ox, oy, oz = orig['positions'][pi]
            ex, ey, ez = exp['positions'][pi]
            if abs(ox-ex) > 0.1 or abs(oy-ey) > 0.1 or abs(oz-ez) > 0.1:
                mismatches.append({
                    'pi': pi,
                    'orig': (ox, oy, oz),
                    'exp': (ex, ey, ez),
                    'delta': (ex-ox, ey-oy, ez-oz),
                })

    print(f"\n  Same-index value mismatches: {len(mismatches)}")
    for m in mismatches[:50]:
        print(f"    posIdx={m['pi']:4d}: orig=({m['orig'][0]:9.3f},{m['orig'][1]:9.3f},{m['orig'][2]:9.3f}) "
              f"exp=({m['exp'][0]:9.3f},{m['exp'][1]:9.3f},{m['exp'][2]:9.3f}) "
              f"delta=({m['delta'][0]:+8.3f},{m['delta'][1]:+8.3f},{m['delta'][2]:+8.3f})")
    if len(mismatches) > 50:
        print(f"    ... {len(mismatches)-50} more")

    if mismatches:
        y_deltas = sorted(set(round(m['delta'][1], 1) for m in mismatches))
        print(f"\n    Distinct Y deltas: {y_deltas}")
        x_deltas = sorted(set(round(m['delta'][0], 1) for m in mismatches))
        print(f"    Distinct X deltas: {x_deltas}")
        z_deltas = sorted(set(round(m['delta'][2], 1) for m in mismatches))
        print(f"    Distinct Z deltas: {z_deltas}")

    # Y-shift matching between missing/extra values
    missing = ov - ev
    extra = ev - ov
    y_shifts = []
    for mx, my, mz in sorted(missing):
        for ex, ey, ez in extra:
            if abs(mx-ex) < 1.0 and abs(mz-ez) < 1.0 and abs(my-ey) >= 20:
                y_shifts.append(((mx,my,mz), (ex,ey,ez), ey-my))
                break

    print(f"\n  Y-shifted pairs (missing<->extra, |dY|>=20): {len(y_shifts)}")
    for o, e, dy in y_shifts[:20]:
        print(f"    orig=({o[0]:8.2f},{o[1]:8.2f},{o[2]:8.2f}) -> exp=({e[0]:8.2f},{e[1]:8.2f},{e[2]:8.2f}) dY={dy:+.2f}")

    # Per-batch overlap
    print(f"\n  Per-batch posIndex overlap:")
    for bi in sorted(set(list(orig['batch_pos'].keys()) + list(exp['batch_pos'].keys()))):
        op = orig['batch_pos'].get(bi, set())
        ep = exp['batch_pos'].get(bi, set())
        print(f"    Batch {bi}: orig={len(op)}, exp={len(ep)}, shared={len(op&ep)}")

    # Full position pool diff
    print(f"\n  Total position pool: orig={len(orig['positions'])}, exp={len(exp['positions'])}")
    pool_mismatches = 0
    for pi in range(min(len(orig['positions']), len(exp['positions']))):
        ox, oy, oz = orig['positions'][pi]
        ex, ey, ez = exp['positions'][pi]
        if abs(ox-ex) > 0.01 or abs(oy-ey) > 0.01 or abs(oz-ez) > 0.01:
            pool_mismatches += 1
    print(f"  Position pool mismatches (any index): {pool_mismatches} / {min(len(orig['positions']), len(exp['positions']))}")

    # Dump first few mismatched positions from the full pool
    if pool_mismatches > 0:
        print(f"\n  First 30 mismatched positions in full pool:")
        count = 0
        for pi in range(min(len(orig['positions']), len(exp['positions']))):
            ox, oy, oz = orig['positions'][pi]
            ex, ey, ez = exp['positions'][pi]
            if abs(ox-ex) > 0.01 or abs(oy-ey) > 0.01 or abs(oz-ez) > 0.01:
                in_weighted_o = pi in orig['all_pos_idx']
                in_weighted_e = pi in exp['all_pos_idx']
                print(f"    posIdx={pi:4d}: orig=({ox:9.3f},{oy:9.3f},{oz:9.3f}) "
                      f"exp=({ex:9.3f},{ey:9.3f},{ez:9.3f}) "
                      f"delta=({ex-ox:+8.3f},{ey-oy:+8.3f},{ez-oz:+8.3f}) "
                      f"inWeighted=[o={'Y' if in_weighted_o else 'N'},e={'Y' if in_weighted_e else 'N'}]")
                count += 1
                if count >= 30:
                    break


def main():
    orig_path = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"
    export_path = r"C:\Users\ryana\Downloads\reconstruct_test.bmd"

    orig = analyze(orig_path, "ORIGINAL")
    exp = analyze(export_path, "EXPORTED")
    compare(orig, exp)


if __name__ == '__main__':
    main()
