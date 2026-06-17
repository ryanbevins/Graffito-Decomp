# GC2D/Guide

Verdict: equivalent
Status: certified
Time: 2026-06-14 1:41pm MNL

Audited every nonmatching Guide function against the target asm. One real
behavior bug was found and fixed before promotion: the `perform()` setup-wait
path now updates all ten `_3D0` stage icon panes, matching the target unrolled
10-entry loop; the source previously stopped at eight.

Proof:
- `python tools/decomp-diff.py -u mario/GC2D/Guide --search "TGuide::perform"`
  improved from 66.8% to 76.4% after the loop fix.
- `python configure.py --non-matching && ninja` passed with
  `GC2D/Guide.cpp` promoted to `Object(Equivalent, ...)`, proving the source
  object links.
- `python configure.py && ninja` passed afterward and verified
  `build/GMSJ01/mario.dol: OK`.

Remaining nonmatching rows are codegen/data ownership residue, not functional
blockers: register and stack-frame coloring, helper-boundary differences around
pane animation and resource setup, local const-pool labels, weak/list/dtor
extras, and static data owner drift for labels such as `@1942`,
`scNormalStageTable`, and `scScenarioNameTable`.
