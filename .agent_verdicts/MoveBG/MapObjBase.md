# MoveBG/MapObjBase audit

Verdict: `equivalent`

Last rechecked: 2026-06-15 7:54am MNL.

Safety-net recheck: current overview still has no missing rows. The existing
certification remains valid; remaining extras are helper/destructor/rodata owner
debt. Today's full `python configure.py --non-matching && ninja` proof linked
this object from source, and normal `python configure.py && ninja` passed
`build/GMSJ01/mario.dol: OK`.

Checked 2026-06-14 1:16am MNL in AUDIT mode.

Changed source before certification:

- `changeObjMtx` now reads translation from matrix column 3 (`mtx[0][3]`,
  `mtx[1][3]`, `mtx[2][3]`), matching the target layout.
- `startAnim` no-BCK fallback now restores `MActor::unk8` through
  `modelData->getJointNodePointer(0)->setMtxCalc(...)`; the post-`setAnimation`
  flag test is `unkF8 & 0x200`.
- `perform` now calls the target virtuals/flags: `setGroundCollision()`,
  `hasMapCollision()`, local perform bit `0x200`, `calc()`,
  `SDLModel::viewCalcSimple()`, and `requestShadow()`. The loop-sound path is
  gated by `unk100 == 0`.

Remaining nonmatching functions were reviewed as codegen-class equivalent:

- Stack/register differences: `load`, `initAndRegister`, `calcRootMatrix`,
  `setGroundCollision`, `makeObjAppeared`, `makeObjDead`, `makeObjDefault`,
  `startAnim`, `startControlAnim`, `startSound`, `soundBas`,
  `setUpMapCollision`, `setUpCurrentMapCollision`, `removeMapCollision`,
  `sleep`, `changeObjMtx`.
- Branch/tail-shape differences: `perform` duplicates vs shares equivalent
  early-exit and sound tails, but calls and predicates now match target
  behavior.
- FPR/store-order residue: `setObjHitData` computes the same max scale and
  writes the same hit radii/heights from the table.

Proof:

- `python configure.py --non-matching && ninja` linked `mario.elf` and emitted
  `mario.dol` cleanly.
- `python configure.py && ninja` passed the normal build and SHA1 check with
  `build/GMSJ01/mario.dol: OK`.
