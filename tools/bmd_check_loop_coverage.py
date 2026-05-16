"""Check if all loop indices used by loop_triangles are covered by the loop_indices dict."""
import sys
sys.path.insert(0, r"C:\Users\ryana\AppData\Roaming\Blender Foundation\Blender\5.0\scripts\addons")

# This must run inside Blender. Let's instead check the exported BMD for
# vertices with posIndex=0 that look suspicious.
import struct, math

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

def get_all_pos_indices(f, path_label):
    """Get histogram of posIndex usage from all primitives."""
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

    pos_index_count = {}
    for bi in range(batch_count):
        f.seek(shp1_offset + offset_batches + bi * 40)
        unknown = read_u16(f)
        pkt_count = read_u16(f)
        attrib_off = read_u16(f)
        first_mtx_data = read_u16(f)
        first_pkt_loc = read_u16(f)
        _unk3 = read_u16(f)
        _floats = [read_f32(f) for _ in range(7)]

        attribs = []
        f.seek(shp1_offset + offset_attribs + attrib_off)
        while True:
            attr = read_u32(f)
            dtype = read_u32(f)
            if attr == 0xFF:
                break
            attribs.append((attr, dtype))

        for pi in range(pkt_count):
            pkt_loc_idx = first_pkt_loc + pi
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
                for _ in range(vert_count):
                    vert = {}
                    for attr, dtype in attribs:
                        if dtype == 1:
                            val = read_u8(f)
                            bytes_read += 1
                        elif dtype == 3:
                            val = read_u16(f)
                            bytes_read += 2
                        else:
                            val = 0
                        vert[attr] = val
                    pos_idx = vert.get(9, -1)
                    pos_index_count[pos_idx] = pos_index_count.get(pos_idx, 0) + 1

    return pos_index_count

for path, label in [
    (r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd", "ORIGINAL"),
    (r"C:\Users\ryana\Downloads\reconstruct_test.bmd", "EXPORTED"),
]:
    print(f"\n=== {label} ===")
    with open(path, 'rb') as f:
        counts = get_all_pos_indices(f, label)

    total_refs = sum(counts.values())
    unique_indices = len(counts)
    max_idx = max(counts.keys())
    print(f"  Total vertex refs: {total_refs}")
    print(f"  Unique posIndices: {unique_indices}")
    print(f"  Max posIndex: {max_idx}")
    print(f"  posIndex=0 count: {counts.get(0, 0)}")

    # Check for any unreferenced indices
    all_used = set(counts.keys())
    missing = [i for i in range(max_idx + 1) if i not in all_used]
    if missing:
        print(f"  Unreferenced posIndices: {missing[:20]}{'...' if len(missing) > 20 else ''}")
    else:
        print(f"  All posIndices 0..{max_idx} referenced")
