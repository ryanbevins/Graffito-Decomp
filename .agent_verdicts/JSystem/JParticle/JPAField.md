# JSystem/JParticle/JPAField audit

Verdict: `equivalent`

Checked 2026-06-14 10:43am MNL in AUDIT mode.

Proof:
- `python configure.py --non-matching && ninja` linked the source DOL with
  `JPAField.cpp` included from source.
- `python configure.py && ninja` restored the normal matching build and ended with
  `build/GMSJ01/mario.dol: OK`.

Behavior fixes landed during audit:
- `JPABaseField::loadFieldBlock(JPADataBlock*)` now reads the two-byte field status
  into object offset `0x54` (`unk54`) and then skips the same two padding bytes as
  target.
- `JPAConvectionField::affect(JPAParticle*)` now reuses the radial vector when adding
  the secondary `unk34` component instead of shadowing it with an uninitialized local.

Remaining byte debt is codegen-class only: stack/register/FPR allocation, helper
boundary differences for vector normalization (`inv_sqrt` call vs inline sequence),
right-to-left random argument scheduling in `JPARandomField::affect`, and weak/helper
symbol emission. No missing target symbols remain.
