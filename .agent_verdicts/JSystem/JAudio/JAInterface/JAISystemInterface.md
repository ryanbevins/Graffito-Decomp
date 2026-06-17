## Verdict: equivalent

Date: 2026-06-13 5:50am MNL
Unit: `mario/JSystem/JAudio/JAInterface/JAISystemInterface`
Source: `src/JSystem/JAudio/JAInterface/JAISystemInterface.cpp`
Classification: `Object(Equivalent, "JSystem/JAudio/JAInterface/JAISystemInterface.cpp")`

Reason:
- Re-verified during the audit sweep. The overview has no missing target
  symbols. Most functions and static data match byte-for-byte.
- `setSeqPortargsF32` and `setSeqPortargsU32` compute the same
  `unk4C[param_2].unk4[param_3]` target address and store the incoming value.
  The diff is only address-expression codegen (`add` plus base-offset store
  versus indexed `stfsx`/`stwx`).
- `setSePortParameter` preserves the same null-track guard, flag tests,
  `TOuterParam::setParam` calls, flag clears via `xori`, and optional
  interrupt trigger. Remaining differences are stack-frame and save-slot
  offsets only.
- Extra empty/dead helper functions are target-absent but unused; source-link
  validation succeeds.

Proof:
- `python configure.py --non-matching && ninja` linked
  `build/GMSJ01/mario.dol` from source.
- `python configure.py && ninja` restored the matching config and passed
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 9:55am MNL recheck:
- Current overview still has no missing target symbols.
- Re-read full diffs for `setSeqPortargsF32`, `setSeqPortargsU32`, and
  `setSePortParameter`. The two port-arg helpers still write the same indexed
  `unk4C[param_2].unk4[param_3]` slot with only indexed-store versus
  base-plus-offset codegen drift. `setSePortParameter` still performs the same
  null guard, flag tests, `setParam` calls, flag clears, optional interrupt,
  and return behavior; residue is frame/save-slot layout.
- Proof batch passed: `python configure.py --non-matching && ninja`, then
  `python configure.py && ninja` with `mario.dol: OK`.

2026-06-14 10:07pm MNL recheck:
- Current overview still has no missing target symbols.
- Re-read no-collapse diffs for `setSeqPortargsF32`,
  `setSeqPortargsU32`, and `setSePortParameter`. The port-arg helpers still
  compute the same `unk4C[param_2].unk4[param_3]` address and write the
  incoming float/word; remaining drift is indexed store versus
  base-plus-offset codegen. `setSePortParameter` still preserves the null-track
  return, six flag tests, matching `TOuterParam::setParam` arguments, `xori`
  flag clears, optional interrupt trigger, and return behavior; only frame and
  save-slot offsets differ.
- The same proof batch passed this tick: `python configure.py --non-matching
  && ninja`, then `python configure.py && ninja` with
  `build/GMSJ01/mario.dol: OK`.
