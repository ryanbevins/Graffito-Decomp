# Enemy/amiNoko

Verdict: equivalent
Date: 2026-06-13 5:40pm MNL

Certified `Object(Equivalent, "Enemy/amiNoko.cpp")`.

Behavior review:
- No missing `.text` functions remain, and no extra `sqrtf` reference remains.
- `TNerveAmiNokoFreeze::execute`, `TNerveAmiNokoTurn::execute`,
  `TAmiNoko::creepToCurPathNode`, `TAmiNoko::bind`,
  `TAmiNoko::getGravityY`, `TAmiNoko::TAmiNoko`, `TAmiNoko::attackToMario`,
  `TAmiNoko::isHitValid`, `TAmiHit::perform`, `TAmiNokoManager::load`, and
  `__sinit_amiNoko_cpp` differ only by frame size, stack slots, register/FPR
  coloring, local constant/static-owner labels, or word-vs-float copies of the
  same vector data.
- `TNerveAmiNokoDie::execute` performs the same launch, wall-hit particle/sound,
  airborne/distance gate, death-flag, spine reset, item-drop, and return logic;
  residue is frame/temporary layout, helper label drift, and branch-layout
  shape around the equivalent wall-hit boolean/distance test.
- `TNerveAmiNokoWalkOnFence::execute` now uses the target
  `JGeometry::TUtil<f32>::sqrt` semantics. Current code inlines that helper
  rather than preserving the target call boundary, which is byte debt only and
  no longer creates an unresolved external.
- `TAmiNoko::calcRootMatrix` performs the same sound gate, particle bindings,
  freeze effects, BCK-0 early matrix copy, wall/ground/roof plane selection,
  normal/forward smoothing, degeneracy fallbacks, matrix writes, translation
  offset, and model-scale copy. The large fuzzy gap is stack/vector temporary
  layout, saved-FPR choices, local labels, and word-vs-float copies.

Proof:
- `python configure.py --non-matching && ninja` passed with
  `Enemy/amiNoko.cpp` source-linked.
- `python configure.py && ninja` then passed with `build/GMSJ01/mario.dol: OK`.

Verdict: fixed_by_implementation
Status: ready_for_audit
Time: 2026-06-13 5:35pm MNL

Implementation fixed the source-link blocker. `TNerveAmiNokoWalkOnFence::execute`
now calls `JGeometry::TUtil<f32>::sqrt`, matching the target asm's
`sqrt__Q29JGeometry8TUtil<f>Ff` call and removing the target-absent external
C `sqrtf` reference.

Proof:
- Normal `python configure.py && ninja` passed and verified
  `build/GMSJ01/mario.dol: OK`.
- Temporary local promotion of `Enemy/amiNoko.cpp` to
  `Object(Equivalent, ...)` passed `python configure.py --non-matching &&
  ninja`; the promotion was reverted for the next AUDIT tick to certify.
- Focused overview now shows `TNerveAmiNokoWalkOnFence::execute` at `92.1%`
  instead of the old `99.8%`, but the regression is expected codegen residue
  from using the real target helper instead of an unresolved external call.

Ready for the next AUDIT tick to re-run full behavior review and promote if no
new structural issue appears.

Verdict: needs_impl
Date: 2026-06-13 5:19pm MNL

Rechecked after the `TYoshi::onYoshi()` owner split. The duplicate Yoshi owner is
gone, but the TU still cannot be certified because it does not link from source:

- Temporary `Object(Equivalent, "Enemy/amiNoko.cpp")` promotion failed
  `python configure.py --non-matching && ninja`.
- Remaining linker error: undefined `sqrtf`, referenced from
  `TNerveAmiNokoWalkOnFence::execute(TSpineBase<TLiveActor>*) const` in
  `amiNoko.o`.
- Restored `Object(NonMatching, "Enemy/amiNoko.cpp")`; normal
  `python configure.py && ninja` passes with `build/GMSJ01/mario.dol: OK`.

Next implementation/investigation work should remove or correctly own the
`sqrtf` reference before AUDIT retries certification.

Verdict: needs_impl
Date: 2026-06-13 3:10am MNL

Behavior notes for this TU are strong, but the source does not link under
`--non-matching` yet.

Temporary promotion to `Object(Equivalent, "Enemy/amiNoko.cpp")` failed:

- multiply-defined `TYoshi::onYoshi()` in `amiNoko.o`, previously defined in
  `MarioAction.o`;
- undefined `sqrtf`, referenced from
  `TNerveAmiNokoWalkOnFence::execute(TSpineBase<TLiveActor>*) const`.

Reverted the classification to `NonMatching` and restored the normal generated
configuration with `python configure.py`. Leave this TU red until ownership of
the extra weak and the `sqrtf` reference are corrected, then re-audit behavior.
