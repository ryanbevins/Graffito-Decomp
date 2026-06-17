# mario/JSystem/JDrama/JDRDrawBufObj

Verdict: equivalent
Date: 2026-06-13 4:15pm MNL

Re-verified the existing `Equivalent` certification during the audit fallback
sweep.

- `python tools/decomp-diff.py -u mario/JSystem/JDrama/JDRDrawBufObj` still
  shows no missing target rows.
- `TDrawBufObj::perform` still has the same `0x80`, `0x400`, and `0x8`
  perform-bit gates, calls `J3DDrawBuffer::frameInit()` / `draw()` in the same
  cases, and writes the same values to `j3dSys.mDrawBuffer[0]`,
  `j3dSys.mDrawBuffer[1]`, and `j3dSys.unk4C`.
- Remaining drift is stack-frame/save-slot size and branch-label address
  displacement from weak-owner extras.
- Source-link proof is covered by this tick's successful
  `python configure.py --non-matching && ninja`; normal config was restored
  with `python configure.py && ninja` and verified `build/GMSJ01/mario.dol: OK`.

Verdict: equivalent
Date: 2026-06-13 9:10am MNL

Reason:
- Re-verified during the AUDIT sweep. Both constructors, `load`, the
  destructor, rodata, and `JDrama::TDrawBufObj::__vtable` are exact. The only
  source-owned extras are weak `JDrama::TViewObj` destructor/vtable ownership
  drift.
- Full `--no-collapse` diff for
  `JDrama::TDrawBufObj::perform(unsigned long, JDrama::TGraphics*)` shows the
  same behavior: the same `0x80`, `0x400`, and `0x8` perform-bit gates, the
  same `J3DDrawBuffer::frameInit`/`draw` calls, and the same writes to
  `j3dSys.mDrawBuffer[0]`, `j3dSys.mDrawBuffer[1]`, and `j3dSys.unk4C`.
  Remaining mismatches are stack-frame size / save slots and branch-label
  address drift from the extra weak owner rows.
- Proof: `python configure.py --non-matching && ninja` linked from source,
  then normal `python configure.py && ninja` passed and verified
  `mario.dol: OK`.
- 2026-06-13 9:10am MNL recheck: full current diff remains codegen-only, and
  the proof batch again linked from source before the normal build verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
