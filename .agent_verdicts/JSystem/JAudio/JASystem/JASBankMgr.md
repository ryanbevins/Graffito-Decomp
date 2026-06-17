## Verdict: equivalent

Date: 2026-06-13 5:50am MNL
Unit: `mario/JSystem/JAudio/JASystem/JASBankMgr`
Source: `src/JSystem/JAudio/JASystem/JASBankMgr.cpp`
Classification: `Object(Equivalent, "JSystem/JAudio/JASystem/JASBankMgr.cpp")`

Reason:
- Re-verified during the audit sweep. The overview has no missing target
  symbols. Static data and most functions match byte-for-byte.
- `setVir2PhyTable` preserves the same `0xffff` guard and halfword write into
  `sVir2PhyTable`; the diff is only direct halfword store versus indexed
  `sthx` selection.
- `noteOn` preserves the same gate-on shortcut for notes above `0xef`, bank and
  instrument lookup null exits, `TInstParam` initialization/use, oscillator
  fallback, channel key construction, logical-channel request, wave/channel
  field setup, pitch-table lookup, volume/effect writes, clamp-to-0..1
  behavior, oscillator initialization loop, release, `play`, and return/null
  behavior. Remaining diffs are stack/register allocation, constant-label
  ownership, and equivalent clamp/load scheduling.
- Extra helper bodies (`registBank`, `getBank`, `clamp01`, `getUsedHeapSize`)
  are unused source-owned helper drift and do not block source linking.

Proof:
- `python configure.py --non-matching && ninja` linked
  `build/GMSJ01/mario.dol` from source.
- `python configure.py && ninja` restored the matching config and passed
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 5:47pm MNL recheck:
- Current overview still has no missing target symbols. Re-read focused diffs
  for `setVir2PhyTable` and `noteOn`.
- `setVir2PhyTable` is still behavior-identical: same `0xffff` sentinel guard
  and same halfword write into `sVir2PhyTable`; residue is direct indexed-store
  selection.
- `noteOn` still preserves the high-note shortcut, bank/instrument/wave null
  exits, `TInstParam` setup, logical-channel request, wave/channel field setup,
  pitch/volume/effect math, clamp behavior, oscillator loop, release, `play`,
  and return/null behavior. Remaining residue is stack/register/FPR coloring,
  local-label ownership, and equivalent scheduling.
- Existing proof remains covered by this tick's successful
  `python configure.py --non-matching && ninja` and normal
  `python configure.py && ninja`.

2026-06-13 9:55am MNL recheck:
- Current overview still has no missing target symbols.
- Re-read full diffs for `setVir2PhyTable` and `noteOn`. `setVir2PhyTable`
  still has identical guard and halfword table write. `noteOn` still preserves
  the high-note `noteOnOsc` path (raw asm confirms the helper label), bank /
  instrument / wave null exits, channel-key construction, logical-channel
  request, channel field setup, pitch/volume/effect math, clamping, oscillator
  loop, release, `play`, and return behavior.
- Remaining residue is stack/register/FPR coloring, constant-label ownership,
  and equivalent address-expression scheduling. Proof batch passed:
  `python configure.py --non-matching && ninja`, then `python configure.py &&
  ninja` with `mario.dol: OK`.
