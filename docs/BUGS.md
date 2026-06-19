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

## BUG 0004 - Camera flips randomly while walking

Status: fixed; source-linked `MarioUtil/MathUtil.cpp`

Symptom:
- Camera randomly flips to different directions while Mario is walking.

Current isolation notes:
- Isolation batch 1 original-links the highest-probability camera control and
  walking-mode TUs:
  - `Camera/CameraBGCheck.cpp`
  - `Camera/CameraChange.cpp`
  - `Camera/cameragc.cpp`
  - `Camera/CameraInbetween.cpp`
  - `Camera/cameralib.cpp`
  - `Camera/CameraMarioData.cpp`
  - `Camera/CameraNormal.cpp`
  - `Camera/CameraMode.cpp`
  - `Camera/CameraSecureView.cpp`
- Isolation batch 1 deployed DOL:
  `E8018B6B3ECCD25E68C519892EF94CA2CF322B80`
- User test result: not fixed with isolation batch 1.
- Isolation batch 2 keeps batch 1 original-linked and additionally
  original-links likely world/camera trigger TUs:
  - `Camera/CubeManagerBase.cpp`
  - `Camera/CameraMapTool.cpp`
  - `Camera/CubeMapTool.cpp`
- Isolation batch 2 deployed DOL:
  `948AE5E0F04E4805B2AEA18945D074C897D143AC`
- User test result: not fixed with isolation batch 2.
- Isolation batch 3 keeps batches 1-2 original-linked and additionally
  original-links all remaining non-matching-capable Camera TUs:
  - `Camera/CameraCodeControl.cpp`
  - `Camera/CameraHeightPan.cpp`
  - `Camera/CameraNotice.cpp`
  - `Camera/camerasave.cpp`
  - `Camera/camerashake.cpp`
  - `Camera/CameraTalk.cpp`
  - `Camera/lensflare.cpp`
  - `Camera/lensglow.cpp`
  - `Camera/sunmgr.cpp`
  - `Camera/sunmodel.cpp`
  - `Camera/CameraMultiPlayer.cpp`
  - `Camera/CameraJetCoaster.cpp`
  - `Camera/CameraBck.cpp`
  - `Camera/CameraOption.cpp`
  - `Camera/CameraDemo.cpp`
  - `Camera/CameraWarp.cpp`
- Isolation batch 3 deployed DOL:
  `B285297DBFD587B9FAD0490CFF8365F7FF28C29B`
- User test result: not fixed with isolation batch 3. Since all Camera TUs were
  original-linked and the flip still happened, the source bug is probably
  feeding bad position/yaw/state data into the original camera.
- Isolation batch 4 keeps all Camera TUs original-linked and additionally
  original-links Player/input yaw feeder TUs:
  - `System/MarioGamePad.cpp`
  - `Player/MarioCollision.cpp`
  - `Player/MarioJump.cpp`
  - `Player/MarioMove.cpp`
  - `Player/MarioPhysics.cpp`
  - `Player/MarioRun.cpp`
  - `Player/MarioCheckCol.cpp`
- Isolation batch 4 deployed DOL:
  `853AC32C5427090DC899B2F9BBE120580E1D5308`
- User test result: not fixed with isolation batch 4.
- Isolation batch 5 keeps all prior original-linked Camera and Player/input
  feeder TUs and additionally original-links the shared math helper TU:
  - `MarioUtil/MathUtil.cpp`
  - Rationale: this TU provides `matan`, `MsMtxSetXYZRPH`, and rotation helpers
    used by camera polar math, camera cube helpers, player stick yaw, and player
    facing/ground matrix code. It remains source-linked even when Camera and
    Player TUs are original-linked unless isolated separately.
- Isolation batch 5 deployed DOL:
  `A7B1ED114EF1369D6CCBE492E9DEF07F4AFA847A`
- User test result: fixed. Camera flipping stopped, and this also fixed some
  player movement issues.
- After the successful test, all prior Camera/Player/GamePad isolation TUs were
  returned to source-linking. `MarioUtil/MathUtil.cpp` was the isolated
  responsible TU.
- Narrowed isolation deployed DOL:
  `3F2A3CFA477764A9B7D29E2CEABCE78A80007D3E`
- Source-linked candidate deployed DOL:
  `946CB568A19F2F0D55D396B91E38836479932109`
- User test result: fixed. Camera flipping stopped, and related player
  movement issues were also corrected.
- Final source-linked cleanup deployed DOL:
  `DD193C2DB1192D6BE60BEF955A37DF8F6A8FCF3C`
- Later user testing found that top-left Mario/camera movement still triggered
  the direction lock/flip. Root cause was the both-negative quadrant in
  `matan`: the source used the wrong absolute-axis split and table argument
  order compared with original asm.
- Corrected source-linked deployed DOL:
  `DACB0A268CACB9379D144AC264AC2C1CBE8F0566`
- User test result: fixed.

## BUG 0005 - Delfino Plaza music pitch/notes broken

Status: fixed

Symptom:
- Delfino Plaza BGM sounds pitched far down and the notes are wrong/mangled.

Current isolation notes:
- Broad first-pass audio isolation original-linked JAudio interface, JAS
  sequence/track/channel/DSP, and MSound BGM/control suspects:
  - `JSystem/JAudio/JAInterface/JAIBasic.cpp`
  - `JSystem/JAudio/JAInterface/JAIGFrameSequence.cpp`
  - `JSystem/JAudio/JAInterface/JAISound.cpp`
  - `JSystem/JAudio/JAInterface/JAISystemInterface.cpp`
  - `JSystem/JAudio/JASystem/JASChannel.cpp`
  - `JSystem/JAudio/JASystem/JASDSPChannel.cpp`
  - `JSystem/JAudio/JASystem/JASDSPInterface.cpp`
  - `JSystem/JAudio/JASystem/JASTrack.cpp`
  - `JSystem/JAudio/JASystem/JASSeqParser.cpp`
  - `MSound/MSound.cpp`
  - `MSound/MSoundBGM.cpp`
  - `MSound/MSModBgm.cpp`
- Broad batch deployed DOL:
  `402F053389AF3F2BFA832FBF282B17A85AA6D8A7`
- User test result: OSPanic on boot, so the broad interface/channel/DSP swap is
  too wide or crosses badly with source-linked audio init/state.
- Narrow MSound-only isolation original-linked:
  - `MSound/MSound.cpp`
  - `MSound/MSoundBGM.cpp`
  - `MSound/MSModBgm.cpp`
- MSound-only deployed DOL:
  `C4F6B6C71BA695FD6941EAB64AF537507D11DD16`
- User test result: not fixed.
- Current isolation keeps MSound-only and additionally original-links narrow JAS
  sequence/track suspects:
  - `JSystem/JAudio/JASystem/JASTrack.cpp`
  - `JSystem/JAudio/JASystem/JASSeqParser.cpp`
  - `MSound/MSound.cpp`
  - `MSound/MSoundBGM.cpp`
  - `MSound/MSModBgm.cpp`
- Current deployed DOL:
  `29C439E7BBAA8DC3C8FB051D6711DC29252A3C6F`
- User test result: not fixed.
- JAS channel runtime isolation original-linked:
  - `JSystem/JAudio/JASystem/JASChannel.cpp`
- JASChannel-only deployed DOL:
  `DB2FE8CA0785DACC61BAE26AAEE39C651C17DF7E`
- User test result: not fixed.
- Bank/wave metadata isolation original-linked:
  - `JSystem/JAudio/JASystem/JASBankMgr.cpp`
  - `JSystem/JAudio/JASystem/JASBasicWaveBank.cpp`
  - `JSystem/JAudio/JASystem/JASBNKParser.cpp`
  - `JSystem/JAudio/JASystem/JASWSParser.cpp`
- Bank/wave metadata deployed DOL:
  `9E66DBEAEF2F9E5EC87DA6BA6948DD25814600C5`
- User test result: fixed.
- Bank-side half split original-linked:
  - `JSystem/JAudio/JASystem/JASBankMgr.cpp`
  - `JSystem/JAudio/JASystem/JASBNKParser.cpp`
- Bank-side half split deployed DOL:
  `FE710B77A8EA26BB1CACE7B3DA616DE26084D786`
- User test result: fixed.
- JASBankMgr-only isolation original-linked:
  - `JSystem/JAudio/JASystem/JASBankMgr.cpp`
- JASBankMgr-only deployed DOL:
  `B6CA9F4AE60A9A7FB69D7CDC5E1502119956D02A`
- User test result: fixed. BUG 0005 is isolated to
  `JSystem/JAudio/JASystem/JASBankMgr.cpp`.
- Source fix: `BankMgr::noteOn` was initializing `chan->unk50` from
  `TInstParam::unk18`, but the original asm uses the next float field,
  `TInstParam::unk1C`, as the channel pitch multiplier.
- Source-linked deployed DOL:
  `12A84D44183B64D72DDBBBD3B65F1C7B4027CE3A`
- User test result: fixed.

## BUG 0006 - Mario shadow not rendering

Status: open; isolating likely shadow/render TUs

Symptom:
- Mario's ground shadow is missing.

Current isolation notes:
- First-pass core Mario shadow pipeline original-linked:
  - `MarioUtil/ShadowUtil.cpp`
  - `M3DUtil/MActor.cpp`
  - `Strategic/liveactor.cpp`
  - `Player/MarioMain.cpp`
  - `Player/MarioInit.cpp`
- First-pass deployed DOL:
  `9F5684F6717046D05BBA511659FF94FAA2BA003A`
- User test result: fixed; Mario shadow renders.
- Second-pass trimmed isolation removed least-likely generic/model TUs and keeps
  only:
  - `MarioUtil/ShadowUtil.cpp`
  - `Player/MarioMain.cpp`
  - `Player/MarioInit.cpp`
- Second-pass deployed DOL:
  `283DBBFA40828094B7736FF5A1043C0B5170C85A`
- User test result: fixed; Mario shadow renders.
- Third-pass isolation source-links `MarioUtil/ShadowUtil.cpp` again, leaving
  only:
  - `Player/MarioMain.cpp`
  - `Player/MarioInit.cpp`
- Third-pass deployed DOL:
  `1C5C7DA6771EAAC7C922CCEEDAD0F0ED4352C58D`
- User test result: not fixed; Mario shadow does not render.
- This rules out `Player/MarioMain.cpp` and `Player/MarioInit.cpp` as
  sufficient fixes by themselves, and points back to `MarioUtil/ShadowUtil.cpp`.
- Fourth-pass isolation keeps only:
  - `MarioUtil/ShadowUtil.cpp`
- Fourth-pass deployed DOL:
  `512308AA7763132C9FC14CC14D0BF6A1C969C926`

## BUG 0007 - Ground pound sticks in landing pose

Status: complete; source-linked `Player/MarioWait.cpp`

Symptom:
- When Mario performs a ground pound, he can get stuck on the ground forever
  in the ground-pound pose and cannot transition out of the state.

Current isolation notes:
- First-pass original-linked the likely hip-drop, jump landing, and special
  state/collision cluster:
  - `Player/MarioJump.cpp`
  - `Player/MarioPhysics.cpp`
  - `Player/MarioSpecial.cpp`
  - `Player/MarioWait.cpp`
- First-pass deployed DOL:
  `0ECFAC85F42B2364430A087639D634242DE692D4`
- User test result: fixed; ground pound works.
- Trimmed isolation source-linked `Player/MarioPhysics.cpp` and
  `Player/MarioSpecial.cpp` again, leaving only:
  - `Player/MarioJump.cpp`
  - `Player/MarioWait.cpp`
- Trimmed deployed DOL:
  `12CBC9572C4C455A93540A88D1EEFE8598775F21`
- User test result: still fixed; ground pound works.
- Final isolation source-links `Player/MarioJump.cpp` again, leaving only:
  - `Player/MarioWait.cpp`
- Final isolation deployed DOL:
  `3E7D5A83BE9F5A493F74AF80821B6054ED8B5D99`
- User test result: still fixed; ground pound works.
- BUG 0007 is isolated to source-linked `Player/MarioWait.cpp`.
- Source fix:
  - `TMario::waitMain()` now handles slip-end as `0x0C00023E`, matching the
    original switch routing. The previous source used `0x0C00023F`, which sent
    the landing/end state through the wrong default path.
  - `TMario::waitMain()` now compares `mAction == 0x04000440` directly for
    the u-turn jump end transition, matching the original compare shape.
  - `TMario::squating()` was reshaped to keep the water-gun pointer flow close
    to original ASM; remaining differences are stack/register allocation noise.
- Source-linked deployed DOL:
  `DCF4055D7D00E02804E4BD8022668577A41F1CEA`
- User test result: still fixed; ground pound works with source-linked
  `Player/MarioWait.cpp`.
- Follow-up source cleanup:
  - `TMario::waitMain()` now lets unhandled switch cases fall through to the
    initially-zero result, matching the original tail instead of emitting a
    redundant default block.
  - The squat-landing case now preserves the `changePlayerStatus()` return
    through a shared local result handoff, matching the original branch shape.
- Follow-up source-linked deployed DOL:
  `D18F943B8624A9DD6FE600ACF9C8125376F947DE`
- Final cleanup:
  - `TMario::canPut()` now uses the original inline/accessor source shape:
    `JMASSin(mFaceAngle.y)`, `JMASCos(mFaceAngle.y)`, and
    `mHeldObject->getDamageRadius()`.
  - `TMario::canPut()` is now a 100% instruction match.
- Final source-linked deployed DOL:
  `8CE88E923E08BB0122BD185528ABEA93D7D215AF`
