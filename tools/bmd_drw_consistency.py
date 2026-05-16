#!/usr/bin/env python3
"""
Check if SHP1.FromBlenderMesh can produce matrixIndex lookups that fall back to 0
because the DRW index isn't in the packet's matrix table.

This runs in Blender context to access the actual mesh data.
"""

import sys
import os

# This script must be run inside Blender
try:
    import bpy
except ImportError:
    print("This script must be run inside Blender!")
    print('Usage: blender --background --python bmd_drw_consistency.py')
    sys.exit(1)

import importlib
import logging

logging.basicConfig(level=logging.WARNING)

# Clear scene and import
bpy.ops.wm.read_homefile(use_empty=True)
import addon_utils
addon_utils.enable("blemd-master", default_set=True)

BModel_mod = importlib.import_module("blemd-master.BModel")
BModel_out = importlib.import_module("blemd-master.BModel_out")
Shp1_mod = importlib.import_module("blemd-master.Shp1")
Vtx1_mod = importlib.import_module("blemd-master.Vtx1")

orig_path = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"

# Import the BMD
temp = BModel_mod.BModel()
addon_path = os.path.dirname(importlib.import_module("blemd-master").__file__)
temp.SetBmdViewExePath(addon_path + os.sep)
temp.Import(orig_path,
    import_anims=False, anim_rot_smallest=False, use_nodes=True,
    imtype='TGA', tx_pck='DO', import_anims_type='SEPARATE',
    ic_sc=True, frc_cr_bn=False, boneThickness=10,
    dvg=False, val_msh=False, paranoia=False, no_rot_cv=False, nat_bn=False)

print("[CHECK] Import complete")

# Find the mesh object
mesh_obj = BModel_out.find_mesh_object(temp)
if mesh_obj is None:
    print("ERROR: Cannot find mesh object")
    sys.exit(1)

print(f"[CHECK] Mesh object: {mesh_obj.name}")

mesh = mesh_obj.data
mesh.calc_loop_triangles()

jnt = temp.jnt
drw = temp.drw
evp = temp.evp

# Extract batch order from INF1
batch_mat_order = BModel_out._extract_batch_material_order(temp.inf)
batch_mat_order.sort(key=lambda x: x[0])
batch_order = [blender_mat for _, blender_mat in batch_mat_order]

# Run PrecomputeVertexDRW (same as export path)
vert_drw, mat_classification = Shp1_mod.Shp1.PrecomputeVertexDRW(
    mesh_obj, drw, evp, batch_order)

print(f"[CHECK] PrecomputeVertexDRW: {len(vert_drw)} vertices assigned")

# Now simulate what FromBlenderMesh does for weighted batches
# Build the same lookup tables
has_armature = (mesh_obj.parent is not None and mesh_obj.parent.type == 'ARMATURE')
bone_names = []
vgroup_to_drw = {}
vgroup_to_bone_idx = {}
bone_set_to_drw_candidates = {}
bone_to_any_weighted_drw = {}

if has_armature and mesh_obj.vertex_groups:
    arm_obj = mesh_obj.parent
    bone_names = [b.name for b in arm_obj.data.bones]
    for vg in mesh_obj.vertex_groups:
        if vg.name in bone_names:
            bone_idx = bone_names.index(vg.name)
            vgroup_to_bone_idx[vg.index] = bone_idx
            for di, (isW, data_val) in enumerate(zip(drw.isWeighted, drw.data)):
                if not isW and data_val == bone_idx:
                    vgroup_to_drw[vg.index] = di
                    break

    if evp is not None:
        for di, (isW, data_val) in enumerate(zip(drw.isWeighted, drw.data)):
            if isW and data_val < len(evp.weightedIndices):
                mm = evp.weightedIndices[data_val]
                bone_key = frozenset(mm.indices)
                weight_map = {mm.indices[i]: mm.weights[i] for i in range(len(mm.indices))}
                if bone_key not in bone_set_to_drw_candidates:
                    bone_set_to_drw_candidates[bone_key] = []
                bone_set_to_drw_candidates[bone_key].append((di, weight_map))
                for bi in mm.indices:
                    if bi not in bone_to_any_weighted_drw:
                        bone_to_any_weighted_drw[bi] = di

# Group triangles by material
mat_triangles = {}
for lt in mesh.loop_triangles:
    mi = lt.material_index
    if mi not in mat_triangles:
        mat_triangles[mi] = []
    mat_triangles[mi].append(lt)

# Check each weighted batch
total_fallbacks = 0
total_mismatches = 0

for batch_idx, mat_idx in enumerate(batch_order):
    is_rigid, batch_drw, batch_bone = mat_classification.get(mat_idx, (False, None, None))
    if is_rigid:
        continue

    triangles = mat_triangles.get(mat_idx, [])
    if not triangles:
        continue

    # Simulate _split_into_packets
    packet_splits = Shp1_mod.Shp1._split_into_packets(
        triangles, mesh, vgroup_to_drw, bone_set_to_drw_candidates,
        bone_to_any_weighted_drw, vgroup_to_bone_idx, max_bones=10)

    print(f"\n[CHECK] Batch {batch_idx} (mat={mat_idx}, WEIGHTED): {len(triangles)} tris, {len(packet_splits)} packets")

    for pkt_idx, (pkt_tris, pkt_drw_list) in enumerate(packet_splits):
        local_mtx_map = {drw_idx: local_idx for local_idx, drw_idx in enumerate(pkt_drw_list)}

        fallback_count = 0
        mismatch_count = 0

        for lt in pkt_tris:
            for i in (0, 2, 1):
                vert_idx = lt.vertices[i]
                vert = mesh.vertices[vert_idx]

                # What FromBlenderMesh would compute for matrixIndex
                shp1_drw_idx = Shp1_mod.Shp1._get_vert_drw_index_weighted(
                    vert, vgroup_to_drw, bone_set_to_drw_candidates,
                    bone_to_any_weighted_drw, vgroup_to_bone_idx)
                local_idx = local_mtx_map.get(shp1_drw_idx, 0)

                # Check if fallback to 0 happened
                if shp1_drw_idx not in local_mtx_map:
                    fallback_count += 1
                    if fallback_count <= 5:
                        print(f"  FALLBACK! pkt={pkt_idx} vert={vert_idx}: "
                              f"drw={shp1_drw_idx} not in pkt_drw_list={pkt_drw_list}")
                        # What would the correct DRW be?
                        precomp = vert_drw.get(vert_idx)
                        print(f"    PrecomputeVertexDRW says: {precomp}")

                # Check consistency with PrecomputeVertexDRW
                precomp = vert_drw.get(vert_idx)
                if precomp is not None:
                    precomp_drw_idx = precomp[0]
                    if precomp_drw_idx != shp1_drw_idx:
                        mismatch_count += 1
                        if mismatch_count <= 5:
                            print(f"  MISMATCH! pkt={pkt_idx} vert={vert_idx}: "
                                  f"PrecomputeVertexDRW={precomp_drw_idx} "
                                  f"FromBlenderMesh={shp1_drw_idx}")

        if fallback_count > 0:
            print(f"  Packet {pkt_idx}: {fallback_count} FALLBACKS (drw not in matrix table)")
        if mismatch_count > 0:
            print(f"  Packet {pkt_idx}: {mismatch_count} MISMATCHES (PrecomputeVertexDRW != FromBlenderMesh)")
        total_fallbacks += fallback_count
        total_mismatches += mismatch_count

print(f"\n{'='*60}")
print(f"TOTAL FALLBACKS: {total_fallbacks}")
print(f"TOTAL MISMATCHES: {total_mismatches}")
print(f"{'='*60}")

if total_fallbacks == 0 and total_mismatches == 0:
    print("ALL CLEAR: VTX1 and SHP1 DRW assignments are consistent")
else:
    print("BUG FOUND: Inconsistency between VTX1 and SHP1 DRW assignments!")
