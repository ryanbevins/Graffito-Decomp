# JSystem/JParticle/JPADrawVisitor audit

Verdict: equivalent
Time: 2026-06-13 6:31am MNL

Status: equivalent
Time: 2026-06-13 5:55am MNL

Unit: `mario/JSystem/JParticle/JPADrawVisitor`
Source: `src/JSystem/JParticle/JPADrawVisitor.cpp`
Commit: `8604bd9d`

## Verdict

Promoted to `Object(Equivalent, ...)`.

Full audit found and fixed concrete behavior mismatches before promotion:
- `JPADrawExecRotYBillBoard::exec` now uses `MTXMultVecSR` for the particle
  position, matching the target's rotation-only transform.
- `JPABaseShape` texture matrix accessors now return the offsets used by the
  target draw code: the Y-translation pair and X-scale pair were reversed in the
  inline getters.
- `JPADrawExecDirectionalCross::exec` and
  `JPADrawExecRotDirectionalCross::exec` now pass eight vertices to `GXBegin`;
  both functions emit eight positions/texcoords and the target count is `8`.
- `JPADrawExecDirBillBoard::exec` now crosses against row 2 of `unk34`
  (`[2][0..2]`) instead of column 1.
- `JPADrawExecStripeCross::exec` now resets the strip texture-coordinate
  accumulator before the second triangle-strip pass.

Remaining nonmatching text is codegen-class: frame size/stack slot drift,
FPR/register allocation, expression scheduling, and equivalent local matrix /
vector temporary placement. The true target vtables and constants are
byte-identical; the extra symbols are unused compiler-emitted visitor base
vtables/destructors and do not block the source link.

Proof:
- `python configure.py --non-matching && ninja` linked
  `build/GMSJ01/mario.dol` from source with JPADrawVisitor promoted.
- `python configure.py && ninja` restored the matching config and passed
  `build/GMSJ01/mario.dol: OK`.
- 2026-06-13 6:31am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.
- 2026-06-13 11:31am MNL recheck: overview and unresolved-symbol check still
  show no missing target behavior. Remaining nonmatches are the same
  frame/register/matrix temporary residues and unused visitor
  destructor/vtable extras. `python configure.py --non-matching && ninja`
  linked from source, then `python configure.py && ninja` restored the normal
  config and verified `build/GMSJ01/mario.dol: OK`.
