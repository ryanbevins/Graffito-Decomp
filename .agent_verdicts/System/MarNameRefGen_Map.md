# System/MarNameRefGen_Map audit

Verdict: equivalent
Date: 2026-06-13 2:19pm MNL

`mario/System/MarNameRefGen_Map` is functionally identical and links from
source.

Review:
- `TMarNameRefGen::getNameRef_Map(const char*) const` preserves the same string
  dispatch, allocation sizes, constructor arguments, returned object types, and
  null fallback.
- The formerly suspicious `TPollutionTest` and `TStickyStainManager` paths are
  now classified as codegen/helper-owner drift: target calls base constructors
  while current source inlines equivalent `TNameRef` / `TViewObj` / `TFlagT`
  initialization stores before installing the same derived vtables.
- The current `@2089` / `@2093` rows match; the remaining missing infectious
  strings are paired with same-size extras from local ownership and do not
  change runtime data.

Proof:
- `python configure.py --non-matching && ninja` linked `mario.dol` from source.
- `python configure.py && ninja` restored the normal matching config and
  verified `build/GMSJ01/mario.dol: OK`.
