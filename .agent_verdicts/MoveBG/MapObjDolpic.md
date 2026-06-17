# mario/MoveBG/MapObjDolpic

Verdict: equivalent
Status: source_link_proven
Time: 2026-06-14 7:56am MNL

## Proof

- Fixed one remaining behavioral blocker before certification:
  `TMonumentShine::control()` now calls `MsAngleDiff(target, current)` for the
  final snap toward `mInitialRotation.y +/- 360.0f`. Target asm computes
  `target - MsWrap(current, target - 180.0f, target + 180.0f)`; the old source
  passed operands in the opposite order and flipped the correction sign.
- `python configure.py && ninja` passed after the source fix.
- Promoted `MoveBG/MapObjDolpic.cpp` to `Object(Equivalent, ...)`.
- `python configure.py --non-matching && ninja` linked successfully with the
  TU source-linked.
- Follow-up normal `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.

## Remaining Byte Debt

- `TTurboNozzleDoor::loadAfter()` is string-base/register order and reload
  scheduling around the same four door-name comparisons and partner searches.
- `TTurboNozzleDoor::touchPlayer()` is particle vector store scheduling; the
  dash gate, stage branch, sound IDs, particle IDs, scale, position, collision
  removal, and flag set match behavior.
- `TDemoCannon::initMapObj()` is saved-register/string-base residue around the
  same resources, model flags, frame setup, and `TSharedParts` construction.
- `TDemoCannon::perform()` is stack-frame size residue; all frame gates,
  sounds, shake/rumble, and particle matrix binds are behavior-identical.
- `TBellDolpic::ring()` is stack/local layout and commutative multiply/cross
  product scheduling; normalization fallback and random delay match behavior.
- `TBellDolpic::control()` is stack/local layout and commutative multiply order
  on the final damping multiply; behavior matches.
- `TMonumentShine::hitByWater()` is stack/FPR/static-vector scheduling; the
  water direction, Mario direction, cross product, dot-product sign, and spin
  acceleration/deceleration behavior now match.
- `.data` / `.sdata2` drift and source-only weak/base extras are owner/layout
  byte debt; the `--non-matching` link proves they are not undefined/duplicate
  blockers for source-linking this TU.
