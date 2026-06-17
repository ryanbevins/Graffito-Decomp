# System/MarNameRefGen Audit

Verdict: equivalent
Recorded: 2026-06-13 2:51am MNL
Updated: 2026-06-13 10:05pm MNL

Unit: `mario/System/MarNameRefGen`
Source: `src/System/MarNameRefGen.cpp`

## Previous Blocking Missing Target Text

- `TSunGlass::TSunGlass(JUtility::TColor, const char*)` (176B)
- `JGadget::TList_pointer<THitActor*>::TList_pointer(const JGadget::TAllocator<void*>&)` (48B)

Both are now recovered and exact 100% matches:

- `TSunGlass` ctor is an inline `JDrama::TViewObj` constructor in
  `SunGlass.hpp` that initializes the target-written fields and leaves
  `unk1C/unk1D` untouched.
- `TList_pointer<THitActor*>` ctor is emitted by declaring the template
  constructor in-class and defining it out-of-class in `std-list.hpp`; the old
  in-class body inlined through to `TList_pointer_void` and left the 48B weak
  target owner missing.

Source-link proof:

- Temporary local `Object(Equivalent, "System/MarNameRefGen.cpp")` passed
  `python configure.py --non-matching && ninja` after adding the inline
  `TStageEnemyInfo::TStageEnemyInfo()` dependency body.
- Restored `configure.py`, then `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.

Audit verdict:

- Certified `Equivalent` after current-tree review. Text missing filter is
  empty; remaining extras are target-absent weak/template owners and do not
  leave undefined references when source-linked.
- `TMarNameRefGen::getNameRef` preserves the same strcmp chain, allocations,
  constructor calls, global assignments, and fallback `TNameRefGen` call.
  Residue is frame size, rodata offsets, constructor-inline label drift, and
  a redundant `TPerformList` list-node zero store that is immediately
  overwritten by `TSingleNodeLinkList::Initialize_()`.
- `TNameRefAryT<...>::load` instantiations perform the same count read,
  backing-vector resize, stream wrapper setup, and per-element `load` loop.
  Diffs are register/stack slots and template-helper owner labels.
- `JGadget::TVector<...>::InsertRaw` instantiations use the same capacity
  arithmetic, relocation/copy/destructor loops, allocation, old-buffer
  deletion, and return pointer. `TCameraMapTool` scores low because our build
  calls `TCameraMapTool::operator=`, which copies the same post-vptr fields as
  the target's unrolled word-copy loop.
- `JDrama::TViewObjPtrListT<THitActor, JDrama::TViewObj>` helpers keep the
  same iterator traversal, virtual `perform/searchF/loadAfter` calls, and
  stream insertion logic; remaining differences are iterator temporary layout
  and weak label drift.
