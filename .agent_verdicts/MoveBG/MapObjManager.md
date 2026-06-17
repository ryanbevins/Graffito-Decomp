# MoveBG/MapObjManager

Verdict: equivalent
Date: 2026-06-14 7:34am MNL

Certification:
- Promoted `MoveBG/MapObjManager.cpp` to `Object(Equivalent, ...)`.
- `python configure.py --non-matching && ninja` linked successfully with
  `MapObjManager` source-linked.
- Follow-up `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.

Strict audit:
- The rebuilt object has no undefined reference to the missing target-local
  `JGeometry::TVec3<float>::set<float>(float, float, float)` helper. The target
  calls that helper while constructing `TTelesaBlock`; source emits equivalent
  stores for `TJuiceBlock::unk140` inline. This is byte/helper-owner debt, not a
  behavior or source-link blocker.
- `TMapObjBaseManager::newAndRegisterObjByEventID(unsigned long, const char*)`
  preserves the same event switch, object lookup/creation paths, position/
  rotation/scale copies, and `initAndRegister()` calls. Residue is stack-frame
  and scratch-vector placement plus misleading helper labels caused by missing
  local owners.
- `newUniqueObjByName(const char*)` preserves the same string chain and
  constructor choices. The visible constructor diffs are codegen/inline-owner
  debt: target calls `TVec3::set(1,1,1)` in the `TTelesaBlock` construction path
  and an out-of-line `SMatrix34C<f>` ctor in the `TSandLeaf` path; source emits
  the same field stores inline.
- `makeObjAppear(float,float,float,unsigned long,bool)` and
  `TMapObjManager::load(JSUMemoryInputStream&)` differ only by stack slot/frame
  layout while preserving the same map checks, object filters, material-table
  loads, draw-buffer setup, SDL model setup, and stage-conditional resource
  loads.
- Remaining data/extra drift is weak/destructor/static-owner debt from included
  map-object classes and infectious strings; source-link proof confirms no
  duplicate/undefined blocker under the current `--non-matching` set.
