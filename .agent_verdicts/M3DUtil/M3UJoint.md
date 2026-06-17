# M3DUtil/M3UJoint audit

Verdict: equivalent

Reviewed: 2026-06-13 7:10am MNL

Unit: `mario/M3DUtil/M3UJoint`

## Evidence

- `python tools/decomp-diff.py -u mario/M3DUtil/M3UJoint` has no missing
  target text symbols.
- `M3UMtxCalcSIAnmBlendQuat::init` is a 99.7% diff with only label/constant
  owner drift around the inherited `J3DMtxCalc` vtable path.
- `M3UMtxCalcBlendAux` is lower-fuzzy but follows the same matrix/quaternion
  blend logic with no missing calls, wrong branch predicates, or wrong data
  flow observed; the remaining differences are register, stack, and constant
  ownership/classic codegen drift.
- Recheck at 7:10am: `init` still loads an equivalent `{1,1,1}` constant from
  source-owned `@242` instead of target `.data @1411`; raw relocations confirm
  data ownership drift rather than logic drift. `M3UMtxCalcBlendAux` still
  preserves scale interpolation, `mScaleFlagArr` write, Euler-to-quaternion
  calls, quaternion lerp, matrix concat/copy, and the scaled-current-matrix
  fallback. `python configure.py --non-matching && ninja` linked successfully,
  then `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.

## Residual risk

- Data labels still drift (`@1411` vs local emitted labels/vtables), so this is
  not byte-perfect. This is audit-certified as functionally equivalent only.

Reverified: 2026-06-13 10:57am MNL — still equivalent. Re-read both
nonmatching functions. `init` still writes the same current/parent scale and
matrix data; `M3UMtxCalcBlendAux` still preserves scale interpolation,
`mScaleFlagArr` writes, Euler-to-quaternion conversion, quaternion lerp, matrix
concat/copy, and scaled-current-matrix fallback. Remaining drift is
stack/register/FPR layout plus local vtable/constant owner labels. Proof passed
again with `python configure.py --non-matching && ninja`, then plain `python
configure.py && ninja` restored the matching config and verified
`build/GMSJ01/mario.dol: OK`.

Reverified: 2026-06-13 12:39pm MNL — still equivalent. Fresh full diffs show
`init` still copies `vec` into `J3DSys::mCurrentS`, initializes parent scale to
the same `{1,1,1}` values through different constant ownership, and fills or
copies `mCurrentMtx` with the same scale-multiplied rows. `M3UMtxCalcBlendAux`
still computes `1.0f - blend`, chooses either unit scale or previous current
scale, interpolates and applies scale, writes `mScaleFlagArr[param_1]`, converts
both Euler rotations to quaternions, lerps them, builds the matrix, writes the
same translated column, then follows the same `param_5` / scale-flag matrix
concat/copy behavior. The 79% fuzzy score is stack/FPR/GPR temp placement and
local data-owner drift, not logic drift. Proof reused from this tick:
`python configure.py --non-matching && ninja`, then normal `python configure.py
&& ninja` with `build/GMSJ01/mario.dol: OK`.

Reverified: 2026-06-13 1:21pm MNL — still equivalent. Current full diffs again
show `init` preserving `mCurrentS`, parent scale, and current matrix
construction/copy behavior. `M3UMtxCalcBlendAux` still preserves scale
interpolation and scale-flag storage, both Euler-to-quaternion calls, quaternion
lerp, `PSMTXQuat`, translated-column stores, `PSMTXConcat`, and both current
matrix copy/fallback paths. Remaining drift is stack/FPR/GPR allocation, branch
address layout, and local `{1,1,1}`/vtable/constant ownership. Source-link and
normal proof from this tick passed.
