# mario/JSystem/JAudio/JASystem/JASHardStream

## Verdict: equivalent

Date: 2026-06-14 12:04am MNL

Reason:
- Removed target-absent public stub/API bodies from `JASHardStream.cpp` and
  made the three helpers that target inlines (`unregistBgmAll`,
  `TControl::fileOpen`, `TControl::setLastAddr`) explicit `inline` definitions.
  Current overview has no missing or extra text symbols.
- All remaining nonmatching text functions are behavior-identical:
  `main()`, `firstBgmCallback`, `getAddrCallback`, `startFirst`,
  `startSecond`, and `volFloatToU8` have the same state transitions, DVD/AI
  calls, list clearing, file-open path construction, last-address updates, and
  float clamping/conversion. Diffs are stack-frame size, local static label
  names, and equivalent inlined helper layout.
- `.sdata2` drift is constant order/ownership only. Target and rebuild contain
  the same runtime constants (`0.0f`, `1.0f`, conversion double, `255.0f`) with
  different symbol order after suppressing target-absent helpers.

Proof:
- `python configure.py --non-matching && ninja` linked successfully with
  `JSystem/JAudio/JASystem/JASHardStream.o` from source.
- `python configure.py && ninja` passed afterward with
  `build/GMSJ01/mario.dol: OK`.
