# mario/JSystem/JAudio/JASystem/JASWSParser

Verdict: equivalent
Date: 2026-06-13 8:39am MNL

Reverified current `Object(Equivalent, ...)` row during the audit sweep.

Reason:
- `python tools/decomp-diff.py -u mario/JSystem/JAudio/JASystem/JASWSParser`
  still reports no missing or extra symbols; all other functions match exactly.
- Full diffs for `createBasicWaveBank(void*)` and `createSimpleWaveBank(void*)`
  preserve heap selection/allocation, offset conversion, group/wave loops,
  `TWaveInfo` copies, wave-table sizing, wave-arc filename assignment, and
  heap-usage accounting. Residue is stack/register coloring, plus one elided
  `mr r4,r0` where the masked wave id is already in `r4` before the same call.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 12:55pm MNL recheck:
- Current overview still has no missing or extra target symbols.
- Re-read `createBasicWaveBank(void*)` and `createSimpleWaveBank(void*)`.
  Heap selection/allocation, offset conversions, group/wave loops,
  `TWaveInfo` copies, wave-table sizing, wave-arc filename assignment, and
  heap-usage accounting still match behavior. Residue is stack/register
  coloring and the known elided `mr r4,r0` before the same simple-wave
  `setWaveInfo` call. Reused this tick's successful source-link and normal DOL
  proof batch.

---

Verdict: equivalent
Date: 2026-06-13 4:28am MNL

Reason:
- `python tools/decomp-diff.py -u mario/JSystem/JAudio/JASystem/JASWSParser`
  reports no missing or extra symbols. All other functions match exactly.
- `JASystem::WSParser::createBasicWaveBank(void*)` is 98.8% and exact-size.
  The full `--no-collapse` diff shows identical heap selection/allocation,
  control-block offset conversion, group/wave loops, `TWaveInfo` copies,
  wave-table sizing, wave-arc filename assignment, and heap-usage accounting.
  Residue is stack frame / temp-slot placement and GPR coloring.
- `JASystem::WSParser::createSimpleWaveBank(void*)` is 98.1% and exact-size.
  The full diff shows identical single-group validation, allocation, wave max
  scan, wave-info loop, file-name assignment, and heap accounting. Residue is
  stack/register coloring plus an elided `mr r4,r0` because the source-side
  mask writes the wave id directly into `r4` before the same call.
- Re-verification of existing `Object(Equivalent, ...)` linked cleanly under
  `python configure.py --non-matching && ninja`.

Offending functions: none.
