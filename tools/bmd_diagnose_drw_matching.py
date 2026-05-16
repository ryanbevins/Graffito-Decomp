#!/usr/bin/env python3
"""
Diagnose WHY the DRW matching fails for weighted vertices.
The issue: vertices have bone weights from Blender vertex groups that
don't exactly match any EVP1 envelope, so they fall back to rigid.

This script checks: for each vertex in the export's weighted batch,
what are its Blender vertex group weights, and which EVP1 envelope
should it match?
"""

import struct, math
from pathlib import Path

ORIG_BMD = Path(r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd")
EXPORT_BMD = Path(r"C:\Users\ryana\Downloads\reconstruct_test.bmd")

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
    r = R(data)
    r.seek(0); r.p += 4; r.p += 4; fsize = r.u32(); nsec = r.u32(); r.p += 16
    pos = r.p
    for _ in range(nsec):
        if pos + 8 > len(data): break
        tag = data[pos:pos+4].decode('ascii', errors='replace')
        size = struct.unpack_from('>I', data, pos+4)[0]
        sections[tag] = (pos, size)
        pos += size
    return sections

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

def parse_drw1(data, off):
    r = R(data, off+8)
    count = r.u16(); r.p += 2
    o_w, o_d = r.u32(), r.u32()
    r.seek(off+o_w); is_w = [r.u8() for _ in range(count)]
    r.seek(off+o_d); drw_d = [r.u16() for _ in range(count)]
    return is_w, drw_d

def main():
    data = ORIG_BMD.read_bytes()
    sec = find_sections(data)
    envs = parse_evp1(data, sec['EVP1'][0])
    is_w, drw_d = parse_drw1(data, sec['DRW1'][0])

    print("EVP1 Envelopes (original BMD):")
    for i, env in enumerate(envs):
        bone_set = frozenset(env['indices'])
        weights = {env['indices'][j]: env['weights'][j] for j in range(len(env['indices']))}
        print(f"  Env[{i}]: bones={sorted(bone_set)}, weights={weights}")

    print(f"\nDRW1 entries that are weighted:")
    weighted_drws = []
    for i, (w, d) in enumerate(zip(is_w, drw_d)):
        if w:
            env = envs[d]
            bone_set = frozenset(env['indices'])
            print(f"  DRW[{i}]: evp_idx={d}, bones={sorted(bone_set)}, weights={dict(zip(env['indices'], env['weights']))}")
            weighted_drws.append((i, d, bone_set))

    # Now let's see what unique bone sets exist
    print(f"\nUnique bone sets in weighted DRW entries:")
    bone_sets = {}
    for di, evp_idx, bs in weighted_drws:
        if bs not in bone_sets:
            bone_sets[bs] = []
        bone_sets[bs].append((di, evp_idx))
    for bs, entries in sorted(bone_sets.items(), key=lambda x: sorted(x[0])):
        print(f"  {sorted(bs)}: {len(entries)} DRW entries -> {entries}")

    # Key question: how many UNIQUE bone sets are there?
    print(f"\n{len(bone_sets)} unique bone sets in weighted DRW entries")

    # Check: are there vertices in the original batch 9 that use
    # bone combos NOT present in any EVP1 envelope?
    print("\nAll EVP1 bone sets:")
    evp_bone_sets = set()
    for env in envs:
        bs = frozenset(env['indices'])
        evp_bone_sets.add(bs)
        print(f"  {sorted(bs)}")
    print(f"\n{len(evp_bone_sets)} unique bone sets in EVP1")

    # The matching logic in Shp1.py builds bone_set_to_drw_candidates from DRW+EVP.
    # If a Blender vertex has weights on bones {A, B} and there's no EVP envelope
    # with exactly bones {A, B}, it falls back to rigid.
    #
    # But Blender should preserve vertex groups from import exactly...
    # UNLESS the mesh was modified or vertex groups got merged/split.
    #
    # The REAL issue might be simpler: during import, vertices in weighted batches
    # get transformed by the blended matrix and their vertex groups come from the
    # EVP1 envelope. But after Blender processes the mesh (triangulation, etc.),
    # do the vertex groups survive intact?
    #
    # Actually, the more likely issue: the Blender vertex has weights that are
    # NORMALIZED differently. If Blender normalizes weights (sum=1) but EVP1
    # had weights that didn't sum to 1, the bone set would match but weights differ.
    # That's handled by closest-weight matching.
    #
    # OR: the vertex has EXTRA bone weights (e.g., from being at a seam between
    # rigid and weighted areas). A vertex might pick up a tiny weight from
    # additional bones through Blender's weight painting or skinning.

    print("\n\nLet's check what the Blender export actually produces...")
    print("The _get_vert_drw_index_weighted function filters groups with weight > 0.001")
    print("Then builds frozenset of bone indices from those groups")
    print("Then looks up in bone_set_to_drw_candidates")
    print("")
    print("If the bone set doesn't match ANY EVP1 envelope exactly, it falls back to rigid.")
    print("")
    print("HYPOTHESIS: The bone set matching is too strict. A vertex with bones {2, 8, 14}")
    print("won't match EVP envelope {2, 8} because the sets differ.")
    print("This could happen if Blender adds small weights from nearby bones.")
    print("")
    print("SOLUTION: Instead of requiring exact bone set match, we should:")
    print("1. First try exact match (current behavior)")
    print("2. If no match, try matching with subsets (drop low-weight bones)")
    print("3. OR: normalize to match - find the EVP envelope whose bone set is closest")

    # Let's verify by checking how Blender imports weighted vertices
    # In the import code (Matrix44.py/BModel.py), weighted vertices get the
    # transform: sum(w_i * bone_world_i @ inv_bind_i) for each DRW weighted entry.
    # The vertex groups assigned = the bones in the EVP envelope with their weights.
    # So if import is faithful, the vertex groups should exactly match EVP envelopes.

    # BUT if the mesh gets modified in Blender (even just triangulation), vertices
    # at boundaries could get interpolated weights, changing the bone set.

if __name__ == '__main__':
    main()
