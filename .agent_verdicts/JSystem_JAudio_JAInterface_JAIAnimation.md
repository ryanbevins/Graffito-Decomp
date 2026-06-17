## Verdict: equivalent

Date: 2026-06-13 7:52am MNL

This legacy flat-path note is superseded by the canonical audit file at
`state/audit/JSystem/JAudio/JAInterface/JAIAnimation.md`.

The earlier `needs_impl` verdict came from structural-looking diffs in
`setAnimSoundActor()` and `playActorAnimSound()`. A deeper recheck found those
to be codegen-class residue:

- `setAnimSoundActor()`: forward and reverse playback branches perform the same
  pending-event scan, counter resets, loop-count updates, slot iteration,
  frame-window predicates, speed-modify calls, stop gates, tail playback scan,
  and final `mCurrentTime` store. The deleted `li r29, 0` / `li r28, -1`
  setup in the reverse branch is register-lifetime noise around local loop
  setup, not a missing state update.
- `playActorAnimSound()`: duplicate-slot detection, stream/non-stream gate,
  flag/direction checks, virtual `startAnimSound`, slot writes, volume/pitch/pan
  setup, and `mDataCounter += mDataCounterInc` tail all match behaviorally.
  The pitch block differs by local constant order and integer-to-double
  scheduling only.

No missing or extra text symbols were present in the fresh recheck.
