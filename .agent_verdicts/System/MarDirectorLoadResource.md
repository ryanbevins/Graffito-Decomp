# System/MarDirectorLoadResource

Verdict: equivalent
Date: 2026-06-15 1:45pm MNL

Reason:
- Certified `Object(Equivalent, "System/MarDirectorLoadResource.cpp")`.
  `python configure.py --non-matching && ninja` linked with this TU sourced,
  then plain `python configure.py && ninja` passed
  `build/GMSJ01/mario.dol: OK`.
- `loadParticleMario()` is behavior-identical despite frame-size/rodata-label
  accounting residue; it performs the same flag checks, resource loads, IDs,
  and flag stores.
- `loadParticle()` has the same archive load/mount/current-archive path,
  `loadParticleMario()` call, resource-load sequence, emitter-manager store,
  BossHanachan conditional archive path, and heap-free tail. Remaining drift is
  frame/local placement and a source-only `mr r3, this` before a call whose
  callee ignores `this`.
- `loadResource()` has the target stage/scenario sizing switch, resource and
  emitter-manager allocations, `gpMarioParticleManager->unk3B8` assignment,
  archive mount/error returns, stage archive path, params allocation direction,
  and THP return lattice. Remaining drift is codegen-only: `JKRDvdFile` stack
  placement, dead `r3=0` setup before the no-arg `THPPlayerInit`, source-only
  inlined `thpInit()` owner, extra `clrrwi` before THP buffer allocation, and
  frame offsets.
- Extra JSUList destructors and infectious-string data come from rogue includes
  for static init/BSS shape; no target symbols are missing and the source-link
  proof has no undefined references.

## Prior ready_for_audit handoff

Date: 2026-06-15 1:35pm MNL

Implementation update:
- `TMarDirector::loadResource()` now reconstructs the target
  stage/scenario resource sizing switch:
  default `effectInfoCount=0x20`, `emitterCount=1000`, `drawCount=0x100`;
  stage `0x21` -> `3000`/`0x78`; stage `5` scenario `1` and stage `9`
  scenario `0` -> `1500`; stage `0x3A` -> `4000`; stages `0x38`,
  `0x39`, `0x34`, and stage `4` scenario `2` -> `3000`; stage `0x3C`
  -> `5000`.
- Restored the target assignment of the first `JPAEmitterManager` to
  `gpMarioParticleManager->unk3B8`.
- Changed `params.arc` blob/archive allocation to `new (-0x20)` to match
  target allocation direction.

## Prior verdict

Verdict: needs_impl
Date: 2026-06-13 1:07am MNL

Reason:
- Not promoted. `TMarDirector::loadResource()` is structurally incomplete in
  source (`TODO: giant switch`) and only reports 72.4% text match, so the TU
  cannot be functionally certified.

Blocking function:
- `TMarDirector::loadResource()` needs the missing area/resource switch and
  related allocation/count behavior reconstructed.

Not fully audited:
- `TMarDirector::loadParticleMario()` reports 100.0% fuzzy but is still
  displayed as nonmatching, likely label/accounting residue.
- `TMarDirector::loadParticle()` reports 99.8% fuzzy; review after
  `loadResource()` is implemented.

## Recheck

Date: 2026-06-15 4:49am MNL

Current overview has no missing target symbols, but the verdict remains
`needs_impl`. `TMarDirector::loadResource()` is still the blocking structural
row at 72.4% and the source still contains the explicit `TODO: giant switch`
placeholder before resource-manager/archive setup. Do not promote until that
area/resource switch and its allocation/count behavior are reconstructed.
