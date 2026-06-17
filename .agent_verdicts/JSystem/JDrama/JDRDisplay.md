# mario/JSystem/JDrama/JDRDisplay

Verdict: equivalent
Date: 2026-06-13 8:39am MNL

Reverified current `Object(Equivalent, ...)` row during the audit sweep.

Reason:
- `python tools/decomp-diff.py -u mario/JSystem/JDrama/JDRDisplay` still
  reports no missing or extra symbols.
- Full `--no-collapse` diff for `JDrama::TDisplay::startRendering()` shows the
  same render-mode copies, XFB/gamma/frame calls, pixel-format flag extraction,
  and final issue call. Residue is stack frame/save-slot size only.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 12:46pm MNL recheck: verdict remains `equivalent`. Fresh
`--no-collapse` diff for `JDrama::TDisplay::startRendering()` still performs
the same render-mode copy into the video object, next-XFB selection, gamma and
frame-to-field GX calls, pixel-format flag extraction, and
`IssueGXPixelFormatSetting` call. The only differences are frame size and
saved-register slot offsets. Proof refreshed with `python configure.py
--non-matching && ninja`, then normal `python configure.py && ninja` with
`build/GMSJ01/mario.dol: OK`.

---

Verdict: equivalent
Date: 2026-06-13 4:16am MNL

Reason:
- `python tools/decomp-diff.py -u mario/JSystem/JDrama/JDRDisplay` reports no
  missing or extra symbols. The constructor and data symbols match exactly.
- `JDrama::TDisplay::startRendering()` is 99.9% and exact-size. The full
  `--no-collapse` diff shows identical render-mode copies, XFB/gamma/frame
  calls, pixel-format flag extraction, and final issue call. The only residue
  is stack frame size / save-slot offsets (`0x20` target vs `0x18` source).
- `python configure.py --non-matching && ninja` linked successfully with this
  object sourced; `python configure.py && ninja` restored the normal config and
  passed the DOL hash check.

Offending functions: none.
