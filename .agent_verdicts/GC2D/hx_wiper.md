# mario/GC2D/hx_wiper

Verdict: equivalent  
Date: 2026-06-13 1:49pm MNL

`GC2D/hx_wiper.c` is functionally identical and links from source.

Reverified in the audit-only sweep against the existing
`state/notes/hx_wiper.md` campaign notes, a fresh unit overview, raw asm labels,
full/no-collapse diffs for all nonmatching helpers, and data/relocation dumps.
The remaining text differences are codegen-class: shared-label/state-block
layout in `Hx_Logo`, saved-register/FPR coloring, stack frame and local-slot
layout in the logo texture/circle/game-over/test helpers, FIFO expression
scheduling, and static data base/label ownership. The source performs the same
resource reads, GX setup, draw calls, timer updates, wipe state transitions,
path interpolation, logo texture math, circle/framebuffer passes, and
copy-filter updates.

Raw asm confirms the logo/pen draw call sites target the expected
`Hxs_Logo_*` helpers despite objdiff relocation-label drift. The `.data` bytes
are identical; the visible data mismatches are relocation order/jump-label
offsets for `Hx_Logo` and SDA/static ownership differences. `.sdata`/`.sdata2`
differences are static placement/local-constant order only, with the same
relocated values used at runtime. No missing symbols remain in the unit
overview.

Proof:

- `python configure.py --non-matching && ninja` linked `mario.dol` from source
  at 1:49pm MNL.
- `python configure.py && ninja` passed and verified `build/GMSJ01/mario.dol:
  OK` at 1:49pm MNL.
