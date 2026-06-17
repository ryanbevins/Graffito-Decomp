# TRK Portable support.c

Verdict: equivalent
Date: 2026-06-13 9:20pm MNL

Unit: `mario/TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/support`
Source: `src/TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/support.c`
Classification: `Object(Equivalent, ".../support.c")`

## Review

- `TRKRequestSend` is byte-identical in the current overview and full diff.
- `TRKSuppAccessFile` preserves behavior: parameter validation, `DS_IONoError`
  initialization, 0x800-byte chunking, read/write request construction,
  optional data append for writes, request/reply path, reply length correction,
  IO-result propagation, one-way message path, buffer releases, `done +=
  length`, and final `*count = done`.
- Residue is GPR coloring only (`done` in `r28` vs `r29`, reply buffer pointer
  in `r29` vs `r28`) with the same memory accesses and branch conditions.

## Validation

- Shared proof from this tick: `python configure.py --non-matching && ninja`
  linked successfully with current `Equivalent` rows.
- Normal `python configure.py && ninja` passed with `build/GMSJ01/mario.dol:
  OK`.
