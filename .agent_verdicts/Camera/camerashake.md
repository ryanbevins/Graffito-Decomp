# Camera/camerashake

Verdict: equivalent
Date: 2026-06-14 11:08am MNL

Promoted to `Object(Equivalent, "Camera/camerashake.cpp")`.

Proof:
- Added the missing source owner for `gpCameraShake` in `camerashake.cpp`,
  matching the target `.sbss` owner at `0x8040B380`.
- `python configure.py --non-matching && ninja` links with
  `Camera/camerashake.o` built from source.
- `python configure.py && ninja` restores the normal matching build and passes
  `build/GMSJ01/mario.dol: OK`.

Reviewed functions:
- `TCameraShake::startShake` and `TCameraShake::keepShake` byte-match.
- `TCameraShake::TCameraShake()` matches the constructor behavior: saves the
  41 named shake entries, allocates `TCamSaveShake` records, clears the active
  slot table, and initializes the per-slot defaults. Remaining diffs are
  register/order/unrolled reset shape.
- `TCameraShake::getUseShakeData_()` matches the inactive-first scan and the
  fallback active slot with the least remaining frames. Remaining diffs are
  loop unrolling, pointer arithmetic, and register allocation.
- `TCameraShake::execShake()` matches the visible shake semantics: save outPos,
  scan active slots, convert to polar, accumulate sinusoidal X/Y/Z offsets,
  increment/pause/decay/reset frames, convert back to cross coordinates,
  normalize the camera axis, build the yaw rotation, and rotate outAt.
  Remaining diffs are stack/register/FPR allocation plus helper-boundary debt
  around `setRotate`, `dot`, and `scale`.

Residual debt:
- Helper ownership and vector/matrix expression shape keep the low-fuzzy rows
  nonmatching, but no structural behavior difference remained after review.
