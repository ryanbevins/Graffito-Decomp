verdict: equivalent
date: 2026-06-13 7:36pm MNL
unit: mario/JSystem/JDrama/JDRLighting
source: src/JSystem/JDrama/JDRLighting.cpp
commit: 850be556

Reason:
- Certified `JSystem/JDrama/JDRLighting.cpp` as `Object(Equivalent, ...)`.
- Current overview has no missing target `.text` symbols. The recovered
  `JUtility::TColor::set(u8,u8,u8,u8)` and
  `JDrama::TViewObj::TViewObj(const char*)` helpers are present and 100%.
- Proof passed with `python configure.py --non-matching && ninja`; normal
  `python configure.py && ninja` then restored the matching config and passed
  with `build/GMSJ01/mario.dol: OK`.

Behavior review:
- `TLight::load`, `TLightAry::setLightNum`, `TLightAry::perform`, and
  `TAmbColor::JSGSetColor` differ only by stack frame size, stack slots, and
  saved-register coloring.
- `TIdxLight::TIdxLight()` performs the same `TLight("<IdxLight>")`
  construction, default light attenuation/color setup, vtable writes, and
  `unk68 = 0`; residual drift is stack/register and helper-label/codegen debt.
- `TAmbColor::TAmbColor()` target inlines the `TViewObj` construction and
  color byte stores, while current source calls the recovered helpers; both set
  the same name/key/flag/vtables and color bytes `0x4c,0x4c,0x4c,0xff`.
- `.rodata`, `.data`, and `.sdata2` drift is label/relocation/helper-owner byte
  debt. Extra JSU/TViewObj weak helpers are not source-link blockers and do not
  change runtime behavior.
