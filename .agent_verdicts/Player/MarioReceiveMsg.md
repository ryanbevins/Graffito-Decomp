# Player/MarioReceiveMsg audit

Verdict: needs_impl
Checked: 2026-06-14 11:34pm MNL

Unit: `mario/Player/MarioReceiveMsg`

Blocking evidence:
- `TMario::receiveMessage(THitActor*, unsigned long)` is still structurally
  different, not just codegen drift. The late message paths around offsets
  `0x1f50-0x223c` have different return/control-flow layout: source inserts
  early `li r3, 0` exits where target falls through into status/animation work,
  and one branch has the inverse condition around `mHeldObject`/message checks.
- The final fruit/damage path has different load ordering and operands around
  `keepDistance`, `rumbleStart`, and `calcDamagePos`; this needs a dedicated
  implementation pass, not audit certification.
- Static data ownership is still incomplete/noisy: missing ctor rows `@4221`,
  `@4227`, and `@4272`, plus nonmatching jump-table/data/sdata2 rows.
- `TMario::getGesso(THitActor*)` is near-exact and looks codegen-only, but the
  main dispatcher blocks the TU.

Implementation follow-up:
- Reconstruct the late held-object/message branch structure from target asm
  before revisiting source-link equivalence.
- Then re-audit jump-table and sdata2 ownership after the dispatcher behavior
  is structurally aligned.
