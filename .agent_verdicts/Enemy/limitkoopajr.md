# Enemy/limitkoopajr audit

Verdict: equivalent
Status: equivalent
Updated: 2026-06-15 11:11pm MNL

Source-link proof:
- `Object(Equivalent, "Enemy/limitkoopajr.cpp")`
- `python configure.py --non-matching && ninja` linked from source during the
  implementation handoff, and this audit is rerunning the same proof after the
  final promotion.

Behavior review:
- All target text symbols are present. `createModelData()` and the manager
  functions are exact after the const model-data table restoration.
- `TNerveLimitKoopaJrYahoo::execute()` and
  `TNerveLimitKoopaJrLaunch::execute()` perform the same initial BCK/sound
  setup, animation-end transition to Wait, and return values. Remaining diffs
  are frame/static-local owner naming.
- `TNerveLimitKoopaJrWait::execute()` and
  `TNerveLimitKoopaJrRun::execute()` perform the same target-vs-Mario
  direction check, transition decisions, shot-timer launch decision, Mario-jump
  yahoo decision, rotation update, and `moveRun()` call. Remaining diffs are
  stack/register placement, helper-boundary drift (`TVec3::setLength` in target
  vs inlined normalization in source), and static nerve owner labels.
- `TLimitKoopaJr::moveRun()` performs the same round-angle turn, target-relative
  orbit position update, height override, vector normalization/fallbacks,
  up-vector cross product, turn-sign flip, and facing update. Remaining diffs
  are helper-boundary/register/frame choices (`dot`/`scale`/normalization).
- `TLimitKoopaJr::calcRootMatrix()` performs the same yaw quaternion-to-matrix
  calculation, translation stores, `PSMTXCopy`, and base-scale copy. Target
  keeps an out-of-line `TRotation3::setQuat` call where source inlines the same
  math.
- `perform()`, `reset()`, `init()`, and `TLimitKoopaJrParams` have the same
  manager/bathtub lookup, timer stores, spine reset/init behavior, hit actor
  setup, scale/store defaults, shape flag loop, and parameter defaults. Diffs
  are rodata offsets, frame/register allocation, and static-init owner names.

Data review:
- Target `@2659` and `@2661` are 12-byte zero/one vector rows at `.rodata`
  offsets `0xe0` and `0xec`; source emits byte-identical rows as
  `dummy2659`/`dummy2661`, so objdiff reports missing/extra labels only.
- Other `.rodata`, `.data`, `.sdata`, and `.sdata2` drift is constant-pool,
  vtable/static-init, or owner-label debt. No reviewed diff changes a loaded
  value, branch condition, call side effect, or store target.
