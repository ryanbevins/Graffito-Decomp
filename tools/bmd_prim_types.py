"""Check primitive types used in both original and exported BMD."""
import struct
import sys

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

def check_prim_types(path):
    print(f"\n=== {path} ===")
    with open(path, 'rb') as f:
        shp1_offset, shp1_size = find_section(f, b'SHP1')
        if shp1_offset is None:
            print("No SHP1")
            return

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

        batches = []
        for bi in range(batch_count):
            f.seek(shp1_offset + offset_batches + bi * 40)
            unknown = read_u16(f)
            pkt_count = read_u16(f)
            attrib_off = read_u16(f)
            first_mtx_data = read_u16(f)
            first_pkt_loc = read_u16(f)
            _unk3 = read_u16(f)
            _floats = [read_f32(f) for _ in range(7)]

            # Read attribs
            attribs = []
            f.seek(shp1_offset + offset_attribs + attrib_off)
            while True:
                attr = read_u32(f)
                dtype = read_u32(f)
                if attr == 0xFF:
                    break
                attribs.append((attr, dtype))

            batches.append({
                'mattype': unknown,
                'pkt_count': pkt_count,
                'first_pkt_loc': first_pkt_loc,
                'attribs': attribs,
            })

        for bi, batch in enumerate(batches):
            print(f"\n  Batch {bi}: mattype=0x{batch['mattype']:04x}, pkts={batch['pkt_count']}")
            prim_type_counts = {}
            total_verts = 0

            for pi in range(batch['pkt_count']):
                pkt_loc_idx = batch['first_pkt_loc'] + pi
                f.seek(shp1_offset + offset_pkt_locations + pkt_loc_idx * 8)
                prim_size = read_u32(f)
                prim_offset = read_u32(f)

                f.seek(shp1_offset + offset_data + prim_offset)
                bytes_read = 0
                while bytes_read < prim_size:
                    prim_type = read_u8(f)
                    bytes_read += 1
                    if prim_type == 0 or bytes_read >= prim_size:
                        break
                    vert_count = read_u16(f)
                    bytes_read += 2

                    key = f"0x{prim_type:02x}"
                    prim_type_counts[key] = prim_type_counts.get(key, 0) + 1
                    total_verts += vert_count

                    # Skip vertex data
                    bytes_per_vert = sum(2 if d == 3 else 1 for _, d in batch['attribs'])
                    skip = vert_count * bytes_per_vert
                    f.read(skip)
                    bytes_read += skip

            print(f"    Prim types: {prim_type_counts}, total verts: {total_verts}")

orig = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"
exported = r"C:\Users\ryana\Downloads\reconstruct_test.bmd"

check_prim_types(orig)
check_prim_types(exported)
