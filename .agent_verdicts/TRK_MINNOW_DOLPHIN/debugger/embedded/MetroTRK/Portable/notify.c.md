# TRK Portable notify.c

Verdict: equivalent
Date: 2026-06-13 9:18pm MNL

Unit: `mario/TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/notify`
Source: `src/TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/notify.c`
Classification: `Object(Equivalent, ".../notify.c")`

## Review

- The only target function, `TRKDoNotifyStopped`, is 100.0% fuzzy. Full
  no-collapse diff shows identical behavior: get a free buffer, append the
  command byte with the same `0x880` overflow guard, add stop or exception info
  based on command `0x90`, send the request with `(2, 3, 1)`, release the
  request buffer on success, release the original buffer, and return the final
  `DSError`.
- Residue is only stack-slot selection for the original free-buffer index
  local (`r1+8` vs `r1+0x14`).

## Validation

- Shared proof from this tick: `python configure.py --non-matching && ninja`
  linked successfully with current `Equivalent` rows.
- Normal `python configure.py && ninja` passed with `build/GMSJ01/mario.dol:
  OK`.
