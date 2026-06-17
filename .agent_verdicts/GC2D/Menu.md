# GC2D/Menu audit

Verdict: equivalent
Date: 2026-06-12 1:40pm MNL

Reason: all functions are either byte-matching or behaviorally aligned, and
`python configure.py --non-matching && ninja` linked the TU from source.

Function review:
- `TMenuPlane::perform(unsigned long, JDrama::TGraphics*)`: fade guard,
  accept/cancel flag setting, movement-input gate, previous/current color
  restoration, vertical/page navigation, wraparound, and selected-color updates
  match behaviorally. Remaining drift is stack size and register coloring.
- `TMenuPlane::TMenuPlane(const TMarioGamePad*, J2DPane*, unsigned long,
  unsigned long)`: base setup, color defaults, pane-tree walk, `rset` skip,
  first text-box color capture/recolor, count increment, pointer-array
  allocation, and copy loop match behaviorally. Remaining drift is stack size,
  register coloring, and signed-vs-unsigned compare for copying a nonnegative
  constructor-built count.
- `TMenuBase::perform(unsigned long, JDrama::TGraphics*)`: 2D graph setup,
  screen draw, scissor restore, and graph vtable teardown match. Remaining
  drift is stack size only.
- `TFlashPane` methods and destructors byte-match.

Notes:
- Source emits extra `J2DOrthoGraph`/`JDrama::TViewObj` weak owners, but the
  required `--non-matching` source-link proof passed.

2026-06-13 8:13am MNL recheck:
- Overview is unchanged: no missing target functions; only
  `TMenuPlane::perform`, `TMenuPlane` ctor, and `TMenuBase::perform` remain
  nonmatching.
- `TMenuPlane::perform`: current full diff preserves the fade-out guard,
  accept/cancel flag writes, movement-input gate, current-color restore,
  page/vertical navigation, wraparound, and selected-color updates. Residue is
  stack frame size, color-copy stack slots, and equivalent register coloring in
  the page-step arithmetic.
- `TMenuPlane` ctor: current full diff preserves base setup, color defaults,
  pane-tree walk, `rset` skip, first text-box color capture/recolor, count
  increment, pointer-array allocation, and copy loop. Residue is stack frame
  size, register coloring, and signed-vs-unsigned compare on the nonnegative
  constructor-built count.
- `TMenuBase::perform`: current full diff preserves 2D graph construction,
  setup, screen draw, scissor restore, and vtable teardown. Residue is stack
  frame/slot size only.
- Proof refreshed in the same audit sweep: `python configure.py --non-matching
  && ninja` linked from source, and normal `python configure.py && ninja`
  verified `build/GMSJ01/mario.dol: OK`.

2026-06-13 11:43am MNL recheck: verdict remains `equivalent`. Current overview
is unchanged: no missing target rows, same three nonmatching text functions,
and source-owned `J2DOrthoGraph` / `JDrama::TViewObj` weak-owner extras. The
existing full-diff review remains valid: menu navigation/color-state,
constructor pane discovery/copying, and draw/scissor behavior are aligned.
Shared proof from this tick passed: `python configure.py --non-matching &&
ninja`, then normal `python configure.py && ninja` verified
`build/GMSJ01/mario.dol: OK`.

2026-06-15 4:03am MNL recheck: verdict remains `equivalent`. Current full
diffs still show only codegen-class residue:
- `TMenuPlane::perform`: behavior-preserving input/navigation/color-state
  logic; residue is stack frame size, stack slots for color copies, and register
  coloring in the page-step arithmetic.
- `TMenuPlane` ctor: base construction, defaults, pane-tree walk, `rset` skip,
  first-pane color capture/recolor, pointer-array allocation, and copy loop
  remain aligned. Residue is frame/register shape and signed-vs-unsigned compare
  on the nonnegative discovered-pane count.
- `TMenuBase::perform`: graph setup, screen draw, scissor restore, and teardown
  remain aligned; residue is stack frame/slot size.

Proof refreshed: `python configure.py --non-matching && ninja` linked from
source, then normal `python configure.py && ninja` restored matching config and
verified `build/GMSJ01/mario.dol: OK`.
