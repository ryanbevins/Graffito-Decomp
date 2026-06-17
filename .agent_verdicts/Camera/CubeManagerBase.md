# Camera/CubeManagerBase audit

Verdict: `equivalent`

Checked: 2026-06-14 6:35pm MNL in AUDIT safety-net mode.

## Result

Reverified existing `Object(Equivalent, "Camera/CubeManagerBase.cpp")`.

The source-link ownership fix still stands: this TU owns the global cube-manager
pointers in target order (`gpCubeCamera`, `gpCubeMirror`, `gpCubeWire`,
`gpCubeStream`, `gpCubeShadow`, `gpCubeArea`, `gpCubeFastA/B/C`,
`gpCubeSoundChange`, `gpCubeSoundEffect`).

Current nonmatching functions remain behavior-equivalent:

- `SMS_IsInSameCameraCube(const Vec&)` preserves the Mario position copy,
  `+75.0f` Y adjustment, two cube lookups, and `same && != -1` result.
- `SMS_IsInOtherFastCube(const Vec&)` preserves the director mode guard and
  A/B/C fast-cube checks; residue is bool materialization, register coloring,
  and helper boundary shape.
- `TCubeManagerArea::isInAreaCube(const Vec&) const` preserves current/last
  cube lookup and the map-7 floor-name transition logic.
- `TCubeManagerBase::isInCube`, `getInCubeNo`, `load`, and constructors have
  codegen-class frame/register/string-label/helper-owner residue.

The remaining missing `@1490`/`@1526` rows are rodata-owner byte debt, not
source undefined references or behavior blockers.

## Verification

- `python tools/decomp-diff.py -u mario/Camera/CubeManagerBase -s missing`
  reports only rodata-owner rows `@1490` and `@1526`.
- This tick's `python configure.py --non-matching && ninja` linked all current
  `Equivalent` objects successfully.
- This tick's `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.
