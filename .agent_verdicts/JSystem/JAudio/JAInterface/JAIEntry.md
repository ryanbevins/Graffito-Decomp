# mario/JSystem/JAudio/JAInterface/JAIEntry

Verdict: equivalent
Date: 2026-06-13 8:39am MNL

Reverified current `Object(Equivalent, ...)` row during the audit sweep.

Reason:
- `python tools/decomp-diff.py -u mario/JSystem/JAudio/JAInterface/JAIEntry`
  still reports no missing or extra symbols.
- Full `--no-collapse` diff for
  `JAIEntry::checkSoundHandle(JAISound**, unsigned long, void*)` shows
  identical null guards, sound-id comparison, priority checks, `stop()` calls,
  and return behavior. Residue is stack-frame/save-slot size only.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 12:46pm MNL recheck: verdict remains `equivalent`. Fresh full diff
for `JAIEntry::checkSoundHandle(JAISound**, unsigned long, void*)` still shows
the same null guards, sound-id family comparison, old-sound `stop(0)` path,
new/old priority queries, priority comparison, replacement stop, and boolean
return. The residue is only frame/save-slot size. Proof refreshed with
`python configure.py --non-matching && ninja`, then normal `python configure.py
&& ninja` with `build/GMSJ01/mario.dol: OK`.

---

Verdict: equivalent
Date: 2026-06-13 4:24am MNL

Reason:
- `python tools/decomp-diff.py -u mario/JSystem/JAudio/JAInterface/JAIEntry`
  reports no missing or extra symbols.
- `JAIEntry::checkSoundHandle(JAISound**, unsigned long, void*)` is 99.8% and
  exact-size. The full `--no-collapse` diff shows identical null guards,
  sound-id comparison, priority checks, `stop()` calls, and return behavior.
  The only residue is stack frame size / saved-register slot offsets.
- Re-verification of existing `Object(Equivalent, ...)` linked cleanly under
  `python configure.py --non-matching && ninja`.

Offending functions: none.
