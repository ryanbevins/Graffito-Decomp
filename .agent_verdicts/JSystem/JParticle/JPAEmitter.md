# JSystem/JParticle/JPAEmitter audit

Verdict: `equivalent`

Checked 2026-06-14 6:48am MNL in AUDIT mode.

Promoted `JSystem/JParticle/JPAEmitter.cpp` to `Object(Equivalent, ...)`.

Strict review after the implementation fixes found no remaining behavior
blocker. The fixed source now matches target-visible behavior for:

- `JPABaseEmitter::calcEmitterGlobalParams()`: copies X/Y/Z basis rows from
  `JPAEmitterInfoObj.unkCC` into `JPAEmitterInfoObj.unk48` instead of reusing X.
- `JPABaseEmitter::calcKeyFrameAnime()`: keyframe case 10 writes
  `mDraw.unkB4` at emitter offset `0xe4`.
- `createParticle()` and `createChildParticle()`: same allocation/list
  movement, status bits, fixed-interval sphere/circle/line indexing, random
  draw counts, volume-shape vector math, scale inheritance, velocity/normal
  setup, dynamics/air/life calculations, and JPADraw init calls.
- `loadBaseEmitterBlock()`, `calcCreateParticle()`, `doParticle()`, `calc()`,
  constructor, and deletion loops: same calls, reads/writes, branch predicates,
  constants, and state/status transitions.

Remaining diffs are codegen/data debt: stack frame size, saved GPR/FPR coloring,
temp layout, helper-boundary ownership (`inv_sqrt`, vector normalize/set
helpers), local label/rodata/sdata2 numbering, and extra weak/header helper
emissions. No missing target symbols remain.

Proof:

- `python configure.py --non-matching && ninja` linked successfully with
  `JPAEmitter` source-linked.
- Follow-up normal `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.
