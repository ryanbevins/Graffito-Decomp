#!/usr/bin/env python3
"""Check packet data alignment, offsets, and primitive encoding in detail."""

import struct

def main():
    for label, path in [("ORIG", r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"),
                         ("EXP", r"C:\Users\ryana\Downloads\reconstruct_test.bmd")]:
        with open(path, 'rb') as f:
            data = f.read()

        shp1_off = data.find(b'SHP1')
        size = struct.unpack_from('>I', data, shp1_off + 4)[0]

        p = shp1_off + 0x0C
        off_batches = struct.unpack_from('>I', data, p)[0]; p += 4
        off_unknown = struct.unpack_from('>I', data, p)[0]; p += 4
        off_zero = struct.unpack_from('>I', data, p)[0]; p += 4
        off_attribs = struct.unpack_from('>I', data, p)[0]; p += 4
        off_mtx_table = struct.unpack_from('>I', data, p)[0]; p += 4
        off_data = struct.unpack_from('>I', data, p)[0]; p += 4
        off_mtx_data = struct.unpack_from('>I', data, p)[0]; p += 4
        off_pkt_locs = struct.unpack_from('>I', data, p)[0]; p += 4

        print(f"\n{'='*60}")
        print(f"[{label}] SHP1 at {shp1_off:#x}")
        print(f"{'='*60}")

        batch_count = struct.unpack_from('>H', data, shp1_off + 8)[0]

        for bi in range(batch_count):
            bd_off = shp1_off + off_batches + bi * 40
            bd_unknown = struct.unpack_from('>H', data, bd_off)[0]
            bd_pkt_count = struct.unpack_from('>H', data, bd_off + 2)[0]
            bd_attribs_off = struct.unpack_from('>H', data, bd_off + 4)[0]
            bd_first_mtx = struct.unpack_from('>H', data, bd_off + 6)[0]
            bd_first_pkt = struct.unpack_from('>H', data, bd_off + 8)[0]
            bd_unk3 = struct.unpack_from('>H', data, bd_off + 10)[0]

            # Read attribs
            attr_off = shp1_off + off_attribs + bd_attribs_off
            attribs = []
            while True:
                attr = struct.unpack_from('>I', data, attr_off)[0]
                dtype = struct.unpack_from('>I', data, attr_off + 4)[0]
                if attr == 0xff:
                    break
                attribs.append((attr, dtype))
                attr_off += 8

            # Compute stride
            stride = 0
            for attr, dtype in attribs:
                if dtype == 1: stride += 1
                elif dtype == 3: stride += 2

            is_weighted = any(a[0] == 0 for a in attribs)

            if bi not in (9, 10):
                continue

            print(f"\n  Batch {bi}: type={'W' if is_weighted else 'R'}, "
                  f"descriptor_flag=0x{bd_unknown:04x}, "
                  f"pkts={bd_pkt_count}, stride={stride}")

            total_data_bytes = 0
            for pi in range(bd_pkt_count):
                pl_off = shp1_off + off_pkt_locs + (bd_first_pkt + pi) * 8
                pkt_size = struct.unpack_from('>I', data, pl_off)[0]
                pkt_offset = struct.unpack_from('>I', data, pl_off + 4)[0]

                abs_data = shp1_off + off_data + pkt_offset
                abs_data_end = abs_data + pkt_size

                # Read primitives
                p = abs_data
                prim_count = 0
                vert_total = 0
                actual_bytes = 0
                while p < abs_data_end:
                    prim_type = data[p]; p += 1; actual_bytes += 1
                    if prim_type == 0 or actual_bytes >= pkt_size:
                        break
                    vert_count = struct.unpack_from('>H', data, p)[0]; p += 2; actual_bytes += 2
                    prim_count += 1
                    vert_total += vert_count
                    p += vert_count * stride
                    actual_bytes += vert_count * stride

                pad_bytes = pkt_size - actual_bytes
                offset_aligned = (pkt_offset % 32 == 0)
                size_aligned = (pkt_size % 32 == 0)

                # Read matrix data
                md_off = shp1_off + off_mtx_data + (bd_first_mtx + pi) * 8
                md_unk1 = struct.unpack_from('>H', data, md_off)[0]
                md_count = struct.unpack_from('>H', data, md_off + 2)[0]
                md_first = struct.unpack_from('>I', data, md_off + 4)[0]

                print(f"    Pkt {pi}: offset={pkt_offset:#x}(aligned32={offset_aligned}), "
                      f"size={pkt_size}(aligned32={size_aligned}), "
                      f"prims={prim_count}, verts={vert_total}, "
                      f"actual_data={actual_bytes}, pad={pad_bytes}")
                print(f"           mtxData: useMtx={md_unk1:#06x}, count={md_count}, firstIdx={md_first}")

                total_data_bytes += pkt_size

            print(f"    Total data bytes: {total_data_bytes}")

    # Now check something specific: is the batch descriptor unknown4 (bounding box) important?
    print(f"\n{'='*60}")
    print("BATCH DESCRIPTOR COMPARISON")
    print(f"{'='*60}")

    for label, path in [("ORIG", r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"),
                         ("EXP", r"C:\Users\ryana\Downloads\reconstruct_test.bmd")]:
        with open(path, 'rb') as f:
            data = f.read()
        shp1_off = data.find(b'SHP1')
        off_batches = struct.unpack_from('>I', data, shp1_off + 0x0C)[0]
        batch_count = struct.unpack_from('>H', data, shp1_off + 8)[0]

        print(f"\n  [{label}]")
        for bi in (9, 10):
            bd_off = shp1_off + off_batches + bi * 40
            raw = data[bd_off:bd_off + 40]
            hex_str = ' '.join(f'{b:02x}' for b in raw)
            print(f"    Batch {bi}: {hex_str}")

            # Parse the 7 floats
            bd_unknown = struct.unpack_from('>H', raw, 0)[0]
            floats = [struct.unpack_from('>f', raw, 12 + i * 4)[0] for i in range(7)]
            print(f"           unknown=0x{bd_unknown:04x}, bbox floats={[f'{f:.4f}' for f in floats]}")


if __name__ == '__main__':
    main()
