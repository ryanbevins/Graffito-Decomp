# Bianco episode 1 freeze capture

## Capture a freeze

Leave the frozen game running in Dolphin, then double-click
`tools/capture-freeze.cmd`, or run from the repository root:

```powershell
python tools/capture-freeze.py
```

No rebuild, emulator restart, plugin, or extra Python package is needed.
The tool only reads process memory. It saves three MEM1 snapshots, the installed
symbol map, Dolphin's log, a readable `summary.txt`, and `report.json` under
`build/freezes/<timestamp>/`. The JSON includes saved CPU registers, thread wait
queues, stack frames, application/director state, and saved FIFO fields.
Keep the whole capture folder for analysis.

The default installation is `%USERPROFILE%\Downloads\SunshineJPExtract\sys`.
For another installation use `--dol PATH --map PATH`; use `--pid NUMBER` when
multiple Dolphin processes are open. The supplied map must belong to that DOL.
The report compares both executable sections against live memory and records
DOL/map hashes. Any mismatch needs investigation before trusting symbol names.

These are non-atomic live snapshots, not emulator savestates. Saved context PCs
are scheduler snapshots, not a continuously sampled instruction pointer. Compare
several samples; a single sleeping thread is normal. FIFO pointers in the SDK
object are saved values, not live hardware register readings. Raw MEM1 is retained
so further object/command inspection does not require reproducing the freeze.

## Verified September 12 capture

Installed DOL SHA1: `c197471b53e5be1e001d92f30bf786e86c7a2351`.
Evidence: `build/freezes/bianco-confirmed/` (local, not committed).
Both executable sections match the running code byte for byte.

Three consecutive snapshots show application state 5, area 2, episode 0
(Bianco's first episode), with director `setup_objects_complete = 0`.
The main thread `803e9008` waits on `FinishQueue` at `804074cc`:

```text
TApplication::gameLoop
  TMarDirector::direct
    TMarDirector::setupObjects
      TMarDirector::setup2 + 0x508
        GXWaitDrawDone + 0x24
          OSSleepThread
```

`DrawDone` at `804074c8` remains zero. In `MarDirectorSetup2.cpp`, this is the
wait immediately following the initial `unk40` and `unk38` rendering passes
and `GXSetDrawDone()`. The completion signal has not released the main thread,
so it cannot finish setup and advance to gameplay.

## Underlying failure and isolated validation

The pollution counter virtual methods were reversed in the source vtables.
Retail slot `0x0c` is `getCounterNo` (token minus the range base), and slot
`0x10` is `getTokenNo` (counter index plus the range base). Source also called
the wrong conversion in both `setCallback` and `drawSyncCallback`.

The captured GPU stream contains object token `0xff6f`, outside the registered
object range `0x92..0xa5`. The unmatched token is discarded by
`TDrawSyncManager::drawSyncCallbackSub`, so its breakpoint queue cannot advance.
The FIFO saved read pointer and `__GXCurrentBP` both equal `0x804c52c0`, with
saved CP status `0x1a` indicating the breakpoint. The main thread waits for
completion behind this stopped command stream. This is a synchronization
deadlock, not a resource-loading wait.

Fixed the method declaration order and conversion call sites in
`include/Map/PollutionCount.hpp` and `src/Map/PollutionCount.cpp`. Both derived
vtables, all four conversion functions, and `drawSyncCallback` compare as
100% matches after compiling with MWCC.

Also found and corrected a separate texture destination bug in
`MarDirectorInitECT.cpp`: `(u8*)&img` addressed the local pointer on the stack;
retail uses `(u8*)img`, the texture resource itself. Five EFB copy targets in
the original capture pointed at `0x80423204` or `0x804232c4`. Testing this
one-instruction correction alone still reproduced the GPU deadlock.

Installed test DOLs were patched in place without moving code or changing the
map, to isolate these corrections from other source/build differences:

- Original: `c197471b53e5be1e001d92f30bf786e86c7a2351`.
- Texture pointer only: `42e0168692f772f054cd795e7427243b5bf530c5` (still freezes).
- Pointer plus the two corrected vtables:
  `2c9a3f9a0ed1cf3c64235c6eb05164e18942b7e2` (Bianco setup completes).

`build/freezes/bianco-token-validation/` records area 2, episode 0, director
state 1, setup complete, `DrawDone=1`, and no active GPU breakpoint. The running
executable matches the patched DOL. Original backup and patch manifests are in
`build/freezes/bianco-pointer-fix/` and `build/freezes/bianco-token-fix/`.
This is a targeted runtime patch plus source-object compile validation, not a
fresh full-source linked build.

## Subsequent windmill audio warning

After removing the GPU deadlock, the user reported an invalid read from
`0x0ff00008` at `0x8004f4d8` in `JAISeEntry::storeBuffer`. The paused warning
capture is in `build/freezes/audio-warning-paused/`. Its actual main-stack
backchain (starting at `0x804235e8`, rather than the stale saved OS context)
leads through the sound wrappers to `TBigWindmill::control`, sound ID `0x3047`.

`TBigWindmill` passed its uninitialized `unk148` sound handle to the audio
system. Retail's factory constructor explicitly clears that member at
`801026a0: stw r0,0x148(r30)`, while the source constructor was empty after the
base initialization. Added `unk148(nullptr)` in `MapObjBianco.hpp` and compiled
the factory translation unit successfully.

The isolated installed correction uses a short trampoline in verified unused
executable alignment padding (`0x80081be4`) to preserve the displaced factory
store, initialize `unk148`, and resume the existing constructor. This changes
no function addresses. Test DOL SHA1:
`10309fb59f1602338170b90814bbee460921710a`.
Patch details and the DOL are in `build/freezes/bianco-windmill-fix/`.
The permanent source correction requires no trampoline.

The user confirmed **Resolved** after restarting with this final DOL. Final
runtime capture: `build/freezes/bianco-resolved/`. No ignored-warning workaround
is required. The original white-screen deadlock and subsequent windmill sound
warning have both been addressed; unrelated gameplay has not been exhaustively
tested.

## Validation

Full non-matching builds and explicit objdiff reports were subsequently generated
on the VPS before and after the four source/header corrections, using the same
baseline commit `ff9e4dbf10f0b4162d7e1ccca1059f81ddcf25f8`.

| Measure | Before | After | Gain |
| --- | ---: | ---: | ---: |
| Overall fuzzy matching | 96.232070% | 96.233120% | +0.001050 percentage points |
| Exact matched code | 1,526,352 bytes | 1,526,352 bytes | 0 |
| Exact matched functions | 9,736 | 9,736 | 0 |
| Exact matched data | 461,407 bytes | 461,471 bytes | +64 bytes |

Both full builds succeeded, with no exact-code or function regression. Reports
and build logs are retained under
`/opt/graffito/state/bianco-runtime-20260912/`. Runtime confirmation above applies
to the isolated patched DOL; the newly linked full-source DOL has build validation
but has not separately undergone the user's gameplay test.

The tool captured the user's running frozen game and resolved its installed map.
Parser tests cover big-endian thread fields, corrupt thread pointers, cyclic
stack/thread links, and symbol range boundaries:

```powershell
python -m unittest discover -s tools/tests -p test_capture_freeze.py
```
