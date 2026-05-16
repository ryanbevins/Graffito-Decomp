"""Diagnose degenerate triangles and winding order in exported BMD."""
import struct
import math
import sys

def read_u8(f): return struct.unpack('>B', f.read(1))[0]
def read_u16(f): return struct.unpack('>H', f.read(2))[0]
def read_s16(f): return struct.unpack('>h', f.read(2))[0]
def read_u32(f): return struct.unpack('>I', f.read(4))[0]
def read_f32(f): return struct.unpack('>f', f.read(4))[0]

def find_section(f, tag):
    f.seek(0x20)  # skip BMD header (J3D file header is 0x20 bytes)
    file_size_saved = f.seek(0, 2)
    f.seek(0x20)
    while f.tell() < file_size_saved:
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

def parse_vtx1_positions(f):
    vtx1_offset, vtx1_size = find_section(f, b'VTX1')
    if vtx1_offset is None:
        print("ERROR: VTX1 section not found")
        return []

    f.seek(vtx1_offset + 8)  # skip tag + size
    array_format_offset = read_u32(f)
    offsets = [read_u32(f) for _ in range(13)]

    pos_offset = offsets[0]
    if pos_offset == 0:
        print("ERROR: no position data offset")
        return []

    # Read array format for positions
    f.seek(vtx1_offset + array_format_offset)
    af_array_type = read_u32(f)
    af_comp_count = read_u32(f)
    af_data_type = read_u32(f)
    af_decimal_point = read_u8(f)

    print(f"VTX1 positions: arrayType=0x{af_array_type:x}, compCount={af_comp_count}, dataType={af_data_type}, decimalPt={af_decimal_point}")

    # Calculate data length
    next_offset = None
    for i in range(1, 13):
        if offsets[i] != 0:
            next_offset = offsets[i]
            break
    if next_offset is None:
        next_offset = vtx1_size
    data_length = next_offset - pos_offset

    positions = []
    f.seek(vtx1_offset + pos_offset)
    if af_data_type == 4:  # f32
        count = data_length // 12
        for _ in range(count):
            x, y, z = read_f32(f), read_f32(f), read_f32(f)
            positions.append((x, y, z))
    elif af_data_type == 3:  # s16 fixed
        scale = 1.0 / (2 ** af_decimal_point)
        count = data_length // 6
        for _ in range(count):
            x, y, z = read_s16(f) * scale, read_s16(f) * scale, read_s16(f) * scale
            positions.append((x, y, z))

    print(f"  {len(positions)} positions loaded")
    if positions:
        print(f"  Position[0] = {positions[0]}")
    return positions

def parse_shp1_triangles(f):
    """Parse SHP1 using the actual header structure from blemd."""
    shp1_offset, shp1_size = find_section(f, b'SHP1')
    if shp1_offset is None:
        print("ERROR: SHP1 section not found")
        return []

    # Shp1Header: tag(4) + size(4) + batchCount(2) + pad(2) +
    #   offsetToBatches(4) + offsetUnknown(4) + zero(4) +
    #   offsetToBatchAttribs(4) + offsetToMatrixTable(4) +
    #   offsetData(4) + offsetToMatrixData(4) + offsetToPacketLocations(4)
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

    print(f"\nSHP1: {batch_count} batches")
    print(f"  offsetBatches=0x{offset_batches:x}, offsetAttribs=0x{offset_attribs:x}")
    print(f"  offsetMtxTable=0x{offset_mtx_table:x}, offsetData=0x{offset_data:x}")
    print(f"  offsetMtxData=0x{offset_mtx_data:x}, offsetPktLocs=0x{offset_pkt_locations:x}")

    # Read batch descriptors (each 40 bytes)
    batches = []
    for bi in range(batch_count):
        f.seek(shp1_offset + offset_batches + bi * 40)
        unknown = read_u16(f)  # mattype | 0xff
        pkt_count = read_u16(f)
        attrib_off = read_u16(f)  # relative to offsetToBatchAttribs
        first_mtx_data = read_u16(f)
        first_pkt_loc = read_u16(f)
        _unk3 = read_u16(f)
        _floats = [read_f32(f) for _ in range(7)]
        batches.append({
            'mattype': unknown & 0xff,
            'pkt_count': pkt_count,
            'attrib_off': attrib_off,
            'first_mtx_data': first_mtx_data,
            'first_pkt_loc': first_pkt_loc,
        })

    all_triangles = []

    for bi, batch in enumerate(batches):
        # Read attribs
        attribs = []
        f.seek(shp1_offset + offset_attribs + batch['attrib_off'])
        while True:
            attr = read_u32(f)
            dtype = read_u32(f)
            if attr == 0xFF:
                break
            attribs.append((attr, dtype))

        has_mtx = any(a[0] == 0 for a in attribs)
        has_pos = any(a[0] == 9 for a in attribs)

        print(f"\n  Batch {bi}: mattype=0x{batch['mattype']:02x}, pkts={batch['pkt_count']}, "
              f"attribs={[(hex(a), d) for a,d in attribs]}")

        for pi in range(batch['pkt_count']):
            # Read packet location
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

                vertices = []
                for _ in range(vert_count):
                    vert = {}
                    for attr, dtype in attribs:
                        if dtype == 1:  # u8
                            val = read_u8(f)
                            bytes_read += 1
                        elif dtype == 3:  # u16
                            val = read_u16(f)
                            bytes_read += 2
                        else:
                            val = 0
                        vert[attr] = val
                    vertices.append(vert)

                # Extract triangles
                if prim_type == 0x90:  # GX_TRIANGLES
                    for t in range(0, len(vertices) - 2, 3):
                        tri = (vertices[t], vertices[t+1], vertices[t+2])
                        all_triangles.append((bi, pi, prim_type, tri))
                elif prim_type == 0x98:  # GX_TRIANGLESTRIP
                    for t in range(len(vertices) - 2):
                        if t % 2 == 0:
                            tri = (vertices[t], vertices[t+1], vertices[t+2])
                        else:
                            tri = (vertices[t+1], vertices[t], vertices[t+2])
                        all_triangles.append((bi, pi, prim_type, tri))

        print(f"    Total tris from batch {bi}: {sum(1 for b,_,_,_ in all_triangles if b == bi)}")

    return all_triangles

def dist(p1, p2):
    return math.sqrt(sum((a-b)**2 for a,b in zip(p1, p2)))

def main():
    bmd_path = sys.argv[1] if len(sys.argv) > 1 else r"C:\Users\ryana\Downloads\reconstruct_test.bmd"

    print(f"Analyzing: {bmd_path}\n")

    with open(bmd_path, 'rb') as f:
        positions = parse_vtx1_positions(f)
        triangles = parse_shp1_triangles(f)

    print(f"\n{'='*60}")
    print(f"Total triangles: {len(triangles)}")

    # Find degenerate triangles (edge > 50 units)
    THRESHOLD = 50.0
    degen_count = 0
    zero_idx_tris = 0

    print(f"\n=== Degenerate triangles (max edge > {THRESHOLD}) ===")
    for bi, pi, ptype, tri in triangles:
        pos_indices = [v.get(9, 0) for v in tri]

        # Get positions
        try:
            p = [positions[idx] for idx in pos_indices]
        except IndexError:
            print(f"  Batch {bi} pkt {pi}: posIndex OUT OF RANGE: {pos_indices} (max={len(positions)-1})")
            degen_count += 1
            continue

        e01 = dist(p[0], p[1])
        e12 = dist(p[1], p[2])
        e20 = dist(p[2], p[0])
        max_edge = max(e01, e12, e20)

        if max_edge > THRESHOLD:
            degen_count += 1
            if degen_count <= 20:  # limit output
                print(f"  Batch {bi} pkt {pi} type=0x{ptype:02x}:")
                print(f"    posIndices: {pos_indices}")
                print(f"    positions: {[positions[i] for i in pos_indices]}")
                print(f"    edges: {e01:.1f}, {e12:.1f}, {e20:.1f}  (max={max_edge:.1f})")

    if degen_count > 20:
        print(f"  ... and {degen_count - 20} more")

    # Count triangles referencing posIndex=0
    for bi, pi, ptype, tri in triangles:
        pos_indices = [v.get(9, 0) for v in tri]
        if 0 in pos_indices:
            zero_idx_tris += 1

    print(f"\nSummary:")
    print(f"  Degenerate triangles (edge > {THRESHOLD}): {degen_count}")
    print(f"  Triangles with posIndex=0: {zero_idx_tris}")
    if positions:
        print(f"  Position[0] = {positions[0]}")

    # Also compare with original
    orig_path = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"
    try:
        with open(orig_path, 'rb') as f:
            orig_positions = parse_vtx1_positions(f)
            orig_triangles = parse_shp1_triangles(f)

        print(f"\n{'='*60}")
        print(f"ORIGINAL BMD comparison:")
        print(f"  Positions: {len(orig_positions)} vs exported {len(positions)}")
        print(f"  Triangles: {len(orig_triangles)} vs exported {len(triangles)}")

        orig_degen = 0
        for bi, pi, ptype, tri in orig_triangles:
            pos_indices = [v.get(9, 0) for v in tri]
            try:
                p = [orig_positions[idx] for idx in pos_indices]
            except IndexError:
                orig_degen += 1
                continue
            max_edge = max(dist(p[0], p[1]), dist(p[1], p[2]), dist(p[2], p[0]))
            if max_edge > THRESHOLD:
                orig_degen += 1
        print(f"  Degenerate triangles in original: {orig_degen}")
    except FileNotFoundError:
        print(f"\nOriginal BMD not found at: {orig_path}")

if __name__ == '__main__':
    main()
