# mario/System/PositionHolder

Verdict: equivalent
Date: 2026-06-13 8:39am MNL

Reverified current `Object(Equivalent, ...)` row during the audit sweep.

Reason:
- `python tools/decomp-diff.py -u mario/System/PositionHolder` still reports
  no missing target symbols; the vtable/data block are exact, with one
  source-owned extra weak destructor.
- Full `--no-collapse` diff for
  `TStagePositionInfo::load(JSUMemoryInputStream&)` shows identical behavior:
  base `TNameRef::load`, then the same nine 4-byte stream reads in the same
  order. Residue is stack-frame size and scratch-slot offsets only.
- The extra destructor is weak-owner drift from the vtable reference, not
  target-absent behavior.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 12:46pm MNL recheck: verdict remains `equivalent`. Fresh full diff
for `TStagePositionInfo::load(JSUMemoryInputStream&)` still shows base
`JDrama::TNameRef::load`, then the same ordered nine 4-byte stream reads into
the same destination fields/scratch locals. The source-owned destructor remains
weak-owner drift. Proof refreshed with `python configure.py --non-matching &&
ninja`, then normal `python configure.py && ninja` with
`build/GMSJ01/mario.dol: OK`.

---

Verdict: equivalent
Date: 2026-06-13 6:06am MNL

Reason:
- Re-verified during the AUDIT sweep. `python tools/decomp-diff.py -u
  mario/System/PositionHolder` reports `TStagePositionInfo::__vtable` and the
  data block as exact, plus one source-owned extra weak
  `TStagePositionInfo::~TStagePositionInfo()`.
- Full `--no-collapse` diff for
  `TStagePositionInfo::load(JSUMemoryInputStream&)` shows identical behavior:
  base `JDrama::TNameRef::load`, then the same nine 4-byte stream reads in the
  same order to the same destination fields / scratch locals. The remaining
  differences are stack-frame size and scratch-slot offsets only.
- The extra destructor is weak-owner drift: this TU's vtable references the
  destructor, but the target owns that weak body in another object. It is not
  target-absent runtime behavior.
- Proof: `python configure.py --non-matching && ninja` linked from source,
  then normal `python configure.py && ninja` passed and verified
  `mario.dol: OK`.

Offending functions: none.
