# mario/Strategic/HitActor

Verdict: equivalent
Date: 2026-06-13 6:06am MNL

Reason:
- Re-verified during the AUDIT sweep. `TVec3::set`, `initHitActor`,
  `perform`, adjustor destructor thunk, vtable, and scalar data rows match
  exactly. Source-owned weak destructors/JDrama helper rows are target-present
  ownership drift.
- Full `--no-collapse` constructor diff shows the same base actor construction,
  zero/one vector initialization, collision fields, type/count stores, and
  final `THitActor` vtable install; visible mismatches are relocation
  label-owner drift only.
- Full `--no-collapse` `calcEntryRadius()` diff shows identical behavior:
  same max-X/Z selection, same squared-radius sum, same positive guard,
  reciprocal-square-root approximation path, zero fallback, and final
  `mEntryRadius` store. Remaining mismatches are stack-frame size and FPR
  coloring.
- Proof: `python configure.py --non-matching && ninja` linked from source,
  then normal `python configure.py && ninja` passed and verified
  `mario.dol: OK`.

2026-06-13 10:11am MNL recheck:
- Current overview still has no missing target symbols. The constructor still
  has the same base actor construction, vector initialization, collision field
  stores, type/count stores, and final vtable install. `calcEntryRadius` still
  computes the same max-X/Z squared radius, positive guard, reciprocal-sqrt
  approximation path, zero fallback, and `mEntryRadius` store.
- Remaining residue is relocation label-owner drift, stack-frame size, and FPR
  coloring.

Offending functions: none.
