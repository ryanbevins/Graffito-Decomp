"""Quick test to verify the addon code compiles without errors."""
import sys
import bpy
try:
    bpy.ops.preferences.addon_enable(module='blemd-master')
    print("SUCCESS: Addon enabled without errors")
except Exception as e:
    print(f"ERROR: {e}")
    import traceback
    traceback.print_exc()

# Try importing the modified modules
try:
    from importlib import reload
    import importlib
    # Force reload to pick up changes
    mod = importlib.import_module('blemd-master.Shp1')
    reload(mod)
    print(f"Shp1 loaded: {mod}")
    print(f"  PrecomputeVertexDRW: {hasattr(mod.Shp1, 'PrecomputeVertexDRW')}")

    mod2 = importlib.import_module('blemd-master.Vtx1')
    reload(mod2)
    print(f"Vtx1 loaded: {mod2}")

    mod3 = importlib.import_module('blemd-master.BModel_out')
    reload(mod3)
    print(f"BModel_out loaded: {mod3}")

    print("\nAll modules compile OK")
except Exception as e:
    print(f"Module load error: {e}")
    import traceback
    traceback.print_exc()
