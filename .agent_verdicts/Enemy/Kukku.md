# Enemy/Kukku

Verdict: equivalent
Date: 2026-06-15 2:21am MNL

Reason: strict audit found no behavioral differences after the implementation
cleanup. Remaining text diffs are stack/frame size, register/FPR coloring,
helper-boundary differences, static nerve owner numbering, and rodata label
layout. The only missing target row is `@3359` (`"tori_hit"`), an unused plain
literal with no target code relocations; anchoring it adds a source-only pointer
object and is byte-debt, not a runtime dependency.

Reviewed functions:
- `TNerveKukkuRecoverGraph::execute` and `TNerveKukkuFall::execute`: same nerve
  transitions, animation setup, velocity/friction updates, particle emits, and
  return values; diffs are frame/static-owner and stack-slot drift.
- `TNerveKukkuGraphWander::execute`: same initial graph reset, animation
  selection, range/cooldown checks, free-ball search, hit/flag writes, sound
  call, and forward velocity update; diffs are register/stack/helper-inline and
  rodata-offset drift.
- `TKukkuManager::load`: same params, defaults, overrides, and base load call;
  diffs are constructor inlining/layout residue.
- `TKukku::dropCoins`: same early 1-up/map object path, random coin count clamp,
  coin spawn loop, live-flag clears, velocity writes, and stop-at-10 behavior;
  diffs are quaternion/rotate helper expression shape and FPR allocation.
- `TKukku::calcMomentum`, `updateRotation`, `behaveToWater`,
  `calcRootMatrix`, `init`, `TKukkuBall::perform`, and `TKukkuBall::init`:
  same calls, field offsets, branch predicates, constants, hit/live flag writes,
  particles, collision checks, and matrix/model updates; remaining gaps are
  codegen-class.
- `__sinit_Kukku_cpp`: static-init order/owner residue only.

Proof:
- `python tools/decomp-diff.py -u mario/Enemy/Kukku`
- `python configure.py --non-matching && ninja` linked with Kukku sourced.
- `python configure.py && ninja` restored matching config and passed
  `build/GMSJ01/mario.dol: OK`.
