# Camera/lensflare audit

Verdict: equivalent

Certified 2026-06-14 5:31pm MNL during AUDIT.

Promoted `Camera/lensflare.cpp` to `Object(Equivalent, ...)` after rechecking
the stale structural blocker. No source change was needed:
`TLensFlare::perform(unsigned long, JDrama::TGraphics*)` is
behavior-equivalent, and the source-link promotion now passes the required
proof build.

Proof:
- Full `perform` diff reviewed against
  `build/GMSJ01/asm/Camera/lensflare.s`.
- Visibility and alpha paths match behavior: same indoor early false path,
  same `unk40`/`unk44` symmetric screen bounds, same 17-sample z-buffer count,
  same `unk48 * (1.0f - count / 17.0f)` base alpha, same ease-to-255 call, and
  same chase-rate selection from `unk2C/unk30/unk34/unk38`.
- Near-plane math matches behavior: after `CLBCalcNearNinePos`, target computes
  `center + (nearPos[5] - center) * (-sunFPos.x * unk3C)
          + (nearPos[1] - center) * (-sunFPos.y * unk3C)`, subtracts the sun
  position, calls `MsGetRotFromZaxis`, rounds X/Y, and builds the TRS matrix
  with sun position and `unk18/unk1C/unk20` scale. Current source uses the same
  values and formula despite different helper-boundary/stack shape.
- Constructor reviewed as 99.9% stack/data-label residue only.
- `Object(Equivalent, "Camera/lensflare.cpp")` passed
  `python configure.py --non-matching && ninja`.
- Restored normal config; final `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.

Remaining byte debt: stack-frame/slot placement, `TVec3::set`,
`TVec3::TVec3`, `JMASSin`, and `JMASCos` helper-boundary ownership, plus
local data-label/vtable residue. These are codegen/source-shape issues for a
future matching pass, not implementation blockers.

Verdict: not_equivalent

Checked 2026-06-13 1:44am MNL during AUDIT sweep.

Unit: `mario/Camera/lensflare`

Blocking evidence:
- `TLensFlare::perform(unsigned long, JDrama::TGraphics*)` is 69.3% and has
  structural-looking differences in the visibility/alpha path and the
  `CLBCalcNearNinePos` / flare-position vector math region.
- The target constructs the near-position `TVec3` array with
  `__construct_array` and a `JGeometry::TVec3<float>` ctor owner; source emits
  extra `JGeometry::TVec3<float>::TVec3()`, `JDrama::TViewObj::~TViewObj()`,
  and `JDrama::TViewObj::__vtable` ownership.
- `.data` is still nonmatching, and `TLensFlare::TLensFlare(const char*)` is
  only near-matching at 99.9%.

Do not promote until the array construction / weak-owner shape and the
near-plane flare-position math are reviewed in a focused pass.
