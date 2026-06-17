# GC2D/PauseMenu2

Verdict: needs_impl
Date: 2026-06-13 12:29pm MNL

Do not promote yet. Rechecked during the AUDIT sweep under the behavioral
equivalence rule.

The overview still reports a missing target-owned
`JGeometry::TVec3<float>::set<float>(float, float, float)` helper, but that
alone is not enough to keep the TU red if the source-linked object has identical
behavior. This TU still needs implementation/audit work for behavior-heavy
functions before that helper can be treated as byte debt.

Current blockers:
- `TPauseMenu2::perform(unsigned long, JDrama::TGraphics*)` is only `63.0%`
  and drives state transitions, input handling, card-save integration, sound,
  and draw/update side effects. It has not been behavior-certified.
- `TPauseMenu2::appearWindow()` (`78.9%`) and
  `TPauseMenu2::drawAppearPane(...)` (`80.8%`) are low enough that the
  remaining diffs cannot be assumed to be codegen-only without a full asm pass.
- Data/constant ownership also differs (`@3222` missing, `@3154` nonmatching,
  source-owned shine-table extras), so the static UI tables should be checked
  alongside the behavior functions.

Keep this row `NonMatching` until the low-score UI state/draw paths are
reviewed or implemented, then rerun the source-link proof.
