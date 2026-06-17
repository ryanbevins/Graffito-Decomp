# Enemy/BossHanachanParts

Verdict: equivalent
Date: 2026-06-13 11:59pm MNL

Reason:
- Re-audited after the `considerSetAnm_` hip-drop fix and found four additional
  behavior mismatches in current source. Corrected them before certification:
  tumble/flipped checks use `mRotation.z` (`this+0x38`), BCK completion treats
  `frame + 0.1f >= end` as complete, `mWaterHit` is a plain `TWaterHitActor`
  initialized with `(mActorType, 1, 0x80000000, ...)`, and the base part itself
  calls `initHitActor(actorType, 0, 0, ...)`.
- Current text overview has no missing target functions. Remaining nonmatching
  functions are behavior-identical; diffs are stack/frame size, register
  coloring, branch layout for equivalent conditions, inline/helper ownership
  (`getLatestNerve`, JGadget iterators, destructors), local rodata/data owner
  labels, and store/order/codegen residue.
- `initFootHitActor_` and `initMapCollisionAndHitActor_` now create the same
  actors, pass the same hit-actor constants, register them with the same group,
  clear the same hit flag, and copy the same joint matrix translations.
- `receiveMessage`, `considerSetAnm_`, damage fog, shadow, frame-copy, tumble
  rate, and constructors now perform the same calls, constants, field offsets,
  and branch conditions as the target.

Proof:
- `python configure.py --non-matching && ninja` linked successfully with
  `Enemy/BossHanachanParts.o` from source.
- `python configure.py && ninja` passed afterward with
  `build/GMSJ01/mario.dol: OK`.
