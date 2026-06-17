# JSystem/JDrama/JDREfbSetting Audit

Verdict: equivalent
Date: 2026-06-13 4:13pm MNL

Re-verified the existing `Equivalent` certification during the audit fallback
sweep.

- `python tools/decomp-diff.py -u mario/JSystem/JDrama/JDREfbSetting` still
  shows no missing or extra target rows.
- Re-read all sub-100% function diffs. The two pixel-format functions preserve
  the same `DecidePixelFmt`, `GXSetPixelFmt`, `GXSetDither`, and
  `GXSetFieldMode` calls; raw `objdump -drC` confirms the render-mode overload
  calls `DecidePixelFmt` on both sides despite a misleading pretty-diff label.
- Copy-clear/copy-disp paths still perform the same flag predicates, clear
  color copy, color/alpha/z update calls, optional `GXSetZCompLoc`, clamp,
  copy filter, source/destination setup, and `GXCopyDisp` clear argument.
- Remaining drift is stack size/slots, register coloring, and equivalent
  boolean materialization.
- Source-link proof is covered by this tick's successful
  `python configure.py --non-matching && ninja`; normal config was restored
  with `python configure.py && ninja` and verified `build/GMSJ01/mario.dol: OK`.

Verdict: equivalent  
Date: 2026-06-13 8:59am MNL

Kept as `Object(Equivalent, "JSystem/JDrama/JDREfbSetting.cpp")`.

Proof:

- `python tools/decomp-diff.py -u mario/JSystem/JDrama/JDREfbSetting` shows
  no missing or extra target rows.
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored the matching config and verified
  `build/GMSJ01/mario.dol: OK`.

Behavior review:

- Pixel-format helpers issue the same `GXSetPixelFmt`, `GXSetDither`, and
  `GXSetFieldMode` calls with the same pixel-format and dither predicates.
- Copy-filter and copy-clear paths preserve the same antialias/vfilter
  guards, clear-color copy, color/alpha/z update predicates, `GXSetZMode`,
  optional `GXSetZCompLoc`, and return value.
- `IssueGXCopyDisp()` performs the same clamp, filter, conditional clear,
  source rectangle, y-scale, destination width alignment, and `GXCopyDisp`
  call. Remaining deltas are stack size/slots, register coloring, and
  equivalent boolean materialization (`mr` versus `li 0`).
