# mario/NPC/NpcThrow

Verdict: matching
Date: 2026-06-13 4:42am MNL

Reason:
- Re-verified during the AUDIT sweep. `python tools/decomp-diff.py -u
  mario/NPC/NpcThrow` reports only exact matches:
  `TNpcThrow::throwMario(THitActor*)` and all `.sdata2` rows are 100%, with
  no missing or extra symbols.
- The refreshed full `--no-collapse` diff for `throwMario` shows identical
  vertical / forward direction selection, pitch/yaw conversion, JMA sin/cos
  rotation, Mario message, and `SMS_ThrowMario` call.
- The TU-scope explicit specialization declaration for `CLBRoundf<s16>(f32)`
  still routes both angle conversions to the existing weak owner instead of
  emitting a local helper and duplicate `.sdata2` constants.
- Promoted `NPC/NpcThrow.cpp` from `Object(Equivalent, ...)` to
  `Object(Matching, ...)`; the normal `python configure.py && ninja` build
  passed and verified `mario.dol: OK`.

Offending functions: none.
