"""
Run this in Blender's Script Editor to debug DRW matching.
It inspects the mesh's vertex groups and checks which vertices
would fail to find their EVP1 envelope.
"""

import bpy
import json
from pathlib import Path

# Get the mesh object (assumes Mario model is selected or named)
mesh_obj = None
for obj in bpy.data.objects:
    if obj.type == 'MESH' and obj.data.vertices:
        if len(obj.data.vertices) > 100:  # main mesh
            mesh_obj = obj
            break

if mesh_obj is None:
    print("ERROR: No suitable mesh found")
else:
    print(f"Found mesh: {mesh_obj.name}, {len(mesh_obj.data.vertices)} vertices")

    mesh = mesh_obj.data
    mesh.calc_loop_triangles()

    # Check armature
    arm_obj = mesh_obj.parent
    if arm_obj and arm_obj.type == 'ARMATURE':
        bone_names = [b.name for b in arm_obj.data.bones]
        print(f"Armature: {arm_obj.name}, {len(bone_names)} bones")

        # Map vgroup index -> bone index
        vgroup_to_bone_idx = {}
        for vg in mesh_obj.vertex_groups:
            if vg.name in bone_names:
                vgroup_to_bone_idx[vg.index] = bone_names.index(vg.name)

        print(f"Vertex groups mapped to bones: {len(vgroup_to_bone_idx)}")

        # Analyze ALL vertices
        single_bone = 0
        multi_bone = 0
        no_bone = 0
        multi_bone_sets = {}  # frozenset -> count
        multi_bone_fail_sets = {}  # bone sets that don't match any EVP

        # EVP1 bone sets (from the original BMD analysis)
        evp_bone_sets = [
            frozenset([3, 4]), frozenset([3, 4]), frozenset([4, 6]),
            frozenset([2, 8]), frozenset([22, 23]),
            frozenset([2, 8, 14]), frozenset([2, 8]), frozenset([2, 8]),
            frozenset([2, 8, 14]), frozenset([2, 8, 14]),
            frozenset([2, 8]), frozenset([8, 9]),
            frozenset([14, 20]), frozenset([2, 14]),
            frozenset([9, 11]), frozenset([9, 11]),
            frozenset([20, 22]), frozenset([20, 22]), frozenset([20, 22]),
            frozenset([2, 14]), frozenset([20, 22]), frozenset([20, 22]),
            frozenset([14, 21]), frozenset([8, 9]),
            frozenset([15, 16]), frozenset([15, 16]),
            frozenset([16, 17]),
            frozenset([2, 3]), frozenset([2, 14]),
            frozenset([2, 3]), frozenset([2, 3]),
            frozenset([2, 3, 14]),
            frozenset([2, 3]),
            frozenset([14, 15]), frozenset([15, 16]), frozenset([15, 16]),
            frozenset([2, 3]),
            frozenset([15, 16]), frozenset([14, 15]),
            frozenset([15, 16]),
            frozenset([20, 22]),
            frozenset([14, 20, 22]),
            frozenset([15, 22])
        ]
        unique_evp_sets = set(evp_bone_sets)
        print(f"Unique EVP1 bone sets: {len(unique_evp_sets)}")

        for vi, vert in enumerate(mesh.vertices):
            sig_groups = []
            for g in vert.groups:
                bi = vgroup_to_bone_idx.get(g.group)
                if bi is not None and g.weight > 0.001:
                    sig_groups.append((bi, g.weight))

            if len(sig_groups) == 0:
                no_bone += 1
            elif len(sig_groups) == 1:
                single_bone += 1
            else:
                multi_bone += 1
                bone_set = frozenset(bi for bi, w in sig_groups)
                multi_bone_sets[bone_set] = multi_bone_sets.get(bone_set, 0) + 1

                if bone_set not in unique_evp_sets:
                    multi_bone_fail_sets[bone_set] = multi_bone_fail_sets.get(bone_set, 0) + 1

        print(f"\nVertex classification:")
        print(f"  No bone:     {no_bone}")
        print(f"  Single bone: {single_bone}")
        print(f"  Multi bone:  {multi_bone}")

        print(f"\nMulti-bone vertex bone sets:")
        for bs, cnt in sorted(multi_bone_sets.items(), key=lambda x: -x[1]):
            match = "MATCH" if bs in unique_evp_sets else "NO MATCH"
            print(f"  {sorted(bs)}: {cnt} verts [{match}]")

        if multi_bone_fail_sets:
            print(f"\n*** BONE SETS WITH NO EVP MATCH ({len(multi_bone_fail_sets)} unique): ***")
            for bs, cnt in sorted(multi_bone_fail_sets.items(), key=lambda x: -x[1]):
                print(f"  {sorted(bs)}: {cnt} verts")
                # Show example vertex
                for vi, vert in enumerate(mesh.vertices):
                    sig_groups = [(vgroup_to_bone_idx[g.group], g.weight)
                                  for g in vert.groups
                                  if vgroup_to_bone_idx.get(g.group) is not None and g.weight > 0.001]
                    if frozenset(bi for bi, _ in sig_groups) == bs:
                        print(f"    Example vert {vi}: {sig_groups}")
                        break
        else:
            print("\nAll multi-bone sets match an EVP envelope!")

        # Also check: which materials (batches) have the failing vertices?
        print(f"\nPer-material breakdown:")
        for mi in range(len(mesh.materials)):
            mat = mesh.materials[mi]
            mat_verts = set()
            for lt in mesh.loop_triangles:
                if lt.material_index == mi:
                    for vi in lt.vertices:
                        mat_verts.add(vi)

            single = 0
            multi = 0
            multi_fail = 0
            for vi in mat_verts:
                vert = mesh.vertices[vi]
                sig_groups = [(vgroup_to_bone_idx[g.group], g.weight)
                              for g in vert.groups
                              if vgroup_to_bone_idx.get(g.group) is not None and g.weight > 0.001]
                if len(sig_groups) <= 1:
                    single += 1
                else:
                    multi += 1
                    bs = frozenset(bi for bi, _ in sig_groups)
                    if bs not in unique_evp_sets:
                        multi_fail += 1

            if mat_verts:
                mname = mat.name if mat else f"(none)"
                print(f"  Material {mi} ({mname}): {len(mat_verts)} verts, {single} single, {multi} multi, {multi_fail} FAIL")
    else:
        print("No armature parent found")
