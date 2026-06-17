# JSystem/JUtility/JUTNameTab

Verdict: `Equivalent` reverified.

## 2026-06-13 1:32pm MNL

- Proof earlier this tick, before any source/config changes:
  - `python configure.py --non-matching && ninja` linked `mario.elf` and `mario.dol`.
  - `python configure.py && ninja` linked and checksum passed.
- Reviewed `JUTNameTab::getIndex(const char*) const` at `98.2%`.
- Behavior matches: calculates the 16-bit key, scans every `ResNTAB::Entry`, skips mismatched keys, compares the table-relative name string with `strcmp`, returns the matching index, and returns `-1` when no entry matches.
- Residual drift is register coloring only.
