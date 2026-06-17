# MarioUtil/DrawUtil

Verdict: equivalent
Date: 2026-06-15 9:11am MNL

Reason: certified source-linkable and behavior-equivalent. The prior
implementation cleared the previous missing-helper blocker, and this audit
also narrowed `TTrembleModelEffect::init()` to the target's first
vertex-attribute-format entry loop (no robust later-entry scan). The rebuilt TU
now emits exact target rows for:
- `JGeometry::TRotation3<JGeometry::TMatrix34<JGeometry::SMatrix34C<float>>>::identity33()`
- `JGeometry::TVec3<float>::sub(const JGeometry::TVec3<float>&)`
- `JGeometry::TVec3<short>::sub(const JGeometry::TVec3<short>&)`
- `JGeometry::TVec3<float>::add(const JGeometry::TVec3<float>&)`
- `JGeometry::TVec3<short>::add(const JGeometry::TVec3<short>&)`

Evidence:
- `python tools/decomp-diff.py -u mario/MarioUtil/DrawUtil` shows all five rows
  at 100%.
- `Object(Equivalent, "MarioUtil/DrawUtil.cpp")` plus
  `python configure.py --non-matching && ninja` linked cleanly.
- Plain `python configure.py && ninja` passed
  `build/GMSJ01/mario.dol: OK`.

Remaining non-exact rows are behavior-complete codegen/data residue documented
in `state/notes/DrawUtil.md`: loop unroll/frame/register shape, local
copy-temporary placement, rodata label/constant-pool layout, weak extras, and
missing 12B rodata `@2036` (`{0.9, 0.5, 0.05}`) used only for target constant
pool layout in `TSilhouette::loadAfter()`.
