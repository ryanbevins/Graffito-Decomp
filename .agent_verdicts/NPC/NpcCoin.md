# NPC/NpcCoin

Verdict: equivalent
Date: 2026-06-13 1:07pm MNL

Reverified current `Object(Equivalent, "NPC/NpcCoin.cpp")` again during the
audit-only sweep. The audit found three nonmatching gameplay functions:
`TNpcCoin::updateCoin`, `TNpcCoin::requestAppearCoin(const Vec&, float, int)`,
and `TNpcCoin::TNpcCoin(int)`. All differences are codegen-class register,
frame-size, or vtable-load-register choices; control flow, constants, memory
offsets, object reuse/new-spawn behavior, flag clearing, and SE call arguments
match the target.

`__sinit_NpcCoin_cpp` initially had shifted MSound `JALList` static-init
ownership. Adding `<MSound/MSoundBGM.hpp>` after `<MSound/MSoundSE.hpp>`
restored the canonical 15-list sinit and made it byte-match.

Validation:
- Shared proof: `python configure.py --non-matching && ninja` linked from source, then `python configure.py && ninja` restored the normal matching config and verified `build/GMSJ01/mario.dol: OK` at 2026-06-13 1:07pm MNL.
