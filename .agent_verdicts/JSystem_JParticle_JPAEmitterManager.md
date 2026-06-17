# Audit: mario/JSystem/JParticle/JPAEmitterManager

## Verdict
equivalent

## Date
2026-06-14 7:02am MNL

## Proof
- Promoted `JSystem/JParticle/JPAEmitterManager.cpp` to `Object(Equivalent, ...)`.
- `python configure.py --non-matching && ninja` linked successfully from source.
- Follow-up normal `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.

## Review Notes
- Constructor now matches target-visible behavior: particle heap allocation uses
  `sizeof(JPAParticle)` (`0xdc`), emitter and field heap slot sizes match, all
  constructed objects are prepended and initialized as in target, and manager
  arrays/counts are stored to the same offsets.
- Remaining constructor drift is stack frame/register allocation plus
  JSUList ctor/dtor owner-label debt around the `unk44` array construction;
  the emitted list ctor/dtors are layout-identical and do not change behavior.
- `draw(JPADrawInfo*)` performs the same fovy/aspect writes and the same eight
  group traversal/draw loop. Its only visible drift is frame size and branch
  target/owner layout caused by source-owned `drawBase`.
- `createEmitterBase()` has the same guard conditions, resource lookup,
  emitter allocation/reconstruction, list append, base block load, color
  initialization, field loop, sweep-shape flag, draw initialization, and
  callback stores. Residue is saved-register coloring, stack size, and helper
  boundary layout.
- `createEmitter()` stores the result position and records `unkC4`/`unkC8[0][0]`
  identically; residue is stack-frame size only.
- Source-only extras `drawBase(JPADrawInfo*, u8)` and JSUList destructor
  template owners are byte-matching debt, not referenced-missing symbols; the
  source-link proof confirms they do not block functional equivalence.
