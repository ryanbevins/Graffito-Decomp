verdict: equivalent
date: 2026-06-14 8:42am MNL
unit: Enemy/Amenbo

Proof:
- `python configure.py --non-matching && ninja` linked `mario.elf` and built
  `mario.dol`.
- Restored normal config with `python configure.py && ninja`; final check printed
  `build/GMSJ01/mario.dol: OK`.

Behavior fixes made during audit:
- `TAmenbo::doAdjustTarget` now uses the target reciprocal multiply
  `frame * (1.0f / 63.0f)`, recovering target rodata `@2942`.
- `TAmenbo::checkMarioWaterIn` now excludes
  `TNerveSmallEnemyChange` (`isFreeze() && !isChangedBlock()`), matching the
  original branch that skips the water-entry reaction while changing block.
- `TAmenbo::init` now indexes effect joints through
  `J3DModelData::getJointName()`, matching the target `J3DModelData+0xb0`
  lookup and the later `getAnmMtx(effect->mJointIdx)` use.
- The shared `TModelWaterManager` particle-slot helpers now read the water hit
  actor slot as a word, matching Amenbo/smallEnemy target `lwz 0x68`; the
  static water hit actor slot writers in `ModelWaterManager` and
  `MarioCheckCol` write the same word slot.

Remaining non-100% text diffs are codegen/ownership class: stack frame size,
register coloring, inline helper labels, function-local static labels, and
source-only weak helper owners. No missing target symbols remain.
