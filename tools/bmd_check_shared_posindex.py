#!/usr/bin/env python3
"""
Check if position indices are shared between batches, which would cause
vertexMultiMatrixEntry to be overwritten during import.
"""
import struct, math
from pathlib import Path

ORIG_BMD = Path(r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd")

class R:
    def __init__(self, data, pos=0):
        self.d = data; self.p = pos
    def seek(self, p): self.p = p
    def u8(self):  v = self.d[self.p]; self.p += 1; return v
    def s16(self): v = struct.unpack_from('>h', self.d, self.p)[0]; self.p += 2; return v
    def u16(self): v = struct.unpack_from('>H', self.d, self.p)[0]; self.p += 2; return v
    def u32(self): v = struct.unpack_from('>I', self.d, self.p)[0]; self.p += 4; return v
    def f32(self): v = struct.unpack_from('>f', self.d, self.p)[0]; self.p += 4; return v

def find_sections(data):
    sections = {}
    r = R(data); r.p = 12; nsec = r.u32(); r.p += 16
    pos = r.p
    for _ in range(nsec):
        if pos + 8 > len(data): break
        tag = data[pos:pos+4].decode('ascii', errors='replace')
        size = struct.unpack_from('>I', data, pos+4)[0]
        sections[tag] = (pos, size); pos += size
    return sections

def parse_drw1(data, off):
    r = R(data, off+8)
    count = r.u16(); r.p += 2
    o_w, o_d = r.u32(), r.u32()
    r.seek(off+o_w); is_w = [r.u8() for _ in range(count)]
    r.seek(off+o_d); drw_d = [r.u16() for _ in range(count)]
    return is_w, drw_d

def parse_evp1(data, off):
    r = R(data, off+8)
    count = r.u16(); r.p += 2
    o0,o1,o2,o3 = r.u32(),r.u32(),r.u32(),r.u32()
    r.seek(off+o0); counts = [r.u8() for _ in range(count)]
    r.seek(off+o1)
    envs = []
    for i in range(count):
        envs.append({'indices': [r.u16() for _ in range(counts[i])], 'weights': []})
    r.seek(off+o2)
    for i in range(count):
        envs[i]['weights'] = [r.f32() for _ in range(counts[i])]
    return envs

def parse_shp1(data, off):
    r = R(data, off+8)
    nbatch = r.u16(); r.p += 2
    off_batches = r.u32(); r.p += 4; r.p += 4
    off_attribs = r.u32()
    off_mtx_table = r.u32()
    off_prim_data = r.u32()
    off_mtx_data = r.u32()
    off_pkt_locs = r.u32()

    batches = []
    for bi in range(nbatch):
        r.seek(off + off_batches + bi * 40)
        batch = {}
        batch['matrixType'] = r.u16()
        batch['numPackets'] = r.u16()
        batch['offsetToAttribs'] = r.u16()
        batch['firstMtxData'] = r.u16()
        batch['firstPktLoc'] = r.u16()

        attribs = []
        r.seek(off + off_attribs + batch['offsetToAttribs'])
        while True:
            a_type = r.u32()
            a_dt = r.u32()
            if a_type == 0xFF: break
            attribs.append({'attrib': a_type, 'dataType': a_dt})
        batch['attribs'] = attribs

        batch['packets'] = []
        for pi in range(batch['numPackets']):
            pkt = {}
            r.seek(off + off_pkt_locs + (batch['firstPktLoc'] + pi) * 8)
            pkt_size = r.u32(); pkt_offset = r.u32()
            r.seek(off + off_mtx_data + (batch['firstMtxData'] + pi) * 8)
            mtx_unk = r.u16(); mtx_count = r.u16(); mtx_first = r.u32()
            r.seek(off + off_mtx_table + mtx_first * 2)
            pkt['matrixTable'] = [r.u16() for _ in range(mtx_count)]

            r.seek(off + off_prim_data + pkt_offset)
            pkt['primitives'] = []
            bread = 0
            while bread < pkt_size:
                ptype = r.u8(); bread += 1
                if ptype == 0 or bread >= pkt_size: break
                count = r.u16(); bread += 2
                prim = {'type': ptype, 'points': []}
                for _ in range(count):
                    pt = {}
                    for attr in attribs:
                        if attr['dataType'] == 1: val = r.u8(); bread += 1
                        elif attr['dataType'] == 3: val = r.u16(); bread += 2
                        else: val = 0
                        if attr['attrib'] == 0:   pt['mtxIdx'] = val
                        elif attr['attrib'] == 9:  pt['posIdx'] = val
                    prim['points'].append(pt)
                pkt['primitives'].append(prim)
            batch['packets'].append(pkt)
        batches.append(batch)
    return batches

def main():
    data = ORIG_BMD.read_bytes()
    sec = find_sections(data)
    is_w, drw_d = parse_drw1(data, sec['DRW1'][0])
    envs = parse_evp1(data, sec['EVP1'][0])
    batches = parse_shp1(data, sec['SHP1'][0])

    # For each batch, collect posIndex -> set of (drw_idx, multiMat)
    # Simulating the import: vertexMultiMatrixEntry[posIndex] = multiMat
    pos_to_multimat = {}  # posIndex -> list of (batch_idx, drw_idx, is_weighted, env_or_bone)

    for bi, batch in enumerate(batches):
        has_mtx = any(a['attrib'] == 0 for a in batch['attribs'])
        for pkt in batch['packets']:
            mt = pkt['matrixTable']
            for prim in pkt['primitives']:
                for pt in prim['points']:
                    pi = pt.get('posIdx', 0)
                    mi = pt.get('mtxIdx', 0)
                    li = mi // 3 if has_mtx else 0
                    drw_idx = mt[li] if li < len(mt) else mt[0] if mt else 0
                    if drw_idx == 0xFFFF:
                        continue

                    w = is_w[drw_idx]
                    d = drw_d[drw_idx]
                    if w:
                        env = envs[d]
                        info = ('weighted', frozenset(env['indices']), dict(zip(env['indices'], env['weights'])))
                    else:
                        info = ('rigid', d, None)

                    if pi not in pos_to_multimat:
                        pos_to_multimat[pi] = []
                    pos_to_multimat[pi].append((bi, drw_idx, info))

    # Find positions used in multiple batches with DIFFERENT multiMat
    conflicts = 0
    print("Position indices shared between batches with different DRW assignments:")
    for pi, entries in sorted(pos_to_multimat.items()):
        # Deduplicate
        unique = set()
        for bi, drw_idx, info in entries:
            unique.add((bi, drw_idx, info[0], info[1]))

        batch_set = set(e[0] for e in unique)
        if len(batch_set) > 1:
            # Shared between batches
            drw_types = set(e[2] for e in unique)
            drw_indices = set(e[1] for e in unique)
            if len(drw_indices) > 1 or len(drw_types) > 1:
                conflicts += 1
                if conflicts <= 30:
                    print(f"\n  posIndex={pi}:")
                    for bi, drw_idx, info in entries[:5]:
                        print(f"    batch={bi}, DRW[{drw_idx}] {info[0]}, data={info[1]}")

    print(f"\nTotal conflicting shared positions: {conflicts}")

    # Also: which batch is processed LAST? That determines the final vertex groups.
    print("\n\nSIMULATING IMPORT: final vertexMultiMatrixEntry per posIndex")
    final_vme = {}  # posIndex -> final multiMat (last write wins)
    for bi, batch in enumerate(batches):
        has_mtx = any(a['attrib'] == 0 for a in batch['attribs'])
        for pkt in batch['packets']:
            mt = pkt['matrixTable']
            for prim in pkt['primitives']:
                for pt in prim['points']:
                    pi = pt.get('posIdx', 0)
                    mi = pt.get('mtxIdx', 0)
                    li = mi // 3 if has_mtx else 0
                    drw_idx = mt[li] if li < len(mt) else mt[0] if mt else 0
                    if drw_idx == 0xFFFF: continue
                    w = is_w[drw_idx]
                    d = drw_d[drw_idx]
                    if w:
                        env = envs[d]
                        multimat = ('weighted', tuple(env['indices']), tuple(env['weights']))
                    else:
                        multimat = ('rigid', (d,), (1.0,))
                    final_vme[pi] = (bi, drw_idx, multimat)

    # Now check: for batch 9 (weighted), what are the ORIGINAL DRW assignments
    # vs what the import would assign (last-write-wins)?
    print("\nBatch 9 vertices: original DRW vs import-assigned vertex groups")
    batch9 = batches[9]
    has_mtx = any(a['attrib'] == 0 for a in batch9['attribs'])
    mismatch_count = 0
    for pkt in batch9['packets']:
        mt = pkt['matrixTable']
        for prim in pkt['primitives']:
            for pt in prim['points']:
                pi = pt.get('posIdx', 0)
                mi = pt.get('mtxIdx', 0)
                li = mi // 3
                drw_idx = mt[li] if li < len(mt) else 0
                if drw_idx == 0xFFFF: continue

                orig_w = is_w[drw_idx]
                orig_d = drw_d[drw_idx]

                # What would the import assign?
                final_bi, final_drw, final_mm = final_vme.get(pi, (None, None, None))

                if final_drw != drw_idx:
                    mismatch_count += 1
                    if mismatch_count <= 20:
                        orig_type = 'weighted' if orig_w else 'rigid'
                        if orig_w:
                            env = envs[orig_d]
                            orig_info = f"env bones={env['indices']}"
                        else:
                            orig_info = f"bone={orig_d}"
                        print(f"  posIdx={pi}: orig DRW[{drw_idx}] {orig_type} ({orig_info})")
                        print(f"    -> import assigns from batch {final_bi}, DRW[{final_drw}] {final_mm[0]}")
                        print(f"       vgroups will be: bones={final_mm[1]}, weights={final_mm[2]}")

    print(f"\nTotal mismatches: {mismatch_count} (vertices whose vgroups differ from original DRW)")

if __name__ == '__main__':
    main()
