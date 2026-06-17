Verdict: equivalent
Time: 2026-06-13 6:30am MNL
Unit: mario/JSystem/J2D/J2DPicture
Source: src/JSystem/J2D/J2DPicture.cpp
commit_reviewed: 985fa36f

Reason:
- Promoted `JSystem/J2D/J2DPicture.cpp` to `Object(Equivalent, ...)` and
  proved it with `python configure.py --non-matching && ninja`.
- All data/vtable sections match and no target symbol is missing.
- Remaining function diffs in `drawFullSet` and `setTevMode` are stack-frame
  and local float/GXColor temporary placement only. Branches, constants, GX
  state calls, texture coordinate math, `swap` calls, `drawTexCoord` calls, and
  returns match target behavior.
- Extra text is weak/inline `J2DPane` helper ownership; it does not block the
  source-link build.
- 2026-06-13 6:30am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.
- 2026-06-13 10:00am MNL recheck: full `--no-collapse` diffs for
  `drawFullSet` and `setTevMode` remain behavior-equivalent. `drawFullSet`
  preserves the binding/mirror adjustment math, texture-coordinate selection,
  `setBlendKonstColor` calls, and final `setTevMode`/draw dimensions; residue
  is local float and conversion-slot placement. `setTevMode` is the same
  channel, TEV order/color/alpha, k-color, blend, and stage-count setup with
  only frame/color-temp placement drift. Proof rerun passed:
  `python configure.py --non-matching && ninja`, then normal
  `python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
- 2026-06-13 1:21pm MNL recheck: current diffs still classify as codegen-only.
  `drawFullSet` preserves width/height clamping, mirror/binding booleans,
  UV math, optional swaps, and final `drawTexCoord` argument values; its
  differences are conversion/float stack-slot placement. `setTevMode` preserves
  all GX channel, TEV order/color/alpha/k-color, stage-count, and blend calls
  with identical constants and predicates; drift is frame/color-temp layout.
  Source-link and normal proof reruns passed.
