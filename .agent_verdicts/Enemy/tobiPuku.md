# Enemy/tobiPuku Audit

Verdict: equivalent  
Date: 2026-06-14 2:36pm MNL

Promoted `Enemy/tobiPuku.cpp` to `Object(Equivalent, ...)`.

Proof:

- `python tools/decomp-diff.py -u mario/Enemy/tobiPuku -s missing` reports no
  missing symbols after the implementation fix to
  `TNerveTobiPukuSwimWander::execute`.
- `python configure.py --non-matching && ninja` linked the DOL with
  `tobiPuku` sourced.
- `python configure.py && ninja` restored normal matching config and passed
  `build/GMSJ01/mario.dol: OK`.

Behavior review:

- Rechecked the lower-score nerves and helpers:
  `ReturnLaunch`, `PrepareFly`, `Bound`, `Land`, `Die`, `HitWater`,
  `Attack`, `Fly`, `Generate`, `TTobiPuku::hitWater`, `forceKill`,
  `kill`, `hitWall`, `TTobiPukuLaunchPad::forceLaunch`, and
  `TobiPukuRollCallback`.
- Remaining text diffs are behavior-neutral stack/register/FPR allocation,
  vector temporary placement, helper-boundary choices, equivalent boolean or
  branch materialization, and local static / const-pool label drift.
- The callback target filters the same three roll states as source
  (`Land`, `PrepareFly`, `ReturnLaunch`); objdiff's local-static labels are
  shifted, but raw asm confirms the same vtables and same roll matrix update.
- `pukupuku_bastable` and `moepuku_bastable` contain the same runtime entries
  as target. Their nonmatching data rows are caused by source-owned infectious
  data before the arrays and relocation/label drift, not wrong BAS entries.

Known byte debt:

- Extra source-owned weak/destructor/list helpers and infectious data rows.
- Stack-frame and local-vector shape residue in `ReturnLaunch`,
  `TMoePuku::calcRootMatrix`, `TTobiPukuLaunchPad::forceLaunch`, and
  `TobiPukuRollCallback`.
