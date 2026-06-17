# mario/NPC/NpcCallback

Verdict: equivalent
Date: 2026-06-13 6:19am MNL

Reason:
- `NPCNeckCallBack(J3DNode*, int)` preserves the same phase guard,
  null/live-flag checks, neck-straight gate, Mario tracking vector,
  distance/height tests, Z-axis rotation delta, pitch clamp/chase, yaw
  clamp/ease, neck-angle store-back, and final joint/current-matrix update.
- Rebuilt-object disassembly confirms the same helper calls (`CLBSquared`,
  `CLBRoundf`, `CLBPalIntSpeed`, `CLBChaseGeneralConstantSpecifySpeed`,
  `CLBCalcRatio`, and `CLBEaseOutInbetween`). Objdiff's misleading call-label
  mismatches come from source-owned helper/local-rodata ownership.
- Remaining residue is stack/temp layout, saved-register/FPR coloring, and
  helper/local-rodata ownership (`TVec3::sub`, `CLBSquared`, `CLBRoundf`).
- All template helper functions are byte-matching.
- Source-link proof passed under `python configure.py --non-matching && ninja`,
  then normal `python configure.py && ninja` restored the matching config and
  verified `build/GMSJ01/mario.dol: OK`.

2026-06-13 10:11am MNL recheck:
- Current overview still has no missing target symbols. Full diff review keeps
  the same phase/null/live gates, neck-straight path, Mario tracking vector,
  distance/height tests, two `MsGetRotFromZaxis` calls, pitch/yaw clamps,
  chase/ease helpers, neck-angle store-back, matrix concat, and current-matrix
  copy.
- Remaining residue is stack/temp layout, saved-register/FPR coloring, helper
  call-label ownership, and local-rodata ownership.

Offending functions: none.
