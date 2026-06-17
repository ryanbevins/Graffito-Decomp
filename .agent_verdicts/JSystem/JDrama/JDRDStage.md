# mario/JSystem/JDrama/JDRDStage

Verdict: equivalent  
Date: 2026-06-14 2:57am MNL

`mario/JSystem/JDrama/JDRDStage` is certified functionally equivalent and
`configure.py` now marks `JSystem/JDrama/JDRDStage.cpp` as `Equivalent`.

Build proof:
- `python configure.py --non-matching && ninja` linked with this TU from
  source.
- Final `python configure.py && ninja` passed with `mario.dol: OK`.

Reviewed:
- The only nonmatching function is
  `JDrama::TDStageDisp::TDStageDisp(const char*, TFlagT<unsigned short>)`.
  Its behavior matches the target: it constructs the `TViewConnecter` base with
  null endpoints and flag `1`, installs `TDStageDisp`'s vtable, allocates a
  `TEfbCtrlDisp("<EfbCtrlDisp>", flag)` into `unk10`, then allocates a default
  `TViewObjPtrListT<TViewObj>` into `unk14`.
- The old blocker was objdiff label drift around inlined child construction.
  Direct disassembly of the compiled object shows the source still calls
  `TNameRef` with the local rodata strings, constructs the `TFlagT<u16>`,
  initializes the EFB rect, stores the incoming flag to `TEfbCtrl::unk20`, and
  constructs the list base. Remaining differences are instruction scheduling,
  register choices, local rodata/vtable labels, and extra weak/header symbol
  ownership.

Residual:
- Objdiff still reports extra weak/header emissions such as
  `TViewObjPtrListT` methods and stream destructors, plus vtable/data drift.
  No target symbol is missing, and the source-link proof shows these extras do
  not block the non-matching DOL link.
