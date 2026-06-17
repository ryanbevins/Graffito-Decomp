# mario/MoveBG/MapObjGeneral

Verdict: equivalent
Status: equivalent
Checked: 2026-06-14 5:23pm MNL

## Verdict
Promoted `MoveBG/MapObjGeneral.cpp` to `Equivalent`.

The previous structural blocker in
`TMapObjGeneral::receiveMessage(THitActor*, unsigned long)` was fixed by
testing actor type `0x10000025` and `0x80000001` on the message sender instead
of `this`.

The inline `TMapObjGeneral::getFlushTime() const` was also fixed to return
`mNormalFlushTime`; the old header body emitted a one-instruction `blr` and
would return garbage through the source-linked vtable.

## Review
After those fixes, the remaining function diffs reviewed this tick appear
codegen-class: stack frame size, register/FPR lifetime, source-vs-target
temporary ordering, and label drift around weak/simple helpers.

Reviewed nonmatching functions:
- `receiveMessage`, `perform`, `calcRootMatrix`, `control`, `bind`,
  `calcVelocity`, `touchGround`, `checkWallCollision`, `appear`,
  `ensureTakeSituation`, `recover`, `appearing`, `sinking`, `recovering`,
  `holding`, `thrown`, `put`, `waitingToAppear`.

No missing behavior symbols remain after the source fixes. The source object
still emits a weak inline `TMapObjGeneral::getLivingTime() const` copy because
that inline placement is what keeps `getFlushTime` as the first out-of-line
virtual and routes the vtable owner to `MapObjInit.o`; this is source-link-safe
and not a behavior difference.

## Source-link proof
Resolved the old source-link blocker:
- `getLivingTime`/`getFlushTime` are declared before the override list so their
  lexical order controls the key-virtual choice without shifting inherited
  vtable slots.
- `getLivingTime` remains inline in the class, preserving its slot before
  `getFlushTime` but not becoming the vtable home.
- `getFlushTime` is defined out-of-line in `MapObjInit.cpp`, so
  `MapObjGeneral.o` imports `TMapObjGeneral::__vt` and links against the
  original `MapObjInit.o` owner.
- `checkIllegalAttr` is defined in `WoodBarrel.cpp`, matching its target owner.

Proof:

```sh
python configure.py --non-matching && ninja
python configure.py && ninja
```

The full source-link build and normal matching build both pass.

## Recheck 2026-06-14 5:23pm MNL

Current overview still has no missing target symbols. The same nonmatching rows
remain behavior-equivalent by the existing review, with stack/register/FPR,
helper-owner, local constant, and `.sdata2` order debt only. This tick's
`python configure.py --non-matching && ninja` and normal
`python configure.py && ninja` proof builds covered the current object.
