# mario/JSystem/JParticle/JPABaseShape

Verdict: equivalent
Date: 2026-06-13 2:42pm MNL

Revalidated as part of the AUDIT secondary safety sweep. Current overview is
unchanged: all behavior-bearing text/data symbols are present; the only text
diff is `makeColorTable(JPAColorRegAnmKey*, int, int, JKRHeap*)`, and the only
data drift is paired `JPADataBlock` weak/vtable ownership.

Current full diff classification:
- `makeColorTable`: same allocation size, initial channel loads, keyframe loop,
  exact-key `GXColor` copy, next-key denominator, per-channel interpolation
  deltas, zero-delta terminal path, accumulated channel updates, `fctiwz`
  rounding, byte stores, loop counters, and return value. Residue is FPR/GPR
  coloring, scheduling of the initial double conversions, and local constant
  label ownership.

Proof reused from this tick: `python configure.py --non-matching && ninja`
linked with this row from source, then `python configure.py && ninja` restored
normal config and verified `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

Verdict: equivalent
Date: 2026-06-13 9:08am MNL

Reason:
- Re-verified during the AUDIT sweep. All target behavior-bearing symbols are
  present; `JPABaseShape`'s constructor, destructor, static tables, vtable, and
  `.sdata2` match.
- The existing `Object(Equivalent, ...)` classification linked successfully
  under the current `python configure.py --non-matching && ninja` proof; the
  plain matching build then restored normal config and verified
  `build/GMSJ01/mario.dol: OK`.

Reviewed nonmatching function:
- `makeColorTable(JPAColorRegAnmKey*, int, int, JKRHeap*)`: allocation size,
  keyframe loop, exact-key copy path, interpolation denominator, per-channel
  accumulators, rounded color writes, loop counters, and return match target
  behavior. The refreshed full diff shows only FPR/register scheduling in the
  initial channel setup and interpolated channel values.

Notes:
- Extra `JPADataBlock` destructor/vtable symbols are weak ownership residue;
  the nonmatching aggregate `.data` row is paired with that source-owned weak
  vtable and is not target-absent runtime state.

Offending functions: none.
