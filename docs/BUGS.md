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

Status: fixed; source fix in `MoveBG/Item.cpp`

Fixed in:
- `TNozzleBox::load()` now initializes the box packet material colors with
  `GX_TEVREG1`, matching the original assembly immediate `2`.

Verified:
- User test passed on 2026-06-20 with deployed source-linked DOL
  `EFA30B2A601F548466EC30AD2F5C68F684D04221`.

Symptom:
- Delfino Plaza boots with the BUG 0001 minimum, but FLUDD nozzle boxes render
  incorrectly/weirdly in some source-linked combinations.

Isolation notes:
- The regression appeared after removing `MoveBG/Item.cpp` from the
  original-linked set.
- `MoveBG/Item.cpp` is therefore not needed for BUG 0001 boot, but may be
  required for this rendering issue or may expose a separate source bug.
- Reconfirmed on 2026-06-20: original-linking only `MoveBG/Item.cpp` fixed the
  item/nozzle box rendering issue.
- Source bug found in `TNozzleBox::load()`: the three
  `TMapObjBase::initPacketMatColor()` calls used `GX_TEVREG2`.
- Original assembly for those calls passes immediate `2`, which corresponds to
  `GX_TEVREG1` in the local SDK enum (`GX_TEVPREV`, `GX_TEVREG0`,
  `GX_TEVREG1`, `GX_TEVREG2`).
- Fixed source-linked DOL deployed:
  `EFA30B2A601F548466EC30AD2F5C68F684D04221`
- User test result: fixed. Item/nozzle boxes render correctly again.

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

## BUG 0008 - FLUDD nozzle/action state behaves like rocket

Status: complete; source-linked `Player/WaterGun.cpp`

Symptom:
- The normal FLUDD water nozzle behaves like the rocket nozzle and crashes
  when used.
- The correct nozzle must render and enter the normal spray action instead of
  the rocket action.

Current isolation notes:
- First-pass original-links the likely FLUDD/nozzle emission and player action
  cluster:
  - `Player/WaterGun.cpp`
  - `Player/MarioMove.cpp`
  - `Player/MarioJump.cpp`
  - `Player/MarioDraw.cpp`
  - `Player/MarioEffect.cpp`
  - `Player/ModelWaterManager.cpp`
- First-pass isolation deployed DOL:
  `3F02D68822E71253C74D517C641764360359C1B8`
- Isolation batch 2 keeps only these original-linked:
  - `Player/WaterGun.cpp`
  - `Player/MarioMove.cpp`
  - `Player/MarioJump.cpp`
- Isolation batch 2 deployed DOL:
  `17B3D0EF91AE905C82476B0F5A9BC284A8C9EE49`
- Batch 2 result: nozzle/action side works. FLUDD shows the correct nozzle and
  pretends to shoot, but visible spray water is still missing. Split that into
  BUG 0009 and continue isolating this action-state bug first.
- Isolation batch 3 source-links `Player/MarioJump.cpp` again, leaving only:
  - `Player/WaterGun.cpp`
  - `Player/MarioMove.cpp`
- Isolation batch 3 deployed DOL:
  `ACE85792C467A93AD11860C10251EF6378850E5B`
- Batch 3 result: still works correctly, so `Player/MarioJump.cpp` is not
  required for this action-state fix.
- Isolation batch 4 source-links `Player/MarioMove.cpp` again, leaving only:
  - `Player/WaterGun.cpp`
- Isolation batch 4 deployed DOL:
  `BE9FDC2B7B325916F8022244D6512314D9A52D19`
- Source-linked test imported the mostly matched external `Player/WaterGun.cpp`
  and adapted it to local headers/API names. `Player/WaterGun.cpp` is now
  marked `Equivalent` so the DOL links our source instead of the retail object.
- Source-linked WaterGun deployed DOL:
  `7A933B0C886A57A8C09529A98215309D2C2EFF7B`
- White nozzle texture regression was confirmed to disappear when
  `Player/WaterGun.cpp` was linked as retail/original. Retail asm passes body
  texture slot 1 (`getResTIMG(1)`, pointer offset `+0x20`) into
  `SMS_ChangeTextureAll`; our source was using slot 0. Patched and redeployed
  source-linked WaterGun DOL:
  `8BB1CA1C96AA22B797358781AC119A52F22160FE`
- Runtime result: fixed. Normal nozzle/action state works and the nozzle
  texture renders correctly with source-linked `Player/WaterGun.cpp`.

## BUG 0009 - FLUDD spray water does not render

Status: fixed; source-linked `Player/ModelWaterManager.cpp` and
`Player/WaterGun.cpp`

Symptom:
- FLUDD enters the correct spray/nozzle state and pretends to shoot.
- Visible spray water does not render from the nozzle.

Current notes:
- Split from BUG 0008 after isolation batch 2. The missing water stream is
  likely in the restored render/effect/water-manager half, not the current
  nozzle/action-state isolation target.
- The source-linked WaterGun import also corrected local nozzle model callback
  wiring for spray, hover, and turbo nozzle joints, so this DOL should be used
  to test both nozzle action state and visible spray rendering.
- If nozzle textures are still white after
  `8BB1CA1C96AA22B797358781AC119A52F22160FE`, continue in
  `TWaterGun::init()` around nozzle model setup and texture replacement.
- Isolation batch 1 keeps source-linked `Player/WaterGun.cpp` and
  original-links likely downstream water/effect render TUs:
  - `Player/ModelWaterManager.cpp`
  - `Player/MarioEffect.cpp`
  - `Player/MarioParticle.cpp`
  - `Player/SplashManager.cpp`
  - `System/EmitterViewObj.cpp`
- Isolation batch 1 deployed DOL:
  `56B896F304F3BEB39A5A715388A9580E95ECCA16`
- Batch 1 result: visible FLUDD water renders, so BUG 0009 is inside this
  downstream batch, not `Player/WaterGun.cpp`.
- Isolation batch 2 keeps only `Player/ModelWaterManager.cpp` original-linked;
  `Player/MarioEffect.cpp`, `Player/MarioParticle.cpp`,
  `Player/SplashManager.cpp`, and `System/EmitterViewObj.cpp` are source-linked
  again.
- Isolation batch 2 deployed DOL:
  `7353EEA00CE362ED452B4D5241B69923B7E097BF`
- Source pass 1 focuses on the two missing/rough render-bound functions in
  `Player/ModelWaterManager.cpp`:
  - `TModelWaterManager::calcWorldMinMax()` now follows the retail scalar
    min/max scan and is 97.1% size-matched against retail asm.
  - `TModelWaterManager::calcDrawVtx(MtxPtr)` now rebuilds the retail
    view-space water quad path and is 90.2% size-matched against retail asm.
  - `Player/ModelWaterManager.cpp` is source-linked again as `Equivalent`.
- Source pass 1 deployed DOL:
  `B1CA4994A3192FE420EBC07D37F22F718D68D946`
- Source fix:
  - `TModelWaterManager::calcDrawVtx(MtxPtr)` now uses the retail airborne
    water-particle draw scale: `1.414f * (0.5f * particleSize)`. The previous
    source used `1.5f * (2.0f * particleSize)`, making in-air spray quads far
    too large even though the renderer path was otherwise structurally close.
  - `TNozzleDeform::emit(int)` now uses the retail analog-pressure size lerp:
    `(pressure - sizeMinPressure) / (sizeMaxPressure - sizeMinPressure)`.
    The previous source divided by `(sizeMaxPressure - pressure)`, which made
    feathered trigger pressure produce unstable particle sizes.
- Fixed deployed DOL:
  `4E82FFC3EAC7DD90D4E048022A105F80852771E6`
- User test result: fixed. FLUDD spray water renders correctly, including when
  using less analog trigger pressure.

## BUG 0010 - Dolpic fog/mist planes spawn in wrong positions

Status: fixed; source fix in `JSystem/JParticle/JPAEmitter.cpp`

Symptom:
- In Delfino Plaza/Dolpic, persistent fog or mist planes appear in the main
  play area at incorrect positions.

Current isolation notes:
- First-pass Dolpic/object-side isolation did not fix the misplaced planes:
  - `MoveBG/MapObjInit.cpp`
  - `MoveBG/MapObjDolpic.cpp`
  - `MoveBG/MapObjEx.cpp`
  - `Map/MapEventDolpic.cpp`
  - `Map/BathWaterManager.cpp`
- First-pass deployed DOL:
  `484B2789B2F463D0F79EAFACBA14826BD943C284`
- User test result: not fixed.
- Second-pass JParticle/effect isolation fixed or changed the issue enough to
  confirm the responsible code is somewhere in this batch:
  - `JSystem/JParticle/JPAEmitter.cpp`
  - `JSystem/JParticle/JPAEmitterManager.cpp`
  - `JSystem/JParticle/JPAParticle.cpp`
  - `JSystem/JParticle/JPADraw.cpp`
  - `JSystem/JParticle/JPADrawVisitor.cpp`
  - `JSystem/JParticle/JPABaseShape.cpp`
  - `JSystem/JParticle/JPAResourceManager.cpp`
  - `JSystem/JParticle/JPATexture.cpp`
  - `MarioUtil/EffectUtil.cpp`
  - `System/EmitterViewObj.cpp`
- Second-pass deployed DOL:
  `C41FEE0A68A8EBBBF16CC7F56A06D981439EDA70`
- User test result: isolated somewhere in this batch.
- Third-pass source-links the lower-probability resource/manager/texture and
  view-object side again, leaving only this original-linked subset:
  - `JSystem/JParticle/JPAEmitter.cpp`
  - `JSystem/JParticle/JPAParticle.cpp`
  - `JSystem/JParticle/JPADraw.cpp`
  - `JSystem/JParticle/JPADrawVisitor.cpp`
  - `MarioUtil/EffectUtil.cpp`
- Third-pass deployed DOL:
  `D4F4C033D651885437CA47300B603C95E5026B9D`
- User test result: BUG 0010 is not present and particles are working. The
  issue is therefore still inside this five-TU subset, or requires a
  combination within it.
- Fourth-pass keeps only the particle draw executors original-linked:
  - `JSystem/JParticle/JPADraw.cpp`
  - `JSystem/JParticle/JPADrawVisitor.cpp`
- Fourth-pass deployed DOL:
  `900608556903AE59C68560D40BBBDD662125A8EA`
- User test result: BUG 0010 appeared again and particles were gone. Therefore
  `JPADraw.cpp` and `JPADrawVisitor.cpp` alone are not sufficient; at least one
  of `JPAEmitter.cpp`, `JPAParticle.cpp`, or `MarioUtil/EffectUtil.cpp` is also
  required.
- Fifth-pass adds `JPAEmitter.cpp` back to the original-linked draw pair:
  - `JSystem/JParticle/JPAEmitter.cpp`
  - `JSystem/JParticle/JPADraw.cpp`
  - `JSystem/JParticle/JPADrawVisitor.cpp`
- Fifth-pass deployed DOL:
  `89AEA245A85C62BF1D44A448B5FF36AEA0575AC3`
- User test result: BUG 0010 is gone and particles render. This proves
  `JPAEmitter.cpp` is required for the currently working isolation set, while
  `JPAParticle.cpp` and `MarioUtil/EffectUtil.cpp` are not required.
- Sixth-pass keeps only `JPAEmitter.cpp` original-linked:
  - `JSystem/JParticle/JPAEmitter.cpp`
- Sixth-pass deployed DOL:
  `F10FBC7903498FC082806C95F0A46674BBBE4EFD`
- User test result: BUG 0010 is gone and particles render. This isolates the
  Dolpic fog/mist placement issue to source-linked `JPAEmitter.cpp`.
- Source fix:
  - In `JPABaseEmitter::calcEmitterGlobalParams`, the emitter translation was
    being stored on the scale-only matrix and `JPAEmitterInfoObj.unk24` was read
    back from that matrix.
  - Retail behavior stores `mTrans` on the copied rotation matrix, concatenates
    that through `JPAEmitterInfoObj.unk9C`, then reads `unk24` from the
    transformed matrix.
- Source-linked deployed DOL:
  `50D190094B991D59D58DDBE277ED876904C16526`
- User test result: appears fixed.

## BUG 0011 - General particles not rendering

Status: fixed; source fix in `JSystem/JParticle/JPAEmitter.cpp`

Symptom:
- Some general particles were not rendering.

Current isolation notes:
- The same second-pass JParticle/effect isolation used for BUG 0010 caused
  particles to start rendering again:
  - `JSystem/JParticle/JPAEmitter.cpp`
  - `JSystem/JParticle/JPAEmitterManager.cpp`
  - `JSystem/JParticle/JPAParticle.cpp`
  - `JSystem/JParticle/JPADraw.cpp`
  - `JSystem/JParticle/JPADrawVisitor.cpp`
  - `JSystem/JParticle/JPABaseShape.cpp`
  - `JSystem/JParticle/JPAResourceManager.cpp`
  - `JSystem/JParticle/JPATexture.cpp`
  - `MarioUtil/EffectUtil.cpp`
  - `System/EmitterViewObj.cpp`
- Deployed DOL:
  `C41FEE0A68A8EBBBF16CC7F56A06D981439EDA70`
- User test result: particles render again with this batch original-linked.
- Third-pass BUG 0010 isolation also kept particles working with only these
  original-linked:
  - `JSystem/JParticle/JPAEmitter.cpp`
  - `JSystem/JParticle/JPAParticle.cpp`
  - `JSystem/JParticle/JPADraw.cpp`
  - `JSystem/JParticle/JPADrawVisitor.cpp`
  - `MarioUtil/EffectUtil.cpp`
- Deployed DOL:
  `D4F4C033D651885437CA47300B603C95E5026B9D`
- Fourth-pass BUG 0010 isolation kept only `JPADraw.cpp` and
  `JPADrawVisitor.cpp` original-linked.
- Deployed DOL:
  `900608556903AE59C68560D40BBBDD662125A8EA`
- User test result: particles were gone, so at least one of
  `JPAEmitter.cpp`, `JPAParticle.cpp`, or `MarioUtil/EffectUtil.cpp` is also
  required for BUG 0011.
- Fifth-pass added `JPAEmitter.cpp` back to the original-linked draw pair.
- Deployed DOL:
  `89AEA245A85C62BF1D44A448B5FF36AEA0575AC3`
- User test result: particles render, clearing `JPAParticle.cpp` and
  `MarioUtil/EffectUtil.cpp` as required TUs for this working subset.
- Sixth-pass keeps only `JPAEmitter.cpp` original-linked.
- Deployed DOL:
  `F10FBC7903498FC082806C95F0A46674BBBE4EFD`
- User test result: particles still render, isolating the general missing
  particle issue to source-linked `JPAEmitter.cpp`.
- Source-linked deployed DOL:
  `50D190094B991D59D58DDBE277ED876904C16526`
- User test result: appears fixed by the same
  `JPABaseEmitter::calcEmitterGlobalParams` translation fix as BUG 0010.

## BUG 0012 - Heatwave effects render poorly

Status: candidate fixed; source fix in `JSystem/JParticle/JPAEmitter.cpp`

Symptom:
- Heatwave/distortion effects render poorly.

Current isolation notes:
- While testing BUG 0010/BUG 0011, the user noticed that the previous heatwave
  rendering issue also appears to be fixed by original-linking only:
  - `JSystem/JParticle/JPAEmitter.cpp`
- Deployed DOL:
  `F10FBC7903498FC082806C95F0A46674BBBE4EFD`
- User test result: particles render, BUG 0010 is gone, and the previous
  heatwave rendering issue appears to be isolated here as well.
- Source-linked deployed DOL:
  `50D190094B991D59D58DDBE277ED876904C16526`
- User test result: BUG 0010/BUG 0011 appear fixed. Re-check heatwave if it
  needs separate confirmation.

## BUG 0013 - Mario ledgegrab climb regression

Status: open; testing `Player/MarioSpecial.cpp`

Symptom:
- Mario often fails to ledgegrab, and when he does grab a ledge he can get stuck
  instead of climbing up.

Current notes:
- The ledge hang/climb path is in `TMario::hanging()` and the hang landing cases
  in `Player/MarioSpecial.cpp`.
- A sister-project `MarioSpecial.cpp` comparison found the local hang movement
  branch had several suspicious divergences:
  - the ground check for side movement used `targetPos.y + 10.0f` instead of
    `targetPos.y + 50.0f`,
  - the follow-up wall probe used a 20-unit forward offset instead of 30,
  - successful wall transfer placed Mario from the earlier wall-check center
    instead of the corrected follow-up wall-check center,
  - the final animation branch was reversed, playing ledge-move animations when
    Mario had not actually moved along the ledge.
- Source-linked candidate deployed DOL:
  `233C5C703129D086DAA335E170ED08E2E8FA173D`
- User test result: Mario still often fails to ledgegrab.
- Isolation test original-links only `Player/MarioSpecial.cpp` while keeping the
  JPAEmitter particle fix source-linked.
- Original-`MarioSpecial.cpp` isolation deployed DOL:
  `FCD46A102E7E653A18E473076B48C3F08A3B5953`
- Next result needed: if this build fixes ledgegrab/climb, the offending object
  is `Player/MarioSpecial.cpp`; otherwise continue the Mario/action isolation
  outward.

## BUG 0014 - Actor meshes teleport around on screen

Status: isolated; testing common actor/model stack

Symptom:
- Pianta meshes jump/teleport around on screen, including positions in the air.
- Birds showed the same teleporting behavior, so this is not Pianta-only NPC
  logic.

Current isolation notes:
- Broad original-link batch fixed the teleporting/despawn behavior:
  - `System/MarNameRefGen_NPC.cpp`
  - all source-linked `NPC/*.cpp` equivalents
  - all source-linked `M3DUtil/*.cpp` equivalents
  - all source-linked `Strategic/*.cpp` equivalents
  - all source-linked `Animal/*.cpp` equivalents
- Broad deployed DOL:
  `DD42969E5937937177C5245D9BB92060E4EBEF61`
- Massive trim kept only the common actor/model stack original-linked:
  - `M3DUtil/M3UJoint.cpp`
  - `M3DUtil/M3UModel.cpp`
  - `M3DUtil/MActor.cpp`
  - `M3DUtil/MActorAnm.cpp`
  - `M3DUtil/MActorData.cpp`
  - `M3DUtil/SDLModel.cpp`
  - `M3DUtil/MActorUtil.cpp`
  - `M3DUtil/SampleCtrlNode.cpp`
  - `Strategic/liveactor.cpp`
  - `Strategic/liveinterp.cpp`
  - `Strategic/livemanager.cpp`
  - `Strategic/ObjHitCheck.cpp`
  - `Strategic/objmanager.cpp`
  - `Strategic/ObjModel.cpp`
  - `Strategic/spcinterp.cpp`
  - `Strategic/Strategy.cpp`
  - `Strategic/question.cpp`
  - `Strategic/HitActor.cpp`
  - `Strategic/MirrorActor.cpp`
- Trimmed deployed DOL:
  `959FCAEA6E6DB47AADB545180712FD647A07F7C3`
- User test result: teleporting is gone, but despawning still happens.
- Next split for this bug should be `M3DUtil` vs `Strategic` after BUG 0015 is
  handled.

## BUG 0015 - Actor meshes despawn randomly

Status: open; isolating despawn separately from BUG 0014

Symptom:
- Pianta meshes randomly despawn.
- Birds also showed despawn-like behavior during broad testing, but Piantas are
  the primary repro target for this bug.
- This is separate from BUG 0014 because the `M3DUtil` + `Strategic` trimmed
  isolation removes teleporting but leaves despawning.

Current isolation notes:
- Broad original-link batch fixed both teleporting and despawning:
  `DD42969E5937937177C5245D9BB92060E4EBEF61`
- Massive trim with only `M3DUtil` + `Strategic` original-linked fixes
  teleporting but not despawning:
  `959FCAEA6E6DB47AADB545180712FD647A07F7C3`
- Pianta-focused test kept the BUG 0014 common-stack isolation and added
  `System/MarNameRefGen_NPC.cpp` plus the non-matching `NPC/*.cpp`
  equivalents original-linked, while leaving `Animal/*.cpp` source-linked:
  `926C0FD6BA433ABB0EEA32BD73979CB82DA93E92`
- User test result: Piantas still despawn.
- Current reconfirmation build restores the broad original-link set by adding
  the non-matching `Animal/*.cpp` equivalents original-linked too:
  `DD42969E5937937177C5245D9BB92060E4EBEF61`
- User test result: fixed. Pianta despawn stops only after adding `Animal`
  back on top of the NPC/common-stack original-link baseline.
- Current split test keeps only the Animal core TUs original-linked:
  `Animal/AnimalBase.cpp`, `Animal/AnimalManager.cpp`, and
  `Animal/AnimalNerve.cpp`; species/behavior TUs are source-linked.
- Animal core split deployed DOL:
  `C9BF7D051C9B053F36C290F5EC928660762FAACB`
- User test result: fixed; Piantas did not despawn.
- Current split test keeps only `Animal/AnimalManager.cpp` original-linked from
  the Animal core set; `Animal/AnimalBase.cpp` and `Animal/AnimalNerve.cpp` are
  source-linked again.
- AnimalManager-only split deployed DOL:
  `A3483D164976BDFB744091A8047D50289CCD81CE`
- User test result: despawning returned, and the repro depends on where Mario
  stands in the world.
- Current split test adds `Animal/AnimalBase.cpp` back to the original-linked
  set while keeping `Animal/AnimalManager.cpp` original-linked and
  `Animal/AnimalNerve.cpp` source-linked. If this fixes the bug, suspect an
  `AnimalBase` position/culling/global-list interaction.
- AnimalBase + AnimalManager split deployed DOL:
  `CD283D7709DAB2A3181BFA53EF4AE1D04DE04946`
- User test result: fixed; Piantas did not despawn.
- Isolated to `Animal/AnimalBase.cpp`: the `AnimalManager`-only build was bad,
  and the only source/original difference between the bad and fixed splits was
  `AnimalBase`.
- Source bug found in `TAnimalBase::perform`: after temporarily replacing
  `j3dSys.mViewMtx` with the animal-local view matrix, the restore call copied
  the live matrix back into the stack save instead of restoring the stack save
  into `j3dSys.mViewMtx`. This leaks an animal transform into later rendering,
  matching the position-dependent Pianta despawn symptom.
- Current verification build leaves all Animal TUs source-linked with the
  `AnimalBase` view-matrix restore fixed.
- Source-fixed verification deployed DOL:
  `1059CE2B0E742D9E39A539589902DD3E7630D5C0`
- Fully source-linked verification for the previously isolated
  `M3DUtil`/`Strategic`/`NPC`/`System/MarNameRefGen_NPC`/`Animal` buckets:
  `18BFF94128D137F9C3ECC42F4D39681F306B8B1B`

## BUG 0016 - Pianta skirt/cloth colors render incorrectly

Status: open; isolating NPC color/material setup

Symptom:
- Pianta skirt/cloth geometry renders, but its color/material result is wrong
  compared with the rest of the model. It looks like bad NPC cloth color,
  material, or packet color setup rather than missing geometry.

Current isolation notes:
- Fully source-linked build after BUG 0015 fix:
  `18BFF94128D137F9C3ECC42F4D39681F306B8B1B`
- Source inspection points at the NPC color/init path:
  - `NPC/NpcInitData.cpp` owns Monte/Pianta body and cloth color tables.
  - `NPC/NpcInitPrg.cpp` calls `SMS_InitChangeNpcColor` during individual
    difference setup.
  - `NPC/NpcColor.cpp` writes those color entries into material packets.
  - `NPC/NpcParts.cpp` applies the same path for attached parts.
- First isolation attempted to original-link `NPC/NpcParts.cpp` too, but the
  original `NpcParts.o` leaves `cNpcPartsNameRootJoint` unresolved while the
  source object provides it, so keep `NpcParts.cpp` source-linked for this
  pass.
- Current first isolation build original-links:
  - `NPC/NpcBase.cpp`
  - `NPC/NpcInitData.cpp`
  - `NPC/NpcInitPrg.cpp`
  - `NPC/NpcColor.cpp`
- First color/init isolation deployed DOL:
  `DD26270E2B33B4485A2981857602B85A5F0321DC`
- User test result: fixed; skirt/cloth colors render correctly with this
  NPC color/init cluster original-linked.
- Current split test keeps only `NPC/NpcInitData.cpp` original-linked from that
  cluster, because it owns the Monte cloth color tables and has many
  nonmatching static color/init objects.
- `NpcInitData`-only split deployed DOL:
  `28476E9DD7E854DD84EA144E176D80F818256B6B`
- User test result: broke again; `NPC/NpcInitData.cpp` alone is not sufficient.
- Current split test keeps only `NPC/NpcColor.cpp` original-linked from the
  fixed color/init cluster.
- `NpcColor`-only split deployed DOL:
  `D0B860F3293FD45ED148CAB71FE9F8D7C9EF5406`
- User test result: not fixed; `NPC/NpcColor.cpp` alone is not sufficient.
- Current split test keeps only `NPC/NpcInitPrg.cpp` original-linked from the
  fixed color/init cluster.
- `NpcInitPrg`-only split deployed DOL:
  `CC801EB8C1336AC77D45791DA6A1EE3F7278EF10`
- User test result: fixed; skirt/cloth colors render correctly.
- BUG 0016 is isolated to source-linked `NPC/NpcInitPrg.cpp`.
- Source fix in `TBaseNPC::setIndividualDifference_`:
  - compute `unk178` from the third color component (`colorData[2]`), not the
    first component, so clean NPCs do not get a bogus pollution amount.
  - skip `_eye_mat` when applying fallback pollution KColor packets; the source
    had the condition inverted and only touched the eye material.
  - only clear Peach parasol part-mask bits on the Peach path instead of
    clearing those bits for every non-Peach NPC.
- Source-linked verification deployed DOL:
  `07ED5B5EB43C54EBA88DE2CD45AE4CF685E7F002`

## BUG 0017 - Piantas play dirty/goop stuck animation when clean

Status: isolated with BUG 0016 to `NPC/NpcInitPrg.cpp`

Symptom:
- Before `NPC/NpcInitPrg.cpp` was original-linked, Piantas were playing the
  stuck-in-goop/dirty animation even when they should be using their normal
  animations.
- With the `NpcInitPrg`-only original-linked build, Piantas return to their
  normal animations.

Current isolation notes:
- Same deployed DOL as the BUG 0016 `NpcInitPrg`-only split:
  `CC801EB8C1336AC77D45791DA6A1EE3F7278EF10`
- This strongly suggests the source bug is in
  `TBaseNPC::setIndividualDifference_` or nearby NPC init code that sets
  individual color/state/action data.
- Source fix is shared with BUG 0016: the wrong pollution amount component made
  clean NPCs look dirty and enter dirty/goop animation selection through
  `npcWaitIn()`.
- Source-linked verification deployed DOL:
  `07ED5B5EB43C54EBA88DE2CD45AE4CF685E7F002`

## BUG 0018 - FLUDD nozzle boxes loop open animation and do not connect nozzle

Status: fixed; source-linked `MoveBG/MapObjBase.cpp`

Symptom:
- FLUDD/nozzle item boxes render correctly after BUG 0002, but when opened they
  play a looping animation repeatedly.
- The inner nozzle item does not connect/attach correctly after the box opens.

Current notes:
- This is separate from BUG 0002's material color regression. BUG 0002 is fixed
  by the `TNozzleBox::load()` TEV register correction in `MoveBG/Item.cpp`.
- Original-linking `MoveBG/Item.cpp` alone did not fix this issue, so the
  source fix for BUG 0002 is not the cause of this state/animation bug.
- Broad map-object support isolation fixed the issue:
  - `MoveBG/MapObjBase.cpp`
  - `MoveBG/MapObjGeneral.cpp`
  - `MoveBG/MapObjManager.cpp`
  - `MoveBG/ItemManager.cpp`
- Broad map-object support deployed DOL:
  `019E7ABA2874273B8D690A6B6E9C14BDF4767510`
- Trimmed split kept only these original-linked and the issue stayed fixed:
  - `MoveBG/MapObjBase.cpp`
  - `MoveBG/MapObjGeneral.cpp`
- Trimmed split deployed DOL:
  `F0C42F7821AF607260497C35F8B4AD2A63331C02`
- `MoveBG/MapObjManager.cpp`, `MoveBG/ItemManager.cpp`, and `MoveBG/Item.cpp`
  are source-linked in the trimmed fixed build.
- Final isolation kept only `MoveBG/MapObjBase.cpp` original-linked and both
  BUG 0018 and BUG 0019 stayed fixed.
- Source candidate in `TMapObjBase::makeObjAppeared()`:
  - map-collision setup now checks map-object flag `0x8`, matching original
    `rlwinm. ..., 28, 28`; source had incorrectly checked `0x10`.
  - fallback collision matrix Y now uses `mPosition.y - mYOffset` after the
    temporary appearance Y adjustment, matching the original extra `fsubs`.
- Source-linked candidate deployed DOL:
  `0502B8D4B84D99D1AF29783A19A1DE2BC4EA3C50`
- User test result: not fixed. Nozzle boxes still loop the opening animation
  indefinitely and do not shoot the nozzle item out.
- Second source candidate in `TMapObjBase::perform()`:
  - the pre-calc block that can call `hasMapCollision()` and clear the calc
    phase now only runs inside the same `gpMarDirector->unk124` state gate as
    the original assembly.
  - this matches the retail branch shape where non-1/2 director states skip
    that block and continue into the common update path instead of stripping
    the model/animation calc phase unconditionally.
- Second source-linked candidate deployed DOL:
  `29771E597E032E82D1AE8DA8C9C4CEF11A29B6AB`
- User test result: fixed. Nozzle boxes stop looping and correctly shoot the
  nozzle item out.

## BUG 0019 - Save select blocks do not move correctly

Status: fixed; source-linked `MoveBG/MapObjBase.cpp`

Symptom:
- Save select blocks in the save select menu were not moving properly.

Current notes:
- User observed this bug was fixed by the same link set that keeps BUG 0018
  fixed.
- Verified fixed with only these original-linked:
  - `MoveBG/MapObjBase.cpp`
  - `MoveBG/MapObjGeneral.cpp`
- Source-linked in the same build:
  - `MoveBG/MapObjManager.cpp`
  - `MoveBG/ItemManager.cpp`
  - `MoveBG/Item.cpp`
- Deployed DOL:
  `F0C42F7821AF607260497C35F8B4AD2A63331C02`
- Because this affects a menu object and BUG 0018 affects a nozzle box, the
  common suspect is shared `TMapObjBase` / `TMapObjGeneral` animation or state
  transition logic rather than item-specific code.
- Final isolation kept only `MoveBG/MapObjBase.cpp` original-linked and both
  BUG 0018 and BUG 0019 stayed fixed.
- Candidate source fix is shared with BUG 0018: the `makeObjAppeared()`
  map-collision setup now uses the original flag `0x8` and original collision
  Y value.
- Source-linked candidate deployed DOL:
  `0502B8D4B84D99D1AF29783A19A1DE2BC4EA3C50`
- User test result: not fixed. Save select blocks still stay stationary when
  jumped into.
- Second source candidate is shared with BUG 0018: `TMapObjBase::perform()`
  now gates the pre-calc map-collision phase clear like the original assembly.
- Second source-linked candidate deployed DOL:
  `29771E597E032E82D1AE8DA8C9C4CEF11A29B6AB`
- User test result: fixed. Save select blocks animate upward when jumped into.

## BUG 0020 - Nozzle pickup does not update current nozzle

Status: fixed; source-linked `MoveBG/Item.cpp`

Symptom:
- After a nozzle box opens and Mario collects the spawned nozzle item, the
  active/current nozzle does not update.

Current notes:
- The box opening and item ejection path is fixed by BUG 0018, so this is now
  the separate pickup path for the spawned `TItemNozzle`.
- Original `TItemNozzle::touchPlayer(THitActor*)` passes Mario's actor pointer
  into the virtual at vtable offset `0x1E0`.
- The `TItemNozzle` vtable shows offset `0x1E0` is `TItem::taken(THitActor*)`.
- The source incorrectly called `TItemNozzle::put()` at vtable offset `0x1A0`,
  skipping the normal item-taken message sent to Mario.
- Candidate source fix:
  - `TItemNozzle::touchPlayer()` now calls `taken(actor)` when Mario touches a
    collectable nozzle item, matching the original call target and preserving
    the nozzle event path.
- Source-linked candidate deployed DOL:
  `3977251E90F3F94D57CE600F1DDB3CA606688948`
- User test result: fixed. Collecting nozzles from boxes now updates Mario's
  current nozzle.

## BUG 0021 - NPCs and signs always show the same dialogue line

Status: fixed in current link set; `System/EventWatcher.cpp` remains
original-linked

Symptom:
- NPCs and signs do not display their intended dialogue.
- They repeatedly show the same line instead:
  `あっ! きがつけば もうこんなに そとが あかるいぞ!!`

Current notes:
- Because both NPCs and signs show the wrong line, the shared talk/message
  path is more likely than NPC-specific behavior.
- Broad original-link isolation fixed the issue:
  - `System/EventWatcher.cpp`
  - `System/TalkCursor.cpp`
  - `GC2D/Talk2D2.cpp`
  - `GC2D/MessageLoader.cpp`
- Broad isolation deployed DOL:
  `071E5106E17436CA14729B11149FE3B0CD02E848`
- User test result: fixed. NPCs and signs showed correct dialogue with the
  broad talk-stack isolation.
- Current trim source-links likely-cleared support TUs again:
  - `System/TalkCursor.cpp`
  - `GC2D/MessageLoader.cpp`
- Current trim keeps original-linked:
  - `System/EventWatcher.cpp`
  - `GC2D/Talk2D2.cpp`
- Trimmed isolation deployed DOL:
  `6158261BCB90D07278C61DE0604316C66DFAC761`
- User test result: still fixed. `System/TalkCursor.cpp` and
  `GC2D/MessageLoader.cpp` are cleared for this bug.
- Current one-TU isolation source-links `System/EventWatcher.cpp` again and
  keeps only `GC2D/Talk2D2.cpp` original-linked.
- `GC2D/Talk2D2.cpp`-only isolation deployed DOL:
  `32ED422ADE3D40CDDF2A6A45E723C52D28CA4A4C`
- User test result: not fixed. NPCs/signs still repeat one dialogue line,
  though the repeated line changed to the 30-second crate message. This means
  `GC2D/Talk2D2.cpp` original-linked alone is not sufficient.
- Current opposite one-TU isolation source-links `GC2D/Talk2D2.cpp` again and
  keeps only `System/EventWatcher.cpp` original-linked.
- `System/EventWatcher.cpp`-only isolation deployed DOL:
  `FAE6CE0CBDAAD5A8502E7DEF6643C0AAF94E9BCB`
- User test result: not fixed. NPCs/signs still repeat one dialogue line,
  again the `あっ! きがつけば...` line. This means
  `System/EventWatcher.cpp` original-linked alone is not sufficient either;
  the fixed broad set depends on both `System/EventWatcher.cpp` and
  `GC2D/Talk2D2.cpp`.
- Current source attempt restores both TUs to source-linked and rewrites
  `evSetTalkMsgID` to keep the two popped `TSpcSlice` values as explicit
  temporaries before converting them, matching the original assembly's
  source-level data flow more closely.
- Source-attempt deployed DOL:
  `084193234313DDC959805FEC05E94AE5F22CDFD0`
- User test result: not fixed. NPCs still repeated the
  `あっ! きがつけば...` line.
- `System/EventWatcher.cpp` pop-shape experiment was reverted because it did
  not affect the runtime bug and made `evSetTalkMsgID` less accurate.
- Original `TTalk2D2::setMessageID` calls `setupTextBox(data, entry)` after
  selecting the message entry. `setupTextBox` delegates to `setupBoardTextBox`
  only when `unk28` marks the talking actor as a board/sign. The source was
  incorrectly calling `setupBoardTextBox(data, entry)` directly from
  `setMessageID`, bypassing the normal NPC dialogue text setup path.
- Source candidate deployed DOL:
  `FEE06CBED1BC5665310CA45B206B815422565F0C`
- User test result: not fixed. The repeated line changed to the 30-second
  crate/minigame line, which means the `setupTextBox` call-target fix changed
  the displayed entry but the message ID/loader selection is still wrong.
- Current isolation keeps the corrected source `GC2D/Talk2D2.cpp` linked and
  original-links only `System/EventWatcher.cpp`.
- Corrected-`Talk2D2` plus original-`EventWatcher` isolation deployed DOL:
  `D829B9A674FA6F6618E595D54CAC71321F8364EA`
- User test result: fixed. NPCs and signs now show the correct dialogue with
  the corrected source `GC2D/Talk2D2.cpp` and original-linked
  `System/EventWatcher.cpp`.
- Current working set keeps `System/EventWatcher.cpp` marked `NonMatching` in
  `configure.py`; the remaining source-side issue is in that TU.
