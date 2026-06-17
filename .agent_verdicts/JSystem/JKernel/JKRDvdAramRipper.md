# mario/JSystem/JKernel/JKRDvdAramRipper

Verdict: equivalent
Status: source_link_proven
Time: 2026-06-14 8:00am MNL

## Proof

- Implemented the missing 148B overload
  `JKRDvdAramRipper::loadToAram(char*, unsigned long, JKRExpandSwitch,
  unsigned long, unsigned long)`.
- The overload matches the target and mirrors the existing `s32` wrapper:
  construct a stack `JKRDvdFile`, open by path, return `nullptr` on open
  failure, otherwise call the `JKRDvdFile*` overload, then destroy the local
  file before returning.
- `python configure.py && ninja` passed; the overload is a 100.0% match and
  the TU has no missing target symbols.
- Remaining nonmatching rows were reviewed as codegen/data-label residue:
  `callCommand_Async()` frame-size/stack-buffer alignment, `syncAram()` frame
  size, `JKRDecompressFromDVDToAram()` saved-register/data-label ownership,
  `decompSZS_subroutine()` equivalent register and static-label residue, and
  `firstSrcData()` frame-size/static-label residue.
- Promoted `JSystem/JKernel/JKRDvdAramRipper.cpp` to
  `Object(Equivalent, ...)`.
- `python configure.py --non-matching && ninja` linked successfully with this
  TU source-linked.
- Follow-up normal `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.

## Remaining Byte Debt

- Source still emits extra helper owners:
  `JSUFileInputStream::~JSUFileInputStream()`,
  `JKRDvdFile::getFileSize() const`, and
  `JSULink<JKRADCommand>::~JSULink()`. They are harmless for source-linking;
  the proof build did not produce duplicate/undefined link failures.
