# mario/M3DUtil/MActorUtil

Verdict: equivalent
Date: 2026-06-13 8:27am MNL

Reason:
- `python tools/decomp-diff.py -u mario/M3DUtil/MActorUtil` reports no missing
  or extra symbols. Three functions match exactly.
- `SMS_MakeMActor(const char*, const char*, unsigned long, unsigned long)` is
  99.8% and exact-size. The full `--no-collapse` diff shows identical
  allocation, `MActorAnmData` construction/init, and model-data factory call;
  only stack frame / save-slot offsets differ.
- `SMS_MakeMActorWithAnmData(const char*, MActorAnmData*, unsigned long,
  unsigned long)` is 99.7% and exact-size. The full diff shows identical
  resource load, model-data construction, actor factory call, and return; only
  stack frame / save-slot offsets differ.
- Re-verification of existing `Object(Equivalent, ...)` linked cleanly under
  `python configure.py --non-matching && ninja`.
- `python configure.py && ninja` passed afterward and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 12:36pm MNL recheck: verdict remains `equivalent`.
Fresh full diffs for `SMS_MakeMActor` and `SMS_MakeMActorWithAnmData`
still show identical allocation, constructor/init/load/factory calls, stored
actor pointer, and return behavior. The only differences are stack-frame size
and saved-register slot offsets. Proof refreshed with `python configure.py
--non-matching && ninja`, then normal `python configure.py && ninja` with
`build/GMSJ01/mario.dol: OK`.
