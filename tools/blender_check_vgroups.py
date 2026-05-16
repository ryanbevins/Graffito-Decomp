"""
Blender headless script to import a BMD and check vertex groups.
"""
import sys
import bpy

# Don't delete default objects yet - might need scene context

# Enable the addon
try:
    bpy.ops.preferences.addon_enable(module='blemd-master')
    print("Addon enabled")
except Exception as e:
    print(f"Addon enable error: {e}")

# Import the BMD
bmd_path = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"
print(f"\nImporting: {bmd_path}")

try:
    result = bpy.ops.import_mesh.bmd(filepath=bmd_path, frc_cr_bn=True, nat_bn=False)
    print(f"Import result: {result}")
except Exception as e:
    print(f"Import exception: {e}")
    import traceback
    traceback.print_exc()

# Check all data blocks
print(f"\nbpy.data.objects: {len(bpy.data.objects)}")
for obj in bpy.data.objects:
    print(f"  {obj.name}: type={obj.type}")

print(f"\nbpy.data.meshes: {len(bpy.data.meshes)}")
for m in bpy.data.meshes:
    print(f"  {m.name}: verts={len(m.vertices)}")

print(f"\nbpy.data.armatures: {len(bpy.data.armatures)}")
for a in bpy.data.armatures:
    print(f"  {a.name}: bones={len(a.bones)}")

# Check collections
print(f"\nCollections:")
for col in bpy.data.collections:
    print(f"  {col.name}: {len(col.objects)} objects")

# Check scene view layers
print(f"\nScene objects:")
for scene in bpy.data.scenes:
    print(f"  Scene '{scene.name}': {len(scene.objects)} objects")
    for obj in scene.objects:
        print(f"    {obj.name}: type={obj.type}")
