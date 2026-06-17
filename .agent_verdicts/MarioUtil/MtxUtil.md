# mario/MarioUtil/MtxUtil

Verdict: equivalent  
Date: 2026-06-13 3:05am MNL

`MarioUtil/MtxUtil.cpp` is functionally identical and links from source.

Reviewed the unit overview, `state/notes/MtxUtil.md`, and the full diff for
the lowest-scoring `TRope::TRope` constructor. The 0% constructor row is
codegen-class: target uses a different unroll/loop lowering, but both sides
allocate the same `TRopePoint` array and initialize each point with the same
position/previous-position copies, zero velocity, segment length, and flags.
Other remaining differences are also codegen/data debt: frame/local-slot layout,
register/FPR coloring, vector helper boundary choices, matrix/quaternion branch
shape, static rodata labels, and weak `TVec3` helper ownership.

The source preserves the same rope constraints/collision/head motion, light
perspective matrix setup, joint arc interpolation, multi-matrix setup, swing
callbacks, time-lag matrix/quaternion filtering, and quaternion conversion
behavior.

Proof:

- `python configure.py --non-matching && ninja` linked `mario.dol` from source.
- `python configure.py && ninja` passed and verified `build/GMSJ01/mario.dol:
  OK`.

## 2026-06-13 11:24am MNL recheck

Refreshed during the stale-Equivalent sweep. `powerpc-eabi-nm -u` on the source
object shows no unresolved `TVec3::sub` / copy-constructor references, so the
extra helper rows are owner drift. Full diffs rechecked `TRope::TRope` and
`TMtxTimeLag::calc`: the rope constructor still initializes the same point array
fields with a different unroll strategy, and raw target asm confirms both
time-lag quaternion paths call `MtxToQuat` despite misleading pretty-diff
labels. Remaining `.rodata`/`.sdata2` rows are label/constant ownership debt.

Proof passed:
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja` (`build/GMSJ01/mario.dol: OK`)
