# Enemy/TabePuku

Verdict: equivalent
Date: 2026-06-15 5:37pm MNL

Implementation cleared the previous hard blockers. `entry$2868` is now a
`static const TModelDataLoadEntry` in read-only data, `@3954` is recovered via
the target anti-parallel `TQuat4::setRotate(from, to, amount)` path
(`PI()`), and `@5098` is recovered by the anonymous `cAngleMax = PI()/8`
dynamic initializer. `__sinit_TabePuku_cpp` and `entry$2868` both diff at
100%.

Remaining overview blocker:
- `JGeometry::TVec3<float>::set<float>(float, float, float)` is still reported
  missing as a 16B target-local helper boundary. There is no undefined
  reference, and a temporary `Object(Equivalent, "Enemy/TabePuku.cpp")` proof
  passed `python configure.py --non-matching && ninja`.

Behavior review:
- `TTabePuku::swimTo` matches target semantics: zero target skips quaternion
  work, nonzero target normalizes direction, builds a +Z-to-direction
  quaternion, handles anti-parallel with a +Y PI rotation, slerps by
  `mTurnSlerpRate`, normalizes, applies water friction, and writes yaw from
  velocity.
- `TTabePuku::control` matches the helper collision loop and sound gate: Mario
  collision calls owner vtable slot `0x168` (`attackToMario()`), and the
  bite/dive/drag nerves gate sound `0x2123` through `gpMSound->gateCheck()`.
- `TTabePuku::doKeepDistance`, `TTPHitActor::bind`, and the reviewed TabePuku
  nerve bodies have matching state transitions and side effects. Residue is
  helper-boundary, local-label, stack/register/FPR, and branch-shape codegen.

Proof:
- Promoted `Object(Equivalent, "Enemy/TabePuku.cpp")`.
- `python configure.py --non-matching && ninja` passed and linked
  `build/GMSJ01/mario.dol` from source.
- Restored the normal matching configuration with `python configure.py &&
  ninja`; `build/GMSJ01/mario.dol: OK`.
