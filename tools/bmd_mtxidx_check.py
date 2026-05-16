#!/usr/bin/env python3
"""Check matrixIndex values in exported batch 9 for correctness."""

import struct
import sys
sys.path.insert(0, r"C:\Users\ryana\Documents\sms\tools")
from bmd_deep_diff import BMDParser

def main():
    orig_path = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"
    export_path = r"C:\Users\ryana\Downloads\reconstruct_test.bmd"

    orig = BMDParser(orig_path)
    exp = BMDParser(export_path)

    orig_batches = orig.parse_shp1()
    exp_batches = exp.parse_shp1()

    for bi in (9, 10):
        print(f"\n{'='*60}")
        print(f"Batch {bi}")
        print(f"{'='*60}")

        for label, batches in [("ORIG", orig_batches), ("EXP", exp_batches)]:
            if bi >= len(batches):
                continue
            batch = batches[bi]
            if not batch['has_mtx_idx']:
                continue

            print(f"\n  [{label}]")
            for pi, pkt in enumerate(batch['packets']):
                mtx_table = pkt['matrixTable']
                max_local_idx = len(mtx_table) - 1

                # Collect all matrixIndex values
                mtx_indices = [v.get('matrixIndex', 0) for v in pkt['vertices']]
                local_indices = [mi // 3 for mi in mtx_indices]
                max_used = max(local_indices) if local_indices else 0
                min_used = min(local_indices) if local_indices else 0

                oob_count = sum(1 for li in local_indices if li > max_local_idx)
                unique_mtx_indices = sorted(set(mtx_indices))

                print(f"    Pkt {pi}: table_size={len(mtx_table)}, "
                      f"verts={len(pkt['vertices'])}, "
                      f"matrixIndex range=[{min(mtx_indices) if mtx_indices else 0}..{max(mtx_indices) if mtx_indices else 0}], "
                      f"local_idx range=[{min_used}..{max_used}], "
                      f"OOB={oob_count}, "
                      f"unique_mtxIdx={unique_mtx_indices}")

                # Check for the *3 pattern
                non_divisible = [mi for mi in mtx_indices if mi % 3 != 0]
                if non_divisible:
                    print(f"      WARNING: {len(non_divisible)} matrixIndex values not divisible by 3!")
                    print(f"      Examples: {sorted(set(non_divisible))[:10]}")

    # Also check: what does the primitive data actually look like in the binary?
    # Read raw primitive bytes for batch 9
    print(f"\n{'='*60}")
    print("RAW PRIMITIVE DATA SAMPLING - Batch 9 Packet 0")
    print(f"{'='*60}")

    for label, path in [("ORIG", orig_path), ("EXP", export_path)]:
        with open(path, 'rb') as f:
            data = f.read()

        # Find SHP1
        shp1_off = data.find(b'SHP1')
        size = struct.unpack_from('>I', data, shp1_off + 4)[0]

        # Parse header
        p = shp1_off + 0x0C
        off_batches = struct.unpack_from('>I', data, p)[0]; p += 4
        off_unknown = struct.unpack_from('>I', data, p)[0]; p += 4
        off_zero = struct.unpack_from('>I', data, p)[0]; p += 4
        off_attribs = struct.unpack_from('>I', data, p)[0]; p += 4
        off_mtx_table = struct.unpack_from('>I', data, p)[0]; p += 4
        off_data = struct.unpack_from('>I', data, p)[0]; p += 4
        off_mtx_data = struct.unpack_from('>I', data, p)[0]; p += 4
        off_pkt_locs = struct.unpack_from('>I', data, p)[0]; p += 4

        # Batch 9
        bi = 9
        bd_off = shp1_off + off_batches + bi * 40
        bd_pkt_count = struct.unpack_from('>H', data, bd_off + 2)[0]
        bd_first_mtx = struct.unpack_from('>H', data, bd_off + 6)[0]
        bd_first_pkt = struct.unpack_from('>H', data, bd_off + 8)[0]

        # Packet 0
        pl_off = shp1_off + off_pkt_locs + bd_first_pkt * 8
        pkt_size = struct.unpack_from('>I', data, pl_off)[0]
        pkt_offset = struct.unpack_from('>I', data, pl_off + 4)[0]

        abs_data = shp1_off + off_data + pkt_offset
        print(f"\n  [{label}] Packet 0 at file offset {abs_data:#x}, size={pkt_size}")

        # Read first 64 bytes of raw primitive data
        raw = data[abs_data:abs_data + min(64, pkt_size)]
        hex_str = ' '.join(f'{b:02x}' for b in raw)
        print(f"  First 64 bytes: {hex_str}")

        # Parse first primitive header
        prim_type = raw[0]
        vert_count = struct.unpack_from('>H', raw, 1)[0]
        print(f"  Prim type: {prim_type:#04x}, vert count: {vert_count}")

        # Attribs for batch 9: (0, 1), (9, 3), (10, 3), (13, 3), (14, 3), (15, 3)
        # dataType 1 = u8, dataType 3 = u16
        # stride = 1 + 2 + 2 + 2 + 2 + 2 = 11 bytes per vertex
        stride = 1 + 2 + 2 + 2 + 2 + 2  # matIdx(u8) + pos(u16) + nrm(u16) + tex0(u16) + tex1(u16) + tex2(u16)
        print(f"  Vertex stride: {stride} bytes")

        # Show first 5 vertices
        p = 3  # after prim header
        for vi in range(min(5, vert_count)):
            if p + stride > len(raw):
                break
            mtx_idx = raw[p]
            pos_idx = struct.unpack_from('>H', raw, p + 1)[0]
            nrm_idx = struct.unpack_from('>H', raw, p + 3)[0]
            tex0_idx = struct.unpack_from('>H', raw, p + 5)[0]
            tex1_idx = struct.unpack_from('>H', raw, p + 7)[0]
            tex2_idx = struct.unpack_from('>H', raw, p + 9)[0]
            print(f"    Vert {vi}: mtxIdx={mtx_idx}, posIdx={pos_idx}, nrmIdx={nrm_idx}, "
                  f"tex0={tex0_idx}, tex1={tex1_idx}, tex2={tex2_idx}")
            p += stride


if __name__ == '__main__':
    main()
