# Player/MarioSpecial Audit

Verdict: needs_impl  
Date: 2026-06-13 4:00am MNL

Not promoted.

Reason:

- `python tools/decomp-diff.py -u mario/Player/MarioSpecial` shows missing
  local data symbols, including `.ctors` `@3404` and `@4992`.
- The rebuilt object emits target-absent helpers such as
  `THitActor::receiveMessage(...)`, `TWaterGun::isEmitting()`,
  `TYoshi::onYoshi()`, JSUList destructors, and extra `.rodata`.
- The missing target data symbols block certification.
