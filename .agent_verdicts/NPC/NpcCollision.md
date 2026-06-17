# NPC/NpcCollision

Verdict: equivalent
Date: 2026-06-13 1:07pm MNL

Reverified current `Object(Equivalent, "NPC/NpcCollision.cpp")` again during
the audit-only sweep.

Reviewed functions:
- `TBaseNPC::bind()`: same next-position integration, gravity/min-Y clamp,
  `checkGroundIgnoreWaterSurface` call, ground-height bias, legal-ground
  landing path, airborne flag path, optional wall move, and final
  linear-velocity write. Residue is stack/vector-helper layout and a redundant
  target null-plane check with no side effects.
- `TBaseNPC::setVariableDamageRadius_()`: same scaled base damage radius,
  trample/Mario-airborne/height gates, horizontal-distance test, small-radius
  override, and `calcEntryRadius()` call. Residue is frame/FPR allocation.
- `TBaseNPC::execNpcObjCollision_()`: same collision loop, walking/reverse-dir
  gating, separation vector construction, degenerate-overlap fallback, overlap
  magnitude clamp, normalization, and either collided-NPC position adjustment
  or self linear-velocity adjustment. Residue is stack/FPR/register allocation
  and equivalent branch layout.
- `TBaseNPC::initNpcObjCollision_(const TNpcInitInfo*)`: same actor-type switch
  for disabling hit flags, scaled attack/damage dimensions, `initHitActor`
  arguments, no-collision flag clear, and `HIT_FLAG_UNK2` set for type zero.

No missing symbols. The source-only `TVec3<float>::sub` row is helper-owner
drift; `.sdata2` differences are local constant-label/order residue.

Validation:
- Shared proof: `python configure.py --non-matching && ninja` linked from source, then `python configure.py && ninja` restored the normal matching config and verified `build/GMSJ01/mario.dol: OK` at 2026-06-13 1:07pm MNL.
