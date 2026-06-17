# THPPlayer/THPPlayer audit

Verdict: equivalent  
Status: certified  
Time: 2026-06-13 4:50pm MNL

Unit: `mario/THPPlayer/THPPlayer`  
Source: `src/THPPlayer/THPPlayer.c`  
Classification: `Object(Equivalent, "THPPlayer/THPPlayer.c")`

## Verdict

Promoted to `Equivalent` after restoring the missing JAS mix callback
registration paths:

- `THPPlayerInit()` now calls
  `JASystem::Kernel::registerMixCallback(audioCallbackWithMSound, 3)` after
  disabling interrupts, matching the target behavior.
- `THPPlayerQuit()` and the audio-exists path of `THPPlayerStop()` now
  unregister with `JASystem::Kernel::registerMixCallback(nullptr, 0)`.

Remaining nonmatching text is codegen-class:

- `THPPlayerPrepare()` has register allocation/scheduling drift around the same
  offset read, thread setup, message receive, and callback install sequence.
- `THPPlayerCalcNeedMemory()` differs only in accumulator register choice.
- `THPPlayerInit()` differs only in stack frame/slot layout and local-label
  attribution for the same callback address.

The rebuilt object still emits unused standalone helper/public functions that
are not target symbols (`ProperTimingForStart`, `WaitUntilPrepare`,
`PopUsedTextureSet`, `PushUsedTextureSet`, `THPPlayerGetTotalFrame`,
`THPPlayerPostDrawDone`, `initAudio`, `quitAudio`) plus local THP JPEG rodata
from headers. They are unreferenced byte debt, not runtime behavior blockers.

Build proof:

- `python configure.py --non-matching && ninja` passed with `THPPlayer` linked
  from source.
- `python configure.py && ninja` passed and verified `build/GMSJ01/mario.dol:
  OK`.
