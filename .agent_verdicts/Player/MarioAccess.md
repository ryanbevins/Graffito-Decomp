# Player/MarioAccess audit

Verdict: equivalent
Status: certified
Time: 2026-06-13 5:12pm MNL

Unit: `mario/Player/MarioAccess`
Source: `src/Player/MarioAccess.cpp`
Classification: `Object(Equivalent, "Player/MarioAccess.cpp")`

Certified behavior-equivalent after the `TYoshi::onYoshi()` owner split. Current
overview has every behavior function exact except `SMS_IsMarioOnWire()` at
93.8%; its full no-collapse diff is reload/CSE shape only:

- target loads `gpMarioOriginal->mHolder` into `r0` for the null check, then
  reloads `mHolder` into `r3` before reading `mActorType`;
- current source reuses the already-loaded holder pointer and reads
  `mActorType` directly;
- null check, `0x40000098` comparison, true/false stores, and `!!ret` tail
  conversion are identical in behavior.

Proof:
- `python tools/decomp-diff.py -u mario/Player/MarioAccess` shows no missing
  behavior-bearing symbols; the old duplicate `TYoshi::onYoshi()` extra is gone.
- `python configure.py --non-matching && ninja` linked successfully from source.

Verdict: fixed_by_implementation
Status: ready_for_audit
Time: 2026-06-13 5:05pm MNL

Implementation fixed the source-link blocker. `TYoshi::onYoshi()` is now
declared in `include/Player/Yoshi.hpp` but its `#pragma dont_inline` body lives
in owner-only `include/Player/YoshiInline.hpp`, included from
`src/Player/MarioMove.cpp`. This keeps the target 28B weak owner in
`mario/Player/MarioMove` and stops `Player/MarioAccess.o` from emitting an
extra copy through the `MarioMain.hpp` include chain.

Proof:
- `python configure.py && ninja` passed and verified
  `build/GMSJ01/mario.dol: OK`.
- `python tools/decomp-diff.py -u mario/Player/MarioMove --search
  "TYoshi::onYoshi"` still reports the owner as `match 100.0%`.
- `python tools/decomp-diff.py -u mario/Player/MarioAccess --search
  "TYoshi::onYoshi"` now reports no matching rows, so the duplicate 28B extra is
  gone.
- Temporary local promotion of `Player/MarioAccess.cpp` to
  `Object(Equivalent, ...)` passed `python configure.py --non-matching &&
  ninja`; the promotion was reverted for the next AUDIT tick to certify.

Remaining residue:
- `SMS_IsMarioOnWire()` remains `93.8%` due to the target reloading
  `gpMarioOriginal->mHolder` before reading `mActorType`, while current source
  CSEs the holder pointer. The null check, actor-type comparison, bool
  normalization, and return behavior are equivalent; see
  `state/notes/MarioAccess_OnWire.md`.
- Existing `__sinit_MarioAccess_cpp` / JSUList static-owner extras remain byte
  debt, not the previous link blocker.

Ready for the next AUDIT tick to re-run full diff review and promote if no new
structural issue appears.

Verdict: needs_impl
Status: needs_impl
Time: 2026-06-13 5:35am MNL

Unit: `mario/Player/MarioAccess`
Source: `src/Player/MarioAccess.cpp`
Classification: `Object(NonMatching, "Player/MarioAccess.cpp")`

## Verdict

Do not promote yet. `SMS_IsMarioOnWire()` is behavior-equivalent: the target
reloads `gpMarioOriginal->mHolder` before reading `mActorType`, while the
source build reuses the already-loaded holder pointer. The null check,
`0x40000098` actor-type comparison, and bool normalization are otherwise the
same.

Temporary promotion to `Object(Equivalent, "Player/MarioAccess.cpp")` failed
the source-link proof:

- multiply-defined `TYoshi::onYoshi()` in `MarioAccess.o`;
- previously defined in `MarioAction.o`.

Leave this TU red until the `TYoshi::onYoshi()` weak/inline ownership issue is
fixed, then re-audit and source-link again.
