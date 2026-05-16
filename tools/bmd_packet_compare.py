"""Compare packet structure between original and exported BMD in detail."""
import struct

def read_u8(f): return struct.unpack('>B', f.read(1))[0]
def read_u16(f): return struct.unpack('>H', f.read(2))[0]
def read_s16(f): return struct.unpack('>h', f.read(2))[0]
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

def dump_shp1_detail(f, label):
    print(f"\n{'='*70}")
    print(f"  {label}")
    print(f"{'='*70}")

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

    print(f"SHP1 offset=0x{shp1_offset:x}, size={shp1_size}")
    print(f"  batches={batch_count}")
    print(f"  offsetBatches=0x{offset_batches:x}")
    print(f"  offsetAttribs=0x{offset_attribs:x}")
    print(f"  offsetMtxTable=0x{offset_mtx_table:x}")
    print(f"  offsetData=0x{offset_data:x}")
    print(f"  offsetMtxData=0x{offset_mtx_data:x}")
    print(f"  offsetPktLocs=0x{offset_pkt_locations:x}")

    for bi in range(batch_count):
        f.seek(shp1_offset + offset_batches + bi * 40)
        mattype_raw = read_u16(f)
        pkt_count = read_u16(f)
        attrib_off = read_u16(f)
        first_mtx_data = read_u16(f)
        first_pkt_loc = read_u16(f)
        unk3 = read_u16(f)
        floats = [read_f32(f) for _ in range(7)]

        mattype = (mattype_raw >> 8) & 0xFF

        # Read attribs
        attribs = []
        f.seek(shp1_offset + offset_attribs + attrib_off)
        while True:
            attr = read_u32(f)
            dtype = read_u32(f)
            if attr == 0xFF:
                break
            attribs.append((attr, dtype))

        print(f"\n  Batch {bi}: mattype=0x{mattype:02x}, raw=0x{mattype_raw:04x}, "
              f"pkts={pkt_count}, attribOff={attrib_off}, "
              f"firstMtxData={first_mtx_data}, firstPktLoc={first_pkt_loc}")
        print(f"    attribs: {[(hex(a), d) for a,d in attribs]}")
        print(f"    bbox floats: {[f'{v:.2f}' for v in floats]}")

        for pi in range(pkt_count):
            # Matrix data
            mtx_data_idx = first_mtx_data + pi
            f.seek(shp1_offset + offset_mtx_data + mtx_data_idx * 8)
            mtx_unk1 = read_u16(f)
            mtx_count = read_u16(f)
            mtx_first_idx = read_u32(f)

            # Read matrix table entries for this packet
            mtx_entries = []
            f.seek(shp1_offset + offset_mtx_table + mtx_first_idx * 2)
            for _ in range(mtx_count):
                mtx_entries.append(read_u16(f))

            # Packet location
            pkt_loc_idx = first_pkt_loc + pi
            f.seek(shp1_offset + offset_pkt_locations + pkt_loc_idx * 8)
            prim_size = read_u32(f)
            prim_offset = read_u32(f)

            print(f"    Packet {pi}: mtxData(unk1={mtx_unk1}, count={mtx_count}, firstIdx={mtx_first_idx})")
            print(f"      mtxTable entries: {mtx_entries}")
            print(f"      primData: size={prim_size}, offset=0x{prim_offset:x} (abs=0x{shp1_offset + offset_data + prim_offset:x})")

            # Read primitive headers
            f.seek(shp1_offset + offset_data + prim_offset)
            bytes_read = 0
            prim_num = 0
            while bytes_read < prim_size:
                pt = read_u8(f)
                bytes_read += 1
                if pt == 0 or bytes_read >= prim_size:
                    break
                vc = read_u16(f)
                bytes_read += 2

                # Count bytes per vertex
                bpv = sum(2 if d == 3 else 1 for _, d in attribs)
                skip = vc * bpv
                f.read(skip)
                bytes_read += skip

                print(f"      prim[{prim_num}]: type=0x{pt:02x}, verts={vc}, bytes={skip}")

                # For weighted batches, sample first few vertex matrixIndices
                if any(a == 0 for a, _ in attribs) and vc > 0 and prim_num == 0:
                    # Go back and read first few
                    save = f.tell()
                    f.seek(shp1_offset + offset_data + prim_offset + 3)  # skip type+count
                    sample_count = min(vc, 6)
                    sample_mtx = []
                    for _ in range(sample_count):
                        vals = {}
                        for attr, dtype in attribs:
                            if dtype == 1:
                                v = read_u8(f)
                            elif dtype == 3:
                                v = read_u16(f)
                            else:
                                v = 0
                            vals[attr] = v
                        sample_mtx.append(f"mtx={vals.get(0,'?')} pos={vals.get(9,'?')}")
                    f.seek(save)
                    print(f"        sample verts: {sample_mtx}")

                prim_num += 1

for path, label in [
    (r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd", "ORIGINAL"),
    (r"C:\Users\ryana\Downloads\reconstruct_test.bmd", "EXPORTED"),
]:
    with open(path, 'rb') as f:
        dump_shp1_detail(f, label)
