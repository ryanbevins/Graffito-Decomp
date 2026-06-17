# JSystem/JDrama/JDREfbCtrl

Verdict: `Equivalent` reverified.

## 2026-06-13 1:36pm MNL

- Proof earlier this tick, before any source/config changes:
  - `python configure.py --non-matching && ninja` linked `mario.elf` and `mario.dol`.
  - `python configure.py && ninja` linked and checksum passed.
- Reviewed `JDrama::TEfbCtrl::perform(unsigned long, JDrama::TGraphics*)`, `JDrama::TEfbCtrlDisp::perform(unsigned long, JDrama::TGraphics*)`, and `JDrama::TEfbCtrlTex::setTexAttb(const _GXTexObj&)`.
- `TEfbCtrl::perform` matches behavior: flag `0x80` gates the GX color/alpha/Z state updates and copies `unk10` to `TGraphics::mDisplayRect`.
- `TEfbCtrlDisp::perform` matches behavior: flag `0x80` applies pixel-format state, then color/alpha/Z state and display-rect copy; flag `0x8` plus absence of `unkFC` bit `0x40` calls `IssueGXCopyDisp` with the framebuffer, display rect, render mode, clear color, clear Z, FB clamp, and flags.
- `TEfbCtrlTex::setTexAttb` matches behavior: `GXGetTexObjAll` writes image pointer, dimensions, format, wraps, and mipmap flag; width and height are stored to the same members. The target routes the zero-extended dimensions through stack words first, but the stored values are identical.
- Residual drift is codegen/symbol-owner only: frame size, saved-register set, stack temporary placement, argument load scheduling for `IssueGXCopyDisp`, and local ownership of `TEfbCtrl`/`TViewObj` destructor/vtable.
