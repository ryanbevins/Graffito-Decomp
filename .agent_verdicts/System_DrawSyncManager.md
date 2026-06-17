# Audit: mario/System/DrawSyncManager

## Verdict
equivalent

## Date
2026-06-14 7:10am MNL

## Proof
- Promoted `System/DrawSyncManager.cpp` to `Object(Equivalent, ...)`.
- `python configure.py --non-matching && ninja` linked successfully from source.
- Follow-up normal `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.

## Review Notes
- Overview has no missing target symbols; `.sbss` and `.sdata2` rows are exact.
- `setCallback()` stores the same token range aggregate into `mCallbacks[param_1]`;
  the only drift is stack slot placement for the temporary range.
- `threadFunc()` preserves the same message-loop behavior: high addresses are
  pushed into the FIFO and enable the breakpoint when FIFO size reaches 2;
  low token messages advance the read index, disable at size 1, and re-enable
  the next queued breakpoint at size >= 2; messages >= `0x10000` exit.
  Residue is frame size and stack-slot offsets.
- `start()` performs the same singleton guard, allocation, vector construction,
  thread/message-queue/FIFO setup, resume, and singleton store. The source-owned
  out-of-line constructor is byte debt; the target-visible constructor body is
  inlined in `start()`.
- `JGadget::TVector<TDrawSyncTokenRange>::insert()` and its inlined raw insert
  path perform the same capacity test, in-place hole creation, reallocation
  copies, old-element destruction, delete, and final fill. The apparent helper
  label mismatch in decomp-diff is the raw target `DestroyElement_` call; the
  raw asm confirms it is not a behavior difference.
- Source-only extras (`TDrawSyncManager` ctor/dtor/end and vector helper
  owners) are unreferenced/source ownership byte debt. The source-link proof
  confirms they do not leave unresolved target behavior.
