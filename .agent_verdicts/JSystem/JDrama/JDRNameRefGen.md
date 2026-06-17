# JSystem/JDrama/JDRNameRefGen

Verdict: equivalent
Status: equivalent
Date: 2026-06-13 7:58pm MNL

Certified `Object(Equivalent, "JSystem/JDrama/JDRNameRefGen.cpp")`.

Behavior review:
- `TNameRefGen::getNameRef(const char*) const` tests the same factory strings
  in target order and returns the same object types/default names/constants for
  `GroupObj`, `SmJ3DScn`, `PolarCamera`, `SmJ3DAct`, `SmplChara`, `Light`,
  `IdxLight`, `LightAry`, `AmbColor`, `AmbAry`, `NameRefGrp`, `DrawBufObj`,
  `EfbCtrlTex`, and `Viewport`; unknown names return `nullptr`.
- `TNameRefPtrListT` / `TViewObjPtrListT` methods perform the same list
  iteration, insertion, virtual calls, and null handling; remaining drift is
  stack-slot/helper-label residue.
- `TLight(const char*)` now exists in this TU and initializes the same base
  fields, light type, GX attenuation, and white color; remaining drift is
  stack-slot/color temporary placement and local labels.
- Extra weak destructors/vtables and the duplicate dummy rodata are unreferenced
  source-emission/byte debt, not behavior differences.

Proof:
- `python configure.py --non-matching && ninja` linked with
  `JDRNameRefGen` from source.
- `python configure.py && ninja` restored normal config and passed
  `build/GMSJ01/mario.dol: OK`.

---

Verdict: fixed_by_implementation
Status: ready_for_audit
Time: 2026-06-13 7:56pm MNL

Implementation fixed the source-link blocker. `JDrama::TLight::TLight(const
char*)` is now declaration-only for `src/JSystem/JDrama/JDRNameRefGen.cpp` and
defined in that TU, while the normal `JDRLighting.hpp` inline constructor remains
available to other TUs. This recovers the target 248B constructor owner used by
the `IdxLight` factory path without disturbing the normal DOL build.

Proof:
- Normal `python configure.py && ninja` passed and verified
  `build/GMSJ01/mario.dol: OK`.
- Focused `python tools/decomp-diff.py -u mario/JSystem/JDrama/JDRNameRefGen`
  now reports `JDrama::TLight::TLight(const char*)` as present at 248B and
  `99.7%`; the remaining constructor diff is stack-slot/local-label residue.
- Temporary local promotion of `JSystem/JDrama/JDRNameRefGen.cpp` to
  `Object(Equivalent, ...)` passed `python configure.py --non-matching &&
  ninja`; the promotion was reverted for the next AUDIT tick to certify.

Ready for the next AUDIT tick to re-run full behavior review and promote if no
new structural issue appears.

Verdict: needs_impl
Date: 2026-06-13 2:42am MNL

Reason: current source is not functionally certifiable because the target object
contains `JDrama::TLight::TLight(const char*)` (248B), but the rebuilt object does
not define it. `getNameRef(const char*)` also constructs `TLight` through the
current default constructor source shape, producing extra `TLight`/`TIdxLight`
constructor/destructor/vtable ownership instead of the target constructor symbol.

Offending symbols:
- `JDrama::TLight::TLight(const char*)` — missing from rebuilt TU.
- `JDrama::TNameRefGen::getNameRef(const char*) const` — behavior around the
  `"Light"` factory path cannot be certified until constructor ownership/signature
  is reconstructed.

Audit evidence:
- `python tools/decomp-diff.py -u mario/JSystem/JDrama/JDRNameRefGen`
  reports the missing 248B text symbol plus constructor/destructor extras.
