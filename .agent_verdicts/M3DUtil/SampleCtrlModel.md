Verdict: matching
Time: 2026-06-13 6:57am MNL
Unit: mario/M3DUtil/SampleCtrlModel
Source: src/M3DUtil/SampleCtrlModel.cpp

Reason:
- `SampleCtrlModelData::SampleCtrlModelData(J3DModelData*)` matches exactly.
- `SampleCtrlModelData::makeHierarchy(J3DJoint*)` matches exactly.
- `SampleCtrlModelData::~SampleCtrlModelData()` matches exactly.
- The vtable and target `.data` rows match exactly. The only objdiff extras are
  two unreferenced local `.rodata` labels plus the aggregate `.rodata` row;
  runtime code does not use a behaviorally different path.
- Re-verification of existing `Object(Equivalent, ...)` linked cleanly under
  `python configure.py --non-matching && ninja`.
- 2026-06-13 6:30am MNL recheck: overview remains exact for owned text/data,
  and `python configure.py --non-matching && ninja` linked from source.
- 2026-06-13 6:57am MNL recheck: overview remains exact for all target
  functions and target data. The only extras are the same unreferenced rodata
  constants (`@196`, `@209`, aggregate `.rodata`); current source-link proof
  and normal DOL hash proof both passed during this audit tick.
- 2026-06-13 8:08am MNL promotion: changed the row from `Object(Equivalent, ...)` to `Object(Matching, ...)`; the normal `python configure.py && ninja` build passed and verified `build/GMSJ01/mario.dol: OK`.
