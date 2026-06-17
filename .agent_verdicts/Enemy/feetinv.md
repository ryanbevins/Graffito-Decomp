# Enemy/feetinv audit

Verdict: needs_impl

Checked 2026-06-13 1:27am MNL during AUDIT sweep.

Unit: `mario/Enemy/feetinv`

Blocking evidence:
- `FeetInvCalc(J3DModel*, unsigned short, unsigned short, unsigned short,
  float)` is only 62.9% and is the core inverse-kinematics routine for the TU.
- Objdiff reports missing data symbols `@1431`, `@1411`, `@1210`, `@2179`,
  and `@2180`, plus nonmatching `.data` and `.sdata2`.
- The source object emits many extra J3D matrix-calc weak/destructor/helper
  symbols and dummy rodata/vtables. This TU does not satisfy the audit
  requirement that every symbol is accounted for.

Do not promote until the static data ownership and J3D weak-emission shape are
fixed, then re-audit `FeetInvCalc` instruction-by-instruction.
