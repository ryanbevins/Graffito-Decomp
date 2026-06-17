# Enemy/bombhei

Verdict: equivalent
Date: 2026-06-13 4:12pm MNL

Source-link certification passed after fixing all known behavior blockers.

- `TBombHei::mSerialBomb` now initializes to `true`, matching the target
  `.sdata` byte `0x01`; the previous zero-initialized `.sbss` source disabled
  serial bomb explosion propagation by default.
- Previous implementation fixes remain present: the thrown nerve clears
  actor hit flags at `0x64` on frame `0x78`, and `genEventCoin()` clears
  spawned coin live flag bit `0x10` after velocity assignment.
- Remaining non-100% diffs are codegen/source-shape or ownership debt:
  frame/register layout, branch-materialization shape, local label naming,
  inlined vs out-of-line vector helper boundaries, extra weak/header-owned
  helpers, and data-owner drift from infectious strings/static locals. No
  unresolved behavioral difference found in the current audit pass.

Proof:
- `python configure.py && ninja` passed after the serial flag fix.
- `python tools/decomp-diff.py -u mario/Enemy/bombhei` shows
  `TBombHei::mSerialBomb`, `.sdata`, `.sdata2`, and `.rodata` exact.
- Temporary `Object(Equivalent, "Enemy/bombhei.cpp")` promotion linked with
  `python configure.py --non-matching && ninja`.
- Restored normal config with `python configure.py && ninja`, which verified
  `build/GMSJ01/mario.dol: OK`.

Verdict: fixed_by_implementation
Date: 2026-06-13 4:01pm MNL

Implementation fixed the two behavior blockers from the previous audit:

- `TNerveBombHeiThrown::execute(TSpineBase<TLiveActor>*) const` now clears
  hit flag word `unk64` (`0x64`) at frame `0x78`, matching the target
  `lwz/clrrwi/stw` sequence. The old source cleared `mLiveFlag` at `0xF0`.
- `TBombHei::genEventCoin()` now clears spawned coin live flag bit `0x10`
  after assigning outward velocity, matching the target
  `rlwinm ..., 0, 28, 26` store to `0xF0`.

Proof:
- `python configure.py && ninja` passed.
- `python tools/decomp-diff.py -u mario/Enemy/bombhei -d "TNerveBombHeiThrown" --no-collapse`
  shows the frame-`0x78` clear using offset `0x64` on both sides.
- `python tools/decomp-diff.py -u mario/Enemy/bombhei -d "genEventCoin" --no-collapse`
  shows the spawned-item `mLiveFlag &= ~0x10` clear on both sides.

Ready for the next AUDIT tick to source-link certify or bounce with a fresh
structural finding. Current visible residue is source expression/codegen shape
(`JGeometry::TVec3::sub` call boundary, frame size, register coloring), local
label/data-owner drift, and helper-owner extras.

Verdict: not_equivalent
Date: 2026-06-13 3:09am MNL

`TBombHei::genEventCoin()` has a real behavior gap. After spawning the
event coin and assigning outward velocity, the target clears a bit in the
spawned item's flags at offset `0xF0`:

- target loads `item->0xF0`, applies `rlwinm ..., 0, 28, 26`, then stores it
  back;
- current source returns after writing velocity and does not perform that flag
  clear.

`TNerveBombHeiThrown::execute()` also has a suspicious flag-offset mismatch
around the 120-frame clear: target clears bit 0 at actor offset `0x64`, while
current source clears offset `0xF0`. That needs implementation review before
the TU can be certified.

Leave `Object(NonMatching, "Enemy/bombhei.cpp")` until these behavioral flag
operations are corrected and the TU is re-audited.
