"""Verify exported BMD packet data integrity - check if primitive data is readable."""
import struct

def read_u8(f): return struct.unpack('>B', f.read(1))[0]
def read_u16(f): return struct.unpack('>H', f.read(2))[0]
def read_u32(f): return struct.unpack('>I', f.read(4))[0]
def read_f32(f): return struct.unpack('>f', f.read(4))[0]

def find_section(f, tag):
    f.seek(0x20)
    file_size = f.seek(0, 2)
    f.seek(0x20)
    while f.tell() < file_size:
        pos = f.tell()
        section_tag = f.read(4)
        if len(section_tag) < 4:
            break
        section_size = read_u32(f)
        if section_tag == tag:
            f.seek(pos)
            return pos, section_size
        f.seek(pos + section_size)
    return None, None

def verify_packets(path, label):
    print(f"\n{'='*60}")
    print(f"  {label}: {path}")
    print(f"{'='*60}")

    with open(path, 'rb') as f:
        shp1_offset, shp1_size = find_section(f, b'SHP1')
        f.seek(shp1_offset + 8)
        batch_count = read_u16(f)
        _pad = read_u16(f)
        offset_batches = read_u32(f)
        offset_unknown = read_u32(f)
        _zero = read_u32(f)
        offset_attribs = read_u32(f)
        offset_mtx_table = read_u32(f)
        offset_data = read_u32(f)
        offset_mtx_data = read_u32(f)
        offset_pkt_locations = read_u32(f)

        # Calculate max data area size
        data_end = min(offset_mtx_data, shp1_size)  # data area ends before matrix data
        data_area_size = data_end - offset_data
        print(f"  Prim data area: 0x{offset_data:x} to 0x{data_end:x} ({data_area_size} bytes)")

        for bi in range(batch_count):
            f.seek(shp1_offset + offset_batches + bi * 40)
            mattype_raw = read_u16(f)
            pkt_count = read_u16(f)
            attrib_off = read_u16(f)
            first_mtx_data = read_u16(f)
            first_pkt_loc = read_u16(f)

            # Read attribs
            attribs = []
            f.seek(shp1_offset + offset_attribs + attrib_off)
            while True:
                attr = read_u32(f)
                dtype = read_u32(f)
                if attr == 0xFF:
                    break
                attribs.append((attr, dtype))

            bytes_per_vert = sum(2 if d == 3 else 1 for _, d in attribs)
            is_weighted = (mattype_raw >> 8) == 0x03

            if not is_weighted:
                continue  # Only check weighted batches

            print(f"\n  Batch {bi} (WEIGHTED): pkts={pkt_count}, bytesPerVert={bytes_per_vert}")

            for pi in range(pkt_count):
                # Matrix data
                mtx_data_idx = first_mtx_data + pi
                f.seek(shp1_offset + offset_mtx_data + mtx_data_idx * 8)
                mtx_unk1 = read_u16(f)
                mtx_count = read_u16(f)
                mtx_first_idx = read_u32(f)

                # Matrix table
                f.seek(shp1_offset + offset_mtx_table + mtx_first_idx * 2)
                mtx_entries = [read_u16(f) for _ in range(mtx_count)]

                # Packet location
                pkt_loc_idx = first_pkt_loc + pi
                f.seek(shp1_offset + offset_pkt_locations + pkt_loc_idx * 8)
                prim_size = read_u32(f)
                prim_offset = read_u32(f)

                abs_offset = shp1_offset + offset_data + prim_offset
                print(f"    Pkt {pi}: primSize={prim_size}, primOff=0x{prim_offset:x}, "
                      f"abs=0x{abs_offset:x}, mtxCount={mtx_count}, mtxEntries={mtx_entries}")

                # Verify we can read the primitive data
                f.seek(abs_offset)
                bytes_read = 0
                total_verts = 0
                prim_num = 0
                issues = []

                while bytes_read < prim_size:
                    pt = read_u8(f)
                    bytes_read += 1
                    if pt == 0 or bytes_read >= prim_size:
                        break

                    if pt not in (0x90, 0x98, 0xa0, 0x80):
                        issues.append(f"Unknown prim type 0x{pt:02x} at offset +{bytes_read-1}")
                        break

                    vc = read_u16(f)
                    bytes_read += 2

                    vert_bytes = vc * bytes_per_vert
                    if bytes_read + vert_bytes > prim_size:
                        issues.append(f"Prim {prim_num}: {vc} verts * {bytes_per_vert} = {vert_bytes} bytes "
                                     f"would exceed packet (read={bytes_read}, size={prim_size})")
                        break

                    # Read and validate vertex data
                    max_mtx_idx = 0
                    for vi in range(vc):
                        for attr, dtype in attribs:
                            if dtype == 1:
                                val = read_u8(f)
                            elif dtype == 3:
                                val = read_u16(f)
                            else:
                                val = 0
                            if attr == 0:  # matrixIndex
                                if val >= mtx_count * 3:
                                    issues.append(f"Vert {total_verts+vi}: matrixIndex={val} "
                                                 f"but mtxCount*3={mtx_count*3}")
                                max_mtx_idx = max(max_mtx_idx, val)

                    bytes_read += vert_bytes
                    total_verts += vc
                    prim_num += 1

                if issues:
                    for iss in issues:
                        print(f"      *** {iss}")
                else:
                    print(f"      OK: {prim_num} prims, {total_verts} verts, "
                          f"{bytes_read}/{prim_size} bytes read")

verify_packets(r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd", "ORIGINAL")
verify_packets(r"C:\Users\ryana\Downloads\reconstruct_test.bmd", "EXPORTED")
