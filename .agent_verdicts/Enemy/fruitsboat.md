# Enemy/fruitsboat

Verdict: equivalent
Date: 2026-06-15 11:40pm MNL

Source-link proof:
- Promoted `Object(Equivalent, "Enemy/fruitsboat.cpp")`.
- `python configure.py --non-matching && ninja` linked `fruitsboat` from
  source.
- Restored normal config with `python configure.py && ninja`;
  `build/GMSJ01/mario.dol: OK`.

Audit fixes made before promotion:
- `TNerveFruitsBoatBckTrace::execute()` now converts the full signed sum of
  the two BCK rotations. The old source truncated the sum back to `s16`, while
  target asm converts the untruncated integer sum.
- `TFruitsBoat::moveObject()` now uses real static guard bytes for the two
  cached up vectors. The old source used the first byte of each vector as the
  guard, corrupting the stored `x` float after initialization; target has
  separate `init$2719` / `init$2733` bytes in `.sbss`.

Behavior review:
- `TNerveFruitsBoatGraphWander::execute()` matches graph/dummy early exits,
  reached-goal flag handling, attribute toggle, directed-next-node update,
  spline tracing, negative-speed `MsWrap`, walk fallback, sound request, and
  nerve push semantics. Remaining drift is stack/register/helper-boundary
  residue; raw asm confirms the suspicious call labels are local `MsWrap`.
- `TFruitsBoat::moveObject()` matches wave probe setup, wave-height tilt,
  shortest-angle clamp, Mario-on-boat transition, wave-normal updates, BG-type
  BCK selection, `mLiveFlag` edits, sway integration, and final
  `TLiveActor::moveObject()` call. Remaining low score is helper inlining,
  stack frame/register allocation, static-label naming, and equivalent
  comparison lowering.
- `init`, `load`, `setGroundCollision`, `requestShadow`, `setBckTrack`,
  `calcRootMatrix`, manager load/model-data, and all exact rows have matching
  calls, offsets, constants, stores, and control-flow semantics.

Remaining byte debt:
- Missing target data labels `@2333`, `@2335`, `@2707`, and `@2709`.
  `@2333`/`@2335` are source-owned as `dummy2333`/`dummy2335`; `@2707`/`@2709`
  are the target `.sdata` clamp constants `1.0f` and `-1.0f`.
- Nonexact rows are codegen/data-label debt, not behavior blockers.
