# JSystem/JUtility/JUTException Audit

Verdict: equivalent
Date: 2026-06-14 7:37pm MNL

Safety-net recheck. Current overview is unchanged: all target text/data are
exact except `printContext`, `createFB`, `appendMapFile`, and
`queryMapAddress_single`, whose existing review still classifies the diffs as
stack/register/helper-owner residue with matching behavior. The missing
`@1210/@1411/@1431` ctor rows are paired with same-sized source dummy ctor
entries and are data-owner byte debt. This tick's successful
`python configure.py --non-matching && ninja` and normal
`python configure.py && ninja` builds covered the current source-linked object.

## 2026-06-13 1:07pm MNL - equivalent, reverified

Verdict: `equivalent`.

Rechecked the current overview and all four nonmatching functions:
`printContext`, `createFB`, `appendMapFile`, and `queryMapAddress_single`.
The same guards, prints, framebuffer setup, map-file list handling, map parsing,
and close/destructor paths are present. Remaining drift is stack/register
allocation, helper-owner labels, and paired dummy ctor/data ownership. Source-link
proof and normal DOL proof both passed at 1:07pm MNL.

## 2026-06-13 8:39am MNL - equivalent, reverified

Verdict: `equivalent`.

Reverified current `Object(Equivalent, ...)` row during the audit sweep.

Reason:
- `python tools/decomp-diff.py -u mario/JSystem/JUtility/JUTException` still
  reports no missing target text/data that affects behavior. The three missing
  `.ctors` rows remain paired with same-sized source dummy ctor entries.
- Rechecked all four nonmatching functions:
  `JUTException::printContext(...)`, `createFB()`, `appendMapFile(char*)`, and
  `queryMapAddress_single(...)`.
- The visible operations still line up: console guards and print loops,
  framebuffer setup, map-file list append, map-file parsing/printing, and
  close/destructor paths. Apparent local-call label mismatches inside
  `printContext` are still helper-owner/weak-layout drift, not changed calls.
- Remaining residue is stack-frame/slot size, saved-register coloring,
  local helper label ownership, and ctor/data label drift.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and verified
  `build/GMSJ01/mario.dol: OK`.

## 2026-06-12 9:47pm MNL - equivalent

Verdict: `equivalent`.

Promoted `JSystem/JUtility/JUTException.cpp` from `NonMatching` to
`Equivalent`.

Reason:
- No missing target text symbols. The three missing `.ctors` rows are paired
  with same-sized source dummy ctor entries and were source-link safe.
- Reviewed all four nonmatching text functions:
  `JUTException::printContext(unsigned short, OSContext*, unsigned long, unsigned long)`,
  `JUTException::createFB()`,
  `JUTException::appendMapFile(char*)`, and
  `JUTException::queryMapAddress_single(char*, unsigned long, long, unsigned long*, unsigned long*, char*, unsigned long, bool, bool)`.
- `printContext` preserves the direct-print/console guards, main-info/debug
  page loop, post callback, pad-scrolling loop, and automatic scroll loop. Raw
  asm confirmed apparent `decomp-diff.py` call-label mismatches are
  symbol-owner drift around nearby helpers; target calls `waitTime`, `readPad`,
  `showGPR`, and `showGPRMap` at the expected points.
- `createFB` preserves render-mode selection, arena allocation/alignment,
  `JUTExternalFB` placement construction, direct-print framebuffer change, VI
  setup, and `mFrameMemory` store.
- `appendMapFile` preserves null guard, duplicate-path scan, allocation,
  `JSUPtrLink` construction, and list append.
- `queryMapAddress_single` preserves map-file open, section scanning,
  address/size parsing, output line cleanup, optional printing, newline logic,
  close/destructor, and bool return.
- Remaining residue is codegen-class only: stack frame/slot size, saved-register
  coloring, helper label ownership, and ctor/data label drift.

Proof:
- `python configure.py --non-matching && ninja` linked with `JUTException` from
  source.
- `python configure.py && ninja` passed and verified `mario.dol: OK`.
