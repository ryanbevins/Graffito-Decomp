# Strategic/MirrorActor audit

Verdict: equivalent
Date: 2026-06-13 7:47am MNL

Reason: reverified in the audit-only sweep. All nonmatching function diffs are
codegen-class after the prior `TMirrorActor::perform(unsigned long,
JDrama::TGraphics*)` weight-matrix count fix, and `python configure.py
--non-matching && ninja` links the TU from source. Raw asm confirms apparent
objdiff call-name mismatches are relocation-label drift: `perform` calls
`checkIsInMirror()`, and `init` calls the JGadget iterator/list helpers.

Function review:
- `TMirrorActor::init(J3DModel*, unsigned short)`: allocation, model creation,
  mirror-scene registration, and draw-buffer setup match. Remaining drift is
  JGadget iterator helper ownership/source-labeling and stack-slot layout.
- `TMirrorActor::entryMirrorDrawBufferAlways(J3DModel*)`: searches, draw-buffer
  stores, and model virtual calls match. Remaining drift is stack frame size.
- `TMirrorActor::perform(unsigned long, JDrama::TGraphics*)`: control flow,
  calls, matrix-array offsets, and joint/weight-matrix loop counts match.
  Remaining drift is frame size and register coloring in the `PSMTXCopy` loops.
- `TMirrorActor::checkIsInMirror()`: mirror visibility, cube lookup, plane
  check, and final flag stores match. Remaining drift is frame size and an
  equivalent `addi` + `lwz` form for `gpMirrorModelManager->unk18`.

Notes:
- Source still emits extra weak/infectious-string owners, but the required
  `--non-matching` source-link proof passed.
- Plain `python configure.py && ninja` also passed with `mario.dol: OK` after
  the proof build, leaving the repo in matching configuration.

2026-06-13 10:45am MNL recheck: verdict remains `equivalent`. Re-read the
current diffs for `init`, `entryMirrorDrawBufferAlways`, `perform`, and
`checkIsInMirror`. The init/list insertion and draw-buffer lookup paths still
make the same allocations, registrations, and virtual calls. `perform` still
copies the same model/weight matrices using the same loop counts and calls
`checkIsInMirror` before the mirror draw-buffer entry. `checkIsInMirror` keeps
the same visibility bit handling, cube lookup, data-number comparison,
mirror-plane test, and final flag stores. Drift is stack frame/slot size,
register coloring, JGadget/helper labels, and equivalent `addi+lwz` versus
offset `lwz` shape. Proof refreshed with `python configure.py --non-matching
&& ninja`, then normal `python configure.py && ninja` with
`build/GMSJ01/mario.dol: OK`.
