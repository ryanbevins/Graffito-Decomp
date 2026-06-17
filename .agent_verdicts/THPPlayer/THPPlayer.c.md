# THPPlayer/THPPlayer.c

Verdict: equivalent
Date: 2026-06-13 9:24pm MNL

Unit: `mario/THPPlayer/THPPlayer`
Source: `src/THPPlayer/THPPlayer.c`
Classification: `Object(Equivalent, "THPPlayer/THPPlayer.c")`

## Review

- Most target-visible functions and all target-visible data objects are
  byte-identical in the current overview.
- `THPPlayerPrepare` preserves the same state/open checks, optional on-memory
  frame-header read, init-frame validation, audio flag setup, video/audio/read
  thread creation, buffer setup, thread starts, prepare-ready message receive,
  state/counter initialization, and VI callback install. Residue is GPR
  coloring and instruction scheduling.
- `THPPlayerCalcNeedMemory` preserves the same open guard, movie/buffer memory
  choice, aligned Y/U/V texture sizes, optional audio buffer allocation, and
  final `+0x1000`; residue is accumulator register choice.
- `THPPlayerInit` preserves global zeroing, LC enable, used-texture queue init,
  `THPInit` failure path, interrupt-guarded audio callback init, sound-buffer
  clear/flush, initialized flag, and return value. Residue is frame size,
  register allocation, and local label ownership.
- Source-only helpers such as `THPPlayerPostDrawDone`, queue helpers, and
  audio init/quit are inlined/owner byte debt for exact target-visible users,
  not source-link behavior blockers.

## Validation

- Shared proof from this tick: `python configure.py --non-matching && ninja`
  linked successfully with current `Equivalent` rows.
- Normal `python configure.py && ninja` passed with `build/GMSJ01/mario.dol:
  OK`.
