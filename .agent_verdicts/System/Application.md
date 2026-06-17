# mario/System/Application

Verdict: equivalent
Status: equivalent
Time: 2026-06-15 8:44am MNL

## Verdict

Certified `System/Application.cpp` as functionally equivalent and promoted it to
`Object(Equivalent, "System/Application.cpp")`.

## Audit fixes

- `initialize_nlogoAfter()` destroys the game heap, then calls virtual
  `JKRHeap::free(spGameHeapBlock)` through `sRootHeap`. The previous
  `getSize(spGameHeapBlock)` source was a real behavior bug; vtable slot `0x10`
  is `free(void*)`, while `getSize` is slot `0x20`.
- `proc()` must guard `mDirector` and call the virtual destructor with the
  non-deleting flag (`r4 = -1`), then null the pointer. `delete mDirector`
  passed the deleting flag (`r4 = 1`) and would free memory differently.
- `TMenuDirector` allocation is `0x58`; the class needs a 4-byte tail pad after
  `unk50`.
- `MSound` allocation is `0xd4`; the old `0x30c` tail fields were not used by
  source and made Application allocate too much memory.

## Remaining byte debt

The remaining non-exact rows are codegen/data-layout class, not functional:

- `TApplication::TApplication()` uses target helper calls for zero
  `TGameSequence`/`JDrama::TFlagT<Us>` temporaries; current source performs
  equivalent direct zero stores.
- `gameLoop()` has helper-boundary drift from the owner-only
  `TTimeRec::crTimeAry()` body; source and target select the same timer slot.
- `SetupThreadFuncLogo` differs by inline/member helper boundary, while
  `TApplication::setupThreadFuncLogo()` itself follows the same operation
  sequence.
- `mountStageArchive`, `drawDVDErr`, `SMSSwitch2DArchive`, and
  `SMSMountAramArchive` differ by stack/register placement and local data-label
  ordering, with matching calls, branches, constants, loads, and stores.

No missing target-owned symbols remain in this object.

## Proof

- `python tools/decomp-diff.py -u mario/System/Application -s missing` reports
  no missing symbols.
- `python configure.py --non-matching && ninja` linked successfully with
  `System/Application.cpp` sourced.
- Restored normal configuration with `python configure.py && ninja`; final gate
  passed `build/GMSJ01/mario.dol: OK`.
