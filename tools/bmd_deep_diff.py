#!/usr/bin/env python3
"""Deep binary diff of original vs exported BMD - focusing on weighted batch runtime transforms."""

import struct
import math
import sys
from collections import defaultdict

# ============================================================================
# BMD Binary Parser (standalone, no Blender dependency)
# ============================================================================

class BMDParser:
    def __init__(self, filepath):
        with open(filepath, 'rb') as f:
            self.data = f.read()
        self.pos = 0
        self.filepath = filepath

    def seek(self, pos):
        self.pos = pos

    def read_u8(self):
        v = self.data[self.pos]
        self.pos += 1
        return v

    def read_u16(self):
        v = struct.unpack_from('>H', self.data, self.pos)[0]
        self.pos += 2
        return v

    def read_s16(self):
        v = struct.unpack_from('>h', self.data, self.pos)[0]
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

    def read_str(self, n):
        v = self.data[self.pos:self.pos+n].decode('ascii')
        self.pos += n
        return v

    def find_section(self, tag):
        """Find a section by its 4-byte tag, return its start offset."""
        tag_bytes = tag.encode('ascii')
        offset = self.data.find(tag_bytes, 0x20)
        if offset == -1:
            raise ValueError(f"Section {tag} not found")
        return offset

    # --- INF1 ---
    def parse_inf1(self):
        base = self.find_section('INF1')
        self.seek(base + 4)
        size = self.read_u32()
        self.seek(base + 8)
        unknown1 = self.read_u16()
        pad = self.read_u16()
        packet_count = self.read_u32()
        vertex_count = self.read_u32()
        entries_offset = self.read_u32()

        # Parse entries to build joint hierarchy
        self.seek(base + entries_offset)
        entries = []
        while True:
            etype = self.read_u16()
            eindex = self.read_u16()
            if etype == 0:
                break
            entries.append((etype, eindex))

        # Build hierarchy: track parent of each joint
        joint_parent = {}  # joint_idx -> parent_joint_idx
        stack = []
        current_joint = -1
        for etype, eindex in entries:
            if etype == 0x10:  # Joint
                joint_parent[eindex] = current_joint if stack else -1
                current_joint = eindex
            elif etype == 0x01:  # Down
                stack.append(current_joint)
            elif etype == 0x02:  # Up
                current_joint = stack.pop() if stack else -1

        return joint_parent

    # --- JNT1 ---
    def parse_jnt1(self):
        base = self.find_section('JNT1')
        self.seek(base + 4)
        size = self.read_u32()
        count = self.read_u16()
        pad = self.read_u16()
        jnt_entry_offset = self.read_u32()

        joints = []
        for i in range(count):
            self.seek(base + jnt_entry_offset + i * 0x40)
            unknown = self.read_u16()
            pad2 = self.read_u16()
            sx = self.read_f32()
            sy = self.read_f32()
            sz = self.read_f32()
            rx = self.read_s16()
            ry = self.read_s16()
            rz = self.read_s16()
            pad3 = self.read_u16()
            tx = self.read_f32()
            ty = self.read_f32()
            tz = self.read_f32()
            joints.append({
                'sx': sx, 'sy': sy, 'sz': sz,
                'rx': rx * math.pi / 32768.0,
                'ry': ry * math.pi / 32768.0,
                'rz': rz * math.pi / 32768.0,
                'tx': tx, 'ty': ty, 'tz': tz,
            })
        return joints

    # --- EVP1 ---
    def parse_evp1(self):
        base = self.find_section('EVP1')
        self.seek(base + 4)
        size = self.read_u32()
        count = self.read_u16()
        pad = self.read_u16()
        offsets = [self.read_u32() for _ in range(4)]

        # Counts
        self.seek(base + offsets[0])
        counts = [self.read_u8() for _ in range(count)]
        total = sum(counts)

        # Indices
        self.seek(base + offsets[1])
        num_matrices = 0
        weighted_indices = []
        for i in range(count):
            indices = []
            for j in range(counts[i]):
                idx = self.read_u16()
                indices.append(idx)
                num_matrices = max(num_matrices, idx + 1)
            weighted_indices.append(indices)

        # Weights
        self.seek(base + offsets[2])
        weighted_weights = []
        for i in range(count):
            weights = []
            for j in range(counts[i]):
                weights.append(self.read_f32())
            weighted_weights.append(weights)

        # Inverse bind matrices (3x4)
        self.seek(base + offsets[3])
        inv_bind = []
        for i in range(num_matrices):
            m = [[0]*4 for _ in range(4)]
            for r in range(3):
                for c in range(4):
                    m[r][c] = self.read_f32()
            m[3][3] = 1.0
            inv_bind.append(m)

        envelopes = []
        for i in range(count):
            envelopes.append({
                'indices': weighted_indices[i],
                'weights': weighted_weights[i],
            })

        return envelopes, inv_bind

    # --- DRW1 ---
    def parse_drw1(self):
        base = self.find_section('DRW1')
        self.seek(base + 4)
        size = self.read_u32()
        count = self.read_u16()
        pad = self.read_u16()
        offset_is_weighted = self.read_u32()
        offset_data = self.read_u32()

        self.seek(base + offset_is_weighted)
        is_weighted = [self.read_u8() for _ in range(count)]

        self.seek(base + offset_data)
        data = [self.read_u16() for _ in range(count)]

        return is_weighted, data

    # --- VTX1 positions ---
    def parse_vtx1_positions(self):
        base = self.find_section('VTX1')
        self.seek(base + 4)
        size = self.read_u32()
        fmt_offset = self.read_u32()
        offsets = [self.read_u32() for _ in range(13)]

        # Find position array (slot 0 typically)
        pos_slot = -1
        for i in range(13):
            if offsets[i] != 0:
                pos_slot = i
                break

        if pos_slot == -1:
            return []

        # Read format for position slot
        # Count active slots before pos_slot
        active_before = sum(1 for i in range(pos_slot) if offsets[i] != 0)
        self.seek(base + fmt_offset + active_before * 16)
        array_type = self.read_u32()
        comp_count = self.read_u32()
        data_type = self.read_u32()
        decimal_point = self.read_u8()

        assert array_type == 9, f"Expected position array type 9, got {array_type}"

        # Compute length
        start_offset = offsets[pos_slot]
        end_offset = size
        for i in range(pos_slot + 1, 13):
            if offsets[i] != 0:
                end_offset = offsets[i]
                break

        length = end_offset - start_offset
        self.seek(base + start_offset)

        positions = []
        if data_type == 4:  # f32
            count = length // 12  # 3 floats per position
            for _ in range(count):
                x = self.read_f32()
                y = self.read_f32()
                z = self.read_f32()
                positions.append((x, y, z))
        elif data_type == 3:  # s16 fixed point
            scale = 1.0 / (2 ** decimal_point)
            count = length // 6
            for _ in range(count):
                x = self.read_s16() * scale
                y = self.read_s16() * scale
                z = self.read_s16() * scale
                positions.append((x, y, z))

        return positions

    # --- SHP1 ---
    def parse_shp1(self):
        base = self.find_section('SHP1')
        self.seek(base + 4)
        size = self.read_u32()
        batch_count = self.read_u16()
        pad = self.read_u16()
        offset_batches = self.read_u32()      # +0x0C
        offset_unknown = self.read_u32()       # +0x10
        zero = self.read_u32()                 # +0x14
        offset_attribs = self.read_u32()       # +0x18
        offset_mtx_table = self.read_u32()     # +0x1C
        offset_data = self.read_u32()          # +0x20
        offset_mtx_data = self.read_u32()      # +0x24
        offset_pkt_locs = self.read_u32()      # +0x28

        batches = []
        for bi in range(batch_count):
            self.seek(base + offset_batches + bi * 40)
            unknown = self.read_u16()
            packet_count = self.read_u16()
            attribs_off = self.read_u16()
            first_mtx_data = self.read_u16()
            first_pkt_loc = self.read_u16()
            unk3 = self.read_u16()
            bbox = [self.read_f32() for _ in range(7)]

            # Read attribs for this batch
            attribs = []
            self.seek(base + offset_attribs + attribs_off)
            while True:
                attr = self.read_u32()
                dtype = self.read_u32()
                if attr == 0xff:
                    break
                attribs.append((attr, dtype))

            has_mtx_idx = any(a[0] == 0 for a in attribs)

            # Read packets
            packets = []
            for pi in range(packet_count):
                # Packet location
                self.seek(base + offset_pkt_locs + (first_pkt_loc + pi) * 8)
                pkt_size = self.read_u32()
                pkt_offset = self.read_u32()

                # Matrix data
                self.seek(base + offset_mtx_data + (first_mtx_data + pi) * 8)
                mtx_unk1 = self.read_u16()
                mtx_count = self.read_u16()
                mtx_first_index = self.read_u32()

                # Matrix table
                mtx_table = []
                self.seek(base + offset_mtx_table + mtx_first_index * 2)
                for _ in range(mtx_count):
                    mtx_table.append(self.read_u16())

                # Read primitives
                self.seek(base + offset_data + pkt_offset)
                vertices = []
                read_bytes = 0
                while read_bytes < pkt_size:
                    prim_type = self.read_u8()
                    read_bytes += 1
                    if prim_type == 0 or read_bytes >= pkt_size:
                        break
                    vert_count = self.read_u16()
                    read_bytes += 2
                    for _ in range(vert_count):
                        vert = {}
                        for attr, dtype in attribs:
                            if dtype == 1:  # u8
                                val = self.read_u8()
                                read_bytes += 1
                            elif dtype == 3:  # u16
                                val = self.read_u16()
                                read_bytes += 2
                            else:
                                val = 0

                            if attr == 0:
                                vert['matrixIndex'] = val
                            elif attr == 9:
                                vert['posIndex'] = val
                        vertices.append(vert)

                packets.append({
                    'matrixTable': mtx_table,
                    'vertices': vertices,
                    'mtx_unk1': mtx_unk1,
                })

            batches.append({
                'unknown': unknown,
                'has_mtx_idx': has_mtx_idx,
                'packets': packets,
                'attribs': attribs,
            })

        return batches


# ============================================================================
# Matrix math (pure Python, no numpy/mathutils)
# ============================================================================

def mat4_identity():
    return [[1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]]

def mat4_mul(a, b):
    r = [[0]*4 for _ in range(4)]
    for i in range(4):
        for j in range(4):
            for k in range(4):
                r[i][j] += a[i][k] * b[k][j]
    return r

def mat4_translate(tx, ty, tz):
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

def mat4_transform_point(m, p):
    x = m[0][0]*p[0] + m[0][1]*p[1] + m[0][2]*p[2] + m[0][3]
    y = m[1][0]*p[0] + m[1][1]*p[1] + m[1][2]*p[2] + m[1][3]
    z = m[2][0]*p[0] + m[2][1]*p[1] + m[2][2]*p[2] + m[2][3]
    return (x, y, z)

def mat4_zero():
    return [[0]*4 for _ in range(4)]

def mat4_mad(r, m, f):
    """r += m * f (for rows 0-2)"""
    for i in range(3):
        for j in range(4):
            r[i][j] += f * m[i][j]

def dist3(a, b):
    return math.sqrt((a[0]-b[0])**2 + (a[1]-b[1])**2 + (a[2]-b[2])**2)


def build_bone_world_matrices(joints, joint_parent):
    """Build world matrices for each joint by traversing hierarchy."""
    n = len(joints)
    world_mats = [None] * n

    # Build children list
    children = defaultdict(list)
    roots = []
    for ji in range(n):
        parent = joint_parent.get(ji, -1)
        if parent == -1:
            roots.append(ji)
        else:
            children[parent].append(ji)

    def compute(ji, parent_mat):
        j = joints[ji]
        # Local matrix: Translation @ RotZ @ RotY @ RotX (XYZ Euler)
        t = mat4_translate(j['tx'], j['ty'], j['tz'])
        rx = mat4_rot_x(j['rx'])
        ry = mat4_rot_y(j['ry'])
        rz = mat4_rot_z(j['rz'])
        # XYZ Euler: Rz @ Ry @ Rx
        local_rot = mat4_mul(rz, mat4_mul(ry, rx))
        local_mat = mat4_mul(t, local_rot)

        # Scale
        sx, sy, sz = j['sx'], j['sy'], j['sz']
        if abs(sx) < 0.01: sx = 1.0
        if abs(sy) < 0.01: sy = 1.0
        if abs(sz) < 0.01: sz = 1.0
        s = mat4_scale(sx, sy, sz)
        local_mat_scaled = mat4_mul(local_mat, s)

        world_mats[ji] = mat4_mul(parent_mat, local_mat)
        for child in children[ji]:
            compute(child, world_mats[ji])

    for root in roots:
        compute(root, mat4_identity())

    # Handle any orphaned joints
    for ji in range(n):
        if world_mats[ji] is None:
            world_mats[ji] = mat4_identity()

    return world_mats


def compute_drw_matrix(drw_idx, drw_is_weighted, drw_data, bone_world_mats, evp_envelopes, evp_inv_bind):
    """Compute the runtime transform matrix for a DRW1 entry."""
    is_w = drw_is_weighted[drw_idx]
    data_val = drw_data[drw_idx]

    if not is_w:
        # Rigid: just the bone world matrix
        if data_val < len(bone_world_mats):
            return bone_world_mats[data_val], 'rigid', data_val
        return mat4_identity(), 'rigid', data_val
    else:
        # Weighted: sum(w_i * bone_world[bone_i] @ evp_inv_bind[bone_i])
        if data_val < len(evp_envelopes):
            env = evp_envelopes[data_val]
            m = mat4_zero()
            for i in range(len(env['indices'])):
                bi = env['indices'][i]
                w = env['weights'][i]
                if bi < len(bone_world_mats) and bi < len(evp_inv_bind):
                    comp = mat4_mul(bone_world_mats[bi], evp_inv_bind[bi])
                    mat4_mad(m, comp, w)
            m[3][3] = 1.0
            return m, 'weighted', data_val
        return mat4_identity(), 'weighted', data_val


# ============================================================================
# Main analysis
# ============================================================================

def analyze_batch(bmd, batch_idx, positions, drw_is_weighted, drw_data,
                  bone_world_mats, evp_envelopes, evp_inv_bind, label):
    """Analyze a single batch: for each vertex, compute runtime position."""
    batches = bmd.parse_shp1()
    if batch_idx >= len(batches):
        print(f"  [{label}] Batch {batch_idx} not found (only {len(batches)} batches)")
        return []

    batch = batches[batch_idx]
    print(f"\n  [{label}] Batch {batch_idx}: unknown=0x{batch['unknown']:04x}, "
          f"has_mtx_idx={batch['has_mtx_idx']}, {len(batch['packets'])} packets")
    print(f"  [{label}] Attribs: {batch['attribs']}")

    results = []
    for pi, pkt in enumerate(batch['packets']):
        mtx_table = pkt['matrixTable']
        # Check for 0xFFFF entries
        ffff_count = sum(1 for v in mtx_table if v == 0xFFFF)
        print(f"  [{label}] Packet {pi}: {len(mtx_table)} mtx_table entries, "
              f"{ffff_count} are 0xFFFF, values={mtx_table[:30]}{'...' if len(mtx_table) > 30 else ''}")

        for vi, vert in enumerate(pkt['vertices']):
            pos_idx = vert.get('posIndex', 0)
            mtx_idx_raw = vert.get('matrixIndex', None)

            if pos_idx >= len(positions):
                results.append({
                    'pos_idx': pos_idx, 'vtx1_pos': None,
                    'runtime_pos': None, 'drw_idx': -1,
                    'drw_type': 'ERROR', 'drw_data': -1,
                    'error': f'posIndex {pos_idx} out of range ({len(positions)})',
                    'packet': pi, 'mtx_idx_raw': mtx_idx_raw,
                })
                continue

            vtx1_pos = positions[pos_idx]

            if batch['has_mtx_idx'] and mtx_idx_raw is not None:
                # Weighted batch: matrixIndex / 3 = index into packet matrix table
                local_idx = mtx_idx_raw // 3
                if local_idx >= len(mtx_table):
                    results.append({
                        'pos_idx': pos_idx, 'vtx1_pos': vtx1_pos,
                        'runtime_pos': None, 'drw_idx': -1,
                        'drw_type': 'OOB', 'drw_data': -1,
                        'error': f'matrixIndex {mtx_idx_raw} / 3 = {local_idx} >= mtx_table size {len(mtx_table)}',
                        'packet': pi, 'mtx_idx_raw': mtx_idx_raw,
                    })
                    continue

                drw_idx = mtx_table[local_idx]
                if drw_idx == 0xFFFF:
                    results.append({
                        'pos_idx': pos_idx, 'vtx1_pos': vtx1_pos,
                        'runtime_pos': None, 'drw_idx': 0xFFFF,
                        'drw_type': 'FFFF', 'drw_data': -1,
                        'error': f'mtx_table[{local_idx}] = 0xFFFF (inherit from prev packet)',
                        'packet': pi, 'mtx_idx_raw': mtx_idx_raw,
                    })
                    continue
            else:
                # Rigid batch: use first matrix table entry (single-bone batch)
                if mtx_table:
                    drw_idx = mtx_table[0]
                else:
                    drw_idx = 0

            if drw_idx >= len(drw_is_weighted):
                results.append({
                    'pos_idx': pos_idx, 'vtx1_pos': vtx1_pos,
                    'runtime_pos': None, 'drw_idx': drw_idx,
                    'drw_type': 'DRW_OOB', 'drw_data': -1,
                    'error': f'drw_idx {drw_idx} >= DRW1 count {len(drw_is_weighted)}',
                    'packet': pi, 'mtx_idx_raw': mtx_idx_raw,
                })
                continue

            drw_mat, drw_type, drw_data_val = compute_drw_matrix(
                drw_idx, drw_is_weighted, drw_data,
                bone_world_mats, evp_envelopes, evp_inv_bind)

            runtime_pos = mat4_transform_point(drw_mat, vtx1_pos)

            results.append({
                'pos_idx': pos_idx, 'vtx1_pos': vtx1_pos,
                'runtime_pos': runtime_pos, 'drw_idx': drw_idx,
                'drw_type': drw_type, 'drw_data': drw_data_val,
                'error': None, 'packet': pi, 'mtx_idx_raw': mtx_idx_raw,
            })

    return results


def main():
    orig_path = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"
    export_path = r"C:\Users\ryana\Downloads\reconstruct_test.bmd"

    print("=" * 80)
    print("BMD DEEP DIFF: Weighted Batch Runtime Transform Analysis")
    print("=" * 80)

    # Parse both BMDs
    print("\nParsing original BMD...")
    orig = BMDParser(orig_path)
    print("Parsing exported BMD...")
    exp = BMDParser(export_path)

    # Parse all sections from both
    print("\n--- Parsing sections ---")

    orig_joints = orig.parse_jnt1()
    exp_joints = exp.parse_jnt1()
    print(f"JNT1: orig={len(orig_joints)} joints, exp={len(exp_joints)} joints")

    orig_parent = orig.parse_inf1()
    exp_parent = exp.parse_inf1()

    orig_evp, orig_inv_bind = orig.parse_evp1()
    exp_evp, exp_inv_bind = exp.parse_evp1()
    print(f"EVP1: orig={len(orig_evp)} envelopes/{len(orig_inv_bind)} matrices, "
          f"exp={len(exp_evp)} envelopes/{len(exp_inv_bind)} matrices")

    orig_drw_w, orig_drw_d = orig.parse_drw1()
    exp_drw_w, exp_drw_d = exp.parse_drw1()
    print(f"DRW1: orig={len(orig_drw_w)} entries, exp={len(exp_drw_w)} entries")
    rigid_orig = sum(1 for w in orig_drw_w if not w)
    weighted_orig = sum(1 for w in orig_drw_w if w)
    rigid_exp = sum(1 for w in exp_drw_w if not w)
    weighted_exp = sum(1 for w in exp_drw_w if w)
    print(f"  Orig: {rigid_orig} rigid, {weighted_orig} weighted")
    print(f"  Exp:  {rigid_exp} rigid, {weighted_exp} weighted")

    orig_positions = orig.parse_vtx1_positions()
    exp_positions = exp.parse_vtx1_positions()
    print(f"VTX1: orig={len(orig_positions)} positions, exp={len(exp_positions)} positions")

    orig_batches = orig.parse_shp1()
    exp_batches = exp.parse_shp1()
    print(f"SHP1: orig={len(orig_batches)} batches, exp={len(exp_batches)} batches")

    # Build bone world matrices
    print("\n--- Building bone world matrices ---")
    orig_world = build_bone_world_matrices(orig_joints, orig_parent)
    exp_world = build_bone_world_matrices(exp_joints, exp_parent)

    # Compare bone matrices
    max_bone_diff = 0
    for i in range(min(len(orig_world), len(exp_world))):
        for r in range(3):
            for c in range(4):
                d = abs(orig_world[i][r][c] - exp_world[i][r][c])
                max_bone_diff = max(max_bone_diff, d)
    print(f"Max bone world matrix difference: {max_bone_diff:.6f}")

    # Compare DRW1 entries
    print("\n--- DRW1 comparison ---")
    drw_diffs = 0
    for i in range(min(len(orig_drw_w), len(exp_drw_w))):
        if orig_drw_w[i] != exp_drw_w[i] or orig_drw_d[i] != exp_drw_d[i]:
            if drw_diffs < 10:
                print(f"  DRW1[{i}]: orig=({'W' if orig_drw_w[i] else 'R'},{orig_drw_d[i]}) "
                      f"exp=({'W' if exp_drw_w[i] else 'R'},{exp_drw_d[i]})")
            drw_diffs += 1
    print(f"  Total DRW1 differences: {drw_diffs}")

    # Analyze ALL batches to get an overview
    print("\n--- Batch overview ---")
    for bi in range(max(len(orig_batches), len(exp_batches))):
        ob = orig_batches[bi] if bi < len(orig_batches) else None
        eb = exp_batches[bi] if bi < len(exp_batches) else None
        o_verts = sum(len(p['vertices']) for p in ob['packets']) if ob else 0
        e_verts = sum(len(p['vertices']) for p in eb['packets']) if eb else 0
        o_type = 'W' if ob and ob['has_mtx_idx'] else 'R'
        e_type = 'W' if eb and eb['has_mtx_idx'] else 'R'
        print(f"  Batch {bi:2d}: orig={o_type} {o_verts:5d}v  exp={e_type} {e_verts:5d}v")

    # Now deep-dive into batch 9
    TARGET_BATCH = 9
    print(f"\n{'='*80}")
    print(f"DEEP ANALYSIS: Batch {TARGET_BATCH}")
    print(f"{'='*80}")

    orig_results = analyze_batch(orig, TARGET_BATCH, orig_positions,
                                  orig_drw_w, orig_drw_d, orig_world,
                                  orig_evp, orig_inv_bind, "ORIG")
    exp_results = analyze_batch(exp, TARGET_BATCH, exp_positions,
                                 exp_drw_w, exp_drw_d, exp_world,
                                 exp_evp, exp_inv_bind, "EXP")

    # Count errors
    orig_errors = [r for r in orig_results if r['error']]
    exp_errors = [r for r in exp_results if r['error']]
    print(f"\n  Orig: {len(orig_results)} vertices, {len(orig_errors)} errors")
    print(f"  Exp:  {len(exp_results)} vertices, {len(exp_errors)} errors")
    for err in exp_errors[:10]:
        print(f"    EXP ERROR: {err['error']} (packet={err['packet']}, mtx_idx={err['mtx_idx_raw']})")

    # Match vertices by vtx1_pos
    print(f"\n--- Matching vertices by VTX1 position ---")
    orig_by_pos = defaultdict(list)
    for r in orig_results:
        if r['vtx1_pos'] is not None and r['runtime_pos'] is not None:
            key = (round(r['vtx1_pos'][0], 2),
                   round(r['vtx1_pos'][1], 2),
                   round(r['vtx1_pos'][2], 2))
            orig_by_pos[key].append(r)

    matched = []
    unmatched_exp = []
    for r in exp_results:
        if r['vtx1_pos'] is not None and r['runtime_pos'] is not None:
            key = (round(r['vtx1_pos'][0], 2),
                   round(r['vtx1_pos'][1], 2),
                   round(r['vtx1_pos'][2], 2))
            if key in orig_by_pos and orig_by_pos[key]:
                orig_r = orig_by_pos[key][0]  # Take first match
                d = dist3(orig_r['runtime_pos'], r['runtime_pos'])
                matched.append((orig_r, r, d))
            else:
                unmatched_exp.append(r)

    print(f"  Matched: {len(matched)}")
    print(f"  Unmatched export vertices: {len(unmatched_exp)}")

    # Categorize
    good = [m for m in matched if m[2] < 1.0]
    bad = [m for m in matched if 1.0 <= m[2] < 10.0]
    explosion = [m for m in matched if m[2] >= 10.0]

    print(f"\n  GOOD (dist < 1):     {len(good)}")
    print(f"  BAD (1 <= dist < 10): {len(bad)}")
    print(f"  EXPLOSION (dist >= 10): {len(explosion)}")

    # Top 10 worst
    worst = sorted(matched, key=lambda m: -m[2])[:20]
    print(f"\n--- Top 20 worst matches ---")
    for i, (orig_r, exp_r, d) in enumerate(worst):
        print(f"\n  #{i+1}: distance = {d:.2f}")
        print(f"    vtx1_pos = ({orig_r['vtx1_pos'][0]:.2f}, {orig_r['vtx1_pos'][1]:.2f}, {orig_r['vtx1_pos'][2]:.2f})")
        print(f"    ORIG: drw_idx={orig_r['drw_idx']} type={orig_r['drw_type']} data={orig_r['drw_data']} "
              f"runtime=({orig_r['runtime_pos'][0]:.2f}, {orig_r['runtime_pos'][1]:.2f}, {orig_r['runtime_pos'][2]:.2f})")
        print(f"    EXP:  drw_idx={exp_r['drw_idx']} type={exp_r['drw_type']} data={exp_r['drw_data']} "
              f"runtime=({exp_r['runtime_pos'][0]:.2f}, {exp_r['runtime_pos'][1]:.2f}, {exp_r['runtime_pos'][2]:.2f})")
        if orig_r['drw_idx'] != exp_r['drw_idx']:
            print(f"    >>> DRW INDEX MISMATCH: orig={orig_r['drw_idx']} exp={exp_r['drw_idx']}")
        if orig_r['drw_type'] != exp_r['drw_type']:
            print(f"    >>> DRW TYPE MISMATCH: orig={orig_r['drw_type']} exp={exp_r['drw_type']}")

    # Check DRW type distribution in mismatches
    print(f"\n--- DRW type analysis for mismatched vertices ---")
    type_combos = defaultdict(int)
    for orig_r, exp_r, d in matched:
        if d >= 1.0:
            combo = f"orig={orig_r['drw_type']}({orig_r['drw_idx']})->exp={exp_r['drw_type']}({exp_r['drw_idx']})"
            type_combos[combo] += 1
    for combo, count in sorted(type_combos.items(), key=lambda x: -x[1])[:20]:
        print(f"  {combo}: {count}")

    # Also analyze batch 10 if it exists
    TARGET_BATCH2 = 10
    if TARGET_BATCH2 < len(exp_batches):
        print(f"\n{'='*80}")
        print(f"DEEP ANALYSIS: Batch {TARGET_BATCH2}")
        print(f"{'='*80}")

        orig_results2 = analyze_batch(orig, TARGET_BATCH2, orig_positions,
                                       orig_drw_w, orig_drw_d, orig_world,
                                       orig_evp, orig_inv_bind, "ORIG")
        exp_results2 = analyze_batch(exp, TARGET_BATCH2, exp_positions,
                                      exp_drw_w, exp_drw_d, exp_world,
                                      exp_evp, exp_inv_bind, "EXP")

        exp_errors2 = [r for r in exp_results2 if r['error']]
        print(f"  Exp errors: {len(exp_errors2)}")
        for err in exp_errors2[:10]:
            print(f"    EXP ERROR: {err['error']}")

    # Check: are there exported vertices in weighted batches where
    # matrixIndex points to a rigid DRW entry when it should be weighted?
    print(f"\n{'='*80}")
    print("DRW TYPE CHECK: Are weighted batch vertices pointing to rigid DRW entries?")
    print(f"{'='*80}")

    for bi in range(len(exp_batches)):
        batch = exp_batches[bi]
        if not batch['has_mtx_idx']:
            continue
        rigid_refs = 0
        weighted_refs = 0
        total_verts = 0
        for pkt in batch['packets']:
            for vert in pkt['vertices']:
                total_verts += 1
                mtx_idx = vert.get('matrixIndex', 0)
                local_idx = mtx_idx // 3
                if local_idx < len(pkt['matrixTable']):
                    drw_idx = pkt['matrixTable'][local_idx]
                    if drw_idx != 0xFFFF and drw_idx < len(exp_drw_w):
                        if exp_drw_w[drw_idx]:
                            weighted_refs += 1
                        else:
                            rigid_refs += 1
        if total_verts > 0:
            print(f"  Batch {bi}: {total_verts} verts, {rigid_refs} rigid refs, {weighted_refs} weighted refs")

    # Final: check the unique DRW indices used per batch in both
    print(f"\n{'='*80}")
    print("UNIQUE DRW INDICES PER BATCH")
    print(f"{'='*80}")

    for bi in range(max(len(orig_batches), len(exp_batches))):
        for label, batches_list, drw_w, drw_d in [("orig", orig_batches, orig_drw_w, orig_drw_d),
                                                     ("exp", exp_batches, exp_drw_w, exp_drw_d)]:
            if bi >= len(batches_list):
                continue
            batch = batches_list[bi]
            drw_indices = set()
            for pkt in batch['packets']:
                for entry in pkt['matrixTable']:
                    if entry != 0xFFFF:
                        drw_indices.add(entry)
            drw_details = []
            for di in sorted(drw_indices):
                if di < len(drw_w):
                    t = 'W' if drw_w[di] else 'R'
                    drw_details.append(f"{di}({t},{drw_d[di]})")
            if bi in (9, 10) or (len(drw_indices) > 0 and bi < 15):
                print(f"  Batch {bi:2d} [{label}]: {drw_details[:20]}{'...' if len(drw_details) > 20 else ''}")


if __name__ == '__main__':
    main()
