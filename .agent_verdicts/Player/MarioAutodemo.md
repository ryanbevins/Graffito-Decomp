# mario/Player/MarioAutodemo

Verdict: equivalent
Status: equivalent
Time: 2026-06-14 2:19am MNL

## Verdict

Promoted `Player/MarioAutodemo.cpp` to `Object(Equivalent, ...)`.

## Behavior Fix

- `TMario::readBillboard()` now builds the close-range `moveRequest` target
  from the talking NPC position (`targetPos.x/z`) plus the normalized
  displacement. The previous source used Mario's current `mPosition.x/z`,
  which moved Mario relative to himself rather than relative to the billboard
  NPC.

## Reviewed Residue

- `demoMain`, `warpOut`, `isUnUsualStageStart`, `warpIn`, and the repaired
  `readBillboard` have only codegen-class differences: frame size, stack slots,
  saved-register coloring, helper-label drift caused by extra weak/template
  symbols, and equivalent vector helper call lowering.
- The overview still reports extra weak `StageUtil`/JAL-list/template symbols
  and data ownership noise. The source-link proof accepted those emissions.

## Proof

- `python configure.py --non-matching && ninja`
- `python configure.py && ninja` (`build/GMSJ01/mario.dol: OK`)
