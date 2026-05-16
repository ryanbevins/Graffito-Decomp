#!/usr/bin/env python3
"""Compare actual position values in VTX1 between original and export."""

import struct
import math

def read_positions_s16(data, base_offset, count, decimal_point):
    """Read count s16 xyz positions from data starting at base_offset."""
    scale = 1.0 / (2 ** decimal_point)
    positions = []
    pos = base_offset
    for _ in range(count):
        x = struct.unpack_from('>h', data, pos)[0] * scale
        y = struct.unpack_from('>h', data, pos + 2)[0] * scale
        z = struct.unpack_from('>h', data, pos + 4)[0] * scale
        positions.append((x, y, z))
        pos += 6
    return positions

def read_positions_raw_s16(data, base_offset, count):
    """Read count raw s16 xyz values."""
    positions = []
    pos = base_offset
    for _ in range(count):
        x = struct.unpack_from('>h', data, pos)[0]
        y = struct.unpack_from('>h', data, pos + 2)[0]
        z = struct.unpack_from('>h', data, pos + 4)[0]
        positions.append((x, y, z))
        pos += 6
    return positions

def main():
    orig_path = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"
    export_path = r"C:\Users\ryana\Downloads\reconstruct_test.bmd"

    with open(orig_path, 'rb') as f:
        orig = f.read()
    with open(export_path, 'rb') as f:
        exp = f.read()

    # Find VTX1 sections
    def get_vtx1_info(data):
        vtx1_off = data.find(b'VTX1')
        size = struct.unpack_from('>I', data, vtx1_off + 4)[0]
        fmt_offset = struct.unpack_from('>I', data, vtx1_off + 8)[0]
        offsets = []
        for i in range(13):
            offsets.append(struct.unpack_from('>I', data, vtx1_off + 12 + i * 4)[0])

        # Position slot (first non-zero)
        pos_slot = next(i for i in range(13) if offsets[i] != 0)
        active_before = sum(1 for i in range(pos_slot) if offsets[i] != 0)

        fmt_pos = vtx1_off + fmt_offset + active_before * 16
        arr_type = struct.unpack_from('>I', data, fmt_pos)[0]
        comp_count = struct.unpack_from('>I', data, fmt_pos + 4)[0]
        data_type = struct.unpack_from('>I', data, fmt_pos + 8)[0]
        dec_pt = data[fmt_pos + 12]

        # Compute length
        start_off = offsets[pos_slot]
        end_off = size
        for i in range(pos_slot + 1, 13):
            if offsets[i] != 0:
                end_off = offsets[i]
                break
        length = end_off - start_off

        abs_start = vtx1_off + start_off
        count = length // 6 if data_type == 3 else length // 12

        return {
            'vtx1_off': vtx1_off,
            'abs_start': abs_start,
            'count': count,
            'data_type': data_type,
            'dec_pt': dec_pt,
            'length': length,
        }

    orig_info = get_vtx1_info(orig)
    exp_info = get_vtx1_info(exp)

    print(f"Original: {orig_info['count']} positions, type={'s16' if orig_info['data_type'] == 3 else 'f32'}, "
          f"decPt={orig_info['dec_pt']}")
    print(f"Export:   {exp_info['count']} positions, type={'s16' if exp_info['data_type'] == 3 else 'f32'}, "
          f"decPt={exp_info['dec_pt']}")

    # Read raw s16 values
    orig_raw = read_positions_raw_s16(orig, orig_info['abs_start'], orig_info['count'])
    exp_raw = read_positions_raw_s16(exp, exp_info['abs_start'], exp_info['count'])

    # Check for any extremely large values (potential corruption)
    print(f"\nOriginal raw s16 range:")
    orig_flat = [v for p in orig_raw for v in p]
    print(f"  min={min(orig_flat)}, max={max(orig_flat)}")

    exp_flat = [v for p in exp_raw for v in p]
    print(f"Export raw s16 range:")
    print(f"  min={min(exp_flat)}, max={max(exp_flat)}")

    # Convert to float
    scale_orig = 1.0 / (2 ** orig_info['dec_pt'])
    scale_exp = 1.0 / (2 ** exp_info['dec_pt'])

    orig_pos = [(x * scale_orig, y * scale_orig, z * scale_orig) for x, y, z in orig_raw]
    exp_pos = [(x * scale_exp, y * scale_exp, z * scale_exp) for x, y, z in exp_raw]

    print(f"\nFloat positions range:")
    orig_floats = [v for p in orig_pos for v in p]
    print(f"  Orig: [{min(orig_floats):.4f}, {max(orig_floats):.4f}]")
    exp_floats = [v for p in exp_pos for v in p]
    print(f"  Exp:  [{min(exp_floats):.4f}, {max(exp_floats):.4f}]")

    # Build lookup: position -> set of indices for each file
    orig_lookup = {}
    for i, p in enumerate(orig_pos):
        key = (round(p[0], 2), round(p[1], 2), round(p[2], 2))
        if key not in orig_lookup:
            orig_lookup[key] = []
        orig_lookup[key].append(i)

    exp_lookup = {}
    for i, p in enumerate(exp_pos):
        key = (round(p[0], 2), round(p[1], 2), round(p[2], 2))
        if key not in exp_lookup:
            exp_lookup[key] = []
        exp_lookup[key].append(i)

    # Check how many positions match
    matched_keys = set(orig_lookup.keys()) & set(exp_lookup.keys())
    only_orig = set(orig_lookup.keys()) - set(exp_lookup.keys())
    only_exp = set(exp_lookup.keys()) - set(orig_lookup.keys())

    print(f"\nPosition matching (round to 0.02):")
    print(f"  Matched: {len(matched_keys)}")
    print(f"  Only in orig: {len(only_orig)}")
    print(f"  Only in exp: {len(only_exp)}")

    # Show positions only in export
    if only_exp:
        print(f"\nPositions only in export:")
        for key in sorted(only_exp)[:20]:
            indices = exp_lookup[key]
            for idx in indices:
                raw = exp_raw[idx]
                print(f"  [{idx}] raw=({raw[0]}, {raw[1]}, {raw[2]}) "
                      f"float=({key[0]:.2f}, {key[1]:.2f}, {key[2]:.2f})")

    # Show positions only in original
    if only_orig:
        print(f"\nPositions only in original:")
        for key in sorted(only_orig)[:20]:
            indices = orig_lookup[key]
            for idx in indices:
                raw = orig_raw[idx]
                print(f"  [{idx}] raw=({raw[0]}, {raw[1]}, {raw[2]}) "
                      f"float=({key[0]:.2f}, {key[1]:.2f}, {key[2]:.2f})")

    # For matched positions, check exact raw value differences
    print(f"\nExact raw value comparison for matched positions:")
    raw_diffs = 0
    for key in sorted(matched_keys):
        o_idx = orig_lookup[key][0]
        e_idx = exp_lookup[key][0]
        o_raw = orig_raw[o_idx]
        e_raw = exp_raw[e_idx]
        if o_raw != e_raw:
            raw_diffs += 1
            if raw_diffs <= 10:
                print(f"  [{o_idx}] orig=({o_raw[0]}, {o_raw[1]}, {o_raw[2]}) "
                      f"exp=({e_raw[0]}, {e_raw[1]}, {e_raw[2]})")
    print(f"  Total positions with raw s16 diffs: {raw_diffs}")

    # Check: for positions used in batch 9, are they all present?
    # We need SHP1 to know which positions batch 9 uses
    from bmd_deep_diff import BMDParser
    import sys
    sys.path.insert(0, r"C:\Users\ryana\Documents\sms\tools")

    orig_bmd = BMDParser(orig_path)
    exp_bmd = BMDParser(export_path)
    orig_batches = orig_bmd.parse_shp1()
    exp_batches = exp_bmd.parse_shp1()

    # Get batch 9 position indices
    def batch_pos_indices(batches, bi):
        batch = batches[bi]
        indices = set()
        for pkt in batch['packets']:
            for vert in pkt['vertices']:
                indices.add(vert.get('posIndex', 0))
        return indices

    orig_b9_pos = batch_pos_indices(orig_batches, 9)
    exp_b9_pos = batch_pos_indices(exp_batches, 9)

    print(f"\nBatch 9 position indices:")
    print(f"  Orig: {len(orig_b9_pos)} unique (range {min(orig_b9_pos)}-{max(orig_b9_pos)})")
    print(f"  Exp:  {len(exp_b9_pos)} unique (range {min(exp_b9_pos)}-{max(exp_b9_pos)})")

    # Check if any exported position index is out of range
    oob = [i for i in exp_b9_pos if i >= len(exp_pos)]
    print(f"  Exp out-of-bounds: {len(oob)}")

    # Compare the actual position values for batch 9 vertices
    # Build: orig_pos[idx] -> exp_pos[idx] for each batch 9 pos index
    print(f"\nBatch 9 position value comparison:")
    for pi in sorted(exp_b9_pos):
        ep = exp_pos[pi]
        # Find matching original position
        key = (round(ep[0], 2), round(ep[1], 2), round(ep[2], 2))
        if key in orig_lookup:
            continue  # matches
        print(f"  exp[{pi}] = ({ep[0]:.4f}, {ep[1]:.4f}, {ep[2]:.4f}) - NO MATCH in original")


if __name__ == '__main__':
    main()
