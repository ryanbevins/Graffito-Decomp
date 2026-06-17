verdict: equivalent
date: 2026-06-14 4:36pm MNL
unit: mario/Strategic/spcinterp

Reason:
- `TSpcBinary::getHeader() const` is present and byte-matches after the
  implementation fix; there are no missing target symbols.
- `.rodata`, `.data`, and `.sdata2` all match exactly. Remaining text extras
  are source-only/dead API/helper owners and are not source-link blockers.
- Full diff review of the low/nonmatching rows found no behavior differences:
  `execstr()` performs the same `fetchU32` -> data-offset -> data-pointer
  string lookup and string-slice push, only with different helper boundaries
  and frame shape; `execjne()`, `spcTypeof()`, constructor, `dump()`,
  `dispatchBuiltinDefault()`, `TSpcBinary::init()`, and representative
  arithmetic/call/frame opcodes differ by stack/register/indexing/helper-label
  codegen only.
- `python configure.py --non-matching && ninja` linked cleanly with
  `Strategic/spcinterp.cpp` sourced.
- `python configure.py && ninja` restored the normal matching graph and passed
  `build/GMSJ01/mario.dol: OK`.

Byte debt:
- Many rows still carry stack-frame/spill-slot drift from the fetch helpers and
  inlined `TSpcStack`/`TSpcSlice` paths.
- `execstr()` has a large helper-boundary difference but the same binary data
  lookup and push semantics.
