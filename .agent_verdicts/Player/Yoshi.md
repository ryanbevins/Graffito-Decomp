# Player/Yoshi Audit

Verdict: needs_impl  
Date: 2026-06-13 4:00am MNL

Not promoted.

Reason:

- `python tools/decomp-diff.py -u mario/Player/Yoshi` shows missing target
  data symbols, including `.ctors` `@4907` and `@3802`.
- The rebuilt object also emits target-absent helper/list symbols such as
  `TYoshi::onYoshi()`, JSUList destructors, `MtxCalcTypeName`,
  `dummyMactorStringValue1`, `SMS_NO_MEMORY_MESSAGE`, and extra `.sdata`.
- Because target symbols are missing, the TU cannot be certified
  `Equivalent` in this audit sweep even before reviewing behavior-level diffs.
