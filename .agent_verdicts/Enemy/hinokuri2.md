verdict: equivalent
date: 2026-06-14 6:17am MNL
unit: mario/Enemy/hinokuri2

Proof:
- Promoted `Enemy/hinokuri2.cpp` to `Object(Equivalent, ...)`.
- `python configure.py --non-matching && ninja` linked from source.
- Follow-up `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.

Behavior fixes made during audit:
- `TNerveHino2Pollute::execute()` now transitions BCK `3 -> 0x10`,
  then `0x10 -> 0x11`, and the unclipped pollution transform uses joint
  `0x18`, matching target asm.
- `THinokuri2::moveObject()` now applies the level-1 head-scale interpolation
  as `1.0 + (scale - 1.0) * damageRatio - currentScale`, matching target math.
- `THinokuri2::moveObject()` now uses `mSLHeadHitH` for head hit height instead
  of `mSLBodyScale`.
- `THinokuri2::receiveMessageLv1()` now zeros HP when water damage is greater
  than or equal to current HP; otherwise it subtracts damage.

Equivalent classification:
- Remaining text diffs are stack frame size/slot layout, register allocation,
  local singleton/data owner labels, JGadget iterator temporary shape, matrix
  temporary layout, clamp branch layout, and equivalent operand/order drift.
- `Hino2HeadCallback()` performs the same mode gate, scale/rotation matrix
  construction, and `PSMTXConcat` sequence; its low fuzzy score is temp-matrix
  layout.
- `TNerveHino2Landing::execute()` target has dead frame/flag reads before
  `curAnmEndsNext()`; no side-effecting operation is missing.
- `TWaterHitActor::unk68` is still read as a halfword in rebuilt
  `receiveMessageLv1/Lv2` where target uses a fullword load, but
  `TWaterHitActor::receiveMessage()`/`onWaterHitCounter()` maintain it as a
  small halfword particle slot, so lookup behavior is identical.
- Data drift is rodata/sdata/sdata2 placement and owner-label debt. The visible
  param default stores and names remain behaviorally correct.
