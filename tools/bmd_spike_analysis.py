#!/usr/bin/env python3
"""
BMD Spike Vertex Analysis
=========================
Parses both original and exported BMDs, computes runtime vertex positions
for batch 9, and finds vertices where the export has wrong DRW entries
causing star-shaped spikes.

Also traces the Blender addon's _get_vert_drw_index_weighted() logic
to explain WHY the wrong DRW entry was chosen.
"""

import struct
import math
import sys
from collections import defaultdict

# ============================================================
# BMD Binary Reader
# ============================================================
class BinaryReader:
    def __init__(self, data):
        self.data = data
        self.pos = 0

    def seek(self, pos):
        self.pos = pos

    def tell(self):
        return self.pos

    def read_u8(self):
        v = self.data[self.pos]
        self.pos += 1
        return v

    def read_s16(self):
        v = struct.unpack_from('>h', self.data, self.pos)[0]
        self.pos += 2
        return v

    def read_u16(self):
        v = struct.unpack_from('>H', self.data, self.pos)[0]
        self.pos += 2
        return v

    def read_u32(self):
        v = struct.unpack_from('>I', self.data, self.pos)[0]
        self.pos += 4
        return v

    def read_f32(self):
        v = struct.unpack_from('>f', self.data, self.pos)[0]
        self.pos += 4
        return v

    def read_string(self, n):
        s = self.data[self.pos:self.pos+n]
        self.pos += n
        return s.decode('ascii', errors='replace')


# ============================================================
# Matrix math (pure Python, no numpy needed)
# ============================================================
def mat4_identity():
    return [[1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]]

def mat4_zero():
    return [[0,0,0,0],[0,0,0,0],[0,0,0,0],[0,0,0,0]]

def mat4_mul(a, b):
    r = mat4_zero()
    for i in range(4):
        for j in range(4):
            for k in range(4):
                r[i][j] += a[i][k] * b[k][j]
    return r

def mat4_mul_vec3(m, v):
    """Multiply 4x4 matrix by 3D point (w=1), return (x,y,z)."""
    x = m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2] + m[0][3]
    y = m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2] + m[1][3]
    z = m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2] + m[2][3]
    return (x, y, z)

def mat4_translation(tx, ty, tz):
    m = mat4_identity()
    m[0][3] = tx
    m[1][3] = ty
    m[2][3] = tz
    return m

def mat4_rot_x(angle):
    c, s = math.cos(angle), math.sin(angle)
    m = mat4_identity()
    m[1][1] = c; m[1][2] = -s
    m[2][1] = s; m[2][2] = c
    return m

def mat4_rot_y(angle):
    c, s = math.cos(angle), math.sin(angle)
    m = mat4_identity()
    m[0][0] = c; m[0][2] = s
    m[2][0] = -s; m[2][2] = c
    return m

def mat4_rot_z(angle):
    c, s = math.cos(angle), math.sin(angle)
    m = mat4_identity()
    m[0][0] = c; m[0][1] = -s
    m[1][0] = s; m[1][1] = c
    return m

def mat4_scale(sx, sy, sz):
    m = mat4_identity()
    m[0][0] = sx; m[1][1] = sy; m[2][2] = sz
    return m

def mat4_mad(r, m, weight):
    """r += weight * m (for 3x4 portion)"""
    for i in range(3):
        for j in range(4):
            r[i][j] += weight * m[i][j]

def vec3_dist(a, b):
    return math.sqrt((a[0]-b[0])**2 + (a[1]-b[1])**2 + (a[2]-b[2])**2)


# ============================================================
# BMD Section Parsers
# ============================================================

def find_sections(data):
    """Find all section offsets in a BMD/BDL file."""
    br = BinaryReader(data)
    tag = br.read_string(4)  # J3D2
    subtype = br.read_string(4)  # bmd3 or bdl4
    file_size = br.read_u32()
    num_sections = br.read_u32()
    # Skip SVR3 subheader (16 bytes padding)
    br.seek(0x20)

    sections = {}
    pos = br.tell()
    for _ in range(num_sections):
        br.seek(pos)
        stag = br.read_string(4)
        ssize = br.read_u32()
        sections[stag] = (pos, ssize)
        pos += ssize
    return sections


def parse_inf1(data, offset, size):
    """Parse INF1 to get scene graph (for bone hierarchy)."""
    br = BinaryReader(data)
    br.seek(offset)
    tag = br.read_string(4)
    section_size = br.read_u32()
    unknown1 = br.read_u16()
    pad = br.read_u16()
    packet_count = br.read_u32()
    vertex_count = br.read_u32()
    entries_offset = br.read_u32()

    entries = []
    br.seek(offset + entries_offset)
    while True:
        etype = br.read_u16()
        eindex = br.read_u16()
        if etype == 0:
            break
        entries.append((etype, eindex))

    return entries


def build_bone_hierarchy(inf1_entries):
    """Build parent map from INF1 entries. Returns parent[bone_idx] = parent_bone_idx."""
    parent = {}
    stack = []
    current_joint = -1

    for etype, eindex in inf1_entries:
        if etype == 0x10:  # Joint
            if stack:
                parent[eindex] = stack[-1]
            else:
                parent[eindex] = -1  # root
            current_joint = eindex
        elif etype == 0x01:  # hierarchy down
            stack.append(current_joint)
        elif etype == 0x02:  # hierarchy up
            if stack:
                current_joint = stack.pop()

    return parent


def parse_vtx1(data, offset, size):
    """Parse VTX1 to get vertex positions."""
    br = BinaryReader(data)
    br.seek(offset)
    tag = br.read_string(4)
    section_size = br.read_u32()
    array_format_offset = br.read_u32()
    offsets = []
    for _ in range(13):
        offsets.append(br.read_u32())

    # Read array formats
    br.seek(offset + array_format_offset)
    num_arrays = sum(1 for o in offsets if o != 0)
    formats = []
    for _ in range(num_arrays):
        array_type = br.read_u32()
        comp_count = br.read_u32()
        data_type = br.read_u32()
        decimal_point = br.read_u8()
        unk3 = br.read_u8()
        unk4 = br.read_u16()
        formats.append((array_type, comp_count, data_type, decimal_point))

    # Find position array (arrayType=9)
    positions = []
    fmt_idx = 0
    for i in range(13):
        if offsets[i] != 0:
            af = formats[fmt_idx]
            fmt_idx += 1
            if af[0] == 9:  # positions
                pos_offset = offset + offsets[i]
                # Determine length
                end_offset = section_size
                for j in range(i+1, 13):
                    if offsets[j] != 0:
                        end_offset = offsets[j]
                        break
                length = end_offset - offsets[i]

                br.seek(pos_offset)
                if af[2] == 4:  # f32
                    count = length // 12  # 3 floats * 4 bytes
                    for _ in range(count):
                        x = br.read_f32()
                        y = br.read_f32()
                        z = br.read_f32()
                        positions.append((x, y, z))
                elif af[2] == 3:  # s16
                    scale = 1.0 / (2 ** af[3])
                    count = length // 6
                    for _ in range(count):
                        x = br.read_s16() * scale
                        y = br.read_s16() * scale
                        z = br.read_s16() * scale
                        positions.append((x, y, z))
                break
    return positions


def parse_drw1(data, offset, size):
    """Parse DRW1 section."""
    br = BinaryReader(data)
    br.seek(offset)
    tag = br.read_string(4)
    section_size = br.read_u32()
    count = br.read_u16()
    pad = br.read_u16()
    offset_isweighted = br.read_u32()
    offset_data = br.read_u32()

    is_weighted = []
    br.seek(offset + offset_isweighted)
    for _ in range(count):
        is_weighted.append(br.read_u8())

    drw_data = []
    br.seek(offset + offset_data)
    for _ in range(count):
        drw_data.append(br.read_u16())

    return is_weighted, drw_data


def parse_evp1(data, offset, size):
    """Parse EVP1 section. Returns (weightedIndices, inverseBind matrices)."""
    br = BinaryReader(data)
    br.seek(offset)
    tag = br.read_string(4)
    section_size = br.read_u32()
    count = br.read_u16()
    pad = br.read_u16()
    off0 = br.read_u32()  # counts
    off1 = br.read_u32()  # indices
    off2 = br.read_u32()  # weights
    off3 = br.read_u32()  # inverse bind matrices

    # Read counts
    br.seek(offset + off0)
    counts = []
    num_matrices = 0
    for i in range(count):
        c = br.read_u8()
        counts.append(c)

    # Read indices
    br.seek(offset + off1)
    weighted_indices = []
    for i in range(count):
        indices = []
        for j in range(counts[i]):
            idx = br.read_u16()
            indices.append(idx)
            num_matrices = max(num_matrices, idx + 1)
        weighted_indices.append(indices)

    # Read weights
    br.seek(offset + off2)
    weights = []
    for i in range(count):
        w = []
        for j in range(counts[i]):
            w.append(br.read_f32())
        weights.append(w)

    # Read inverse bind matrices (3x4)
    br.seek(offset + off3)
    inv_bind = []
    for i in range(num_matrices):
        m = mat4_identity()
        for row in range(3):
            for col in range(4):
                m[row][col] = br.read_f32()
        m[3] = [0, 0, 0, 1]
        inv_bind.append(m)

    return weighted_indices, weights, inv_bind


def parse_jnt1(data, offset, size):
    """Parse JNT1 section. Returns list of (name, sx,sy,sz, rx,ry,rz, tx,ty,tz)."""
    br = BinaryReader(data)
    br.seek(offset)
    tag = br.read_string(4)
    section_size = br.read_u32()
    count = br.read_u16()
    pad = br.read_u16()
    jnt_entry_offset = br.read_u32()
    unknown_offset = br.read_u32()
    string_table_offset = br.read_u32()

    # Read string table
    br.seek(offset + string_table_offset)
    num_strings = br.read_u16()
    str_pad = br.read_u16()
    string_entries = []
    for _ in range(num_strings):
        hash_val = br.read_u16()
        str_off = br.read_u16()
        string_entries.append(str_off)

    names = []
    str_base = offset + string_table_offset + 4 + num_strings * 4
    for soff in string_entries:
        br.seek(str_base + soff)
        name = b''
        while True:
            c = br.read_u8()
            if c == 0:
                break
            name += bytes([c])
        names.append(name.decode('ascii', errors='replace'))

    # Read joint entries
    joints = []
    br.seek(offset + jnt_entry_offset)
    for i in range(count):
        unknown = br.read_u16()
        jpad = br.read_u16()
        sx = br.read_f32()
        sy = br.read_f32()
        sz = br.read_f32()
        rx = br.read_s16()
        ry = br.read_s16()
        rz = br.read_s16()
        pad2 = br.read_u16()
        tx = br.read_f32()
        ty = br.read_f32()
        tz = br.read_f32()
        unknown2 = br.read_f32()
        bb_min = [br.read_f32() for _ in range(3)]
        bb_max = [br.read_f32() for _ in range(3)]

        name = names[i] if i < len(names) else f"joint_{i}"
        joints.append({
            'name': name,
            'sx': sx, 'sy': sy, 'sz': sz,
            'rx': rx * math.pi / 32768.0,
            'ry': ry * math.pi / 32768.0,
            'rz': rz * math.pi / 32768.0,
            'tx': tx, 'ty': ty, 'tz': tz,
        })

    return joints


def parse_shp1_batch(data, offset, size, batch_index=9):
    """Parse SHP1 to get batch data for a specific batch index.

    SHP1 header layout (44 bytes):
      tag(4) + sizeOfSection(4) + batchCount(2) + pad(2)
      + offsetToBatches(4) + offsetUnknown(4) + zero(4)
      + offsetToBatchAttribs(4) + offsetToMatrixTable(4)
      + offsetData(4) + offsetToMatrixData(4) + offsetToPacketLocations(4)
    """
    br = BinaryReader(data)
    br.seek(offset)
    tag = br.read_string(4)  # SHP1
    section_size = br.read_u32()
    batch_count = br.read_u16()
    pad = br.read_u16()
    offsetToBatches = br.read_u32()
    offsetUnknown = br.read_u32()
    zero = br.read_u32()
    offsetToBatchAttribs = br.read_u32()
    offsetToMatrixTable = br.read_u32()
    offsetData = br.read_u32()
    offsetToMatrixData = br.read_u32()
    offsetToPacketLocations = br.read_u32()

    if batch_index >= batch_count:
        print(f"ERROR: batch {batch_index} >= batch_count {batch_count}")
        return None

    # Read batch descriptor (40 bytes each)
    br.seek(offset + offsetToBatches + batch_index * 40)
    b_matrix_type = br.read_u16()  # 2 bytes: unknown/matrix type
    b_packet_count = br.read_u16()
    b_attrib_offset = br.read_u16()  # relative to offsetToBatchAttribs
    b_first_matrix_data = br.read_u16()
    b_first_packet_location = br.read_u16()
    b_pad = br.read_u16()  # 0xffff
    b_unknown_floats = [br.read_f32() for _ in range(7)]

    print(f"  Batch {batch_index}: matrixType={b_matrix_type}, packetCount={b_packet_count}, attribOff={b_attrib_offset}")

    # Read attributes for this batch from the attrib table
    br.seek(offset + offsetToBatchAttribs + b_attrib_offset)
    attribs = []
    while True:
        attr_type = br.read_u32()
        attr_data_type = br.read_u32()
        if attr_type == 0xFF:
            break
        attribs.append((attr_type, attr_data_type))

    print(f"  Attributes: {[(hex(a), d) for a, d in attribs]}")

    # Read packets
    all_vertices = []

    # Maintain a running matrix table that persists across packets
    # (0xffff means "keep previous value")
    running_matrix_table = []

    for pkt_i in range(b_packet_count):
        # Read packet location
        br.seek(offset + offsetToPacketLocations + (b_first_packet_location + pkt_i) * 8)
        pkt_data_size = br.read_u32()
        pkt_data_offset = br.read_u32()

        # Read matrix data for this packet
        br.seek(offset + offsetToMatrixData + (b_first_matrix_data + pkt_i) * 8)
        md_unknown1 = br.read_u16()
        md_count = br.read_u16()
        md_first_index = br.read_u32()

        # Read matrix table entries for this packet
        br.seek(offset + offsetToMatrixTable + md_first_index * 2)
        new_entries = []
        for _ in range(md_count):
            new_entries.append(br.read_u16())

        # Extend running table if needed
        while len(running_matrix_table) < len(new_entries):
            running_matrix_table.append(0xFFFF)

        # Update running table (0xffff = keep old value)
        for n in range(len(new_entries)):
            if new_entries[n] != 0xFFFF:
                running_matrix_table[n] = new_entries[n]

        # Read primitive data
        prim_start = offset + offsetData + pkt_data_offset
        br.seek(prim_start)
        prim_end = prim_start + pkt_data_size

        while br.tell() < prim_end:
            prim_type = br.read_u8()
            if prim_type == 0:
                break

            num_verts = br.read_u16()

            for vi in range(num_verts):
                matrix_index = None
                pos_index = None

                for atype, adtype in attribs:
                    if adtype == 1:  # u8
                        val = br.read_u8()
                    elif adtype == 3:  # u16
                        val = br.read_u16()
                    else:
                        continue

                    if atype == 0:  # PNMTXIDX
                        matrix_index = val
                    elif atype == 9:  # POS
                        pos_index = val

                # Convert matrixIndex to DRW1 index via matrix table
                if matrix_index is not None:
                    slot = matrix_index // 3
                    if slot < len(running_matrix_table):
                        drw_index = running_matrix_table[slot]
                    else:
                        drw_index = 0xFFFF
                else:
                    drw_index = running_matrix_table[0] if running_matrix_table else 0xFFFF

                all_vertices.append({
                    'matrixIndex': matrix_index,
                    'posIndex': pos_index,
                    'drwIndex': drw_index,
                    'packet': pkt_i,
                    'matrixTable': list(running_matrix_table),
                })

    return all_vertices


# ============================================================
# Compute bone world matrices
# ============================================================

def compute_bone_world_matrices(joints, parent_map):
    """Compute world matrix for each bone."""
    num_bones = len(joints)
    world_matrices = [None] * num_bones

    def get_local_matrix(j):
        """Translation @ RotX @ RotY @ RotZ"""
        t = mat4_translation(j['tx'], j['ty'], j['tz'])
        rx = mat4_rot_x(j['rx'])
        ry = mat4_rot_y(j['ry'])
        rz = mat4_rot_z(j['rz'])
        # XYZ euler: Rx @ Ry @ Rz
        rot = mat4_mul(mat4_mul(rx, ry), rz)
        return mat4_mul(t, rot)

    def compute_world(bone_idx):
        if world_matrices[bone_idx] is not None:
            return world_matrices[bone_idx]

        local = get_local_matrix(joints[bone_idx])
        parent_idx = parent_map.get(bone_idx, -1)

        if parent_idx < 0:
            world_matrices[bone_idx] = local
        else:
            parent_world = compute_world(parent_idx)
            world_matrices[bone_idx] = mat4_mul(parent_world, local)

        return world_matrices[bone_idx]

    for i in range(num_bones):
        compute_world(i)

    return world_matrices


def compute_runtime_transform(drw_index, drw_is_weighted, drw_data,
                               evp_indices, evp_weights, evp_inv_bind,
                               bone_world_matrices):
    """Compute the runtime transform matrix for a given DRW1 index."""
    if drw_index == 0xFFFF:
        return mat4_identity(), "INVALID(0xFFFF)"

    is_weighted = drw_is_weighted[drw_index]
    data_val = drw_data[drw_index]

    if not is_weighted:
        # Rigid: just the bone's world matrix
        bone_idx = data_val
        return bone_world_matrices[bone_idx], f"rigid(bone={bone_idx})"
    else:
        # Weighted: sum(weight_i * world_matrix(bone_i) @ inv_bind(bone_i))
        evp_idx = data_val
        indices = evp_indices[evp_idx]
        weights_list = evp_weights[evp_idx]

        m = mat4_zero()
        for bone_idx, weight in zip(indices, weights_list):
            world = bone_world_matrices[bone_idx]
            inv_bind = evp_inv_bind[bone_idx]
            skin = mat4_mul(world, inv_bind)
            mat4_mad(m, skin, weight)
        m[3][3] = 1.0

        bone_names = [str(b) for b in indices]
        weight_strs = [f"{w:.3f}" for w in weights_list]
        desc = f"weighted(evp={evp_idx}, bones={indices}, weights={weight_strs})"
        return m, desc


# ============================================================
# Main analysis
# ============================================================

def parse_bmd(filepath):
    """Parse a BMD file and return all relevant data."""
    with open(filepath, 'rb') as f:
        data = f.read()

    sections = find_sections(data)
    print(f"\nParsing: {filepath}")
    print(f"  Sections: {list(sections.keys())}")

    # INF1
    inf1_off, inf1_size = sections['INF1']
    inf1_entries = parse_inf1(data, inf1_off, inf1_size)
    parent_map = build_bone_hierarchy(inf1_entries)

    # VTX1
    vtx1_off, vtx1_size = sections['VTX1']
    positions = parse_vtx1(data, vtx1_off, vtx1_size)
    print(f"  VTX1: {len(positions)} positions")

    # DRW1
    drw1_off, drw1_size = sections['DRW1']
    drw_is_weighted, drw_data = parse_drw1(data, drw1_off, drw1_size)
    print(f"  DRW1: {len(drw_data)} entries")

    # EVP1
    evp1_off, evp1_size = sections['EVP1']
    evp_indices, evp_weights, evp_inv_bind = parse_evp1(data, evp1_off, evp1_size)
    print(f"  EVP1: {len(evp_indices)} envelopes, {len(evp_inv_bind)} inverse bind matrices")

    # JNT1
    jnt1_off, jnt1_size = sections['JNT1']
    joints = parse_jnt1(data, jnt1_off, jnt1_size)
    print(f"  JNT1: {len(joints)} joints")

    # Compute bone world matrices
    bone_world = compute_bone_world_matrices(joints, parent_map)

    # SHP1 batch 9
    shp1_off, shp1_size = sections['SHP1']
    batch9_verts = parse_shp1_batch(data, shp1_off, shp1_size, batch_index=9)
    print(f"  SHP1 batch 9: {len(batch9_verts)} vertex references")

    return {
        'positions': positions,
        'drw_is_weighted': drw_is_weighted,
        'drw_data': drw_data,
        'evp_indices': evp_indices,
        'evp_weights': evp_weights,
        'evp_inv_bind': evp_inv_bind,
        'joints': joints,
        'bone_world': bone_world,
        'batch9_verts': batch9_verts,
        'parent_map': parent_map,
    }


def analyze_spike_vertices():
    orig_path = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"
    export_path = r"C:\Users\ryana\Downloads\reconstruct_test.bmd"

    print("=" * 80)
    print("BMD SPIKE VERTEX ANALYSIS")
    print("=" * 80)

    orig = parse_bmd(orig_path)
    export = parse_bmd(export_path)

    # Compute runtime positions for batch 9 in both files
    def compute_batch9_runtime(bmd):
        results = []
        for v in bmd['batch9_verts']:
            pos_idx = v['posIndex']
            drw_idx = v['drwIndex']
            vtx_pos = bmd['positions'][pos_idx]

            transform, desc = compute_runtime_transform(
                drw_idx, bmd['drw_is_weighted'], bmd['drw_data'],
                bmd['evp_indices'], bmd['evp_weights'], bmd['evp_inv_bind'],
                bmd['bone_world']
            )
            runtime_pos = mat4_mul_vec3(transform, vtx_pos)

            results.append({
                'posIndex': pos_idx,
                'vtxPos': vtx_pos,
                'drwIndex': drw_idx,
                'drwDesc': desc,
                'runtimePos': runtime_pos,
                'isWeighted': bmd['drw_is_weighted'][drw_idx] if drw_idx < len(bmd['drw_is_weighted']) else False,
            })
        return results

    print("\n\nComputing runtime positions for ORIGINAL batch 9...")
    orig_runtime = compute_batch9_runtime(orig)
    print(f"  Got {len(orig_runtime)} vertices")

    print("\nComputing runtime positions for EXPORT batch 9...")
    export_runtime = compute_batch9_runtime(export)
    print(f"  Got {len(export_runtime)} vertices")

    # Match vertices between original and export by VTX1 position
    # Build position -> original vertices map
    print("\n\nMatching vertices by VTX1 position...")

    orig_by_pos = defaultdict(list)
    for i, v in enumerate(orig_runtime):
        key = tuple(round(c, 2) for c in v['vtxPos'])
        orig_by_pos[key].append((i, v))

    export_by_pos = defaultdict(list)
    for i, v in enumerate(export_runtime):
        key = tuple(round(c, 2) for c in v['vtxPos'])
        export_by_pos[key].append((i, v))

    # Find matching pairs and compare runtime positions
    spike_vertices = []
    matched_count = 0
    unmatched_export = 0

    for ekey, everts in export_by_pos.items():
        if ekey in orig_by_pos:
            overts = orig_by_pos[ekey]
            # Match first of each (positions should be unique enough)
            for ei, ev in everts:
                best_match = None
                best_dist = float('inf')
                for oi, ov in overts:
                    d = vec3_dist(ov['runtimePos'], ev['runtimePos'])
                    if d < best_dist:
                        best_dist = d
                        best_match = (oi, ov)

                matched_count += 1
                if best_dist > 5.0:
                    spike_vertices.append({
                        'vtxPos': ev['vtxPos'],
                        'orig_idx': best_match[0],
                        'export_idx': ei,
                        'orig_drw': best_match[1]['drwIndex'],
                        'orig_drw_desc': best_match[1]['drwDesc'],
                        'export_drw': ev['drwIndex'],
                        'export_drw_desc': ev['drwDesc'],
                        'orig_runtime': best_match[1]['runtimePos'],
                        'export_runtime': ev['runtimePos'],
                        'distance': best_dist,
                    })
        else:
            unmatched_export += len(everts)

    print(f"  Matched: {matched_count}")
    print(f"  Unmatched export vertices: {unmatched_export}")
    print(f"  Spike vertices (dist > 5): {len(spike_vertices)}")

    # Sort by distance descending
    spike_vertices.sort(key=lambda x: -x['distance'])

    print("\n" + "=" * 80)
    print("SPIKE VERTICES (runtime position difference > 5 units)")
    print("=" * 80)

    for i, sv in enumerate(spike_vertices):
        print(f"\n--- Spike vertex #{i+1} ---")
        print(f"  VTX1 Position: ({sv['vtxPos'][0]:.2f}, {sv['vtxPos'][1]:.2f}, {sv['vtxPos'][2]:.2f})")
        print(f"  ORIGINAL DRW1[{sv['orig_drw']}]: {sv['orig_drw_desc']}")
        print(f"  EXPORT   DRW1[{sv['export_drw']}]: {sv['export_drw_desc']}")
        print(f"  Original runtime:  ({sv['orig_runtime'][0]:.2f}, {sv['orig_runtime'][1]:.2f}, {sv['orig_runtime'][2]:.2f})")
        print(f"  Export runtime:    ({sv['export_runtime'][0]:.2f}, {sv['export_runtime'][1]:.2f}, {sv['export_runtime'][2]:.2f})")
        print(f"  DISTANCE: {sv['distance']:.2f}")

    # ============================================================
    # Trace the Blender addon logic for spike vertices
    # ============================================================
    print("\n" + "=" * 80)
    print("TRACING BLENDER ADDON _get_vert_drw_index_weighted() LOGIC")
    print("=" * 80)

    # Build the same lookup tables the addon builds
    # vgroup_to_drw: for rigid entries, bone_idx -> drw_idx
    orig_drw_is_weighted = orig['drw_is_weighted']
    orig_drw_data = orig['drw_data']
    orig_evp_indices = orig['evp_indices']

    # bone_idx -> rigid DRW index (first match)
    bone_to_rigid_drw = {}
    for di, (isW, data_val) in enumerate(zip(orig_drw_is_weighted, orig_drw_data)):
        if not isW:
            if data_val not in bone_to_rigid_drw:
                bone_to_rigid_drw[data_val] = di

    # bone_set_to_drw: frozenset(bone_indices) -> DRW1 index
    bone_set_to_drw = {}
    bone_to_any_weighted_drw = {}
    for di, (isW, data_val) in enumerate(zip(orig_drw_is_weighted, orig_drw_data)):
        if isW and data_val < len(orig_evp_indices):
            bone_key = frozenset(orig_evp_indices[data_val])
            bone_set_to_drw[bone_key] = di
            for bi in orig_evp_indices[data_val]:
                if bi not in bone_to_any_weighted_drw:
                    bone_to_any_weighted_drw[bi] = di

    print(f"\nbone_set_to_drw has {len(bone_set_to_drw)} entries:")
    for bset, drw_idx in sorted(bone_set_to_drw.items(), key=lambda x: x[1]):
        evp_idx = orig_drw_data[drw_idx]
        weights = orig['evp_weights'][evp_idx]
        print(f"  frozenset({sorted(bset)}) -> DRW1[{drw_idx}] (evp={evp_idx}, weights={[f'{w:.3f}' for w in weights]})")

    print(f"\nbone_to_rigid_drw has {len(bone_to_rigid_drw)} entries")
    print(f"bone_to_any_weighted_drw has {len(bone_to_any_weighted_drw)} entries")

    # For each spike vertex, figure out what bone groups it would have
    # and trace the matching logic
    if spike_vertices:
        print("\n\nDETAILED TRACE FOR EACH SPIKE VERTEX:")
        print("-" * 60)

        # Get unique spike vertex positions
        seen_pos = set()
        unique_spikes = []
        for sv in spike_vertices:
            key = tuple(round(c, 2) for c in sv['vtxPos'])
            if key not in seen_pos:
                seen_pos.add(key)
                unique_spikes.append(sv)

        for sv in unique_spikes[:20]:  # limit output
            print(f"\n  VTX1 pos: ({sv['vtxPos'][0]:.2f}, {sv['vtxPos'][1]:.2f}, {sv['vtxPos'][2]:.2f})")
            print(f"  Original DRW1[{sv['orig_drw']}]: {sv['orig_drw_desc']}")
            print(f"  Export   DRW1[{sv['export_drw']}]: {sv['export_drw_desc']}")

            # What bones does the original DRW entry use?
            orig_drw_idx = sv['orig_drw']
            if orig_drw_is_weighted[orig_drw_idx]:
                evp_idx = orig_drw_data[orig_drw_idx]
                orig_bones = frozenset(orig_evp_indices[evp_idx])
                orig_bone_weights = list(zip(orig_evp_indices[evp_idx], orig['evp_weights'][evp_idx]))
                print(f"  Original envelope bones: {sorted(orig_bones)}")
                print(f"  Original weights: {[(b, f'{w:.3f}') for b, w in orig_bone_weights]}")
            else:
                orig_bones = frozenset([orig_drw_data[orig_drw_idx]])
                print(f"  Original: rigid bone {orig_drw_data[orig_drw_idx]}")

            # What does the export use?
            export_drw_idx = sv['export_drw']
            export_drw_is_w = export['drw_is_weighted']
            export_drw_d = export['drw_data']
            export_evp_i = export['evp_indices']
            if export_drw_is_w[export_drw_idx]:
                evp_idx = export_drw_d[export_drw_idx]
                export_bones = frozenset(export_evp_i[evp_idx])
                export_bone_weights = list(zip(export_evp_i[evp_idx], export['evp_weights'][evp_idx]))
                print(f"  Export envelope bones: {sorted(export_bones)}")
                print(f"  Export weights: {[(b, f'{w:.3f}') for b, w in export_bone_weights]}")
            else:
                export_bones = frozenset([export_drw_d[export_drw_idx]])
                print(f"  Export: rigid bone {export_drw_d[export_drw_idx]}")

            # Simulate addon logic
            # The vertex would have vertex groups corresponding to the
            # ORIGINAL bone set. In Blender, after import, each vertex
            # gets assigned to vertex groups for its contributing bones.
            # The export logic then tries to find the matching DRW entry.

            # Check: does the original bone set exist in bone_set_to_drw?
            lookup_result = bone_set_to_drw.get(orig_bones)
            print(f"  bone_set_to_drw[{sorted(orig_bones)}] = {lookup_result}")

            if lookup_result is not None and lookup_result != orig_drw_idx:
                print(f"  ** MISMATCH: bone_set_to_drw returns DRW1[{lookup_result}] but original uses DRW1[{orig_drw_idx}]!")
                # Check if there are multiple DRW entries with the same bone set
                duplicates = []
                for di, (isW, data_val) in enumerate(zip(orig_drw_is_weighted, orig_drw_data)):
                    if isW and data_val < len(orig_evp_indices):
                        if frozenset(orig_evp_indices[data_val]) == orig_bones:
                            duplicates.append((di, data_val))
                if len(duplicates) > 1:
                    print(f"  ** DUPLICATE BONE SETS: {duplicates}")
                    for di, evp_idx in duplicates:
                        w = orig['evp_weights'][evp_idx]
                        print(f"     DRW1[{di}] -> evp[{evp_idx}]: bones={sorted(orig_evp_indices[evp_idx])}, weights={[f'{x:.4f}' for x in w]}")

            elif lookup_result is None:
                print(f"  ** NO MATCH: bone set {sorted(orig_bones)} not found in bone_set_to_drw!")
                # What would the fallback do?
                # Fallback: rigid entry for heaviest bone
                if orig_drw_is_weighted[orig_drw_idx]:
                    evp_idx = orig_drw_data[orig_drw_idx]
                    bone_weight_pairs = list(zip(orig_evp_indices[evp_idx], orig['evp_weights'][evp_idx]))
                    heaviest_bone = max(bone_weight_pairs, key=lambda x: x[1])[0]
                    rigid_fallback = bone_to_rigid_drw.get(heaviest_bone)
                    weighted_fallback = bone_to_any_weighted_drw.get(heaviest_bone)
                    print(f"  Fallback: heaviest bone={heaviest_bone}, rigid_drw={rigid_fallback}, weighted_drw={weighted_fallback}")

    # Summary statistics
    print("\n" + "=" * 80)
    print("SUMMARY")
    print("=" * 80)

    # Count DRW type mismatches
    type_mismatch = 0
    index_mismatch = 0
    for sv in spike_vertices:
        o_isw = orig['drw_is_weighted'][sv['orig_drw']]
        e_isw = export['drw_is_weighted'][sv['export_drw']]
        if o_isw != e_isw:
            type_mismatch += 1
        if sv['orig_drw'] != sv['export_drw']:
            index_mismatch += 1

    print(f"Total spike vertices: {len(spike_vertices)}")
    print(f"DRW type mismatches (rigid vs weighted): {type_mismatch}")
    print(f"DRW index mismatches: {index_mismatch}")

    if spike_vertices:
        # Show unique DRW mappings
        print("\nUnique DRW1 index mappings (original -> export):")
        mapping_counts = defaultdict(int)
        for sv in spike_vertices:
            mapping_counts[(sv['orig_drw'], sv['export_drw'])] += 1
        for (o, e), count in sorted(mapping_counts.items()):
            o_desc = f"{'W' if orig['drw_is_weighted'][o] else 'R'}(data={orig['drw_data'][o]})"
            e_desc = f"{'W' if export['drw_is_weighted'][e] else 'R'}(data={export['drw_data'][e]})"
            print(f"  DRW1[{o}] {o_desc} -> DRW1[{e}] {e_desc}  ({count} vertices)")

    # Also dump all DRW1 entries for comparison
    print("\n\nDRW1 COMPARISON (showing differences):")
    print(f"Original: {len(orig['drw_data'])} entries, Export: {len(export['drw_data'])} entries")
    max_len = max(len(orig['drw_data']), len(export['drw_data']))
    diffs = 0
    for i in range(max_len):
        if i < len(orig['drw_data']) and i < len(export['drw_data']):
            o_w = orig['drw_is_weighted'][i]
            e_w = export['drw_is_weighted'][i]
            o_d = orig['drw_data'][i]
            e_d = export['drw_data'][i]
            if o_w != e_w or o_d != e_d:
                diffs += 1
                o_type = "weighted" if o_w else "rigid"
                e_type = "weighted" if e_w else "rigid"
                o_detail = ""
                e_detail = ""
                if o_w and o_d < len(orig['evp_indices']):
                    o_detail = f" evp bones={orig['evp_indices'][o_d]}"
                if e_w and e_d < len(export['evp_indices']):
                    e_detail = f" evp bones={export['evp_indices'][e_d]}"
                print(f"  DRW1[{i}]: orig={o_type}({o_d}{o_detail}) export={e_type}({e_d}{e_detail})")
    print(f"  Total differences: {diffs}")


if __name__ == '__main__':
    analyze_spike_vertices()
