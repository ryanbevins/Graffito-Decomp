# GC2D/ScrnFader Audit

Verdict: equivalent
Date: 2026-06-13 9:12am MNL

## Verdict
equivalent — 2026-06-12 7:24pm MNL

## Reason
`GC2D/ScrnFader.cpp` links from source under
`python configure.py --non-matching && ninja`, and the normal matching build
still verifies `mario.dol: OK`.

Reviewed all three nonmatching text functions:
- `TSMSFader::load(JSUMemoryInputStream&)`
- `TSMSFader::update()`
- `@unnamed@::draw_wipe_box(const JDrama::TRect&, JUtility::TColor)`

The behavior matches the target: load performs the same base load, timer read,
fade-in start, color read, RGBA extraction, and channel clamps; update performs
the same request countdown, deferred wipe request, fade-out alpha forcing, wipe
range guard, and fade-in/out state updates; `draw_wipe_box` emits the same GX
state and sixteen-vertex wipe frame with equivalent inset rectangle math.
Remaining diffs are stack/register allocation, equivalent byte/float
evaluation order, local helper label attribution, and data-label ownership. No
missing target symbols were reported; source-owned extras link cleanly.

2026-06-13 9:12am MNL recheck: full current diffs still show no behavior
drift. `load` keeps the same base load, timer/color reads, fade transition,
and channel clamps; `update` keeps the same request countdown, wipe request,
fade-out alpha forcing, range guard, and fade update; `draw_wipe_box` keeps the
same GX setup and 16-vertex frame writes. `python configure.py --non-matching
&& ninja` linked from source, then `python configure.py && ninja` verified
`build/GMSJ01/mario.dol: OK`.

2026-06-13 12:52pm MNL recheck: current overview still has no missing target
symbols. Re-read the three nonmatching diffs; `load`, `update`, and
`draw_wipe_box` still perform the same reads, fade/wipe state updates,
channel clamps, GX setup, wipe rectangle math, and FIFO writes. Residue is
stack/register allocation, byte/float temporary order, and helper/data owner
labels. Reused this tick's successful source-link and normal DOL proof batch.

Offending functions: none.
