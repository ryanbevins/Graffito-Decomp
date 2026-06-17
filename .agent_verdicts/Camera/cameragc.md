# Camera/cameragc

Verdict: equivalent
Status: promoted
Date: 2026-06-15 2:33pm MNL

`configure.py` now classifies `Camera/cameragc.cpp` as
`Object(Equivalent, ...)`.

Audit review: no missing target `.text` rows remain. The non-exact functions
(`perform`, `ctrlGameCamera_`, `calcFinalPosAndAt_`, `calcPosAndAt_`,
`calcSlopeAngleX_`, `isMomentDefinite_`, `isMarioCrabWalk_`,
`isMarioAimWithGun_`, `onMoveApproach_`, stick rotation helpers,
`calcNowTargetFromPosAndAt_`, `loadAfter`, `JetCoasterDemoCallBack`, and the
constructor) preserve the target operations: same calls, branch conditions,
camera mode/state updates, allocations, save-param loop, startup/demo resource
names, matrix setup, wall/roof/ground checks, and slope/nozzle math. Remaining
differences are codegen/data debt: stack-frame/local placement, register/FPR
coloring, helper-boundary choices (`TVec3::set`/`setLength`/`inv_sqrt`,
template owner labels), source-only weak/base extras, and rodata/static-init
ownership drift.

Source-link proof:

- `python configure.py --non-matching && ninja` linked with `cameragc` sourced.
- Plain `python configure.py && ninja` then passed with
  `build/GMSJ01/mario.dol: OK`.

## Prior implementation handoff

Implementation cleared the prior missing-helper blocker. Focused
`tools/decomp-diff.py` searches now show these target helper rows at 100.0%:

- `MsClamp<float>(float, float, float)`
- `JGeometry::TUtil<float>::one()`
- `JGeometry::TVec3<float>::set<float>(float, float, float)`
- `MsSqrtf(float)`
- `CLBEaseInInbetween<float>(float, float, float)`
- `CLBTwoDegreeGeneralInbetween<float>(float, float, float, float)`

Source-link proof:

- Temporary `Object(Equivalent, "Camera/cameragc.cpp")` passed
  `python configure.py --non-matching && ninja`.
- `configure.py` was restored to `Object(NonMatching, ...)`.
- Normal `python configure.py && ninja` then passed with
  `build/GMSJ01/mario.dol: OK`.

This was the handoff state before the full audit above.

## Prior verdict

Verdict: needs_impl
Date: 2026-06-13 3:26am MNL

Reason: target math/vector helper text symbols are missing from the rebuilt TU.

Offending symbols:
- `MsClamp<float>(float, float, float)`
- `JGeometry::TUtil<float>::one()`
- `JGeometry::TVec3<float>::set<float>(float, float, float)`
- `MsSqrtf(float)`
- `CLBEaseInInbetween<float>(float, float, float)`
- `CLBTwoDegreeGeneralInbetween<float>(float, float, float, float)`

Audit evidence: `python tools/decomp-diff.py -u mario/Camera/cameragc`.
