# Enemy/Koopa Audit

Verdict: equivalent  
Date: 2026-06-15 10:16pm MNL

Promoted by AUDIT tick after behavior review and source-link proof.

Certification:

- Fixed two structural mismatches found during audit before promotion:
  `TKoopa::init` now stores the target literal `mBodyRadius = 300.0f`, and
  `TNerveKoopaFlame::execute` default path only changes to BCK 5 instead of
  recomputing flame direction / queuing the Flame nerve.
- Reviewed Koopa's behavior-heavy nonexact functions against target asm,
  including `perform`, `init`, `calcRootMatrix`, hit actor setup, neck callback,
  receive-message paths, and all Koopa nerve `execute` bodies. Remaining
  mismatches are codegen-class differences: helper inlining/boundaries,
  register/frame scheduling, matrix/quaternion source shape, and local-data
  label ownership.
- Source-link proof passed with `Object(Equivalent, "Enemy/Koopa.cpp")`:
  `python configure.py --non-matching && ninja`.
- Standard build proof passed afterward:
  `python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.

Known byte-level residue:

- `python tools/decomp-diff.py -u mario/Enemy/Koopa -s missing` still reports
  target helper rows for
  `JGeometry::TVec3<float>::set<float>(const JGeometry::TVec3<float>&)` and
  `JGeometry::TVec3<float>::set<float>(float, float, float)`. Natural explicit
  specialization/body routing attempts either hit MWCC "unimplemented C++
  feature" diagnostics or left the rows missing; Koopa.o has no undefined
  reference to these helpers, so this is source ownership/byte debt unless the
  behavior review finds a structural mismatch.
- Missing `.ctors`/local-data labels remain (`@2322`, `@2561`-`@2564`,
  `@2850`, `entry$3400`, `@4421`, `@6099`), with matching-sized source extras
  under human-readable names for most rows. Source-link proof shows they do not
  block linking.
