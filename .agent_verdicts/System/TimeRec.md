# mario/System/TimeRec

Verdict: equivalent
Date: 2026-06-13 6:06am MNL

Reason:
- Re-verified during the AUDIT sweep. `drawSyncCallback`, `snapGXTime`,
  `TTimeArray` constructor, `start`, `append`, vtables, and data rows match
  exactly. Source-owned extra `suppleGXTime`, constructor,
  `drawSyncCallbackSt`, and `end` rows are owner/emission drift.
- Full `--no-collapse` diff for `TTimeRec::flip()` shows identical behavior:
  same active-buffer selection, same backfill loop over missing timestamps,
  same buffer-index flip, and same zeroing of both array counters for the new
  active buffer. Remaining mismatches are GPR coloring and branch-label address
  drift.
- Proof: `python configure.py --non-matching && ninja` linked from source,
  then normal `python configure.py && ninja` passed and verified
  `mario.dol: OK`.

2026-06-13 10:11am MNL recheck:
- Current overview still has no missing target symbols. `flip` still performs
  the same active-buffer selection, missing timestamp backfill loop, buffer
  index flip, and counter zeroing for the new active buffer.
- Remaining residue is GPR coloring and branch-label address drift.

Offending functions: none.
