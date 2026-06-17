## M3DUtil/SampleCtrlNode

Verdict: equivalent
Audited: 2026-06-12 11:09am MNL
Reverified: 2026-06-13 9:44am MNL — current source still links under
`python configure.py --non-matching && ninja`; restored matching build with
`python configure.py && ninja` and DOL hash check passed.

Promoted `Object(NonMatching, "M3DUtil/SampleCtrlNode.cpp")` to `Equivalent`.

Evidence:
- `SampleCtrlMaterial::SampleCtrlMaterial(J3DMaterial*)` is the only
  nonmatching target function; the joint/shape constructors, destructors, and
  vtables are byte-exact.
- The material constructor preserves the same vptr/default-field setup, source
  material store, material-color copy, four color-channel extraction loop, TEV
  order copy, stage-count read, and TEV-stage extraction loop.
- Remaining differences are frame size, register coloring, and local
  label/constant ownership around the tiny lookup table used for color-channel
  decoding.
- The missing/extra small data/sdata/rodata entries are local-label ownership
  residue; source-link validation succeeds with this TU enabled.
- 9:44am MNL recheck: fresh full diff for
  `SampleCtrlMaterial::SampleCtrlMaterial(J3DMaterial*)` still has identical
  vptr/default stores, material pointer/color copy, four color-channel decode
  loop, TEV order copy, stage-count read, and TEV-stage decode loop. Residue
  is frame size, register coloring, and local lookup-table label ownership.
- `python configure.py --non-matching && ninja` linked a source DOL with this
  TU enabled.
- `python configure.py && ninja` restored the matching build and passed the
  DOL hash check.

Reverified: 2026-06-13 12:41pm MNL — verdict remains `equivalent`. Fresh full
diff for `SampleCtrlMaterial::SampleCtrlMaterial(J3DMaterial*)` still shows
the same vptr/default setup, source material store, material-color copy, four
color-channel extraction loop, TEV order copy, stage-count read, and TEV-stage
field extraction loop. The missing `@1211` / sdata rows and extra local rodata
are lookup-table ownership drift; visible behavior is unchanged. Proof reused
from this tick: `python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.

Reverified: 2026-06-13 1:21pm MNL — verdict remains `equivalent`. Current full
diff still preserves the material pointer store, default TEV order/color state,
four-channel bit extraction loop, TEV order copy, stage-count read, and
per-stage field extraction loop. Remaining differences are frame size,
register coloring, and the small lookup-table/data label ownership (`@1211`
versus source local rodata/sdata2). Source-link and normal proof from this tick
passed.
