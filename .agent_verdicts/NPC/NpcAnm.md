# NPC/NpcAnm Audit

Verdict: needs_impl  
Date: 2026-06-13 4:00am MNL

Not promoted.

Reason:

- `python tools/decomp-diff.py -u mario/NPC/NpcAnm` shows missing target
  helper text for `CLBLinearInbetween<float>`,
  `CLBCalcRatio<float>`, `CLBChaseConstantSpecifyFrame<float>`,
  `CLBChaseGeneralConstantSpecifySpeed<float>`, `CLBPalFrame<long>`, and
  `CLBRoundf<long>`.
- Missing data includes `.ctors` `@1490`, `@1937`, `@2255`-`@2258`,
  peach/sunflower animation tables, several local rows, all of `.data-0`,
  and `.sdata-0`.
- The TU is not source-symbol complete, so it cannot be certified.
