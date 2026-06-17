# mario/Enemy/egggen

Verdict: needs_impl  
Date: 2026-06-13 11:55am MNL

Do not promote yet. The visible behavior is effectively reconstructed:
`TEggGenerator::control()` is byte-identical, `createModelData()` calls the same
model-data virtual with an equivalent static entry table, and `init()` differs
only by label/owner drift around the same actor setup, BCK setup, hit actor
radii, and `MsWrap` rotation normalization.

Current blocker is source-link ownership, not runtime logic:

- A temporary `Object(Equivalent, "Enemy/egggen.cpp")` promotion failed
  `python configure.py --non-matching && ninja`.
- Linker error: multiply-defined `TYoshi::onYoshi()` in `egggen.o`, previously
  defined in `MarioAction.o`.

Keep this `NonMatching` until the weak/helper ownership for `TYoshi::onYoshi()`
is corrected or suppressed naturally in this TU.
