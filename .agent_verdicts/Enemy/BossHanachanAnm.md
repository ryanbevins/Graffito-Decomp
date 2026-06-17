Verdict: equivalent
Time: 2026-06-13 6:30am MNL
Unit: Enemy/BossHanachanAnm

Audited all nonmatching text symbols and promoted the TU to
`Object(Equivalent, ...)` in `configure.py`.

- `TBossHanachan::changeAnmRateAndFrameUpdate_()` matches behavior across the
  tumble nerve path, walk/run threshold branches, blend-ratio transitions,
  frame-rate clamp, and final head/body `MActor` updates. Residue is branch
  spelling, stack frame size, register coloring, and objdiff label noise around
  nearby `setHeadAndBodyAnm` calls.
- `isAllBckAlreadyEnd()`, `setAnmTimerWhenDead()`,
  `setAnmTimerWhenDamage()`, `setTumbleAnm()`, and
  `setHeadAndBodyAnm()` are behavior-identical; remaining diffs are
  bool/branch shape, stack slots, and register/FPR allocation.
- Proof: `python configure.py --non-matching && ninja` linked
  `build/GMSJ01/mario.dol` from source, then plain `python configure.py &&
  ninja` passed and verified `build/GMSJ01/mario.dol: OK`.
- 2026-06-13 6:30am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.

Reverified: 2026-06-13 10:57am MNL — still equivalent. Re-read all current
nonmatching helpers. Tumble/walk/run thresholds, blend-ratio transitions,
absolute part-timer math for dead/damage, tumble animation selection, old-frame
copying, head/body animation selection, and final head/body frame updates still
preserve the same calls, stores, constants, and branch semantics. Remaining
drift is stack/register/FPR layout, equivalent compare-tree spelling, and
objdiff helper/local-label presentation. Proof passed again with `python
configure.py --non-matching && ninja`, then plain `python configure.py && ninja`
restored the matching config and verified `build/GMSJ01/mario.dol: OK`.
