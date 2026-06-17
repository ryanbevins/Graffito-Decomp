# Enemy/wireTrap

Verdict: equivalent
Date: 2026-06-15 7:02pm MNL

Certified `mario/Enemy/wireTrap` as functionally equivalent after fixing two
strict-audit behavior blockers:

- `TWireTrap::checkHitActors()` now matches the target two-sided wire-trap
  collision response. The direction-dot test uses both traps' scaled movement
  vectors, including `mScaleSpeed`, and both eligible traps get the 30-frame
  biri timer, possible direction flip, and `TNerveWireTrapWait` reset.
- `TWireTrap::load()` now casts the yaw table angle through `u16`, matching the
  target `clrlwi` mask before `jmaSinShift` instead of sign-extending via
  `s16`.

Remaining non-exact rows are byte/codegen debt only: stack and register
placement, helper-boundary differences around vector scaling and nerve
singletons, rodata/data label drift, and source-only weak/static-init owners.
No missing target rows remain.

Proofs:
- `python configure.py --non-matching && ninja` linked with
  `Object(Equivalent, "Enemy/wireTrap.cpp")`.
- Plain `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.
