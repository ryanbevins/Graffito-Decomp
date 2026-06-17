# mario/MarioUtil/EffectUtil

Verdict: equivalent
Date: 2026-06-13 8:39am MNL

Reverified current `Object(Equivalent, ...)` row during the audit sweep.

Reason:
- `python tools/decomp-diff.py -u mario/MarioUtil/EffectUtil` still reports no
  missing or extra symbols; other functions match exactly.
- Full `--no-collapse` diff for `SMS_EmitSinkInPollutionEffect(...)` preserves
  frame-gate timing, basis-vector cross products, zero-length fallback,
  inverse-sqrt normalization, matrix construction, and both particle emissions.
  Residue is larger frame/FPR save set and FPR coloring only; the stored matrix
  basis/position values are the same.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 12:55pm MNL recheck:
- Current overview still has no missing or extra target symbols.
- Re-read `SMS_EmitSinkInPollutionEffect(...)`; the frame-gate timing,
  cross-product basis construction, zero-length fallback, inverse-sqrt
  normalization, matrix stores, and both particle emissions still match
  behavior. Residue is frame/FPR save set and FPR coloring over the same stored
  matrix basis/position values. Reused this tick's successful source-link and
  normal DOL proof batch.

---

Verdict: equivalent
Date: 2026-06-13 4:28am MNL

Reason:
- `python tools/decomp-diff.py -u mario/MarioUtil/EffectUtil` reports no
  missing or extra symbols. Other functions match exactly.
- `SMS_EmitSinkInPollutionEffect(const TVec3f&, const TVec3f&, bool)` is 96.7%
  and exact-size. The full `--no-collapse` diff shows identical frame-gate
  timing, basis-vector cross products, zero-length fallback, inverse-sqrt
  normalization, matrix construction from basis/position vectors, and particle
  emissions. Residue is larger frame/FPR save set and FPR coloring: the
  source-side `f24/f25/f28` and `f3/f2/f1` values map to the target's stored
  basis components, so the matrix contents and emitted effects are equivalent.
- Re-verification of existing `Object(Equivalent, ...)` linked cleanly under
  `python configure.py --non-matching && ninja`.

Offending functions: none.
