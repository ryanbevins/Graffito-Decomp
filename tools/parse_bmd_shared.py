"""Parse original BMD to find shared posIndices between rigid and weighted batches."""
import struct

BMD_PATH = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"

def read_u8(data, off): return struct.unpack_from('>B', data, off)[0]
def read_u16(data, off): return struct.unpack_from('>H', data, off)[0]
def read_s16(data, off): return struct.unpack_from('>h', data, off)[0]
def read_u32(data, off): return struct.unpack_from('>I', data, off)[0]
def read_f32(data, off): return struct.unpack_from('>f', data, off)[0]

with open(BMD_PATH, 'rb') as f:
    data = f.read()

def find_section(tag):
    idx = data.find(tag)
    if idx < 0: raise ValueError(f"Section {tag} not found")
    return idx

shp1_off = find_section(b'SHP1')
vtx1_off = find_section(b'VTX1')
drw1_off = find_section(b'DRW1')
evp1_off = find_section(b'EVP1')
jnt1_off = find_section(b'JNT1')

# ---- DRW1 ----
drw1_count = read_u16(data, drw1_off + 8)
drw1_type_off = drw1_off + read_u32(data, drw1_off + 0x0C)
drw1_data_off = drw1_off + read_u32(data, drw1_off + 0x10)
drw1_entries = []
for i in range(drw1_count):
    is_weighted = read_u8(data, drw1_type_off + i)
    idx = read_u16(data, drw1_data_off + i * 2)
    drw1_entries.append((is_weighted, idx))

# ---- EVP1 ----
evp1_count = read_u16(data, evp1_off + 8)
evp1_counts_off = evp1_off + read_u32(data, evp1_off + 0x0C)
evp1_indices_off = evp1_off + read_u32(data, evp1_off + 0x10)
evp1_weights_off = evp1_off + read_u32(data, evp1_off + 0x14)
evp1_matrices_off = evp1_off + read_u32(data, evp1_off + 0x18)

envelopes = []
env_idx_cursor = 0
for i in range(evp1_count):
    cnt = read_u8(data, evp1_counts_off + i)
    bones = []
    weights = []
    for j in range(cnt):
        bone = read_u16(data, evp1_indices_off + (env_idx_cursor + j) * 2)
        weight = read_f32(data, evp1_weights_off + (env_idx_cursor + j) * 4)
        bones.append(bone)
        weights.append(weight)
    env_idx_cursor += cnt
    envelopes.append((bones, weights))

# ---- JNT1 ----
jnt1_count = read_u16(data, jnt1_off + 8)
jnt1_data_off = jnt1_off + read_u32(data, jnt1_off + 0x0C)

# ---- VTX1 positions ----
vtx1_fmt_off = vtx1_off + read_u32(data, vtx1_off + 0x08)
vtx1_data_offsets = [read_u32(data, vtx1_off + 0x0C + i * 4) for i in range(13)]

fmt_cursor = vtx1_fmt_off
pos_comp_type = pos_frac_bits = None
while True:
    attr = read_u32(data, fmt_cursor)
    if attr == 0xFF: break
    if attr == 9:  # GX_VA_POS
        pos_comp_type = read_u32(data, fmt_cursor + 8)
        pos_frac_bits = read_u8(data, fmt_cursor + 12)
    fmt_cursor += 16

pos_data_off = vtx1_off + vtx1_data_offsets[0]

def read_position(idx):
    if pos_comp_type == 3:  # s16
        off = pos_data_off + idx * 6
        x = read_s16(data, off) / (1 << pos_frac_bits)
        y = read_s16(data, off + 2) / (1 << pos_frac_bits)
        z = read_s16(data, off + 4) / (1 << pos_frac_bits)
        return (x, y, z)
    elif pos_comp_type == 4:  # f32
        off = pos_data_off + idx * 12
        return (read_f32(data, off), read_f32(data, off + 4), read_f32(data, off + 8))

# ---- SHP1 ----
shp1_batch_count = read_u16(data, shp1_off + 8)
shp1_batches_off = shp1_off + read_u32(data, shp1_off + 0x0C)
shp1_attribs_off = shp1_off + read_u32(data, shp1_off + 0x18)
shp1_mtx_table_off = shp1_off + read_u32(data, shp1_off + 0x1C)
shp1_prim_data_off = shp1_off + read_u32(data, shp1_off + 0x20)
shp1_mtx_init_off = shp1_off + read_u32(data, shp1_off + 0x24)
shp1_draw_init_off = shp1_off + read_u32(data, shp1_off + 0x28)

# Max mtx table entries (based on gap between mtx_table and prim_data)
max_mtx_table_entries = (shp1_prim_data_off - shp1_mtx_table_off) // 2

batch_info = []
for i in range(shp1_batch_count):
    boff = shp1_batches_off + i * 0x28
    mat_type = read_u8(data, boff)
    packet_count = read_u16(data, boff + 2)
    attrib_offset = read_u16(data, boff + 4)
    first_mtx_data = read_u16(data, boff + 6)
    first_packet_loc = read_u16(data, boff + 8)

    attribs = []
    a_off = shp1_attribs_off + attrib_offset
    while True:
        attr = read_u32(data, a_off)
        if attr == 0xFF: break
        data_type = read_u32(data, a_off + 4)
        attribs.append((attr, data_type))
        a_off += 8

    has_pnmtxidx = any(a[0] == 0 for a in attribs)
    batch_info.append({
        'matType': mat_type, 'packetCount': packet_count,
        'attribOffset': attrib_offset, 'firstMtxData': first_mtx_data,
        'firstPacketLoc': first_packet_loc, 'attribs': attribs,
        'hasPNMTXIDX': has_pnmtxidx,
    })
    attr_names = {0:'PNMTXIDX',9:'POS',10:'NRM',11:'CLR0',13:'TEX0',14:'TEX1',15:'TEX2'}
    attr_str = ', '.join(attr_names.get(a[0], f'attr{a[0]}') for a in attribs)
    print(f"Batch[{i}]: matType={mat_type}, packets={packet_count}, attribs=[{attr_str}], hasPNMTXIDX={has_pnmtxidx}")

# Parse primitives
batch_pos_indices = []
batch_vertex_details = []  # (pkt, pnmtxidx, posIdx, drw_idx)

for bi, batch in enumerate(batch_info):
    pos_indices = set()
    vertex_details = []
    attr_list = batch['attribs']

    for pi in range(batch['packetCount']):
        # Draw init: (u32 size, u32 offset)
        pkt_loc_off = shp1_draw_init_off + (batch['firstPacketLoc'] + pi) * 8
        prim_size = read_u32(data, pkt_loc_off)
        prim_offset = read_u32(data, pkt_loc_off + 4)
        prim_base = shp1_prim_data_off + prim_offset
        prim_end = prim_base + prim_size

        # Matrix init: (u16 useMtxCount, u16 unknown, u32 firstIndex)
        mtx_data_off = shp1_mtx_init_off + (batch['firstMtxData'] + pi) * 8
        mtx_use_count = read_u16(data, mtx_data_off)
        mtx_first_idx = read_u32(data, mtx_data_off + 4)

        mtx_table_entries = []
        if mtx_use_count != 0xFFFF:
            for mi in range(mtx_use_count):
                idx = mtx_first_idx + mi
                if idx < max_mtx_table_entries:
                    mtx_idx = read_u16(data, shp1_mtx_table_off + idx * 2)
                    mtx_table_entries.append(mtx_idx)

        cursor = prim_base
        while cursor < prim_end:
            prim_type = read_u8(data, cursor)
            if prim_type == 0: break
            vtx_count = read_u16(data, cursor + 1)
            cursor += 3

            for vi in range(vtx_count):
                pnmtxidx = None
                posIdx = None
                for attr, dtype in attr_list:
                    if attr == 0:  # PNMTXIDX
                        pnmtxidx = read_u8(data, cursor)
                        cursor += 1
                    elif dtype == 3:  # INDEX16
                        val = read_u16(data, cursor)
                        if attr == 9: posIdx = val
                        cursor += 2
                    elif dtype == 2:  # INDEX8
                        val = read_u8(data, cursor)
                        if attr == 9: posIdx = val
                        cursor += 1
                    elif dtype == 1:  # GX_DIRECT
                        cursor += 1
                    else:
                        cursor += 2

                if posIdx is not None:
                    pos_indices.add(posIdx)
                    drw_idx = None
                    if batch['hasPNMTXIDX'] and pnmtxidx is not None:
                        mtx_slot = pnmtxidx // 3
                        if mtx_slot < len(mtx_table_entries):
                            drw_idx = mtx_table_entries[mtx_slot]
                    elif not batch['hasPNMTXIDX'] and mtx_table_entries:
                        drw_idx = mtx_table_entries[0]
                    vertex_details.append((pi, pnmtxidx, posIdx, drw_idx))

    batch_pos_indices.append(pos_indices)
    batch_vertex_details.append(vertex_details)
    if pos_indices:
        print(f"  -> {len(pos_indices)} unique posIndices, range [{min(pos_indices)}-{max(pos_indices)}]")
    else:
        print(f"  -> 0 posIndices (empty)")

# Find shared indices
rigid_indices = set()
for i in range(9): rigid_indices |= batch_pos_indices[i]
weighted_indices = set()
for i in range(9, shp1_batch_count): weighted_indices |= batch_pos_indices[i]
shared = rigid_indices & weighted_indices

print(f"\n{'='*60}")
print(f"SHARED POSITION ANALYSIS")
print(f"{'='*60}")
print(f"Rigid batches (0-8): {len(rigid_indices)} unique posIndices")
print(f"Weighted batches (9-{shp1_batch_count-1}): {len(weighted_indices)} unique posIndices")
print(f"Shared posIndices: {len(shared)}")
if shared:
    print(f"Shared indices: {sorted(shared)}")

# For each shared index, show position and DRW1 usage
if shared:
    print(f"\n{'='*60}")
    print(f"SHARED VERTEX DETAILS")
    print(f"{'='*60}")
    for pidx in sorted(shared):
        pos = read_position(pidx)
        print(f"\nPosIndex {pidx}: ({pos[0]:.4f}, {pos[1]:.4f}, {pos[2]:.4f})")

        for bi in range(shp1_batch_count):
            seen = False
            for (pkt, pnmtx, posI, drw_idx) in batch_vertex_details[bi]:
                if posI == pidx and not seen:
                    seen = True
                    batch_type = "RIGID" if bi < 9 else "WEIGHTED"
                    drw_info = "no drw"
                    if drw_idx is not None and drw_idx < len(drw1_entries) and drw_idx != 0xFFFF:
                        is_w, d_idx = drw1_entries[drw_idx]
                        if is_w:
                            drw_info = f"DRW1[{drw_idx}]: envelope={d_idx}, bones={envelopes[d_idx][0]}, weights={[f'{w:.2f}' for w in envelopes[d_idx][1]]}"
                        else:
                            drw_info = f"DRW1[{drw_idx}]: rigid joint={d_idx}"
                    print(f"  Batch[{bi}] ({batch_type}): pnmtxidx={pnmtx}, {drw_info}")

# Y-value analysis
print(f"\n{'='*60}")
print(f"Y-VALUE ANALYSIS")
print(f"{'='*60}")
rigid_y = [read_position(idx)[1] for idx in rigid_indices]
weighted_y = [read_position(idx)[1] for idx in weighted_indices]
print(f"Rigid Y values: min={min(rigid_y):.4f}, max={max(rigid_y):.4f}")
print(f"Weighted Y values: min={min(weighted_y):.4f}, max={max(weighted_y):.4f}")
if shared:
    shared_y = [read_position(idx)[1] for idx in shared]
    print(f"Shared vertex Y values: min={min(shared_y):.4f}, max={max(shared_y):.4f}")

# Interpretation
print(f"\n{'='*60}")
print(f"INTERPRETATION")
print(f"{'='*60}")
# Joint 1 translate Y = 58.0 (spine). If positions are bone-local, Y would be small.
# If world-space, Y would be ~58+ for upper body.
print("Joint[1] (spine) translate Y = 58.0")
print("If positions are in bone-local space: Y values relative to bone (~small)")
print("If positions are in world-space: Y values include bone offset (~50-100 for upper body)")
if rigid_y:
    print(f"\nRigid vertex Y range: {min(rigid_y):.2f} to {max(rigid_y):.2f}")
if weighted_y:
    print(f"Weighted vertex Y range: {min(weighted_y):.2f} to {max(weighted_y):.2f}")

# Sample some rigid vertices and their DRW1 joints
print(f"\n{'='*60}")
print(f"SAMPLE RIGID VERTICES WITH JOINTS")
print(f"{'='*60}")
for bi in range(min(3, 9)):
    examples = [(pkt, pnmtx, posI, drw) for (pkt, pnmtx, posI, drw) in batch_vertex_details[bi]][:3]
    for pkt, pnmtx, posI, drw_idx in examples:
        pos = read_position(posI)
        if drw_idx is not None and drw_idx < len(drw1_entries) and drw_idx != 0xFFFF:
            is_w, d_idx = drw1_entries[drw_idx]
            if not is_w:
                joff = jnt1_data_off + d_idx * 0x40
                jy = read_f32(data, joff + 0x1C)
                print(f"  Batch[{bi}] pos[{posI}]=({pos[0]:.2f},{pos[1]:.2f},{pos[2]:.2f}) -> joint[{d_idx}] (transY={jy:.2f})")

# Sample weighted vertices
print(f"\n{'='*60}")
print(f"SAMPLE WEIGHTED VERTICES WITH ENVELOPES")
print(f"{'='*60}")
for bi in range(9, shp1_batch_count):
    examples = [(pkt, pnmtx, posI, drw) for (pkt, pnmtx, posI, drw) in batch_vertex_details[bi]][:5]
    for pkt, pnmtx, posI, drw_idx in examples:
        pos = read_position(posI)
        if drw_idx is not None and drw_idx < len(drw1_entries) and drw_idx != 0xFFFF:
            is_w, d_idx = drw1_entries[drw_idx]
            if is_w:
                bones, weights = envelopes[d_idx]
                print(f"  Batch[{bi}] pos[{posI}]=({pos[0]:.2f},{pos[1]:.2f},{pos[2]:.2f}) -> envelope[{d_idx}] bones={bones} weights={[f'{w:.2f}' for w in weights]}")
            else:
                print(f"  Batch[{bi}] pos[{posI}]=({pos[0]:.2f},{pos[1]:.2f},{pos[2]:.2f}) -> rigid joint={d_idx}")
