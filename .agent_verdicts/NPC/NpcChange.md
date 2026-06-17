# mario/NPC/NpcChange

Verdict: equivalent
Status: certified
Time: 2026-06-15 4:42am MNL

## Audit Result
Certified `mario/NPC/NpcChange` as behaviorally equivalent after the tick 802
implementation fixes.

One additional audit fix landed before promotion: `behaveToBeTrampled_()` now
uses signed actor-type range compares for the Mare/adult sound selector, matching
the target branch conditions. The remaining text differences are codegen-class:
stack/register/FPR allocation, helper ownership/inline boundaries
(`CLBRoundf`/`CLBPalFrame`, `isNerveCanGoToTalk()` inlined into
`changeNerveProc_()`), local-bool branch layout, same-final-value live-flag store
ordering, and const-pool label drift.

Data review:
- `.ctors` in target and source both contain one relocation to
  `__sinit_NpcChange_cpp`; reported ctor extras are label/accounting noise.
- `.sdata2` has extra source constants from helper/codegen ownership, but no
  missing target symbols and no behavior-bearing data mismatch was found.

Proof:
- `git diff --check`
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja` (`build/GMSJ01/mario.dol: OK`)
