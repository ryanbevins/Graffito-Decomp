# mario/MoveBG/MapObjFence

Verdict: equivalent  
Date: 2026-06-13 3:04am MNL

`MoveBG/MapObjFence.cpp` is functionally identical and links from source.

Reviewed `state/notes/MapObjFence.md`, the unit overview, and fresh diffs for
the lowest-score functions: `TRailFence::goOnRail`, `TFenceWaterH::control`,
and `TRevolvingFenceInner::setGroundCollision`. Remaining differences are
codegen-class: frame/local-slot layout, saved-register/FPR coloring, matrix
temporary scheduling, inline vs out-of-line `gekko_ps_copy12`, graph/vector
temporary placement, and local data/helper ownership. A suspicious
`TFenceWaterH::control` call label in `decomp-diff.py` was checked against raw
target asm; the target calls `controlRotation__11TFenceWaterFv`, matching the
source behavior.

The source preserves the same rail graph progression, terminal-node wait/sound
state, water-fence messenger positioning, rotation-matrix composition, Yoshi
bounding checks, collision matrix movement, and revolving-fence wall/roof state
transitions.

Proof:

- `python configure.py --non-matching && ninja` linked `mario.dol` from source.
- `python configure.py && ninja` passed and verified `build/GMSJ01/mario.dol:
  OK`.

## 2026-06-13 11:24am MNL recheck

Refreshed during the stale-Equivalent sweep. The rebuilt object has no
undefined references to the missing dirty-texture data rows. Full diffs for
`TRailFence::goOnRail`, `TFenceWaterH::control`, and
`TRevolvingFenceInner::setGroundCollision` still preserve behavior: same rail
advance/terminal-node sound and wait-state path, same water-fence rotation
matrix plus messenger tracking, and same active-Yoshi bounding-box gate with
collision `moveMtx`. Residue is stack/frame shape, helper inlining
(`gekko_ps_copy12`), local matrix scheduling, and register/FPR coloring.

Proof passed:
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja` (`build/GMSJ01/mario.dol: OK`)
