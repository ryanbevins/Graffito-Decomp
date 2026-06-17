# mario/Enemy/DemoBossHanachanBase

Verdict: matching
Date: 2026-06-13 6:06am MNL

Reason:
- Re-verified during the AUDIT sweep. Every owned text function matches
  exactly: `receiveMessage`, `initBase`, manager `clipEnemies`, save-params
  constructor, and the adjustor destructor thunk. All rodata and `.sdata2`
  rows match.
- Remaining overview drift is data / weak ownership only: source-owned
  destructors, tiny weak helpers, JDrama adjustor helpers, param vtables, and
  infectious strings are target-present or ownership/layout artifacts, not
  different runtime operations.
- Proof: `python configure.py --non-matching && ninja` linked from source,
  then normal `python configure.py && ninja` passed and verified
  `mario.dol: OK`.

- 2026-06-13 8:08am MNL promotion: changed the row from `Object(Equivalent, ...)` to `Object(Matching, ...)`; the normal `python configure.py && ninja` build passed and verified `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
