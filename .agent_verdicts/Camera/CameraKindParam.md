Verdict: matching
Time: 2026-06-13 6:30am MNL
Unit: mario/Camera/CameraKindParam
Source: src/Camera/CameraKindParam.cpp

Reason:
- `TCameraKindParam::copySaveParam(const TCamSaveKindParam&)` matches exactly.
- `TCameraKindParam::inbetweenData(const TCameraKindParam&, float)` matches exactly.
- All target `.sdata2` rows match. The only objdiff extra is the unused local
  helper body `inbetweenS16(short*, short, float)`; `inbetweenData` inlines the
  helper body and remains byte-exact, so the extra body is byte debt only.
- Re-verification of existing `Object(Equivalent, ...)` linked cleanly under
  `python configure.py --non-matching && ninja`.
- 2026-06-13 6:30am MNL recheck: overview still has byte-exact owned functions,
  and `python configure.py --non-matching && ninja` linked from source.
- 2026-06-13 8:08am MNL promotion: changed the row from `Object(Equivalent, ...)` to `Object(Matching, ...)`; the normal `python configure.py && ninja` build passed and verified `build/GMSJ01/mario.dol: OK`.
