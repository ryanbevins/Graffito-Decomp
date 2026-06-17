# Camera/CameraBck audit

Verdict: equivalent
Status: certified
Time: 2026-06-13 5:15pm MNL

Unit: `mario/Camera/CameraBck`
Source: `src/Camera/CameraBck.cpp`
Classification: `Object(Equivalent, "Camera/CameraBck.cpp")`

Certified after the `TYoshi::onYoshi()` owner split removed the prior
source-link blocker.

Behavior review:

- `TCameraBck::updateDemo(...)` performs the same `calcAnm`, position/lookat/up
  matrix extraction, optional fov transform lookup/store, optional offset
  application, and frame-state bool return.
- The end-of-function branch layout differs but implements the same result:
  return true when the frame controller is null or its state bit is set, return
  false when the bit is clear.
- Remaining data/text drift is stack-frame/slot placement, rodata/static-owner
  labels, and source-owned `__sinit`/JSUList extras.
- `TYoshi::onYoshi()` no longer appears as an extra in this TU.

Proof:

- `python configure.py --non-matching && ninja` linked successfully from source.

Verdict: needs_impl
Status: needs_impl
Time: 2026-06-13 5:35am MNL

Unit: `mario/Camera/CameraBck`
Source: `src/Camera/CameraBck.cpp`
Classification: `Object(NonMatching, "Camera/CameraBck.cpp")`

## Verdict

Do not promote yet. The only nonmatching text function,
`TCameraBck::updateDemo(...)`, is behavior-equivalent at the reviewed level:
it performs the same `calcAnm`, position/lookat/up matrix extraction, fov
`J3DAnmTransform::getTransform` lookup, optional offset application, frame
state test, and bool return. The remaining text deltas are stack-frame/slot
size, branch-layout normalization, and label/source ownership.

Temporary promotion to `Object(Equivalent, "Camera/CameraBck.cpp")` failed
the source-link proof:

- multiply-defined `TYoshi::onYoshi()` in `CameraBck.o`;
- previously defined in `MarioAction.o`.

The overview also still reports target rodata ownership drift
(`@1526`, `@1593`..`@1596`, `@1681`..`@1683`) and source-owned ctor/sinit
extras. Leave this TU red until the `TYoshi::onYoshi()` weak/inline ownership
issue is fixed, then re-audit and source-link again.
