# Audit verdict: equivalent

- Date: 2026-06-13 11:34pm MNL
- TU: `mario/JSystem/JDrama/JDRDirector`
- Source: `src/JSystem/JDrama/JDRDirector.cpp`
- Verdict: `equivalent`

## Reason

Current `configure.py` already marks this TU `Equivalent`; this refresh clears a
stale red cache. There are no missing target symbols. The only nonmatching text
is codegen/layout residue:

- `JDrama::TDirector::JSGFindObject(const char*, JStage::TEObject) const`
  returns the same camera/actor object adjustment for accepted types and null
  otherwise; the diff is register/base-pointer and branch-label drift.
- `JDrama::TDirector::direct()` constructs the same local `TGraphics`, calls
  `unk10->testPerform(3, &graphics)`, clears `graphics.unk0`, then calls
  `unk14->testPerform(8, &graphics)`. The rebuild additionally zeroes
  `TGraphics+0xfe` because the current header models that slot as
  `TFlagT<u16> unkFE`. The target leaves that halfword uninitialized, but the
  project-wide original assembly search for halfword/byte accesses at offset
  `0xfe` found no `TGraphics` consumers; active render flags use `unkFC`.

## Proof

- `python tools/decomp-diff.py -u mario/JSystem/JDrama/JDRDirector`
- `python tools/decomp-diff.py -u mario/JSystem/JDrama/JDRDirector -d "JDrama::TDirector::direct" --no-collapse`
- `python tools/decomp-diff.py -u mario/JSystem/JDrama/JDRDirector -d "JDrama::TDirector::JSGFindObject" --no-collapse`
- `python configure.py --non-matching && ninja` passed earlier this tick after
  the `MarDirectorInitECT` promotion.
- `python configure.py && ninja` passed earlier this tick after the
  `MarDirectorInitECT` promotion.
