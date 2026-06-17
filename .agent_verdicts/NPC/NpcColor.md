# mario/NPC/NpcColor

Verdict: equivalent
Date: 2026-06-13 1:59pm MNL

Reason:
- `python tools/decomp-diff.py -u mario/NPC/NpcColor` reports no missing or
  extra symbols.
- `SMS_InitChangeNpcColor(const MActor*, const TColorChangeInfo*, short, const
  _GXColor*)` is 99.6% and exact-size. The full `--no-collapse` diff shows the
  same material lookup, mode switch, color table null guards, packet helper
  calls, TEV/KColor argument ids, and exits. Residue is frame size/save-slot
  placement and consistent GPR coloring of the two color-table pointers; the
  call arguments remain equivalent.
- Re-verification of existing `Object(Equivalent, ...)` linked cleanly under
  `python configure.py --non-matching && ninja`, then normal `python
  configure.py && ninja` restored the matching config and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
