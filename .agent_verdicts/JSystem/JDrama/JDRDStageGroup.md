# JSystem/JDrama/JDRDStageGroup

Verdict: equivalent
Date: 2026-06-13 1:07pm MNL

Reverified current `Object(Equivalent, "JSystem/JDrama/JDRDStageGroup.cpp")`
again during the audit-only sweep.

Reviewed functions:
- `JDrama::TDStageGroup::perform(unsigned long, JDrama::TGraphics*)` already
  byte-matches.
- `JDrama::TDStageGroup::~TDStageGroup()` differs because the target inlines
  member/base destruction while the source emits helper calls. The source
  destructor sets the `TDStageGroup` vtable, calls
  `TFrmGXSet::~TFrmGXSet(-1)` on the member, calls
  `TViewObjPtrListT<TViewObj>::~TViewObjPtrListT(0)` on the base, then deletes
  when the delete flag is positive. The target spells out the same vtable
  resets, `TNameRef` destruction, list-pointer destruction, and delete guard.
- Source-only weak helper functions and helper vtables account for the extra
  symbols/data rows; source-link validation accepts that ownership drift.

Validation:
- Shared proof: `python configure.py --non-matching && ninja` linked from source, then `python configure.py && ninja` restored the normal matching config and verified `build/GMSJ01/mario.dol: OK` at 2026-06-13 1:07pm MNL.
