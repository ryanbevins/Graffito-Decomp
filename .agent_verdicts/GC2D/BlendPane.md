# mario/GC2D/BlendPane

Verdict: equivalent
Date: 2026-06-13 1:59pm MNL

Reason:
- `python tools/decomp-diff.py -u mario/GC2D/BlendPane` reports no missing or
  extra symbols. Constructors/destructor/data symbols match exactly.
- `TBlendPane::update()` is 99.5% and exact-size. The full `--no-collapse`
  diff shows identical base update call, active/clamp logic, blend constant
  helper calls, progress update, and return bool materialization. The residue is
  an FPR coloring swap between `progress` and `1.0f - progress`; each field is
  still stored with the same value as target.
- Re-verification of existing `Object(Equivalent, ...)` linked cleanly under
  `python configure.py --non-matching && ninja`, then normal `python
  configure.py && ninja` restored the matching config and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
