# Map/PollutionManager Audit

Verdict: equivalent
Time: 2026-06-13 6:32am MNL

## Verdict
equivalent — 2026-06-12 7:24pm MNL

## Reason
`Map/PollutionManager.cpp` links from source under
`python configure.py --non-matching && ninja`, and the normal matching build
still verifies `mario.dol: OK`.

Reviewed all three nonmatching text functions:
- `TPollutionManager::load(JSUMemoryInputStream&)`
- `TPollutionManager::cleanedAll() const`
- `TPollutionManager::clean(float, float, float, float)`

The behavior matches the target. `load` performs the same base load, pollution
info resource lookup, joint count write, data-address patching, Mare table
selection, texture resource loads, counter initialization, layer registration,
object counter initialization, and texture-stamp registration; the main residue
is target `bl setDataAddress` versus source inlining of the matching helper.
`cleanedAll` is the same pollution-degree accumulation and cleaned-threshold
comparison with only frame/register layout differences. `clean` is frame-only.

No missing target symbols were reported. The source-owned extra helpers and
rogue-include symbols link cleanly in the source DOL.

2026-06-13 6:32am MNL recheck: overview still has no missing target rows, and
`python configure.py --non-matching && ninja` linked from source.

Reverified: 2026-06-13 10:57am MNL — still equivalent. Re-read
`load`, `cleanedAll`, and `clean`. The load path still performs the same base
load, pollution-info resource lookup, data-address patching/new-joint-model
helper behavior, Mare table selection, texture resource loads, counter/layer
registration, and texture-stamp registration. `cleanedAll` still accumulates
the same pollution degree and threshold check, and `clean` still gates and
dispatches each layer clean call. Remaining drift is stack/register layout,
helper inlining/owner labels, and source-owned extra helper rows. Proof passed
again with `python configure.py --non-matching && ninja`, then plain `python
configure.py && ninja` restored the matching config and verified
`build/GMSJ01/mario.dol: OK`.
