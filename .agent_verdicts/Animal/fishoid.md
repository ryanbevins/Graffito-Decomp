# mario/Animal/fishoid

Verdict: equivalent
Date: 2026-06-13 2:15pm MNL

`Animal/fishoid.cpp` is functionally identical and links from source after the
`TFishoid::load(JSUMemoryInputStream&)` coin activation fix.

Audit checks:
- Full current diffs for all sub-100% text rows preserve behavior:
  `TFishoid::createRealoidActor`, `TFishoid::load`, `TFishoid::perform`,
  `TFishoid::TFishoid`, `TRealoid::perform`, `TRealoid::clipBoids`,
  `TRealoid::loadDefault`, `TRealoidActor::checkHitActors`,
  `TRealoidActor::calcRootMatrix`, `TFishoid::~TFishoid`, and
  `TFish::~TFish`.
- Remaining text residue is codegen/data-owner class: stack/frame size,
  register and loop-induction coloring, constructor/destructor helper ownership,
  local rodata/constant labels, equivalent bool materialization, and vector
  temporary placement.
- The old structural blocker is fixed: both target and rebuilt `load()` now
  dispatch `mCoinObj` through virtual slot `0x100` (`makeObjAppeared()`) before
  copying the last fish actor position.
- The lone missing rodata row,
  `@unnamed@::cFishoidMdlNames`, is paired with a same-size extra under the same
  name, so this is placement/ownership byte debt rather than absent data.

Proof:
- `python configure.py --non-matching && ninja` linked `mario.dol` from source.
- `python configure.py && ninja` restored the normal matching config and
  verified `build/GMSJ01/mario.dol: OK`.

Verdict: fixed_by_implementation
Date: 2026-06-13 2:06pm MNL

Implementation fixed the audit blocker in `TFishoid::load(JSUMemoryInputStream&)`.
The source now calls `mCoinObj->makeObjAppeared()`, and the focused diff shows
both target and rebuilt code loading virtual slot `0x100` before the coin
position copy.

Build / diff proof:
- `python configure.py && ninja` passed.
- `python tools/decomp-diff.py -u mario/Animal/fishoid -d 'TFishoid::load' --no-collapse -C 8`
  now shows `lwz r12, 0x100(r12)` on both sides at the former mismatch.

Ready for the next AUDIT tick to source-link re-certify as `Equivalent`.
The remaining nonmatching rows are the codegen/data-owner residue documented
below and in `state/notes/fishoid.md`.

Verdict: not_equivalent
Date: 2026-06-13 11:10am MNL

Do not certify until `TFishoid::load(JSUMemoryInputStream&)` is fixed.

Structural issue:
- Target calls virtual slot `0x100` on `mCoinObj` after making the last fish
  actor visible. The original and rebuilt `TMapObjBase` vtables both put
  `appear()` at `0xfc` and `makeObjAppeared()` at `0x100`, so the target call is
  `makeObjAppeared()`.
- Current source calls `mCoinObj->appear()`, emitting slot `0xfc`. This is a real
  behavior difference, not codegen or label drift.

Other current residue is codegen/data-owner class only:
- `@unnamed@::cFishoidMdlNames` appears as both a missing and extra 16B rodata
  row, so it is local table placement/ownership rather than absent data.
- Low-score rows still match the behavior described in `state/notes/fishoid.md`
  aside from the `mCoinObj` virtual call above: constructor inlining, boid-loop
  induction-variable shape, constant-compare form, vector temporary placement,
  and destructor vtable label ownership.

Previous stale verdict below retained for history.

Verdict: equivalent  
Date: 2026-06-13 3:03am MNL

`Animal/fishoid.cpp` is functionally identical and links from source.

Reviewed the unit overview, the existing `state/notes/fishoid.md`
functionally-complete notes, and fresh diffs for the lowest-score functions:
`TFishoid::createRealoidActor`, `TRealoidActor::checkHitActors`, and
`TRealoidActor::calcRootMatrix`. Remaining text differences are codegen-class:
constructor inlining at the `new TFish` site, frame/local-slot layout,
callee-saved register coloring, constant-compare form for `0x80000001`, vector
temporary placement, and local weak/data ownership. The source preserves the
same actor allocation/initialization, collision/message loop, boid clipping and
perform paths, fish matrix construction, model-table selection, load defaults,
and manager behavior.

Proof:

- `python configure.py --non-matching && ninja` linked `mario.dol` from source.
- `python configure.py && ninja` passed and verified `build/GMSJ01/mario.dol:
  OK`.
