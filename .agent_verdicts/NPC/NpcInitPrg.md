# NPC/NpcInitPrg Audit

Verdict: needs_impl
Date: 2026-06-13 7:43pm MNL

Rechecked after the `TYoshi::onYoshi()` owner split. The old duplicate
`TYoshi::onYoshi()` extra is gone and no target `.text` functions are missing,
but the TU is still not certifiable:

- Current overview has many missing local `.rodata` rows, including NPC
  individual animation/static tables such as
  `TBaseNPC::setMtxEffect_()::sMtxEffectInitData`,
  `sIndividualHoldArrowBck`, Kinopio/Kinojii BCK/BTP tables, Mare animation
  tables, and `sWaistJointName`.
- Rebuilt source emits same-size extra static rows under local `$line` names
  plus extra `__sinit_NpcInitPrg_cpp` / helper template owners. This remains
  static-data ownership work, not a solved source-link certification.

Keep this TU `NonMatching` until the static data layout is reconstructed or an
implementation pass proves those rows are harmless byte debt and source-link
validation passes.

Verdict: needs_impl  
Date: 2026-06-13 3:24am MNL

Source-link proof failed after a temporary `Object(Equivalent, "NPC/NpcInitPrg.cpp")`
classification:

- `python configure.py --non-matching && ninja`
- Linker error: multiply-defined `TYoshi::onYoshi()` in `NpcInitPrg.o`,
  previously defined in `MarioAction.o`.

No behavioral certification was made. Keep the TU `NonMatching` until helper
ownership is corrected enough for the object to link from source.
