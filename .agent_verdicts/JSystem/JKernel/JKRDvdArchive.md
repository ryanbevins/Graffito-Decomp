# mario/JSystem/JKernel/JKRDvdArchive

Verdict: equivalent
Date: 2026-06-13 9:44am MNL

Reason:
- Re-verified during the AUDIT sweep. All functions except
  `JKRDvdArchive::fetchResource(JKRArchive::SDIFileEntry*, unsigned long*)`
  match exactly, and rodata / vtable rows match.
- Full `--no-collapse` diff for the nonmatching overload shows identical
  behavior: file-entry cache check, archive attribute compression selection,
  `fetchResource_subroutine` call with the same file offset / size / heap /
  direction / compression arguments, optional return-size store, `mData` cache
  write, cached-size fallback, and final return. Remaining differences are
  frame size, save-slot offsets, scratch pointer slot, and local label names.
- Source-owned weak `JKRFile::~JKRFile()` and `JKRFile::__vtable` are
  target-present weak symbols owned by another object; this is ownership drift,
  not runtime behavior.
- Recheck at 7:10am: current full diff still shows only frame/scratch-slot and
  helper-label drift in `fetchResource(SDIFileEntry*, u32*)`; all other text and
  rodata/vtable rows are exact.
- Proof: `python configure.py --non-matching && ninja` linked from source,
  then normal `python configure.py && ninja` passed and verified
  `mario.dol: OK`.
- Recheck at 9:44am: current overview is unchanged. The fresh full diff for
  `fetchResource(SDIFileEntry*, u32*)` still has identical cache check,
  compression selection, DVD subroutine call arguments, optional size store,
  cached-data write, cached-size fallback, and return. Source-link and normal
  hash proof passed again.
- Recheck at 10:07pm: current overview is unchanged. Full no-collapse diff for
  `fetchResource(SDIFileEntry*, u32*)` still differs only in frame size,
  save-slot offsets, scratch pointer slot, and local helper label; operation
  order and all call/store semantics are identical. The source-link build
  (`python configure.py --non-matching && ninja`) passed, then normal
  `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
