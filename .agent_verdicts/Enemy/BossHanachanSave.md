Verdict: matching
Time: 2026-06-13 6:30am MNL
Unit: mario/Enemy/BossHanachanSave
Source: src/Enemy/BossHanachanSave.cpp

Reason:
- `TBossHanachanChangeSaveParams::TBossHanachanChangeSaveParams(const char*)`
  matches exactly.
- `TBossHanachanCommonSaveParams::TBossHanachanCommonSaveParams(const char*)`
  matches exactly.
- All target rodata and `.sdata2` rows match exactly. Objdiff extras are
  unreferenced TParam template vtable/data artifacts plus infectious string
  pointers.
- Re-verification of existing `Object(Equivalent, ...)` linked cleanly under
  `python configure.py --non-matching && ninja`.
- 2026-06-13 6:30am MNL recheck: overview remains exact for target-owned
  constructors/data, and `python configure.py --non-matching && ninja` linked
  from source.
- 2026-06-13 8:08am MNL promotion: changed the row from `Object(Equivalent, ...)` to `Object(Matching, ...)`; the normal `python configure.py && ninja` build passed and verified `build/GMSJ01/mario.dol: OK`.
