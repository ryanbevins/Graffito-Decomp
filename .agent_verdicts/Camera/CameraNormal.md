# Camera/CameraNormal audit

Verdict: equivalent

Checked 2026-06-15 6:35am MNL during AUDIT.

Certified `mario/Camera/CameraNormal` as `Equivalent`. The 6:27am
implementation cleanup removed the only source-link blockers: target-absent
static initializer/ctors, sound-list destructor owners, and the local
`TNameRefAryT<TStagePositionInfo>::searchF` helper owner. Re-read the two
nonmatching routines as behavior-identical: `calcTowerCenterPos_()` has the
same tower-mode switch, zero-vector fallback, virtual position-holder lookup,
and word-copy of stage position data; `ctrlNormalOrTowerCamera_()` keeps the
same mode gates, aim/parallel-camera transition, tail tracking, and
normal/tower-control dispatch. Remaining diffs are codegen/data-label debt
only: frame/register/FPR allocation, cached pointer lifetimes, branch layout,
bool materialization, and local rodata symbol names.

Proof:
- `python configure.py --non-matching`
- `ninja` linked with `CameraNormal.o` sourced.
- `python configure.py`
- `ninja` restored normal matching config and passed `build/GMSJ01/mario.dol: OK`.

Checked 2026-06-13 1:39am MNL during AUDIT sweep.

Unit: `mario/Camera/CameraNormal`

Blocking evidence:
- Missing rodata includes
  `CPolarSubCamera::calcTowerCenterPos_(Vec*)::sPositionNameTable` and several
  camera infectious-string labels.
- Source emits extra `__sinit_CameraNormal_cpp`, ctor records, and sound-list /
  Yoshi / `TNameRefAryT::searchF` helper owners, while target does not.
- `CPolarSubCamera::ctrlNormalOrTowerCamera_()` remains 81.4% and
  `calcTowerCenterPos_()` remains 84.0%; these were not certifiable after the
  static-data ownership mismatch.

2026-06-14 12:18pm MNL recheck:
- Still `needs_impl` under the current behavioral audit bar. The missing local
  rodata table alone would be byte-debt, and the two visible function diffs
  mostly read as control-flow/codegen-equivalent, but the source object still
  emits `__sinit_CameraNormal_cpp` plus a `.ctors` entry. That is a target-absent
  startup side effect if the TU is sourced, not harmless symbol accounting.

Do not promote until the local static table and unwanted static-initializer /
weak-owner emission are resolved, then re-audit the two camera routines.

2026-06-15 6:27am MNL implementation recheck:
- Source-link blockers are resolved. `CameraNormal.cpp` no longer includes the
  heavy `FireWanwan.hpp`, `MarioMain.hpp`, `PositionHolder.hpp`, or
  `NameRefAry.hpp` chains; it uses narrow local declarations for
  `TFireWanwanTailHit::getHostPos()`, `gpMarioOriginal`, and a
  `JDrama::TNameRef`-derived position-holder type so `searchF` remains a
  virtual call through vtable slot `0x1c`.
- The previous target-absent `__sinit_CameraNormal_cpp`, `.ctors`, sound-list
  destructor owners, and local `TNameRefAryT<TStagePositionInfo>::searchF`
  extra rows are gone.
- The infectious camera strings and tower position-name pointer table are now
  present; objdiff reports aggregate `.rodata`, `.data`, and `.sdata2` rows as
  100%. The remaining missing/extra rows are local label-name accounting
  (`@1490/@1526/@1857-@1860` and the target localstatic table name versus the
  source names), not missing bytes.
- Re-read `calcTowerCenterPos_()`: behavior matches the target switch over
  modes `0x27/0x28/0x29/0x37/0x41`, zero-vector default, virtual
  `gpPositionHolder->searchF(calcKeyCode(name), name)`, and word-copy of
  `TStagePositionInfo::unkC` on hit.
- Re-read `ctrlNormalOrTowerCamera_()`: remaining differences are codegen-class
  frame/register/FPR allocation, cached pointer lifetimes, mode-compare order
  for the `0x12/0x2b` cases, bool materialization around the aim/parallel-camera
  gate, and local label names. I did not find a behavior gap.
- Proof passed: `python configure.py --non-matching && ninja`, then normal
  `python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
