# JSystem/JDrama/JDRFrmGXSet

Verdict: `Equivalent` reverified.

## 2026-06-13 1:34pm MNL

- Proof earlier this tick, before any source/config changes:
  - `python configure.py --non-matching && ninja` linked `mario.elf` and `mario.dol`.
  - `python configure.py && ninja` linked and checksum passed.
- Reviewed `JDrama::TFrmGXSet::perform(unsigned long, JDrama::TGraphics*)` at `96.8%`.
- Behavior matches: flag `0x8` gates the whole update; the current framebuffer and render mode are copied from `TDisplay`; bits `0x1` through `0x40` in `TDisplay::unk64` are mirrored into `TGraphics::unkFC`; framebuffer clamp, clear color, and clear Z are copied to the same graphics fields.
- Residual drift is codegen/symbol-owner only: larger stack frame, repeated GPR choices while reloading `unk10`, clear-color temporary stack slot, and local ownership of `JDrama::TViewObj` destructor/vtable.
