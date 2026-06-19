# ModelWaterManager Matching Tracker

Goal: clear every `mario/Player/ModelWaterManager` function so
`tools/check-diff-noise.py` reports no differences other than ignored
register, stack, branch-target, or commutative-source-swap noise.

Last inventory: 2026-06-20

Verification command pattern:

```powershell
python tools\check-diff-noise.py -u mario/Player/ModelWaterManager -d "<function substring>"
```

## Needs Structural Work

- [ ] `TModelWaterManager::drawShineShadowVolume(float(*)[4])`
  - Current: 96.51%, 1456B
  - Structural: 10
  - Categories: insert-delete=8, instruction-order=2
- [ ] `TModelWaterManager::drawMirror(float(*)[4])`
  - Current: 95.61%, 2028B
  - Structural: 21
  - Categories: insert-delete=11, opcode=5, operand=5
- [ ] `TModelWaterManager::calcVMMtxWall(float(*)[4], float, const JGeometry::TVec3<float>&, const JGeometry::TVec3<float>&, float(*)[4])`
  - Current: 85.99%, 276B
  - Structural: 5
  - Categories: insert-delete=4, operand=1
- [ ] `TModelWaterManager::calcVMMtxGround(float(*)[4], float, const JGeometry::TVec3<float>&, const JGeometry::TVec3<float>&, float(*)[4])`
  - Current: 82.66%, 316B
  - Structural: 11
  - Categories: insert-delete=6, opcode=1, operand=4
- [ ] `TModelWaterManager::move()`
  - Current: 89.47%, 4180B
  - Structural: 145
  - Categories: arg-count=1, insert-delete=74, instruction-order=4,
    opcode=8, operand=58
## Clear By Noise Gate

- [x] `TModelWaterManager::~TModelWaterManager()`
- [x] `TModelWaterManager::perform(unsigned long, JDrama::TGraphics*)`
- [x] `TModelWaterManager::drawRefracAndSpec() const`
- [x] `TModelWaterManager::drawWaterVolume(float(*)[4])`
  - Cleared: 2026-06-19
  - `check-diff-noise`: only ignored stack differences remain.
- [x] `TModelWaterManager::drawSilhouette(float(*)[4])`
  - Cleared: 2026-06-19
  - `check-diff-noise`: only ignored register/stack differences remain.
- [x] `TModelWaterManager::calcDrawVtx(float(*)[4])`
  - Cleared: 2026-06-19
  - `check-diff-noise`: only ignored register/stack differences remain.
- [x] `TModelWaterManager::calcWorldMinMax()`
  - Cleared: 2026-06-19
  - `check-diff-noise`: 100% match, no instruction differences.
- [x] `TModelWaterManager::load(JSUMemoryInputStream&)`
  - Cleared: 2026-06-19
  - `check-diff-noise`: 100% match, no instruction differences.
- [x] `TWaterEmitInfo::TWaterEmitInfo(const char*)`
  - Cleared: 2026-06-19
  - `check-diff-noise`: only ignored stack differences remain.
- [x] `__sinit_ModelWaterManager_cpp`
  - Cleared: 2026-06-19
  - `check-diff-noise`: 100% match, no instruction differences.
- [x] `TModelWaterManager::drawTouching()`
- [x] `TModelWaterManager::calcVMAll(float(*)[4])`
- [x] `TBGCheckData::isWaterSlip() const`
- [x] `TModelWaterManager::garbageCollect()`
- [x] `TModelWaterManager::wind(const JGeometry::TVec3<float>&)`
- [x] `TModelWaterManager::askDoWaterHitCheck()`
- [x] `TModelWaterManager::emitRequest(const TWaterEmitInfo&)`
- [x] `TModelWaterManager::makeEmit(const TWaterEmitInfo&)`
  - Cleared: 2026-06-19
  - `check-diff-noise`: only ignored stack/commutative-source-swap differences
    remain.
- [x] `TModelWaterManager::askHitWaterParticleOnGround(const JGeometry::TVec3<float>&)`
- [x] `TModelWaterManager::loadAfter()`
- [x] `TWaterHitActor::~TWaterHitActor()`

## Unpaired Symbols

These are not directly classified by `check-diff-noise.py` because there is no
paired function body. Track them while resolving ordering, inline emission, and
weak-symbol cleanup.

Missing:

- [ ] `@32@__dt__14TWaterHitActorFv` - 8B

Extra:

- [ ] `init_sphere_glist()` - 4B
- [ ] `TModelWaterManager::drawTouchingMask()` - 172B
- [ ] `TDLTexQuad::reset()` - 24B
- [ ] `THitActor::receiveMessage(THitActor*, unsigned long)` - 8B
- [ ] `SMS_isGetShine(unsigned long, unsigned long, bool)` - 148B
- [ ] `SMS_getShineID(unsigned long, unsigned long, bool)` - 92B
- [ ] `SMS_getNormalStage(unsigned long)` - 28B
- [ ] `JDrama::TViewObj::~TViewObj()` - 100B
- [ ] `JSUList<JALSeModVolFunk>::~JSUList()` - 88B
- [ ] `JSUList<JALSeModPitFunk>::~JSUList()` - 88B
- [ ] `JSUList<JALSeModEffFunk>::~JSUList()` - 88B
- [ ] `JSUList<JALSeModVolDist>::~JSUList()` - 88B
- [ ] `JSUList<JALSeModPitDist>::~JSUList()` - 88B
- [ ] `JSUList<JALSeModEffDist>::~JSUList()` - 88B
- [ ] `JSUList<JALSeModVolFGrp>::~JSUList()` - 88B
- [ ] `JSUList<JALSeModPitFGrp>::~JSUList()` - 88B
- [ ] `JSUList<JALSeModEffFGrp>::~JSUList()` - 88B
- [ ] `JSUList<JALSeModVolDGrp>::~JSUList()` - 88B
- [ ] `JSUList<JALSeModPitDGrp>::~JSUList()` - 88B
- [ ] `JSUList<JALSeModEffDGrp>::~JSUList()` - 88B
- [ ] `JSUList<MSSetSound>::~JSUList()` - 88B
- [ ] `JSUList<MSSetSoundGrp>::~JSUList()` - 88B
- [ ] `JSUList<MSBgm>::~JSUList()` - 88B
- [ ] `THitActor::~THitActor()` - 108B
