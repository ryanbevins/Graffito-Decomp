# mario/JSystem/JAudio/JALibrary/JALCalc

Verdict: equivalent
Date: 2026-06-13 10:28am MNL

Reason:
- `python tools/decomp-diff.py -u mario/JSystem/JAudio/JALibrary/JALCalc`
  reports no missing or extra symbols. All other functions/data symbols match.
- `JALCalc::getParamByExp(float, float, float, float, float, float,
  JALCalc::CurveSign)` is 98.2% and exact-size. The full `--no-collapse` diff
  shows identical exponential branches, linear interpolation math, upper/lower
  clamp tests, and return behavior. Residue is frame/FPR save-slot offsets,
  local const-label numbering, and one source-side `fmr f1, f0` before the
  shared final clamp because the linear branch holds the candidate value in a
  different FPR; the value entering the clamp is equivalent.
- Re-verification of existing `Object(Equivalent, ...)` linked cleanly under
  `python configure.py --non-matching && ninja`.
- 2026-06-13 10:28am MNL recheck: overview and full diff shape are unchanged.
  The current `--non-matching` source-link proof and normal DOL hash proof both
  passed in this audit tick.

Offending functions: none.
