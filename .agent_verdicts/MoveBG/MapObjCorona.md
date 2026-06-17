# MoveBG/MapObjCorona Audit

Verdict: equivalent
Date: 2026-06-16 MNL

Functionally equivalent under --non-matching: compiles, links with no
undefined refs, and behaves identically to the target.

What was wrong / fixed:

- Missing `std::fmodf(float, float)` owner. MapObjCorona is the owner TU for
  the out-of-line `fmodf__3stdFff` (other TUs, e.g. Enemy/BathtubPeach,
  reference it as an undefined-weak). The source used inline `std::fmodf`,
  which compiled to a call to the undefined double `::fmod`. Fixed by defining
  `MSL_STDFMODF_OUT_OF_LINE` (so the MSL header exposes only a declaration) and
  reconstructing the real MSL float `std::fmodf` body in the .cpp:
  `if (fabsf(y) > fabsf(x)) return x; else return x - y*(f32)(s64)(x/y);`.
  The emitted `fmodf__3stdFff` body is byte-identical to the target (size 0x5c,
  same `__cvt_dbl_usll`/`__cvt_sll_flt` sequence). Removes the `::fmod`
  undefined reference and provides the weak owner the other TUs link against.

- `updatePosture_()` used a wrong closed-form for the restoring-axis/angle
  (sign error on axis.z and `acosf(upY)` instead of `acosf(-upY)`). The target
  rotates a static `yDown(0,-1,0)` by the orientation quaternion and forms the
  restoring axis from cross products of yDown with the rotated up-vector, with
  `acosf(yDown . up)`. Rewrote both the recover branch and the angle-limit
  branch to match the target's cross-product axes and dot-based angles, and
  introduced the `static yDown(0,-1,0)` (shared with control()). Match went
  15% -> 63%; behavior (calls/stores/branch-conditions/constants) now matches.

Remaining (non-behavioral / codegen only):
- `std::fmodf`, `JGeometry::TVec4<f32>::dot/scale`, `set<f>__TVec3`, and the
  empty `SMatrix33R<f>` ctor are inlined or omitted vs. emitted-weak in the
  target. None affect behavior (fmodf/dot/scale/set are pure leaves that
  inline to identical code; the SMatrix33R ctor is an empty no-op) and none are
  referenced as undefined by other TUs, so linking is unaffected.
- `fmodf__3stdFff` is emitted global (target weak); it is the sole definition,
  so no duplicate-symbol conflict at link.
