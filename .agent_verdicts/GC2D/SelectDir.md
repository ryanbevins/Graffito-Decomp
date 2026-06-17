# GC2D/SelectDir

Verdict: equivalent  
Date: 2026-06-13 1:04am MNL  
Commit: 019c9bcc

Reason:
- Reviewed all three nonmatching functions in `mario/GC2D/SelectDir`.
- `TSelectDir::direct()` differs only by stack frame/slot layout, saved GPR
  choice, local `TColor` stack placement, and constant-label numbering. Branch
  conditions, return values, fader/menu/reset/sound calls, and field offsets
  match behaviorally.
- `TSelectDir::~TSelectDir()` differs only by frame size and saved-register
  slots. The archive lookup/unmount, gamepad flag clear, base destructors, and
  delete guard match.
- `TSelectDir::rsetup()` has the same archive mount, object/group graph,
  particle resources, emitter views, screen/camera assignments, visibility
  flags, and return path. The only non-operand instruction clusters are
  `TLookAtCamera` construction source-shape residue: target homes the position,
  up, and target vectors to stack then copies them with integer stores, while
  current source writes the same values component-wise. Constants and field
  offsets match: position `(300, 240, 1300)`, up `(0, 1, 0)`, target
  `(300, 240, 0)`, near/far `50/10000`, fovy/aspect `30/1.3333334`.
- Data mismatches are local label-numbering/source-owner residue for the vtable
  and `.sdata2` constants; there are no missing target symbols in the overview.

Proof:
- `python configure.py --non-matching && ninja` linked `mario.dol` from
  source successfully.
- `python configure.py && ninja` passed and verified `mario.dol: OK`.

2026-06-13 10:11am MNL recheck:
- Current overview still has no missing target symbols. Full diffs for
  `direct`, `rsetup`, and the destructor remain behavior-equivalent: same
  setup thread join/result handling, menu/fader/sound transitions, archive and
  scene graph construction, particle/resource setup, screen/camera assignment,
  and final visibility flag stores.
- Remaining residue is stack-frame size, stack-slot offsets, saved-register
  coloring, local helper/rodata label attribution, and the known
  `TLookAtCamera` construction source-shape drift.
