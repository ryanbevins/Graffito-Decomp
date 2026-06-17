# Audit verdict: equivalent

- Date: 2026-06-13 11:32pm MNL
- TU: `mario/GC2D/MessageLoader`
- Source: `src/GC2D/MessageLoader.cpp`
- Verdict: `equivalent`

## Reason

All target symbols are present. Exact functions: `getMessageEntry`,
`loadMessageData`, default constructor, and the weak `JSUIosBase` constructor.
The two non-exact target functions are codegen/ownership-class only:

- `TMessageLoader::parseBlock(unsigned long, unsigned long, void*)`: target and
  rebuild parse the same block loop, switch on `INF1`/`DAT1`/`STR1`, skip the
  same block sizes, return the same `DAT1` data pointer, and destroy the same
  stream objects. Raw target asm and object relocations confirm the suspicious
  `decomp-diff` call label in the `INF1` path is actually
  `JSUIosBase::JSUIosBase()`, not `loadMessageData`. The target inlines the
  same `readInfoBlock` behavior; the rebuild also emits `readInfoBlock` as an
  extra standalone helper.
- `TMessageLoader::TMessageLoader(const char*)`: same initialization,
  `JKRGetResource`, `parseBlock`, `unk4` store, and null check. Residue is
  stack-frame/slot layout only.

Extra helper/destructor/vtable symbols are source-emission debt; there are no
missing target symbols and no extra startup constructors.

## Proof

- This tick's `python configure.py --non-matching && ninja` source-linked the
  current `Equivalent` set successfully.
- `python configure.py && ninja` restored the normal matching config and passed
  with `build/GMSJ01/mario.dol: OK`.
