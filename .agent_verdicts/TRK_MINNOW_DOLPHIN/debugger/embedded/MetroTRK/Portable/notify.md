# mario/TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/notify

Verdict: equivalent  
Status: equivalent  
Date: 2026-06-13 8:01pm MNL  
Source: `src/TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/notify.c`

## Reason

Re-verified existing `Object(Equivalent, ...)`.

- `TRKDoNotifyStopped` performs the same `TRKGetFreeBuffer` call, message
  append with the same `0x880` overflow guard, stop-vs-exception info branch
  (`cmd == 0x90`), `TRKRequestSend` arguments, conditional request-buffer
  release, free-buffer release, and error return.
- The only objdiff residue is the stack slot used for the free buffer index
  (`r1+8` in target, `r1+0x14` in source). That is frame/local layout debt, not
  a behavioral difference.

## Proof

- `python tools/decomp-diff.py -u
  mario/TRK_MINNOW_DOLPHIN/debugger/embedded/MetroTRK/Portable/notify -d
  TRKDoNotifyStopped --no-collapse` shows the full function instruction stream
  equivalent except stack-slot operands.
- The current tick's `python configure.py --non-matching && ninja` linked all
  `Equivalent` objects from source.
- The current tick's `python configure.py && ninja` restored normal config and
  passed `build/GMSJ01/mario.dol: OK`.
