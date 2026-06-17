Verdict: equivalent
Time: 2026-06-13 10:28am MNL
Unit: mario/Enemy/effectEnemy
Source: src/Enemy/effectEnemy.cpp

Reason:
- 2026-06-13 10:28am MNL recheck: current full diffs still show only
  codegen-class residue. `TEffectEnemy::setDeadAnm()` differs only by frame
  size. `TEffectEnemy::forceKill()` preserves the original illegal-data gate,
  BG-type kill predicate, `LIVE_FLAG_AIRBORNE`/`LIVE_FLAG_UNK10` protection,
  and out-of-map fallback; branch layout and bool-materialization shape differ
  but the kill/map-check semantics are the same.
- Re-reviewed both remaining nonmatching functions. `TEffectEnemy::setDeadAnm`
  differs only by stack frame size while preserving the particle emission,
  sound gate/start call, and `mLiveFlag |= 0x20000` store.
- `TEffectEnemy::forceKill` preserves the same behavior: skip illegal ground,
  classify death/water/ground-death BG types into the same kill predicate,
  skip forced kill while airborne or protected by `LIVE_FLAG_UNK10`, then kill
  on out-of-map area failure. The residue is branch layout, bool materialization
  shape, stack frame size, and local register choice.
- No missing symbols. Nonmatching anonymous `.rodata`/`.data`/`.sdata2` rows
  and extra weak/base rows are owner/layout drift; named vtables and live text
  behavior are present.
- Proof: `python configure.py --non-matching && ninja` linked from source, then
  plain `python configure.py && ninja` passed and verified `mario.dol: OK`.
- 2026-06-13 6:33am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.
