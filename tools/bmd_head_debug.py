#!/usr/bin/env python3
"""
Diagnostic script to investigate head displacement in rigid batches.

Parses both original and exported BMD files, computes runtime world positions
for rigid batch 0 (M_head) vertices, and compares them.

Usage: python tools/bmd_head_debug.py
"""

import struct
import math
import sys
import os

ORIGINAL = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"
EXPORTED = r"C:\Users\ryana\Downloads\reconstruct_test.bmd"


# ── Minimal BMD parser ──────────────────────────────────────────────

class BinaryReader:
    def __init__(self, data):
        self.data = data
        self.pos = 0

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
        v = self.data[self.pos:self.pos+n].decode('ascii', errors='replace')
        self.pos += n
        return v


def find_section(data, tag):
    """Find section by 4-byte tag, return offset or None."""
    tag_bytes = tag.encode('ascii')
    offset = 0x20  # skip file header
    while offset < len(data) - 8:
        if data[offset:offset+4] == tag_bytes:
            return offset
        size = struct.unpack_from('>I', data, offset + 4)[0]
        if size < 8 or size > len(data):
            break
        offset += size
    return None


# ── INF1 parsing ────────────────────────────────────────────────────

def parse_inf1(data, offset):
    br = BinaryReader(data)
    br.seek(offset)
    tag = br.read_str(4)
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


def build_scene_graph(entries):
    root = {'type': -1, 'index': -1, 'children': []}

    def _build(parent, pos):
        while pos < len(entries):
            etype, eindex = entries[pos]
            if etype == 1:
                pos = _build(parent['children'][-1], pos + 1)
            elif etype == 2:
                return pos + 1
            elif etype in (0x10, 0x11, 0x12):
                node = {'type': etype, 'index': eindex, 'children': []}
                parent['children'].append(node)
                pos += 1
            else:
                pos += 1
        return pos

    _build(root, 0)
    if len(root['children']) == 1:
        return root['children'][0]
    return root


def find_batch_parent_joint(sg, target_batch):
    def _search(node, joint_stack):
        if node['type'] == 0x10:
            joint_stack = joint_stack + [node['index']]
        if node['type'] == 0x12 and node['index'] == target_batch:
            return joint_stack
        for child in node['children']:
            result = _search(child, joint_stack)
            if result is not None:
                return result
        return None
    return _search(sg, [])


def walk_inf1_batch_assignments(sg):
    results = {}
    def _walk(node, current_joint):
        if node['type'] == 0x10:
            current_joint = node['index']
        if node['type'] == 0x12:
            results[node['index']] = current_joint
        for child in node['children']:
            _walk(child, current_joint)
    _walk(sg, -1)
    return results


# ── JNT1 parsing ────────────────────────────────────────────────────

def parse_string_table(data, offset, expected_count):
    br = BinaryReader(data)
    br.seek(offset)
    count = br.read_u16()
    pad = br.read_u16()
    entries = []
    for i in range(count):
        hash_val = br.read_u16()
        str_offset = br.read_u16()
        entries.append(str_offset)
    names = []
    for str_off in entries:
        abs_off = offset + str_off
        end = data.index(b'\x00', abs_off)
        name = data[abs_off:end].decode('ascii', errors='replace')
        names.append(name)
    return names


def parse_jnt1(data, offset):
    br = BinaryReader(data)
    br.seek(offset)
    tag = br.read_str(4)
    section_size = br.read_u32()
    count = br.read_u16()
    pad = br.read_u16()
    jnt_entry_offset = br.read_u32()
    unknown_offset = br.read_u32()
    string_table_offset = br.read_u32()
    names = parse_string_table(data, offset + string_table_offset, count)

    joints = []
    for i in range(count):
        br.seek(offset + jnt_entry_offset + i * 0x40)
        j = {}
        j['matrix_type'] = br.read_u16()
        j['pad'] = br.read_u16()
        j['sx'] = br.read_f32()
        j['sy'] = br.read_f32()
        j['sz'] = br.read_f32()
        j['rx'] = br.read_s16()
        j['ry'] = br.read_s16()
        j['rz'] = br.read_s16()
        j['pad2'] = br.read_u16()
        j['tx'] = br.read_f32()
        j['ty'] = br.read_f32()
        j['tz'] = br.read_f32()
        j['unknown2'] = br.read_f32()
        j['bb_min'] = [br.read_f32() for _ in range(3)]
        j['bb_max'] = [br.read_f32() for _ in range(3)]
        j['name'] = names[i] if i < len(names) else f'joint_{i}'
        joints.append(j)
    return joints


# ── Matrix math ─────────────────────────────────────────────────────

def mat4_identity():
    return [[1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]]

def mat4_mul(a, b):
    r = [[0]*4 for _ in range(4)]
    for i in range(4):
        for j in range(4):
            for k in range(4):
                r[i][j] += a[i][k] * b[k][j]
    return r

def mat4_translation(tx, ty, tz):
    m = mat4_identity()
    m[0][3] = tx
    m[1][3] = ty
    m[2][3] = tz
    return m

def mat4_from_euler_xyz(rx, ry, rz):
    cx, sx = math.cos(rx), math.sin(rx)
    cy, sy = math.cos(ry), math.sin(ry)
    cz, sz = math.cos(rz), math.sin(rz)
    rx_mat = [[1,0,0,0],[0,cx,-sx,0],[0,sx,cx,0],[0,0,0,1]]
    ry_mat = [[cy,0,sy,0],[0,1,0,0],[-sy,0,cy,0],[0,0,0,1]]
    rz_mat = [[cz,-sz,0,0],[sz,cz,0,0],[0,0,1,0],[0,0,0,1]]
    return mat4_mul(mat4_mul(rx_mat, ry_mat), rz_mat)

def mat4_vec3(m, v):
    x = m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2] + m[0][3]
    y = m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2] + m[1][3]
    z = m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2] + m[2][3]
    return (x, y, z)

def joint_frame_matrix(j):
    rx = j['rx'] / 32768.0 * math.pi
    ry = j['ry'] / 32768.0 * math.pi
    rz = j['rz'] / 32768.0 * math.pi
    t = mat4_translation(j['tx'], j['ty'], j['tz'])
    r = mat4_from_euler_xyz(rx, ry, rz)
    return mat4_mul(t, r)


def compute_bone_world_matrices(joints, inf1_entries):
    world_matrices = [None] * len(joints)

    def _walk(entries, pos, parent_matrix):
        current_joint_matrix = parent_matrix
        while pos < len(entries):
            etype, eindex = entries[pos]
            if etype == 0x10:
                local = joint_frame_matrix(joints[eindex])
                world = mat4_mul(parent_matrix, local)
                world_matrices[eindex] = world
                current_joint_matrix = world
                pos += 1
            elif etype == 0x11 or etype == 0x12:
                pos += 1
            elif etype == 1:
                pos = _walk(entries, pos + 1, current_joint_matrix)
            elif etype == 2:
                return pos + 1
            else:
                pos += 1
        return pos

    _walk(inf1_entries, 0, mat4_identity())
    return world_matrices


# ── DRW1 parsing ────────────────────────────────────────────────────

def parse_drw1(data, offset):
    br = BinaryReader(data)
    br.seek(offset)
    tag = br.read_str(4)
    section_size = br.read_u32()
    count = br.read_u16()
    pad = br.read_u16()
    offset_to_weighted = br.read_u32()
    offset_to_data = br.read_u32()

    is_weighted = []
    br.seek(offset + offset_to_weighted)
    for i in range(count):
        is_weighted.append(br.read_u8())

    data_indices = []
    br.seek(offset + offset_to_data)
    for i in range(count):
        data_indices.append(br.read_u16())

    return is_weighted, data_indices


# ── SHP1 parsing ────────────────────────────────────────────────────

def parse_shp1(data, offset):
    """Parse SHP1. Returns (batches, header_info)."""
    br = BinaryReader(data)
    br.seek(offset)
    tag = br.read_str(4)              # +0x00
    section_size = br.read_u32()       # +0x04
    batch_count = br.read_u16()        # +0x08
    pad = br.read_u16()                # +0x0A
    offset_to_batches = br.read_u32()  # +0x0C
    offset_unknown = br.read_u32()     # +0x10
    zero = br.read_u32()               # +0x14
    offset_to_attribs = br.read_u32()  # +0x18
    offset_to_mtx_table = br.read_u32() # +0x1C
    offset_data = br.read_u32()        # +0x20
    offset_to_mtx_data = br.read_u32() # +0x24
    offset_to_pkt_locs = br.read_u32() # +0x28

    batches = []
    for i in range(batch_count):
        br.seek(offset + offset_to_batches + i * 40)  # Shp1BatchDescriptor.size = 40
        b = {}
        b['mattype'] = br.read_u16()  # "unknown" field, 0x00 or 0x03
        b['packet_count'] = br.read_u16()
        b['offset_to_attribs'] = br.read_u16()
        b['first_matrix_data'] = br.read_u16()
        b['first_packet_location'] = br.read_u16()
        b['pad'] = br.read_u16()
        b['unknown4'] = [br.read_f32() for _ in range(7)]

        # Parse attribs for this batch
        b['attribs'] = []
        br.seek(offset + offset_to_attribs + b['offset_to_attribs'])
        for _ in range(20):
            attr = br.read_u32()
            if attr == 0xFF:
                break
            dtype = br.read_u32()
            b['attribs'].append((attr, dtype))

        # Parse matrix data and tables for each packet
        b['matrix_tables'] = []
        for pkt in range(b['packet_count']):
            md_off = offset + offset_to_mtx_data + (b['first_matrix_data'] + pkt) * 8
            br.seek(md_off)
            md_unknown1 = br.read_u16()
            md_count = br.read_u16()
            md_first_index = br.read_u32()

            entries = []
            br.seek(offset + offset_to_mtx_table + md_first_index * 2)
            for j in range(md_count):
                entries.append(br.read_u16())
            b['matrix_tables'].append(entries)

        # Parse primitives for this batch
        b['vertices'] = []
        for pkt in range(b['packet_count']):
            pkt_loc_off = offset + offset_to_pkt_locs + (b['first_packet_location'] + pkt) * 8
            br.seek(pkt_loc_off)
            pkt_size = br.read_u32()
            pkt_offset = br.read_u32()

            abs_prim = offset + offset_data + pkt_offset
            br.seek(abs_prim)
            end_pos = abs_prim + pkt_size

            while br.pos < end_pos:
                prim_type = br.read_u8()
                if prim_type == 0:
                    break
                num_verts = br.read_u16()
                for v in range(num_verts):
                    mat_idx = None
                    pos_idx = None
                    for attr_id, data_type in b['attribs']:
                        if data_type == 1:  # u8
                            val = br.read_u8()
                            if attr_id == 0:
                                mat_idx = val // 3
                        elif data_type == 3:  # u16
                            val = br.read_u16()
                            if attr_id == 9:
                                pos_idx = val
                    b['vertices'].append((mat_idx, pos_idx, pkt))

        batches.append(b)

    return batches


# ── VTX1 parsing ────────────────────────────────────────────────────

def parse_vtx1_positions(data, offset):
    br = BinaryReader(data)
    br.seek(offset)
    tag = br.read_str(4)
    section_size = br.read_u32()
    array_format_offset = br.read_u32()
    data_offsets = [br.read_u32() for _ in range(13)]

    pos_data_offset = data_offsets[0]
    if pos_data_offset == 0:
        return []

    # Read array format for positions
    br.seek(offset + array_format_offset)
    fmt_attr = br.read_u32()
    fmt_comp_count = br.read_u32()
    fmt_data_type = br.read_u32()
    fmt_frac_bits = br.read_u8()

    num_components = 3 if fmt_comp_count == 1 else 2

    # Figure out where position data ends: next non-zero data offset or section end
    next_offset = section_size
    for d in data_offsets[1:]:
        if d != 0 and d > pos_data_offset:
            next_offset = min(next_offset, d)
            break

    positions = []
    br.seek(offset + pos_data_offset)
    remaining = next_offset - pos_data_offset

    if fmt_data_type == 4:  # F32
        stride = num_components * 4
        count = remaining // stride
        for i in range(count):
            coords = [br.read_f32() for _ in range(num_components)]
            if num_components == 2:
                coords.append(0.0)
            positions.append(tuple(coords))
    elif fmt_data_type == 3:  # S16
        scale = 1.0 / (1 << fmt_frac_bits)
        stride = num_components * 2
        count = remaining // stride
        for i in range(count):
            coords = [br.read_s16() * scale for _ in range(num_components)]
            if num_components == 2:
                coords.append(0.0)
            positions.append(tuple(coords))

    return positions


# ── Main analysis ───────────────────────────────────────────────────

def analyze_bmd(filepath, label):
    print(f"\n{'='*70}")
    print(f"Analyzing: {label}")
    print(f"  File: {filepath}")
    print(f"{'='*70}")

    with open(filepath, 'rb') as f:
        data = f.read()

    inf1_off = find_section(data, 'INF1')
    vtx1_off = find_section(data, 'VTX1')
    drw1_off = find_section(data, 'DRW1')
    jnt1_off = find_section(data, 'JNT1')
    shp1_off = find_section(data, 'SHP1')

    print(f"\n  Section offsets:")
    for name, off in [('INF1', inf1_off), ('VTX1', vtx1_off), ('DRW1', drw1_off),
                       ('JNT1', jnt1_off), ('SHP1', shp1_off)]:
        print(f"    {name}: 0x{off:X}" if off else f"    {name}: NOT FOUND")

    # ── INF1 ──
    inf1_entries = parse_inf1(data, inf1_off)
    sg = build_scene_graph(inf1_entries)
    batch_joints = walk_inf1_batch_assignments(sg)
    print(f"\n  INF1 batch -> parent joint (first 10):")
    for batch_idx in sorted(batch_joints.keys())[:10]:
        print(f"    Batch {batch_idx} -> Joint {batch_joints[batch_idx]}")

    path = find_batch_parent_joint(sg, 0)
    print(f"  Joint path to batch 0: {path}")

    # ── JNT1 ──
    joints = parse_jnt1(data, jnt1_off)
    print(f"\n  JNT1: {len(joints)} joints")
    for idx in [0, 28]:
        if idx < len(joints):
            j = joints[idx]
            print(f"    Joint {idx} ({j['name']}): t=({j['tx']:.2f}, {j['ty']:.2f}, {j['tz']:.2f})")

    # ── Bone world matrices ──
    world_matrices = compute_bone_world_matrices(joints, inf1_entries)
    print(f"\n  Bone world positions:")
    for idx in [0, 14, 26, 27, 28]:
        if idx < len(joints) and world_matrices[idx]:
            m = world_matrices[idx]
            print(f"    Joint {idx} ({joints[idx]['name']}): ({m[0][3]:.2f}, {m[1][3]:.2f}, {m[2][3]:.2f})")

    # ── DRW1 ──
    is_weighted, drw_data = parse_drw1(data, drw1_off)
    print(f"\n  DRW1: {len(is_weighted)} entries")
    for i in range(min(25, len(is_weighted))):
        w_str = "W" if is_weighted[i] else "R"
        bone = joints[drw_data[i]]['name'] if not is_weighted[i] and drw_data[i] < len(joints) else str(drw_data[i])
        print(f"    DRW[{i:3d}]: {w_str} -> {bone}")

    head_drw = None
    for i, (iw, d) in enumerate(zip(is_weighted, drw_data)):
        if not iw and d == 28:
            head_drw = i
            break
    print(f"\n  DRW for bone 28 (M_head): DRW[{head_drw}]")

    # ── SHP1 ──
    batches = parse_shp1(data, shp1_off)
    print(f"\n  SHP1: {len(batches)} batches")
    for i in range(min(5, len(batches))):
        b = batches[i]
        print(f"    Batch {i}: mattype=0x{b['mattype']:02X}, pkts={b['packet_count']}, "
              f"attribs={[(hex(a),d) for a,d in b['attribs']]}")
        for pkt_idx, mt in enumerate(b['matrix_tables']):
            info = []
            for drw_idx in mt:
                if drw_idx != 0xFFFF and drw_idx < len(is_weighted):
                    w = "W" if is_weighted[drw_idx] else "R"
                    d = drw_data[drw_idx]
                    name = joints[d]['name'] if not is_weighted[drw_idx] and d < len(joints) else str(d)
                    info.append(f"DRW[{drw_idx}]={w}:{name}")
                else:
                    info.append(f"0x{drw_idx:04X}")
            print(f"      Pkt {pkt_idx} mtx: {info}")
        print(f"      Vertices: {len(b['vertices'])}")

    # ── VTX1 ──
    positions = parse_vtx1_positions(data, vtx1_off)
    print(f"\n  VTX1: {len(positions)} positions")

    # ── Compute runtime positions for batch 0 ──
    print(f"\n  Batch 0 first 10 vertices (runtime world positions):")
    b0 = batches[0]
    for i, (mi, pi, pkt) in enumerate(b0['vertices'][:10]):
        if pi is None or pi >= len(positions):
            continue
        vtx_pos = positions[pi]

        # Resolve matrix: pkt -> matrix table -> DRW -> bone -> world matrix
        mtx_table = b0['matrix_tables'][pkt] if pkt < len(b0['matrix_tables']) else []
        if mi is not None and mi < len(mtx_table):
            drw_idx = mtx_table[mi]
        elif len(mtx_table) >= 1:
            drw_idx = mtx_table[0]
        else:
            drw_idx = None

        if drw_idx is not None and drw_idx < len(is_weighted) and not is_weighted[drw_idx]:
            bone_idx = drw_data[drw_idx]
            wm = world_matrices[bone_idx] if bone_idx < len(world_matrices) else None
            if wm:
                rt = mat4_vec3(wm, vtx_pos)
                print(f"    [{i:2d}] mi={mi} pi={pi:4d} DRW[{drw_idx}]->{joints[bone_idx]['name']:15s} "
                      f"vtx1=({vtx_pos[0]:8.2f},{vtx_pos[1]:8.2f},{vtx_pos[2]:8.2f}) "
                      f"runtime=({rt[0]:8.2f},{rt[1]:8.2f},{rt[2]:8.2f})")
            else:
                print(f"    [{i:2d}] mi={mi} pi={pi:4d} DRW[{drw_idx}]->bone {bone_idx} (no world matrix)")
        else:
            print(f"    [{i:2d}] mi={mi} pi={pi:4d} DRW={drw_idx}")

    # Return data for comparison
    return {
        'positions': positions, 'batches': batches,
        'world_matrices': world_matrices, 'joints': joints,
        'is_weighted': is_weighted, 'drw_data': drw_data,
        'inf1_entries': inf1_entries, 'batch_joints': batch_joints,
    }


def compare_results(orig, export):
    print(f"\n{'='*70}")
    print("COMPARISON")
    print(f"{'='*70}")

    # INF1 batch assignments
    print("\n  INF1 batch -> joint:")
    for b in sorted(set(list(orig['batch_joints'].keys()) + list(export['batch_joints'].keys())))[:10]:
        oj = orig['batch_joints'].get(b, '?')
        ej = export['batch_joints'].get(b, '?')
        match = "OK" if oj == ej else "MISMATCH"
        oj_name = orig['joints'][oj]['name'] if isinstance(oj, int) and oj < len(orig['joints']) else '?'
        ej_name = export['joints'][ej]['name'] if isinstance(ej, int) and ej < len(export['joints']) else '?'
        if match != "OK":
            print(f"    Batch {b}: orig=J{oj}({oj_name}), export=J{ej}({ej_name}) [{match}]")
        else:
            print(f"    Batch {b}: J{oj}({oj_name}) [{match}]")

    # DRW1 differences
    print("\n  DRW1 mismatches:")
    mismatch_count = 0
    for i in range(min(len(orig['is_weighted']), len(export['is_weighted']))):
        ow, od = orig['is_weighted'][i], orig['drw_data'][i]
        ew, ed = export['is_weighted'][i], export['drw_data'][i]
        if ow != ew or od != ed:
            print(f"    DRW[{i}]: orig=({'W' if ow else 'R'},{od}) export=({'W' if ew else 'R'},{ed})")
            mismatch_count += 1
    if mismatch_count == 0:
        print(f"    None (all {min(len(orig['is_weighted']), len(export['is_weighted']))} entries match)")

    # Batch 0 comparison
    print("\n  Batch 0 comparison:")
    ob0 = orig['batches'][0]
    eb0 = export['batches'][0]
    print(f"    Orig:   mattype=0x{ob0['mattype']:02X}, pkts={ob0['packet_count']}, verts={len(ob0['vertices'])}")
    print(f"    Export: mattype=0x{eb0['mattype']:02X}, pkts={eb0['packet_count']}, verts={len(eb0['vertices'])}")
    print(f"    Orig   attribs: {[(hex(a),d) for a,d in ob0['attribs']]}")
    print(f"    Export attribs: {[(hex(a),d) for a,d in eb0['attribs']]}")
    for pkt in range(min(ob0['packet_count'], eb0['packet_count'])):
        omt = ob0['matrix_tables'][pkt] if pkt < len(ob0['matrix_tables']) else []
        emt = eb0['matrix_tables'][pkt] if pkt < len(eb0['matrix_tables']) else []
        match = "OK" if omt == emt else "MISMATCH"
        print(f"    Pkt {pkt} mtx: orig={omt} export={emt} [{match}]")

    # Compare runtime positions
    print("\n  Runtime position comparison (batch 0, first 5 unique verts):")
    def _runtime(b, pos_pool, wm, iw, dd, joints_list, vert_entry):
        mi, pi, pkt = vert_entry
        if pi is None or pi >= len(pos_pool):
            return None, None, None
        vtx_pos = pos_pool[pi]
        mt = b['matrix_tables'][pkt] if pkt < len(b['matrix_tables']) else []
        if mi is not None and mi < len(mt):
            drw_idx = mt[mi]
        elif mt:
            drw_idx = mt[0]
        else:
            return vtx_pos, None, None
        if drw_idx is not None and drw_idx < len(iw) and not iw[drw_idx]:
            bone_idx = dd[drw_idx]
            m = wm[bone_idx] if bone_idx < len(wm) and wm[bone_idx] else None
            if m:
                return vtx_pos, mat4_vec3(m, vtx_pos), joints_list[bone_idx]['name'] if bone_idx < len(joints_list) else '?'
        return vtx_pos, None, None

    seen = set()
    count = 0
    for i in range(min(len(ob0['vertices']), len(eb0['vertices']))):
        _, opi, _ = ob0['vertices'][i]
        if opi in seen:
            continue
        seen.add(opi)

        o_vtx, o_rt, o_bone = _runtime(ob0, orig['positions'], orig['world_matrices'],
                                         orig['is_weighted'], orig['drw_data'], orig['joints'],
                                         ob0['vertices'][i])
        e_vtx, e_rt, e_bone = _runtime(eb0, export['positions'], export['world_matrices'],
                                         export['is_weighted'], export['drw_data'], export['joints'],
                                         eb0['vertices'][i])
        if o_vtx and e_vtx:
            vtx_diff = sum((a-b)**2 for a,b in zip(o_vtx, e_vtx))**0.5
            print(f"    [{count}] VTX1: orig=({o_vtx[0]:8.2f},{o_vtx[1]:8.2f},{o_vtx[2]:8.2f}) "
                  f"export=({e_vtx[0]:8.2f},{e_vtx[1]:8.2f},{e_vtx[2]:8.2f}) diff={vtx_diff:.4f}")
        if o_rt and e_rt:
            rt_diff = sum((a-b)**2 for a,b in zip(o_rt, e_rt))**0.5
            print(f"          RUNTIME: orig=({o_rt[0]:8.2f},{o_rt[1]:8.2f},{o_rt[2]:8.2f}) [{o_bone}]"
                  f" export=({e_rt[0]:8.2f},{e_rt[1]:8.2f},{e_rt[2]:8.2f}) [{e_bone}] diff={rt_diff:.4f}")
        count += 1
        if count >= 5:
            break

    # Check if ALL batches 0-3 (rigid head) have correct joint parentage
    print("\n  Rigid head batches (0-3) analysis:")
    for bi in range(4):
        if bi < len(export['batches']):
            eb = export['batches'][bi]
            ej = export['batch_joints'].get(bi, '?')
            print(f"    Export batch {bi}: parent_joint={ej}, mattype=0x{eb['mattype']:02X}, "
                  f"mtx_table={eb['matrix_tables']}")


def main():
    for f in [ORIGINAL, EXPORTED]:
        if not os.path.exists(f):
            print(f"ERROR: File not found: {f}")
            sys.exit(1)

    orig = analyze_bmd(ORIGINAL, "ORIGINAL")
    export = analyze_bmd(EXPORTED, "EXPORTED")
    compare_results(orig, export)


if __name__ == '__main__':
    main()
