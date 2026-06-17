# mario/System/EmitterViewObj

Verdict: equivalent
Date: 2026-06-13 3:40pm MNL

Reason:
- Source now matches both behavior blockers found in audit/implementation:
  `SMSSetEmitterPolColor(JPABaseEmitter*, int)` writes the second color triple
  to environment bytes `0x184-0x186`, and `TMarioParticleManager::perform()`
  re-emits normal `unk10` entries (`253..466`) through group `1` while the
  `unk368` entries (`486..504`) stay group `3`.
- Full focused diffs for the nonmatching functions show no remaining
  behavior-bearing mismatches. The remaining residue is stack/frame size,
  saved-register coloring, identical bool predicate layout, local constant /
  callback label ownership, and data/ctor owner drift.

Proof:
- `python tools/decomp-diff.py -u mario/System/EmitterViewObj -d "TMarioParticleManager::perform" -C 6 --no-collapse`
  shows the group arguments matching (`li r6, 3` in the `unk368` loop and
  `li r6, 1` in the `unk10` loop).
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored matching config and verified
  `build/GMSJ01/mario.dol: OK`.

Verdict: fixed_by_implementation
Date: 2026-06-13 3:31pm MNL

Implementation fixed the behavior blocker in
`SMSSetEmitterPolColor(JPABaseEmitter*, int)`. The shared inline
`JPABaseEmitter::setEnviColor(u8, u8, u8)` now stores into the environment
color bytes `unk184/unk185/unk186` (`0x184-0x186`) instead of clobbering
`unk180` parameter color bytes.

Build / diff proof:
- `python configure.py && ninja` passed and verified
  `build/GMSJ01/mario.dol: OK`.
- `python tools/decomp-diff.py -u mario/System/EmitterViewObj -d "SMSSetEmitterPolColor" -C 6 --no-collapse`
  now shows the second color triple stored to `0x184`, `0x185`, and `0x186`,
  matching target behavior.

Ready for the next AUDIT tick to source-link certify as `Equivalent`. Remaining
residue in the focused function is register coloring (`value` in `r4` vs `r0`)
and local data-label ownership (`@1431` vs `envarray$571`), not behavior drift.

Verdict: not_equivalent
Date: 2026-06-12 10:39pm MNL

Reason:
- `SMSSetEmitterPolColor(JPABaseEmitter*, int)` has an observable field-offset mismatch. The target stores the second computed color group into the emitter's environment color bytes at offsets `0x184`, `0x185`, and `0x186`; the current source build stores that group back into `0x180`, `0x181`, and `0x182`, clobbering the parameter color bytes.
- The missing/extra `.ctors` arrays (`@1431`, `@1411`, `@1210` vs `dummy1431`, `dummy1411`, `dummy1210`) look like label ownership residue, and most remaining `.text` differences are stack/register/label drift. The color-field mismatch is enough to block `Equivalent`.

Evidence:
- `python tools/decomp-diff.py -u mario/System/EmitterViewObj -d "SMSSetEmitterPolColor" -C 5`
- Target tail writes param color to `0x180-0x182`, then env color to `0x184-0x186`; source tail writes both groups to `0x180-0x182`.

Reverification: 2026-06-13 8:21pm MNL

- Current overview still matches the certified shape: the known `.ctors` dummy
  label drift and weak/helper extra emissions are unchanged, and the behavior
  blockers listed above remain fixed.
- The current tick's `python configure.py --non-matching && ninja` proof linked
  all `Equivalent` rows from source, then `python configure.py && ninja`
  restored the matching config with `build/GMSJ01/mario.dol: OK`.
