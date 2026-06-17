# mario/TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/support

Verdict: equivalent  
Status: equivalent  
Date: 2026-06-13 8:01pm MNL  
Source: `src/TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/support.c`

## Reason

Re-verified existing `Object(Equivalent, ...)`.

- `TRKRequestSend` is byte-identical.
- `TRKSuppAccessFile` preserves the same parameter validation, chunking at
  `0x800`, read/write message command selection, request buffer construction,
  optional reply path, reply length/error reconciliation, buffer releases,
  `done += length`, final `*count = done`, and error return.
- Remaining objdiff residue is a clean callee-saved GPR coloring swap (`r28` /
  `r29`) for the byte-count progress value and reply buffer pointer. No branch,
  call, constant, load/store offset, or loop-count behavior differs.

## Proof

- `python tools/decomp-diff.py -u
  mario/TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/support -d
  TRKSuppAccessFile --no-collapse` shows only `r28` / `r29` operand swaps.
- `python tools/decomp-diff.py -u
  mario/TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/support -d
  TRKRequestSend --no-collapse` is 100%.
- The current tick's `python configure.py --non-matching && ninja` linked all
  `Equivalent` objects from source.
- The current tick's `python configure.py && ninja` restored normal config and
  passed `build/GMSJ01/mario.dol: OK`.
