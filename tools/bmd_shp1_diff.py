#!/usr/bin/env python3
"""Binary diff SHP1 primitive data between original and exported BMD files."""

import struct
import sys

ORIG = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"
EXPORTED = r"C:\Users\ryana\Downloads\reconstruct_test.bmd"

def read_file(path):
    with open(path, "rb") as f:
        return f.read()

def find_section(data, tag):
    """Find a section by its 4-byte tag and return (offset, size)."""
    pos = 0
    while pos < len(data) - 8:
        t = data[pos:pos+4]
        if t == tag:
            size = struct.unpack_from(">I", data, pos+4)[0]
            return pos, size
        # Try next 4 bytes (sections are 32-byte aligned usually)
        if t in (b'J3D2', b'bmd3', b'bdl4'):
            # File header - skip 32 bytes
            pos += 32
        else:
            # Try to read as section and skip
            if pos + 8 <= len(data):
                sz = struct.unpack_from(">I", data, pos+4)[0]
                if 8 < sz < len(data):
                    pos += sz
                else:
                    pos += 4
            else:
                pos += 4
    return None, None

def parse_shp1(data, shp1_off):
    """Parse SHP1 section header."""
    base = shp1_off
    tag = data[base:base+4]
    size = struct.unpack_from(">I", data, base+4)[0]
    batch_count = struct.unpack_from(">H", data, base+8)[0]
    pad = struct.unpack_from(">H", data, base+10)[0]

    batches_off = struct.unpack_from(">I", data, base+0x0C)[0]
    # 0x10 = index remap table offset (unused usually)
    # 0x14 = ???
    attribs_off = struct.unpack_from(">I", data, base+0x18)[0]
    # 0x1C = matrix table offset
    prim_data_off = struct.unpack_from(">I", data, base+0x20)[0]
    # 0x24 = matrix data offset
    draw_init_off = struct.unpack_from(">I", data, base+0x28)[0]

    print(f"  SHP1 at 0x{base:X}, size=0x{size:X}")
    print(f"  batch_count={batch_count}")
    print(f"  batches_off=0x{batches_off:X} (abs 0x{base+batches_off:X})")
    print(f"  attribs_off=0x{attribs_off:X} (abs 0x{base+attribs_off:X})")
    print(f"  prim_data_off=0x{prim_data_off:X} (abs 0x{base+prim_data_off:X})")
    print(f"  draw_init_off=0x{draw_init_off:X} (abs 0x{base+draw_init_off:X})")

    return {
        'base': base,
        'size': size,
        'batch_count': batch_count,
        'batches_off': base + batches_off,
        'attribs_off': base + attribs_off,
        'prim_data_off': base + prim_data_off,
        'draw_init_off': base + draw_init_off,
    }

def parse_batch(data, shp1, batch_idx):
    """Parse a single batch header (0x28 bytes each)."""
    off = shp1['batches_off'] + batch_idx * 0x28
    mat_type = struct.unpack_from(">B", data, off)[0]
    # +1 padding
    packet_count = struct.unpack_from(">H", data, off+2)[0]
    attrib_offset = struct.unpack_from(">H", data, off+4)[0]  # into attrib table
    first_matrix_data = struct.unpack_from(">H", data, off+6)[0]
    first_packet_loc = struct.unpack_from(">H", data, off+8)[0]  # into draw init data
    # +0x0A padding
    # +0x0C bounding box (6 floats = 24 bytes)

    print(f"  Batch {batch_idx}: mat_type={mat_type}, packets={packet_count}, attrib_off={attrib_offset}, first_matrix={first_matrix_data}, first_packet={first_packet_loc}")

    return {
        'mat_type': mat_type,
        'packet_count': packet_count,
        'attrib_offset': attrib_offset,
        'first_matrix_data': first_matrix_data,
        'first_packet_loc': first_packet_loc,
    }

def parse_attribs(data, shp1, attrib_offset):
    """Parse vertex attributes for a batch. Each entry is 4 bytes (attr, datatype). Terminated by 0xFF."""
    off = shp1['attribs_off'] + attrib_offset * 4  # NOT byte offset - it's entry offset? Let me check
    # Actually attrib_offset is a byte offset into the attrib table
    # Wait - the batch stores attrib_offset as index. Let me try both.

    # Try as byte offset first
    off = shp1['attribs_off'] + attrib_offset

    attrs = []
    while True:
        attr_type = struct.unpack_from(">I", data, off)[0]
        if attr_type == 0xFF or attr_type == 0x000000FF:
            break
        # attr_type is actually 2 fields: u32 attrib, u32 data_type
        # Actually it's: u8 attrib, u8 padding, u16 data_type? No...
        # GX batch attrib: u32 attrib, u32 cnt_or_type
        # Let me re-read: it's pairs of (GXAttr, GXAttrType) each as u32
        attr = struct.unpack_from(">I", data, off)[0]
        dtype = struct.unpack_from(">I", data, off+4)[0]
        if attr == 0xFF:
            break
        attrs.append((attr, dtype))
        off += 8
        if len(attrs) > 20:
            break

    # GXAttr names
    attr_names = {
        0: "PNMTXIDX", 1: "TEX0MTXIDX", 2: "TEX1MTXIDX", 3: "TEX2MTXIDX",
        9: "POS", 10: "NRM", 11: "CLR0", 12: "CLR1",
        13: "TEX0", 14: "TEX1", 15: "TEX2", 16: "TEX3",
        0xFF: "NULL"
    }
    dtype_names = {0: "NONE", 1: "DIRECT", 2: "INDEX8", 3: "INDEX16"}

    print(f"  Attributes (at byte offset {attrib_offset}):")
    for a, d in attrs:
        aname = attr_names.get(a, f"UNK_{a}")
        dname = dtype_names.get(d, f"UNK_{d}")
        print(f"    {aname}({a}) = {dname}({d})")

    return attrs

def parse_draw_init(data, shp1, first_packet, packet_count):
    """Parse draw init data entries. Each is 8 bytes: u32 prim_size, u32 prim_offset."""
    entries = []
    for i in range(packet_count):
        off = shp1['draw_init_off'] + (first_packet + i) * 8
        prim_size = struct.unpack_from(">I", data, off)[0]
        prim_offset = struct.unpack_from(">I", data, off+4)[0]
        entries.append((prim_size, prim_offset))
        print(f"  DrawInit[{i}]: size=0x{prim_size:X}, offset=0x{prim_offset:X}")
    return entries

def calc_vertex_size(attrs):
    """Calculate bytes per vertex from attributes."""
    size = 0
    for attr, dtype in attrs:
        if dtype == 1:  # DIRECT
            if attr == 0:  # PNMTXIDX
                size += 1
            elif attr in (1,2,3,4,5,6,7,8):  # TEXnMTXIDX
                size += 1
            elif attr == 11 or attr == 12:  # CLR0/CLR1
                size += 4  # RGBA direct
            else:
                size += 1
        elif dtype == 2:  # INDEX8
            size += 1
        elif dtype == 3:  # INDEX16
            size += 2
    return size

def parse_primitives(data, shp1, draw_init, attrs, max_verts=30):
    """Parse primitive data and return vertices."""
    has_pnmtxidx = any(a == 0 for a, d in attrs)

    prim_size, prim_offset = draw_init[0]
    base = shp1['prim_data_off'] + prim_offset

    print(f"  Primitive data starts at abs 0x{base:X}, size=0x{prim_size:X}")
    print(f"  Has PNMTXIDX: {has_pnmtxidx}")

    # Show first 32 bytes of raw prim data
    raw = data[base:base+min(64, prim_size)]
    print(f"  First {len(raw)} raw bytes: {raw.hex()}")

    vertices = []
    pos = base
    end = base + prim_size
    total_verts = 0

    while pos < end and total_verts < max_verts:
        cmd = data[pos]
        if cmd == 0:
            pos += 1
            continue

        # GX primitive commands: 0x90=triangles, 0x98=tristrip, 0xA0=trifan, 0x80=quads
        prim_type = cmd & 0xF8
        vat_idx = cmd & 0x07
        vert_count = struct.unpack_from(">H", data, pos+1)[0]
        pos += 3

        prim_names = {0x90: "TRIANGLES", 0x98: "TRISTRIP", 0xA0: "TRIFAN", 0x80: "QUADS", 0xA8: "LINESTRIP", 0x88: "LINES", 0xB0: "POINTS", 0xB8: "POINTS2"}
        pname = prim_names.get(prim_type, f"0x{prim_type:02X}")
        print(f"  Prim cmd=0x{cmd:02X} ({pname}), vat={vat_idx}, verts={vert_count}")

        for v in range(vert_count):
            if total_verts >= max_verts:
                break
            vert = {}
            for attr, dtype in attrs:
                if dtype == 1:  # DIRECT
                    if attr == 0:  # PNMTXIDX
                        vert['pnmtxidx'] = data[pos]
                        pos += 1
                    elif attr in (1,2,3,4,5,6,7,8):
                        vert[f'texmtxidx{attr-1}'] = data[pos]
                        pos += 1
                    else:
                        vert[f'direct_{attr}'] = data[pos]
                        pos += 1
                elif dtype == 2:  # INDEX8
                    val = data[pos]
                    pos += 1
                    attr_names = {9: 'pos', 10: 'nrm', 11: 'clr0', 12: 'clr1', 13: 'tex0', 14: 'tex1', 15: 'tex2'}
                    key = attr_names.get(attr, f'attr{attr}')
                    vert[key] = val
                elif dtype == 3:  # INDEX16
                    val = struct.unpack_from(">H", data, pos)[0]
                    pos += 2
                    attr_names = {9: 'pos', 10: 'nrm', 11: 'clr0', 12: 'clr1', 13: 'tex0', 14: 'tex1', 15: 'tex2'}
                    key = attr_names.get(attr, f'attr{attr}')
                    vert[key] = val

            vertices.append(vert)
            total_verts += 1

    return vertices

def parse_vtx1(data, vtx1_off):
    """Parse VTX1 section to get position data."""
    base = vtx1_off
    size = struct.unpack_from(">I", data, base+4)[0]

    # VTX1 header:
    # +0x00: tag "VTX1"
    # +0x04: size
    # +0x08: array format offset
    # +0x0C: offset to offset table (for each attr's data)

    fmt_off = struct.unpack_from(">I", data, base+0x08)[0]

    print(f"  VTX1 at 0x{base:X}, size=0x{size:X}")
    print(f"  Array format offset: 0x{fmt_off:X}")

    # Array format entries: each 16 bytes
    # GXAttr(u32), componentCount(u32), componentType(u32), fracBits(u8), pad(3)
    # Then data offsets follow

    # The offset table at +0x0C gives offsets to actual vertex data arrays
    # There are 13 possible arrays (POS, NRM, CLR0, CLR1, TEX0-TEX7)

    # Read data offsets - these are at base + 0x0C, and there can be up to 13
    # Actually the structure is:
    # +0x08: array format offset (relative to section start)
    # +0x0C: 13 u32 offsets for data arrays (POS=0, NRM=1, NBT=2, CLR0=3, CLR1=4, TEX0=5, ..., TEX7=12)

    data_offsets = []
    for i in range(13):
        off = struct.unpack_from(">I", data, base + 0x0C + i*4)[0]
        data_offsets.append(off)

    # Find position data
    # Parse array format to find position entry
    fbase = base + fmt_off
    formats = []
    for i in range(13):
        attr = struct.unpack_from(">I", data, fbase + i*16)[0]
        comp_cnt = struct.unpack_from(">I", data, fbase + i*16 + 4)[0]
        comp_type = struct.unpack_from(">I", data, fbase + i*16 + 8)[0]
        frac_bits = data[fbase + i*16 + 12]
        formats.append((attr, comp_cnt, comp_type, frac_bits))

    # comp_type: 0=U8, 1=S8, 2=U16, 3=S16, 4=F32
    comp_type_names = {0: "U8", 1: "S8", 2: "U16", 3: "S16", 4: "F32"}
    comp_cnt_names_pos = {0: "XY", 1: "XYZ"}
    comp_cnt_names_nrm = {0: "NRM", 1: "NBT", 2: "NBT3"}
    comp_cnt_names_tex = {0: "S", 1: "ST"}

    print(f"  Array formats:")
    for i, (attr, cc, ct, fb) in enumerate(formats):
        if attr == 0 and cc == 0 and ct == 0 and fb == 0:
            continue
        attr_names_vtx = {9: "POS", 10: "NRM", 11: "NBT", 12: "CLR0", 13: "CLR1",
                          14: "TEX0", 15: "TEX1", 16: "TEX2", 17: "TEX3"}
        aname = attr_names_vtx.get(attr, f"attr{attr}")
        ctname = comp_type_names.get(ct, f"type{ct}")
        doff = data_offsets[i]
        print(f"    [{i}] {aname}: compCnt={cc}, compType={ctname}({ct}), fracBits={fb}, dataOff=0x{doff:X}")

    return {
        'base': base,
        'size': size,
        'formats': formats,
        'data_offsets': data_offsets,
    }

def read_positions(data, vtx1, pos_indices, max_count=30):
    """Read position data for given indices."""
    # Find POS format
    pos_fmt = None
    pos_data_off_idx = None
    for i, (attr, cc, ct, fb) in enumerate(vtx1['formats']):
        if attr == 9:  # GX_VA_POS
            pos_fmt = (cc, ct, fb)
            pos_data_off_idx = i
            break

    if pos_fmt is None:
        print("  ERROR: No position format found!")
        return []

    cc, ct, fb = pos_fmt
    num_components = 3 if cc == 1 else 2  # XYZ or XY

    # Component size
    comp_sizes = {0: 1, 1: 1, 2: 2, 3: 2, 4: 4}
    comp_size = comp_sizes[ct]
    stride = num_components * comp_size

    base = vtx1['base'] + vtx1['data_offsets'][pos_data_off_idx]

    print(f"  Position data at abs 0x{base:X}, stride={stride}, components={num_components}, type={ct}, fracBits={fb}")

    # Verify by reading first few raw bytes
    raw = data[base:base+24]
    print(f"  First 24 raw pos bytes: {raw.hex()}")

    positions = []
    for idx in pos_indices[:max_count]:
        off = base + idx * stride
        if ct == 4:  # F32
            vals = struct.unpack_from(f">{num_components}f", data, off)
        elif ct == 3:  # S16
            raw_vals = struct.unpack_from(f">{num_components}h", data, off)
            scale = 1.0 / (1 << fb)
            vals = tuple(v * scale for v in raw_vals)
        elif ct == 2:  # U16
            raw_vals = struct.unpack_from(f">{num_components}H", data, off)
            scale = 1.0 / (1 << fb)
            vals = tuple(v * scale for v in raw_vals)
        else:
            vals = (0, 0, 0)
        positions.append(vals)

    return positions

def parse_drw1(data, drw1_off):
    """Parse DRW1 section."""
    base = drw1_off
    size = struct.unpack_from(">I", data, base+4)[0]
    count = struct.unpack_from(">H", data, base+8)[0]
    # +0x0A padding
    type_off = struct.unpack_from(">I", data, base+0x0C)[0]
    data_off = struct.unpack_from(">I", data, base+0x10)[0]

    print(f"  DRW1 at 0x{base:X}, size=0x{size:X}, count={count}")
    print(f"  type_off=0x{type_off:X}, data_off=0x{data_off:X}")

    types = []
    for i in range(count):
        t = data[base + type_off + i]
        types.append(t)

    indices = []
    for i in range(count):
        idx = struct.unpack_from(">H", data, base + data_off + i*2)[0]
        indices.append(idx)

    # Print first 30 entries
    print(f"  First {min(30, count)} DRW1 entries:")
    for i in range(min(30, count)):
        tname = "RIGID" if types[i] == 0 else "ENVELOPE"
        print(f"    [{i}]: type={tname}({types[i]}), index={indices[i]}")

    return {'types': types, 'indices': indices, 'count': count}

def parse_jnt1(data, jnt1_off):
    """Parse JNT1 section for bone data."""
    base = jnt1_off
    size = struct.unpack_from(">I", data, base+4)[0]
    count = struct.unpack_from(">H", data, base+8)[0]
    # +0x0A padding
    joint_off = struct.unpack_from(">I", data, base+0x0C)[0]
    # +0x10 index remap
    name_off = struct.unpack_from(">I", data, base+0x14)[0]

    print(f"  JNT1 at 0x{base:X}, size=0x{size:X}, count={count}")

    # Each joint entry is 0x40 bytes:
    # +0x00: u16 flag, u8 pad, u8 pad
    # +0x04: f32 scaleX, scaleY, scaleZ
    # +0x10: s16 rotX, rotY, rotZ (+ pad)
    # +0x18: f32 transX, transY, transZ
    # +0x24: f32 bounding sphere radius
    # +0x28: f32 bbox min XYZ
    # +0x34: f32 bbox max XYZ

    joints = []
    jbase = base + joint_off
    for i in range(count):
        joff = jbase + i * 0x40
        flag = struct.unpack_from(">H", data, joff)[0]
        sx, sy, sz = struct.unpack_from(">3f", data, joff+4)
        rx, ry, rz = struct.unpack_from(">3h", data, joff+0x10)
        tx, ty, tz = struct.unpack_from(">3f", data, joff+0x18)
        joints.append({
            'flag': flag,
            'scale': (sx, sy, sz),
            'rot': (rx, ry, rz),
            'trans': (tx, ty, tz),
        })

    # Print first 25 joints
    print(f"  First {min(25, count)} joints:")
    for i in range(min(25, count)):
        j = joints[i]
        print(f"    [{i}]: scale=({j['scale'][0]:.3f},{j['scale'][1]:.3f},{j['scale'][2]:.3f}), "
              f"rot=({j['rot'][0]},{j['rot'][1]},{j['rot'][2]}), "
              f"trans=({j['trans'][0]:.3f},{j['trans'][1]:.3f},{j['trans'][2]:.3f})")

    return joints

def parse_inf1(data, inf1_off):
    """Parse INF1 to get scene graph (needed for bone world matrices)."""
    base = inf1_off
    size = struct.unpack_from(">I", data, base+4)[0]
    # +0x08: u16 scalingRule, u16 pad
    # +0x0C: u32 packet count (not bones)
    # +0x10: u32 hierarchy data offset

    hier_off = struct.unpack_from(">I", data, base+0x14)[0]

    # Hierarchy entries: u16 type, u16 index
    # type 0 = end, type 1 = open child, type 2 = close child, type 0x10 = joint, type 0x11 = material, type 0x12 = shape

    entries = []
    pos = base + hier_off
    while True:
        typ = struct.unpack_from(">H", data, pos)[0]
        idx = struct.unpack_from(">H", data, pos+2)[0]
        entries.append((typ, idx))
        pos += 4
        if typ == 0:
            break
        if len(entries) > 500:
            break

    return entries

import math

def make_rotation_matrix(rx_s16, ry_s16, rz_s16):
    """Convert S16 rotation values to 3x3 rotation matrix."""
    # S16 to radians: value * (pi / 32768)
    rx = rx_s16 * math.pi / 32768.0
    ry = ry_s16 * math.pi / 32768.0
    rz = rz_s16 * math.pi / 32768.0

    cx, sx = math.cos(rx), math.sin(rx)
    cy, sy = math.cos(ry), math.sin(ry)
    cz, sz = math.cos(rz), math.sin(rz)

    # ZYX rotation order (common in J3D)
    m = [
        [cy*cz, sx*sy*cz - cx*sz, cx*sy*cz + sx*sz],
        [cy*sz, sx*sy*sz + cx*cz, cx*sy*sz - sx*cz],
        [-sy,   sx*cy,            cx*cy],
    ]
    return m

def compute_bone_world_matrices(joints, inf1_entries):
    """Compute world matrices for all bones using INF1 hierarchy."""
    # Identity 4x4
    def identity():
        return [[1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]]

    def local_matrix(j):
        rot = make_rotation_matrix(*j['rot'])
        sx, sy, sz = j['scale']
        tx, ty, tz = j['trans']
        # SRT matrix: Scale * Rot * Trans (column major thinking, but we store row-major)
        m = identity()
        for r in range(3):
            for c in range(3):
                s = [sx, sy, sz][c]
                m[r][c] = rot[r][c] * s
        m[0][3] = tx
        m[1][3] = ty
        m[2][3] = tz
        return m

    def mat_mul(a, b):
        result = [[0]*4 for _ in range(4)]
        for i in range(4):
            for j in range(4):
                for k in range(4):
                    result[i][j] += a[i][k] * b[k][j]
        return result

    bone_matrices = [None] * len(joints)
    stack = [identity()]
    joint_idx = 0

    for typ, idx in inf1_entries:
        if typ == 0x10:  # Joint
            parent_mtx = stack[-1]
            local = local_matrix(joints[idx])
            world = mat_mul(parent_mtx, local)
            bone_matrices[idx] = world
            # This joint becomes current for push
            stack[-1] = world  # replace current
        elif typ == 1:  # Push (open child)
            stack.append([row[:] for row in stack[-1]])
        elif typ == 2:  # Pop (close child)
            if len(stack) > 1:
                stack.pop()
        elif typ == 0:  # End
            break

    return bone_matrices

def transform_pos(mtx, pos):
    """Transform a position by a 4x4 matrix."""
    x, y, z = pos
    rx = mtx[0][0]*x + mtx[0][1]*y + mtx[0][2]*z + mtx[0][3]
    ry = mtx[1][0]*x + mtx[1][1]*y + mtx[1][2]*z + mtx[1][3]
    rz = mtx[2][0]*x + mtx[2][1]*y + mtx[2][2]*z + mtx[2][3]
    return (rx, ry, rz)

def analyze_file(path, label):
    print(f"\n{'='*80}")
    print(f"  ANALYZING: {label}")
    print(f"  File: {path}")
    print(f"{'='*80}")

    data = read_file(path)
    print(f"  File size: {len(data)} bytes (0x{len(data):X})")

    # Find sections
    shp1_off, shp1_size = find_section(data, b'SHP1')
    vtx1_off, vtx1_size = find_section(data, b'VTX1')
    drw1_off, drw1_size = find_section(data, b'DRW1')
    jnt1_off, jnt1_size = find_section(data, b'JNT1')
    inf1_off, inf1_size = find_section(data, b'INF1')

    print(f"\n  --- SHP1 ---")
    shp1 = parse_shp1(data, shp1_off)

    print(f"\n  --- Batch 0 ---")
    batch = parse_batch(data, shp1, 0)

    print(f"\n  --- Attributes ---")
    attrs = parse_attribs(data, shp1, batch['attrib_offset'])

    print(f"\n  --- Draw Init Data ---")
    draw_init = parse_draw_init(data, shp1, batch['first_packet_loc'], batch['packet_count'])

    print(f"\n  --- Primitive Data (first 30 verts) ---")
    vertices = parse_primitives(data, shp1, draw_init, attrs, max_verts=30)

    print(f"\n  --- Parsed Vertices ---")
    for i, v in enumerate(vertices):
        parts = []
        for k in ('pnmtxidx', 'pos', 'nrm', 'clr0', 'tex0', 'tex1'):
            if k in v:
                parts.append(f"{k}={v[k]}")
        print(f"    v[{i:2d}]: {', '.join(parts)}")

    print(f"\n  --- VTX1 ---")
    vtx1 = parse_vtx1(data, vtx1_off)

    # Read positions for the vertices we parsed
    pos_indices = [v.get('pos', 0) for v in vertices]
    print(f"\n  --- Position Lookup ---")
    positions = read_positions(data, vtx1, pos_indices)
    for i, (pos_idx, pos_val) in enumerate(zip(pos_indices, positions)):
        print(f"    v[{i:2d}] posIdx={pos_idx:4d} -> ({pos_val[0]:10.4f}, {pos_val[1]:10.4f}, {pos_val[2]:10.4f})")

    print(f"\n  --- DRW1 ---")
    drw1 = parse_drw1(data, drw1_off)

    print(f"\n  --- JNT1 ---")
    joints = parse_jnt1(data, jnt1_off)

    print(f"\n  --- INF1 hierarchy ---")
    inf1_entries = parse_inf1(data, inf1_off)
    print(f"  {len(inf1_entries)} hierarchy entries")

    print(f"\n  --- Bone World Matrices ---")
    bone_matrices = compute_bone_world_matrices(joints, inf1_entries)

    # For batch 0, find the DRW1 mapping
    # The matrix data for batch 0 tells us which DRW1 entries are used
    # Matrix table: at shp1['batches_off'] + batch['first_matrix_data'] * 2 in the matrix data section
    # Actually, the matrix indices come from the primitive's PNMTXIDX / 3
    # For rigid skinning (DRW1 type 0), the DRW1 data gives the bone index

    # Let's check DRW1 entry 19 specifically (as mentioned in the task)
    if drw1['count'] > 19:
        drw_type = drw1['types'][19]
        drw_idx = drw1['indices'][19]
        print(f"\n  DRW1[19]: type={'RIGID' if drw_type==0 else 'ENVELOPE'}({drw_type}), bone_index={drw_idx}")
        if drw_type == 0 and drw_idx < len(joints):
            j = joints[drw_idx]
            print(f"  Bone {drw_idx}: scale={j['scale']}, rot={j['rot']}, trans={j['trans']}")
            if bone_matrices[drw_idx]:
                mtx = bone_matrices[drw_idx]
                print(f"  World matrix:")
                for r in range(3):
                    print(f"    [{mtx[r][0]:10.4f} {mtx[r][1]:10.4f} {mtx[r][2]:10.4f} {mtx[r][3]:10.4f}]")

                # Transform first few positions
                print(f"\n  --- Transformed positions (bone {drw_idx} @ DRW1[19]) ---")
                for i, pos in enumerate(positions[:10]):
                    tp = transform_pos(mtx, pos)
                    print(f"    v[{i:2d}]: local({pos[0]:10.4f},{pos[1]:10.4f},{pos[2]:10.4f}) -> world({tp[0]:10.4f},{tp[1]:10.4f},{tp[2]:10.4f})")

    # Also check what the matrix table says for batch 0
    # The matrix data is at an offset we need from SHP1
    # Let me read the matrix data offset
    mtx_data_off_rel = struct.unpack_from(">I", data, shp1['base']+0x1C)[0]
    mtx_data_base = shp1['base'] + mtx_data_off_rel

    # Read matrix indices used by batch 0's first packet
    # first_matrix_data is the index into the packet-level matrix index table
    # Each packet has a matrix table that maps PNMTXIDX/3 -> DRW1 index
    # But we need to know the count... it's stored in the matrix data

    # Actually, the matrix group data has: u16 count, u16 first_index
    # at shp1_base + 0x24 offset
    mtx_group_off_rel = struct.unpack_from(">I", data, shp1['base']+0x24)[0]
    if mtx_group_off_rel > 0:
        mtx_group_base = shp1['base'] + mtx_group_off_rel
        # Read for batch 0's first matrix data entry
        mg_off = mtx_group_base + batch['first_matrix_data'] * 8
        mg_count = struct.unpack_from(">H", data, mg_off)[0]
        mg_start = struct.unpack_from(">I", data, mg_off+4)[0]  # or u16 at +2?

        # Actually matrix group entries are: u16 useMtxCount, u16 pad, u32 firstMtxIndex
        # Let me try simpler: u32 count_and_pad, u32 first_index
        print(f"\n  --- Matrix Table for Batch 0 ---")
        print(f"  Matrix group at 0x{mg_off:X}: count={mg_count}")

        # Read the actual matrix indices
        # These are u16 indices at mtx_data_base
        for i in range(min(mg_count, 20)):
            midx = struct.unpack_from(">H", data, mtx_data_base + mg_start * 2 + i * 2)[0] if mg_start < 0x10000 else 0
            if midx < drw1['count']:
                dt = drw1['types'][midx]
                di = drw1['indices'][midx]
                print(f"    mtx[{i}] -> DRW1[{midx}]: type={'RIGID' if dt==0 else 'ENV'}({dt}), idx={di}")

    return {
        'data': data,
        'shp1': shp1,
        'vtx1': vtx1,
        'drw1': drw1,
        'vertices': vertices,
        'positions': positions,
        'joints': joints,
        'bone_matrices': bone_matrices,
    }


# VTX1 data verification for exported file
def verify_vtx1_data(data, vtx1):
    """Verify VTX1 data offsets and values are correct."""
    print(f"\n  --- VTX1 Data Verification ---")

    for i, (attr, cc, ct, fb) in enumerate(vtx1['formats']):
        if attr == 0 and cc == 0 and ct == 0:
            continue

        doff = vtx1['data_offsets'][i]
        if doff == 0:
            continue

        abs_off = vtx1['base'] + doff
        attr_names = {9: "POS", 10: "NRM", 11: "NBT", 12: "CLR0", 13: "CLR1",
                      14: "TEX0", 15: "TEX1"}
        aname = attr_names.get(attr, f"attr{attr}")

        print(f"\n  Checking {aname} (format idx {i}):")
        print(f"    Data offset: 0x{doff:X} (abs 0x{abs_off:X})")
        print(f"    CompType={ct}, FracBits={fb}, CompCount={cc}")

        # Read first 5 entries
        comp_count = 3 if cc == 1 else 2
        if attr == 10:  # NRM
            comp_count = 3 if cc == 0 else (9 if cc == 1 else 9)
        if attr >= 13:  # TEX
            comp_count = 2 if cc == 1 else 1

        comp_sizes = {0: 1, 1: 1, 2: 2, 3: 2, 4: 4}
        cs = comp_sizes.get(ct, 4)
        stride = comp_count * cs

        print(f"    Components={comp_count}, Stride={stride}")
        print(f"    First 5 entries (raw and decoded):")

        for e in range(5):
            eoff = abs_off + e * stride
            raw = data[eoff:eoff+stride]

            if ct == 4:  # F32
                vals = struct.unpack_from(f">{comp_count}f", data, eoff)
                print(f"      [{e}] raw={raw.hex()} -> {tuple(f'{v:.4f}' for v in vals)}")
            elif ct == 3:  # S16
                vals_raw = struct.unpack_from(f">{comp_count}h", data, eoff)
                scale = 1.0 / (1 << fb) if fb > 0 else 1.0
                vals = tuple(v * scale for v in vals_raw)
                print(f"      [{e}] raw={raw.hex()} -> raw_s16={vals_raw} -> scaled={tuple(f'{v:.4f}' for v in vals)}")
            elif ct == 2:  # U16
                vals_raw = struct.unpack_from(f">{comp_count}H", data, eoff)
                scale = 1.0 / (1 << fb) if fb > 0 else 1.0
                vals = tuple(v * scale for v in vals_raw)
                print(f"      [{e}] raw={raw.hex()} -> raw_u16={vals_raw} -> scaled={tuple(f'{v:.4f}' for v in vals)}")
            elif ct == 1:  # S8
                vals_raw = struct.unpack_from(f">{comp_count}b", data, eoff)
                scale = 1.0 / (1 << fb) if fb > 0 else 1.0
                vals = tuple(v * scale for v in vals_raw)
                print(f"      [{e}] raw={raw.hex()} -> raw_s8={vals_raw} -> scaled={tuple(f'{v:.4f}' for v in vals)}")
            elif ct == 0:  # U8
                vals_raw = struct.unpack_from(f">{comp_count}B", data, eoff)
                scale = 1.0 / (1 << fb) if fb > 0 else 1.0
                vals = tuple(v * scale for v in vals_raw)
                print(f"      [{e}] raw={raw.hex()} -> raw_u8={vals_raw} -> scaled={tuple(f'{v:.4f}' for v in vals)}")


if __name__ == "__main__":
    print("=" * 80)
    print("  BMD SHP1 PRIMITIVE DATA DIFF")
    print("=" * 80)

    orig_result = analyze_file(ORIG, "ORIGINAL (ma_mdl1.bmd)")
    print("\n\n")
    exp_result = analyze_file(EXPORTED, "EXPORTED (reconstruct_test.bmd)")

    # Verify exported VTX1 data
    verify_vtx1_data(exp_result['data'], exp_result['vtx1'])

    # Side-by-side comparison
    print("\n\n")
    print("=" * 80)
    print("  SIDE-BY-SIDE COMPARISON: Batch 0 First 30 Vertices")
    print("=" * 80)

    orig_verts = orig_result['vertices']
    exp_verts = exp_result['vertices']

    max_v = min(len(orig_verts), len(exp_verts), 30)

    mismatches = 0
    for i in range(max_v):
        ov = orig_verts[i]
        ev = exp_verts[i]

        # Compare key fields
        match = True
        diffs = []
        for key in ('pos', 'nrm', 'tex0', 'tex1', 'clr0', 'pnmtxidx'):
            oval = ov.get(key, None)
            eval_ = ev.get(key, None)
            if oval != eval_:
                match = False
                diffs.append(f"{key}: {oval} vs {eval_}")

        status = "OK" if match else "DIFF"
        if not match:
            mismatches += 1

        orig_parts = ", ".join(f"{k}={v}" for k, v in ov.items())
        exp_parts = ", ".join(f"{k}={v}" for k, v in ev.items())

        print(f"  v[{i:2d}] [{status}]")
        print(f"    ORIG: {orig_parts}")
        print(f"    EXPT: {exp_parts}")
        if diffs:
            print(f"    DIFFS: {'; '.join(diffs)}")

    print(f"\n  Total mismatches: {mismatches}/{max_v}")

    # Compare positions
    print(f"\n  --- Position Value Comparison ---")
    orig_pos = orig_result['positions']
    exp_pos = exp_result['positions']

    for i in range(min(len(orig_pos), len(exp_pos), 30)):
        op = orig_pos[i]
        ep = exp_pos[i]

        diff = tuple(abs(a-b) for a, b in zip(op, ep))
        max_diff = max(diff)
        status = "OK" if max_diff < 0.01 else f"DIFF (max={max_diff:.4f})"

        oi = orig_verts[i].get('pos', '?')
        ei = exp_verts[i].get('pos', '?') if i < len(exp_verts) else '?'

        print(f"    v[{i:2d}] orig[{oi}]=({op[0]:10.4f},{op[1]:10.4f},{op[2]:10.4f})  exp[{ei}]=({ep[0]:10.4f},{ep[1]:10.4f},{ep[2]:10.4f})  {status}")
