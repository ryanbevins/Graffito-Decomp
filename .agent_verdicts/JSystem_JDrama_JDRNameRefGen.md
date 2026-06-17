# mario/JSystem/JDrama/JDRNameRefGen

Verdict: equivalent  
Date: 2026-06-13 7:58pm MNL  
Source: `src/JSystem/JDrama/JDRNameRefGen.cpp`

## Reason

Certified behavior-equivalent and source-linkable after recovering the
`JDrama::TLight::TLight(const char*)` owner.

- `TNameRefGen::load` and the emitted owner helpers either byte-match or differ
  only by local label/slot naming.
- `TNameRefGen::getNameRef(const char*) const` tests the same factory strings
  in the same order and returns the same object types/default names:
  `GroupObj`, `SmJ3DScn`, `PolarCamera`, `SmJ3DAct`, `SmplChara`, `Light`,
  `IdxLight`, `LightAry`, `AmbColor`, `AmbAry`, `NameRefGrp`, `DrawBufObj`,
  `EfbCtrlTex`, and `Viewport`; unknown names return `nullptr`.
- `TNameRefPtrListT` / `TViewObjPtrListT` virtual methods perform the same list
  iteration, virtual calls, insertion, and null handling. Remaining drift is
  stack-slot size and helper-owner/label display noise.
- `TLight(const char*)` initializes the same base fields, light type, GX
  attenuation, and white color; remaining drift is stack-slot/color temporary
  placement and local constant labels.
- Extra weak destructors/vtables and the extra `dummyMactorStringValue1` symbol
  are unreferenced source-emission/byte debt, not behavior differences.

## Proof

- `python configure.py --non-matching && ninja` linked with
  `JDRNameRefGen` from source.
- `python configure.py && ninja` restored the matching config and passed
  `build/GMSJ01/mario.dol: OK`.
