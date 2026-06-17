verdict: equivalent
date: 2026-06-14 12:16pm MNL
unit: mario/System/PerformList

Source-link proof:
- Promoted `System/PerformList.cpp` to `Object(Equivalent, ...)`.
- `python configure.py --non-matching && ninja` linked successfully.
- Restored normal config with `python configure.py && ninja`; `build/GMSJ01/mario.dol: OK`.

Review notes:
- `TPerformList::perform` walks the same `JGadget::TSingleLinkList` nodes,
  masks the caller flags with each link's stored flags, and calls
  `JDrama::TViewObj::testPerform` on the stored view object. The low fuzzy
  score is iterator temporary/helper ownership and register layout.
- `TPerformList::load` matches behavior: base load, loop until stream end,
  read an 80-byte name, resolve it through `JDrama::TNameRefGen`, read the
  flags, OR `0x3000` when bit 0 is set, and push a new `TPerformLink` only
  when the name resolves.
- Both `push_back` overloads allocate the same `TPerformLink` and insert it at
  the list end. Remaining diffs are inline list helper ownership and iterator
  construction shape.
- Destructor differences are codegen-class list teardown/frame residue.
- Missing tiny weak list helpers and extra `JDrama::TViewObj` weak symbols are
  symbol ownership drift; no target behavior is missing.
