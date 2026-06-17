# Audit verdict: equivalent

Date: 2026-06-14 02:11am MNL
Mode: AUDIT
Unit: `mario/Enemy/yunbo`

Verdict: `equivalent`

Source-link proof:
- `python configure.py --non-matching && ninja` linked successfully with
  `Enemy/yunbo.cpp` source-linked as `Object(Equivalent, ...)`.
- Follow-up normal `python configure.py && ninja` passed and verified
  `build/GMSJ01/mario.dol: OK`.

Behavior review:
- Fixed one real blocker before certification: `TYumboManager::load()` now keeps
  the allocated `TYumboParams` pointer and, after `TSmallEnemyManager::load()`,
  writes target inherited hit-param overrides:
  `mSLAttackRadius=97`, `mSLAttackHeight=225`,
  `mSLDamageRadius=90`, `mSLDamageHeight=225`.
- `TNerveYumboFreeze`, `Attack`, `Appearing`, `Hiding`, and `Dancing` execute
  the same animation, sound, sight/give-up, particle, and nerve-transition
  behavior. Remaining diffs are frame size, static singleton owner labels, and
  equivalent temporary/register shape.
- `TYumbo::shotSeeds()` now performs the target seed selection, animation reset,
  randomized vertical lift, speed normalization, yaw/spread quaternion rotation,
  seed life fetch, position/velocity/life/scale setup, and hit-flag wakeup.
  Remaining low fuzzy is loop-unroll choice, stack/FPR allocation, and quaternion
  expression scheduling.
- `TYumbo::moveObject()`, `perform()`, `behaveToWater()`, `receiveMessage()`,
  and `init()` perform the same particle/model/seed/hit-flag/spine behavior.
  Residue is current-nerve reload vs cached local, singleton owner labels,
  rodata label ownership, and loop/register coloring.
- `TYumboSeed::perform()` and `init()` are behavior-identical: hidden early-out,
  draw-pass matrix/model update, move-pass gravity/friction/collision attack,
  life expiry hide/collision flag, and parent name-ref list insertion all match.
  Residue is stack layout, pointer-loop lowering, constant materialization, and
  JGadget iterator construction shape.

Byte-debt:
- The TU still has nonmatching data/rodata/sdata labels and many weak/helper
  owner extras, but none are undefined in the source-link build and the visible
  operations are behavior-equivalent.
