# JSystem/JDrama/JDRDrawBufObj

Verdict: `Equivalent` reverified.

## 2026-06-13 1:32pm MNL

- Proof earlier this tick, before any source/config changes:
  - `python configure.py --non-matching && ninja` linked `mario.elf` and `mario.dol`.
  - `python configure.py && ninja` linked and checksum passed.
- Reviewed `JDrama::TDrawBufObj::perform(unsigned long, JDrama::TGraphics*)` at `99.8%`.
- Behavior matches: flag `0x80` frame-inits the draw buffer, flag `0x400` installs it into `j3dSys.mDrawBuffer[0/1]` according to `unk18` bits, and flag `0x8` stores `unk18` to `j3dSys.unk4C` before drawing.
- Residual drift is frame-size/save-slot codegen plus local ownership of `JDrama::TViewObj` destructor/vtable.
