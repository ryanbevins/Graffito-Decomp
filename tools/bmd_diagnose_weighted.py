#!/usr/bin/env python3
"""
Comprehensive diagnostic for weighted batch vertex explosion.
Parses BOTH original and exported BMD files, builds full runtime transforms,
and compares vertex positions for weighted batches.
"""

import struct
import math
from pathlib import Path

ORIG_BMD = Path(r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd")
EXPORT_BMD = Path(r"C:\Users\ryana\Downloads\reconstruct_test.bmd")

# ============================================================================
# Binary reader
# ============================================================================
class R:
    def __init__(self, data, pos=0):
        self.d = data
        self.p = pos

    def seek(self, p): self.p = p
    def skip(self, n): self.p += n

    def u8(self):
        v = self.d[self.p]; self.p += 1; return v
    def s16(self):
        v = struct.unpack_from('>h', self.d, self.p)[0]; self.p += 2; return v
    def u16(self):
        v = struct.unpack_from('>H', self.d, self.p)[0]; self.p += 2; return v
    def u32(self):
        v = struct.unpack_from('>I', self.d, self.p)[0]; self.p += 4; return v
    def f32(self):
        v = struct.unpack_from('>f', self.d, self.p)[0]; self.p += 4; return v
    def tag(self):
        t = self.d[self.p:self.p+4].decode('ascii'); self.p += 4; return t

# ============================================================================
# Matrix math
# ============================================================================
def m4id(): return [[1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]]
def m4z():  return [[0,0,0,0],[0,0,0,0],[0,0,0,0],[0,0,0,0]]

def m4mul(a, b):
    r = m4z()
    for i in range(4):
        for j in range(4):
            for k in range(4):
                r[i][j] += a[i][k] * b[k][j]
    return r

def m4xfm(m, v):
    return (m[0][0]*v[0]+m[0][1]*v[1]+m[0][2]*v[2]+m[0][3],
            m[1][0]*v[0]+m[1][1]*v[1]+m[1][2]*v[2]+m[1][3],
            m[2][0]*v[0]+m[2][1]*v[1]+m[2][2]*v[2]+m[2][3])

def m4mad(dst, src, f):
    for j in range(3):
        for k in range(4):
            dst[j][k] += f * src[j][k]

def dist3(a, b):
    return math.sqrt(sum((a[i]-b[i])**2 for i in range(3)))

def make_trs(tx,ty,tz,rx,ry,rz,sx,sy,sz):
    """Build T @ Rot(XYZ euler) @ S matrix"""
    cx,sx_ = math.cos(rx), math.sin(rx)
    cy,sy_ = math.cos(ry), math.sin(ry)
    cz,sz_ = math.cos(rz), math.sin(rz)
    # Rotation = Rz @ Ry @ Rx (Blender XYZ euler convention)
    r00 = cy*cz; r01 = sx_*sy_*cz - cx*sz_; r02 = cx*sy_*cz + sx_*sz_
    r10 = cy*sz_; r11 = sx_*sy_*sz_ + cx*cz; r12 = cx*sy_*sz_ - sx_*cz
    r20 = -sy_;    r21 = sx_*cy;              r22 = cx*cy
    return [
        [r00*sx, r01*sy, r02*sz, tx],
        [r10*sx, r11*sy, r12*sz, ty],
        [r20*sx, r21*sy, r22*sz, tz],
        [0, 0, 0, 1]
    ]

# ============================================================================
# Section finding
# ============================================================================
def find_sections(data):
    sections = {}
    r = R(data)
    r.seek(0)
    r.tag()  # J3D2
    r.skip(4)
    fsize = r.u32()
    nsec = r.u32()
    r.skip(16)  # SVR3

    pos = r.p
    for _ in range(nsec):
        if pos + 8 > len(data):
            break
        tag = data[pos:pos+4].decode('ascii', errors='replace')
        size = struct.unpack_from('>I', data, pos+4)[0]
        sections[tag] = (pos, size)
        pos += size
    return sections

# ============================================================================
# INF1
# ============================================================================
def parse_inf1(data, off):
    r = R(data, off+8)
    r.skip(2+2)  # flags, pad
    r.skip(4+4)  # num_packets, vertex_count
    hier_off = r.u32()
    r.seek(off + hier_off)

    parent_stack = [-1]
    joint_parent = {}
    batch_order = []

    while True:
        etype = r.u16()
        idx = r.u16()
        if etype == 0: break
        elif etype == 0x10: parent_stack.append(parent_stack[-1])
        elif etype == 0x11:
            if len(parent_stack) > 1: parent_stack.pop()
        elif etype == 0x12:
            joint_parent[idx] = parent_stack[-1]
            parent_stack[-1] = idx
        elif etype == 0x02:
            batch_order.append(idx)

    return joint_parent, batch_order

# ============================================================================
# JNT1
# ============================================================================
def parse_jnt1(data, off):
    r = R(data, off+8)
    nj = r.u16()
    r.skip(2)
    joff = r.u32()

    joints = []
    for i in range(nj):
        r.seek(off + joff + i * 0x40)
        j = {}
        j['flags'] = r.u16()
        r.skip(2)
        j['sx'], j['sy'], j['sz'] = r.f32(), r.f32(), r.f32()
        j['rx'] = r.s16() * math.pi / 32768.0
        j['ry'] = r.s16() * math.pi / 32768.0
        j['rz'] = r.s16() * math.pi / 32768.0
        r.skip(2)
        j['tx'], j['ty'], j['tz'] = r.f32(), r.f32(), r.f32()
        joints.append(j)
    return joints

def build_bone_worlds(joints, joint_parent):
    world = [None] * len(joints)
    for i in range(len(joints)):
        j = joints[i]
        local = make_trs(j['tx'],j['ty'],j['tz'], j['rx'],j['ry'],j['rz'], j['sx'],j['sy'],j['sz'])
        p = joint_parent.get(i, -1)
        world[i] = m4mul(world[p], local) if (p >= 0 and world[p]) else local
    return world

# ============================================================================
# EVP1
# ============================================================================
def parse_evp1(data, off):
    r = R(data, off+8)
    count = r.u16()
    r.skip(2)
    o0,o1,o2,o3 = r.u32(), r.u32(), r.u32(), r.u32()

    r.seek(off + o0)
    counts = [r.u8() for _ in range(count)]

    r.seek(off + o1)
    envelopes = []
    for i in range(count):
        env = {'indices': [r.u16() for _ in range(counts[i])], 'weights': []}
        envelopes.append(env)

    r.seek(off + o2)
    for i in range(count):
        envelopes[i]['weights'] = [r.f32() for _ in range(counts[i])]

    max_bone = max((bi for env in envelopes for bi in env['indices']), default=-1) + 1
    r.seek(off + o3)
    inv_bind = []
    for _ in range(max_bone):
        m = m4id()
        for row in range(3):
            for col in range(4):
                m[row][col] = r.f32()
        inv_bind.append(m)

    return envelopes, inv_bind

# ============================================================================
# DRW1
# ============================================================================
def parse_drw1(data, off):
    r = R(data, off+8)
    count = r.u16()
    r.skip(2)
    o_w, o_d = r.u32(), r.u32()
    r.seek(off + o_w)
    is_w = [r.u8() for _ in range(count)]
    r.seek(off + o_d)
    drw_d = [r.u16() for _ in range(count)]
    return is_w, drw_d

# ============================================================================
# VTX1
# ============================================================================
def parse_vtx1(data, off, size):
    r = R(data, off+8)
    fmt_off = r.u32()
    offsets = [r.u32() for _ in range(13)]

    # Read format entries for non-zero offsets
    pos_fmt = None
    r.seek(off + fmt_off)
    for i in range(13):
        if offsets[i] != 0:
            fmt = {'arrayType': r.u32(), 'componentCount': r.u32(),
                   'dataType': r.u32(), 'decimalPoint': r.u8()}
            r.skip(3)
            if fmt['arrayType'] == 9:
                pos_fmt = fmt

    if not pos_fmt:
        return []

    pos_off = offsets[0]
    # Find end of position data
    next_off = size
    for o in offsets[1:]:
        if o != 0:
            next_off = o
            break

    scale = 1.0 / (1 << pos_fmt['decimalPoint']) if pos_fmt['dataType'] == 3 else 1.0
    nbytes = next_off - pos_off
    r.seek(off + pos_off)

    positions = []
    if pos_fmt['dataType'] == 3:  # s16
        for _ in range(nbytes // 6):
            positions.append((r.s16()*scale, r.s16()*scale, r.s16()*scale))
    elif pos_fmt['dataType'] == 4:  # f32
        for _ in range(nbytes // 12):
            positions.append((r.f32(), r.f32(), r.f32()))

    return positions

# ============================================================================
# SHP1
# ============================================================================
def parse_shp1(data, off):
    r = R(data, off+8)
    nbatch = r.u16()
    r.skip(2)
    off_batches = r.u32()
    r.skip(4)   # offsetUnknown
    r.skip(4)   # zero
    off_attribs = r.u32()
    off_mtx_table = r.u32()
    off_prim_data = r.u32()
    off_mtx_data = r.u32()
    off_pkt_locs = r.u32()

    batches = []
    for bi in range(nbatch):
        r.seek(off + off_batches + bi * 40)
        batch = {}
        batch['matrixType'] = r.u16()  # unknown field
        batch['numPackets'] = r.u16()
        batch['offsetToAttribs'] = r.u16()
        batch['firstMtxData'] = r.u16()
        batch['firstPktLoc'] = r.u16()

        # Read attribs
        attribs = []
        r.seek(off + off_attribs + batch['offsetToAttribs'])
        while True:
            a_type = r.u32()
            a_dt = r.u32()
            if a_type == 0xFF:
                break
            attribs.append({'attrib': a_type, 'dataType': a_dt})
        batch['attribs'] = attribs

        # Read packets
        batch['packets'] = []
        for pi in range(batch['numPackets']):
            pkt = {}

            # Packet location
            r.seek(off + off_pkt_locs + (batch['firstPktLoc'] + pi) * 8)
            pkt_size = r.u32()
            pkt_offset = r.u32()

            # Matrix data
            r.seek(off + off_mtx_data + (batch['firstMtxData'] + pi) * 8)
            mtx_unk = r.u16()
            mtx_count = r.u16()
            mtx_first = r.u32()

            # Matrix table
            r.seek(off + off_mtx_table + mtx_first * 2)
            pkt['matrixTable'] = [r.u16() for _ in range(mtx_count)]

            # Primitives
            r.seek(off + off_prim_data + pkt_offset)
            pkt['primitives'] = []
            bread = 0
            while bread < pkt_size:
                ptype = r.u8(); bread += 1
                if ptype == 0 or bread >= pkt_size:
                    break
                count = r.u16(); bread += 2
                prim = {'type': ptype, 'points': []}
                for _ in range(count):
                    pt = {}
                    for attr in attribs:
                        if attr['dataType'] == 1:
                            val = r.u8(); bread += 1
                        elif attr['dataType'] == 3:
                            val = r.u16(); bread += 2
                        else:
                            val = 0
                        if attr['attrib'] == 0:   pt['mtxIdx'] = val
                        elif attr['attrib'] == 9:  pt['posIdx'] = val
                        elif attr['attrib'] == 0xa: pt['nrmIdx'] = val
                    prim['points'].append(pt)
                pkt['primitives'].append(prim)

            batch['packets'].append(pkt)
        batches.append(batch)
    return batches

# ============================================================================
# Runtime transform
# ============================================================================
def compute_runtime(pos, drw_idx, bmd):
    iw = bmd['is_w'][drw_idx]
    dv = bmd['drw_d'][drw_idx]
    if not iw:
        return m4xfm(bmd['bone_world'][dv], pos), 'rigid', dv, None
    else:
        env = bmd['envs'][dv]
        m = m4z()
        for bi, w in zip(env['indices'], env['weights']):
            m4mad(m, m4mul(bmd['bone_world'][bi], bmd['inv_bind'][bi]), w)
        m[3][3] = 1.0
        return m4xfm(m, pos), 'weighted', dv, list(zip(env['indices'], env['weights']))

def get_batch_verts(bmd, batch_idx):
    batch = bmd['batches'][batch_idx]
    verts = []
    for pkt in batch['packets']:
        mt = pkt['matrixTable']
        for prim in pkt['primitives']:
            for pt in prim['points']:
                pi = pt.get('posIdx', 0)
                mi = pt.get('mtxIdx', 0)
                li = mi // 3
                drw = mt[li] if li < len(mt) else 0
                if drw == 0xFFFF or pi >= len(bmd['pos']):
                    continue
                vpos = bmd['pos'][pi]
                rpos, dtype, detail, einfo = compute_runtime(vpos, drw, bmd)
                verts.append({
                    'pi': pi, 'vpos': vpos, 'rpos': rpos,
                    'drw': drw, 'dtype': dtype, 'detail': detail, 'einfo': einfo,
                    'mi': mi, 'li': li
                })
    return verts

# ============================================================================
# Main
# ============================================================================
def fv(v): return f"({v[0]:.2f}, {v[1]:.2f}, {v[2]:.2f})"

def analyze(path, label):
    print(f"\n{'='*60}")
    print(f"{label}: {path}")
    print(f"{'='*60}")
    data = path.read_bytes()
    sec = find_sections(data)
    for t, (o, s) in sorted(sec.items(), key=lambda x: x[1][0]):
        print(f"  {t}: 0x{o:X} (0x{s:X})")

    jp, bo = parse_inf1(data, sec['INF1'][0])
    joints = parse_jnt1(data, sec['JNT1'][0])
    envs, ib = parse_evp1(data, sec['EVP1'][0])
    iw, dd = parse_drw1(data, sec['DRW1'][0])
    pos = parse_vtx1(data, sec['VTX1'][0], sec['VTX1'][1])
    batches = parse_shp1(data, sec['SHP1'][0])
    bw = build_bone_worlds(joints, jp)

    print(f"  JNT1: {len(joints)} joints, EVP1: {len(envs)} envs, {len(ib)} inv_bind")
    print(f"  DRW1: {len(iw)} entries, VTX1: {len(pos)} positions, SHP1: {len(batches)} batches")
    print(f"  INF1 batch order: {bo}")

    return {'pos': pos, 'batches': batches, 'is_w': iw, 'drw_d': dd,
            'envs': envs, 'inv_bind': ib, 'bone_world': bw, 'joints': joints, 'bo': bo}

def main():
    orig = analyze(ORIG_BMD, "ORIGINAL")
    exp = analyze(EXPORT_BMD, "EXPORT")

    # DRW1 comparison
    print(f"\n{'='*60}")
    print("DRW1 COMPARISON")
    print(f"{'='*60}")
    ndiff = 0
    for i in range(min(len(orig['is_w']), len(exp['is_w']))):
        if orig['is_w'][i] != exp['is_w'][i] or orig['drw_d'][i] != exp['drw_d'][i]:
            ndiff += 1
            if ndiff <= 30:
                print(f"  [{i}] orig(w={orig['is_w'][i]},d={orig['drw_d'][i]}) vs exp(w={exp['is_w'][i]},d={exp['drw_d'][i]})")
    print(f"Total DRW1 diffs: {ndiff}/{min(len(orig['is_w']), len(exp['is_w']))}")

    # Batch overview
    print(f"\n{'='*60}")
    print("BATCH OVERVIEW")
    print(f"{'='*60}")
    for bi in range(len(orig['batches'])):
        b = orig['batches'][bi]
        nv = sum(len(p['points']) for pk in b['packets'] for p in pk['primitives'])
        has_mtx = any(a['attrib'] == 0 for a in b['attribs'])
        print(f"  Batch {bi}: mtype=0x{b['matrixType']:04X}, pkts={len(b['packets'])}, verts~{nv}, hasMtxIdx={has_mtx}")

    # Analyze weighted batches
    for tb in range(len(orig['batches'])):
        ob = orig['batches'][tb]
        has_mtx = any(a['attrib'] == 0 for a in ob['attribs'])
        if not has_mtx:
            continue  # skip rigid batches

        if tb >= len(exp['batches']):
            continue

        print(f"\n{'='*60}")
        print(f"BATCH {tb} (WEIGHTED) DETAILED COMPARISON")
        print(f"{'='*60}")

        ov = get_batch_verts(orig, tb)
        ev = get_batch_verts(exp, tb)
        print(f"  Original: {len(ov)} vert refs")
        print(f"  Export:   {len(ev)} vert refs")

        or_ = sum(1 for v in ov if v['dtype']=='rigid')
        ow_ = sum(1 for v in ov if v['dtype']=='weighted')
        er_ = sum(1 for v in ev if v['dtype']=='rigid')
        ew_ = sum(1 for v in ev if v['dtype']=='weighted')
        print(f"  Orig: {or_} rigid, {ow_} weighted")
        print(f"  Exp:  {er_} rigid, {ew_} weighted")

        # DRW usage
        def drw_usage(verts, bmd, label):
            usage = {}
            for v in verts:
                k = (v['drw'], v['dtype'])
                usage[k] = usage.get(k, 0) + 1
            print(f"\n  {label} DRW usage:")
            for (di,dt), cnt in sorted(usage.items()):
                extra = ""
                if dt == 'weighted':
                    env = bmd['envs'][bmd['drw_d'][di]]
                    extra = f" env={list(zip(env['indices'],[round(w,3) for w in env['weights']]))}"
                else:
                    extra = f" bone={bmd['drw_d'][di]}"
                print(f"    DRW[{di}] {dt} (d={bmd['drw_d'][di]}): {cnt}{extra}")

        drw_usage(ov, orig, "Original")
        drw_usage(ev, exp, "Export")

        # Compare runtime positions
        n = min(len(ov), len(ev))
        close = med = far = ext = 0
        mismatches = []
        for i in range(n):
            d = dist3(ov[i]['rpos'], ev[i]['rpos'])
            if d < 1: close += 1
            elif d < 10: med += 1
            elif d < 100: far += 1
            else: ext += 1
            if d > 1:
                mismatches.append((i, d, ov[i], ev[i]))

        print(f"\n  Runtime position comparison ({n} verts):")
        print(f"    <1: {close}, 1-10: {med}, 10-100: {far}, >100: {ext}")

        mismatches.sort(key=lambda x: -x[1])
        print(f"\n  Top 20 worst mismatches:")
        for rank, (idx, d, o, e) in enumerate(mismatches[:20]):
            print(f"\n    #{rank+1} idx={idx} dist={d:.1f}")
            print(f"      VTX pos: orig[{o['pi']}]={fv(o['vpos'])} exp[{e['pi']}]={fv(e['vpos'])}")
            print(f"      Runtime: orig={fv(o['rpos'])} exp={fv(e['rpos'])}")
            print(f"      DRW: orig[{o['drw']}] {o['dtype']}(d={orig['drw_d'][o['drw']]}) | exp[{e['drw']}] {e['dtype']}(d={exp['drw_d'][e['drw']]})")
            if o['einfo']: print(f"      Orig env: {o['einfo']}")
            if e['einfo']: print(f"      Exp env:  {e['einfo']}")
            if o['dtype'] != e['dtype']:
                print(f"      *** TYPE MISMATCH ***")

        # Root cause
        tm = sum(1 for _,_,o,e in mismatches if o['dtype']!=e['dtype'])
        dm = sum(1 for _,_,o,e in mismatches if o['dtype']==e['dtype'] and o['drw']!=e['drw'])
        pm = sum(1 for _,_,o,e in mismatches if o['drw']==e['drw'] and o['pi']!=e['pi'])
        sm = sum(1 for _,_,o,e in mismatches if o['drw']==e['drw'] and o['pi']==e['pi'])

        print(f"\n  ROOT CAUSE ({len(mismatches)} mismatches):")
        print(f"    Type mismatch (rigid/weighted): {tm}")
        print(f"    DRW index mismatch:             {dm}")
        print(f"    Pos index mismatch (same DRW):  {pm}")
        print(f"    Same DRW+pos (vtx pos diff):    {sm}")

if __name__ == '__main__':
    main()
