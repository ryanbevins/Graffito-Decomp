# mario/JSystem/JAudio/JASystem/JASBasicWaveBank

Verdict: equivalent
Date: 2026-06-13 8:39am MNL

Reverified current `Object(Equivalent, ...)` row during the audit sweep.

Reason:
- `python tools/decomp-diff.py -u mario/JSystem/JAudio/JASystem/JASBasicWaveBank`
  still reports no missing or extra symbols; all other functions/data match
  exactly.
- Full `--no-collapse` diff for
  `JASystem::TBasicWaveBank::incWaveTable(const TWaveGroup*)` shows the same
  loop bounds, pointer-table updates, null guard, and link-back stores.
  Residue is only GPR coloring in loop temporaries.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 12:55pm MNL recheck:
- Current overview still has no missing or extra target symbols.
- Re-read `TBasicWaveBank::incWaveTable(const TWaveGroup*)`; it still uses the
  same loop bounds, pointer-table update sequence, null guard, and link-back
  stores. Residue is only GPR coloring in loop temporaries. Reused this tick's
  successful source-link and normal DOL proof batch.

---

Verdict: equivalent
Date: 2026-06-13 4:26am MNL

Reason:
- `python tools/decomp-diff.py -u mario/JSystem/JAudio/JASystem/JASBasicWaveBank`
  reports no missing or extra symbols. All other functions and data symbols
  match exactly.
- `JASystem::TBasicWaveBank::incWaveTable(const TWaveGroup*)` is 96.0% and
  exact-size. The full `--no-collapse` diff shows the same loop bounds,
  pointer-table updates, null guard, and link-back stores. The residue is only
  GPR coloring in the loop temporaries.
- Re-verification of existing `Object(Equivalent, ...)` linked cleanly under
  `python configure.py --non-matching && ninja`.

Offending functions: none.
