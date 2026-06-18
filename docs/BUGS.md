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

## BUG 0003 - Mario dies after standing on ground

Status: fixed

Fixed in:
- `TMario::thinkWaterSurface()` now checks `mState & 0x1000` for the fallback
  air-loss/drowning path, matching the original ASM.

Verified:
- User test passed on 2026-06-18 with deployed DOL
  `795D9228B8CBCBCA1E1D89534D5B637BD17A09FA`.

Symptom:
- Mario spawns on the ground successfully, but after some time he dies without
  an obvious normal death cause.
- The death may be caused by periodic damage or an incorrect drowning-style
  state/condition being applied while Mario is grounded.

Current isolation notes:
- This is distinct from the earlier airborne-spawn issue: Mario is now on the
  ground before the delayed death happens.
- Last known booting baseline when this bug was filed:
  - `System/MarDirectorDirect.cpp`: source-linked
  - `MoveBG/MapObjFlag.cpp`: source-linked
  - `Map/Shimmer.cpp`: original-linked / `NonMatching`
  - deployed DOL `359898AEEA95311BEDF292DB2F32779C4D2300F4`
- Isolation batch 1 original-links the first player damage/water/death cluster:
  - `Player/MarioAction.cpp`
  - `Player/MarioCollision.cpp`
  - `Player/MarioMain.cpp`
  - `Player/MarioMove.cpp`
  - `Player/MarioPhysics.cpp`
  - `Player/MarioSpecial.cpp`
  - `Player/MarioWait.cpp`
  - `Player/MarioSwim.cpp`
  - `Player/MarioInit.cpp`
  - `Player/MarioCheckCol.cpp`
  - `Player/MarioReceiveMsg.cpp`
- Isolation batch 1 deployed DOL:
  `765C447ED5D870E43D73826EA03223A4120C8E3A`
- User test result: Mario stopped dying with isolation batch 1.
- Isolation batch 2 keeps the most likely core original-linked but source-links
  lower-probability state/action TUs again:
  - Source-linked again:
    - `Player/MarioAction.cpp`
    - `Player/MarioMain.cpp`
    - `Player/MarioSpecial.cpp`
    - `Player/MarioWait.cpp`
  - Still original-linked:
    - `Player/MarioCollision.cpp`
    - `Player/MarioMove.cpp`
    - `Player/MarioPhysics.cpp`
    - `Player/MarioSwim.cpp`
    - `Player/MarioInit.cpp`
    - `Player/MarioCheckCol.cpp`
    - `Player/MarioReceiveMsg.cpp`
- Isolation batch 2 deployed DOL:
  `108EC5AD1DAA61368B50C467EBB275F9DAE69E51`
- User test result: Mario still did not die with isolation batch 2, so
  `Player/MarioAction.cpp`, `Player/MarioMain.cpp`,
  `Player/MarioSpecial.cpp`, and `Player/MarioWait.cpp` are cleared for this
  bug.
- Isolation batch 3 source-links `Player/MarioInit.cpp` again as the next
  least likely remaining candidate. Still original-linked:
  - `Player/MarioCollision.cpp`
  - `Player/MarioMove.cpp`
  - `Player/MarioPhysics.cpp`
  - `Player/MarioSwim.cpp`
  - `Player/MarioCheckCol.cpp`
  - `Player/MarioReceiveMsg.cpp`
- Isolation batch 3 deployed DOL:
  `473497546E7D113681CFF4A96948A972368DA7CD`
- User test result: Mario still did not die with isolation batch 3, so
  `Player/MarioInit.cpp` is cleared for this bug.
- Isolation batch 4 source-links `Player/MarioMove.cpp` again as the most
  likely remaining candidate because it contains grounded water-surface checks,
  floor damage, death-plane checks, and direct HP decrement paths. Still
  original-linked:
  - `Player/MarioCollision.cpp`
  - `Player/MarioPhysics.cpp`
  - `Player/MarioSwim.cpp`
  - `Player/MarioCheckCol.cpp`
  - `Player/MarioReceiveMsg.cpp`
- Isolation batch 4 deployed DOL:
  `1C467A00B7E9A11E8ECAF7D46C647CE32E34B803`
- User test result: Mario died again with isolation batch 4, isolating the bug
  to source-linked `Player/MarioMove.cpp`.
- Root cause found in `TMario::thinkWaterSurface()`: the source checked
  `mState & 0x8000` for the fallback air-loss/drowning path, but the original
  ASM checks `mState & 0x1000`.
- ASM evidence:
  - Original: `rlwinm. r0, r3, 0, 19, 19`
  - Fixed source build: `rlwinm. r0, r3, 0, 19, 19`
- Fixed test DOL deployed:
  `795D9228B8CBCBCA1E1D89534D5B637BD17A09FA`
- User test result: fixed. Mario no longer dies after standing on the ground.
- BUG 0003 Player-TU isolation was removed after confirmation; all Player TUs
  are source-linked again.
- Post-isolation deployed DOL:
  `A34EC1AC7B499A770532F2ADC1802DF5A167BD66`
