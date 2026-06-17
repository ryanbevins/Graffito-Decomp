# Enemy/enemy audit

Verdict: equivalent
Checked: 2026-06-14 5:08pm MNL
Unit: `mario/Enemy/enemy`

## Result

`Enemy/enemy.cpp` is now `Object(Equivalent, ...)`.

Proof:

- `python configure.py --non-matching && ninja` linked with the source object.
- `python configure.py && ninja` restored the matching config and passed
  `build/GMSJ01/mario.dol: OK`.

## Behavior fixes during audit

- `TSpineEnemy::walkToCurPathNode()` now checks the computed
  `JMASin(turn_speed)` for zero before dividing.
- `TSpineEnemy::zigzagToCurPathNode()` now checks the same sine value in its
  copied turn-radius block.

The target asm compares the sine result against zero in both blocks; the prior
source compared an uninitialized local.

## Remaining byte debt

- Missing target weak `JGeometry::TVec3<float>::operator=(...)` is owner debt
  only. The source object has no undefined reference to it and inlines the same
  word-copy body at the call sites.
- Remaining text diffs are stack/register/FPR allocation, helper-boundary, and
  local label drift across the pathing/root-matrix routines.
- `.sdata2` contains the same float constants in a different order; behavior is
  preserved through relocations.
