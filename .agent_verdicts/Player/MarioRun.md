# Player/MarioRun Audit

Verdict: needs_impl
Date: 2026-06-13 7:43pm MNL

Rechecked after the `TYoshi::onYoshi()` owner split. The old duplicate
`TYoshi::onYoshi()` extra is gone, and there are no missing target `.text`
functions, but the TU is still not source-symbol/data complete:

- Missing `.ctors`/data rows remain for `@2111`, `@2532`-`@2535`,
  `cDirtyFileName`, `cDirtyTexName`, `@1431`, `@1411`, `@1210`,
  `MtxCalcTypeName`, `param$3393` through `param$3410`, and later local
  constants.
- Current overview still shows broad `.rodata`, `.data`, and `.sdata2` drift
  plus extra JSUList destructor/static rows.

Keep this `NonMatching` until the static data/owner layout is reconstructed or
an implementation pass proves those rows are harmless source-link byte debt.

Verdict: needs_impl  
Date: 2026-06-13 4:00am MNL

Not promoted.

Reason:

- `python tools/decomp-diff.py -u mario/Player/MarioRun` shows many missing
  target local data symbols, including `.ctors` `@2111`, `@2532`-`@2535`,
  `cDirtyFileName`, `cDirtyTexName`, `@1431`, `@1411`, `@1210`,
  `MtxCalcTypeName`, `param$3393` through `param$3410`, and several later
  constant rows.
- The rebuilt object emits target-absent `TYoshi::onYoshi()`, JSUList
  destructors, and `sRecords$1658`.
- The TU is not source-symbol complete, so it remains `NonMatching`.
