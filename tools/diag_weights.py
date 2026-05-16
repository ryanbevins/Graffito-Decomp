import bpy, sys, os, importlib, addon_utils

bpy.ops.wm.read_homefile(use_empty=True)
addon_utils.enable("blemd-master", default_set=True)

BModel_mod = importlib.import_module("blemd-master.BModel")
temp = BModel_mod.BModel()
addon_path = os.path.dirname(importlib.import_module("blemd-master").__file__)
temp.SetBmdViewExePath(addon_path + os.sep)
temp.Import(r'C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd',
    import_anims=False, anim_rot_smallest=False, use_nodes=True,
    imtype='TGA', tx_pck='DO', import_anims_type='SEPARATE',
    ic_sc=True, frc_cr_bn=False, boneThickness=10,
    dvg=False, val_msh=False, paranoia=False, no_rot_cv=False, nat_bn=False)

mesh_obj = None
for obj in bpy.data.objects:
    if obj.type == 'MESH':
        mesh_obj = obj
        break

mesh = mesh_obj.data
mesh.calc_loop_triangles()
print(f'Mesh: {mesh_obj.name}, {len(mesh.loop_triangles)} tris, {len(mesh.materials)} materials')

batch_order_map = {9: 0, 7: 1, 10: 2, 8: 3, 6: 4, 5: 5, 4: 6, 2: 7, 3: 8, 0: 9, 1: 10}

for mi in range(len(mesh.materials)):
    tris = [lt for lt in mesh.loop_triangles if lt.material_index == mi]
    seen = set()
    single = multi = 0
    for lt in tris:
        for vi in lt.vertices:
            if vi in seen:
                continue
            seen.add(vi)
            vert = mesh.vertices[vi]
            sig = [g for g in vert.groups if g.weight > 0.001]
            if len(sig) <= 1:
                single += 1
            else:
                multi += 1
    total = single + multi
    pct = single / total * 100 if total else 0
    verdict = 'RIGID' if single > multi else 'WEIGHTED'
    batch_idx = batch_order_map.get(mi, '?')
    print(f'mat[{mi}] batch={batch_idx}: {len(tris)} tris, single={single} multi={multi} ({pct:.0f}%) -> {verdict}')
