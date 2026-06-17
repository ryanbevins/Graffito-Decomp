# System/MarNameRefGen_MapObj

Verdict: needs_impl  
Date: 2026-06-13 1:11am MNL

Reason:
- Not promoted. The overview has many missing target symbols and vtables, so
  this TU fails the strict audit gate before functional equivalence review.

Representative blockers:
- Missing `.text`: `TTelesaSlot::TTelesaSlot(const char*)`,
  `TFruitHitHideObj::~TFruitHitHideObj()`, `TFence::TFence(const char*)`,
  `TSirenaRollMapObj` virtual helpers/destructor, `TMapObjBase` virtual
  helpers, `TTakeActor` thunk/destructor helpers, and
  `JGeometry::TVec3<float>::set<float>(float, float, float)`.
- Missing `.rodata`: `TSirenaRollMapObj::__vtable`,
  `TTakeActor::__vtable`, plus `@2364` and `@2366`.
- `.data` remains substantially nonmatching with source-owned extra vtables for
  several stubbed local classes.

Blocking function:
- `TMarNameRefGen::getNameRef_MapObj(const char*) const` needs a source-shape
  and class-definition pass before this TU can be audited green.
