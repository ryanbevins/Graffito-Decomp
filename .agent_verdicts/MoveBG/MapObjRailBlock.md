# MoveBG/MapObjRailBlock audit

Verdict: `equivalent`

Checked 2026-06-15 5:08am MNL in AUDIT mode.

The previous blocker is fixed. `TRollBlock::calcRootMatrix()` now uses the
target `65536.0f / 360.0f` degree-to-short-angle scale for all three base
rotations and the rolling sine-table angle. The former missing `@2693`
constant now emits; `decomp-diff -s missing` reports no rows.

Behavior review notes:
- `TWoodBlock::load()` differs because target keeps the base rail-load body
  behind a call while the build inlines the same operations. The base-load
  sequence, collision setup, four color reads, color stores, and
  `SMS_InitPacket_OneTevColor(..., GX_TEVREG0, &unk164)` tail match behavior.
- `TRailBlock::control()` preserves the same base control, damage refresh,
  riding/recycle/rail-flag gates, rail advance, speed override, step
  recomputation, matrix accumulation, identity snap, rail pitch/yaw/roll
  storage, rotation-step division, and ongoing rotation wrap semantics.
- `TRailMapObj::{moveToNextNode,resetPosition,initGraphTracer}`,
  `TNormalLift::{control,readRailFlag,setGroundCollision,resetPosition}`,
  `TWoodBlock::calcRecycle`, and `TRollBlock::calcRootMatrix` reviewed as
  behavior-equivalent with only stack/register/FPR/helper-boundary residue.
- Remaining extras are weak/helper owner drift (`TMapObjBase::~TMapObjBase`,
  `TTakeActor::moveRequest`, `TRailMapObj::~TRailMapObj`,
  `TMapCollisionBase::moveMtx`, `gekko_ps_copy12`, JDrama adjustors) and small
  rodata owner drift, not unresolved references.

Proof:
- `python configure.py --non-matching && ninja` linked from source and emitted
  `build/GMSJ01/mario.dol`.
- `python configure.py && ninja` restored normal config and reported
  `build/GMSJ01/mario.dol: OK`.
