# JSystem/J3D/J3DGraphAnimator/J3DJoint audit

Verdict: equivalent  
Date: 2026-06-13 7:25am MNL

Reason: all text/data/rodata symbols are present and
`python configure.py --non-matching && ninja` links successfully with
`JSystem/J3D/J3DGraphAnimator/J3DJoint.cpp` promoted to
`Object(Equivalent, ...)`.

Reviewed nonmatching functions:
- `J3DMtxCalcBasic::calcTransform`: same current-scale multiplication,
  scale-flag store, translate/rotate matrix call, optional scale application,
  concat, model matrix lookup, and `PSMTXCopy`; residue is stack offset and
  register-expression ordering.
- `J3DMtxCalcSoftimage::calcTransform`: same scaled translation matrix build,
  concat, current-scale update, scale-flag path, scaled-copy path, direct-copy
  path, and target matrix store; residue is stack/register layout.
- `J3DMtxCalcMaya::calcTransform`: same scale-compensate lookup, scale-flag
  path, translate/rotate matrix build, optional scale and parent-scale
  compensation, concat/copy, and parent-scale stores; residue is stack/register
  layout.
- `J3DJoint::updateIn` and `J3DJoint::entryIn`: material iteration, hidden
  material skips, optional callbacks, material calc/current-mtx updates,
  packet fields, draw-buffer dispatch, display-list calls, and loop exits
  match; residue is stack-frame size/branch labels.
- `J3DMatPacket::entry`: same sort function pointer-to-member table lookup and
  `__ptmf_scall`; residue is stack slot placement.

Notes:
- No missing or extra symbols were reported for this TU in the audit overview.

Reverified in the current audit sweep. Current overviews still report no
missing or extra symbols. Re-read the current diffs for
`J3DMtxCalcBasic::calcTransform`, `J3DMtxCalcSoftimage::calcTransform`,
`J3DMtxCalcMaya::calcTransform`, `J3DJoint::updateIn`, `J3DJoint::entryIn`,
and `J3DMatPacket::entry`; the matrix scale/compensation/copy operations,
material iteration/callback/display-list paths, draw-buffer entry dispatch,
stores, predicates, and constants remain behavior-identical. Residue is
stack/frame layout, register coloring, and independent load/store scheduling.
Source link proof passed in the same batch as the 2026-06-13 7:25am MNL notes
refresh.

2026-06-13 10:43am MNL recheck: verdict remains `equivalent`. Re-read the
current diffs for `J3DMtxCalcBasic`, `J3DMtxCalcSoftimage`,
`J3DMtxCalcMaya`, `J3DJoint::updateIn`, `J3DJoint::entryIn`, and
`J3DMatPacket::entry`. The three matrix calculators still perform the same
scale multiplication, scale-flag updates, matrix construction, parent-scale
compensation, concat/copy, and parent-scale stores. Joint update/entry still
iterate materials with the same hidden/callback/current-mtx/display-list logic,
and the packet entry still dispatches through the same sort-function
pointer-to-member. Residue is stack frame/slot layout, register/FPR coloring,
and independent matrix temp load/store scheduling. Proof refreshed with
`python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
