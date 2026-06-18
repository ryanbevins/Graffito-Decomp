# Bug Tracking

## BUG 0001 - Delfino Plaza boot/render dependency

Status: fixed

Fixed in:
- `a1737393 Fix MapObjFlag GX strip emission`

Verified:
- Dolphin boot test passed on 2026-06-18 with deployed DOL
  `359898AEEA95311BEDF292DB2F32779C4D2300F4`.

Symptom:
- Delfino Plaza could black-screen during load while ambience continued.
- Some builds booted the plaza but failed to spawn or show Mario.
- Bad `MoveBG/MapObjFlag.cpp` source builds could crash with Dolphin GFX FIFO
  unknown opcode warnings.

Resolution:
- `System/MarDirectorDirect.cpp` is now source-linked in the verified booting
  build.
- `MoveBG/MapObjFlag.cpp` is now source-linked in the verified booting build.
- The final blocker was `TMapObjFlag::draw()`: `GXBegin` declared the original
  ASM strip vertex count, but the source loop emitted one fewer row than that
  count required. GX then consumed later float data as FIFO commands.
- `TMapObjFlag::draw()` now loops with `row < totalRows - step`, matching the
  original strip count behavior.
- `MapObjFlag.cpp` also opts into the out-of-line
  `JGeometry::gekko_ps_copy12` helper, matching the original TU's call shape.

Last verified linkage:
- `System/MarDirectorDirect.cpp`: source-linked
- `MoveBG/MapObjFlag.cpp`: source-linked
- `Map/Shimmer.cpp`: original-linked / `NonMatching`

Notes:
- `Map/Shimmer.cpp` has a source cleanup for explicit `mTranslate` component
  stores, but the last verified booting DOL still kept this TU original-linked.
- If we want a fully source-linked BUG 0001 closure, retest `Map/Shimmer.cpp`
  source-linked after the MapObjFlag FIFO fix.

## BUG 0002 - FLUDD nozzle box rendering regression

Status: open

Symptom:
- Delfino Plaza boots with the BUG 0001 minimum, but FLUDD nozzle boxes render
  incorrectly/weirdly in some source-linked combinations.

Current isolation notes:
- The regression appeared after removing `MoveBG/Item.cpp` from the
  original-linked set.
- `MoveBG/Item.cpp` is therefore not needed for BUG 0001 boot, but may be
  required for this rendering issue or may expose a separate source bug.

Next step:
- Re-test with `MoveBG/Item.cpp` original-linked while keeping the BUG 0001
  booting set otherwise unchanged to confirm whether BUG 0002 is specifically
  caused by source-linked `MoveBG/Item.cpp`.
