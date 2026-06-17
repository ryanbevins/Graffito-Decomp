# mario/MoveBG/MapObjHide

Verdict: equivalent
Status: source_link_proven
Time: 2026-06-14 9:31pm MNL

## Reason
`MoveBG/MapObjHide.cpp` now links cleanly from source and the remaining diffs
are behavior-neutral codegen/owner residue. The implementation commit
`c106dbcf` fixed the real string/data blockers:

- Hidden-object shine camera names use target `シャイン（%s）カメラ`.
- Twin-picture camera names use target `%sカメラ`.
- The target zero/one Vec rodata bytes are present as `cHideObjZeroVec` and
  `cHideObjOneVec`; objdiff still reports `@2782`/`@2784` missing and the
  named constants extra, but this is local-label ownership churn.

Reviewed current nonmatching gameplay paths: `TWoodBox::loadAfter/kill`,
`TBreakHideObj::control/receiveMessage`,
`THideObjPictureTwin::loadAfter/afterFinishedAnim`,
`TWaterHitPictureHideObj::load/loadAfter/control/touchWater/forward/afterFinishedAnim`,
`TFruitBasket::loadAfter/touchFruit/countFruit`, the three hide-object `load`
variants, and `THideObjBase::loadAfter/appearObjFromPoint`. Calls, branch
conditions, constants, state writes, object appear/throw behavior, water color
updates, sound IDs, particle IDs, and camera dispatch semantics match.

Remaining debt:

- stack-frame/register/FPR scheduling differences,
- objdiff local-label pairing noise in rodata and some destructor/vtable rows,
- source-only auto destructor/JSUList weak-owner emission,
- `.data`/`.rodata` label ownership drift.

Validation:

- `python configure.py --non-matching && ninja` passed with MapObjHide sourced.
- `python configure.py && ninja` restored normal matching config and passed
  `build/GMSJ01/mario.dol: OK`.
