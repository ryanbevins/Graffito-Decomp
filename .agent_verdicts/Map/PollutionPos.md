# Map/PollutionPos audit

Verdict: equivalent
Date: 2026-06-13 7:54am MNL

Reason: all functions are either byte-matching or behaviorally aligned, and
`python configure.py --non-matching && ninja` linked the TU from source.

Fresh recheck: full pass repeated on 2026-06-13 7:54am MNL. No missing target
text symbols were present; the only extra text symbols are weak MSound list
destructors from the rogue includes.

Function review:
- `TPollutionPos::isSame(int, int, float) const`: bounds checks, map index,
  `0xff` depth guard, world-depth conversion, owner depth tolerance, and final
  inclusive range test match behaviorally. Remaining drift is stack size,
  register coloring, and redundant fctiwz spill shape.
- `TPollutionPos::getDepthWorld(int, int) const`: map index, `0xff` sentinel,
  depth-to-world scaling/offset, and `-9999.0f` fallback match. Remaining drift
  is stack/FPR coloring.
- `TPollutionPos::getEdgeDegree(int, int) const`: bounds check, fixed 3x3
  neighbor scan excluding center, map index, and `0xff` edge count match.
  Remaining drift is stack size and register coloring.
- Constructor, `init`, conversion helpers, `isProhibit`, and `__sinit` byte-match.

Notes:
- Source emits extra MSound list weak owners from rogue includes, but the
  required `--non-matching` source-link proof passed.

2026-06-13 10:49am MNL recheck: verdict remains `equivalent`. Re-read the
current diffs for `isSame`, `getDepthWorld`, and `getEdgeDegree`. The bounds
checks, tile index math, `0xff` sentinel handling, depth/world conversions,
owner-depth tolerance, inclusive range test, and fixed 3x3 edge scan are
unchanged. Residue is stack size, register/FPR coloring, and redundant
conversion spill shape. Proof refreshed with `python configure.py
--non-matching && ninja`, then normal `python configure.py && ninja` with
`build/GMSJ01/mario.dol: OK`.
