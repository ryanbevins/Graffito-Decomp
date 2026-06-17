## Verdict: equivalent

Date: 2026-06-14 8:03pm MNL

Reason: promoted after the missing `TSpineBase<TLiveActor>::getLatestNerve()`
owner was implemented. Current strict review found no behavioral mismatch in
the remaining nonmatching rows. The low-score diffs are stack/register layout,
`TSpcStack<TSpcSlice>::push` inlined stores versus a target helper call,
local rodata/static-label ownership, and unrelated `JALList` static-init owner
ordering.

Proof:
- `python configure.py --non-matching && ninja` linked with
  `NPC/NpcEvent.cpp` sourced and produced `build/GMSJ01/mario.dol`.
- `python configure.py && ninja` restored matching config and passed
  `build/GMSJ01/mario.dol: OK`.

Reviewed functions:
- `TNpcEvent::reviveOneSunflower()` and `ReviveSunflowerCallBack()` preserve
  sunflower lookup, down-count decrement, demo camera callback, sound choice,
  shine demo, and flag clear semantics; remaining drift is frame/static-label
  codegen.
- Builtin event predicates/actions preserve argument pop/push behavior,
  director mode checks, talk-forcing state writes, dummy NPC lookup, fruit
  mapping, demo camera call, Monte-clear check, and nerve comparison behavior.
- `__sinit_NpcEvent_cpp` residue is static-init owner/label ordering debt, not
  a source-link blocker.

---

## Previous blocker

Date: 2026-06-13 2:55am MNL

Reason: the TU is not promotable in the audit sweep because objdiff reports a
missing target text symbol:
`TSpineBase<TLiveActor>::getLatestNerve() const` (28B). The current source uses
the header/inline shape at the call site, so the event predicate behavior may be
close, but the standalone weak/helper owner is absent.

Offending functions/symbols:
- Missing `.text`: `TSpineBase<TLiveActor>::getLatestNerve() const`.
- `evCheckLatestNerve4Npc`: affected caller; target/source differ around the
  latest-nerve helper boundary and the final `TSpcStack<TSpcSlice>::push`
  boundary.

Other reviewed diffs:
- `evCheckCurNerve4Npc` and `evIsNpcSinkBottom` are dominated by stack/register
  coloring plus `TSpcStack<TSpcSlice>::push` inlining/ownership differences.
- Missing data rows around `sCameraNames$2626` / `sCameraNames` appear to be
  local label ownership rather than a separate behavioral issue, but the text
  symbol gap blocks certification.

Verdict: fixed_by_implementation
Status: ready_for_audit
Time: 2026-06-14 7:53pm MNL

Implementation fixed the missing text blocker. `NpcEvent.cpp` now owns
`TSpineBase<TLiveActor>::getLatestNerve() const` through an owner-only
declaration split in `Spine.hpp` and a `dont_inline` specialization placed in
the target symbol-order slot. The helper is 100%.

Proof:
- Normal `python configure.py && ninja` passed and verified
  `build/GMSJ01/mario.dol: OK`.
- Temporary local promotion of `NPC/NpcEvent.cpp` to
  `Object(Equivalent, ...)` passed `python configure.py --non-matching &&
  ninja` through DOL generation; the promotion was reverted for the next AUDIT
  tick.
- The remaining reviewed predicate diffs (`evCheckLatestNerve4Npc`,
  `evCheckCurNerve4Npc`, `evIsNpcSinkBottom`) are stack/register,
  `TSpcStack<TSpcSlice>::push` call-boundary, and local data-label residue.
  No remaining behavior gap was found in those predicates.

Ready for the next AUDIT tick to re-run the full TU review and promote if the
other near-exact rows are still behavior-equivalent.
