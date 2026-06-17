# Player/Tongue

Verdict: equivalent
Status: promoted
Time: 2026-06-15 10:16am MNL

Certified `mario/Player/Tongue` as functionally equivalent and promoted
`Player/Tongue.cpp` to `Object(Equivalent, ...)` in `configure.py`.

Structural blockers fixed during audit:

- `TYoshiTongue::init(TYoshi*)` now allocates the tip `J3DModel` before loading
  `/mario/bmd/yoshi_tongue_tip.bmd`, matching the target's null-allocation
  short-circuit before the resource load.
- `TYoshiTongue::movement()` state 1 and `findTarget()` use target
  `mDamageHeight` (`0x5c`) for midpoint height; target asm does not read
  `mAttackHeight` (`0x54`) there.
- `TYoshiTongue::movement()` states 6/7 call this object's `getTakingMtx()`
  virtual instead of reading `mYoshi->mActor->unk4->getAnmMtx(0)`.
- `TYoshiTongue::movement()` states 6/7 call `mHolder->moveRequest(newTip)`
  directly; the previous `mYoshi->mActor->unk4` guard was a dummy behavior
  mismatch.

Remaining diffs are codegen/data residue: stack/register/FPR placement in
`calcAnim`, `movement`, `findTarget`, `canGo`, and `emit`; data-label drift in
`initInLoadAfter` and string/constant pools; extra unreferenced weak helper
owners (`TTakeActor`, `THitActor`, JGadget iterator, `TVec3::scale/add`,
JSUList destructors, JDrama thunks) plus the extra no-memory pointer/string
owner rows. The final overview has no missing target symbols.

Proofs:

- `python configure.py --non-matching && ninja` linked with Tongue sourced.
- `python configure.py && ninja` passed `build/GMSJ01/mario.dol: OK`.
