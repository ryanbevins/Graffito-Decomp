# Enemy/launcher

Verdict: equivalent
Date: 2026-06-13 2:39pm MNL

Promoted `Enemy/launcher.cpp` back to `Object(Equivalent, ...)` after the
implementation fix in `TLauncher::receiveMessage(THitActor*, unsigned long)`.

Proof:
- `python tools/decomp-diff.py -u mario/Enemy/launcher` has no missing symbols.
- Full no-collapse diffs for all sub-100% functions show no remaining behavior
  drift:
  - `TLauncher::receiveMessage`: now uses sender `param_1 + 0x10` for both the
    water-hit particle and `MSound::startSoundSet(0x6802, ...)`; remaining
    diff is stack frame/save-slot size only.
  - `TCommonLauncher::perform`: same parent perform, launch-effect emitter,
    scale/color stores, regen timer/hitpoint update, collision loop, and Mario
    attack message; remaining diff is frame/save-slot size only.
  - `TCommonLauncher::stateHitByWater`: same first-frame BCK/sound/hitpoint
    update and animation-end state transition; remaining diff is frame size.
  - `TCommonLauncher::stateLaunch`: same enemy lookup, graph copy, rotation
    wrap, matrix/vector setup, `PSMTXMultVec`, `resetSRTV`, cooldown reset, and
    state transition; remaining diff is matrix/vector stack-slot placement and
    local constant labels.
  - `TCommonLauncher::init`: same manager/model/spine setup, launch cooldown
    randomization, hit actor init, group insertion, texture replacement, anim
    sound setup, DL flag, hitpoint initialization, and rotation wrap. Raw
    `objdump -drC` confirms the apparent decomp-diff iterator-call label drift
    is only local helper ownership; both target and build call the same
    `JGadget::TList` iterator constructors.
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and verified
  `build/GMSJ01/mario.dol: OK`.

Remaining byte debt: `.data`/vtable/static-local ownership drift, extra weak
helpers/destructors, stack-frame sizes, and temp stack-slot coloring.

Verdict: fixed_by_implementation
Date: 2026-06-13 2:31pm MNL

Implementation fixed the audit blocker in
`TLauncher::receiveMessage(THitActor*, unsigned long)`. The source now passes
`&param_1->mPosition` to both the water-hit particle emit and
`MSound::startSoundSet(0x6802, ...)`.

Build / diff proof:
- `python configure.py && ninja` passed.
- `python tools/decomp-diff.py -u mario/Enemy/launcher -d 'TLauncher::receiveMessage' --no-collapse`
  now shows the rebuilt function preserving sender `param_1` in `r31` and using
  `addi r5, r31, 0x10` for both calls, matching target behavior. Remaining
  residue is stack/frame offset only.

Ready for the next AUDIT tick to source-link re-certify as `Equivalent`. The
other sub-100% rows remain the codegen/data-owner residue documented below.

Verdict: not_equivalent
Date: 2026-06-13 8:24am MNL

Downgraded `Enemy/launcher.cpp` from `Object(Equivalent, ...)` to
`Object(NonMatching, ...)`.

Offending function:
- `TLauncher::receiveMessage(THitActor*, unsigned long)`: target saves the
  incoming sender actor (`param_1`) in `r31` and, after confirming the sender is
  actor type `0x1000001` and the message is `HIT_MESSAGE_SPRAYED_BY_WATER`,
  passes `param_1 + 0x10` as the position to both
  `TMarioParticleManager::emit(0xE7, ...)` and
  `MSound::startSoundSet(0x6802, ...)`. Current source passes `&mPosition`
  (`this + 0x10`) to both calls. That changes the observable particle/sound
  origin, so the TU is not functionally equivalent.

Other reviewed residue remains codegen/data debt only:
- `TCommonLauncher::perform`: same perform call, launch-effect emitter and
  scale/color setup, regen timer/hitpoint update, collision loop, and Mario
  attack message; residue is frame size/slot placement.
- `TCommonLauncher::stateLaunch`: same BCK change, enemy lookup, graph copy,
  rotation wrapping, matrix/vector setup, resetSRTV call, cooldown reset, and
  state transition; residue is local stack placement for matrix/vector temps.
- `TCommonLauncher::stateHitByWater`: same first-frame BCK/sound/hitpoint
  update and animation-end state transition; residue is frame size.
- `TCommonLauncher::init`: same manager/model/spine setup, launch cooldown
  randomization, hit actor, group insertion, texture replacement, animation
  sound, DL flag, hitpoint initialization, and rotation wrap; residue is
  stack/register shape, `mLaunchPeriod` read scheduling around `rand()`, and
  local helper/label ownership.

Proof:
- `python configure.py && ninja` passed after the classification downgrade.
