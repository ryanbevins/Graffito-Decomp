# Audit verdict: equivalent

- Date: 2026-06-13 11:38pm MNL
- TU: `mario/MarioUtil/ScreenUtil`
- Source: `src/MarioUtil/ScreenUtil.cpp`
- Verdict: `equivalent`

## Reason

Current `configure.py` already marks this TU `Equivalent`; this refresh clears a
stale red cache. There are no missing target symbols. The only nonmatching text
is `TAfterEffect::perform`, and the full diff is codegen/proven-label residue:

- The case-0 blur reset body is inlined in both target and rebuild with the same
  stores to `unk14`, `unk1B`, `unk1C`, `unk16`..`unk18`, `unk28`, `unk2C`,
  `unk30`, and `unk34`.
- The case-2 call is `calcDashBlurValue__12TAfterEffectFv` in both target and
  rebuild. `decomp-diff.py`'s collapsed label was misleading here; raw asm and
  `powerpc-eabi-objdump -dr` relocations both identify the call correctly.
- Remaining differences are saved-register coloring around the viewport rect
  pointer and local zero/color temporaries.
- `TScreenTexture::getTexture()` is an inline `unk10` getter used by matching
  call sites. The extra standalone `TAfterEffect::setBlurDefaultValue()` text is
  unreferenced owner debt; its body is exactly the inlined target case-0 logic.

## Proof

- `python tools/decomp-diff.py -u mario/MarioUtil/ScreenUtil`
- `python tools/decomp-diff.py -u mario/MarioUtil/ScreenUtil -s missing`
- `python tools/decomp-diff.py -u mario/MarioUtil/ScreenUtil -s extra`
- `python tools/decomp-diff.py -u mario/MarioUtil/ScreenUtil -d "TAfterEffect::perform" --no-collapse`
- `powerpc-eabi-objdump -dr build/GMSJ01/obj/MarioUtil/ScreenUtil.o`
- `powerpc-eabi-objdump -dr build/GMSJ01/src/MarioUtil/ScreenUtil.o`
- `python configure.py --non-matching && ninja` passed earlier this tick after
  the `MarDirectorInitECT` promotion.
- `python configure.py && ninja` passed earlier this tick after the
  `MarDirectorInitECT` promotion.
