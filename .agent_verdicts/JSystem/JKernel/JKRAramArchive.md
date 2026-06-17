# mario/JSystem/JKernel/JKRAramArchive

Verdict: equivalent
Date: 2026-06-13 9:44am MNL

Reason:
- `python tools/decomp-diff.py -u mario/JSystem/JKernel/JKRAramArchive` reports
  no missing or extra symbols. Constructors, destructor, `open`, one
  `fetchResource` overload, the `unsigned char*` subroutine overload, and data
  symbols match exactly.
- `JKRAramArchive::fetchResource(JKRArchive::SDIFileEntry*, unsigned long*)`
  is 99.8% and exact-size. The full `--no-collapse` diff shows identical
  cached-resource handling, compression-mode selection, subroutine call, and
  result stores. Residue is frame size and the stack slot used for the
  subroutine output length.
- `JKRAramArchive::fetchResource_subroutine(unsigned long, unsigned long,
  JKRHeap*, int, unsigned char**)` is 99.9% and exact-size. The full diff shows
  identical mode dispatch, allocations, ARAM transfers, header read/free,
  decompressed-size calculation, panic path, and output pointer/length stores.
  Residue is frame size and one stack output slot.
- Re-verification of existing `Object(Equivalent, ...)` linked cleanly under
  `python configure.py --non-matching && ninja`.
- Recheck at 7:10am: the two nonmatching fetch paths still differ only by
  frame/output-slot placement and local helper labels. `python configure.py
  --non-matching && ninja` linked from source, then `python configure.py &&
  ninja` passed with `mario.dol: OK`.
- Recheck at 9:44am: fresh full diffs for both nonmatching fetch paths still
  preserve cached-resource handling, compression-mode selection, ARAM transfer
  setup, header read/free, decompressed-size calculation, panic path, output
  pointer/length stores, and return value. Residue remains frame/output-slot
  placement and local helper labels. Source-link and normal hash proof passed.
- Safety-net recheck at 8:06pm: current overview still has no missing/extra
  rows. Full diffs for both nonmatching fetch paths still show only frame size
  and output stack-slot drift: cached-resource handling, compression dispatch,
  subroutine calls, ARAM transfers, header read/free, decompressed-size
  calculation, panic path, output pointer/length stores, and return values are
  unchanged. This tick's `NpcEvent` `--non-matching` and normal proof builds
  also covered this existing `Equivalent` object from source.

Offending functions: none.
