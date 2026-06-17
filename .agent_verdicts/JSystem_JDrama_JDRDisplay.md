# JSystem/JDrama/JDRDisplay

Verdict: `Equivalent` reverified.

## 2026-06-13 1:32pm MNL

- Proof earlier this tick, before any source/config changes:
  - `python configure.py --non-matching && ninja` linked `mario.elf` and `mario.dol`.
  - `python configure.py && ninja` linked and checksum passed.
- Reviewed `JDrama::TDisplay::startRendering()` at `99.9%`.
- Behavior matches: copies the pending render-mode fields into `TVideo`, selects the next XFB from `unkC`, applies display gamma and progressive field mode, derives dither/RGB565 booleans from `unk64`, and calls `IssueGXPixelFormatSetting`.
- Residual drift is frame-size/save-slot codegen only.
