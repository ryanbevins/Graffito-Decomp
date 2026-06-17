# Enemy/popo

Verdict: equivalent
Date: 2026-06-15 9:01pm MNL

Reason: strict AUDIT review found no behavior blocker. The source object links
from source and every non-exact text row is codegen-class: stack/register/FPR
allocation, helper-boundary/inlining choices, static-init owner label drift,
matrix/vector temporary layout, and branch layout. The one reported missing
row, `@4471`, is not a missing constructor/helper; it is the target `.sdata2`
`20.0f` constant used by `TPopo::explosion()`/params and exists in the rebuilt
object under different local label/order.

Reviewed:
- Nerve executes: wait/attack/possessed/fly/explosion branches, state pushes,
  calls, hit/live flag writes, and animation setup match behavior. Raw asm
  confirms the target calls `explosion`, `flyBehavior`, `possessedIn`, and
  `checkTrigger`; decomp-diff's displayed call names are local-label drift.
- `bind`, `forceKill`, `calcRootMatrix`, `walkBehavior`, `behaveToWater`,
  callbacks, trigger handling, and manager methods have the same calls,
  stores, constants, and branch conditions. `forceKill` lowers the same
  `TBGCheckData` predicate set through a different compare tree.
- Data differences are local-label/order residue in rodata/sdata/sdata2 and
  source-only weak/static owners; no unresolved referenced target symbol
  remains.

Proof:
- `python configure.py --non-matching && ninja` linked with
  `Enemy/popo.cpp` sourced.
- `python configure.py && ninja` passed `build/GMSJ01/mario.dol: OK`.
