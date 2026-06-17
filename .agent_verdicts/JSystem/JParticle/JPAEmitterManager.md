Verdict: equivalent
Date: 2026-06-14 2:15pm MNL
TU: `mario/JSystem/JParticle/JPAEmitterManager`

Proof:
- `configure.py` currently marks `JSystem/JParticle/JPAEmitterManager.cpp` as
  `Object(Equivalent, ...)`.
- The 2026-06-14 2:11pm MNL `python configure.py --non-matching && ninja`
  source-link proof passed with this TU enabled.
- The follow-up `python configure.py && ninja` restored the normal matching
  build and passed `build/GMSJ01/mario.dol: OK`.

Reason:
- No missing target symbols remain in the current overview.
- The previous implementation pass fixed the real constructor behavior bug:
  the particle heap now allocates `param_2 * sizeof(JPAParticle)` slots
  (`0xdc` bytes each), matching the concrete placement-new object.
- Removed target-absent API owners are not referenced by the rebuilt object.
- Remaining function diffs are behavior-neutral frame/register/helper-owner
  residue in the constructor, `draw(JPADrawInfo*)`, `createEmitterBase(...)`,
  and `createEmitter(...)`.
- Remaining source-only symbols (`drawBase(JPADrawInfo*, u8)` and
  `JSUList<JPABaseField/JPABaseParticle>::~JSUList()`) are emitted ownership
  debt; the source-link proof shows they do not create undefined or duplicate
  symbol blockers.
