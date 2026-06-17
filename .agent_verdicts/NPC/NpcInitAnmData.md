# mario/NPC/NpcInitAnmData

Verdict: matching
Date: 2026-06-13 6:19am MNL

Reason:
- `SMSGetNpcInitAnmData(unsigned long)` matches exactly.
- All target NPC animation-index tables and aggregate `.rodata` rows match
  exactly.
- Objdiff reports missing anonymous `@1490`/`@1526` strings and extra
  `sNullNpcParts`/`sNpcAnmShortageMessage`; these are the same source-owned
  bytes under named labels, not behavior-bearing missing symbols.
- Source-link proof passed under `python configure.py --non-matching && ninja`,
  then normal `python configure.py && ninja` restored the matching config and
  verified `build/GMSJ01/mario.dol: OK`.

- 2026-06-13 8:08am MNL promotion: changed the row from `Object(Equivalent, ...)` to `Object(Matching, ...)`; the normal `python configure.py && ninja` build passed and verified `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
