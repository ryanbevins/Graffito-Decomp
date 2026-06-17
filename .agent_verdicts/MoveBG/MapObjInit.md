verdict: equivalent
date: 2026-06-14 11:26pm MNL
unit: MoveBG/MapObjInit

Promoted after re-auditing the fresh `initBckMoveData()` implementation and
fixing two additional `initUnique()` behavior bugs found in the switch compare
tree:
- `0x4000009d` (`LampSeesaw`) falls through in the target; source no longer
  applies the Bianco material table path to it.
- `0x400000cd` and `0x400000ce` take the same Sirena material table +
  `SMS_UnifyMaterial()` path as `0x400000cb`; source now includes those cases.

Text review:
- `initMapObj`, `initActorData`, `makeMActors`, `initMActor`,
  `initBckMoveData`, `initObjCollisionData`, and `initUnique` now have matching
  behavior. Remaining diffs are stack/register allocation, helper inlining,
  dead anim-row loads, `stfs` vs `stfsu` address spelling, and switch compare
  tree shape.
- `initBckMoveData()` performs the target joint-slot copy and joint-0 transform
  reset before creating the frame controller.

Data review:
- No missing `.text` symbols remain. The only missing target rows are dummy
  ctor/static records `@1431`, `@1411`, and `@1210`; the source has local dummy
  ctor/data churn and weak/helper extras instead.
- Large `.rodata`, `.data`, `.sdata`, and `.sdata2` drift remains byte debt
  from section ownership, pointer relocs, static/dummy labels, and table
  alignment. No behavioral data blocker was identified in this pass.

Proof:
- `python configure.py --non-matching && ninja` linked the from-source DOL.
- `python configure.py && ninja` restored matching config and verified
  `build/GMSJ01/mario.dol: OK`.
