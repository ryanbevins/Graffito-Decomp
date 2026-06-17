# System/DrawSyncManager Audit

Verdict: equivalent  
Date: 2026-06-14 4:44pm MNL

Re-verified the stale `not_equivalent` note while the TU is already
`Object(Equivalent, ...)` in `configure.py`.

Current symbol status:
- No missing target symbols.
- `.sbss` and `.sdata2` match exactly.
- Source-only text owners are dead API/vector helpers (`end`, destructor,
  `erase`, `InsertRaw`, etc.) and do not block source-linking.

Behavior review:
- `TDrawSyncManager::setCallback(...)` differs only by stack-slot placement
  around the `TDrawSyncTokenRange` temporary; the destination vector element and
  copied range/callback values are identical.
- `TDrawSyncManager::threadFunc(void*)` matches the raw target FIFO loop:
  high messages are pushed and enable the breakpoint only when FIFO size
  becomes 2; low token messages advance the read index, disable at size 1, and
  enable the next queued pointer at size >= 2 via `getLoopIdx(read + 1)`.
  Differences are frame/register/branch-label drift.
- `TDrawSyncManager::start(...)` allocates the same manager, callback vector
  capacity/default element, OS thread/queue buffers, `TFifo`, resume call, and
  `smInstance` store. Differences are register coloring and constructor/vector
  helper-boundary shape.
- `JGadget::TVector<TDrawSyncTokenRange>::insert(...)` matches the target
  in-place and reallocation algorithms: capacity check, tail copy/shift,
  growth by the vector factor, allocate new storage, copy prefix/suffix, destroy
  and free old storage, update begin/end/capacity, then fill `count` inserted
  elements. The large diff is register allocation, frame size, and objdiff
  helper-label drift (`DestroyElement_`/fill path), not a different operation.

Proof:
- `python tools/decomp-diff.py -u mario/System/DrawSyncManager -s missing`
  reports no rows.
- The earlier `python configure.py --non-matching && ninja` source-link proof
  in this tick included this existing Equivalent TU and linked cleanly.
- Normal `python configure.py && ninja` passed afterward with
  `build/GMSJ01/mario.dol: OK`.

Byte debt:
- Remove/route source-only vector helpers and the `end`/destructor owners in a
  future byte-matching pass.
- Match vector `insert` frame/register/helper-boundary shape if this TU becomes
  an investigation target.
