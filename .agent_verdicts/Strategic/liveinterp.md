# Strategic/liveinterp.cpp

Verdict: equivalent
Time: 2026-06-12 9:59pm MNL

Build proof:
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja`

No missing symbols.

Reviewed nonmatching functions:
- `linPushNerve(TSpcTypedInterp<TLiveActor>*, unsigned long)`
  - Equivalent. Pops the nerve index, resolves it through `NerveGetByIndex`, pushes no result on null, otherwise pushes the resolved nerve onto the actor spine and then pushes the empty interpreter result.
  - Residue is rodata/string-label ownership only.
- `linSetSRT(TSpcTypedInterp<TLiveActor>*, unsigned long)`
  - Equivalent. Pops value/axis/category, wraps rotations to `[0, 360)`, writes position/rotation/scale component by axis, and pushes the empty result.
  - Residue is rodata/string-label ownership only.
- `linGetSRT(TSpcTypedInterp<TLiveActor>*, unsigned long)`
  - Equivalent. Pops axis/category, pushes the requested position/rotation/scale component as a float slice, and pushes an empty slice for out-of-range categories or axes.
  - Residue is stack-frame/local-slice layout and branch-layout swap for position versus rotation cases; each argument value reaches the same actor field offsets.
- `linSetAnmRate(TSpcTypedInterp<TLiveActor>*, unsigned long)`
  - Equivalent. Pops rate/category, maps category 0 to BCK slot 0 and category 1 to slot 3, sets frame rate, and pushes the empty result.
  - Residue is rodata/string-label ownership only.

Data residue:
- The large `.data` mismatch is the `NerveGetByIndex` switch/jump table. The function text and switch cases are behavior-matching; source-link relocates the table to the source labels.
- The aggregate `.rodata` mismatch is ownership/layout drift from infectious strings and string labels; named rodata objects are byte-matching and there are no missing data symbols.

Extra symbols/data are source-emitted unused helpers and infectious-string carriers (`linSetSubBck`, `MtxCalcTypeName`, `dummyMactorStringValue1`, `SMS_NO_MEMORY_MESSAGE`).

2026-06-13 9:24am MNL recheck: verdict remains `equivalent`. Full diffs for
`linPushNerve`, `linSetSRT`, `linGetSRT`, and `linSetAnmRate` were re-read.
`linGetSRT` still has the only meaningful-looking drift, but it is case-body
layout: case 0 returns position fields at `0x10/0x14/0x18`, case 1 returns
rotation fields at `0x30/0x34/0x38`, and case 2 returns scale fields at
`0x24/0x28/0x2c` on both sides. Other differences are stack frame, slice temp
placement, string/local data ownership, and jump-table/data labels. Shared proof
passed: `python configure.py --non-matching && ninja`, then
`python configure.py && ninja` verified `mario.dol: OK`.

2026-06-13 10:18pm MNL recheck: verdict remains `equivalent`. Current overview
still has no missing symbols. Re-read `linPushNerve`, `linSetSRT`,
`linGetSRT`, and `linSetAnmRate`; the only substantive-looking `linGetSRT`
diff is still physical case-body layout for position/rotation, with the same
input categories reaching the same field offsets and empty-slice defaults.
The `--non-matching` proof run for this tick's `MarNameRefGen` certification
also source-linked this existing `Equivalent`, followed by a normal
`python configure.py && ninja` `mario.dol: OK`.
