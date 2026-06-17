# JSystem/JDrama/JDRActor audit

Verdict: equivalent
Status: equivalent
Time: 2026-06-13 6:12pm MNL

Unit: `mario/JSystem/JDrama/JDRActor`
Source: `src/JSystem/JDrama/JDRActor.cpp`
Classification: `Object(Equivalent, "JSystem/JDrama/JDRActor.cpp")`

## Verdict

Certified `Equivalent`. `JDrama::TActor::load(JSUMemoryInputStream&)` is
behavior-equivalent: it loads placement, reads rotation and scale fields, reads
the character name, searches `TNameRefGen`, allocates a `TLightMap`, initializes
the same `TNameRef("<LightMap>")` base, `TViewObj` vtable, `TFlagT<u16>(0)`,
`TLightMap` vtable, and zero fields, stores it at `unk40`, and calls its
virtual `load`.

Implementation follow-up, 2026-06-13 6:03pm MNL:

- Source now owns both missing weak constructors:
  `JDrama::TFlagT<unsigned short>::TFlagT(unsigned short)` and
  `JDrama::TNameRef::TNameRef(const char*)`; focused overview shows no missing
  or extra `.text` rows.
- `TActor::load` improved 80.5% -> 93.3%. Remaining diffs are stack/register
  and equivalent call-label/codegen residue after the base constructor calls.
- Normal proof: `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.
- Temporary source-link proof:
  `Object(Equivalent, "JSystem/JDrama/JDRActor.cpp")` +
  `python configure.py --non-matching && ninja` linked successfully; the row was
  restored to `NonMatching` for AUDIT to certify.

AUDIT certification, 2026-06-13 6:12pm MNL:

- Re-ran current overview and full `TActor::load` diff. `.rodata` and `.data`
  are byte-identical; no missing or extra symbols remain.
- Raw target asm confirms the same `TLightMap` construction and virtual
  `load` behavior. Remaining text drift is stack frame/register coloring and
  misleading weak-call labels in decomp-diff, not a behavioral difference.
- Promoted to `Object(Equivalent, "JSystem/JDrama/JDRActor.cpp")`.
- Proof: `python configure.py --non-matching && ninja` linked cleanly from
  source, then normal `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.
