# mario/JSystem/JUtility/JUTNameTab

Verdict: equivalent
Date: 2026-06-13 8:27am MNL

Reason:
- `python tools/decomp-diff.py -u mario/JSystem/JUtility/JUTNameTab` reports no
  missing or extra symbols. Three functions match exactly.
- `JUTNameTab::getIndex(const char*) const` is 98.2% and exact-size. The full
  `--no-collapse` diff shows identical prologue/epilogue, loop control,
  branches, loads, `strcmp` call, and return behavior; every mismatch is a
  callee-saved GPR coloring difference.
- `python configure.py --non-matching && ninja` linked successfully with this
  object sourced; `python configure.py && ninja` restored the normal config and
  verified `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 12:42pm MNL recheck: verdict remains `equivalent`. Fresh
`--no-collapse` diff for `JUTNameTab::getIndex(const char*) const` still shows
the same entry cursor setup, key-code calculation, loop bound, key-code compare,
offset lookup, `strcmp` call, successful index return, and `-1` fallback. Every
mismatch is callee-saved GPR coloring. Proof reused from this tick:
`python configure.py --non-matching && ninja`, then normal `python configure.py
&& ninja` with `build/GMSJ01/mario.dol: OK`.
