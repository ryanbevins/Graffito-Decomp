# MoveBG/MapObjMamma Audit

Verdict: needs_impl  
Date: 2026-06-13 4:00am MNL

Not promoted.

Reason:

- `python tools/decomp-diff.py -u mario/MoveBG/MapObjMamma` shows missing
  target text for `TMapObjBall::getDepthAtFloating()`.
- Missing local data includes `.ctors` `@3329`, `@3566`, `@3693`,
  `@4971`, `@4974`, `@4976`, `@4978`, `@4979`, `@5077`, `@3569`,
  `@4450`, `@4451`, and many later small constant rows.
- Several map-object behavior functions remain low-score and source-shape
  incomplete, but the missing symbols alone are sufficient for `needs_impl`.
