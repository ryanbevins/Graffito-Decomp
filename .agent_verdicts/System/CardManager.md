# System/CardManager audit verdict

Verdict: equivalent
Status: equivalent
Date: 2026-06-16 12:08am MNL
Unit: `mario/System/CardManager`

Strict AUDIT certified this TU after fixing one real behavior mismatch:
`TCardManager::readOptionBlock_()` must clear the option sector when
`mSectorCriteria[0].mState == STATE_EMPTY` (`1`), not `STATE_UNREAD` (`0`).
The target inlined path in `cmdLoop()` compares the state word against `1`.

Remaining non-exact rows are byte/codegen debt:
- private helper boundaries and source-owned helper bodies vs target inlining
  (`probe_`, `unmount_`, `format_`, `buildHeader_`, `readOptionBlock_`,
  `writeCardSector_`, `TCardSector` helpers, stream destructors)
- stack/register drift in the private worker paths
- `setCardStat_` SDK macro loop shape for clearing icon slots 2..7
- anonymous `@1431`/`@1411`/`@1210` labels vs named dummy declarations
- source-owned `JSUIosBase` weak/vtable residue

Proofs:
- `python configure.py --non-matching && ninja` linked with
  `Object(Equivalent, "System/CardManager.cpp")`.
- `python configure.py && ninja` passed `build/GMSJ01/mario.dol: OK`.
