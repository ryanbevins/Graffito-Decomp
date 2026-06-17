# NPC/NpcEffect

Verdict: equivalent
Date: 2026-06-15 12:53pm MNL

Reason: strict audit found and fixed the remaining behavior blockers, then
proved the TU links from source under `--non-matching`. Remaining mismatches are
byte/codegen debt: local `TVec3<float>::set<float>` helper ownership,
sret/local-copy shape in `getEffectScale_()`, static frame-table/local rodata
labels, stack/register placement, and helper-boundary drift.

Behavior fixes made before promotion:
- `emitParticle_()` water-height gates now use the target ordered `<=` tests for
  the `30.0f` height limit and `waveY` comparison.
- `emitParticle_()` case `0x19` now treats normal Mare males like normal Mare
  females for the `0x173` pollution effect.

Reviewed behavior:
- `getEffectScale_()` returns `(1.0f, 1.0f, 1.0f)` for actor types
  `0x04000016..0x04000017`, otherwise returns `mEffectScaleBase`; target writes
  directly to sret while source copies through a local.
- `isPolWaitCEffectEmitTime_()` selects the Monte/Mare/Kino frame table and
  scans non-negative entries with `checkPass`; remaining differences are static
  label ownership and loop/index shape.
- `emitParticle_()` smoke/fire, note, water, center pollution, left/right
  pollution, and case `0x19` pollution paths now match the target behavior.
  Remaining mismatch clusters are helper-call boundaries, local static labels,
  stack/register shape, and address drift from missing local helper rows.
- `emitHappyEffect_()`, `emitSinkEffect_()`, `IsCheckPassFrame()`, and the
  `set*MtxPtr_()` helpers have no identified runtime gap in the reviewed diffs.

Proof:
- `python configure.py --non-matching && ninja` linked with
  `Object(Equivalent, "NPC/NpcEffect.cpp")`.
- `python configure.py && ninja` restored the normal matching config and passed
  `build/GMSJ01/mario.dol: OK`.
- Repeated both proofs after the final source line wrap; no additional source
  changes remain outside the pushed commit.
