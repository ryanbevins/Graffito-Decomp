# Enemy/rocket audit

Verdict: `equivalent`

Checked 2026-06-14 4:36am MNL in AUDIT mode.

`mario/Enemy/rocket` is behavior-equivalent and source-links under
`--non-matching`.

Fixes made during this audit:
- `TRocket::calcRootMatrix()` now applies `mNozzleOffsetZ` in the local X
  translation slot (`rotOff[0][3]`), matching target asm at
  `build/GMSJ01/asm/Enemy/rocket.s:1261`. The prior source put the offset in
  local Z and moved the rocket along the wrong matrix axis.
- The water-gun matrix normalization block now computes all three column
  lengths before the divide guards, with the target's cross-gated conditions:
  length2 gates column 0, length0 gates column 1, and length1 gates column 2.
  This preserves the original branch behavior for degenerate emit matrices.

Review summary:
- `TNerveRocketFly::execute()` and
  `TNerveRocketPossessedNozzle::execute()` have the same BCK, sound, rumble,
  input, hit-point, particle, and nerve-push behavior. Residue is stack/frame
  size, bool materialization shape, temp placement, singleton owner labels, and
  `TSpineBase::pushNerve` inline layout.
- `TRocket::setDeadAnm()`, `bind()`, `attackToMario()`,
  `TRocketManager::perform()`, `initSetEnemies()`, and `load()` have behavior
  aligned with the target. Remaining diffs are codegen/data-class: stack and
  register layout, model-matrix pointer lifetime, loop shape, TParam constructor
  inlining, local static/weak ownership, and rodata/data/sdata label/layout
  drift.
- No target symbols are missing. Extra weak/local helpers are unreferenced or
  source-inlined byte debt and do not block source linking.

Proof:
```bash
python configure.py --non-matching && ninja
python configure.py && ninja
```

The `--non-matching` build linked `Enemy/rocket.cpp` from source, and the
normal matching build ended with `build/GMSJ01/mario.dol: OK`.

Recheck 2026-06-14 3:30pm MNL:
- Found and fixed one stale behavior bug in
  `TNerveRocketPossessedNozzle::execute()`. Target pushes
  `TNerveRocketFly` onto the spine stack (`pushAfterCurrent` shape) and then
  returns `TRUE`, so the next update pops into Fly. Current source used
  `pushNerve`, which first pushed the current possessed-nozzle nerve and then
  had `update()` clear the newly set current nerve because the execute return
  was `TRUE`; this could return to the wrong nerve.
- Source now calls `spine->pushAfterCurrent(&TNerveRocketFly::theNerve())`.
  `TNerveRocketPossessedNozzle::execute()` moved `92.9% -> 97.9%`; remaining
  residue is frame size, local singleton/static-label ownership, and a
  redundant `firePressed` return retest.
- Re-read `calcRootMatrix`, `TRocketManager::perform`, and both large rocket
  nerves. After the push fix, remaining diffs are codegen/source-shape debt:
  matrix temporary layout, saved FPR/GPR choices, helper/singleton owners, and
  equivalent loop/block layout. No missing target symbols remain.
- Proof: `python configure.py --non-matching && ninja` linked with rocket
  sourced, then `python configure.py && ninja` restored normal config and
  passed `build/GMSJ01/mario.dol: OK`.
