# mario/Player/MarioSwim

Verdict: equivalent
Status: certified
Time: 2026-06-13 5:23pm MNL

Certified `Object(Equivalent, "Player/MarioSwim.cpp")` after fixing the
`TWaterGun::isEmitting()` owner conflict. The target owns that 164B weak body in
`mario/Player/MarioRun`; `Watergun.hpp` now exposes the body only when
`WATERGUN_EMIT_IS_EMITTING` is defined by `src/Player/MarioRun.cpp`, so
`MarioSwim.o` no longer emits a duplicate.

Behavior review:

- `swimMain()` performs the same game-over/status handling, swim-jump result
  handling, wall/fence turn transition, Y clamp, FLUDD-emitting status guard,
  action switch, animations, status changes, water-sound gate, sinking/wait
  timer, jump demo paths, and boolean returns. Remaining diffs are stack-frame
  size, branch layout/dead register setup, local constant labels, and misleading
  decomp-diff call labels where raw target asm calls the expected local
  routines.
- `doSwimming()` performs the same input/status exits, water-depth exit,
  forward velocity/brake, pump-state rotation-speed choice, yaw convergence,
  gravity/buoyancy/brake, jump-process handling, splash emission, and final
  minimum-Y clamp. Remaining diffs are stack slots/FPR ordering and equivalent
  clamp expression shape.
- `swimPaddle()` and `checkSwimJump()` are byte-identical.

Proof:

- `python configure.py --non-matching && ninja` linked successfully from source
  with `Player/MarioSwim.cpp` promoted.
- `python tools/decomp-diff.py -u mario/Player/MarioSwim --search
  "TWaterGun::isEmitting"` reports no duplicate extra.
- Normal `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.

Verdict: needs_impl
Status: needs_impl
Time: 2026-06-13 5:35am MNL

## Verdict

## Reason
Do not promote. `swimMain` and `doSwimming` were previously reviewed as
codegen-class after the Y-clamp fix, but trial promotion still fails source-link
validation because `Player/MarioSwim.cpp` emits a duplicate
`TYoshi::onYoshi()` already defined in `MarioAction.o`.

Fix the weak/inline ownership of `TYoshi::onYoshi()` before re-auditing this
otherwise close TU.
