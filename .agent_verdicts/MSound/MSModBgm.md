# MSound/MSModBgm audit

Verdict: equivalent
Date: 2026-06-13 7:55am MNL

Reason: all functions are byte-matching or behaviorally aligned, and
`python configure.py --non-matching && ninja` linked the TU from source.

Fresh recheck: full pass repeated on 2026-06-13 7:55am MNL. No missing target
text symbols were present; raw asm confirms the `scTiming` / `scExp` adjacency
used by the x-fade code despite objdiff's base-symbol label drift.

Function review:
- `MSBgmXFade::xFadeBgmForce(float)`: scans the same timing interval, maps
  out-of-range to `0xff`, applies the same forced track-volume pair, and stores
  `mLastTiming` only on valid ranges. Remaining drift is branch layout,
  base-symbol labeling, stack size, and register coloring.
- `MSBgmXFade::xFadeBgm(float)`: scans threshold crossings against the previous
  timing, gates valid indices 1 through 0x10, applies the same two-frame
  cross-fade volumes, and always updates `mLastTiming`. Remaining drift is
  branch layout and stack/register residue.
- `MSModBgm::changeTempo(unsigned char, unsigned char)`: handle guard, tempo
  constants, frame counts, and final `setTempoProportion` call match
  behaviorally. Remaining drift is switch compare-tree shape.
- `MSModBgm::modBgm(unsigned char, unsigned char)`: state setup, handle guard,
  counter cases 0/5/0xb4, tempo/pitch calls, stop call, state clear, and return
  value match. Remaining drift is register coloring and late `li r31,0`
  scheduling.
- `MSModBgm::loop()` and `__sinit_MSModBgm_cpp` byte-match.

Notes:
- Objdiff reports local dummy constants `@1431/@1411/@1210` as missing and
  `dummy1431/dummy1411/dummy1210` as extras. The bytes/data behavior are the
  intended infectious dummy shape, and the required source-link proof passed.

Reverified: 2026-06-13 10:52am MNL — still equivalent. Re-read all four
nonmatching helpers. The x-fade loops still choose the same timing buckets and
out-of-range `0xff` path; `changeTempo` and `modBgm` still make the same handle
guards, tempo/pitch/stop calls, constants, stores, and return values. Remaining
drift is branch layout, stack/register coloring, local constant labels, and
dummy-symbol naming. Proof passed again with `python configure.py
--non-matching && ninja`, then plain `python configure.py && ninja` restored the
matching config and verified `build/GMSJ01/mario.dol: OK`.
