# mario/Enemy/BathtubPeach

Verdict: equivalent  
Date: 2026-06-15 3:35am MNL

Reason:
- Re-audited under the current behavioral bar. `TBathtubPeach::calcRootMatrix()`
  has the same bathtub demo-matrix gate and fallback `TLiveActor::calcRootMatrix`
  call; remaining drift is stack/register and local-label debt.
- `TNervePeachEscape::execute()` keeps the same bathtub lookup, demo/stagger
  gates, BCK/BTP/frame-rate setup, Mario/Peach angle selection, radius target,
  speed clamp, movement, and turn-speed clamp. Remaining drift is frame size,
  FPR/GPR coloring, helper-boundary, and data-label debt.
- The missing target `TVec2<float>::dot` row is an inlined helper in source, not
  a source-link undefined. The previous source-link blocker was plain `fmodf`;
  fixed by routing this TU's `std::fmodf` calls to the existing weak owner
  `fmodf__3stdFff` instead of the MSL inline wrapper's unavailable `fmod`.

Proof:
- `python configure.py --non-matching && ninja` linked and built `mario.dol`
  with `Enemy/BathtubPeach.cpp` sourced.
- `python configure.py && ninja` restored normal matching config and passed
  `build/GMSJ01/mario.dol: OK`.
