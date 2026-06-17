# GC2D/GCConsole2 Audit

Verdict: needs_impl  
Date: 2026-06-13 4:00am MNL

Not promoted.

Reason:

- `python tools/decomp-diff.py -u mario/GC2D/GCConsole2` shows missing target
  text for `JGeometry::TVec3<float>::set<float>(float, float, float)`.
- Missing local data includes `.ctors` `@2111`, `@2426`-`@2429`,
  `@11361`, `@11365`-`@11367`, dirty/infectious rows, many
  `scDolpicNews*` tables, `TGCConsole2::drawWater(...)::height`,
  `topDiff`, and `.sdata-0`.
- The rebuilt object also emits target-absent digit/text helper owners and
  shine/news conversion tables, so the source is not symbol-complete.
