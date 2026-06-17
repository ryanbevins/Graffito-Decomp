# MarioUtil/LightUtil audit

Verdict: `equivalent`

Checked 2026-06-15 9:44am MNL in AUDIT mode.

Audit found and fixed one behavioral blocker before promotion:

- `TLightCommon::setLight()` and `TLightMario::setLight()` now transform
  `gpLightManager->unk1C` for the extra DB/effect light position. The old
  source used `unk48`; target asm uses offset `0x1c`, which is populated from
  the first light position in `TLightWithDBSetManager::loadAfter()`.

Remaining non-exact rows are byte/codegen debt only:

- `JGadget::TList_pointer<JDrama::TViewObj*>::end()` calls the typed iterator
  ctor instead of inlining the copy, but returns the same iterator.
- `TLightWithDBSetManager::addChildGroupObj()` has the same two typed
  `end()` calls, two `insert()` calls, and ignored-return iterator
  constructions; displayed call-label drift is symbol/address alignment noise.
- Color accessors and `loadAfter()` differ by stack-frame size, local
  placement, redundant root reloads, and address-calculation form while keeping
  the same offsets, calls, alpha scaling, and loop bounds.
- `TLightWithDBSetManager` constructor differs in expression scheduling for
  attenuation coefficient math and constant-pool/data-label layout; it stores
  the same object graph, defaults, and derived coefficients.
- Missing `JGadget::TList<void*, JGadget::TAllocator<void*>>::end()` is an
  inlined weak helper with no rebuilt undefined reference.
- Missing rodata `@1490` is paired with extra
  `@unnamed@::dummyLightUtilStringValue`; bytes are the same 12-byte zero
  infectious literal.

Proof:

- `python configure.py --non-matching && ninja` linked with LightUtil sourced.
- `python configure.py && ninja` passed `build/GMSJ01/mario.dol: OK`.
