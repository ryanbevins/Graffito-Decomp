# mario/NPC/NpcSound

Verdict: matching
Date: 2026-06-13 6:19am MNL

Reason:
- The only text symbol, `TBaseNPC::getBasNameTable() const`, matches exactly.
- The `.data` aggregate and all named NPC sound table rows match exactly.
- The `.rodata` diff is label ownership only: target anonymous `@NNNN`
  strings/tables correspond to the source's named `sNNNN` statics, while the
  aggregate bytes and table contents match.
- Source-link proof passed under `python configure.py --non-matching && ninja`,
  then normal `python configure.py && ninja` restored the matching config and
  verified `build/GMSJ01/mario.dol: OK`.

- 2026-06-13 8:08am MNL promotion: changed the row from `Object(Equivalent, ...)` to `Object(Matching, ...)`; the normal `python configure.py && ninja` build passed and verified `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
