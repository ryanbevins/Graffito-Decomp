# mario/System/SnapTimeObj

Verdict: equivalent
Date: 2026-06-13 6:06am MNL

Reason:
- Re-verified during the AUDIT sweep. `TSnapTimeObj::~TSnapTimeObj()` and
  `TSnapTimeObj::__vtable` are exact; the only source-owned extras are weak
  `JDrama::TViewObj` destructor/vtable ownership drift.
- Full `--no-collapse` diff for
  `TSnapTimeObj::perform(unsigned long, JDrama::TGraphics*)` shows identical
  behavior: the same active flag test, `0x80` and `0x8` perform-bit gates, the
  same `TTimeRec` singleton null guards, and the same `snapGXTime`, `endTimer`,
  `OSGetTick`, `TTimeArray::append`, and final `snapGXTime` effects with the
  same timer ids. Remaining mismatches are frame size, saved-register slot
  offsets, and one equivalent spill/reload register choice for `unk10`.
- Proof: `python configure.py --non-matching && ninja` linked from source,
  then normal `python configure.py && ninja` passed and verified
  `mario.dol: OK`.

2026-06-13 10:11am MNL recheck:
- Current overview still has no missing target symbols. `perform` still has the
  same active flag, perform-bit gates, `TTimeRec` null guards, `snapGXTime`,
  `OSGetTick`, `TTimeArray::append`, and timer-id effects.
- Remaining residue is frame size, save-slot offsets, and an equivalent
  spill/reload register choice for `unk10`.

Offending functions: none.
