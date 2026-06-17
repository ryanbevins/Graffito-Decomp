# mario/GC2D/BoundPane

Verdict: equivalent
Date: 2026-06-13 6:06am MNL

Reason:
- Re-verified during the AUDIT sweep. `setPaneSize` and `setPanePosition`
  match exactly; all `.sdata2` rows match. Source-owned weak
  `J2DPane::move(int, int)` / `resize(int, int)` are target-present weak owner
  drift.
- Full `--no-collapse` constructor diff shows the same pane lookup, bounds
  copy, zero initialization, float initialization, and enable flag stores; only
  frame/save-slot size differs.
- Full `--no-collapse` `TBoundPane::update()` diff shows identical behavior:
  position and size enable gates, Bezier interpolation math, sign-aware
  rounding, virtual `move` and `resize` calls with the same arguments, progress
  increments, completion flag clears, and final `!unk24 && !unk25` return.
  Remaining differences are stack-slot layout, FPR/GPR coloring, and local
  constant-label names.
- Proof: `python configure.py --non-matching && ninja` linked from source,
  then normal `python configure.py && ninja` passed and verified
  `mario.dol: OK`.

2026-06-13 10:11am MNL recheck:
- Current overview still has no missing target symbols. The constructor remains
  frame-only, and `update` still performs the same position/size enable gates,
  Bezier interpolation math, sign-aware rounding, virtual `move`/`resize`
  calls, progress increments, completion flag clears, and final return.
- Remaining residue is stack-slot layout, FPR/GPR coloring, and local
  constant-label attribution.

Offending functions: none.
