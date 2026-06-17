# Enemy/telesa Audit

Verdict: equivalent  
Date: 2026-06-14 02:00am MNL

Certified after fixing the two behavior-bearing issues found across the recent
implementation/audit pass:

- `TNerveTelesaImitate::execute(...)`: the sight-check branch now returns false
  when `isInSight(...)` returns zero after `resetBaseGround()` fails, matching
  the target reveal gate for areas 7/14.
- `TTelesa::calcRootMatrix()`: particle `0x188` now binds to
  `mMActor->getModel()->getAnmMtx(3)` / model matrix offset `0x90`; particle
  `0x187` remains on `getAnmMtx(4)` / offset `0xC0`, matching raw target asm.
- `TTelesa::init(...)`: the empty first-instance joint loop now uses an 8-bit
  loop counter, matching the target `clrlwi ..., 24` condition.

Reviewed nonmatching rows. Remaining drift is codegen/data-class only:
stack-frame and stack-slot size, saved-register/FPR coloring, random interval
`fmadds` source shape, no-op/inline helper boundaries, local static/vtable label
ownership, and broad extra weak/helper emission from includes. No target symbol
is missing, and no behavior-bearing call, store, constant, memory offset, or
branch condition remains different in the reviewed text rows.

Proof:
- `python configure.py && ninja` passed after the source fixes.
- `python configure.py --non-matching` plus `ninja` linked
  `Enemy/telesa.o` from source.
- Restored normal config with `python configure.py`; normal `ninja` passed with
  `build/GMSJ01/mario.dol: OK`.

Verdict: not_equivalent / needs_impl  
Date: 2026-06-14 01:46am MNL

Do not promote yet. Re-audit after the imitate sight-branch fix found a new
behavior-bearing blocker:

- `TTelesa::calcRootMatrix()`: target binds particle `0x188` to
  `mMActor->getModel()->getAnmMtx(3)` / model matrix offset `0x90`; current
  source calls `getAnmMtx(4)` and emits offset `0xC0`. Raw asm confirms target
  sequence at `build/GMSJ01/asm/Enemy/telesa.s` lines around
  `calcRootMatrix__7TTelesaFv`: particle `0x187` uses `+0xC0`, particle
  `0x188` uses `+0x90`. This changes the visual/effect matrix pointer and is a
  real behavior difference, not codegen drift.

Reviewed before stopping:
- `TNerveTelesaAttackMario::execute`, `TNerveTelesaFreeze::execute`,
  `TNerveTelesaImitate::execute`, `TBoxTelesa::reset`,
  `TTelesa::initItemAttacker`, and `TTelesa::initAttacker`: remaining visible
  drift is stack/frame, FPR choices, no-op/inline helper boundaries, and
  random-interval `fmadds` source shape. The previously fixed imitate sight
  condition now has the correct behavior.
- Several near-exact load/transition rows were also checked and showed only
  stack/frame or local-label residue.

Verdict: fixed_by_implementation / ready_for_audit  
Date: 2026-06-14 12:41am MNL

Implementation fixed the known behavior blocker in
`TNerveTelesaImitate::execute(TSpineBase<TLiveActor>*) const`: after
`resetBaseGround()` is false, source now returns false when
`isInSight(SMS_GetMarioPos(), 0.0f, 0.0f, searchAware)` returns zero, matching
the target's reveal gate for areas 7/14.

Proof:
- `python configure.py && ninja` passed.
- Focused diff now shows the same behavior at the sight check; remaining drift
  is branch-layout/codegen (`beq` to common false return vs local false block).
- A temporary `Object(Equivalent, "Enemy/telesa.cpp")` still passed
  `python configure.py --non-matching && ninja`.
- `Enemy/telesa.cpp` was restored to `Object(NonMatching, ...)` pending a
  fresh complete AUDIT review of all non-100% rows.

Verdict: not_equivalent  
Date: 2026-06-13 3:52am MNL

Temporary source-link proof passed:

- `Object(Equivalent, "Enemy/telesa.cpp")`
- `python configure.py --non-matching && ninja`

The TU still has a behavioral branch mismatch and stays `NonMatching`.

Offending function:

- `TNerveTelesaImitate::execute(TSpineBase<TLiveActor>*) const`: after
  `resetBaseGround()` is false, target calls
  `isInSight(SMS_GetMarioPos(), 0.0f, 0.0f, searchAware)` and branches to the
  return-false path when the result is zero:

  `cmpwi r3, 0; beq .L_802944E4`.

  Current source returns false when `isInSight(...)` is true:

  ```cpp
  if (!self->resetBaseGround()) {
      if (self->isInSight(SMS_GetMarioPos(), 0.0f, 0.0f, searchAware))
          return false;
  }
  ```

  That inverts the reveal condition for imitating Telesas in areas 7/14, so it
  is a real behavior difference, not codegen drift.

Other reviewed rows (`TNerveTelesaAttackMario`, `TTelesa::initAttacker`, and
`TTelesa::initItemAttacker`) were codegen/random-interval source-shape residue,
but the imitate branch blocks certification.
