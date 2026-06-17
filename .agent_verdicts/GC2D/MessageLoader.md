# GC2D/MessageLoader Audit

Verdict: equivalent  
Date: 2026-06-13 1:59pm MNL

Kept as `Object(Equivalent, "GC2D/MessageLoader.cpp")`.

Proof:

- `python tools/decomp-diff.py -u mario/GC2D/MessageLoader` shows no missing
  target symbols.
- `python configure.py --non-matching && ninja` linked the TU from source.
- `python configure.py && ninja` restored the matching config and verified
  `build/GMSJ01/mario.dol: OK`.

Behavior review:

- `TMessageLoader::parseBlock()` performs the same block tag loop, `INF1`,
  `DAT1`, and `STR1` handling, `JSUMemoryInputStream` setup/destruction,
  stream skips, message-entry reads, and returned data pointer. The target
  inlines `readInfoBlock()`; current source emits it as an extra standalone
  helper, but raw asm confirms the inlined operations match despite misleading
  call labels in the rendered diff.
- `TMessageLoader::TMessageLoader(const char*)` performs the same resource
  lookup, header reads, parse-block call, `unk4` store, and null check.
  Remaining differences are stack-frame size/slots and helper-owner labels.
- Extra `readHeader()`, `readInfoBlock()`, JSU stream destructors, and vtable
  rows are ownership/dead-emission drift; they do not change runtime behavior.
