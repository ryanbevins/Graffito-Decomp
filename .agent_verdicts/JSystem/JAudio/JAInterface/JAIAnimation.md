# JSystem/JAudio/JAInterface/JAIAnimation

Verdict: equivalent
Date: 2026-06-13 7:52am MNL

Promoted `JSystem/JAudio/JAInterface/JAIAnimation.cpp` to
`Object(Equivalent, ...)` after source-link proof.

Fresh audit note: this recheck supersedes the older flat-path
`state/audit/JSystem_JAudio_JAInterface_JAIAnimation.md` note. The apparent
structural issues called out there are register-lifetime and constant-scheduling
residue, not behavior gaps.

Reviewed functions:
- `JAIAnimeSound::initActorAnimSound(void*, unsigned long, float)`: same data
  pointer/counter setup, frame-data search, forward/reverse loop-counter setup,
  and eight-slot cleanup. Residue is register choice only.
- `JAIAnimeSound::setAnimSoundActor(...)`: same forward and reverse playback
  branches, loop wrap handling, slot scan/update, frame-window predicates,
  sound start/stop calls, speed-modify calls, and final current-time store.
  Residue is saved-register allocation and equivalent branch layout.
- `JAIAnimeSound::playActorAnimSound(...)`: same duplicate-slot scan,
  sound-ID/flag gating, start path, slot state writes, volume/pitch/pan setup,
  and `mDataCounter += mDataCounterInc` tail. Residue is branch layout,
  scheduling, and integer-to-double temp ordering.

Data:
- `.sdata2` differs by constant order/local labels (`1.0f` and `0.03125f`
  swap positions), but source code references the correct constants. No target
  functions observe the section order directly.

Validation:
- `python configure.py --non-matching && ninja` linked successfully with
  `JAIAnimation` from source.
- `python configure.py && ninja` passed and reported `mario.dol: OK`.
- Rechecked `setAnimSoundActor` and `playActorAnimSound` on 2026-06-13
  7:52am MNL; no missing/extra text symbols.

2026-06-13 10:43am MNL recheck: verdict remains `equivalent`. Re-read the
current diffs for all three nonmatching functions. `initActorAnimSound` still
does the same data setup, frame-data search, direction-counter setup, and
eight-slot cleanup. `setAnimSoundActor` keeps the same forward/reverse wrap
logic, slot scan/update, frame-window predicates, virtual start/modify calls,
stop path, and current-time store; inserted/missing-looking lines are register
lifetime and branch-layout residue. `playActorAnimSound` keeps the same
duplicate-slot scan, sound-id/flag gate, start path, slot-state stores, volume,
pitch, pan, and counter increment; the noisy float conversion block only
reorders constant loads and temporaries. Proof refreshed with
`python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
