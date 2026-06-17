# mario/M3DUtil/M3UModel

Verdict: equivalent
Date: 2026-06-13 6:06am MNL

Reason:
- Re-verified during the AUDIT sweep. All functions except
  `M3UModel::updateInMotion()` match exactly; data/vtable rows match, with
  only source-extra anonymous rodata constants.
- Full `--no-collapse` diff for `updateInMotion()` shows identical behavior:
  same loop bounds and stride, same `M3UMtxCalcSetInfo` indexing, same
  `J3DFrameCtrl::update`, same joint lookup, same `0xff` no-mtx-calc branch,
  same frame write to the selected transform, same case 0/1 basic/softimage
  array stores, same virtual `M3UModelCommon::getMtxCalc` call, and same joint
  `mMtxCalc` store. Remaining mismatches are frame size, save slots, and
  independent GPR coloring before the frame-control update.
- Proof: `python configure.py --non-matching && ninja` linked from source,
  then normal `python configure.py && ninja` passed and verified
  `mario.dol: OK`.

2026-06-13 10:11am MNL recheck:
- Current overview still has no missing target symbols. `updateInMotion` keeps
  the same loop bounds/stride, frame-control update, joint lookup, no-mtx-calc
  branch, transform frame write, basic/softimage array stores, virtual
  `getMtxCalc`, and joint `mMtxCalc` store.
- Remaining residue is frame size, save-slot layout, and independent GPR
  coloring around the frame-control path.

Offending functions: none.

2026-06-13 12:36pm MNL recheck: verdict remains `equivalent`.
Fresh full diff for `M3UModel::updateInMotion` still has identical loop
bounds and stride, frame-control update, joint lookup, no-mtx-calc branch,
transform frame write, basic/softimage array stores, virtual `getMtxCalc`
call, and joint `mMtxCalc` store. The visible differences are only frame size,
save-slot layout, and independent GPR coloring before the frame-control update.
Proof refreshed with `python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
