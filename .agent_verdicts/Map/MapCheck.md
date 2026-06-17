verdict: not_equivalent
date: 2026-06-13 2:29am MNL
unit: mario/Map/MapCheck

Reason:
- No missing target symbols, but the main collision intersection path is not
  behavior-certified.
- `TMapCollisionData::intersectLine(...)` is only 50.1%, and the source carries
  fabricated helpers (`skewProduct`, `someUnknownInline`) with comments noting
  the control-flow shape is not understood.
- Leave this red until the line-intersection and wall-check geometry are
  reconstructed from target asm rather than treated as codegen drift.
