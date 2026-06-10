# MWCC — current understanding

_The graffito decomp bot's living theory of how the Metrowerks CodeWarrior compiler
(MWCC GC/1.2.5) behaves. Updated by the bot as it observes the compiler. Read by the
bot at the start of every tick alongside `CLAUDE.md` and `AGENTS.md`._

This file is **not** authoritative — `CLAUDE.md` is. This is where the bot accumulates
its working hypotheses, open questions, and confirmed observations as it works. When a
hypothesis here becomes well-established with multiple independent confirmations, it
should be promoted into `CLAUDE.md` and removed from here.

---

## Structure

Organise the content under exactly these three top-level sections. Don't add new
top-level sections — extend the existing ones. If you want a deeper hierarchy, use
H3 (`###`) inside a section.

- **Settled** — observations confirmed across at least two independent TUs. Each
  entry is a one-paragraph statement of the rule with a citation list (TU/function
  names where it was observed). These are candidates for promotion to `CLAUDE.md`.
- **Hypotheses under investigation** — patterns the bot has noticed but isn't yet
  sure about. Each entry explains the hypothesis and the specific experiment that
  would confirm or refute it.
- **Open questions** — phenomena the bot doesn't understand yet. Each entry is a
  question + the symptom that prompted it. Move to "Hypotheses under investigation"
  when an idea forms.

Within each section, order entries newest-first (prepend, don't append). Old entries
that turned out wrong should be moved to a `## Refuted / wrong turns` section at the
bottom rather than deleted — preserving the dead-ends prevents the bot from re-trying
them in future ticks.

---

## Settled

### Name repeated `f32` call parameters before helper calls to force saved-FPR lifetimes

**Rule.** When target asm loads a float field into `f31`/`f30` before one or
more helper calls and then passes that value after or repeatedly across those
calls, bind the field to a real `f32` local before the call sequence. Repeating
the member expression inline can make MWCC keep the base pointer in a saved GPR
and reload the float after each call; the local form promotes the value into a
callee-saved FPR and often matches the target's larger FPR save set.

**Citations.**
- `mario/Enemy/chuuhana` `TNerveChuuHanaJumpPrepare::execute` (t267):
  naming `mSLJumpSp.get()` as `jumpSpeed` before `getGravityY()` changed the
  live range from saved pointer `r29` to saved `f31`, moving `94.2 -> 99.8`.
- `mario/Camera/CameraJetCoaster`
  `CPolarSubCamera::ctrlJetCoasterCamera_()` (2026-06-11 MNL): naming repeated
  tail chase speeds (`p68 + 0x94/0x9c` and `p68 + 0xa4/0xa8`) before three
  `CLBChaseDecrease` calls forced saved FPRs instead of a saved `p68` reload
  path, moving `74.6 -> 76.5`.

### Explicit no-op state cases can force dense jump-table dispatch

**Rule.** When target asm uses a jump table for a mostly sparse state-machine
switch, add explicit `case` labels for semantically meaningful no-op states at
the low/high bounds instead of relying on `default`. MWCC's jump-table decision
depends on the visible case density/range, and empty cases can point to the
same default exit while still making the dispatch table and range check match.
Order non-empty case bodies by target body layout; jump-table entries can still
map numeric cases to bodies emitted in source order.

**Citations.**
- `mario/MoveBG/Item` `TEggYoshi::control()` (2026-06-11 MNL): adding no-op
  `case 1` and `case 0x10`, then ordering bodies `0xD -> 0xB -> 0xC -> 0xF`,
  changed a compare tree to the target `0..0x10` jump table and moved the
  function `14.1 -> 99.8`.
- `mario/Enemy/Tongue` `TTongue::movement()` (t169): structuring the state
  dispatch as contiguous `case 0` through `case 7` forced the target jump table
  and moved the function from 0% to the first structurally aligned state.

### Signed `switch ((s32)x)` preserves sparse actor/selector compare trees

**Rule.** When target asm lowers a small sparse selector as a signed compare
tree (`cmpw/cmpwi`, high/adjacent cases tested before lower physical body
layout), write the source as `switch ((s32)x)` rather than an `if/else if`
chain or unsigned equality disjunction. Order `case` bodies by target body
layout, not by semantic importance. For inline predicate helpers, keep the
per-case returns branchy or use inverse guards that fall through to one shared
`return true` block when target has explicit compare/branch exits; compact
boolean expressions may collapse into `subfic/cntlzw` materialization.

**Citations.**
- `mario/MSound/MSoundSE` `MSoundSE::checkSoundArea(unsigned long, const Vec&)`
  (2026-06-10 MNL): `if (param == 7) ... else if (param == 8) ...` tested the
  low case first; `switch ((s32)param)` matched the target high-first signed
  dispatch and moved `91.8 -> 99.5`.
- `mario/MarioUtil/ShadowUtil`
  `TMBindShadowBody::TMBindShadowBody(THitActor*, J3DModel*, float)`
  (2026-06-10 MNL): rewriting actor-type scale selection and the three
  bind-joint inline predicates as signed switches with explicit/inverse-guard
  returns and target case-body order moved the constructor `55.5 -> 81.3`.

### Static inline wrappers can preserve local `MsWrap<float>` helper ownership and call boundaries

**Rule.** When target asm has a TU-local `MsWrap<float>(float, float, float)`
symbol and call sites branch to it, direct `MsWrap<f32>(...)` calls may inline
the wrap loops and leave the local helper missing. Add a small TU-local wrapper:

```cpp
static inline f32 callMsWrap(f32 t, f32 l, f32 r)
{
	return MsWrap<f32>(t, l, r);
}
```

Then call `callMsWrap(...)` at the affected sites. This gives MWCC a natural
out-of-line local owner without changing the shared template or forcing weak
emission globally. Verify per TU; some direct `MsWrap` uses are supposed to
inline.

**Citations.**
- `mario/MoveBG/MapObjRailBlock` `TRailBlock::control` (2026-06-09): routing
  the three angle-delta wraps through `callMsWrap` emitted the exact 72-byte
  local `MsWrap<float>` symbol and moved `control` `66.2 -> 76.5`.
- `mario/MoveBG/MapObjItem2`: existing `callMsWrap` wrapper owns an exact
  72-byte local `MsWrap<float>` symbol used by mushroom angle wrapping.
- `mario/Enemy/popo`: existing `callMsWrap` wrapper owns an exact 72-byte local
  `MsWrap<float>` symbol used by `TPopo` rotation wrapping.

### By-value `TVec3<float>` scalar multiply preserves weak `scale(float)` call boundaries

**Rule.** When target asm copies a `TVec3<float>` to a stack temp, loads a
scalar into `f1`, calls weak `JGeometry::TVec3<float>::scale(float)`, then
copies the temp to the destination, write the source as scalar multiplication
through the by-value friend operator: `dst = src * scalar` or
`tmp = src * scalar`. Direct `tmp = src; tmp.scale(scalar);` often inlines the
three `fmuls/stfs` pairs and can leave the weak owner missing. This is a
natural expression-level lever, not a reason to force weak emission globally.

**Citations.**
- `mario/Enemy/Tongue` `TTongue::emit` / `movement` (t170): switching fixed
  vector math to friend operators recovered target `scale` / `sub` / `add`
  call boundaries and moved `emit` `57% -> 87%`.
- `mario/MarioUtil/MtxUtil` (t403): `TRope::moveHead` and
  `SMS_MakeJointsToArc` use `velocity * mVelocityScale`, `dir * t`, and
  `upDir * (1.0f - t)` to emit the TU-local weak
  `TVec3<float>::scale(float)` owner and move the functions `43.1% -> 54.4%`
  and `51.7% -> 59.7%`.

### Explicit `== true` after a materialized bool forces a compare-to-1 retest

**Rule.** Once source shape has forced MWCC to materialize a bool into a 0/1
local, the branch test is still source-controlled. A bare `if (b)` retests with
`clrlwi. r0, rN, 24; beq`. Writing `if (b == true)` instead emits
`clrlwi r0, rN, 24; cmplwi r0, 1; bne`. Use this when the target already has
the materialization (`li 0`, conditional `li 1`) and differs only in the final
retest form. The materialization itself may come from a `cond ? true : false`
ternary or from an explicit `bool b = false; if (cond) b = true;` local.

**Citations.**
- `mario/Player/MarioEffect` `TMarioEffect::perform` (t317): case 0 uses the
  bare ternary and matches target's `clrlwi.; beq`; case 1 needs explicit
  `== true` to match `clrlwi; cmplwi 1; bne`, contributing to
  `92.6 -> 100.0`.
- `mario/MoveBG/ModelGate` `TModelGate::perform` (t389): explicit
  false-then-set bool plus `== true` produced the target compare-to-1 retest
  and moved `perform` `83.2 -> 84.9`. Toggling only the final test to
  `if (jumping)` changed the retest back to `clrlwi.; beq` and regressed to
  `84.7`.

### Inline `TParamRT<T>::get()` can preserve direct field-load codegen while inflating leaf stack frames

**Rule.** Outside `#pragma dont_inline`, a small `TParamRT<T>::get()` accessor
can inline away to the same visible field load as direct `.value` access, while
still changing MWCC's frame allocation. Use this when a near-exact leaf function
reads one or more `TParamRT<T>` fields directly, the target frame is larger than
the build, and the displayed instruction stream is otherwise already correct.
Toggle direct `.value` reads to `.get()` one at a time and verify that the frame
grows without introducing a `bl` or changing load/order codegen.

This is the inverse of the `dont_inline` TParam rule: under `dont_inline`,
`.get()` emits a real accessor call and `.value` is required; in a normal TU,
the accessor can be a legitimate frame-shape lever without adding a call.

**Citations.**
- `mario/NPC/NpcTrample` `TNpcTrample::startTrample` (t371): direct
  `mSLTrampleAmplitude.value` / `mSLTrampleShakeFrames.value` emitted the
  target instruction stream with a `0x18` frame. Switching only the shake-frame
  read to `.get()` grew the frame to `0x20`; switching both reads grew it to the
  target `0x30` and made the function exact.
- `mario/Enemy/rocket` `TRocket::getGravityY() const` (t371): switching
  `mParams->mSLFlyGravity.value` to `.get()` preserved the field load at
  `0x2f8` and grew the frame from `0x30` to target `0x40`, making the function
  exact.
- `mario/Animal/Bird` `TAnimalBird::initParams` (t375): switching
  `getSaveParam()->mSLHitPointMax.value` to `.get()` preserved the `lbz`
  field read at `0x7c` and grew the frame from `0x20` to target `0x28`, making
  the function exact.

### Explicit template specialization declarations make a TU call the existing weak owner instead of emitting a local helper copy

**Rule.** When a header-defined template helper is called in a TU but the target
calls an existing weak specialization owned by another TU, add a TU-scope
explicit specialization declaration before the caller:

```cpp
template <> f32 CLBCalcRatio<f32>(f32, f32, f32);
template <> s16 CLBLinearInbetween<s16>(s16, s16, f32);
```

This hides the generic template body from that call site's instantiation path
and makes MWCC emit a `bl` relocation to the existing weak owner instead of a
target-absent local weak copy. Use this only when `symbols.txt` / target asm
show a weak specialization already owned elsewhere and the current TU lists the
helper as `extra`.

**Citations.**
- `mario/Camera/CameraSecureView` (t358): declaring
  `CLBLinearInbetween<f32>` kept the secure-view calls external and removed the
  local 12-byte weak body.
- `mario/Camera/CameraMultiPlayer` (t362): declaring
  `CLBCalcRatio<f32>` and `CLBLinearInbetween<s16>` removed three
  target-absent helper extras and made the target calls line up with the weak
  owners in `NpcAnm` / `cameragc`.
- `mario/NPC/NpcCoin` plus the 2026-06-07 camera sweep
  (`CameraDemo`, `CameraInbetween`, `cameragc`, `CameraOption`,
  `CameraBGCheck`, `lensflare`): declaring
  `template <> s16 CLBRoundf<s16>(f32);` routed calls to the existing
  `NpcBase` weak owner and removed local 52-byte `CLBRoundf<short>` extras.

### Predeclare locals in target register order to steer callee-saved GPR coloring

**Rule.** When several locals are live across the same call sequence and the
remaining diff is a clean callee-saved GPR permutation, declaration order can
control MWCC's allocation. Predeclare the locals in the target's desired
high-to-low register order, then assign them at the original evaluation points.
This preserves call/store order while moving lifetimes into the target register
slots. The lever applies to real locals, not to arbitrary expression
reordering; keep the source evaluation order matching asm.

**Citations.**
- `mario/Map/PollutionLayer`
  `TPollutionLayerWallPlusX::stamp` /
  `TPollutionLayerWallPlusZ::stamp` (t337): predeclaring `s`, `t`, and
  `depth` before the `TPollutionPos*` local shifted the live values into
  target `r31/r30/r29/r28` order and moved both wall stamps `84.8% -> 85.2%`.
- `mario/NPC/NpcNerve` `TNerveNPCBlown::execute` (t345): predeclaring
  `bool isMare` before `TBaseNPC* npc` swapped the bool and NPC pointer into
  target `r30/r29` order, taking the function `99.4% -> 100%`.

### For `u16`-indexed pointer-table loops, use an `int` induction variable with a `(u16)i` loop condition to get `clrlslwi` offsets and `clrlwi` compares

**Rule.** When looping over a pointer table through an inline getter that takes
a `u16` index, a source `u16 i` can make MWCC maintain a byte-offset induction
variable (`i += 4`) and reuse that offset for table loads. If the target instead
increments an element index (`i += 1`), computes each pointer-table access as
`clrlslwi index, i, 16, 2`, and narrows before the loop compare with
`clrlwi`, write the loop as:

```cpp
int num = data->getMaterialNum();
for (int i = 0; (u16)i < num; ++i) {
	data->getMaterialNodePointer(i)->...
}
```

The `int` induction variable prevents MWCC from choosing a byte-offset IV, while
the `(u16)i` condition restores the target's explicit narrow-before-compare.
Using plain `int i; i < num` gets the `clrlslwi` table offset but drops the
`clrlwi` compare. Using `u16 i` keeps the compare but often produces the wrong
byte-offset IV.

**Citations.**
- `Camera/lensflare` `TLensFlare::perform` (t333): changing the material-alpha
  loop from cached `J3DMaterial* material` / `u16 i` to uncached
  `getMaterialNodePointer(i)` calls with `int i; (u16)i < num` restored the
  target `clrlslwi` table offset and `clrlwi` compare, moving `perform`
  `62.1 -> 64.4`.
- `Camera/lensglow` `TLensGlow::perform` (t333): the same loop-condition shape
  in the TEV alpha loop restored the `clrlslwi` access pattern and moved
  `perform` `80.6 -> 81.9`.

### Hoist a default assignment before an if-chain when target preloads the default and only recomputes non-default arms

**Rule.** When one arm of a small branch assigns a constant/default value and
target asm loads that default **before** the first compare, write the source as
an unconditional default initialization followed by guarded recomputes:

```cpp
// target shape: default is live before the condition, branch skips recompute
f32 value = DEFAULT;
if (!defaultCondition) {
	if (condA)
		value = A;
	else
		value = B;
}
```

This differs from `if (defaultCondition) value = DEFAULT; else ...`, which
loads the default inside the taken branch and usually emits an extra branch. It
also differs from a `switch` with a `default:` arm: MWCC treats the switch
default as a branch target, not as a preloaded fall-through value. For
`switch`-like integer selections where target does sequential equality checks
and keeps the default on fall-through, write `value = DEFAULT; if (sel == K1)
...; else if (sel == K2) ...;` with no final `else`.

**Citations.**
- `Enemy/igaiga` `TRollEnemy::setBehavior` (t281): natural
  `if/else-if/else` loaded the hidden default in-branch and emitted a redundant
  branch/compare. Preloading `range = 2.0f` and guarding recompute moved the
  function `93.6 -> 95.8`.
- `Enemy/BathtubKiller::resetBathtubKiller` (t289): a `switch` with
  `default: off = 0.0f` lowered to a branch table. Rewriting as
  `off = 0.0f; if (sel == 0) ...; else if (sel == 1) ...;` matched the target
  sequential compare/default-fall-through shape and moved the switch block
  `94.7 -> 97.1`.

### Copy `J3DGXColorS10::color` as a `GXColorS10` aggregate when target uses two-word TEV color copies

**Rule.** The repository's current `J3DGXColorS10` wrapper has an explicit copy
constructor that copies `r/g/b/a` as four `s16` fields, so a typed wrapper copy
like `J3DGXColorS10 c = *material->getTevColor(0);` emits four halfword
loads/stores. When target asm copies the 8-byte TEV color as two word
loads/stores, copy the underlying aggregate member instead:

```cpp
J3DGXColorS10 c;
c.color = material->getTevColor(0)->color;
```

MWCC lowers the `GXColorS10` aggregate assignment to `lwz 0/4` and `stw 0/4`,
which matches the target word-copy block without changing the shared J3D
wrapper. Do **not** globally change `J3DGXColorS10`'s copy constructor without a
dedicated header audit; a t321 probe produced the desired local copy shape but
broke the linked DOL checksum in already-matching J3D-dependent TUs.

**Citations.**
- `Camera/lensflare` `TLensFlare::perform` (t321): wrapper copy emitted
  `lha/sth` channel copies; member aggregate assignment matched target word
  copies and moved `perform` `59.1 -> 61.2`.
- `Camera/lensglow` `TLensGlow::perform`: existing source already uses
  `c.color = getTevColor(0)->color`, and the material alpha loop matches the
  same target `lwz 0/4` + `stw 0/4` copy shape.

### A multi-clause guard before a trailing code block: positive-AND-braces `if (A && B) { body }` lowers to single fall-through; OR-early-return `if (!A || !B) return; body;` lowers the LAST clause to a two-branch `b<cc> body; b end`

**Rule.** When a function ends with a guard protecting a trailing code block,
the source structure picks the branch shape of the **last** guard clause:

- **`if (A && B && C) { body }`** (positive-AND, body in braces) → every
  clause is a direct skip-to-end (`b<!cc> end`), and the body is the natural
  **fall-through**. The last clause is a single branch (`bge end; body`).
- **`if (!A || !B || !C) return; body;`** (OR-early-return) → clauses 1..n-1
  are direct `b<cc> end`, but the **last** clause becomes a **two-branch**
  `b<!cc> body; b end` (body is a forward jump target). One extra `b`.

The two forms are logically identical; only the source shape decides whether
the body falls through (positive-AND) or is jumped into (OR-early-return). To
match, read the target's last guard clause: a single fall-through branch →
write positive-AND; a `b<cc> body; b end` pair → write OR-early-return. This
is a *byte-count* difference (the extra `b`), so it shows up as a base_size
mismatch, not just a coloring nit.

This is distinct from the `!predicate()` entry (single negation flips
bne↔beq, same instruction count) and the predicate-OR accumulator entry
(boolean *return value*, not a code block) — here it's the whole multi-clause
guard + trailing block that picks the layout.

**Citations.**
- `Enemy/BathtubKiller::generateItemBathtubKiller` (tick 295): 94.09 →
  99.12%. Target used the single-fall-through form; our OR-early-return
  emitted the extra `beq body; b end` (656B vs 652B). Switching to positive-AND
  `if (item != nullptr && item->mActorType == 0x20000002) { body }` matched
  the size exactly.
- `MoveBG/MapObjTree::touchPlayer` (tick 295): 97.61 → 99.86%. Opposite
  direction — target used the two-branch form (`blt body; b end`); our
  positive-AND `if (mActor==this && idx>=0 && idx<count) { body }` gave the
  single fall-through (192B vs 196B). Switching to OR-early-return
  `if (mActor!=this || idx<0 || idx>=count) return; body;` reproduced the
  two-branch exactly.

### `volatile` defeats const-fold so a "pick random value in [min,max)" expression homes min/max to the stack and holds `range = max-min` in a callee-saved reg across `rand()`

**Confirmed across many TUs (int and float forms).** When the target compiles a
random-pick-in-range as *store literal min/max to stack → reload → compute
`(max-min)` via a runtime `subf`/`fsubs` into a callee-saved register held
across the `rand()` call → `min + range*scaled_rand`*, reproduce it by making
the min/max operands `volatile` and evaluating `range = max - min` **before**
the `rand()` call (an explicit named temp, not inline `(max-min)*rand`).

**Two distinct sub-cases — pick the right lever:**
- **Runtime min/max** (read from params/getters/members): no `volatile` needed
  — the values already come from memory, so they aren't const-folded. The only
  fix is **evaluation order**: compute `range` before `rand()`. The shared
  `MsRandF(f32 l, f32 r)` helper (`include/MarioUtil/RandomUtil.hpp`) was
  written `rand()*c*(r-l)+l`, which schedules `rand()` first; rewriting it
  `f32 range = r-l; return l + range*(rand()*c);` made all runtime-arg callers
  home `range` into `f31` across the call. One helper edit lifted `Enemy/telesa`
  init/reset/initAttacker 86→90-91, `Enemy/smallEnemy` 92.9→93.1, plus
  `mameGesso`/`conductor`, with zero TU regressions.
- **Literal min/max** (`MsRandF(0.0f, 360.0f)` etc.): the literals const-fold
  through the helper's param copies even after the reorder, so the helper can't
  help. Apply `volatile` locals **at the call site** and expand the pick inline:
  `volatile f32 mn=A; volatile f32 mx=B; f32 range=mx-mn; <use> mn+range*MsRandF();`.
  This is the only working form for literal sites (see Refuted alternative).

Two levers, both required:
1. **`volatile`** defeats MWCC's constant-folding — otherwise `(max-min)` folds
   to a literal and there are no stores/reloads/subf at all.
2. **An explicit `range` temp evaluated before the call** keeps the subf in a
   callee-saved register across `rand()` (MWCC otherwise schedules `bl rand`
   first and recomputes after).

Citations:
- **int form** — `Enemy/igaiga` `reset__7TIgaiga` 74.7% → 99.6% via
  `volatile int min/max; int range = max-min; (min+(int)(range*MsRandF()))*120`.
  Byte-identical instruction stream.
- **float form** — the fabricated `FakeRandInterval{volatile f32 mMin,mMax;}`
  helper (`include/Enemy/WalkerEnemy.hpp`), whose `get()` does
  `f32 r1 = mMax - mMin; ... return mMin + r1*rand`: `Enemy/igaiga`
  `reset__10TRollEnemy` 63.1% → 73.1% and `Enemy/gesso` `setPolluteGoal`
  89.75% → 91.84%. Making the two members `volatile` flipped both from the
  folded form to the target's store/reload/`fsubs f31` shape.
- **int-range, literal `low`/`high`** — `Enemy/elecNokonoko`
  `init__13TElecNokonoko` 83.3% → 99.4% and `loadInit__13TElecCarapace`
  83.7% → 94.4% via `volatile int low/high; int range = high-low;
  low + (int)(MsRandF()*range)` (3rd TU confirming the int form; even
  `low==0` gets homed).
- **float-range, literal call sites (`volatile` locals)** — `Enemy/effectObj`
  `TEffectModel::reset` 74→96 (cascades through 4 derived resets that inline it:
  ColumWater/BombColumWater 82.7→97.7, ColumSand 79.4→95.9, Explosion 79.8→97.3);
  `Enemy/namekuri` reset 77→92; `Enemy/bombhei` setDeadAnm 72→95;
  `Enemy/killer` reset 79→84; `Enemy/hamukuri` `TDangoHamuKuri::reset` 70→96
  (cascades to BossDango 72→96); `Enemy/smallEnemy` generateItem 60→63 and
  `genEventCoin` 91.6→97.9 (literal range as the middle arg to
  `TVec3::set`; residual is fused `fmadds`/FPR coloring); `Enemy/chuuhana`
  `setGoal` 67.9→84.3 (same volatile endpoints plus split
  multiply-then-add form).
- **float-range, runtime call sites (helper reorder)** — see the runtime
  sub-case above (`telesa`, `smallEnemy::init`, `mameGesso`, `conductor`).

Residuals after the lever are register coloring / frame pad only.

**Refuted alternative:** a fabricated `inline int MsRand(int,int)` helper — MWCC
inlines and const-folds the literal args through the param copies (even under
`-inline deferred`), reproducing the folded form. The homing is a
const-fold-inhibition + evaluation-order artifact, not an inlined-param one.

### `#pragma dont_inline` around a single-call-site callee restores the target `bl` under `-inline deferred`

In `-inline deferred` TUs, MWCC can inline an ordinary out-of-line member
function or constructor into its only call site while still emitting the
standalone symbol. If the target keeps that call as a `bl`, wrap the callee
definition in `#pragma dont_inline on` / `#pragma dont_inline off`. This
suppresses the call-site inline without removing the standalone copy. Apply
only after confirming the target really calls the same emitted callee and
re-check the callee itself for regressions. Citations: `Enemy/limitkoopa`
`TLimitKoopa::startHipDrop` into `TNerveLimitKoopaHipDropStart::execute`
(0% -> 88%); `Enemy/elecNokonoko` `TElecCarapace` ctor at the `new` site in
`TElecNokonoko::init` (61% -> 81.7%); `Enemy/pakkun` `TPakkun` ctor at the
`new` site in `TPakkunManager::createEnemyInstance` (25.8% -> 100%, ctor
stayed 100%).

### Static inline wrapper around `MsWrap<f32>` can force the target's local out-of-line template emission

When a target TU emits a local `MsWrap<f>__Ffff` body and calls it, but a
direct `MsWrap<f32>(...)` source call is fully inlined, route the call through
a tiny file-local wrapper:

```cpp
static inline f32 callMsWrap(f32 t, f32 l, f32 r)
{
	return MsWrap<f32>(t, l, r);
}
```

Under `-inline deferred`, MWCC inlines the wrapper body only far enough to
preserve the template call boundary, causing the local `MsWrap<f>` instance to
be emitted and called. Use the opposite settled rule ("manual expansion of
`MsWrap` while loops") when the target inlines the loops and our build emits
the call. Citations: `Enemy/rocket` and `MoveBG/MapObjItem2` both use this
wrapper with a 100% `MsWrap<f>` symbol; `Animal/boid::calcBoids` changed from
direct inline to this wrapper and gained an exact local `MsWrap<f>` while
lifting `calcBoids` 62.2% -> 65.3% (t225); `Animal/AnimalBase` routed the
five `execWalk` wrap sites and one `getRotationFlyToDir` site through the
wrapper, emitted the exact local `MsWrap<f>` owner, and moved `execWalk`
37.0% -> 58.5% plus `getRotationFlyToDir` 75.6% -> 86.7%.

### objdiff constant-pool label-numbering floor (a near-match can be byte-identical)

MWCC assigns anonymous local symbols (`@NNNN` for float/const-pool entries) from an
**object-global, order-dependent counter**. Two objects that emit the *same* constant
(same value, section, order, offset) get *different* `@` numbers if the rest of the TU
differs in size (target reached `@33xx`; our smaller-TU build reaches `@4xx`). objdiff
aligns data by symbol name, so it cannot equate `@3347`==`@449` and scores each
constant-pool reference as DIFF_ARG_MISMATCH — even though `dtk elf disasm` shows the
**instruction bytes are byte-identical**. A function whose only residue is this is at
the matching floor; there is no source-level lever (you'd have to compile the whole
original TU ahead of it to advance the counter). Diagnostic: disasm both
`build/GMSJ01/obj/<TU>.o` and `build/GMSJ01/src/<TU>.o`; if bytes match and only the
`@NNNN` numbers differ, treat the function as done and stop. Cited: limitkoopa
`load__18TLimitKoopaManagerFR20JSUMemoryInputStream` (99.83%) and
`loadAfter__18TLimitKoopaManagerFv` (99.93%), t206. (Distinct from a genuine
leading-`.rodata` byte shift, which CAN be fixed if the missing constant is identified.)


### Routing a field read through its own inline accessor inhibits CSE and forces the target's field *reload*

**Rule.** When the target loads a field, tests it, then *reloads the
same field* to use/modify it (no intervening write), our direct-access
build CSE's the two reads into a single register reuse. Routing **at
least one** of the reads through its own inline accessor presents that
read as a fresh expression at a distinct inline boundary, which MWCC's
CSE will not merge with the other read — reproducing the reload.

```cpp
// target reloads 0x1a4 twice; direct access CSE's to one load (WRONG):
if (unk1A4 > 0)
    unk1A4 -= 1;

// route the compare's read through an inline getter → forces the reload (RIGHT):
s32 getUnk1A4() const { return unk1A4; }   // in the header
...
if (getUnk1A4() > 0)   // load #1 via accessor boundary
    unk1A4 -= 1;       // load #2 direct, NOT CSE'd with #1
```

It is **not** necessary for both reads to go through accessors — one
accessor + one direct read-modify-write is enough to break the CSE
(Kukku). Two distinct accessors (one read, one write) also works
(wireTrap). The mechanism is the inline-expansion boundary, not the
accessor count.

**Symptom that signals this lever.** Target shows two `lwz rX, OFF(rThis)`
of the same field straddling a compare/branch with no store between
them; our build shows a single `lwz` whose register is reused.

**Why.** Each inline accessor presents the field load as a fresh
expression at its own inline boundary; MWCC's CSE doesn't merge loads
across distinct inline expansions even though both read the same
address with no intervening write.

**Citations (2 TUs).**
- `Enemy/wireTrap::kill` (t186): direct `mLiveFlag & 1` / `mLiveFlag |= 1`
  was 92.06% (single load). Switching to `checkLiveFlag(1)` (read accessor)
  + `onLiveFlag(1)` (write accessor) → 96.91%, size-exact.
- `Enemy/Kukku::control` (t190): direct `if (unk1A4 > 0) unk1A4 -= 1;`
  was 92.14% (single load). Routing only the compare read through inline
  `getUnk1A4()` (decrement stays a direct RMW) → **100%**, byte-exact.

### `spine->pushAfterCurrent(nerve)` vs `spine->pushNerve(nerve)` for "transition next-nerve" patterns

**Rule.** When a `DEFINE_NERVE` body ends with "push a successor nerve
and return TRUE", target asm typically shows ONLY the inlined
`mVertebrae.push(nerve)` (writing the new nerve to the spine's stack
array and bumping mSize), NOT the full `pushNerve` body which also
writes `mPrevious = mCurrent` and `mCurrent = nerve`. The source for
this pattern is `spine->pushAfterCurrent(nerve)` — NOT `pushNerve(nerve)`
which would emit the additional mPrevious/mCurrent stores.

```
target asm (pushAfterCurrent):
li r4, instance$NerveX@sda21
cmplwi r4, 0
beq SKIP
lwz r5, 0x8(r31)         ; mVertebrae.mSize
lwz r0, 0x4(r31)         ; mVertebrae.mCapacity
cmpw r5, r0
bge SKIP
lwz r3, 0xc(r31)         ; mVertebrae.mData
slwi r0, r5, 2
stwx r4, r3, r0          ; mData[mSize] = nerve  <- writes new nerve
lwz r3, 0x8(r31)
addi r0, r3, 0x1
stw r0, 0x8(r31)         ; mSize++
SKIP:
li r3, 0x1               ; return TRUE
```

`pushNerve` additionally writes `mPrevious = mCurrent` (`stw r0,
0x1c(r31)`) and updates `mCurrent = nerve` (`stw r5, 0x14(r31)`) and
`mTime = 0` (`stw r0, 0x20(r31)`), and pushes `mCurrent` (NOT nerve)
to vertebrae. Distinct semantics — pushNerve = "save current, become
nerve"; pushAfterCurrent = "queue nerve to run after current finishes".

**When this lever moves the match-%.** Any "nerve.execute() returning
TRUE after queueing a successor" pattern in a DEFINE_NERVE body. The
nerve transition macro at end of execute() bodies is the common case.

**Citations (t184 yunbo).**
- TNerveYumboFreeze::execute: 88.46% → 99.55% (single line swap).
- TNerveYumboHiding::execute: 92.04% → 99.92%.
- TNerveYumboAppearing::execute: 93.79% → 99.81%.
- TNerveYumboDancing::execute: 76.46% → 81.36%.

### Inverted ternary / bool-materialize for null-first or false-first branch ordering

**Rule.** When MWCC's target asm shows the *null* (or *false*) value
emitted BEFORE the *load* (or *true*) value in a ternary / bool
materialize sequence — typically:

```
fcmpo cr0, fA, fK ; or cmplwi r3, 0
[cror...]         ; combined condition
bne   LABEL_VALUE ; jump if condition holds (skip the null path)
li    rARG, 0     ; null/false branch (fallthrough)
b     LABEL_END
LABEL_VALUE:
[load or li 1]    ; value branch (lives at the goto landing)
LABEL_END:
```

— invert the source's condition so the negative branch is the
fall-through:

```cpp
// inline ternary version
self->setAnmSound(!table ? nullptr : table[N]);

// explicit bool version
bool flag;
if (absDir <= K)
    flag = false;
else
    flag = true;
```

NOT the natural form `cond ? value : null` or `if (cond) flag=true
else flag=false` — that produces the swapped path order.

**Why.** MWCC emits the "if" arm as the fall-through and the "else"
arm at the goto landing. The natural form puts value-branch as the
fall-through; the inverted form puts null/false-branch there.

**When this lever moves the match-%.** Anywhere a small ternary or
bool-materialize is in a function and the diff shows your build's
branch with the WRONG path falling-through. Often signaled by the
`li rARG, 0` (or `li r0, 0`) appearing AFTER the value-load in your
build but BEFORE it in target.

**Citations (t180 limitkoopajr).**
- Run nerve execute: 83% → 97.54% (ternary form for setAnmSound,
  plus explicit bool materialize for `turned`).
- Wait nerve execute: 0% → 81.88% (same patterns; stack delta still
  cascades the residual).
- Launch nerve execute: 93.18% → 100%.
- Yahoo nerve execute: 92.40% → 99.90%.

Memory entry: `state/memory/feedback_inverted_ternary_for_null_first_branch.md`.

### Init-list field-zero stores for pre-member-ctor positioning

**Rule.** When target asm shows a field-assign (typically `li r0, 0;
stw r0, OFFSET(rThis)`) emitted *between* the vtable-stores and the
first `bl __ct__memberType` of a non-POD member, the source must put
that field in the initializer list, NOT the body. C++ runs init-list
in declaration order BEFORE all member-subobject ctors; body runs
AFTER all member ctors. MWCC follows this faithfully.

```cpp
TFoo::TFoo(const char* name)
    : TBase(name)
    , mTargetActor(nullptr)   // <-- init list, emits between vt-stores and member ctors
{
    mFlag |= 0x10;            // body, emits AFTER all member ctors
}
```

vs the wrong (body-only) form:

```cpp
TFoo::TFoo(const char* name)
    : TBase(name)
{
    mTargetActor = nullptr;   // body, emits AFTER mDirection1/mDirection2 ctors
    mFlag |= 0x10;
}
```

**When this lever moves the match-%.** When asm shows the zero-store
sandwiched between vtable setup (e.g. `stw vt, 0(r31); addi vt+0x24;
stw vt+0x24, 0x20(r31)`) and the first `bl __ct__memberType`.

**Citations (t180 limitkoopajr).**
- ctor 80.94% → 100% (single-line move of `mTargetActor=nullptr`).
  Sufficient on its own; not paired with any other change.

Memory entry: `state/memory/feedback_initlist_for_field_zero_init.md`.

### `<MSound/MSSetSound.hpp>` + `<MSound/MSoundBGM.hpp>` produce the canonical 15-JALList __sinit shape

**Rule.** When a TU's target `__sinit_<TU>_cpp` registers the standard
15 JALList<T>::smList templates (MSBgm, MSSetSoundGrp, MSSetSound +
12 JALSeMod* variants — Eff/Pit/Vol × DGrp/FGrp/Dist/Funk), include
**both** of these headers in the source:

```cpp
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
```

MSoundBGM.hpp instantiates JALList<MSBgm>::smList; MSSetSound.hpp (via
JALModSe.hpp) brings the 12 JALSeMod classes. MWCC then emits the
matching `__sinit_<TU>_cpp` of 764B (or larger if more templates are
referenced via other paths). The sinit registers each template's smList
via `initiate__10JSUPtrListFv` + `__register_global_object` and stores
a guard byte in `__init__smList__*` @sda21.

**When this lever moves the match-%.** Only when our build's __sinit is
either entirely absent or smaller than target's. If the TU already
includes MSoundBGM.hpp (e.g. transitively via Player/MarioMain.hpp) and
the sinit already matches, adding MSSetSound.hpp is a no-op. Always
verify R sinit size < L sinit size via objdiff before committing.

**Citations (t175).**
- Empty stubs: `Map/PollutionEvent` 13.5 -> 100, `MSound/MSoundDebug`
  0 -> 100 (the latter also needs `template class JADPrm<u8>;` for the
  8B weak ctor emit).
- Partial sinit grew to full 764B: `MSound/MSHandle` 69.6 -> 96.1,
  `Player/SplashManager` 9.6 -> 37.3, `Player/MarioEffect` 5 -> 29.2,
  `MoveBG/MapObjCorona` 1.7 -> 5.7, `MoveBG/MapObjMare` 85 -> 90.1
  (sinit grew 32 -> 788).
- Smaller gains where sinit was 716B -> 764B (missing 1 template):
  `Camera/CameraChange` 75.7 -> 76.4, `NPC/NpcBase` 75.5 -> 75.9,
  `Player/Tongue` 77.8 -> 78.5, `Player/MarioParticle` 24.7 -> 25.2,
  `NPC/NpcAnm` 48.2 -> 48.5.
- No-op TUs (already had MSoundBGM.hpp): MarioMain, MarioSound, Yoshi,
  MapWireManager. Adding MSSetSound did not change sinit size.

### Infectious `dummy1431[3]={1,1,1}; dummy1411[3]={1,1,1}; dummy1210[4]={0,2,1,3}` fixes `.data` layout when target emits @1431/@1411/@1210

**Rule.** When target's `.data` section starts with the anonymous-compound
sequence `@1431, @1411, @1210` (12+12+16 = 0x28 bytes) before any
TU-specific data symbol like `__vt`, declare three matching dummies at
file scope BEFORE any other `.data` emitter (other static structs,
infectious-strings include, etc.):

```cpp
static f32 dummy1431[3] = { 1.0f, 1.0f, 1.0f };
static f32 dummy1411[3] = { 1.0f, 1.0f, 1.0f };
static u32 dummy1210[4] = { 0, 2, 1, 3 };
```

The pattern is already established in `src/MarioUtil/MathUtil.cpp`,
`src/System/EmitterViewObj.cpp`, `src/System/RenderModeObj.cpp`,
`src/MSound/MSModBgm.cpp`, plus the function-local variant in
`src/Enemy/graph.cpp` (`static Vec v1={1,1,1};` inside a `dummy()` fn).

**Why it works.** These three constants appear in many TUs as "ghost
data" — emitted by some header inline (likely J3DMtxCalcBasic/Maya/
Softimage::init with `(Vec){1,1,1}`) but never referenced. They land
at `.data` offset 0..0x27, shifting subsequent `.data` symbols
(particularly `__vt__*`) to offset 0x28. When a function uses
`lis/addi @1431` as a base pointer and computes `addi r,r,0x28` to
reach the vtable, our build *must* have the vtable at the same offset
for the encoding to resolve to the same address.

**When it actually moves the match-%.** Only when:
(a) target's `.data` has `@1431/@1411/@1210` at offset 0
(b) our TU has *no other* `.data` content before the symbol the function
    references via @1431-base addressing
(c) the function uses the `@1431` base-pointer optimization (look for
    `lis r,@1431@ha; addi r,r,@1431@l` followed by `addi r,base,0xN`
    where `0xN >= 0x28`).

For TUs with lots of `.data` (hinokuri2, bgtentacle, MarioRun), the
relative offset between @1431 and the target symbol is already wrong
and adding the dummies doesn't change MWCC's chosen base-pointer.

**Citations.**
- `Enemy/feetinv` (t172): ctor 93.39 → 100.00%, dtor 88.49 → 100.00%.
  TU 70.5 → 71.8%. Adding dummies put `__vt__15TMtxCalcFootInv` at
  `.data:0x28`, exactly matching target.
- `MSound/MSHandle` (t176): MSACos 92.96 → 100.00% via shifted
  `smACosPrm` base. The function originally used `lfsx r0, r3, r0`
  (indexed load from offset 0); after dummies, the array sat at
  `+0x28` and the indexing form became `add r3,r3,r0; lfs r0,0x28(r3)`
  — matching target exactly. TU 96.1 → 96.37%.
- `Enemy/hinokuri2` (t176): one additional function flipped to 100%,
  +192B matched_code (TU 22.31 → 23.12%). The previous "refuted"
  status came from a pre-t175 build state; after the MSound include
  sweep grew the sinit/data, dummy1431 produced an incremental gain.
- `GC2D/Option` (t176): `.data` 82.76 → 97.48%. No function flipped
  to 100%, but data accuracy improved (small step toward TU completeness).
- Pre-existing in `MarioUtil/MathUtil`, `System/EmitterViewObj`,
  `System/RenderModeObj`, `MSound/MSModBgm`, `Enemy/graph` (variant).

**Refuted application targets (t176 sweep).**
- `MoveBG/Item`, `Enemy/bgtentacle`, `Player/MarioRun`,
  `Player/MarioDraw`: adding dummies caused no change in `.text` or
  `.data` match%. In MarioDraw's case, the dummies already match
  target (the first 4576B of `.data` are MATCH); in others, the
  base-pointer reference doesn't depend on the dummy offset. Verify
  by inspecting `data_diff[0]` shape: `DIFF_DELETE size=40` (or
  cleanly larger DELETE starting with the dummy hex) is the right
  signal; `MATCH` at start means dummies are already implicit.
- `GC2D/SelectMenu` (t176): `data_diff[0]=DIFF_DELETE size=180`
  contains the dummies but is followed by 140B of additional
  TU-specific data we're missing; dummies alone won't close it.

### Friend `operator*(TVec3, f32)` / `operator-(TVec3, const TVec3&)` keeps scale/sub as `bl` calls

**Rule.** When target shows `bl scale__TVec3Ff` or `bl sub__TVec3FRC...`
but our build inlines (`fmuls x3` or `fsubs x3`), rewrite the source
expression to use the **fabricated friend operator** form instead of a
direct method call:

```cpp
// What we wrote (compiler inlines scale -> 3x fmuls):
JGeometry::TVec3<f32> tmp(headDir);
tmp.scale(mInitialSpeed);
mInitialVelocity = tmp;

// What target wants (friend op*, emits bl scale):
mInitialVelocity = headDir * mInitialSpeed;
```

Likewise for `dir = a - b` (friend op-, emits `bl sub`) vs
`dir(a); dir.sub(b)` (inlines to fsubs). And `dst = a + b` for `bl add`.

**Why it works.** The `friend TVec3 operator*(TVec3 fst, f32 snd)`
takes its TVec3 parameter **by value**. The by-value param is what
operator*= modifies, and MWCC has to materialize `fst` on the caller's
stack, pass `&fst` as `this` to scale, then copy the result. Because
the friend body is small but its by-value parameter forces a stack
materialization, MWCC keeps `scale` as a `bl` call rather than full
inlining. The friend `operator-` returning `const TVec3&` does the same
for `sub`.

**Side-effect.** The friend op* return-by-value also forces an extra
TVec3 copy at the call site (since MWCC's NRVO doesn't fully elide a
by-value param return). This costs ~7 instructions per use vs target's
direct assign. Acceptable for big match gains; document the leftover.

**Citations.**
- `GC2D/SelectShine2::TSelectShine::move` (t343): replacing
  `world = mPos; world.add(unk18);` with `world = mPos + unk18` restored the
  target `bl add__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>` call boundary
  and moved the function `83.3% -> 87.8%`. Remaining residue is the already
  documented `0.0f * expr` folding in the spline block, stack shape, and an
  extra emitted weak `TVec3<f32>::add` body.
- `Player/MarioParticle::TWarpInCallBack::execute` (t323): clean
  `tmp.scale(k)` source inlined all three scales and scored 0.0%. Rewriting
  the three vector multiplies as friend `operator*(TVec3, f32)` expressions,
  then naming the scalar/vector-result locals to match target lifetimes, forced
  all three `bl scale__Q29JGeometry8TVec3<f>Ff` calls and lifted the callback
  to 60.0%. Remaining residue is stack/copy scheduling and random-scale
  conversion placement, not the scale-call decision.
- `Player/Tongue::emit` (t170): 57.43% → 86.98% (+29.55pp). Replaced
  `tmp.scale(speed); mInitialVelocity = tmp;` with
  `mInitialVelocity = headDir * mInitialSpeed;`. Forced bl scale x2.
- `Player/Tongue::movement` state 1 (t170): scale/add/sub all converted
  via friend ops. TU 37.75% → 50.83% (+13.1pp).
- `Player/Tongue::movement` state 3 (t170): `diff = mTipPos - mHeadPos;
  scaled = diff * retract; mTipPos = mHeadPos + scaled;` form.
- `Player/Tongue::canGo` (pre-t170): friend op- pattern emits bl sub.

### Predicate-OR accumulator: rewrite `if (A() || B()) return;` as `bool b=true; if (!A() && !B()) b=false; if (b) return;` to force the init-true reverse-set accumulator shape

**Rule.** When `if (A() || B()) <early-return>` mismatches target with target
showing the **accumulator shape** (one init-to-1 register, both predicates
inline, two `bne merge` jumps, set-to-0 only on fall-through, single
`bne end` at merge), the source-level lever is the explicit-bool reverse-set
rewrite:

```cpp
// What target compiles - accumulator shape:
//   li r3, 0x1                 ; blocked = 1
//   addi r5, r3, 0x0           ; first predicate result starts at 1
//   ... inline A, set r5 = 0 if A false ...
//   bne merge                  ; A true -> keep blocked = 1, skip B
//   ... inline B, set r0 = 0 if B false ...
//   bne merge                  ; B true -> keep blocked = 1, skip set
//   li r3, 0x0                 ; both false -> blocked = 0
//   merge:
//   clrlwi. r0, r3, 24
//   bne end                    ; blocked == 1 -> return

// What we want in source - reverse-set form:
TMarDirector* dir = gpMarDirector;
bool blocked = true;
if (!dir->isTalkModeNow() && !dir->checkUnk124Thing2())
    blocked = false;
if (blocked) return;
```

**Why MWCC picks the accumulator vs early-return shape.**
`if (A() || B()) return;` compiles to the short-circuit early-return
("bne end" twice) form — each predicate test directly jumps to function
end. The accumulator shape only emerges when source explicitly has a
bool variable that's initialized then conditionally cleared. The `&&`
nested-not form provides the two skip jumps to a merge point, and the
trailing `if (b)` provides the merged accumulator test.

**Caveat: don't introduce intermediate per-predicate bools.**
`bool talking = A(); bool blocked = talking || B();` regresses — MWCC
emits `bl B()` (no inline) when `talking` is materialized as a separate
local. Keep the predicates as part of the same `&&` test inside the
reverse-set branch.

**When to apply.** Diff shows our build emitting the short-circuit
early-return shape (`bne end / bne end`) where target shows the
accumulator shape (`li r3, 0x1; ...; bne merge; ...; bne merge; li r3, 0x0; merge: clrlwi.; bne end`).

**Citations.**
- `NPC/NpcCoin::updateCoin` (tick 168): 94.40% → 97.31% (+2.91pp).
- `NPC/NpcCoin::requestAppearCoin` (tick 168): 96.03% → 98.24% (+2.21pp).
- `NPC/NpcWalkTurn::execWalk` (tick 168): 78.20% → 79.06% (+0.86pp;
  second predicate is `unk124 == 4`, not a function call — accumulator
  shape applies regardless).
- `Enemy/bossgesso::doAttackSingle` (tick 168): 36.08% → 36.63% (+0.55pp;
  TU has many other mismatches, predicate-OR is small relative).
- `Enemy/Kukku::behaveToWater` (tick 194): 72.10% → 95.30% (+23.2pp).
  Here the **direct** form `bool b1 = (cur == X::theNerve() || cur ==
  Y::theNerve()); if (b1) return;` (NOT the reverse-set rewrite) was
  enough to produce the accumulator shape — for a two-term pointer-equality
  disjunction the plain assignment materializes; `if (A || B) return;`
  branches directly. **Bonus mechanism:** with the bool materialized, the
  **first** operand's `theNerve()` Meyers-singleton inlines but the
  **second** operand's becomes a `bl theNerve__...Fv` call (it sits behind
  the short-circuit `beq`). This exactly reproduced the target's
  `bl theNerve__19TNerveKukkuPostFallFv`. Remaining 5% is pure TU-global
  static-guard symbol numbering (`instance$N`/`init$N`), not structural.

### `!predicate()` source-level negation matches target's `bne` skip-on-true branch where our `if (predicate())` produces `beq` skip-on-false

**Rule.** When target's compiled if uses `bne <end>` after an inlined
predicate (skip-on-true) and ours emits `beq <end>` (skip-on-false), the
source-level lever is to add `!` to negate at the call site. The
inlined predicate body is identical between target and ours — only the
**conditional branch opcode at the test site** differs. The source's
condition direction (`if (X)` vs `if (!X)`) is the only thing that
flips bne ↔ beq for the same predicate.

```cpp
// Target asm shows skip-on-true (bne):
//   ... predicate inline body ...
//   clrlwi. r0, r4, 24
//   bne 0x<end>                 ; predicate TRUE → skip body
//   <body>
//   end:

// Source must use negation:
if (!gpMarDirector->isTalkModeNow()) {
    <body>
}
```

The function predicate's semantics (which input values produce TRUE)
are determined by the inline body's shape (init=1 vs init=0,
set-false-on-default vs set-true-on-match). Once the body is settled,
all that's left at call sites is whether the source applies `!`.

**When to apply.** When the inline predicate body is identical in
target and ours but the **branch opcode immediately after** is
different (target `bne`, ours `beq`, or vice versa).

**Citations.**
- `Enemy/tamaNoko::TFlower::perform` (tick 168): three call sites with
  `bne` in target vs `beq` in ours, fixed by adding `!` at lines 76, 78,
  and on `checkUnk124Thing2` at line 141 (kept `isTalkModeNow()` positive
  at 141 — target used `beq` there). Lift 93.85% → 93.91%; small because
  remaining diffs are stack/coloring.

### `*(Vec*)&dst = *(Vec*)&src;` is the source-level lever for forcing inlined `lwz/stw` TVec3 op= when MWCC fails to inline the call

**Rule.** When the target's asm shows a `JGeometry::TVec3<f32>` field
being copied via direct `lwz/stw` instructions (three pairs), but our
build emits `bl __as__Q29JGeometry8TVec3<f>FRCQ29JGeometry8TVec3<f>`
(an out-of-line operator= call), the lever is to write the assignment
through an explicit Vec base-class bitwise cast:

```cpp
// What we want — target has the inline form:
//   lwz r5, 0x10(r_src) ; lwz r0, 0x14(r_src)
//   stw r5, 0xX0(r_dst) ; stw r0, 0xX4(r_dst)
//   lwz r0, 0x18(r_src)
//   stw r0, 0xX8(r_dst)
//
// In source — both work as Vec assignment but only one inlines:

mPosition = other.mPosition;                       // emits `bl __as__...`
*(Vec*)&mPosition = *(Vec*)&other.mPosition;       // emits the inline lwz/stw

// Equivalent through pointer:
*(Vec*)&this->mLoadRot = getFocalPoint();          // for sret-returning calls
```

The TVec3<f32> operator= body is
`*(Vec*)this = *(Vec*)&other;` (integer copy of the Vec base subobject).
Writing the cast manually at the call site removes the operator= call
entirely — MWCC sees a plain `Vec = Vec` C-struct assignment and emits
the three lwz/stw pairs directly.

**Why MWCC sometimes fails to inline operator=.**
Suspected cause: `#pragma dont_inline on` at file scope inhibits all
inlining, including header-defined template-member inline bodies.
The bitwise cast doesn't need inlining to work — it's already a
plain assignment expression at parse time.

**When to apply.** Diff shows `bl __as__Q29JGeometry8TVec3<f>...`
where target has three `lwz`/`stw` pairs (six instructions total) for
a Vec-sized copy.

**Citations.**
- `NPC/NpcBase::setDummyConnectActor` (tick 166): two TVec3 op= calls
  (`mPosition = ...; mRotation = ...;`) → bl __as__ x2. Replacing
  with `*(Vec*)&dst = *(Vec*)&src` → 14.35% → 100%.
- `NPC/NpcBase::load` (tick 166): three TVec3 op= calls
  (mResetPos, mResetRot, mEffectScaleBase) → 42.96% → 100%.
- `NPC/NpcBase::perform` (tick 166): the inner mDummyConnectActor
  copy block; cast pushed perform 58.79% → 61.57%.

### TVec3<f32> fields declared in a class with `#pragma dont_inline on` at file scope generate `bl __ct__Q29JGeometry8TVec3<f>Fv` calls per field at construction

**Rule.** Each `JGeometry::TVec3<f32>` member-by-value field in a class
declaration triggers a constructor call in that class's ctor — even
though the default constructor is empty (`TVec3() { }`). When the
containing .cpp has `#pragma dont_inline on` at file scope, MWCC fails
to inline the empty body and emits one `bl __ct__Q29JGeometry8TVec3<f>Fv`
per TVec3 field at the start of the user's ctor body.

Target's NpcBase __ct has **zero** such calls; our build emitted
**five** (one per TVec3 field: mResetPos, mEffectScaleBase,
mNoteEffectPos, mSmokeEffectPos, mWaterEffectPos). Adding more
TVec3 fields (e.g. converting `char unk1A0[0x4]; f32 unk1A4; char
unk1A8[0x4]` into a single `TVec3<f32>`) ADDS more ctor calls.

**Workaround.** If the field is only used for raw Vec-sized copy and
component access, keep it as the original scalar layout (e.g.
`char unk1A0[0x4]; f32 unk1A4; char unk1A8[0x4]` for a Vec at
offset 0x1A0). Then use `*(Vec*)&unk1A0 = *(Vec*)&otherVec;` for the
copy — see the rule above.

**Open.** A reliable source-level lever to force MWCC to inline the
empty TVec3 default ctor under `#pragma dont_inline on` is not yet
known. `dont_inline off`, `dont_inline reset`,
`inline_max_total_size`, and explicit `inline` keyword have all been
attempted with no effect. Removing the file-level pragma works but
breaks other empty-function-preservation requirements in the same TU.

**Citations.**
- `NPC/NpcBase::__ct` (tick 166): converting `char unk1A0[0x4]; f32
  unk1A4; char unk1A8[0x4]` (3 scalars at 0x1A0/0x1A4/0x1A8) into a
  single `JGeometry::TVec3<f32> mResetRot` field added one `bl
  __ct__Q29JGeometry8TVec3<f>Fv` to the ctor. Reverting to scalars
  removed the call.

### `if (k == K1 || k == K2) A; else if (k == K3) B;` → use a `switch` with case fallthrough when target emits the switch-with-branching tree

**Rule.** When two `case` values share a body (`case K1: case K2:`) and a
third has its own, the target asm often uses MWCC's switch-with-branching
pattern: two `cmpwi`/`beq` instructions plus a `bge` to skip past a
"fallout" branch. An equivalent if-chain (`if (k==K1 || k==K2) ...; else
if (k==K3) ...`) compiles to two independent `cmpwi`/`beq` checks back to
back, with no `bge` skip. The shapes are not the same.

```
# Target (switch with case 5, case 0xE share, case 7 own):
cmpwi r0, 0x7
beq L_seven         # k == 7
bge L_done          # k > 7  → no match (skip past five/e branches)
cmpwi r0, 0x5
beq L_five_or_e     # k == 5
b L_done            # k in {0..4, 6} → no match
cmpwi r0, 0xe
beq L_five_or_e     # k == 0xE
b L_done            # k in {8..0xD, 0xF..} → no match

# If-chain compiles to:
cmpwi r0, 0x5
beq L_a
cmpwi r0, 0xe
beq L_a
cmpwi r0, 0x7
bne L_done
L_b: ...
L_a: ...
```

**When to apply.** If the target has the `cmpwi K_high; beq; bge; cmpwi
K_low; beq; b; cmpwi K_mid; beq; b` tree, write the source as a switch
with fallthrough cases. If the target has a flat `cmpwi; beq; cmpwi;
beq; cmpwi; bne` chain, write it as if/else-if.

**Citations.**
- `NPC/NpcAnm::npcMareStanding` (tick 164): `if (k == 5 || k == 0xE) A; else if (k == 7) B;` → 75.61% with the if-chain. Rewriting as `switch(k) { case 5: case 0xE: A; break; case 7: B; break; }` → 83.72% (+8.1pp). The third comparison was lifted into the binary-search tree shape.
- `NPC/NpcAnm::npcMareStandIn` (tick 164): same `k == 5 || k == 0xE` paired with a `default` branch. Switch form → 85.88 → 88.55%.

### `if (cond1) K1; else if (cond2 && !cond3) K2; else K3;` vs `if (cond1) K1; else if (!cond2 || cond3) K2; else K3;` — pick the form that puts the fall-through value where target's fall-through lands

**Rule.** When an if-elseif-else assigns three constants to the same
variable, MWCC compiles the chain such that exactly one branch is the
"fall-through" (no `b end` before the assignment). The other branches
each end with `b L_end`. The asm tells you which constant is the
fall-through: target's `li rN, <K>` not followed by `b L_end` is the
fall-through, and that's where the source's `else { var = K; }` should
land.

```cpp
// Source form A — kind = 6 is fall-through (no b end)
if (mActionFlag & 0x400)                                kind = 1;
else if ((mActionFlag & 0x1) && !(mActionFlag & 0x4))   kind = 0x13;
else                                                    kind = 6;

// Source form B — kind = 0x13 is fall-through
if (mActionFlag & 0x400)                            kind = 1;
else if (!(mActionFlag & 0x1) || (mActionFlag & 0x4)) kind = 6;
else                                                    kind = 0x13;
```

Both are semantically equivalent. They produce different branch layouts:
form A's asm ends `li r4, 0x13; b L_end; li r4, 6` (no `b` after the
final `li`), while form B's ends `li r4, 6; b L_end; li r4, 0x13`.

**When to apply.** Look at the target's final `li rN, <K>` of the kind
chain. The value `<K>` is the source's `else` branch. Rewrite the
middle condition (with De Morgan) so the OTHER two conditions are the
two `else if` branches.

**Citations.**
- `NPC/NpcAnm::requestTalkAnm_` (tick 164): target's tail had `li r4, 0x13; b end; li r4, 6` (kind=6 as fall-through). My initial form B (`!(flag & 0x1) || (flag & 0x4) → 6`) compiled to the wrong fall-through. Rewriting to form A (`(flag & 0x1) && !(flag & 0x4) → 0x13; else 6`) gained +0.24pp; the structural mismatch around the kind chain disappeared.

### Caching `this->memberPtr` in a local **before a write-then-write sequence to the same struct** eliminates phantom reloads

**Rule.** When MWCC inlines code that does `obj->fieldA = X; obj->fieldB = Y;`
where the two stores are separated by a non-trivial expression (e.g. a
bool-to-int conversion via `neg/subic/subfe`), the compiler may emit:

```
lwz r4, 0x190(r3)       ; r4 = obj
... compute Y into r0 (uses r3) ...
stw rX, 0x0(r4)         ; obj->fieldA = X
... finish Y computation ...
lwz r3, 0x190(r3)       ; phantom RELOAD of obj into r3
stb r0, 0x4(r3)         ; obj->fieldB = Y (via r3)
```

Caching the pointer in a named local before the write block forces
MWCC to keep it in a single register through both stores:

```cpp
TFoo* p = this->memberPtr;
p->fieldA = X;
p->fieldB = Y;
```

Now both stores use the same register; no phantom reload.

**When to apply.** Diff shows your build doing `lwz rN, offset(rThis)`
twice across a two-store window. Cache the pointer once.

**Citations.**
- `NPC/NpcAnm::requestNpcAnm_` standalone (tick 164): `mAnmRequest->mKind = (int)kind; mAnmRequest->mBlend = ...;` — the second store reloaded `mAnmRequest` despite no intervening function call. Caching to `TNpcAnmRequest* req = mAnmRequest;` eliminated the reload and pushed standalone from 84.6% to 100% (this also broke inlining at *In sites because the body grew an AST node; the trick remains documented for standalone-only contexts).
- `NPC/NpcAnm::npcMareStanding` (tick 164): the `mAnmFrameCounter->mCurFrame++` + `mAnmFrameCounter->mMaxFrame` read sequence had the same reload. Caching to `TNpcAnmFrameCounter* fc = mAnmFrameCounter;` collapsed the reload (TU 83.72 → 85.13%).

### Ternary `var = cond ? K1 : K2;` produces `li r0; mr rVar, r0` while `if (cond) var = K1; else var = K2;` writes directly to `rVar`

**Rule.** Two equivalent forms produce different code under MWCC when
the assignee is a callee-saved register:

```cpp
// Form 1 — ternary: emits via a scratch reg then mr
soundId = isChild ? 0x88AB : 0x8844;
//   lis r3, 0x1
//   subi r0, r3, 0x7755     ; r0 = 0x88AB
//   b L_join
//   subi r0, r3, 0x77bc     ; r0 = 0x8844
// L_join:
//   mr r31, r0              ; transfer to the soundId register

// Form 2 — if/else: writes the target reg directly
if (isChild) soundId = 0x88AB; else soundId = 0x8844;
//   lis r3, 0x1
//   subi r31, r3, 0x7755    ; r31 = 0x88AB
//   b L_join
//   subi r31, r3, 0x77bc    ; r31 = 0x8844
// L_join:
```

The ternary lowers as if it were an rvalue expression, which makes
MWCC materialise the value in `r0` (scratch) first; the surrounding
`= store` then copies `r0` into `rVar` (often a callee-saved reg like
`r31`). The explicit if/else lets MWCC see the target lvalue in each
branch and writes directly.

**When to apply.** Look at the target asm. If you see `subi rN, r3, K`
(or any direct `li`/`addi` of the value) **without** a subsequent
`mr rN, r0`, the source is an if/else. If the target has `li r0, K` /
`mr rN, r0`, the source is a ternary. The same pattern occurs for any
constant-by-condition assignment, not just sound IDs.

**Citations.**
- `NPC/NpcChange::behaveToBeTrampled_` (tick 162): three soundId-via-isChildFlag
  ternaries `soundId = isChildFlag ? 0x88AB : 0x8844;` (and two
  others). Target had `subi r31, r3, 0x7755` / `subi r31, r3, 0x77bc`
  directly. Switching all three to explicit if/else pushed
  80.84 → 85.23% (+4.4pp); the `mr r31, r0` redundancy disappeared
  from all three branches simultaneously.

### `BOOL flag = FALSE; if (cond) flag = TRUE;` vs `BOOL flag; if (cond) flag = TRUE; else flag = FALSE;` produce different code; pick by matching the target's branch layout

**Rule.** Two natural ways to compute a `BOOL` from a predicate
compile to materially different code under MWCC:

1. **Init+set form** — `BOOL flag = FALSE; if (cond) flag = TRUE;`.
   Emits `li rN, 0` before the test, a single `beq skip; li rN, 1;
   skip:` shape (the TRUE branch is "skip the init, then set 1";
   the FALSE branch falls through with the init value).

2. **If-else form** — `BOOL flag; if (cond) flag = TRUE; else flag
   = FALSE;`. Emits the test, then a separate TRUE block `li rN, 1;
   b end;`, then a separate FALSE block `li rN, 0;` followed by the
   join point.

Match the form to the target. Form 1 has no explicit `b end` after the
TRUE assignment; form 2 does. Switching between them often picks up
several percentage points on near-100% BOOL-returning script-handler
style functions.

**Why.** MWCC's if-lowering preserves source structure: the init in
form 1 is hoisted before the test, while form 2 produces two parallel
basic blocks with their own jumps.

**When to apply.** Inspect the BOOL-store area of the target asm.
Form 1 → single `li rN, 0` ahead of the cmp, single `li rN, 1` after
the conditional branch. Form 2 → two `li` instructions, one in each
branch, with a `b` linking them. Pick the source form that produces
the same layout. If neither matches, the bool→BOOL widening pattern
below is the next thing to consider.

**Citations.**
- `NPC/NpcEvent::evIsGameModeNormal` (tick 160): explicit if-else
  `BOOL isNormal; if (==0) =TRUE; else =FALSE;` was at 92.05%.
  Switching to init+set form `BOOL isNormal=FALSE; if (==0) =TRUE;`
  brought it to 99.55% (exact instruction sequence).
- `NPC/NpcEvent::evCheckMonteClear` (tick 160): the opposite — init+set
  `BOOL clear=FALSE; if(cond) =TRUE;` was at 96.64%. Switching to
  if-else form brought it to 99.60%.
- `NPC/NpcEvent::evSetFruitType` (tick 160): adding a missing `else`
  branch that stores 0 to the same field went 98.15→99.81%.

### `bool isFoo = true; if (val != K1 && val != K2) isFoo = false;` is the source-level lever for "pre-init r3=1, then `beq end` short-circuit, then `li r3, 0`" target patterns; pair with a separate `BOOL pushVal = FALSE; if (isFoo) pushVal = TRUE;` to add the `clrlwi.` widening

**Rule.** When the target asm shows:

```
li rN, 0x1                  # pre-init bool = true
... lbz ...                 # read enum/byte to compare
cmplwi rV, K1; beq end      # short-circuit if matches K1
cmplwi rV, K2; beq end      # short-circuit if matches K2
li rN, 0x0                  # else clear to false
end:
clrlwi. r0, rN, 24          # widen bool → BOOL
beq skip
li rM, 0x1                  # set companion BOOL = TRUE
skip:
```

The source must be:

```cpp
bool isFoo = true;
if (val != K1 && val != K2)
    isFoo = false;
BOOL pushVal = FALSE;
if (isFoo)
    pushVal = TRUE;
```

The `bool = true` init is what produces the pre-load `li rN, 0x1`.
The `!=` &&-chain produces `beq end` short-circuits (false-skips the
clear). The separate `BOOL pushVal = FALSE; if (isFoo) pushVal = TRUE;`
produces the `clrlwi.` widening and the BOOL set.

**Why.** MWCC short-circuits `&&` by testing each operand and
branching on false; with `!=` operands, "false" means equality, so
`beq` jumps out. The bool→BOOL widening is materialized by writing
the bool to a separate BOOL local with the init+set form rather than
casting at the push site.

**When to apply.** Whenever target's BOOL-producing predicate has a
pre-init `li rN, 1` BEFORE the comparison chain, with `beq` short-
circuits jumping to the widening rather than separate `li rN, 1; b`
blocks per case.

**Citations.**
- `NPC/NpcEvent::evIsDemoMode` (tick 160): `BOOL` if-else-if-else
  chain was 87.06%. Rewriting as `bool isDemo=true; if (!=3 && !=4)
  isDemo=false; BOOL pushVal=FALSE; if (isDemo) pushVal=TRUE;` brought
  to 98.77% (exact sequence; register coloring differs).
- `NPC/NpcEvent::ev__ForceStartTalk` (tick 160): two-stage block check
  `BOOL block; if (==1 || ==2) =TRUE; else =FALSE; if (!block) if (==3
  || ==4) =TRUE;` was 84.26%. Rewriting both stages with the bool init
  +false-update pattern, declaring `bool block=true; bool talking=block;`
  (the `addi r4, r3, 0` copy form), got to 93.42%.
- `NPC/NpcEvent::ev__ForceStartTalkExceptNpc` (tick 160): same rewrite
  applied; 83.47→87.31%.



**Rule.** When target asm shows the signed switch-with-branching
pattern (one `lis rX, KHIGH` constant base, then per-case
`addi r0, rX, KLOW; cmpw rVAL, r0; beq <case>; bge <next>` with
shared base register), the source MUST be a `switch (val)` statement,
NOT an `if (val == K1) ... else if (val == K2) ...` chain. The
if-else chain compiles to per-case `subis r0, rVAL, KHIGH;
cmplwi r0, KLOW; bne <next>` (unsigned, no shared constant base).

**Why.** MWCC's frontend recognises `switch` and lowers it to the
fused signed-comparison form when all cases share a high half. The
if-else chain is lowered case-by-case with no cross-case opt.

**When to apply.** Two signals from target asm:
- Single `lis rN, K_UPPER` before the chain, reused across all cases.
- `addi r0, rN, K_LOWER; cmpw rVAL, r0; beq/bge` per case.

Rewrite the if-else chain as a `switch`. Order of `case` labels in
source doesn't matter — MWCC reorders by value.

**Citations.**
- `NPC/NpcInitPrg::setIndividualDifference_` (tick 158): the outer
  `if (type == 0x4000014) ... else if (type == 0x400000F) ... else
  if (type == 0x4000019)` chain on `mActorType` was emitting
  `subis/cmplwi/bne` per case. Rewriting as `switch (mActorType)`
  with 3 case branches produced exact match for the dispatch skeleton:
  `lis r3, 0x400; lwz r4, 0x4c(r31); addi r0, r3, 0x14; cmpw r4, r0;
  beq ...; bge ...; addi r0, r3, 0xf; cmpw r4, r0; beq ...`. Same
  rewrite on the nested `0x4000016/0x4000017` Kino-dispatch worked.
  setIndividualDifference_ 58.7 → 60.9% (+2.2pp).
- `NPC/NpcCoin::updateCoin` (tick 156): observed but not noted at the
  time — `mFrame` dispatch on small integer constants used the same
  pattern. Source was already a switch there, which is why it matched.

### Storing the result of `a() || b()` into a named `bool` inhibits inlining of the second predicate

**Rule.** When `a()` and `b()` are inline-marked predicates with
identical bodies, writing `bool blocked = a() || b();` causes MWCC to
emit a real `bl` for `b()`. Rewriting as `if (a() || b()) ...`
(or as `if (!a() && !b()) ...`) keeps both inlined.

**Why.** Unknown — likely a quirk in MWCC's inliner heuristic when
the OR-chain result is materialised as a stored boolean instead of
consumed in a control-flow predicate. The first predicate inlines in
both forms; only the second one toggles.

**When to apply.** Whenever target asm shows the second predicate
inlined (no `bl`) but ours emits a `bl symbol__CFv`. Move the OR
into the controlling `if` directly. If a local is needed for
clarity, cache the receiver pointer instead:

```cpp
TMarDirector* dir = gpMarDirector;
if (dir->isTalkModeNow() || dir->checkUnk124Thing2()) return;
```

**Citations.**
- `NPC/NpcCoin::updateCoin` (tick 156): 78.68 → 82.01% after rewriting
  `bool talking = ...; bool blocked = talking || ...;` to the cached-`dir`
  if-form. Second predicate switched from `bl checkUnk124Thing2__12TMarDirectorCFv`
  to inlined.
- `NPC/NpcCoin::requestAppearCoin` (tick 156): same rewrite, same effect.

### JMA sin/cos CSE requires call sites to be adjacent AND results stored to locals before being written to memory

**Rule.** `JMASSin(v)` and `JMASCos(v)` are inline functions that both
dereference `jmaSinShift`/`jmaSinTable`/`jmaCosTable`. When called with
the same argument, MWCC can CSE the shift-and-mul (`sraw; slwi`) and
share the index register across both `lfsx` instructions — **but only
if the calls are scheduled back-to-back with no intervening stores to
memory**.

Pattern that BREAKS CSE (jmaSinShift loaded twice; shift+slwi twice):

```cpp
unk14.x = 0.0f;
unk14.y = JMASSin(fixedPitch);   // first JMA call, then store
unk14.z = JMASCos(fixedPitch);   // store between → no CSE
```

Pattern that ENABLES CSE (one shift, one slwi, two lfsx share index):

```cpp
f32 cosVal = JMASCos(fixedPitch);   // both calls back-to-back
f32 sinVal = JMASSin(fixedPitch);
unk14.x = 0.0f;
unk14.y = sinVal;
unk14.z = cosVal;
```

**Order of the cached locals matters for register colouring.** When
target's first JMA table load is `jmaCosTable`, write `f32 cosVal =
JMASCos(...)` first; if it's `jmaSinTable` first, write sin first.

**Why.** The intervening `stfs` to `unk14.y` ends MWCC's CSE window —
the compiler conservatively assumes the store could have aliased
`jmaSinShift` (a global) and reloads it. Caching the JMA results
into locals first defers all stores until after both calls.

**When to apply.** Any function that computes sin AND cos of the same
angle, then stores to fields. Look in target asm for **one**
`lwz jmaSinShift / sraw / slwi / lfsx` block feeding two `lfsx`
instructions — if ours emits the block twice, this lever applies.

**Citations.**
- `NPC/NpcCoin::requestAppearCoin` (tick 156): 75.12 → 82.25% from the
  CSE-enabling rewrite + cos-first ordering. Combined with the
  cached-`dir` lever, total fn gain 73.11 → 85.22% (+12.11pp).

### `subi rD, rA, imm` is a mnemonic for `addi rD, rA, -imm` — verify the constant by `(lis_value << 16) - imm`, not `+ imm`

**Rule.** When reading a constant built as `lis rN, K; subi rD, rN, imm`,
the result is `(K << 16) - imm` not `(K << 16) + imm`. This matters when
back-deriving sound IDs, asset hashes, or other multi-byte constants
from target asm.

**Example.** `lis r30, 0x1; subi r4, r30, 0x77f9` builds
`0x10000 - 0x77F9 = 0x8807`, not `0x18807`. The disassembler's `subi`
mnemonic is friendlier than the actual `addi rD, rA, 0x8807` encoding
(where 0x8807 is the SIMM as 16-bit signed = -0x77F9).

**When to apply.** Always, when reading sub-built constants from asm
for source reconstruction. Don't trust the mnemonic at face value —
compute the actual value.

**Citations.**
- `NPC/NpcCoin` (tick 156): the coin-pop SFX ID was misread as `0x18807`
  (would require `lis 0x2`); target uses `lis 0x1 + subi 0x77f9 = 0x8807`.
  Fixing the source value matched the constant-building instruction
  encoding.

### Comparing a `u32` field as `(s32)field <op> KCONST` emits `cmpw` (signed); the natural `field <op> KCONST` emits `cmplw` (unsigned)

**Rule.** Range checks against a `u32` member with literal constants
default to **unsigned** lowering — `cmplw`/`cmplwi`. If target's asm
shows the **signed** form `cmpw`, the original source applied a signed
cast (or stored through a signed local) before comparing.

**When to apply.** When target's asm shows `cmpw` (no `l`) between a
loaded `u32` member and an `addi`-built constant, and you're using
unsigned `cmplw` in your build for the matching predicate. Rewrite as:

```cpp
// before — emits cmplw / cmplwi:
if (mActorType >= 0x04000016 && mActorType < 0x04000018) ...
// after — emits cmpw:
if ((s32)mActorType < 0x04000018 && (s32)mActorType >= 0x04000016) ...
```

Reversing the `&&` operands additionally aligns the branch order
target picks when the upper bound is tested first (target emits
`bge ELSE; addi; cmpw; bge THEN; b ELSE` — the two-condition cascade
with the THEN block reached only via a positive `bge`). Source order
matters here because MWCC preserves left-to-right operand evaluation.

**Why.** `u32` comparisons lower to unsigned PPC instructions
(`cmpl`/`cmplw`). Casting through `s32` forces signed lowering
(`cmp`/`cmpw`). The integer-encoding result is the same for values in
the s32 range, but the two instruction families don't share opcodes,
so it's a strict matching concern.

**Citations.**
- `NPC/NpcEffect::emitHappyEffect_`, `getEffectScale_`, `emitParticle_`
  (tick 154): six call sites of `mActorType >= 0x04000016 &&
  mActorType < 0x04000018` flipped to `(s32)mActorType < 0x04000018 &&
  (s32)mActorType >= 0x04000016`. Each emitted `cmpw + cmpw` (instead
  of `cmplw + cmplw`) and the branch direction matches target. Function
  gains: emitHappyEffect_ 93.92 → 95.29, getEffectScale_ 2.14 → 7.95,
  emitParticle_ 66.62 → 67.75 (combined +3.49pp on the three).

### `s32 idx = getIndex(); ... use (u16)idx` keeps full-width result through assignment, masks only at use site

**Rule.** Target stores a `s32`-returning function's result into a
register without truncation (`addi r31, r3, 0` = `mr r31, r3`), then
applies `clrlwi r0, r31, 16` immediately before the multiply that
needs the 16-bit value. Our build's `u16 idx = func()` declaration
forces `clrlwi r31, r3, 16` **at the assignment**, shifting subsequent
register allocation.

**When to apply.** When target's asm shows the truncation `clrlwi r0,
rN, 16` right next to a `mulli r0, r0, <Mtx-size>` (or similar 16-bit
multiply) AND `rN` was just loaded from a function return without
masking, source likely used `s32 idx` (or `int idx`) and cast `(u16)`
at the multiplication site. Rewrite:

```cpp
// before — clrlwi immediately after bl getIndex:
u16 idx = nameTab->getIndex(jntName);
J3DModel* mdl = getModel();
mPtr = (MtxPtr)((u8*)mdl->mNodeMatrices + idx * sizeof(Mtx));

// after — clrlwi only at the multiply:
s32 idx = nameTab->getIndex(jntName);
J3DModel* mdl = getModel();
mPtr = (MtxPtr)((u8*)mdl->mNodeMatrices + (u16)idx * sizeof(Mtx));
```

**Why.** Target intent: keep `idx` as the full 32-bit return value
between the call and the multiply (no intermediate `clrlwi`), then
narrow when actually needed. The source-level lever is the declared
type of the local plus the cast on use.

**Citations.**
- `NPC/NpcEffect::setNoteEffectMtxPtr_` (tick 154): single-line change
  drove the function 86.14 → **99.83%** (+13.69pp; remaining gap is
  phantom +8 stack pad).
- `NPC/NpcEffect::setHappyEffectMtxPtr_` (tick 154): 86.14 → 91.29%
  (+5.15pp).
- `NPC/NpcEffect::setPollutionEffectMtxPtr_` (tick 154): 80.93 →
  93.31% (+12.38pp) across three getIndex sites.

### Local `MtxPtr` (or `T*`) cache forces batched lfs loads across `mtx[i][j]` chains where direct member access reloads the pointer

**Rule.** When source assigns three consecutive `mPos.{x,y,z} =
this->mPtrMtx[N][3];` (or similar member-of-pointer-chain), MWCC
conservatively reloads `this->mPtrMtx` before each access — assuming
intervening side effects could mutate the field. Target's asm shows a
single `lwz r7, OFFSET(this)` followed by 3 `lfs fN, OFFSET(r7)` and
3 `stfs`. Caching `mPtrMtx` in a local pointer ref before the access
chain produces target's batched pattern.

**When to apply.** When target's asm has a single base-pointer load
(`lwz rN, OFFSET(this)`) followed by 3 floating-point loads via `lfs
fK, FIELD_OFFSET(rN)` reusing `rN`, but your build reloads `rN`
between each access. Rewrite:

```cpp
// before — reload mPtrXxxEffectMtx per access:
mPos.x = mPtrXxxEffectMtx[0][3];
mPos.y = mPtrXxxEffectMtx[1][3];
mPos.z = mPtrXxxEffectMtx[2][3];

// after — single load via local:
MtxPtr mtx = mPtrXxxEffectMtx;
mPos.x = mtx[0][3];
mPos.y = mtx[1][3];
mPos.z = mtx[2][3];
```

**Why.** Same root cause as the existing
[[don't-cache-a-global-pointer-in-a-local-across-function-calls]] rule,
but reversed: that rule says **don't** cache when target reloads
across calls; this rule says **do** cache when target keeps a single
load and there are no intervening calls. MWCC errs conservative when
the source writes `this->field` directly because it can't prove the
field isn't aliased through other paths within the basic block. A
local copy is an explicit "I've read this value, freeze it" signal.

**Citations.**
- `NPC/NpcEffect::emitParticle_` (tick 154): caching
  `mPtrSmokeEffectMtx` and `mPtrNoteEffectMtx` in `MtxPtr` locals
  combined for +0.90pp (68.40 → 69.30%). Each batched-load block went
  from 3-instruction interleave to 1+3+3 layout matching target.

### Switch with non-contiguous case labels (`0xF`, `0x19`) lowers as a binary-search branch tree; equivalent `if-else if` chain lowers as sequential `cmpwi/beq`

**Rule.** Sparse switch dispatch (e.g. `case 0xF:`, `case 0x19:`) with
2-3 cases compiles to MWCC's binary-search tree:
```
cmpwi r0, 0x19
beq   case_19
bge   default       ; bigger than max — skip
cmpwi r0, 0xf
beq   case_f
b     default
```
Even though there are only 2 active cases, the compiler inserts a
median-test (`cmpwi r0, 0x19; beq; bge default; cmpwi r0, 0xf; beq;
b default`) — the canonical balanced-binary-tree shape.

An equivalent `if (x == 0xF) ... else if (x == 0x19) ...` lowers to a
flat cascade (`cmpwi 0xF; beq ...; cmpwi 0x19; beq ...; b default`) —
a different structure.

**When to apply.** When target's asm shows the median-cmpwi-then-bge
pattern for a dispatch on a value, rewrite the if-cascade as a
`switch`. The number of cases doesn't matter — even 2 cases get the
binary-search shape.

**Why.** Switch-as-branching is one of MWCC's two switch lowerings;
it's chosen for sparse case ranges (where a jump-table would have too
many empty slots). It generates ordered compare cascades that
short-circuit using the case-value ordering. If-else cascades are
order-preserving but **not** ordered by value, so MWCC won't introduce
the median pivot.

**Citations.**
- `NPC/NpcEffect::emitParticle_` (tick 154): converting `if (anmKind ==
  0xF) ... else if (anmKind == 0x19) ...` to `switch` produced
  target's `cmpwi 0x19; beq; bge default; cmpwi 0xf; beq; b default`
  pattern. Function 69.30 → 70.42% (+1.11pp).
- See also the existing rule
  [[rewrite-if-v-k1-or-v-k2-as-switch-when-target-asm-shows-the-switch-branch-order]]
  for the related `case K1: case K2:` pattern.

### Accumulator initialized to `0.0f` then `acc += a*a; acc += b*b;` emits `lfs f0, @0; fmadds; fmadds` (instead of `fmuls; fmuls; fadds; fadds`)

**Rule.** A sum-of-products with an explicit `0.0f` initializer compiles to:

```
lfs   f0, @zero@sda21          ; f0 = 0.0
fmadds f0, fA, fA, f0           ; f0 = a*a + 0
fmadds f0, fB, fB, f0           ; f0 = b*b + (a*a + 0)
```

while the natural `a*a + b*b` form compiles to:

```
fmuls fX, fA, fA
fmuls fY, fB, fB
fadds f0, fX, fY
```

The 0.0f accumulator is one extra instruction (the leading `lfs`) but
two fewer (fadds collapsed into fmadds). Net: same instruction count
but different register pressure.

**When to apply.** Target asm shows `lfs f0, @zero@sda21` immediately
followed by `fmadds f0, fA, fA, f0; fmadds f0, fB, fB, f0` where you
were computing a sum of products as part of a comparison or
assignment. Rewrite the source as:

```cpp
f32 acc = 0.0f;
acc += dx * dx;
acc += dz * dz;
if (acc < threshold) ...
```

**Why.** Initial `0.0f` makes the first multiply lower into an fmadds
with the loaded zero as the addend; subsequent `+=` continues fusing.
The standalone `+` between two `*` results lacks an accumulator to
fuse into, so MWCC emits separate `fmuls` + `fadds`.

**Citations.**
- `NPC/NpcCollision::setVariableDamageRadius_` (tick 150): rewriting
  `dx*dx + dz*dz < CLBSquared(...)` as `acc = 0; acc += dx*dx; acc +=
  dz*dz; if (acc < ...)` produced target's `lfs f0, @2321; fmadds
  f0, f31, f31, f0; fmadds f0, f30, f30, f0` sequence. Function
  94.5 → 97.1% (+2.6pp).
- `Enemy/BossHanachanSub::BHSCalcRevisionDistXZByRotateZ` (per the
  related "Zero-fmadds rotation pattern" rule below — similar
  mechanism via explicit `+ 0 * x` terms; this rule is the sum-only
  variant).

**Variant.** If the source involves multiple terms (not just two
squares), keep the same pattern: `acc = 0.0f; for each term: acc +=
term;`.

### Predicate declared as `bool` vs `BOOL` (= int) changes the caller's result-test instruction

**Rule.** When a function is declared to return `bool`, MWCC tests the
result at the call site with `clrlwi. r0, r3, 24` (mask to lower 8 bits,
update CR0). When declared `BOOL` (which is `typedef int BOOL`), MWCC
emits `cmpwi r3, 0` (full 32-bit compare). Both yield the same logical
result, but they're different instructions and break byte-identical
matching.

**Why.** `bool` is an 8-bit type in MWCC's C++; `BOOL` is `int`
(32-bit). MWCC's caller-side codegen uses the declared return type to
pick the test width, regardless of the actual function body emitting
the value zero-extended in r3.

**Diagnostic signature.** Look at the instruction immediately after
the `bl predicate__...` in target asm:
- `clrlwi. r0, r3, 24` → callee declared `bool`
- `cmpwi r3, 0` → callee declared `BOOL` / `int`

If our build emits the opposite of what target shows, flip the return
type declaration in the header (the implementation body in asm-land
returns 0/1 either way).

**How to apply.** Predicates whose declared return type doesn't match
target's caller-side test should be flipped. The function body itself
doesn't change semantically; if the implementation uses a result local, make
that local match the new return type to avoid a boundary cast. Also audit
wrapper predicates that simply return another predicate: if the wrapper's
callers require `bool` but the wrapper body normalizes the wrapped call with
`neg/subic/subfe`, the wrapped callee may need a `bool` declaration too.

**Citations.**
- `mario/MSound/MSound` (2026-06-09): `MSound::checkWaveOnAram` callers use
  `clrlwi.`, so the wrapper must remain `bool`; its tail call to
  `JAIBasic::checkSceneWaveOnMemory` normalized through `neg/subic/subfe`
  while that callee was declared `BOOL`. Changing only
  `checkSceneWaveOnMemory` to `bool` kept the JAudio owner exact and moved
  `checkWaveOnAram` 94.5% -> 100% without caller regressions.
- `NPC/NpcCallback::NPCNeckCallBack` (tick 148): `TBaseNPC::isNeedNeckStraight`
  was declared `BOOL` in our header, generating `cmpwi r3, 0` after the
  bl. Target uses `clrlwi. r0, r3, 24`. Changed declaration to `bool`
  → +0.24pp on NPCNeckCallBack and matching test instruction.
- `NPC/NpcCollision::execNpcObjCollision_` (tick 445): `TBaseNPC::isNerveWalk`
  was declared `BOOL`, so the two call sites emitted `cmpwi r3, 0`. Target
  uses `clrlwi. r0, r3, 24`; changing the declaration and owner local to
  `bool` kept `NpcChange`'s owner exact and moved the caller 94.4% -> 96.5%.

### Don't cache a global pointer in a local across function calls — MWCC pins it to a callee-save register and changes register allocation throughout

**Rule.** When source caches a global pointer like
`TBaseNPC* npc = gpCurrentNpc;` and uses `npc->X` throughout a function
with many calls, MWCC allocates the cached pointer to a callee-save
register (e.g. r31) and keeps it live across every call. Target's
codegen often does NOT cache, instead reloading via
`lwz rN, gpCurrentNpc@sda21` after each function call (rN is
caller-save). The local-cache form changes:
1. Which callee-save registers get used (mine grows the saved-reg set
   by one, adds an extra `stw r27` etc. to prologue).
2. Stack frame size (extra saved reg + alignment).
3. Subsequent field accesses (uses cached pointer vs reloaded).

**How to apply.** When target reloads `globalPtr` after every bl, drop
the `T* local = globalPtr;` and use `globalPtr->X` everywhere instead.
MWCC will then treat each access as an independent load, allocate a
caller-save register for the brief duration of each use, and re-fetch
across calls.

**Caveat.** This only matters when the global is used across function
calls. If all uses are in a basic block with no calls, MWCC will cache
either way (and the local-cache form is fine).

**Citations.**
- `NPC/NpcCallback::NPCNeckCallBack` (tick 148): removing `TBaseNPC*
  npc = gpCurrentNpc;` and inlining `gpCurrentNpc->X` at every use site
  shifted register allocation from r27-r31 (saving 5 GPRs) to r28-r31
  (saving 4 GPRs), matched target's reload pattern, and gained
  +8.5pp on the function (76.1 → 84.6%).

### Restructure multiple `return TRUE` paths into a single fall-through return to merge the epilogue's `li r3, 1` entry

**Rule.** Multiple distinct `return TRUE;` statements in the same
function compile to separate `li r3, 1; b L_EPILOGUE` sequences in
mine, even though target consolidates them into one shared entry
point:

```
L_RETURN_TRUE: li r3, 0x1
L_EPILOGUE:    <restore saves, blr>
```

with every "return TRUE" being `b L_RETURN_TRUE` (or a conditional
branch directly to L_RETURN_TRUE), and the function's natural end
falling through into L_RETURN_TRUE.

**How to apply.** If a function has the structure:
```cpp
if (early_cond_1) return TRUE;
if (null_check) return FALSE;
if (early_cond_2) return TRUE;
... body ...
return TRUE;
```

Restructure as a nested-if fall-through:
```cpp
if (!early_cond_1) {
    if (!null_check) {
        if (!early_cond_2) {
            ... body ...
        }
    } else {
        return FALSE;  // the only explicit return
    }
}
return TRUE;  // shared fall-through
```

MWCC's branch-shaping will then point every conditional branch at the
shared `return TRUE` and the body falls through into it. This avoids
the multiple `li r3, 1; b epilogue` blocks and matches target's
single-li-then-epilogue pattern.

**Caveat.** The nested form must be semantically equivalent. Verify
the inversions of each `if (early_cond) return` are correctly
expressed as `if (!early_cond) { rest }`. Indent the block.

**Citations.**
- `NPC/NpcCallback::NPCNeckCallBack` (tick 148): restructure
  consolidated three separate `return TRUE` paths into a single shared
  exit. Combined with other levers, gained +1.6pp (84.6 → 86.2%).

### `operator-` on a TVec3 (returning by-value param) keeps `sub__TVec3` out-of-line under `-inline deferred`, where direct `.sub()` inlines

**Rule.** Under `-inline deferred` TU flags (used by NPC/, Strategic/,
Player/), writing `diff = rotDir - rotAx;` (which inlines
`operator-(TVec3 fst, const TVec3& snd)`) leaves the inner
`fst.sub(snd)` call as `bl sub__TVec3` — MWCC's deferred inliner
won't recurse through the operator- inline to inline sub() at the
deeper level. Writing the direct form `tmp.sub(rotAx);` (with sub
called at depth 1) DOES inline sub() into the caller.

This means:
- `diff = rotDir - rotAx;` → 1 copy ctor + bl sub + 1 assignment copy
- `tmp = rotDir; tmp.sub(rotAx); diff = tmp;` → 1 copy ctor + inlined
  subtraction (3× fsubs) + 1 assignment copy

When target's asm shows `bl sub__Q29JGeometry8TVec3<f>...`, prefer the
operator- form.

**Citations.**
- `NPC/NpcCallback::NPCNeckCallBack` (tick 148): the `diff = rotDir -
  rotAx;` form pushed the function from 65.8% (with inlined sub) to
  71.7% (with bl sub). The remaining gap is the by-value `fst`
  parameter expansion introducing extra intermediate copies — those
  are a separate currently-hard issue.

### Naked `(expr ? true : false)` ternary at the test site forces MWCC's bool-materialize-then-test pattern without a helper function

**Rule.** The existing Settled rule
[[Predicates that materialize a 0/1 BOOL force inline-then-test even
for `&&` short-circuit]] documents the 5-instruction
materialize-then-test sequence (test → `li r0, 1; b; li r0, 0` →
`clrlwi. r0, r0, 24; beq`) and traces it to inline helpers that
return `... ? 1 : 0`. **The same pattern also fires WITHOUT any
helper — just write the ternary inline at the use site:**

```cpp
// Variant 1: ternary around a stored bool variable
bool ok = false;
if (cond) ok = true;

// BAD: single test+branch, omits the materialize-then-test pair
if (!ok) return;            // clrlwi. r0, rN, 24; beq exit

// GOOD: target's full 5-insn materialize+retest
if (!(ok ? true : false))   // clrlwi. r0, rN, 24
    return;                 // beq SKIP; li r0, 1; b CONT; SKIP: li r0, 0;
                            // CONT: clrlwi. r0, r0, 24; beq exit

// Variant 2: ternary around a comparison result
// BAD:
if (mChaseFrame != 0.0f) { ... }   // fcmpu; beq exit
// GOOD:
if ((mChaseFrame != 0.0f) ? true : false) { ... }
// → fcmpu; beq SKIP; li r0, 1; b CONT; SKIP: li r0, 0;
//   CONT: clrlwi. r0, r0, 24; beq exit
```

The ternary `cond ? true : false` is a no-op semantically (bool→bool)
but it forces MWCC to first canonicalize the condition into a true
0-or-1 byte value in r0, then re-test r0 for the branch. That's the
same materialize+test cascade the helper-call form produces, just
spelled inline.

**Diagnostic signature in target asm.** Look for the exact sequence
between two CR-clobbering instructions on the same value:
```
<initial test>         # e.g. clrlwi. r0, rN, 24 (bool), or fcmpu (float)
beq L_zero
li r0, 0x1
b L_test
L_zero: li r0, 0x0
L_test: clrlwi. r0, r0, 24
beq EXIT
```
If our build emits just the first test + branch and skips the middle
5 instructions, the source needs the inline ternary.

**Where to try it next.** Any spot where:
1. Target has the 5-insn materialize-then-test signature above
2. Source uses `if (var)` / `if (!var)` for a bool, OR `if (a OP b)`
   for a non-bool that gets canonicalized
3. There's no obvious inline helper to point to

**Citations.**

- `Camera/sunmgr::perform` (tick 144): two same-TU sites (the `inMode`
  early-return and the `c`-after-warp-gate early-return) both gained
  the materialize cascade after switching `if (!ok) return;` to
  `if (!(ok ? true : false)) return;`. Function 77.8 → 98.07%
  cumulative with other levers, with these two changes worth ~+9pp.
- `Camera/CameraInbetween::execCameraInbetween` (tick 144): switched
  `if (mChaseFrame != 0.0f)` to `if ((mChaseFrame != 0.0f) ? true :
  false)`. Materialize cascade appeared after the `fcmpu`. Function
  87.8 → 90.8% (+3pp). Confirms the lever works for non-bool sources
  (a float comparison result) and across independent TUs.

**Mechanism note.** This is the same MWCC mechanism behind the
existing `isAirborne()`/`isActorTypeOf()` Settled rule above —
the trigger is the explicit ternary `? true : false`, whether it
appears in a helper body or inline at the test site.

### Use `Vec` (not component x/y/z) for save-restore-around-call temporaries to force stack spill instead of non-volatile FPR spill

**Rule.** When a function reads three contiguous f32 fields into a
temporary, calls into something, then restores the fields — and the
target asm saves/restores via stack (`stw/lwz` to `r1+offset`) rather
than non-volatile FPRs (`stfd f29-f31` in prologue) — the source must
use a **`Vec` temporary assigned via struct copy**, not a
`JGeometry::TVec3<f32>` temporary with component-by-component
assignment:

```cpp
// BAD: produces stfd f29-31 + restore via stfs, inflates stack 24B
JGeometry::TVec3<f32> tmp;
if (cond) {
    tmp.x = *(f32*)((u8*)this + 0x80);
    tmp.y = *(f32*)((u8*)this + 0x84);
    tmp.z = *(f32*)((u8*)this + 0x88);
}
tool->call(...);
if (cond) {
    *(f32*)((u8*)this + 0x80) = tmp.x;
    *(f32*)((u8*)this + 0x84) = tmp.y;
    *(f32*)((u8*)this + 0x88) = tmp.z;
}

// GOOD: produces lwz/stw save (integer) + lfs/stfs restore (float)
Vec tmp;
if (cond) {
    tmp = *(Vec*)((u8*)this + 0x80);   // Vec struct copy → lwz/stw
}
tool->call(...);
if (cond) {
    *(f32*)((u8*)this + 0x80) = tmp.x; // component float restore
    *(f32*)((u8*)this + 0x84) = tmp.y;
    *(f32*)((u8*)this + 0x88) = tmp.z;
}
```

**Why it works.** `Vec` (plain C struct of 3 floats) struct-copy
compiles to lwz/stw via auto-gen op=, forcing tmp to live in stack
memory across the function call. Component-by-component f32
assignment uses lfs/stfs, and MWCC tries to keep the values in float
registers; if they need to survive a call, it spills to non-volatile
FPRs f29-f31 (24B prologue inflation). Mixing INT save and FLOAT
restore matches the target's pattern exactly:

```
# Save (integer copy from struct to stack)
lwz r4, 0x80(r29); lwz r0, 0x84(r29)
stw r4, 0xe8(r1);  stw r0, 0xec(r1)
lwz r0, 0x88(r29); stw r0, 0xf0(r1)
# ... function call ...
# Restore (float copy from stack back to struct)
lfs f0, 0xe8(r1); stfs f0, 0x80(r29)
lfs f0, 0xec(r1); stfs f0, 0x84(r29)
lfs f0, 0xf0(r1); stfs f0, 0x88(r29)
```

**Citations.**

- `Camera/CameraChange::changeCamModeSub_` (tick 140):
  78.7 → **82.1%** (+3.4pp). Replacing `JGeometry::TVec3<f32> tmp`
  with `Vec tmp` and using struct-copy for the save eliminated all
  3 `stfd` saves in the prologue, dropped stack from 0x120 to
  0x108, matching target. TU 74.9 → 75.7%.

### Explicit out-of-line `operator=` is the only reliable way to make MWCC emit a callable `__as__` symbol; `inline` always gets fully inlined under `-inline deferred`

**Rule.** When the target shows a `bl __as__<class>` to a weak
auto-generated `operator=`, we cannot reproduce a callable version
of that operator via the compiler-generated default — MWCC's
auto-gen `operator=` is *always* inlined at the call site (even with
`#pragma dont_inline on`). The only reliable way to force MWCC to
emit `__as__` as a separate function and `bl` to it is to declare
the operator= **without `inline`** with an **out-of-line definition**:

```cpp
class TTargetCamera {
public:
    Vec mPos, mTgt, mUp;  // 0x00, 0x0C, 0x18
    s16 unk24, unk26;     // 0x24, 0x26
    f32 unk28;            // 0x28
    s16 unk2C;            // 0x2C
    f32 unk30;            // 0x30  (size 0x34)

    TTargetCamera& operator=(const TTargetCamera& other);  // decl only
};

// Out-of-line, NO `inline` keyword. Body uses Vec assignments
// (not field-by-field x/y/z) so MWCC emits the same auto-gen
// interleaved lwz/lwz/stw/stw pattern as the target.
TTargetCamera& TTargetCamera::operator=(const TTargetCamera& other)
{
    mPos  = other.mPos;
    mTgt  = other.mTgt;
    mUp   = other.mUp;
    unk24 = other.unk24;
    unk26 = other.unk26;
    unk28 = other.unk28;
    unk2C = other.unk2C;
    unk30 = other.unk30;
    return *this;
}
```

**Caveat.** This produces GLOBAL linkage, not WEAK. The target's
`__as__` is `weak`. We have not found a way to get WEAK + bl + the
asymmetric "first-call-bl, second-call-inlined" pattern that MWCC
actually emits for genuine auto-gen `operator=`. Function body
itself matches 100% at instruction level. Adding `inline` (either
in-class or to the out-of-line definition) immediately drops the
function to "missing" — MWCC inlines both call sites.

**For the SECOND call site** (when target has an inlined copy of
`__as__`), use a mix of `Vec` struct copies and scalar field copies:

```cpp
// Target's inlined op= produces interleaved lwz/lwz/stw/stw for each
// Vec3 (auto-gen Vec op= inlining). Reproduce by using Vec struct
// copies (NOT component-by-component x/y/z assignments):
*(Vec*)((u8*)this + 0xB4) = *(Vec*)&dst.mPos;
*(Vec*)((u8*)this + 0xC0) = *(Vec*)&dst.mTgt;
*(Vec*)((u8*)this + 0xCC) = *(Vec*)&dst.mUp;
*(s16*)((u8*)this + 0xD8) = dst.unk24;
*(s16*)((u8*)this + 0xDA) = dst.unk26;
*(f32*)((u8*)this + 0xDC) = dst.unk28;
*(s16*)((u8*)this + 0xE0) = dst.unk2C;
*(f32*)((u8*)this + 0xE4) = dst.unk30;
```

**Citations.**

- `Camera/CameraChange::changeCamModeSub_` (tick 140):
  71.3 → **78.7%**. Target had `bl __as__13TTargetCamera` + one
  inlined copy. Out-of-line `operator=` declaration + Vec struct
  copies at the second site matched both call sites' instruction
  patterns. TU 71.5 → 74.9% fuzzy. Function-level match 100% for
  `__as__13TTargetCameraFRC13TTargetCamera`, though linkage is
  GLOBAL vs target's WEAK.

**Where to try it next.** Any near-match function whose diff shows
target calling `__as__<SomeClass>` (weak auto-gen op=) but our build
emits a sequence of lwz/stw without the bl. Common pattern in TUs
that copy state structs (camera, lighting, transform, anim state).

### Hoist `base + idx` (without the constant offset) into a `u8*` local to force `add+lwz(CONST)` instead of `lwzx`

**Rule.** When the target asm computes a struct-field access by adding
a runtime offset then loading at a constant displacement
(`add rRES, rBASE, rIDX; lwz rDST, OFFSET(rRES)`), and our build instead
emits `addi rOFF, rIDX, OFFSET; lwzx rDST, rBASE, rOFF` (combining the
constant into the runtime offset before an indexed load), introduce a
**`u8*` local** that holds `base + idx` (no constant) and dereference
through it with `+ OFFSET` as a syntactic suffix. MWCC will then keep
the constant in the `lwz` immediate.

```cpp
// BEFORE (our build): MWCC folds 0x2D8 into the runtime offset:
//   slwi r3, r4, 2
//   addi r0, r3, 0x2d8                ; r0 = mode*4 + 0x2D8
//   lwzx r4, r31, r0                  ; r4 = mem[this + r0]
TCameraKindParam* k = *(TCameraKindParam**)((u8*)this + mode * 4 + 0x2D8);

// AFTER (target): MWCC keeps the constant in the lwz immediate:
//   slwi r0, r4, 2
//   add r4, r31, r0                   ; r4 = this + mode*4
//   lwz r4, 0x2d8(r4)                 ; r4 = mem[r4 + 0x2D8]
u8* p = (u8*)this + mode * 4;
TCameraKindParam* k = *(TCameraKindParam**)(p + 0x2D8);
```

**Why it works.** When the constant is folded with the runtime offset
(`mode*4 + 0x2D8` as a single sub-expression), MWCC schedules a runtime
add and uses `lwzx`. Hoisting `base + idx` (no constant) into a typed
pointer local forces the constant to remain a separate `+ OFFSET`
addition on a pointer; MWCC then routes the constant through the `lwz`
immediate. The local pointer itself doesn't need to live in a separate
register — MWCC folds it back into the same `lwz` instruction.

**Diagnostic signature.** Target shows `add rRES, rBASE, rIDX; lwz rDST,
CONST(rRES)`. Our build shows `addi rTMP, rIDX, CONST; lwzx rDST,
rBASE, rTMP`. Same total instruction count but different forms.

**Citations.**

- `Camera/cameragc::ctrlGameCamera_` and `loadAfter` (tick 437):
  `ctrlGameCamera_` **96.2 → 99.0%** and `loadAfter` **77.1 → 77.6%**.
  Both call sites access the save-param table at `this + 0x2D8 +
  mode*4`; spelling this as `const u8* savePtr = (const u8*)this +
  mode * 4; *(savePtr + 0x2D8)` changed MWCC from
  `addi mode*4,0x2D8; lwzx this,index` to the target
  `add this,mode*4; lwz 0x2D8(base)`.
- `Camera/CameraChange::setUpToLButtonCamera_` (tick 138):
  85.79 → **99.88%**. Hoisting `u8* p = (u8*)this + mode * 4` before
  the copySaveParam call collapsed our `slwi+addi+lwzx` to target's
  `slwi+add+lwz(0x2D8)`. Combined with using the typed `unkA8` field
  (vs `*(f32*)((u8*)this + 0xA8)`), eliminated the extra r30/r31
  saves entirely.

**Where to try it next.** Any near-match function whose diff shows
`lwzx rDST, rBASE, rTMP` in our build vs `add rRES, rBASE, rIDX; lwz
rDST, CONST(rRES)` in target. Especially common when accessing
arrays at struct-relative offsets — e.g. `mArray[mode]` where
`mArray` is at offset `O` of `this`.

### `do { body; i++; } while (i < N)` keeps a small-N loop intact; `for (T i = 0; i < N; i++)` triggers MWCC to unroll

**Rule.** When the target asm shows a *runtime* loop (cmp+blt-at-bottom)
over a small statically-known trip count (e.g. N=3), the source must use
`do { body; i++; } while (i < N);`. The `for (T i = 0; i < N; ++i)`
form (and `while (i < N) { body; i++; }` form, in some cases) triggers
MWCC's loop unroller — emitting N copies of the body inline. The
do-while is the only one that consistently keeps the loop as a runtime
cmp+branch with a single body emission.

```cpp
// UNROLLS in MWCC — produces N body copies:
for (u8 s = 0; s < 3; s++) {
    mSumPos[s].x = 0.0f;  // 18 stmts total expanded
    mSumPos[s].y = 0.0f;
    mSumPos[s].z = 0.0f;
    mPool[s][0]  = 0;
    mPool[s][1]  = 0;
    mPool[s][2]  = 0;
}

// LOOP KEPT INTACT — produces cmp+blt-at-bottom matching target:
u8 s = 0;
do {
    mSumPos[s].x = 0.0f;
    ...
    s++;
} while (s < 3);
```

**Diagnostic signature.** Look at the target asm for the body block.
If you see `cmplwi rX, N; blt .Lloop_top` with the body INLINED N times
between, MWCC unrolled in source. If you see `cmplwi rX, N; blt .Lloop_top`
WRAPPING a single body, source is do-while. The loop-counter
initialization (`li rN, 0`) and post-loop increment (`addi rN, rN, 1`)
are also clues — unrolled loops won't have them at runtime.

**Citations.**

- `MSound/MSoundScene` `frameLoop` (tick 136): converted mSumPos init,
  inner avg loop, AND outer sector loop from `for(...)` to `do-while`.
  Match went 58.7 → 75.9% (+17.2pp). The init loop's `for` produced
  18 sequential stmts; `do-while` produced a tight 11-instr loop body
  matching target's 11-instr `.L_80180B84` block.

**Where to try it next.** Any TU with small-N inner loops over fixed
arrays (Vec[3], color[4], etc) where target asm shows a runtime
cmp+blt over a single body emission. Sweep candidate near-match TUs
whose inner loops show unrolled bodies in our build.

### Manually-unrolled-in-source 8x loop matches MWCC's auto-unroll-with-cmp-blt-at-bottom (not srwi+mtctr+bdnz)

**Rule.** When the target asm uses an N-stmt-unrolled inner body with a
`cmplw r3, r0; blt .Lloop_top` controller (not `srwi+mtctr+bdnz`), and
N is small enough (8) that the original source was hand-unrolled, write
all N stores explicitly with a `count > 8` outer guard and a `count - 8`
limit:

```cpp
Vec* p = trans;
u8 i = 0;
if (count > 8) {
    u8 lim = count - 8;
    while (i < lim) {
        mPosPtrs[i + 0] = p;
        mPosPtrs[i + 1] = p + 1;
        ...
        mPosPtrs[i + 7] = p + 7;
        p += 8;
        i += 8;
    }
}
while (i < count) {
    mPosPtrs[i] = p;
    p++;
    i++;
}
```

vs the auto-unrolled form `for (int i = 0; i < count; i++) mPosPtrs[i] = &trans[i];`
which MWCC unrolls but uses `srwi+mtctr+bdnz` for the counter (computing
ceil((count-8)/8) iterations upfront). The hand-unrolled form preserves
the running pointer-arithmetic style (p += 8) that target's source used.

**Citations.**

- `MSound/MSoundScene` `frameLoop` trans-copy (tick 136): replaced
  `for (i; i < count; i++) mPosPtrs[i] = &trans[i]` with the manual
  8x unroll above. Match 75.9 → 83.4% (+7.5pp). Resolves the
  srwi-vs-cmplw-blt residual that the t135 IMPL flagged.

### `PARAM_INIT(field, default)` stringifies `field` — pick the field name to match target's rodata key strings, even if it breaks `m`-prefix convention

**Rule.** `PARAM_INIT(member, default)` expands to
`member(this, default, calcKeyCode(#member), #member)`. The stringified
member name lands in `.rodata`/`.sdata2` AND becomes part of the keycode
hash used to look up params in the .prm file at runtime. If the original
target's rodata shows an unprefixed string (`"turnSpeed"`, `"speed"`,
`"angle"`, `"radius"`), the source field MUST also be unprefixed —
otherwise the `m`-prefixed string (`"mTurnSpeed"`) lands in rodata and
no amount of register/codegen tweaking will fix the rodata mismatch or
shift downstream offsets. The mismatch also breaks runtime behavior:
the keycode hashed from `"mTurnSpeed"` won't match the disc's `.prm`
entry keyed by `"turnSpeed"`, so the param load silently keeps the
default.

```cpp
// BEFORE (our convention, rodata "mTurnSpeed"):
PARAM_INIT(mTurnSpeed, 8.0f)
// member declared: TParamRT<f32> mTurnSpeed;

// AFTER (target rodata "turnSpeed"):
PARAM_INIT(turnSpeed, 8.0f)
// member declared: TParamRT<f32> turnSpeed;
```

**Diagnostic signature.** Look at the asm `.string` entries inside the
TU's rodata + sdata2 sections. If they're lowercase / unprefixed but our
header uses `m`-prefix, that's the lever. Strings ≤ 8 bytes land in
sdata2 (small data area, e.g. `"speed"`, `"angle"`, `"range"`,
`"radius"`); longer ones in rodata (e.g. `"turnSpeed"`, `"turnSpeed2"`).
The break is structural — *every* `addi rN, r27, OFF` to those rodata
chunks downshifts in our build, dragging match% down everywhere they're
referenced.

**Citations.**

- `Enemy/BathtubPeach` `TBathtubPeachParams` (tick 132): renamed
  `mTurnSpeed`/`mTurnSpeed2`/`mSpeed`/`mAngle`/`mRange`/`mRadius` →
  unprefixed. Rodata strings match exactly; downstream addi offsets
  to the param strings drop their shift. TU rose 89.42 → 89.46% and
  unblocks further codegen work in `load`, `escape`, `stagger`.
- Re-confirms a partial citation from the t122 `static const` entry
  below — naming convention is **not** uniformly `m`-prefixed in the
  original. Cross-check `symbols.txt` AND rodata `.string` entries
  before assuming the convention.

**Where to try it next.** Run the rename-sweep heuristic over every
`PARAM_INIT(m...)` use and grep the matching TU's asm for the
stripped-prefix lowercase variant. False-positive risk is moderate
(`brake` / `acc` etc. are also common animation-key strings), so
verify the rodata section + nearby `lfs` defaults match the param's
default value before committing the rename.

### Loop form `for (int i = 0; i < N; i++) arr[i] = K * (i+1);` unrolls and forces per-store register materialization (incl. `clrrwi r,r,0` for *1)

**Rule.** When target asm shows an unrolled N-store sequence where the
"× 1" store materializes the multiplier into a **fresh register** via
`clrrwi rD, rS, 0` (== `slwi rD, rS, 0` == identity copy), rather than
storing the source register directly, the source must use a **loop**
that MWCC unrolls. Hand-unrolled stores with the literal coefficient
constant-fold so the `× 1` store collapses to a direct `stw rS, …`.

The loop variant treats each `(i + 1)` as its own runtime expression
evaluated per (unrolled) iteration, even though all eight unrolled
values are compile-time constants. So `i=0` produces a multiply-by-1
that lowers to "copy `rS` into a fresh `rD`" (since `× 1` is a 0-bit
shift), and subsequent iterations get `slwi`/`mulli` against `rS` as
expected.

```cpp
// BEFORE (our build): hand-unrolled — '× 1' folds to direct stw rS:
//   stw r9, 0x10c(r4)   ; mBody[0]->mAnmCounter = frameDiff (uses r9 directly)
mBody[0]->mAnmCounter = frameDiff;
mBody[1]->mAnmCounter = frameDiff * 2;
...
mBody[7]->mAnmCounter = frameDiff * 8;

// AFTER (target): loop form — MWCC unrolls AND materializes × 1 via clrrwi:
//   clrrwi r6, r9, 0    ; r6 = r9 (fresh reg for × 1)
//   stw r6, 0x10c(r4)
//   slwi r5, r9, 1      ; × 2
//   stw r5, ...
//   mulli r0, r9, 0x3   ; × 3
//   stw r0, ...
for (int i = 0; i < 8; i++) {
    mBody[i]->mAnmCounter = frameDiff * (i + 1);
}
```

**Caveats.**

- Only works for **patterns where every iteration has a unique
  multiplier or scaled value derived from `i`**. If all iterations
  share an identical expression (`arr[i] = K;` with no `i` in the
  RHS), MWCC won't unroll — it'll keep the loop.
- The unroll trigger seems to require the loop body to be very small
  (one store) and the bound to be a compile-time constant.

**Citations.**

- `Enemy/BossHanachanAnm::TBossHanachan::setAnmTimerWhenSnort` (tick 128):
  hand-unrolled at **80.7%**; loop form `mBody[i]->mAnmCounter =
  frameDiff * (i + 1)` → **100%**. The matching pair `setAnmTimerWhenGetUp`
  (already at 100%) uses the *opposite* form — hand-unrolled DESCENDING
  body order — because its `× 1` slot (`mBody[6]`) is sandwiched between
  other already-emitted reg-materialized values, so the `stw r9` direct
  is the natural lowering there.

### Deferred pointer-fetch pattern: `T** pp = &arr[i]; (*pp)->m(); T* x = *pp;` defers the dereference past a virtual call

**Rule.** When target asm computes the array element ADDRESS via
`addi/add` (3-instruction sequence: byte-offset + base-add), and only
later — AFTER a virtual call — loads the dereferenced pointer with
`lwz rN, 0(rN)` (overwriting the same register that held the address),
the source must hold the **address-of-element** in a `**` local first
and dereference *after* the call. Plain `arr[i]->method()` folds the
address+load into a single indexed load (`lwzx`) with the loaded
pointer in a separate register, then re-loads it from memory at every
subsequent use (since the indexed load destination isn't necessarily
callee-saved).

```cpp
// BEFORE (our build): direct subscript → lwzx + reloads:
//   addi r0, r31, 0x150
//   lwzx r27, r25, r0      ; r27 = mBody[i] directly
//   bl setAnm_              ; r27 callee-saved survives but address path scattered
for (int i = 0; i < 8; i++) {
    mBody[i]->setAnm_(anmKind, blend);
    J3DFrameCtrl* fc = mBody[i]->mMActor->getFrameCtrl(0);  // mBody[i] reloaded
    f32 diff = unk194 - mBody[i]->getRotation().z;          // and again
    ...
}

// AFTER (target): deferred fetch — address held, value loaded post-call:
//   addi r29, r31, 0x150
//   add  r29, r25, r29     ; r29 = &mBody[i]
//   lwz  r3, 0(r29)        ; r3 = mBody[i] (transient — for the call)
//   bl setAnm_
//   lwz  r29, 0(r29)       ; r29 = mBody[i] (cached now, reusing r29)
//   ... uses of r29 ...
for (int i = 0; i < 8; i++) {
    TBossHanachanPartsBase** pp = &mBody[i];
    (*pp)->setAnm_(anmKind, blend);
    TBossHanachanPartsBase* part = *pp;       // deferred deref
    J3DFrameCtrl* fc = part->mMActor->getFrameCtrl(0);
    f32 diff = unk194 - part->getRotation().z;
    ...
}
```

**Diagnostic signature.** Target shows `addi rN, rOFF, 0xCONST; add
rN, rTHIS, rN; lwz r3, 0(rN); bl ...; lwz rN, 0(rN)` — the same
non-volatile reg holds the address pre-call and the dereferenced
pointer post-call.

**Citations.**

- `Enemy/BossHanachanAnm::TBossHanachan::setTumbleAnm` (tick 128):
  92.73 → **95.81%**. Direct `mBody[i]->setAnm_(...)` was generating
  `lwzx r27` with separate cache reg; the `&mBody[i]` + `*pp` rewrite
  recovers target's `add r29` + post-call `lwz r29, 0(r29)` shape.

### Rewrite `if (v == K1 || v == K2)` as `switch (v) { case K1: case K2: ... break; }` when target asm shows the switch-branch order

**Rule.** Per `CLAUDE.md` ("Switches" subsection), MWCC compiles some
switches into branching form: `cmpwi K_big; beq match; bge end; cmpwi
K_small; beq match; b end`. The `||` form of the same logic produces
the **reversed** test order (smallest first) because the disjunction
evaluates left-to-right. Rewrite as a `switch` with all keys as case
labels (no body per case, shared block) to recover the target's
"largest-first, range-guarded" emission order.

```cpp
// BEFORE (our build): tests 9 first, then 12:
//   cmpwi r0, 0x9
//   beq target_block
//   cmpwi r0, 0xc
//   bne end
//   target_block: bl isCurBckAlreadyEnd_; ...
if ((cur == 0x9 || cur == 0xC) && mHead->isCurBckAlreadyEnd_())
    result = true;

// AFTER (target): tests 0xC first, range-guard with bge:
//   cmpwi r0, 0xc
//   beq target_block
//   bge end           ; if cur > 0xc, no match
//   cmpwi r0, 0x9
//   beq target_block
//   b end
switch (cur) {
case 0x9:
case 0xC:
    if (mHead->isCurBckAlreadyEnd_())
        result = true;
    break;
}
```

**When to apply.** Any 2-3-way `if (v == K1 || v == K2 [|| v == K3])`
where target shows `cmpwi K_largest; beq; bge end; cmpwi K_next; beq; ...`.
Don't apply to single-test ifs or large disjunctions (MWCC chooses
the optimized "subtract-and-range" form per CLAUDE.md for those).

**Citations.**

- `Enemy/BossHanachanAnm::TBossHanachan::isFinishedGetUp` (tick 128):
  90.78 → **100%**. Two-key `||` rewritten as 2-case fall-through
  switch reproduces target byte-for-byte.
- `Camera/CameraChange::isChangeToParallelCameraByMoveBG_` (tick 138):
  90.32 → **100%**. The first 2-key `||` (`type == 0x400000BB ||
  type == 0x40000049`) rewritten as `switch (type) { case 0x400000BB:
  case 0x40000049: result = true; break; }`. Restored the `lis r3,
  0x4000; addi r0, r3, 0xbb; cmpw r4, r0; beq m; bge end; addi r0, r3,
  0x49; cmpw r4, r0; beq m; b end` chained-with-shared-base pattern.
  Diagnostic: keys share high bits (0x40000000) — when the disjunction
  was in `||` form, MWCC emitted the `subis r0, r4, 0x4000; cmplwi
  r0, 0xbb; beq m; cmplwi r0, 0x49; bne end` subis-relative form
  instead. Note that the SECOND switch in the same function (3+ cases)
  already produces the lis-hoisted form — so the lever is specifically
  about the 2-key disjunction.



**Rule.** When the target asm reads a single field TWICE in immediate
succession — once for a bit test (`rlwinm.`/`beq`), then again to
modify-and-store (`rlwinm`/`stw`) — MWCC's CSE would normally collapse
these to a single load. To prevent the CSE, the original source must
have used **two separate inline helpers** for the test and the modify,
not a direct `v & MASK` + `v &= ~MASK` pair. Each helper expands
into its own self-contained inline body, and MWCC treats the two
expansions as independent IR sequences without CSE'ing across them.

```cpp
// BEFORE (our build): direct ops → MWCC CSEs the load:
//   lwz r3, 0xf0(r30)
//   rlwinm. r0, r3, 0, 14, 14
//   beq ...
//   rlwinm r0, r3, 0, 15, 13      ; reuses r3
//   stw r0, 0xf0(r30)
//   bl startBGM
if (spine->getTime() == 200 && (boss->mLiveFlag & 0x20000)) {
    boss->mLiveFlag &= ~0x20000;
    MSBgm::startBGM(0x80010029);
    ...
}

// AFTER (target): inline-helper pair → MWCC reloads:
//   lwz r0, 0xf0(r30)             ; load #1 from checkLiveFlag
//   rlwinm. r0, r0, 0, 14, 14
//   beq ...
//   lwz r0, 0xf0(r30)             ; load #2 from offLiveFlag — REDUNDANT
//   lis r3, 0x8001                ; arg setup interleaved
//   addi r3, r3, 0x29
//   rlwinm r0, r0, 0, 15, 13
//   stw r0, 0xf0(r30)
//   bl startBGM
if (spine->getTime() == 200 && boss->checkLiveFlag(0x20000)) {
    boss->offLiveFlag(0x20000);
    MSBgm::startBGM(0x80010029);
    ...
}
```

The helper-pair lever also tends to fix instruction scheduling around
the modify-store-call sequence (the `lis/addi` arg setup gets
interleaved between the reload and `stw`, exactly as target).

**Diagnostic signature.** Target asm shows two consecutive `lwz rN,
OFF(rBASE)` of the **same** field — one before the bit test, one
right after the conditional branch, with no intervening function
call or memory write that could plausibly alias. CSE *should* have
collapsed them; the inline helper boundary breaks the CSE.

**Citations.**

- `Enemy/BossHanachanNerve::TNerveBossHanachanSnort::execute` (tick 126):
  93.5 → **99.5%** by replacing `boss->mLiveFlag & 0x20000` /
  `boss->mLiveFlag &= ~0x20000` with `boss->checkLiveFlag(0x20000)` /
  `boss->offLiveFlag(0x20000)`. Target shows `lwz r0, 0xf0(r30)`
  exactly twice; using the helper pair reproduces both loads.
- `Enemy/hamukuri::THamuKuri::moveObject` (existing match, ~99.94%):
  source already uses `offLiveFlag` + `checkLiveFlag` + `isAirborne`
  chain in sequence; target's asm shows the corresponding multiple
  consecutive loads of `mLiveFlag` (offset 0xf0) with no CSE. This
  is the supporting-evidence case — the helper-pair lever is in
  active use here and produces matching output.

**Where to try it next.** Any `lwz X, OFF(Y); rlwinm. ...; beq ...;
lwz X, OFF(Y); rlwinm/ori/...; stw X, OFF(Y)` pattern in target.
Common in `mLiveFlag`/`unk64` test-then-clear/set sequences across
Enemy nerves, MoveBG state machines, NPC behavior code. Both
`TLiveActor` (`checkLiveFlag`/`onLiveFlag`/`offLiveFlag`) and
`THitActor` (`checkHitFlag`/`onHitFlag`/`offHitFlag`) provide the
helper pair.

### Cache a pointer-chain receiver into a local before a call-argument call to force callee-saved allocation

**Rule.** When a method call has the shape `a->b->method(otherCall(...))`, MWCC
evaluates the receiver chain (`a->b`) right where it appears in the source —
**after** the argument call, which means it lives in r3/r4 and gets reloaded
later. If the target asm shows the receiver loaded into a **callee-saved
register (r28-r31) BEFORE** the argument call, the original source stored it
in a local so its live range crossed the call:

```cpp
// BEFORE (our build): receiver loaded after calcKeyCode, into r3:
//   bl calcKeyCode
//   lwz r5, instance@sda21(r0)
//   mr r4, r3
//   lwz r3, 0x4(r5)         ; mRootNameRef → r3 (no callee-save)
unk13C = (TMapObjBase*)JDrama::TNameRefGen::instance->mRootNameRef->searchF(
    JDrama::TNameRef::calcKeyCode("submarine"), "submarine");

// AFTER (target): mRootNameRef into r30 BEFORE bl calcKeyCode:
//   lwz r4, instance@sda21(r0)
//   lwz r30, 0x4(r4)         ; mRootNameRef → r30 (callee-saved)
//   bl calcKeyCode
JDrama::TNameRef* root = JDrama::TNameRefGen::instance->mRootNameRef;
unk13C = (TMapObjBase*)root->searchF(
    JDrama::TNameRef::calcKeyCode("submarine"), "submarine");
```

The local creates a sequence point that forces the receiver evaluation to
land before the argument's `bl`. The local's live range crosses the call, so
MWCC allocates a callee-saved register. The pattern repeats per call site;
reuse the same local (`root = ...;`) at the start of each block.

**Diagnostic signature.** Target prologue saves r28/r29/r30 etc.; the
receiver `lwz` lands BEFORE the `bl calcKeyCode`/`bl <arg>` instruction;
asm uses a callee-saved register as the `this` for the actual method call.

**Citations.**

- `MoveBG/MapObjRicco::TRiccoWatermill::loadAfter` (tick 124): 75.3 →
  **99.5%** by caching `mRootNameRef` before both searchF calls.
- `MoveBG/MapObjRicco::TFruitLauncher::loadAfter` (tick 124): 84.7 →
  **94.0%** by caching `mRootNameRef` before each of the two trailing
  searchF calls.

**Where to try it next.** Any `lookup`-style call chain through a global
singleton:
`SingletonGen::instance->root->searchF(calcKeyCode(name), name)` — the
JDrama name-ref pattern is ubiquitous across `loadAfter` and `init` of
TMapObjBase subclasses in MoveBG/, NPC/, Enemy/. Also `gpItemManager`,
`gpMapObjManager`, `gpStageEventManager` chains. If target asm preserves
something into r28-r31 across a `bl <arg-of-call>`, suspect this pattern.

### Same-TU `static const` class members are inlined at the use site — drop `const` to force the sdata load

**Rule.** When a class declares `static const T m_name;` and the definition
`const T C::m_name = literal;` lives **in the same TU** as the use site,
MWCC at `-O4,p` inlines the literal at every use:

```cpp
// In header:
class C { static const s32 mWaitTimeToFall; };
// In .cpp (same TU as the use site):
const s32 C::mWaitTimeToFall = 60;

void C::touch() { mLifeTimer = mWaitTimeToFall; }
//                            ^^^^^^^^^^^^^^^^ generates `li r0, 0x3c` then store
```

Target's asm instead reads from `.sdata` via `@sda21`:

```
lwz r3, mWaitTimeToFall__C@sda21
stw r3, 0x104(r31)
```

Drop the `const` on **both** the declaration and the definition; the
member becomes a plain mutable static, MWCC can no longer constant-fold,
and the read goes through `.sdata` exactly like the target:

```cpp
class C { static s32 mWaitTimeToFall; };       // no `const`
s32 C::mWaitTimeToFall = 60;                    // no `const`
```

**Diagnostic signature.** Target asm shows `lwz` / `lfs` from
`@sda21` for a class-scope static; our build shows the literal
materialised inline (`li r0, N`, `lfs ..., @anchor@sda21` of the
literal value, etc.). Symbols.txt is the smoking gun — if the
member shows up as `mFoo__Class = .sdata:...`, it's a runtime read,
which means the original code did NOT declare it `const`.

**Naming convention caveat.** SMS classes use an `m`-prefix for both
instance fields AND class-level statics (e.g. `mWaitTimeToFall__10TSandBlock`).
Don't assume `s` prefix just because it looks like a "static". Match
the symbol name from `symbols.txt`.

**Citations.**

- `MoveBG/MapObjBlock::TSandBlock::touchPlayer` (tick 122):
  89.5 → **99.8%** by dropping `const` on `mWaitTimeToFall` (also fixed
  the `s` → `m` prefix per symbols.txt). With `const`, MWCC emitted
  `li r0, 0x3c`; without, the sdata load matched target exactly.
- `MoveBG/MapObjBlock::TIceBlock::control` (tick 122):
  92.1 → **95.1%** by the same fix on `mMeltSpeedWater` / `mMeltSpeedAuto`
  / `mAutoMeltScale`. Multiple `lfs` uses of these floats picked up
  the sdata pointer instead of inline rodata anchors.

**Where to try it next.** Every TU that declares `static const T foo`
class members where `symbols.txt` lists the symbol in `.sdata` (not
`.sdata2`/rodata). Common in physics constants, gameplay tunables,
animation parameters across `Enemy/`, `MoveBG/`, `Player/` — anywhere
the original game programmer used class-scoped tweakable constants.

### Predicates that materialize a 0/1 BOOL force inline-then-test even for `&&` short-circuit

**Rule.** Direct comparisons like `if (sender->mActorType == 0x80000001u)`
or `if (mLiveFlag & 0x80)` compile to a **single** compare-and-branch
(`addis+cmplwi+bne` or `rlwinm.+beq` etc.). The target asm often shows
a **5-instruction materialization** instead:

```
addis r0, r6, 0x8000        ; compute mActorType + 0x80000000
cmplwi r0, 1                 ; compare with 1
bne after_false              ; if not equal, branch
li r0, 1                     ; bool = TRUE
b after
after_false: li r0, 0        ; bool = FALSE
after:
clrlwi. r0, r0, 24           ; mask to byte AND set CR0
beq fail_branch              ; if bool=0, fail
```

The source uses an inline helper whose body **explicitly** materializes
`0` or `1` via a ternary or if/else; calling the helper inlines this
pattern, and even the `&&` operator preserves the materialization
because the boolean result of the helper is already a 0/1 byte.

Two known helpers in this codebase use the pattern:

```cpp
// include/Strategic/HitActor.hpp
u8 isActorTypeOf(u32 base) const {
    u8 result;
    if ((mActorType - base) == 1) { result = 1; } else { result = 0; }
    return result;
}

// include/Strategic/LiveActor.hpp
bool isAirborne() const {
    return checkLiveFlag(LIVE_FLAG_AIRBORNE) ? 1 : 0;
}
```

**How to apply.** Look for the diagnostic signature in target asm: an
`addis`/`subis`/`rlwinm.` test followed by `bne+li 1+b+li 0` materialize
sequence, then `cmpwi r0, 0`/`clrlwi.`+conditional branch. Replace
direct `==`/`&` comparisons with the helper call:

```cpp
// BEFORE:
if (sender->mActorType == 0x80000001u) { ... }
// AFTER (note: helper is `mActorType - base == 1`, so pass base = 0x80000000):
if (sender->isActorTypeOf(0x80000000)) { ... }

// BEFORE:
if (mState == 0 && !(mLiveFlag & 0x80)) mState = 1;
// AFTER:
if (mState == 0 && !isAirborne()) mState = 1;
```

The `0x80000000` base for `isActorTypeOf` produces `addis r0, mActorType,
0x8000` (since `+0x80000000` == `-0x80000000` in 32-bit arithmetic). For
`0x01000000` base, it produces `subis r0, mActorType, 0x100`.

**Citations.**

- `MoveBG/MapObjItem2::TJumpBase::receiveMessage` (tick 120):
  88.72 → **99.94%** by replacing two direct `sender->mActorType ==
  0x{80,01}000001u` checks with `sender->isActorTypeOf(0x{80,01}000000)`.
  Each saves ~+40 bytes of code matching the materialize-then-test
  sequence (target 360B vs our previous 320B before the change).
- `MoveBG/MapObjItem2::TJumpBase::control` + `::TMushroom1up::control`
  (tick 120): 72.0 → 77.6% and 59.8 → 61.7% respectively by replacing
  `!(mLiveFlag & 0x80)` with `!isAirborne()`. The inline ternary
  `?1:0` materializes 0/1 from the bit test even inside an `&&`
  short-circuit.
- `MoveBG/MapObjBlock::TBrickBlock::receiveMessage` (tick 122):
  98.0 → **100%** by `sender->isActorType(0x08000005)` (the
  `mActorType == flag ? true : false` variant, not the `(mActorType
  - base) == 1` variant). For actor types where high+low halves both
  matter, the **exact-match `isActorType(flag)`** helper folds the
  comparison into the `subis r0, rN, hi; cmplwi r0, low` merged form
  that target uses (e.g. `subis r0, r3, 0x800; cmplwi r0, 5` for
  0x08000005); the `(mActorType - base) == 1` helper does NOT merge
  the constants for non-trivial bases like 0x80000800 → produces a
  three-insn `addis + subi + cmplwi 1` instead.

**Helper variant choice.** Two helpers in `THitActor` produce the
same materialize-then-test shape; pick by what the target asm
encodes:

- `isActorTypeOf(u32 base)` → `subis/addis + (optional subi) + cmplwi N+1`.
  Good when the asm shows a single `addis r0, rN, X; cmplwi r0, 1`
  (low part of comparand is 0).
- `isActorType(u32 flag)` → `subis + cmplwi low` merged whenever low
  is small. Good when the asm shows `subis r0, rN, hi; cmplwi r0, low`
  with a non-zero low half.

**Where to try it next.** Any function with target asm that does a
direct compare-and-branch shape replaced by the 5-instruction
materialize-then-test shape. Common indicators: `addis`/`subis` for
hi-bit comparisons (actor types, sentinel values); `rlwinm.` for
bit tests on flags. Look for `?1:0` style helpers on the containing
class — `THitActor::isActorTypeOf/checkActorType/isActorType`,
`TLiveActor::isAirborne/checkLiveFlag2`, etc.

### Multiple `return FALSE` paths share `li r3, 0` epilogue only under a positive `if`-block

**Rule.** A function shape like

```cpp
BOOL foo() {
    if (early-return-condition-A) return FALSE;
    if (early-return-condition-B) return FALSE;
    if (early-return-condition-C) return FALSE;
    // ... main body ...
    return TRUE;
}
```

generates **per-statement** `li r3, 0; b epilogue` blocks for each
early return — every `return FALSE` repeats the two instructions.
MWCC does **not** consolidate them across separate `if`-statements
when the function has any other code path (here, the `return TRUE`).

To force consolidation, wrap the body in a single positive `if`
covering all the conditions and put one `return FALSE` after it:

```cpp
BOOL foo() {
    if (cond-A) {
        if (!cond-B && !cond-C) {
            // ... main body ...
            return TRUE;
        }
    }
    return FALSE;
}
// or equivalently, with && short-circuit on false:
BOOL foo() {
    if (positive-A && positive-B && positive-C) {
        // ... main body ...
        return TRUE;
    }
    return FALSE;
}
```

Now each "fail" condition is the inverse short-circuit edge of `&&`,
and MWCC emits a direct conditional branch (`bge`, `bne`, etc.)
straight to the **single** `li r3, 0` at the function tail. Net
savings: ~8 bytes per saved early-return (the inline `li r3, 0; b end`
pair).

**Diagnostic signature.** Target asm shows several conditional
branches all targeting the same address ending in `li r3, 0; <epilogue>`,
without any `cror eq, gt, eq; beq/bne` short-circuit lattice in
between. Our build emits one `li r3, 0; b end` per `return FALSE`
statement.

**Citations.**

- `MoveBG/ModelGate::receiveMessage` (tick 118): 86.46 → **99.86%**
  after wrapping body in `if (sender->mActorType == 0x01000001u)
  { ... if (a && b && c) { ... return TRUE; } } return FALSE`.
  Target's three distance/z-range `bge` instructions all jump to the
  single `li r3, 0` at .L_801C3690.

**Where to try it next.** Any BOOL/bool function at <100% match with
multiple `if (cond) return FALSE` early-exits before a "do work and
return TRUE" body. Common in `receiveMessage`, `isXxx`, validator
predicates.

### `TVec3<f>::set(x, y, z)` — right-to-left arg eval batches stores at end

**Rule.** When target asm shows a TVec3 being written with the THREE
stores batched at the end (after all three RHS expressions have been
computed and held in registers across function calls), prefer
`vec.set(x_expr, y_expr, z_expr)` over component-by-component
assignment to a TVec3 local. MWCC evaluates function args
right-to-left at -O4,p, so:

- the **z arg** is computed FIRST (its bl-call result is saved across
  later arg compute via a non-volatile FPR)
- the **y arg** is computed SECOND (likewise saved)
- the **x arg** is computed LAST (result in volatile f0/f1)
- the inlined `.set` body then issues three back-to-back `stfs` in
  source-field order (x, y, z) to the addressed slot

```cpp
// BEFORE — interleaved load/store via stack local + final struct copy:
JGeometry::TVec3<f32> pos;
pos.x = (rand() * (1.0f / 32768.0f) * 200.0f + mPosition.x) - 100.0f;
pos.y = mPosition.y;
pos.z = (rand() * (1.0f / 32768.0f) * 200.0f + mPosition.z) - 100.0f;
mParticlePos[mParticleIndex] = pos;  // lwz/stw struct-copy

// AFTER — batched stfs to target slot directly:
mParticlePos[mParticleIndex].set(
    (rand() * (1.0f / 32768.0f) * 200.0f + mPosition.x) - 100.0f,
    mPosition.y,
    (rand() * (1.0f / 32768.0f) * 200.0f + mPosition.z) - 100.0f);
```

**Why.** The right-to-left arg-eval order means:
1. Compute the z-arg (one bl rand → arithmetic → result stored in f30 NV).
2. Compute the y-arg (load mPosition.y → stored in f31 NV).
3. Compute the x-arg (second bl rand → arithmetic → result in f0).
4. Inlined `.set` body: `stfs f0, 0(rA); stfs f31, 4(rA); stfs f30, 8(rA)`.

Component-by-component to a local pos forces interleaved compute/store
per-field, AND the final struct-copy to mParticlePos uses `lwz/stw`
(integer copy — see CLAUDE.md note on float fields copied as ints) which
is worse codegen for the consumer.

**Diagnostic signature.** Target shows the pattern: `bl X; ... lfs fN, mPos.y(this); bl X; ... stfs three values in field order`. If the
target keeps `f30`/`f31` alive across a `bl` between two argument
computations, that's the tell.

**Citations.**

- `MoveBG/MapObjTree::control` (tick 116): 97.96 → ... after .set() form
  applied. Together with `emit(..., this)` (4th arg) and `unk64 &= ~2`
  mask correction, lifted from 84.37% → 98.90%. Target keeps
  `lfs f31, 0x14(r31)` (mPosition.y) alive across the second bl rand;
  store batch `stfs f0/f31/f30, 0(r3)/4(r3)/8(r3)`.

**Where to try it next.** Any function that writes a TVec3 (or other
small POD) with values derived from `rand()` / `bl`-bearing
expressions interleaved with a simple load. Look for: `bl X; ... bl X;
... batched stfs to a field-addressed slot` in target.

### Two-term sum `unk3C * sinf(...) + unk40 * sinf(...)` — split each term into a named local to defeat fused fmadds

**Rule.** When target asm computes a sum-of-two-products as

```
fmuls fX, fA, fSin1       ; X = unk3C * sin1 (saved to NV-FPR)
... second sinf call ...
fmuls f0, fB, fSin2        ; 0 = unk40 * sin2
fadds f1, fX, f0          ; final = X + 0
```

but our build collapses the second multiply-add into a single
`fmadds f1, fB, fSin2, fX`, force separation by giving **both**
products an explicit local name:

```cpp
// BEFORE — MWCC fuses the second multiply into fmadds:
return unk3C * sinf(unk24 * (invTwoPi * x) + unk64)
     + unk40 * sinf(unk28 * (invTwoPi * z) + unk68);

// AFTER — separate fmuls + fadds:
f32 wave1 = unk3C * sinf(unk24 * (invTwoPi * x) + unk64);
f32 wave2 = unk40 * sinf(unk28 * (invTwoPi * z) + unk68);
return wave1 + wave2;
```

**Why.** With `a + b*c` MWCC's peephole sees a fused multiply-add
opportunity and emits `fmadds`. Naming `b*c` as a local (`wave2`)
materializes the multiplication as a distinct subexpression that
must be stored to its named slot first; the final `+` then has no
multiply opportunity left to fuse.

**Important:** naming only ONE term (e.g. `wave1`) is insufficient
— MWCC still fuses the unnamed second product. Both must be named
locals.

**Citations.**

- `MoveBG/MapObjWave::getWaveHeight` (tick 106): wave1+wave2 split
  took 94.69% → **100%**. Target uses `fmuls f31, f4, f1; ... bl
  sinf; fmuls f0, f0, f1; fadds f1, f31, f0`. Single-named wave1
  alone left fmadds (96-ish%); naming both wave1+wave2 cleaned the
  full sequence.
- `MoveBG/MapObjWave::getHeight` (tick 106): same split, 73.45 →
  78.37%. Side-benefit: also dropped a spurious `+ height` term
  and reduced FPR saves from 4 to 3 (stack 0x48 → 0x40).
- `MoveBG/MapObjWave::draw` (tick 106): split applied to both
  `y0` and `y1` inner-loop sin sums, 89.12% → 93.73%.

**Where to try it next.** Any function computing `coeff_a * f(...) +
coeff_b * g(...)` where target asm has `fmuls + fadds` but ours has
`fmadds`. Common in physics integrators, transform compositions, any
2-term linear blend. Even more likely if both terms are sin/cos.

### Inverted `||` form for shared-true assignment dedups duplicate assignment blocks

**Rule.** When source has the pattern "in two separate cases, run
the same assignment block; in the remaining case, run the ratio
branch", do **not** write it as nested `if-else` with duplicated
assignment blocks, and do **not** write it as `if (cond_a &&
cond_b) { ratio } else { assign }`. Target uses the
**disjunctive** form with the "either case" combined via `||`:

```cpp
// BEFORE — duplicates the assign block, generates 3 basic blocks
// (assign-inline, ratio, assign-fallthrough):
if (dist >= 0.0f) {
    if (bgType == 0x700) {
        unk3C = unk2C; unk40 = unk30;    // assign
    } else {
        ratio compute;                   // ratio
    }
} else {
    unk3C = unk2C; unk40 = unk30;        // assign (duplicate)
}

// AFTER — single assign block, target's exact basic-block layout:
if (dist < 0.0f || bgType == 0x700) {
    unk3C = unk2C; unk40 = unk30;        // assign (ONE place)
} else {
    ratio compute;                       // ratio
}
```

**Why.** The disjunctive `||` form short-circuits: `blt assign;
cmplwi bgType, 0x700; beq assign; b ratio; assign: ...; b end;
ratio: ...; end:`. The duplicated-assign form lays out two
separate assignment blocks, increasing function size and producing
a `bne` (combined `cror eq, gt, eq` for the `>= AND ==` test)
which is wrong shape. Note the **branch direction is inverted**:
`>=` becomes `<`, `==` becomes equality short-circuit to true
side, etc.

**How to apply.** Look at the target asm's first comparison after
the cond setup. If it's `blt label` jumping to the assignment
block, the source must start with `dist < 0.0f || ...` (the
disjunction with the FAIL side of `>=`). If it's `bge`, source
starts with `dist >= 0.0f || ...`.

**Citations.**

- `MoveBG/MapObjWave::updateHeightAndAlpha` (tick 106): rewriting
  the dist+bgType branch as `if (dist < 0.0f || bgType == 0x700) {
  assign } else { ratio }` (instead of the original `if (dist >=
  0.0f && bgType == 0x700) { assign } else { ratio }`) jumped from
  82.37% → 87.57%. The original used wrong logic (matching the
  WRONG branch as the assign side) AND wrong shape. Same fix
  applied to the alphaDist branch.
- `Camera/CameraInbetween::execCameraInbetween` (tick 142): rewriting
  the dx/dz xz-threshold warp check from nested if-else with two
  duplicated `CLBPolarToCross` call sites to `if (dx > 0.1f ||
  dz > 0.1f) { single call }` consolidated the warp to one site and
  matched target's stack frame (0x60 → 0x50). 84.6 → 87.4% on its
  own; +0.2pp more with the `!(>=)` abs lever. The eager-`||` form
  (dz always computed) was used because C++98 doesn't allow declaring
  `dz` inside the `||` RHS.

**Where to try it next.** Any function with target asm pattern
`fcmpo + blt asgn_label` followed by a second comparison whose
match-equal branch ALSO jumps to `asgn_label`. The shared label is
the smoking gun for `cond_a || cond_b`. Also flag any "elseif
chain with duplicated body" — likely should be `||`.

### Static-init order is reverse-of-include-order under `-inline deferred`

**Rule.** In a TU compiled with `-inline deferred`, the order MWCC
emits per-template-instantiation `__sinit` blocks is **reverse** of
the order in which the templates' classes are first declared
(typically by header includes). To make `JALList<X>::smList` (or
any other template static) initialize **first** in `__sinit`, move
the header that declares class `X` to the **last** position in the
include block.

```cpp
// BEFORE — JALList<MSBgm>::smList initialized LAST in __sinit
#include <MSound/MSoundBGM.hpp>    // declares MSBgm (first)
#include <MSound/MSSetSound.hpp>   // declares MSSetSoundGrp/MSSetSound
#include <JSystem/JAudio/JALibrary/JALModSe.hpp>   // declares JALSeMod* (last)

// AFTER — JALList<MSBgm>::smList initialized FIRST in __sinit
#include <MSound/MSSetSound.hpp>
#include <JSystem/JAudio/JALibrary/JALModSe.hpp>
#include <MSound/MSoundBGM.hpp>    // moved to last → its template fires first
```

**Why.** With `-inline deferred`, MWCC defers compilation of
template instantiations until after the TU body is parsed, then
processes them in reverse declaration-encounter order. The result
is that the LAST template referenced gets emitted FIRST in
`__sinit`.

**How to verify.** If `__sinit` is < 100% match with the diff
showing template names shifted in a rotation pattern (e.g. our
build's first template is target's second, target's last is ours
N-1, etc.), the order is rotated. Move the "missing-from-start"
template's header to the end of the include block.

**Citations.**
- `MSound/MSModBgm` (tick 104): moving `<MSound/MSoundBGM.hpp>` from
  position 2 to position 5 (last) of the include block flipped
  `JALList<5MSBgm>::smList` from `__sinit` end to start. `__sinit`
  97.64% → 100%, TU 91% → 97.2% fuzzy. Confirmed for a TU with
  `-inline deferred` set via the `MSound` lib block in `configure.py`.

**Where to try it next.** Any TU with `__sinit` matching < 100%
where the diff shows a one-off rotation in template instantiation
order. Common in MSound, Player, Strategic, System TUs (all
`-inline deferred` libs per `configure.py`).

### Bit-mask test (`if (v & MASK)`) vs extract-then-test (`if (b != 0)`) — prefer mask for direct CR0-only test

**Rule.** When target tests a byte/bitfield non-zero with `clrrwi.`
or `rlwinm.` (mask the bits, set CR0, branch) and our build emits
`srwi.`/`extrwi.` (shift/extract into register, set CR0, branch), the
source should test the mask directly rather than naming the extracted
byte first. Move the byte extraction inside the if-body where it's
actually used.

```cpp
// BEFORE — extract then test (emits extrwi. r5, r4, 8, 8):
u8 b2 = (v >> 16) & 0xff;
if (b2 != 0) {
    use(b2 + 1);
}

// AFTER — mask test only (emits rlwinm. r0, r4, 0, 8, 15):
if (v & 0x00FF0000) {
    use(((v >> 16) & 0xff) + 1);  // extraction deferred into body
}
```

Same for the high byte:

```cpp
// BEFORE — `srwi. r5, r4, 24`:
u8 b3 = (v >> 24) & 0xff;
if (b3 != 0 && otherCond) {
    u32 mod = b3 + 1;
    ...
}

// AFTER — `clrrwi. r0, r4, 24`:
if ((v & 0xFF000000) && otherCond) {
    u32 b3  = v >> 24;
    u32 mod = b3 + 1;
    ...
}
```

**Why.** Extract-form keeps the extracted value live in a register
across whatever follows, which constrains MWCC's allocator. Mask-form
only sets CR0 (the dot-suffix), leaving registers free; MWCC can
schedule the lazy extraction independently when the body needs it,
often producing better register coloring downstream.

**Citations:**

- `MSound/MAnmSoundNPC::startAnimSound` (tick 102): both b2 and b3
  tests rewritten to mask-form; 70.3% → 72.9%. Target asm: `clrrwi.
  r0, r4, 24` for high byte, `rlwinm. r0, r4, 0, 8, 15` for middle
  byte. Our extract-form was emitting `srwi. r5, r4, 24` and `extrwi.
  r5, r4, 8, 8` respectively.

(Second citation pending — applies wherever target's mask-test asm
matches the extract-form C source. Sweep targets: any `u8 b = (x >> N)
& 0xff; if (b != 0)` pattern where the value is only used once inside
the if.)

### Zero-fmadds rotation pattern: explicit `+ 0 * sin/cos` terms force fmadds with zero addend

**Rule:** When target's asm computes a 2D rotation as

```
lfs   f0, @zero@sda21     ; f0 = 0.0
fmuls f2, f0, fSin        ; f2 = 0 * sin (zero addend)
fmuls f0, f0, fCos        ; f0 = 0 * cos (zero addend)
fmadds f2, fA, fCos, f2   ; f2 = a * cos + (0 * sin)
fmadds f0, fB, fSin, f0   ; f0 = b * sin + (0 * cos)
```

instead of the simpler `fmuls f2, fA, fCos; fmuls f0, fB, fSin`, the
source has explicit `+ 0 * sin/cos` terms. Write the source as:

```cpp
f32 zero = 0.0f;
*outX = a * cosV + zero * sinV;
*outZ = -b * sinV + zero * cosV;
```

(or `pos.x += a * cosV + zero * sinV;` if accumulating into a Vec3).
MWCC does NOT constant-fold the `0 * x` away when written this way;
instead it emits both the zero-multiply and the fmadds, exactly
matching target's instruction sequence.

This is a real lever for matching the rotation-style code that
appears in chain physics (TSphereLink), particle systems, and any
"rotate vector by Z angle" helper that ends up looking like a
general 2D rotation matrix.

**Why:** MWCC sees the source `a * cos + 0 * sin` and treats the
multiplication of a runtime variable (sin) by literal 0 as a real
floating-point op (since 0 * NaN != 0 etc.). The constant-fold
pass keeps it. The `fmadds` opcode (a*b + c) is the natural way to
write `a*cos + (0*sin)` since the 0*sin term has already been
computed into a register.

**Citations:**
- `Enemy/BossHanachanSub.cpp::BHSCalcRevisionDistXZByRotateZ` —
  89.74% → 92.60% (tick 94). Pattern: `*outX = c * cosV + zero * sinV;`,
  `*outZ = -c * sinV + zero * cosV;`. Bare `*outX = c * cosV;` produces
  just fmuls with no fmadds — won't match.
- `Enemy/BossHanachanSub.cpp::setDegreeZAndRevisionPosXZ` —
  71.07% → 79.59% (tick 94). Same `+ zero * sinV/cosV` pattern in
  the position-update at function end. Combined with manual MsWrap
  expansion (separate lever) for the larger gain.

**Where to try it next:** Any rotation calculation in chain physics,
2D rotation helpers in particle systems (e.g., effect TUs), or any
`pos.x += a * cosV; pos.z += b * sinV` pair where target emits
fmadds instead of fmuls. Inspect asm for `lfs frX, @zero@sda21`
followed by `fmuls frY, frX, frSin` — that's the signature.

### Manual expansion of `MsWrap` while loops defeats forced bl-emit

**Rule:** When `MsAngleWrap(angle)` (which calls inline template
`MsWrap<f>(angle, 0.0f, 360.0f)`) gets emitted as `bl MsWrap<f>__Ffff`
in our build but target inlines the while loops, manually inline
the body in source:

```cpp
// Before — emits bl MsWrap<f>__Ffff + weak instance:
baseAngle = MsAngleWrap(baseAngle);

// After — inlined while loops match target:
while (baseAngle >= 360.0f)
    baseAngle -= 360.0f;
while (baseAngle < 0.0f)
    baseAngle += 360.0f;
```

MWCC inlines the manually-written while loops but apparently chose
not to inline the template instance through `MsAngleWrap` →
`MsWrap<f>`. Manual expansion produces identical inlined code to
target's preference and removes the weak `MsWrap<f>__Ffff` symbol
from the TU's .text.

**Citations:**
- `Enemy/BossHanachanSub.cpp::setDegreeZAndRevisionPosXZ` —
  71.07% → 76.81% (tick 94, before zero-fmadds polish). The remaining
  `bl MsWrap<f>__Ffff` and the weak `MsWrap<f>__Ffff` body in our TU
  both went away. Target's setDegreeZ has the wrapping while loops
  inlined at the same location.

**Where to try it next:** Any TU showing a weak `MsWrap<f>__Ffff` or
`MsWrap<l>__Flll` symbol in its `.text` while target asm has neither.
Common in nerve-execute functions that wrap an angle and in chain
physics that wrap rotation degrees.

### Weak destructor emission in MarNameRefGen TUs requires correct base class + inline derived ctor

**Rule:** When a TU emits a `bl __ct__Foo` for `new Foo(...)` (where
`Foo` has out-of-line ctor declared but inline-defined body, e.g. in
its header as `Foo(const char* name) : Bar(name) {}`), MWCC also
emits `__dt__FooFv` as a weak symbol in that TU. The inline ctor's
visible inheritance is the trigger — MWCC needs the parent's vtable
slot for the derived `Foo`'s vtable, which forces emission of the
derived dtor weak.

Conversely, when `Foo` is a header-declared class whose ctor is
out-of-line (declared `Foo(const char*);` only) AND `Foo` has no
derived classes with inline ctors in the current TU, MWCC will NOT
emit `__dt__FooFv`. The call `bl __ct__Foo` and any vtable refs
become external dependencies.

**To trigger weak emission of a base class's dtor:** add a derived
class with inline ctor in the same TU. The simplest pattern is a
local placeholder class:

```cpp
class TMewManager : public TAnimalManagerBase {
public:
    TMewManager(const char* name) : TAnimalManagerBase(name) {}
};
// ... and use it for `new TMewManager(...)`.
```

If the derived class exists in a header but with an out-of-line
ctor, promote the ctor to inline body in the header (verify
symbols.txt has no `__ct__Derived` symbol first — if absent, the
original code was inline).

**Important:** if the local class uses `: TSpineEnemy(name) {}` but
the TARGET's asm calls `bl __ct__11TSmallEnemyFPCc` after a vtable
write to TSmallEnemy, the placeholder must inherit TSmallEnemy
(NOT TSpineEnemy). The vtable cascade in the dtor reveals the
correct base chain — read the dtor's `lis r3, __vt__X@ha` sequence.

**Citations:**
- `System/MarNameRefGen_BossEnemy` — TBEelTears dtor emitted by
  switching TOilBall (TNameRef → TBEelTears) with inline ctor. TU
  88.21% → 93.54% in tick 87, then 93.54% → 96.41% in this tick.
- `System/MarNameRefGen_Enemy` — TPakkun dtor 81.82% → 100% by
  changing base from TSpineEnemy → TSmallEnemy (match dtor cascade).
- `System/MarNameRefGen_Enemy` — TTobiPuku family (4 dtors) +
  MoePuku derived chain. TU 87.35% → 89.60%.
- `System/MarNameRefGen_Enemy` — TNameKuriManager dtor via inline
  TDiffusionNameKuriManager ctor promotion (NameKuri.hpp). TU →
  90.83%.
- `System/MarNameRefGen_Enemy` — TGesso dtor via inline TSurfGesso
  / TLandGesso ctors (Gesso.hpp). TU → 92.72%.
- `System/MarNameRefGen_Enemy` — TAnimalManagerBase dtor via inline
  TMewManager ctor (AnimalManager.hpp). TU → 93.79%.
- `System/MarNameRefGen_Enemy` — TSimpleEffect dtor via inline
  TSimpleEffect/TEffectPinnaFunsui/TEffectBiancoFunsui ctors
  (EffectObj.hpp). TU → 96.96%.

**Where to try it next:** Every other `MarNameRefGen_*` TU
(MarNameRefGen_MapObj, _NPC, _Map) has many missing weak dtors with
the same pattern. Survey the strcmp branches → for each `new T(...)`
where `T` has a header-out-of-line ctor + no derived class in TU,
check target asm for which derived classes are constructed (look for
`bl __ct__TBase` immediately followed by `lis r3, __vt__TDerived`).
Add a local placeholder OR promote header ctor to inline.

### `TVec3<f32>::set<f32>(x, y, z)` template form batches float loads

**Rule:** When source needs to copy three float values from a base
pointer (e.g. `mtx[i][3]`) into a `TVec3<f32>` member, writing it as

```cpp
mTipPos.set<f32>(mtx[0][3], mtx[1][3], mtx[2][3]);
```

(template `set` with **explicit `<f32>` argument**) produces the
batched pattern:

```
lfs f2, 0x2c(rN)    ; load z first (reverse-order load)
lfs f1, 0x1c(rN)    ; load y
lfs f0, 0xc(rN)     ; load x
stfs f0, X+0(rDest) ; then batched stores
stfs f1, X+4(rDest)
stfs f2, X+8(rDest)
```

The three lfs go to f0/f1/f2 separately so MWCC's scheduler can
interleave them with surrounding ALU work, and stores follow in
order. This contrasts with the **component-by-component** form

```cpp
mTipPos.x = mtx[0][3];
mTipPos.y = mtx[1][3];
mTipPos.z = mtx[2][3];
```

which compiles to interleaved `lfs f0; stfs f0` x3 using a single
FPR, with NO scheduler reordering. Target's batched form is the
common case when 3 floats are copied from a base pointer.

**How to identify:** Target asm shows 3 `lfs frX, off(rN)` to three
different FPRs (f0/f1/f2 typically), in reverse offset order, then
3 `stfs frX, off(rDst)` in normal order. Component-by-component
source produces a single FPR (`lfs f0; stfs f0` x3) and won't
match.

**Why the `<f32>` template arg matters:** `TVec3<f32>::set` is a
template:
```cpp
template <class TY> void set(TY x_, TY y_, TY z_) {
    x = x_; y = y_; z = z_;
}
```
Without the explicit `<f32>`, deduction picks `TY` from arg types.
The 3-arg `set()` overload (non-template `set(f32, f32, f32)`)
does NOT exist — only the template. So `mTipPos.set(a, b, c)`
deduces `TY=f32` and produces the same template instance as the
explicit form. The two forms SHOULD be identical — pending check
of whether the explicit-template-arg form is necessary or just a
stylistic preference. (Open question.)

**Caveat — supersedes CLAUDE.md claim:** CLAUDE.md "TVec3 / Vector
Codegen Patterns" says ".set(x, y, z) (3-arg form) — generates
lfs/stfs like component assignment". That is INCORRECT for the
mtx-load case. The 3-arg template `set` produces BATCHED loads,
not interleaved. The component-by-component form is what produces
interleaved loads. Test with both before committing.

**Citations:**
- `Enemy/DebuTelesa.cpp::calcRootMatrix` — 83.82% → **99.88%**
  (tick 82). Switched `mTipPos.x = mtx[0][3]; mTipPos.y = ...;
  mTipPos.z = ...;` to `mTipPos.set<f32>(mtx[0][3], mtx[1][3],
  mtx[2][3])`. Combined with `getCurrentNerve` → `getLatestNerve`
  fix that restored 4 inlined-fallback instructions. Remaining
  0.12% is just stack-temp slot allocation order (different
  unsolved MWCC quirk).

**Where to try it next:** Any `TVec3` member being filled from
three matrix-cell or struct-field reads using component assignment,
where target shows batched lfs to multiple FPRs. Especially in
`calcRootMatrix`, position-emit, and matrix-derived position
patterns across the Enemy module.

### `bool` return type (not `BOOL`) for pointer-nonnull tail when target lacks `clrlwi` narrow

**Rule:** When a function's tail in target asm is the branchless
pointer-to-int idiom

```
lwz   r0, OFF(rThis)
neg   r3, r0
subic r0, r3, 0x1
subfe r3, r0, r3        ; result directly in r3 — NO clrlwi
blr
```

the source returns **`bool`**, not `BOOL`. Declaring the function
as `BOOL` makes MWCC route the bool intermediate (`p != nullptr`)
through a clrlwi-narrowing step:

```
subfe r0, r0, r3        ; into r0
clrlwi r3, r0, 24       ; narrow into r3  <-- extra insn
blr
```

The C++ semantics are identical (both return 0/1), but the
return-type's "width semantics" controls whether MWCC bothers
narrowing the bool intermediate before placing it in the
return-value register.

**Mirror rule:** When target's tail HAS the `clrlwi r3, r0, 24`
(e.g. `beakHeld__10TBossGessoCFv`), the source returns `BOOL`.
Both forms exist in the original — pick the one whose tail matches.

**Why:** MWCC treats `bool` as "byte-wide" but propagates that bit
directly when the destination is also bool-typed. Casting `bool` →
`BOOL` (int) inserts an explicit "narrow-to-byte then zero-extend"
sequence (the clrlwi). When the destination is `bool`, no
narrow needed — the subfe writes directly to r3.

**How to identify:** look for any function currently declared
`BOOL` that returns a runtime bool expression (`p != nullptr`,
`x != 0`, `(BOOL)(...)` cast on bool). Diff vs target — if target's
tail is `subic; subfe r3, r0, r3; blr` (no clrlwi), flip the
declared return type to `bool` in both header and `.cpp`.

**Citations:**
- `Enemy/BathtubBinder.cpp::init` — 98.58% → **100%** by changing
  `BOOL init(...)` to `bool init(...)`. Body was
  `return (BOOL)(mBathtub != nullptr);`. Removed clrlwi.
- `Enemy/Amenbo.cpp::isCollidMove` (already matching) — declared
  `bool`, body `return param_1 != this;`. Target tail is
  `subic; subfe r3, r0, r3; blr` — confirms the rule.
- `Enemy/bossgesso.cpp::beakHeld` (already matching) — declared
  `BOOL`, body `return !!mBeak->mHolder;`. Target tail HAS
  `clrlwi r3, r0, 24` — confirms the mirror.

**Where to try it next:** any other `BOOL fn() { return expr;`
where `expr` is a bool-typed runtime expression (pointer-nonnull,
inequality test) currently sitting near-100% with a single
trailing clrlwi diff.

**Caveat:** Only applies when the bool expression is computed at
runtime. Functions whose body is `return true;` / `return false;`
already emit `li r3, 0x1` / `li r3, 0x0` without any narrow, so
the return type doesn't matter for matching there.

### Explicit `Base::method()` qualifier in fabricated forwarders suppresses virtual dispatch

**Rule:** A fabricated forwarder method like

```cpp
TFooParams* getSaveParam2() const {
    return (TFooParams*)TSpineEnemy::getSaveParam();   // direct call
}
```

is compiled by MWCC to a single `bl getSaveParam__11TSpineEnemyCFv`.
Dropping the explicit qualifier:

```cpp
TFooParams* getSaveParam2() const {
    return (TFooParams*)getSaveParam();                // virtual call
}
```

restores virtual dispatch and emits the 4-instruction vtable
sequence `lwz r12, 0(this); lwz r12, VTOFFSET(r12); mtlr r12; blrl`.

This is just C++ semantics (explicit qualification suppresses
virtual), but in matching contexts it's the most impactful one-line
lever found this quarter: many "fabricated" save-param accessors
across the Enemy module use the qualifier-disabled form while target
asm uses the virtual form.

**Secondary effect — CSE behavior flips:** MWCC treats direct calls
to `const` methods as pure and CSE's repeated calls in the same
expression. Virtual calls are NOT CSE'd (the dispatch could differ).
So `getSaveParam2()->a + getSaveParam2()->b` becomes 1 call with
direct, 2 calls with virtual. If target shows the 1-call form
(direct + CSE) but the caller you're editing now has multiple
calls (post-flip), cache to a local:

```cpp
TFooParams* params = getSaveParam2();
x = params->a + params->b;
```

**Citations:**
- `Enemy/launcher::resetLaunchTimer` 82.86 → **100%** (tick 76). Same
  4-line vtable diff vanished. Plus init, stateLaunch, stateDie,
  CommonLauncher::stateDie all reached 100%.
- `Enemy/walkerEnemy` TU 97.75 → **99.71%** (tick 76). moveObject,
  reset, isResignationAttack, behaveToFindMario, multiple
  TNerveWalker::execute fns improved.
- `Enemy/smallEnemy` TU 90.99 → **92.91%** (tick 76). init 78→92%,
  reset 65→77%. Two callers needed the local-cache fix per CSE
  caveat above.

**Where to try it next:**
- Any other `(TFoo*)TBase::method()` forwarder in `include/`.

**Don't apply if** target asm shows the direct `bl funcName` form
(no vtable lookup). Verify before flipping.

**Refuted on Hinokuri2** (tick 78): Hinokuri2.hpp:165 had a forwarder
named `getSaveParam` (same name as the base virtual). Renamed to
`getSaveParam2`, dropped qualifier, updated 29 call sites in
hinokuri2.cpp + 6 in the header. **Net regression:** TU 99.23% →
99.18%. Per-fn breakdown: 3 functions gained (perform_Mask +0.18,
moveObject +0.05, execute_PrePol +0.79), 6 functions lost (reset
-0.08, execute_GraphWander/Pollute/Damage -0.04 each,
execute_Burst -2.46, execute_Stamp -0.07). Reverted. The Burst
regression came from register-coloring reorder (r30↔r31 swap),
not from CSE/local-cache issues — caches wouldn't fix it. Conclusion:
in TUs with very many call sites (~30+) and complex nerve-execute
schedules, the qualifier-drop's NV-register-allocation side effect
becomes a coin-flip per function. Apply this only where ALL or
MOST callers visibly need virtual dispatch AND you've audited
each affected function for register-order survivability — or skip
the TU entirely. Hinokuri2 is now in the "skip" category.

---

### `BOOL` (typedef int) return type on inline helpers avoids `clrlwi.` narrowing and changes instruction scheduling

**Rule:** Changing an `inline` helper's return type from `bool` to
`BOOL` (typedef int) eliminates MWCC's bool-narrowing cast at the
caller and can simultaneously re-schedule surrounding loads. Two
distinct symptoms appear together:

1. **Bool-narrowing test:** When the caller does
   `if (inlineHelper(...)) { ... }`, target emits `cmpwi rN, 0x0; beq`
   after the helper's final `mfcr/extrwi` sequence; our `bool`-return
   version emits `clrlwi. rN, rN, 24; beq` (clear-left-24 with record).
   Both work but only the `cmpwi` form matches target.
2. **Instruction-scheduling side-effect:** Changing the return type
   *also* lets MWCC delay one or more `lfs` loads past intervening
   `fsubs` operations. Confirmed on `insideCylinder` (areacylinder):
   the `lfs f0, 0x18(r3)` load for `cyl->unk18` moved from BEFORE
   `fsubs f2, f3, f2` (dx computation) to AFTER it — matching target
   schedule — purely by flipping `bool` → `BOOL` on the helper.

**How to identify the target pattern:**
- Target asm shows `cmpwi rN, 0x0; beq` after `mfcr; extrwi rN, rN, 1, 2`.
- Our build shows `clrlwi. rN, rN, 24; beq` at the same position.
- Helper currently declared as `bool` and called from `if (helper(...))`.

**Source that matches (BOOL form):**
```cpp
static inline BOOL insideCylinder(...) {
    if (cond) return FALSE;
    ...
    return (someCmp);  // bool->BOOL implicit conversion
}
```

**Where observed:**
- `Enemy/areacylinder.cpp::contain__20TAreaCylinderManagerFRCQ29JGeometry8TVec3<f>`
  — 94.72% → **99.82%** by changing `static inline bool insideCylinder(...)`
  to `static inline BOOL insideCylinder(...)` and `return false;` to
  `return FALSE;`. Both the `clrlwi.` → `cmpwi` flip AND the `lfs f0, 0x18`
  scheduling flip happened together.
- `Enemy/areacylinder.cpp::getCylinderContains__20TAreaCylinderManagerFRCQ29JGeometry8TVec3<f>`
  — 94.68% → **99.88%** by the same change to the same helper.

**Mechanism (hypothesis):** MWCC's bool type triggers an extra
"narrow to bit" semantic at every implicit `bool` → `int` conversion
point. The narrowing inserts a `clrlwi.` *and* counts as an extra
node in the dependency graph, perturbing the instruction scheduler's
choice for surrounding `lfs` loads. `BOOL` (typedef int) skips both.

**Where to try it:** Any inline helper currently returning `bool`
whose target asm shows `cmpwi rN, 0x0; beq` after `mfcr/extrwi` while
our build shows `clrlwi. rN, rN, 24; beq`. Especially common in
small predicate helpers called from `if (helper(...))` loops.

### Disjunction (`||`) merges two `return false` early-exits into one shared fail block

**Rule:** When a helper has multiple sequential guard clauses each
producing `return false`, MWCC emits a separate `li rN, 0; b end`
fail block for each. Merging the guards into a single `||`
disjunction produces ONE shared fail block reached by `blt`/`bge`
from each compare — matching target's typical schedule for inline
predicate helpers.

```cpp
// ours: 2 fail blocks
if (cond1) return false;
if (cond2) return false;

// target shape: 1 shared fail block
if (cond1 || cond2) return false;
```

**Where observed:**
- `Enemy/areacylinder.cpp::insideCylinder` y-range check — 90.6% → 94.7%
  on the two outer functions (contain, getCylinderContains) by merging
  `if (pos.y < cyl->unk14) return false; if (cyl->unk14 + cyl->unk20 < pos.y) return false;`
  into `if (pos.y < unk14 || unk14 + unk20 < pos.y) return false;`.
- `Map/MapArea.cpp::checkLinesCollision` (tick 134) — 92.1% → 99.9%
  by merging two `if (A && B) return false; if (C && D) return false;`
  into a single `if ((A && B) || (C && D)) return false;`. Confirmed
  the second test is laid out between the first test's `beq` and the
  shared `li r3, 0; b end` block, with the second test's polarity
  auto-inverted to fall through correctly.

**Variant — single-bool materialization via De Morgan on `&&` chains.**
A predicate body of the form

```cpp
return a <= px && px <= c && b <= pz && pz <= d;
```

inlined into a caller materializes an **incremental** boolean (one
`li rN, 0/1; clrlwi. rN; bne next_pair` per pair of conjuncts: r0
for the first pair, r4 for the third, r3 for the fourth). Rewriting
to a De Morgan'd `||` chain:

```cpp
if (!(a <= px) || !(px <= c) || !(b <= pz) || !(pz <= d))
    return false;
return true;
```

forces a **single** boolean: one shared `li r0, 0` fail block and
one `li r0, 1` pass block, with `cror eq, lt, eq; bne FAIL` per
conjunct (preserving the `<=` lowering instead of switching to
`bgt`). The operator must remain `<=` (negated by `!`) — direct
rewrite to `>` (`if (a > px || ...) return false`) compiles to
`bgt` instead of `cror eq, lt, eq; bne` and loses the per-conjunct
match.

- `Map/MapArea.cpp::pointIsInGrid` (tick 134) — 72.3% → 81.0% TU
  by the De Morgan rewrite. Per-conjunct codegen now matches target
  exactly; remaining diff is register coloring (target uses f0/f3,
  we use f5/f6 — propagates through subsequent uses).

**How to identify:** Target shows `blt fail_label` falling straight
into the next test, where `fail_label` is the SAME `li rN, 0; b end`
block reached by both `blt` from the first test AND `bge` from the
second. Our build has two distinct fail blocks instead.

For the De Morgan variant: target emits exactly *two* `li rN, X` (one
0, one 1) at the end of the inlined predicate body, with `cror eq, lt, eq; bne`
per conjunct. Our build emits multiple `li rN, X` pairs interleaved
between conjunct groups.

### Bind a global pointer to a typed local before a method call to emit `lwz rTMP; mr r3, rTMP` instead of `lwz r3, gFoo@sda21`

**Rule:** When source writes `Foo::sInstance->method(args);` directly,
MWCC emits `lwz r3, sInstance@sda21; bl method` — loading the global
straight into the arg register. Two source patterns produce the
*indirect* form `lwz rTMP, sInstance@sda21; mr r3, rTMP; bl method`
(one extra `mr` instruction):

1. **Explicit typed local:** `Foo* p = Foo::sInstance; p->method(args);`
2. **getInstance() wrapper:** `Foo::getInstance()->method(args);`
   when `getInstance()` is an inline `static T* getInstance() { return sInstance; }`.

As a *secondary effect* the first lever frequently also flips the
encoding of the preceding `this`-save from `mr r31, r3` to
`addi r31, r3, 0x0`. The two effects usually appear together — when
the target uses `addi rN, r3, 0` for the this-save AND has the extra
`mr r3, rTMP` pattern, declare the local.

**How to identify the target pattern:**
- Target asm contains `lwz rN, gFoo@sda21; mr r3, rN; bl method` where
  `rN` is *not* r3 (typically r0 or another scratch).
- For the typed-local lever, target's stack-prolog uses
  `addi r31, r3, 0x0` rather than `mr r31, r3` to save the implicit `this`.

**Source that matches (typed-local form):**
```cpp
TFlagManager* fm = TFlagManager::smInstance;
if (fm->getBool(0x10384)) { ... }
```

**Where observed:**
- `Map/MapEventDolpic.cpp::watch__22TDolpicEventBiancoGateFv` — 88.93% → **100%**
  by declaring `TFlagManager* fm = TFlagManager::smInstance;` before the
  `if (fm->getBool(0x10384))`. Both the extra `mr r3, r0` and the
  `mr r31, r3` → `addi r31, r3, 0x0` flip happened in lockstep.
- `GC2D/CardLoad.cpp::setupScoreScreen__9TCardLoadFv` — already matches
  via the `getInstance()` form (`TFlagManager::getInstance()->getFlag(0x40000)`),
  emitting `lwz r0, smInstance@sda21; mr r3, r0; bl getFlag` as expected.
  Confirms the rule from the second source pattern.

**Caveat 1 — context dependent:** Watch__Ricco in the same TU uses
`if (!TFlagManager::smInstance->getBool(unk2C))` with target emitting
the *direct* `lwz r3, smInstance@sda21` pattern — no local needed.
Only apply when the target's asm shows the indirect `lwz rN; mr r3, rN`
shape.

**Caveat 2 — the addi/mr flip is a side-effect, NOT a control knob.**
The typed-local extraction couples `mr r31, r3 → addi r31, r3, 0` to
the lwz-indirect change. They flip together. Some target functions
have `lwz r0; mr r3, r0` AND `mr r31, r3` (the standard `this`-save).
Applying the local-extract lever to such a function will flip the
`mr r31, r3` to `addi r31, r3, 0` — *regressing* the encoding. Verify
the target's `this`-save form before applying:
- target `addi r31, r3, 0x0` + indirect-lwz → apply lever
- target `mr r31, r3` + indirect-lwz → SKIP (lever breaks encoding)

Observed in `JSystem/JAudio/JALibrary/JALModSe.cpp::processModDistVolume`:
the indirect lwz was already produced *without* the local. Adding the
typed local extraction regressed match from 97.56% → 95.12% by
flipping `mr r31, r3` → `addi r31, r3, 0x0`. Reverted.

**Applied successfully** (committed wins this rule has produced):
- `Map/MapEventDolpic.cpp::watch__22TDolpicEventBiancoGateFv` 88.93→100%
- `MoveBG/Item.cpp::taken__9TCoinBlueFP9THitActor` 96.63→99.86%
- `MoveBG/MapObjMare.cpp::calc__9TMareFallFv` 93.44→99.90%
- `Enemy/smallEnemy.cpp::changeOut__11TSmallEnemyFv` 89.85→95.49%
- `Enemy/hamukuri.cpp::attackToMario__13THaneHamuKuriFv` 88.75→99.82%
- `Player/MarioSound.cpp::startSoundActor__6TMarioFUl` 90.72→99.72%
- `System/Application.cpp::checkAdditionalMovie__12TApplicationFv` 99.06→99.93%
  (this one extracted a struct field, not a global — the lever generalises
  to any single-use value that flows into an arg register)
- `GC2D/Guide.cpp::resetObjects__6TGuideFv` 89.89→92.50%
  (partial: only the 4 ternary-RHS smInstance refs converted to getInstance();
  the other 7 non-ternary refs stay direct because target wants them direct)

### `inline static void dummy(Vec* v) { *v = (Vec){...}; }` emits a `Vec` rodata constant *without* a `.text` symbol

**Rule:** A compound-literal `(Vec){a,b,c}` inside an `inline static`
function body causes MWCC to emit `a`, `b`, `c` as a 12-byte
rodata constant — *even though the inline function is never called
and its body is fully DCE'd from `.text`*. Use this as the canonical
"infectious" lever for materialising unreferenced `Vec`/`TVec3` rodata
constants that the target ships but our build is missing.

The form `static` (without `inline`) ALSO emits the rodata constant
but additionally leaves the dummy as a visible local function in
`.text`, which is the wrong shape for matching most TUs.

```cpp
// emits {0,0,0} into .rodata; no .text symbol
inline static void dummy(Vec* v)  { *v = (Vec){ 0.0f, 0.0f, 0.0f }; }
// emits {1,1,1} into .rodata; no .text symbol
inline static void dummy2(Vec* v) { *v = (Vec){ 1.0f, 1.0f, 1.0f }; }
```

**Where observed:**
- `MoveBG/MapObjManager.cpp:33-34` — original use of the trick;
  ships `{0,0,0}` (`@2760`) and `{1,1,1}` (`@2762`) in target's
  rodata at `.rodata:0xE0/0xEC`, unreferenced from `.text`. Note:
  the source there uses plain `static` (not `inline static`); our
  build does also emit a visible `dummy/dummy2` function as a
  result. Worth a future cleanup test (does adding `inline`
  break MapObjManager's match?).
- `Map/MapEventSirena.cpp` — adopted **`inline static`** form,
  added after `M3DUtil/InfectiousStrings.hpp` include. Emits
  `{0,0,0}`/`{1,1,1}` at `.rodata:0xE0/0xEC` matching target,
  with NO dummy/dummy2 symbols in `.text`. Promoted from Open
  questions (tick 70 closed the mystery).

**How it works (mechanism, partly confirmed):**
The compound literal `(Vec){a,b,c}` is treated by MWCC as a Vec
*aggregate constant* — it materialises the 12 bytes into rodata
at parse time. `*v = literal` then becomes `lwz/stw` copies from
the rodata block. When the function is `inline` and never called,
MWCC DCEs the body but keeps the rodata (since the rodata isn't
attributable to the function symbol; it sits in the TU's anonymous
rodata pool).

**Caveats:**
- Position of dummies in source does NOT cleanly determine rodata
  ordering. MWCC numbers rodata symbols by an internal scheme
  that depends on context (parsing event order, possibly per-TU
  state), not strictly by source line. Attempting to use this in
  `MapEventSink.cpp` (added dummies after `InfectiousStrings.hpp`,
  same as MapObjManager.cpp/MapEventSirena.cpp) placed the new
  zero/ones constants at `.rodata:0xC/0x18/0x24/0x30` —
  *before* the `MtxCalcType` strings — making the layout wrong
  for that TU. The same source-text-level recipe does NOT
  guarantee the same rodata ordering across different TUs.
- For TUs where the dummies land in the wrong rodata position,
  the lever is *not* useful as-is — you need a different way to
  push the constants past the MtxCalcType strings, or a separate
  IMPLEMENTATION strategy.

### Hoist a struct-field read into a local INSIDE a loop, BEFORE a function call, to lock it into a NV-FPR across the call

**Rule:** When the loop body needs `obj.field` *after* a `bl` call but the
field's address is the same each iteration, reading it into a local
**before** the `bl` (still inside the loop) forces MWCC to allocate a
callee-saved FPR (`f31` etc.) for it, so the value survives the call.

Without the explicit local, MWCC re-reads `lfs frV, off(rN)` *after*
the call returns, paying a load per iteration but freeing the NV-FPR.
With the local declared just before the `bl`, MWCC computes the load
*before* the call and keeps the value in `f31`, then stores from `f31`
after the call. This shifts one NV-FPR slot to the field, increasing
the function's NV-FPR count by one (which itself can match or mismatch
the target — verify the count first).

```cpp
// Before — MWCC reloads center.y after cosf returns:
for (int i = 0; i < N; ++i) {
    f32 sinAngle = sinf(angle);
    f32 z        = center.z + drawRadius * sinAngle;
    f32 cosAngle = cosf(angle);            // <-- bl
    f32 x        = center.x + drawRadius * cosAngle;
    GXPosition3f32(x, center.y, z);        // center.y reloaded here
}

// After — MWCC pins center.y into f31 across the cosf bl:
for (int i = 0; i < N; ++i) {
    f32 sinAngle = sinf(angle);
    f32 z        = center.z + drawRadius * sinAngle;
    f32 cy       = center.y;               // <-- hoist BEFORE the bl
    f32 cosAngle = cosf(angle);
    f32 x        = center.x + drawRadius * cosAngle;
    GXPosition3f32(x, cy, z);
}
```

**How to identify the target pattern:**
- Target's loop body has `lfs frV, off(rN)` to a callee-saved FPR
  (f24-f31) *before* the `bl`, and the same `frV` is used *after*
  the call.
- Our build has `lfs frV, off(rN)` *after* the `bl` instead.
- Stack frame includes an extra `stfd/lfd f{V}, off(r1)` save/restore
  for the additional NV-FPR.

**Where observed:**
- `Map/BathWaterManager.cpp::drawCap` — 92.14% → 98.30% by hoisting
  `center.y` into a `cy` local immediately before `cosf` inside the
  outer `for (i < 0x1e)` loop. Target also pre-loads `lfs f31, 0x4(r28)`
  pre-`bl cosf`; ours did `lfs f1, 0x4(r28)` post-call. Hoisting added
  one NV-FPR (f28 was used by target but not us) and matched the loop
  layout. Remaining gap is NV-FPR allocation order (target uses
  f28..f31 in reverse).

### Hoist a scalar pre-computation BEFORE a `lwz`/`stw` integer-copy block

**Rule:** When a function does both (a) a scalar arithmetic expression
involving struct-field loads and (b) an integer-copy `result = src;` of
another struct, putting the scalar expression *first* in source forces
MWCC to interleave its loads/computes with the integer copy. This
matches target's layout when target's prologue mixes
`lfs/fsubs/fmuls` with `lwz/stw`.

The mechanism: MWCC tries to schedule the float load early to hide
the load latency. If source puts the scalar after the copy, MWCC may
still schedule the float load early — but it ends up *between* the
`lwz/stw` pair-sequence (which target does), OR *after* them entirely
(which is what we get with the float expression at source-bottom). The
ambiguity is resolved by the source order: explicit early declaration
locks in the early scheduling.

```cpp
// Before — MWCC schedules unk3C load late:
JGeometry::TVec3<f32> result = unk0;            // 3x lwz/stw
f32 t      = (f32)index / (f32)count;
f32 radius = t * (unk3C - height);              // unk3C loaded LATE

// After — fsubs scheduled BEFORE the lwz/stw chain:
f32 radiusFactor = unk3C - height;              // fsubs early
JGeometry::TVec3<f32> result = unk0;            // 3x lwz/stw, interleaved
f32 t       = (f32)index / (f32)count;
f32 radius  = t * radiusFactor;
```

**Where observed:**
- `Map/BathWaterManager.cpp::TBathtubData::getPos` — 88.40% → 92.95%
  by hoisting `f32 radiusFactor = unk3C - height;` to the first
  statement above `JGeometry::TVec3<f32> result = unk0;`. Target's
  prolog interleaves `lfs f0, 0x3c(r4); fsubs f0, f0, f30; lwz r3,
  0x0(r4); ...` while ours had the lwz/stw block come before any
  unk3C load. The radiusFactor local survives until used in `t *
  radiusFactor` later. Caveat: putting the assignment *between*
  result-copy and t/radius does NOT help — must be FIRST.

### Avoid hoisting a member-array base pointer when the loop is unrolled or short

**Rule:** The 2D-array-cast lever (above) is *not* always a win.
When the loop iterates a known small count and MWCC fully unrolls,
or when there is only one access site per iteration, hoisting the
grid pointer into a local can cause MWCC to allocate a register that
target does *not* use. Target uses `this` directly with the full
`+0x20+field` offset folded into the access.

The asymmetry: the 2D cast helps when target's asm shows a single
`+0x4` (field-only) offset, indicating target also has a local. It
hurts when target's asm shows `+0x20`, `+0x24`, `+0x28` etc.
(this-relative including the array base).

**Where observed:**
- `Map/BathWaterManager.cpp::TBathWaterMeshRenderer::calcCoord` —
  first loop on `unk60020` (TVec2): 91.87% → 92.42% by *removing*
  the 2D cast and using `unk60020[x * 0x80 + z].x` directly. Target
  has `addis r5, r5, 0x6` + `stw f1, 0x20(r8)` (this-relative).
  Second loop on `unk20` (TVec3) keeps the 2D cast (regression
  test: removing it drops to 88.87%).

### Use 2D array casts for `arr[i * stride + j]` flat indexing into row-major data

**Rule:** When source has a 1D-declared array member `T arr[N*N]`
but the original code clearly treats it as a 2D `arr[i][j]` grid,
MWCC compiles `arr[i * N + j].field` differently from `arr2d[i][j].field`:

- `arr[i * N + j].field` produces: one combined `mulli (i*N+j)*sizeof(T)`
  followed by `add this + offset` and `lfs/stfs +4(reg)` (only the `.field`
  byte offset folded into lfs).
- `arr2d[i][j].field` produces: two separate `mulli i*0x600` and
  `mulli j*sizeof(T)`, an `add` of both into `this`, and `lfs/stfs
  +0x24(reg)` (the full `&this->arr[0].field` offset folded — base of arr
  in `this` + field offset).

The two-mulli + offset-folding pattern is what the original code emits
when the source was `T arr[N][N]; arr[i][j].field;`. To match without
changing the header type (which may break sibling accesses), cast a
local pointer:

```cpp
JGeometry::TVec3<f32> (*grid)[0x80]
    = (JGeometry::TVec3<f32>(*)[0x80])unk20;
for (int x = 0; x < 0x80; ++x)
    for (int z = 0; z < 0x80; ++z)
        grid[x][z].y = 0.0f;
```

**Caveat:** the local pointer bakes in the `arr`'s base offset (e.g.
`+0x20`), so MWCC folds `.field` only (`+0x4`). For full match parity
target produces `+0x24` because the original source uses arr as a member
(no local). To get the full match, change the header type to 2D. The
local cast alone closes 80-90% of the gap.

**How to apply:**
- Read target asm for the access — if you see two separate `mulli`s and
  `+0xN` offset that includes the array-base offset, the original was 2D.
- Add a local pointer cast `T (*grid)[INNER_DIM] = (T(*)[INNER_DIM])arr;`
- Replace `arr[i * INNER + j]` with `grid[i][j]`

**Where observed:**
- `Map/BathWaterManager.cpp::TBathWaterMeshRenderer::getHeight` —
  82.76% → 86.44% with grid cast alone, then 90.92% with an additional
  `cell = &grid[ix][iz]` local pointer.
- `Map/BathWaterManager.cpp::TBathWaterMeshRenderer::clearHeightMap` —
  84.96% → 97.68% combining 2D nested loops with the cast.
- `Map/BathWaterManager.cpp::TBathWaterMeshRenderer::makeHeightMap` —
  70.66% → 92.41% via grid cast applied to the inner-loop store.
- `Map/BathWaterManager.cpp::TBathWaterMeshRenderer::calcCoord` —
  88.87% → 91.87% via grid casts on both unk20 (TVec3) and unk60020 (TVec2).

### Hoist `param->field.member` to a local pointer when used across multiple `bl` calls

**Rule:** When a function uses the same `param->field.member` address
twice across `bl` calls (e.g. as a first argument to two different
library functions), MWCC re-emits `addi rN, rParam, OFFSET` at each
use site by default. Hoisting the address into a typed local pointer
causes MWCC to pre-compute the offset once into a callee-saved
register (target's `addi r31, r30, 0x74`) and reuse via `addi r3, r31, 0x0`
or `mr r3, r31` at subsequent sites.

This is closely related to the existing "Reference-typed arguments"
rule but applies to plain pointer arguments — no reference type
needed. The trigger is **multiple bl calls** consuming the same
field-address as a leading argument.

**How to apply:**

```cpp
// Before (recomputes addi rN, rParam, OFFSET twice):
C_MTXPerspective(param_2->mProjMtx.mMtx, fovY, aspect, n, f);
...
GXSetProjection(param_2->mProjMtx.mMtx, GX_PERSPECTIVE);

// After (single addi at top, reused via mr):
MtxPtr projMtx = param_2->mProjMtx.mMtx;
C_MTXPerspective(projMtx, fovY, aspect, n, f);
...
GXSetProjection(projMtx, GX_PERSPECTIVE);
```

The local must be a typed pointer (`MtxPtr`, `T*`, `T&`) so MWCC keeps
it in a register rather than spilling each component. If the field
is accessed only once between the two `bl` calls AND nowhere else,
MWCC may already hoist it, so the lever applies mainly when there's
intervening computation that would otherwise cause re-materialization.

**Where observed:**
- `JSystem/JDrama/JDRCamera.cpp::TLookAtCamera::perform` —
  85.12% → 99.73% by hoisting `param_2->mProjMtx.mMtx` to a single
  `MtxPtr projMtx` local. Diff revealed target's `addi r31, r30, 0x74`
  + `mr r3, r31` + later `addi r3, r31, 0x0` pattern. Adds one extra
  callee-saved register slot.
- `JSystem/JDrama/JDRCamera.cpp::TOrthoProj::perform` — 85.58% → 99.74%
  via identical fix. Both functions share the structure
  `C_MTX*(projMtx, ...); ...; GXSetProjection(projMtx, MODE);`.

### Multi-char tag literals: `+ i` not `+ (i << 24)` for per-iteration suffix increment

**Rule:** When iterating over pane/tag names that differ only in
the last character (e.g. `ss_1`, `ss_2`), the source should use
`'ss_1' + i` — NOT `'ss_1' + (i << 24)`. MWCC packs 4-char literals
big-endian (`'X1X2X3X4'` = `(X1<<24)|(X2<<16)|(X3<<8)|X4`), so the
last char sits in the LOW byte and `+ i` correctly increments it.

**Symptom:** Diff shows target asm doing a 2-instruction tag build:

```
addis r4, rIdx, 0xHHHH
addi  r4, r4,   0xLLLL
```

(where `(0xHHHH << 16) + 0xLLLL` = `'X1X2X3X4'`). Our build does
3 instructions because of the spurious shift:

```
slwi  r4, rIdx, 24
addis r4, r4,   0xHHHH
addi  r4, r4,   0xLLLL
```

The `(i << 24)` form would correctly increment the FIRST char (high
byte) — producing tags like 'ss_1', 'ts_1', 'us_1', not 'ss_1',
'ss_2', 'ss_3'. The names with shifted high byte rarely exist in
the BLO file, so this is both a codegen AND correctness bug.

**How to apply:** look for source patterns like `'XXXX' + (i << N)`
for N in {8,16,24}; nearly always the correct form is just `+ i`,
unless the iteration genuinely changes the high byte (rare —
typically only for stage-prefix tags like `'0c_1' + (stage << 24)`
in resetScore-like code where the stage digit IS the high byte).

**Where observed:**

- `GC2D/Guide::load` — fixing six per-loop tag patterns (ss_1/sq_1/
  sc_1/sb_1/cu_a/pn00) lifted load 89.57% → 93.70% (+4.13pp).
- Also applies to single-tag stage-prefixed literals where target's
  asm uses `lis r24, 0xHHHH; addi r4, r24, 0xLLLL` with the literal
  not matching naive interpretation of the source's `'XXXX' + (... shift)`.
  See `'mi09' → '01mi'` fix in Guide::load — original source used
  the actual BLO pane name (`'01mi'`, stage-prefixed) directly.

### Combined `clrlwi+slwi` → `clrlslwi` requires unsigned source type

**Rule:** When indexing an array by a u8-returning function result
(e.g. `array[SMS_getShineStage(...)]`), the result variable's
declared type controls the codegen:

- `u8 stage = SMS_getShineStage(...)` → MWCC fuses zero-extend +
  shift into one `clrlslwi r0, r3, 24, 2` (for *4 indexing).
- `s16 stage = ...` → MWCC emits `clrlwi r0, r3, 24; extsh r0, r0;
  slwi r0, r0, 2` (3 insns: zero-extend, sign-extend, shift).

The s16 cast forces an unnecessary `extsh` between the zero-extend
and the shift, breaking the `clrlslwi` fusion.

**How to apply:** use the natural unsigned type for the function's
return value. If the function returns `u8`, store in `u8`. Don't
widen to `s16`/`s32` unless the value is genuinely used signed later.

**Where observed:**

- `GC2D/Guide::perform` case 9 — changing `s16 stage` to
  `u8 stage` shaved 2 insns and lifted match 67.52% → 68.11%.
- See also `feedback_s32_intermediate_vs_u8.md` for the opposite
  case (when storing TO a u8 field, `s32` intermediate beats `u8`).

### Unsigned div magic differs from signed: drop `(u32)` cast to get signed `/10`

**Rule:** Dividing an `int` by a constant 10 emits MWCC's signed
magic `lis 0x6666; addi 0x6667 → mulhw → srawi 2 → srwi 31 → add`.
If the source casts the dividend to `u32`, MWCC uses the unsigned
magic `lis 0xcccd; subi 0x3333 → mulhwu → srwi 3` instead. These
produce identical instruction COUNTS but different opcodes — break
the match for any TU that originally used signed division.

**How to apply:** prefer `int x = ...; int q = x / 10;` over
`u32 x = ...; u32 q = x / 10;` unless the dividend is genuinely
unsigned (e.g., from a u8 zero-extend that could exceed INT_MAX —
extremely rare for index math).

**Where observed:**

- `GC2D/Guide::load` 13-loop — changing `u32 hi = (u32)i / 10;`
  to `int hi = i / 10;` matched target's `lis 0x6666` signed magic
  and gained +1.91pp (93.70% → 95.61%).

### Char-array row pointer: avoid pre-adding the constant struct offset to the base

**Rule:** When a function iterates over a fixed-stride per-stage
(or per-row) block of bytes within `this`, MWCC matches better
when the source uses `(u8*)this + stage * stride` as the row
pointer and accesses fields via literal byte offsets within
the struct — NOT `(u8*)this + base_offset + stage * stride`
with offset-relative access.

**Symptom:** Diff shows target computing the row pointer
without the +0x14 (or whatever the struct's static offset is),
e.g. `add r19, r31, r18` (this + stage*8), then reading via
`lbz r0, 0x15(r19)`. Our build computes `addi rN, rStage, 0x14;
add rM, r31, rN; lbz r0, 0x1(rM)`. Same effective address but
extra `addi` to fold the static offset into the address compute.

**How to apply:**

```cpp
// Before — pre-add the static offset to the base:
u8* stageData = (u8*)this + 0x14 + stage * 8;
... stageData[1] ...;  // shineCount
... stageData[2] ...;  // redCoin
... *(u16*)(stageData + 4) ...;  // deaths

// After — keep the row pointer canonical, fold offset into the access:
u8* stageData = (u8*)this + stage * 8;
... stageData[0x15] ...;  // shineCount  (= 0x14 + 1)
... stageData[0x16] ...;  // redCoin     (= 0x14 + 2)
... *(u16*)(stageData + 0x18) ...;  // deaths (= 0x14 + 4)
```

The C ABI guarantees these two forms produce identical addresses,
but MWCC's address-arithmetic lowering emits a single `add`
instruction for the row pointer plus a load with the full offset,
rather than two `add`s for "row pointer + static base" and a load
with the small offset.

The lever may also affect register allocation around the row
pointer (canonical row pointer can be reused across nested blocks
with offset-relative loads, vs needing to recompute when adding
varying offsets).

**Where observed:**
- `GC2D/Guide::resetScore` — 89.22% → 90.00% (also folded in
  with other lever changes for total 90.93%).
- `GC2D/Guide::changeBotStatus` — combined with the if/else
  inversion below, 45.67% → 95.73%.
- `GC2D/Guide::resetObjects` — combined with the static-inline
  wrapper trick, 73.32% → 89.89%.

### Top-level if/else order: target's `bne label` always means the LATER block

**Rule:** MWCC NEVER swaps the THEN and ELSE blocks. If target
asm shows `bne .L_X` (or `beq .L_X`) at a top-level branch,
where .L_X is LATER in the function body, then the jumped-to
block is the LATER body in source order. Specifically:

- `if (cond) A else B` → compiles to `test cond; beq label_B;
  A; b end; label_B: B; end:` — so the FIRST block emitted
  is the THEN.
- `bne label` jumping FORWARD to a later block means the THEN
  block IS the fall-through (the FIRST block).

When a function has an early-return pattern like
`if (cond) { ... return; } /* rest */`, MWCC emits THEN
(the early-return body) first, then a `b end` to skip over
"rest". If target instead emits "rest" first and the early-
return body LATER, the source must be rewritten as
`if (!cond) { rest } else { early-return body }`.

**How to apply:**

```cpp
// Before — early-return pattern produces THEN-first emission:
if (stageData[0] != 0) {
    /* completed-stage block: lots of code, return */
    ...
    return;
}
/* active-stage block: lots of code */
...

// After — explicit if/else produces correct order:
if (stageData[0] == 0) {
    /* active-stage block: lots of code */
    ...
} else {
    /* completed-stage block: lots of code */
    ...
}
```

Same logic applies to inner branches:
- Target `bne .L_X` where X is the LATER block → source has the
  LATER block as the ELSE.
- Target `beq .L_X` where X is the LATER block → source has the
  LATER block as the THEN (if cond is the negation).

**Where observed:**
- `GC2D/Guide::changeBotStatus` — top-level `if (stageData[0x14]
  != 0)` early-return was inverted to `if (== 0) { active } else
  { completed }`. Combined with three inner-block inversions
  (`if (idx <= 1 || stageData[0x16] == 0) { hide } else if
  (stageData[0x16] == 1) ...`, and `if (idx != 0) { show } else
  { hide }`), changeBotStatus went 45.67% → 95.73%.
- `GC2D/Guide::resetScore` — `if ((u8)redCoinTotal == 0) hide
  else show` matched target's `bne show_block` pattern (the show
  block is the LATER one in the function body).

### Field stores hoisted before a copy-ctor bl: source has assignment BEFORE the local declaration

**Rule:** When target asm shows `stw rVal, OFF(rThis)` field
stores happening BEFORE a `bl copy__7JUTRectFRC7JUTRect` (or any
copy-ctor bl) but our build emits them AFTER the bl, the original
source has the assignments LITERALLY BEFORE the local-variable
declaration that triggers the copy-ctor. MWCC respects source
order in this case.

**Symptom:** Diff shows our build with `bl copy__7JUTRectFRC7JUTRect`
followed by `lwz rA, OFF1(rThis); stw rA, OFF2(rThis); ...`, while
target has the loads/stores first, then the bl. Effective ordering
is reversed.

**How to apply:**

```cpp
// Before (our build defers stores until after the bl):
JUTRect rect1(_218[idx]);   // bl copy
_424 = _1C0[idx];           // ← MWCC schedules these after the bl
_428 = _378[idx];

// After (matches target):
_424 = _1C0[idx];           // stores hoisted before the bl
_428 = _378[idx];
JUTRect rect1(_218[idx]);   // bl copy
```

This is purely a source-order lever — MWCC's scheduler treats the
copy-ctor `bl` as a barrier and never reorders explicit field
stores around it in either direction. The original author chose
the order; you just have to find it.

**Where observed:**

- `GC2D/Guide::appearGuidePane` — moving the `_424 = _1C0[idx]; _428 = _378[idx];`
  assignments BEFORE the `JUTRect rect1(_218[idx]);` declaration took it
  from 95.20% → 98.60%. Diff showed our build doing the stores after the
  first `bl copy__7JUTRectFRC7JUTRect`; target does them before.

### `J2DPane::show()` / `hide()` vs direct `mVisible = true/false` literal assignment

**Rule:** Even though `J2DPane::show()` and `hide()` are inline
methods whose bodies are `mVisible = true;` and `mVisible = false;`
respectively, using the inline-helper form changes MWCC's scheduling
of the surrounding code. Specifically, around an `if/else` whose
branches each end in a `mVisible = literal` assignment that follows
a `bl` (e.g. `unkBC->search(tag)->mVisible = true;`), calling
`show()`/`hide()` produces different scheduling than the direct
assignment.

**Symptom (before):** Direct `unkBC->search(tag)->mVisible = true;`
emits `bl search; li r0, 0x1; stb r0, 0xc(r3)`. Target shows the
same sequence — but the surrounding stack frame size, branch
offsets, and constant-pool offsets differ. Match drifts ~5pp.

**How to apply:**

```cpp
// before:
if (flag) {
    unkBC->search(tag)->mVisible = true;
} else {
    unkBC->search(tag)->mVisible = false;
}

// after (use the inline helpers):
if (flag) {
    unkBC->search(tag)->show();
} else {
    unkBC->search(tag)->hide();
}
```

Both forms produce the same final `li r0, 0x1; stb r0, 0xc(r3)`
sequence at the call site, but `show()/hide()` is what the original
authors used and MWCC produces matching scheduling of the
surrounding code (loop counter, prologue/epilogue offsets, branch
targets) when the source uses the helpers.

**Where observed:**

- `GC2D/Guide::placeMario` — switching the 8-iteration loop body
  from `pane->mVisible = true/false` to `pane->show()/pane->hide()`
  jumped match 76.89% → 82.02% (~5pp). Loop body asm is byte-identical
  in both forms; the gain comes from scheduling drift in adjacent
  basic blocks settling into target's layout.

### Reference-typed arguments: bind to a local reference BEFORE the call to control argument-eval order

**Rule:** When a function takes a reference-typed argument
(`T&` / `const T&`) and our source passes a struct field of
`this` directly (e.g. `foo(this->field)`), MWCC may emit a
spurious "save `this` to a callee-saved register first" sequence
before computing the reference address. Binding the reference
to a local **before** the call forces MWCC to compute the
reference's address from `r3` (`this`) directly, in the order
the original code expects.

**Symptom:** Diff shows our build doing `addi rN, r3, 0x0`
(save this) then `addi rM, rN, OFFSET` (compute field-ref via
saved this), while target does `addi rM, r3, OFFSET` directly
(compute field-ref from this in one shot, before clobbering r3
with the first argument).

**How to apply:**

```cpp
// before:
foo(param_1, this->matrix_field, this->vec_field);

// after (when foo signature ends with a `T&` reference arg):
JGeometry::TVec3<f32>& v = this->vec_field;
foo(param_1, this->matrix_field, v);
```

For functions with **multiple** trailing reference arguments,
bind each one in the order they appear (reverse-bind also
works — the order of `T& ref = field;` statements doesn't seem
to matter; MWCC computes addresses from `r3` in argument-list
order at the call site).

**Where observed:**

- `JSystem/JPABaseEmitter::setGlobalRTMatrix` (one
  `TVec3<f32>&` arg) — 91.17% → 100%. Binding `v = unk160` to
  a local reference eliminated the `addi r5, r3, 0x0` (save
  `this`) instruction.
- `JSystem/JPABaseEmitter::setGlobalSRTMatrix` (two
  `TVec3<f32>&` args) — 90.15% → 100%. Same fix applied to both
  the `unk154` and `unk160` parameters.

### `TVec3::zero()` writes z,y,x (descending) — `TVec3::set(0,0,0)` writes x,y,z (ascending)

**Rule:** The inline `void zero() { x = y = z = 0.0f; }` in
`JSystem/JGeometry/JGVec3.hpp` uses C++ right-to-left chained
assignment, which MWCC compiles to **descending** stfs order
(0xC, 0x8, 0x4 — i.e., z first, then y, then x). In contrast,
`set(0.0f, 0.0f, 0.0f)` (the 3-arg template `set<TY>`) assigns
x = x_; y = y_; z = z_; in source order, producing **ascending**
stfs (0x4, 0x8, 0xC).

**How to apply:** look at the target diff for the TVec3 store
group. If the asm hits offsets in ascending order (lowest first),
the source used `set(0,0,0)` or explicit `x = 0; y = 0; z = 0;`.
If descending, the source used `zero()`. The choice may differ
between TUs and even between functions within a TU — don't
mass-rewrite, fix only where the diff demands.

**Where observed:**
- `Strategic/TLiveActor::ctor` — three TVec3 stores in ascending
  order; changing `mLinearVelocity.zero(); mAngularVelocity.zero();`
  to `set(0.0f,0.0f,0.0f)` brought 99.88% → 99.93% (rest is
  unrelated stack phantom).
- `Strategic/TLiveActor::moveObject` — descending order in target;
  source uses `zero()` and matches at 100% — confirming zero()
  produces descending.
- `Enemy/beam::TConeBeam::ctor` — descending in target; source
  uses `zero()`, matches at 100%.
- `Enemy/BathtubKiller::resetBathtubKiller` (t289) — target stores
  `mVelocity`/`unk1BC` ascending (x,y,z); source had `.zero()`
  (descending). Switched both to `.set(0.0f,0.0f,0.0f)`; part of
  94.7% → 99.9%.

### Excess field init: drop the source assignment when target asm skips that offset

**Rule:** The inverse of the missing-field-init pattern below.
If target asm stores zero/0.0f at a contiguous block of offsets
but **skips** one specific offset that our source initializes
(via initializer list or body assignment), **delete that
assignment**. The original source left the field uninitialized.

This often surfaces when a class has a declared `int`/`float`
field at a "natural" zero-init position but the original code
didn't explicitly assign it (despite our intuition saying it
should).

**Where observed:**
- `Enemy/bossManta::TBossMantaManager::ctor` — diff showed our
  build had an EXTRA `stw r0, 0x84(r30)` (the
  `mShadowAlphaTimer` field, initialized as `mShadowAlphaTimer(0)`
  in our init list). Target asm wrote 0 at 0x7c, 0x80, 0x88, 0x8c
  (skipping 0x84). Removing `mShadowAlphaTimer(0)` from the init
  list took it 97.66% → 100%.

### Derived class adds own fields at parent's "next" offset — declare and init

**Rule:** Subclasses that inherit from a parent ending at offset
`OFFSET` will store their *own* fields starting at `OFFSET`. If
target asm stores 0 at those offsets in the derived ctor (after
the parent ctor's bl), but the derived class has no fields
declared in our header, add them.

This differs from the "missing-field-init" pattern in that the
fields don't exist in our header yet — they must be **added** to
the class definition, not just initialized.

**Where observed:**
- `MoveBG/Item::TEggYoshi::ctor` — TEggYoshi inherits from
  TMapObjGeneral (size 0x148). Target ctor stored 0 at 0x148,
  0x14C, 0x150 after `bl __ct__14TMapObjGeneralFPCc`. Adding
  3 `u32 unk148/14C/150` fields to TEggYoshi's class + init body
  took ctor 77.62% → 100%. (Note: these are *different* fields
  from TItem's same-offset fields; TEggYoshi inherits directly
  from TMapObjGeneral, not TItem.)

### Empty-body ctor with declared fields — fields MUST be initialized in body or init list to match

**Rule:** An empty ctor body (`Foo::Foo() { }` or `Foo::Foo(...) : Parent(...) { }`)
will NOT initialize declared member fields. Target asm consistently shows
explicit `stw`/`stfs` stores at field offsets after the parent ctor call —
those stores require **explicit assignments in the source**. Skipping
fields (even if the class declares them) is a top cause of 60-90% near-matches
on small ctors.

When the field is at offset 0 (i.e., before the vtable / before any inherited
fields), it must be in the **initializer list** to fire before parent member
inits (e.g., before an embedded TList's ctor). Body assignments are too late
in this case.

When the field is at higher offset, body assignments are fine — MWCC emits
them in source order.

Heuristic for what to add: in the diff, find missing stw/stfs/stb instructions
at offsets that map to declared member fields. The source code IS missing
those assignments. Check the field type to pick the right init value:
- ptr/int slot stored as `0`: `field = nullptr` or `= 0`
- `lfs f0, @const_zero` stored as 0.0f: `field = 0.0f`
- Non-zero literal (like `li r0, 0x3`): use that literal value
- Stored argument register value: use the constructor parameter

**Where observed:**
- `Enemy/hamukuri::TBossDangoHamuKuri::TBossDangoHamuKuri` — `unk238(0)`
  added to init list, ctor 97.22% → 100% and inlined caller
  `createEnemyInstance` 91.76% → 100%.
- `M3DUtil/MActorData::MActorAnmData::MActorAnmData` — `unk0` (offset 0,
  before TList unk1C) added to **init list**, 83.88% → 99.88%. Body
  assignment was wrong because TList ctor must run AFTER unk0 = 0.
- `MSound/MAnmSound::MAnmSound` — inherited field `mData = nullptr`
  (offset 0x90 in JAIAnimeSound), 84.71% → 100%.
- `Animal/fishoid::TRealoid::TRealoid` — also revealed pattern of `mActors`
  init that was wrong; real ctor was `onLiveFlag(0x38)`, 90.86% → 100%.
- `MoveBG/Item::TCoinRed::TCoinRed` — 3 new f32 fields at 0x158-0x160
  (only on TCoinRed, not TCoin / TFlowerCoin), 88.89% → 96.67%.
- `Map/PollutionLayer::TPollutionLayerWallBase::ctor` — unkAC, unkB0,
  83.33% → 100%.
- `MoveBG/MapObjTrap::TLampTrapSpike::ctor` — unk138=3, unk13C/140=0,
  74.55% → 100%.
- `Animal/Butterfly::TButterfloid::ctor` — 3 fields including
  `unk158 = count` (ctor arg), 60.12% → 100%.
- `MoveBG/MapObjMare::TCogwheel::ctor` — same fields all initialized,
  but **field-init order** matters: reordering body to put scalar
  fields BEFORE the TVec3::zero() calls 94.06% → 100%. (TVec3 zero
  produces multiple stfs in source-line position; non-TVec3 stores
  hoisted by MWCC if before/after a function call differs.)

### `GXColor` by-value calls: static const vs stack-local initializer choose different literal shapes

**Rule:** When passing a small (≤ 4-byte) struct by value through a stack
argument slot, the source spelling controls whether MWCC emits a named
static object or an anonymous small-data literal. Match the target data
shape first.

```cpp
static const GXColor cAmbColor = { 0xFF, 0xFF, 0xFF, 0xFF };
GXColor ambColor = cAmbColor;             // local
GXSetChanAmbColor(GX_COLOR0A0, ambColor); // by-value pass
```

lowers to:

```
lwz r0, cAmbColor@sda21(r0)   ; load const
stw r0, 0x54(r1)               ; store to local
lwz r0, 0x54(r1)               ; reload
stw r0, 0x50(r1)               ; store to arg slot
bl GXSetChanAmbColor
```

The redundant stash+reload at 0x54 is the by-value copy of the local
into the arg slot. MWCC does NOT eliminate the dead store to 0x54
(stores aren't reordered/elided), so this remains visible.

When the target instead has only an anonymous `0xffffffff` literal in
`.sdata2` and no named `cAmbColor` object, use a stack-local initializer:

```cpp
GXColor ambColor = { 0xFF, 0xFF, 0xFF, 0xFF };
GXSetChanAmbColor(GX_COLOR0A0, ambColor);
```

**The anti-pattern:** writing the constant as a u32 and casting through:

```cpp
static const u32 cAmbColor = 0xFFFFFFFFu;
GXColor ambColor;
*(u32*)&ambColor = cAmbColor;   // folded by MWCC: const u32 → li r0, -1
GXSetChanAmbColor(GX_COLOR0A0, ambColor);
```

MWCC inlines the const value (`li r0, -1`) instead of emitting the sda
load, breaking the match against rodata.

**Where observed:**
- `src/GC2D/SelectMenu.cpp::TSelectGrad::perform` ambient color setup —
  target wants the stack-local initializer and anonymous `@2809 =
  0xffffffff` literal; a `static const GXColor cAmbColor` emitted an
  extra named object and left `@2809` missing.

### `s8 result` (not `s32 result`) for int functions that compute via `(s8)idx`

**Rule:** A function whose semantic return type is `int` but whose
"hit" value comes from casting a small unsigned index produces target's
`extsb r_result, r_idx` directly when the result local is declared
`s8`, not `s32`:

```cpp
int func() {
    u32 idx = mField;
    s8 result = -1;          // not s32!
    if (idx == 0) return -1;
    ...
    for (...) {
        if (cond) {
            result = idx;    // implicit (s8) cast on store
            break;
        }
    }
    return result;           // implicit sign-extend on return
}
```

With `s8 result`, MWCC emits `extsb r_result, r_idx` (one
instruction). With `s32 result; result = (s8)idx;`, MWCC emits two
instructions: `extsb r_temp, r_idx; mr r_result, r_temp`. The s8 form
also keeps function byte-size correct.

**Companion lever — return type must also be `s8`.** With `int func()`
returning `int`, MWCC emits `extsb r3, r5` at the return path to
sign-extend the s8-result variable into int. Target uses `mr r3, r5`,
which means **the function actually returns `s8`** — the EABI/PPC
convention delegates s8 sign-extension to the *caller*, so an s8
return needs no callee extsb. The combination is `s8 func() { s8
result; ... result = idx; ... return result; }` — gives `extsb r5,
r4` in-loop AND `mr r3, r5` at tail. (Earlier ticks misread the
single-extsb-at-return as an unavoidable trade-off; that was wrong.)

**Where observed:**
- `src/GC2D/SelectMenu.cpp::TSelectMenu::getNextIndex` reached 100%
  with `s8 return type + s8 result + u32 idx + s32 i` (separate loop
  counter for signed `i < 8` while idx keeps unsigned compare for
  early-return).
- `src/GC2D/SelectMenu.cpp::TSelectMenu::getPrevIndex` reached 100%
  with `s8 return + s8 result + u32 idx`, plus a single-variable loop
  `for (s32 i = idx - 1; i >= 0; i--)` that lets MWCC set CR0 via
  `subic.` on the decrement (vs `addic.` on the loop-count). See
  separate Settled rule on `subic.; addi; blt` vs `subi; addic.; ble`
  below.

### Single-var loop `for (s32 i = base - 1; i >= 0; i--)` flips MWCC's flag-on-which-op choice

**Rule:** When a CTR-loop counts down from `(base - 1)` to `0`
inclusive, **two semantically-equivalent ways** to source it produce
different prologue patterns:

```cpp
// A — separate count + decremented base
base--;
for (s32 i = base + 1; i > 0; i--) {
    ... mStageStates[base] ...
    base--;
}
// → subi r4, r4, 1
//   addic. r0, r4, 1        ← flag on the count (i+1)
//   mtctr r0
//   ble .end                ← test "i+1 <= 0"

// B — single variable, "loop while i >= 0"
for (s32 i = base - 1; i >= 0; i--) {
    ... mStageStates[i] ...
}
// → subic. r4, r4, 1        ← flag on the decrement (i)
//   addi r0, r4, 1
//   mtctr r0
//   blt .end                ← test "i < 0"
```

Both produce a valid CTR loop with the same iteration count. The
difference is which operation sets CR0 and which signed branch tests
the result. To match a target asm that shows `subic.; addi; blt`, use
the single-variable form (B). The `i >= 0` loop check and the use of
`i` directly for array indexing are the cues MWCC reads.

**Where observed:**
- `src/GC2D/SelectMenu.cpp::TSelectMenu::getPrevIndex` — Form A at
  ~88-90% with `subi; addic.; ble`; switching to Form B gave the
  target's `subic.; addi; blt` and the final 10pp to 100%.

### `u32 idx` for unsigned array-index loops produces `cmplwi` (not `cmpwi`)

**Rule:** Loop counters/indices that come from a u8 field (`lbz`-loaded,
always non-negative) should be declared `u32` in source if the target
asm uses `cmplwi` (unsigned compare) for the loop-entry / loop-exit
checks. Declaring `s32 idx` produces signed `cmpwi`.

**Where observed:**
- `src/GC2D/SelectMenu.cpp::TSelectMenu::getNextIndex`,
  `getPrevIndex` — `lbz r4, 0x13b(r3)` followed by target's
  `cmplwi r4, 0x8` (or `r4, 0x0`) settled by changing
  `s32 idx = mScenarioIndex;` to `u32 idx = mScenarioIndex;`.

### `switch (mode) { case 0: ...; case 3: ...; default: }` for sparse two-case dispatch

**Rule:** For an if/else-if cascade over two integer constants (e.g.
`if (mode == 0) ... else if (mode == 3) ...`), MWCC emits a flat
sequence of `cmpwi; bne; cmpwi; bne` — but the target asm sometimes
shows a **switch-table-ish branch tree**:

```
cmpwi r0, 0x3
beq case_3
bge default       ; mode > 3 → skip
cmpwi r0, 0x0
beq case_0
b default
```

That tree (test the largest case first, then `bge default` to bail on
out-of-range high, then test smaller cases) is what MWCC emits for a
`switch` statement with two non-contiguous cases — even when only two
cases exist. Rewriting `if (mode == 0) ... else if (mode == 3) ...`
as a `switch (mode) { case 0: ...; case 3: ...; }` yields the tree.

**Where observed:**
- `src/GC2D/SelectMenu.cpp::TSelectGrad::perform` — color-byte
  inc/dec dispatch (74.13% → 87.35% just from this switch rewrite,
  applied twice).

### `JUtility::TColor color;` triggers default-ctor `set(0xffffffff)` stw — use ctor form instead

**Rule:** The default constructor for `JUtility::TColor` runs
`set(0xffffffff)` which lowers to `li r0, -1; stw r0, 0(slot)` —
visible in the prologue of the local's stack slot. If a function
then immediately overwrites the color with `color.set(r,g,b,a)`
(byte-by-byte), the int store from the default ctor remains in the
asm because **MWCC never eliminates stores to memory** (CLAUDE.md
*MWCC can eliminate redundant reads, but not writes*).

If the target asm has no early `stw -1` for the color slot, the
source did **not** default-construct the local. Two patterns avoid
the default ctor:

1. Direct-init with the 4-arg ctor: `JUtility::TColor color(r,g,b,a);`
2. Pass a ternary of TColor temporaries directly to the call site:
   `gpApplication.mFader->setColor(cond ? TColor(a,b,c,d) : TColor(e,f,g,h));`
   — this avoids both the default ctor AND the extra "anonymous prvalue
   temp → named local" copy step that `T color = cond ? T(...) : T(...);`
   produces.

**Where observed:**
- `src/GC2D/SelectDir.cpp::direct` 91.0% → 95.5% by switching both
  TColor uses from `JUtility::TColor color; color.set(...)` to (a) a
  ternary passed directly to setColor and (b) a `TColor(...)` ctor.
  Each removed the corresponding default-ctor `li -1; stw` prologue.

**Caveat:** The auto-generated TColor copy (`stw` of the loaded u32)
is still emitted at the call-site argument-slot copy (4-byte by-value
pass), which is the desired pattern. The fix removes only the
default-ctor's extra store, not legitimate copies.

### Two-step pointer init `p = base; p += off;` vs combined `p = base + off;`

**Rule:** When you want MWCC to emit an **in-place** `add rDst, rA, rB`
(where rDst is a callee-saved register that already held one of the
operands), declare the local with a base assignment and then add into
it as a separate statement:

```cpp
u8* readPos = dest;
readPos += size;
```

The combined form `u8* readPos = dest + size;` routes the result
through a volatile scratch register first, producing
`add r3, r30, r31; addi r31, r3, 0` (or `mr r31, r3`) — one extra
instruction and a different register layout. The two-step form lets
the allocator reuse the same register the operand `size` already
lived in (e.g. r31 in the example), giving `add r31, r30, r31`.

Required conditions for the lever:
- One operand is already in a callee-saved register (e.g. `size` was
  the third arg in r5 and got mr'd into r31 to survive a `bl memcpy`).
- The destination local will be **used multiple times across calls**
  (otherwise it stays in a volatile and the rule is moot).
- No reads of the original base after the add (so reusing the operand's
  register is safe).

**Where observed:**
- `src/JSystem/JKernel/JKRDvdAramRipper.cpp::nextSrcData` —
  `u8* readPos = dest + size;` capped the function at 98.02% with an
  extra `addi r31, r3, 0`. Switching to
  `u8* readPos = dest; readPos += size;` → 100%. The same two-step
  pattern also separates `origDest` (returned) from `readPos` (used
  for DVDReadPrio + srcLimit), letting the allocator give them
  distinct r30/r31 lives.

**Related but distinct:** This is NOT the same as the `addi rN, rM,
0` vs `mr rN, rM` open question — that one is about how a *move*
gets encoded. This is about whether an *arithmetic op* uses an
in-place destination or a temp + move.

### Static TVec3 init: direct-init `T x(args)` vs copy-init `T x = T(args)`

**Rule:** A file-scope `static JGeometry::TVec3<f32> name(a, b, c);`
(direct-initialization) lowers to three direct `stfs` into the
global's bss slot. The copy-initialization form
`static JGeometry::TVec3<f32> name = JGeometry::TVec3<f32>(a, b, c);`
materialises a temporary on the stack and then copies it word-by-word
via the auto-generated copy ctor — producing 3× `stfs` to stack +
3× `lwz`/`stw` from stack to global + the stack-frame inflation that
the temp implies. The two are NOT equivalent in MWCC despite being
semantically identical in C++98.

**Where observed:**
- `src/Player/MarioInit.cpp::__sinit` — `cDeformedTerrainCenter`
  rewritten 94.95% → 99.95% by changing copy-init to direct-init.
  Stack frame shrunk from 0x20 → 0x10 (the temp was 12 bytes plus
  padding).
- `src/Player/Atom.cpp::__sinit` already used direct-init form,
  matching. The MarioInit form was the outlier.

**How to spot:** `__sinit` shows 3 `stfs` into a stack offset followed
by 3 `lwz` / `stw` from that stack offset into the bss slot. Target's
asm has just the 3 direct `stfs` into the bss slot.

### Header-include controls __sinit JALList chain offsets

**Rule:** Each `JALListHioNode<T,...>`-derived class T defined in a
header instantiates a static `JALList<T>` singleton at TU scope when
that header is `#include`d. Missing the include causes the
TU-local bss to start its JALList chain at a different offset,
shifting EVERY downstream `addi r5, r31, OFFSET` and breaking
`__sinit` match.

`<MSound/MSoundBGM.hpp>` is the most common missing include for
TUs that need `JALList<MSBgm>` at bss offset 0. Without it, MWCC
puts `JALList<MSSetSoundGrp>` at offset 0 instead, shifting the
chain by exactly 0xC.

**Where observed:** 15+ TUs lifted 91.5% → 100% by adding
`#include <MSound/MSoundBGM.hpp>`. Recent ticks:
- `Camera/CameraJetCoaster` (tick 34)
- `Camera/sunmgr` (tick 36)
- `Player/Mario{Action,Autodemo,Collision,Draw,Jump,Run,Sound,Special,Wait,WaterGun,Main}.cpp`
- `MSound/{MAnmSound,MSoundStruct}.cpp`
- `NPC/NpcEvent.cpp`
- `Animal/AnimalBase.cpp`

**Caveat:** Some TUs have additional missing items beyond JALList<MSBgm>
(MarioMain needed `cDeformedTerrainCenter` static; MarioParticle
needs TBubbleCallBack/TWarpInCallBack vtables — these are TU-specific
classes not yet reconstructed).

### TVec2/TVec3 copy-ctor needs explicit POD-cast operator= for `lwz/stw` pattern

**Rule:** MWCC's auto-generated copy ctor for a class containing a
`JGeometry::TVec2<f32>` or `TVec3<f32>` member walks the floats
field-by-field, emitting interleaved `lfs`/`stfs` pairs — even though
the target asm uses batched `lwz`/`stw` integer-move pairs. To force
the integer pattern for these vector members, the vector type itself
must override copy ctor + `operator=` with a *POD struct cast*:

```cpp
struct _POD { T x, y; };       // or T x, y, z for TVec3
*(_POD*)this = *(const _POD*)&other;
```

MWCC lowers POD struct assignment to memcpy-like integer loads/stores;
field-by-field assignment respects the float types. The existing trick
in `TVec3<f32>` uses `*(Vec*)this = *(Vec*)&other` (where `Vec` is the
dolphin SDK POD); for `TVec2` we synthesise an inline POD struct.

**Where observed:**
- `TVec3<f32>::operator=` in `include/JSystem/JGeometry/JGVec3.hpp`
  has carried this trick since project start; comments explicitly note
  "yes, this has to use lwz/stw and not lfs/stf".
- `TVec2<T>::operator=` added in this tick → `TCameraMapTool` copy
  ctor 91.7% → 100% (the only `TVec2<f32>` member of TCameraMapTool's
  layout had been emitting float pattern; everything else integer).

**Caveat:** The POD cast emits 2× `lwz` then 2× `stw` (or 3 of each for
TVec3) in a *batched* shape. For the trick to take effect, the vector
member must be reached through an auto-generated whole-class copy
ctor; explicit field-by-field assignment in user code still uses the
float pattern. If you want batched int moves at a *specific call
site*, write `node.mPos = src.mPos;` (assignment) rather than
`node.mPos.set(src.mPos);` (the `set` overload uses float pattern).

### Typed class field beats `*(T*)((u8*)this + OFFSET)` cast for store/reload sites

**Rule:** Writing to a `void*`-typed class field via the cast form
`*(void**)((u8*)this + 0x2A4) = nullptr;` emits a two-instruction
sequence (`addi rN, rThis, 0x2A4; stw r0, 0(rN)`) — MWCC materialises
the address into a scratch register before the store. The same field
declared as `void* unkXXX` on the class produces the natural
single-instruction `stw r0, 0x2a4(rThis)`.

Loads tend to be unaffected in isolation (MWCC happily emits
`lwz r3, 0x2a4(r4)` in either form), so the gain shows up mostly on
store sites and on addresses passed to subsequent calls. If the same
cast field is stored and then reloaded before a call, the cast form can
also make MWCC cache `this + OFFSET` in a scratch register and use
`stw/lwz 0(rN)`. A typed member access can keep both operations as
direct `OFFSET(rThis)` accesses.

**Where observed:**
- `CPolarSubCamera::execNoticeOnOffProc_` in `CameraNotice.cpp`
  95.67% → 100% after splitting the `unk21C` char-array into
  `unk21C[0x88]` + `void* unk2A4` + `unk2A8[0x20]` and rewriting all
  12 `*(void**)((u8*)this + 0x2A4)` sites to `this->unk2A4`. Also
  lifted `getNoticeActor_` 77.8% → 78.5% in the same TU.
- `CPolarSubCamera::ctrlOptionCamera_` in `CameraOption.cpp`
  99.4% → 99.9% after rewriting the `this + 0x70` map-tool compare,
  store, and reload as the typed `unk70` field. The cast form emitted
  an extra `addi r7, r31, 0x70` and used `stw/lwz 0(r7)` around
  `calcPosAndAt`; the typed field matched target `stw/lwz 0x70(r31)`.
- `CPolarSubCamera::warpPosAndAt(const Vec&, const Vec&)` in
  `CameraWarp.cpp` 95.4% → 100% after declaring `0x6c` as the typed
  `TCameraInbetween* unk6C` field. The raw cast form cached
  `this + 0x6c` in a saved register and reloaded through `lwz 0(rN)`
  after `TCameraInbetween::warpPosAndAt`; the typed field matched the
  target's direct `lwz 0x6c(r31)` reload. The float overload also
  improved 82.5% → 83.8%.

**Caveat:** This does not solve every addi-field-address caching case.
When the target itself caches a field address, the typed member can be
wrong; when the target uses direct-offset accesses and our source uses
a raw `this + OFFSET` cast, try declaring and using the real member
before reaching for more invasive rewrites.

### Repeated `a / b` divisions: rewrite as `a * (1.0f / b)` to enable reciprocal CSE

**Rule:** When the same integer→float divisor `b` is used in multiple
floating-point divisions, MWCC's `-O4,p` optimiser does **not** CSE the
`(1/b)` automatically — each `a / b` emits the full int→float magic
sequence (`lis 0x4330`, `stw`/`stw`, `lfd`, `fsubs` from magic constant)
plus a `fdivs`. Rewriting as `a * (1.0f / b)` with the literal `1.0f`
DOES trigger CSE: the entire reciprocal expression is hoisted once and
the result is reused via `fmuls` at each call site.

**Pattern (target asm, ONE reciprocal computed, reused):**
```
clrlwi r4, r30, 16          ; u16 cast of duration
stfs   ...                   ; (some unrelated store)
lis    r0, 0x4330
stw    r4, OFFSET(r1)
lfd    f1, magic_dbl@sda21
stw    r0, OFFSET-4(r1)
lfs    f2, 1.0f@sda21
lfd    f0, OFFSET-4(r1)
fsubs  f0, f0, f1            ; (double)dur
fdivs  f2, f2, f0            ; f2 = 1.0 / dur     <-- computed once
fmuls  f0, f3, f2            ; phase_X * (1/dur)
... (Y axis later) ...
fmuls  f0, f1, f2            ; phase_Y * (1/dur)  <-- reuses f2!
... (Z axis later) ...
fmuls  f0, f1, f2            ; phase_Z * (1/dur)  <-- reuses f2 again
```

**Source that does NOT CSE (3 separate int→float→fdivs sequences):**
```cpp
mAngleX.mDecrement = phase_X / (f32)(u16)duration;
mAngleY.mDecrement = phase_Y / (f32)(u16)duration;
mAngleZ.mDecrement = phase_Z / (f32)(u16)duration;
```

**Source that DOES CSE (1 reciprocal, 3 multiplies):**
```cpp
mAngleX.mDecrement = phase_X * (1.0f / (f32)(u16)duration);
mAngleY.mDecrement = phase_Y * (1.0f / (f32)(u16)duration);
mAngleZ.mDecrement = phase_Z * (1.0f / (f32)(u16)duration);
```

Important caveats observed:
- Pre-computing `f32 inv_dur = 1.0f / (f32)(u16)duration;` as a function-
  scope local **does not** match: MWCC computes inv_dur EARLY (at the
  declaration site) and uses a callee-saved register to carry it
  across the rest of the function, growing the stack by 4 bytes. The
  target instead computes the reciprocal LAZILY at the first use site
  and keeps it in a volatile fp register. Writing the literal `1.0f / b`
  expression INSIDE each axis's compound expression triggers CSE
  WITHOUT promoting it to a saved local.
- Pre-computing `f32 dur_f = (f32)(u16)duration;` then using `phase / dur_f`
  also fails — MWCC sees three separate divisions and emits three fdivs.
  The literal `1.0f` and the divide it controls must appear in the
  source AT each axis site.

**Where observed:**
- `src/Camera/camerashake.cpp::startShake` — 75.3% → 99.68% (+24.4pp).
  Three axis blocks each computing `phase / (f32)(u16)duration` rewritten
  to `phase * (1.0f / (f32)(u16)duration)`. Stack frame shrank from
  0x38 → 0x50 to match target's 0x50.
- `src/Camera/camerashake.cpp::keepShake` — 75.9% → 87.3% (+11.4pp).
  Same rewrite, three axis blocks. Remaining 13% from unrelated
  pointer-walk pattern in the dispatch loop.

### Multiple bools live simultaneously: declare and init up-front to get separate registers

**Rule:** When target's asm shows multiple boolean variables initialized
to 0 BEFORE any test runs (e.g. `addi r4, r3, 0; addi r0, r3, 0` —
two distinct registers being zeroed at the top of an inline) and each
later conditionally set to 1 via `li rN, 1`, the original source
declared each bool up front with `bool x = false;` and used a
SEPARATE `if (cond) x = true;` for each.

The naive chained form `bool b = a && extraCheck;` puts each bool's
register-life into a tight window (alive only briefly between
assignment and use) so MWCC reuses ONE register across all of them —
producing extra `li r0, 0` re-inits and redundant `clrlwi.` normalize
steps.

**Symptom in our build:** Function inlines `inViewCone`-like helpers
with 4+ normalize positions (vs target's 3) and 1-2 extra
`li rN, 0; clrlwi. r0, ...` reinitializations between each `&&`
check. Match drops 15-25pp depending on how many times the inline is
expanded.

**Pattern (target asm) for a 3-bool chain like inViewCone:**
```
addi r4, r3, 0          ; b = 0 (initialized BEFORE any check)
addi r0, r3, 0          ; a = 0
... a-check ...
li r0, 1                 ; a = 1 if check passed
clrlwi. r0, r0, 24       ; test a
beq L_skip_b             ; if !a, b stays 0
... b-check ...
li r4, 1                 ; b = 1
L_skip_b:
clrlwi. r0, r4, 24       ; test b
beq L_skip_c             ; if !b, c stays 0
... c-check ...
li r3, 1                 ; c = 1
```

Note: 3 distinct registers (r0, r4, r3) for a/b/c, each only zeroed
ONCE at the top. No re-initialization between checks.

**Source that matches:**
```cpp
bool a = false;
bool b = false;
bool c = false;
if (firstCheck()) a = true;
if (a && secondCheck()) b = true;
if (b && thirdCheck()) c = true;
return c ? true : false;
```

**Source that does NOT match (extra re-inits + 1 register reused):**
```cpp
bool a = firstCheck();
bool b = a && secondCheck();
bool c = b && thirdCheck();
return c ? true : false;
```

The chained form makes each bool short-lived → MWCC coalesces them
into one register, requiring `li r0, 0` before each use to "reset" the
register's contents.

The final `return c ? true : false` keeps the trailing
intermediate-byte normalize (see Settled rule
[[Ternary `? true : false` produces intermediate-byte sequence]]).

**Where observed:**
- `src/Camera/CameraNotice.cpp::inViewCone` — namespace-anonymous
  inline used twice in `getNoticeActor_`. With the chained `bool a =
  X && Y;` form, both inline expansions had 4 normalize steps and 1
  reused register, capping the function at ~72.4%. Switching to the
  up-front-init + `if` form gave 3 normalize steps and 3 distinct
  registers per inline — `getNoticeActor_` 72.43% → 75.88% (+3.45pp)
  in one commit, with the second inline expansion confirming the
  same delta. Together with the `? true : false` removal on each
  bool, +22.14pp across the function.

### Cache global-pointer-via-stores: introduce a local to silence reload

**Rule:** When the same global pointer is dereferenced multiple times in a
row, with intervening stores to memory through *another* pointer, MWCC
pessimistically reloads the global (`lwz r3, gFoo@sda21`) before each
deref because the store could alias the global. Caching the pointer
into a local `T* foo = gFoo;` before the first use lets MWCC keep
the pointer in a single register across all the reads.

**Pattern in source:**

```cpp
// Pessimistic — emits 3× `lwz r3, gpCameraMario@sda21`:
out->x = gpCameraMario->mPosX;
out->y = gpCameraMario->mPosY;
out->z = gpCameraMario->mPosZ;

// Cached — single `lwz r3, gpCameraMario@sda21`:
TCameraMarioData* mario = gpCameraMario;
out->x = mario->mPosX;
out->y = mario->mPosY;
out->z = mario->mPosZ;
```

The relevant signal in the target is whether `lwz rN, gFoo@sda21`
appears once or repeatedly across consecutive stores to a different
pointer. Repeated reloads => no local; single load => cached local.

**Where observed:**
- `src/Camera/CameraTalk.cpp::ctrlTalkCamera_` (100% match) — uses
  the cached `TCameraMarioData* mario = gpCameraMario;` pattern.
- `src/Camera/CameraNotice.cpp::getNozzleTopPos_` — 96.90% → 99.84%
  applying the same cache for the non-watergun branch.
- `src/Camera/CameraNotice.cpp::ctrlLButtonCamera_` — same lever
  applied twice (3 separate `gpCameraMario->mPos*` blocks), part of
  61.18% → 73.25% gain.

### `if (!cond)` to flip block-ordering when target's branch is on TRUE

**Rule:** MWCC always emits the source's THEN-block before the
ELSE-block (per CLAUDE.md). So when target's asm shows
`bne THEN_LABEL` (jump *on true*) with the THEN block placed AFTER
the unconditional fall-through ELSE block, the original source
wrote the condition INVERTED. I.e., `if (!cond) ELSE_body else
THEN_body`, with the labels named from the diff's perspective.

**Pattern (target asm):**
```
test
bne LABEL_X     ; jump on TRUE
... block A ... ; fall-through (this is the ELSE branch of original src!)
b END
LABEL_X: ... block B ...   ; reached only if test was TRUE
END:
```

**Source that matches:**
```cpp
// NOT: if (cond) { block_B } else { block_A }
// (would emit `beq LABEL_A; block_B; b END; LABEL_A: block_A; END:`)
//
// YES:
if (!cond) {
    block_A
} else {
    block_B
}
// emits `bne LABEL_B; block_A; b END; LABEL_B: block_B; END:` — match.
```

In other words: invert the condition AND swap the branches if you
want the textual order of THEN/ELSE in asm to swap.

**Where observed:**
- `src/Camera/CameraNotice.cpp::ctrlLButtonCamera_` — outer test
  `SMS_CheckMarioFlag(0x8000)`. Target emits `bne 0x84` (jump on
  true to the watergun-path). Natural `if (flag) { watergun } else
  { copy }` emits `beq` to the else; rewriting as
  `if (!flag) { copy } else if (gun == null) { copy } else { watergun }`
  swaps the layout and matches target. +8pp this fn alone.
- `mario/Camera/CameraBGCheck::isValidCamClip` (tick 22) — also
  uses this lever indirectly via the nested-ternary form of
  `isLegal()`. The `?:` chain creates similar branch-order inversions.

This is a refinement of the broader CLAUDE.md "compiler NEVER swaps
the true/false block" rule — combined with the observation that
target's branch direction (`bne` vs `beq`) tells you whether the
THEN block lives at the fall-through or at the labeled position.

### `isLegal()` style: nested `? true : false` is the double-normalize lever

**Rule:** When the target asm shows the *double* bool-normalize pattern
(two consecutive `li r0, 1; b; li r0, 0` blocks separated by
`clrlwi r0, r0, 24; cmplwi r0, 1; bne ...`), the original source
constructed the bool by *nesting* two ternary `? true : false`
expressions, not by writing `bool x = ...; if (!x)`.

The reference pattern is `TBGCheckData::isLegal()` in
`include/Map/MapData.hpp`:
```cpp
// checkFlag returns first ? true : false  (normalize #1)
bool checkFlag(u32 flag) const { return mFlags & flag ? true : false; }
// isLegal compares the bool with 1, then ? false : true  (normalize #2)
bool isLegal() const {
    return checkFlag(BG_CHECK_FLAG_ILLEGAL) == 1 ? false : true;
}
```
When `isLegal()` is inlined at a call site, the call site emits the
characteristic
```
lhz r0, 4(rX); rlwinm. r0, r0, 0, 27, 27   # mFlags & ILLEGAL
beq L1; li r0, 1; b L2                      # normalize #1: bool conv
L1: mr r0, r3 (or li r0, 0)
L2: clrlwi r0, r0, 24; cmplwi r0, 1; bne L3 # negate the bool
li r0, 0; b L4
L3: li r0, 1                                 # normalize #2: !bool
L4: clrlwi. r0, r0, 24; beq end             # finally, test
```

In contrast, the natural-looking source `bool b = expr; if (!b)`
collapses to a single `cmpwi r0, 0; bne` — *one* normalize, no
roundtrip. So when matching, swap to the nested-ternary form (or
call the existing `isLegal()` / similar helper).

A single-normalize sibling pattern — one `li 1; b; li 0` block,
no `cmplwi r0, 1` afterwards — comes from one explicit
`int_expr & MASK ? true : false`. Used at the same call site for
e.g. `data->mBGType & BG_PROPERTY_FLAG_CAMERA_WONT_CLIP ? true : false`.

**Citations:**
- `CameraBGCheck.cpp::isValidCamClip` (tick 22) — `data->isLegal()`
  + `data->mBGType & WONT_CLIP ? true : false` together produced the
  full target asm for the validity check inlined into
  execGroundCheck_ / execRoofCheck_ / execWallCheck_, +30pp combined.
- `MapData.hpp::isLegal` is itself the canonical example (its
  callers across the codebase emit the same shape).

### Inner parens around the divisor expose `fdivs+fmadds` fusion

**Rule:** With `-fp_contract on`, MWCC fuses `a + b * c` into `fmadds`.
When the multiplicand involves a division, the *position of the division*
in the expression decides whether the multiply (and thus the fusion) is
the top-level rightmost operation.

```cpp
// Outer division — division is the LAST op at top level.
// Compiles as: fmuls tmp, b, (c-d); fdivs tmp, tmp, e; fadds r, a, tmp
//   → 3 instructions, no fusion.
f32 r = a + b * (c - d) / e;

// Inner-parens division — division is buried inside the multiplicand,
// so the multiply is the top-level rightmost op.
// Compiles as: fdivs tmp, (c-d), e; fmadds r, b, tmp, a
//   → 2 instructions, fused.
f32 r = a + b * ((c - d) / e);
```

The same logic explains a sister shape:

```cpp
// Left-to-right `b/c*d` parses as `(b/c)*d`; the multiply is rightmost.
// Compiles as: fdivs tmp, b, c; fmadds r, tmp, d, a (= fdivs + fmadds).
f32 r = a + b / c * d;
```

When the target asm shows `fdivs` immediately followed by `fmadds` (no
intervening `fmuls`), the source has the division *inside* the multiplicand
(via inner parens, or via left-associative `/` placement that puts the
multiply last). When the target shows `fmuls; fdivs; fadds` instead, the
source has the division as the top-level last op (`a + b*(c-d)/e` shape).

This is the inverse of [[Splitting `a + b * c` into two statements
defeats -fp_contract on fusion]] — that one shows how to BREAK fusion;
this one shows how to PROMOTE it from a 3-instruction unfused chain to
the 2-instruction fused pair.

**Where observed:**
- `src/MSound/MSHandle.cpp::calcDolby` HiSence_Dist interpolation
  (tick 19): `0.5f + param * ((d - 0.5f) / cPan_HiSence_Dist)` —
  inner-parens form — compiled to `fdivs; fmadds` matching target.
  Without the inner parens (the natural `a + b*(c-d)/e` form), MWCC
  emits `fmuls; fdivs; fadds`. +6pp on calcDolby.
- `src/MSound/MSHandle.cpp::calcDolby` cDol_HalfRad branch (tick 19,
  same TU, different expression shape):
  `0.5f + 0.5f / (cDol_FullRad - cDol_HalfRad) * (angle - cDol_HalfRad)`
  — left-associative `/` so it parses as `0.5f + ((0.5f / (FR-HR)) * (angle-HR))`,
  putting the multiply last. Compiles to `fdivs + fmadds` matching target.

### `static inline` wrapper trick defeats auto-inlining of one call site

**Rule:** When target keeps a header-defined inline function as a `bl` weak
call but MWCC's `-inline auto` (or `-inline deferred`) is recursively
inlining it at the call site, wrap the call in a `static inline` helper
inside the same TU:

```cpp
// In NpcParts.cpp (the TU that's losing match):
static inline void setEffectMtxOnTex0(J3DMaterial* mat, MtxPtr mtx)
{
    mat->getTexGenBlock()->getTexMtx(0)->setEffectMtx(mtx);
}

// Then at the call site:
setEffectMtxOnTex0(mdata->getMaterialNodePointer(j), effectMtx);
```

MWCC inlines the wrapper at the call site, but does NOT recursively
inline through it — so the contained `bl setEffectMtx` survives as a
direct call to the weak symbol. The weak symbol itself is still emitted
in the TU (whichever way the inline body is reached), so the linker
deduplicates across TUs.

This is **selective** in a way `#pragma dont_inline on` is not: only the
specific call site routed through the wrapper is affected. Other inlines
in the same TU (accessor inlines, `getModel()`/`getModelData()` chains,
etc.) continue to inline as before.

**Where observed:**
- `src/Map/MapModel.cpp` ships with a `static inline void fake(J3DMaterial*,
  MtxPtr)` helper (line 16) wrapping `setEffectMtx`. This is the original
  source pattern — confirmed by 99.97% match on the whole TU.
- `src/NPC/NpcParts.cpp::partsPerform` was at 56.73% with MWCC inlining
  the matrix copy + adding f30/f31 callee-saved spills. Adding the same
  wrapper pattern (`setEffectMtxOnTex0`) lifted match to 92.56% (+35.83pp)
  with the bl call restored. The previous IMPLEMENTATION tick's attempt
  with `#pragma dont_inline on` matched setEffectMtx but regressed the
  ctor 62.5% → 43.3% and addJellyFishParts 89.5% → 76.5% by breaking
  unrelated accessor inlines.
- `src/Map/MapMirror.cpp::TMirrorModelManager::perform` (2026-06-11 MNL):
  routing the mirror material update through `setEffectMtxOnTex0` restored the
  target call boundary for `J3DTexMtxInfo::setEffectMtx` and moved the function
  36.0% → 47.4% (`MapMirror` TU 84.0651% → 86.089355%).
- `mario/JSystem/JDrama/JDRNameRefGen` probe (t376): wrapping
  `return new TPolarCamera();` in a TU-local `static inline
  newPolarCamera()` made MWCC emit and call the exact weak
  `JDrama::TCamera::TCamera(float, float, const char*)` body, moving
  `TNameRefGen::getNameRef` 65.6% -> 72.5%. This was treated as
  diagnostic evidence, not committed source: a branch-local temporary,
  `new TPolarCamera` without parens, and moving `TPolarCamera`'s inline
  constructor body out of the class did not reproduce the boundary, and
  there is no symbol/name evidence for a one-off factory helper.

**Related:** see also the `MWCC sometimes inlines a call selectively`
hypothesis in this file — the wrapper trick is the source-level lever
to control selective inlining, and may resolve that hypothesis once
applied broadly.

### Vtable order in headers must match target asm one-for-one

**Rule:** MWCC emits `virtual` methods into the vtable in their lexical order
of declaration in the class definition. Even a single virtual swap in the
header shifts every later slot by 4 bytes, causing all virtual-call sites
that dispatch the later methods to use the wrong vtable index. The symptom
at the call site is a `lwz r12, OFFSET(r12)` whose `OFFSET` differs from
target by exactly 4 bytes (or a multiple of 4 if multiple slots are wrong),
with no other instruction differences.

A non-`virtual` method declared between `virtual` methods does NOT consume
a vtable slot but does NOT shift later slots either — but if a method
should be virtual and isn't, it's missing from the vtable entirely.

**How to detect:** Diff the function's asm against the target; if the only
mismatch is a single virtual-call `lwz r12, OFFSET(r12)` (and possibly a
trailing `bne 0xXXX` whose only difference is the branch target shifted
by 4 due to the size mismatch), it's a vtable-order bug. The fix is in the
class header, not the .cpp file. The class's own vtable (`__vt__<class>`)
will also appear in the original .o; reading off the symbol order there
gives the canonical order.

**Tool:** `tools/agent/find_vtable_diffs.py` scans `report.json` for
near-match functions with exactly this pattern and prints them as
single-line fixes.

**Where observed:**
- `mario/Player/MarioCollision::floorDamageExec` — target used
  `lwz r12, 0xdc(r12)` (damageExec); ours used `0xd8` (checkCollision).
  TMario header had `damageExec` declared BEFORE `checkCollision`; target's
  vtable order is `checkCollision`, `damageExec`, `getVoiceStatus`,
  `drawSyncCallback`. Also, `drawSyncCallback` was declared non-virtual in
  the header but appears in target's vtable. Fix: swap declaration order
  and add `virtual` to `drawSyncCallback`. Net: +3 matched functions.
- `mario/MoveBG/MapObjSirena::TPanelRevolve::control` — target used
  `lwz r12, 0x114(r12)` (`setUpCurrentMapCollision`); source called
  `updateObjMtx()` (`0x110`). Method name in source was wrong, not vtable
  order — same diagnostic signature.

### Splitting `a + b * c` into two statements defeats `-fp_contract on` fusion

**Rule:** Under the project-wide `-fp_contract on` flag, MWCC fuses
`a + b * c` (or `a - b * c`) into a single `fmadds`/`fmsubs`
instruction. To force the unfused `fmuls; fadds` pair the target
emits, split the expression across two statements with an
intermediate `f32` local:

```cpp
// Fused — produces fmadds f0, b, c, a (1 instr)
f32 r = a + b * c;

// Unfused — produces fmuls tmp, b, c ; fadds r, a, tmp (2 instrs)
f32 r = b * c;
r = a + r;
```

The intermediate store/reload of the multiply result through the
named local introduces a sequence point that MWCC respects: it will
not fuse across the assignment.

**Where observed:**
- `src/NPC/NpcTrample.cpp::updateTrample` —
  `*out = dt * (1.0f + unk0 * (-JMASSin(angle)))` produced
  `fmadds` instead of target's `fmuls; fadds; fmuls`. Splitting
  into `f32 mod = unk0 * (-JMASSin(angle)); mod = 1.0f + mod;
  *out = dt * mod;` produced the target's unfused pair (90.9%→99.2%).

### `JMASSin(idx) + JMASCos(idx)` inline at use site → shared address bases

**Rule:** When the same `s16` index is passed to both `JMASSin` and
`JMASCos`, inlining the lookups *at the assignment-site expression*
(rather than caching `sin_a`/`cos_a` as `f32` locals) causes MWCC to
share the index computation via CSE and to precompute the per-table
addresses (`add r6, sin_table, idx*4`; `add r5, cos_table, idx*4`)
once for reuse across the dependent expressions.

```cpp
// Locals — MWCC emits `lfsx fN, base, idx` indexed loads, possibly
// reloading the table base each time.
f32 sin_a = JMASSin(yawShort);
f32 cos_a = JMASCos(yawShort);
dst.x = a * cos_a + b * sin_a;
dst.z = -a * sin_a + b * cos_a;

// Inlined at use — `add rA, table, idx*4; lfs fN, 0(rA)`. The
// shifted-index register can be reused for the second expression's
// table loads.
dst.x = a * JMASCos(yaw) + b * JMASSin(yaw);
dst.z = -a * JMASSin(yaw) + b * JMASCos(yaw);
```

**Where observed:**
- `src/NPC/NpcThrow.cpp::throwMario` — caching `sin_a`/`cos_a` gave
  88.6% (lfsx form, with `lwz r4, table` reloaded between the two
  rotation expressions). Inlining `JMASSin/JMASCos` directly in the
  two assignments gave 100% (target's `add; lfs 0(rN)` form, with
  the indices/table-bases reused across both expressions).

### Initialise pointer to NULL before the if to fall through the null path

**Rule:** Target prefers `Type* p = NULL; if (cond) p = call();` (init
+ conditional override) over `Type* p; if (cond) p = call(); else
p = NULL;` (explicit else). The init form produces:

```
li r28, 0              ; init NULL
... test ...
bne CALL               ; if non-null, jump to call
b MERGE                ; fall-through past NULL path
CALL: bl getter
mr r28, r3
MERGE:
```

The explicit-else form produces an extra `li r28, 0` in the else
branch:

```
... test ...
beq SETZERO
bl getter
mr r28, r3
b MERGE
SETZERO: li r28, 0
MERGE:
```

The init form drops 1-2 instructions and matches target's typical
layout.

**Where observed:**
- `src/NPC/NpcInbetween.cpp::execMotionBlend` — first occurrence of
  `J3DAnmTransform* old_ptr;` inside the forced-blend branch went
  82.3%→84.3% when rewritten as `J3DAnmTransform* old_ptr = nullptr;
  if (mactor->unkC) old_ptr = ...;`. Note the second occurrence in
  the same function keeps the explicit if/else — so the choice is
  per-call-site, not TU-wide.

### 1-case `switch` defeats the bne-skip optimization for `if (x == K) call();`

**Rule:** Source `if (x == K) call();` produces a 3-instruction
optimized sequence:
```
cmpwi r0, K
bne SKIP
bl call
SKIP:
```
Target sometimes emits the 4-instruction "literal if-then" form instead:
```
cmpwi r0, K
beq DO_CALL
b SKIP
DO_CALL: bl call
SKIP:
```

To force the 4-instruction form, wrap the call in a 1-case switch:
```cpp
switch (x) {
case K:
    call();
    break;
}
```

This is a refinement of the broader rule that `switch` defeats
optimization patterns (see "switch defeats fusion of multiple equality
compares"); even single-case switches can shift codegen.

**Where observed:**
- `src/System/MarDirectorEvent.cpp::movement` — target uses 4-instruction
  `beq;b;bl` layout for `if (mState == 4) movement_game();`. The bare
  `if` form gave 91% (3-instruction `bne;bl`); switching to
  `switch (mState) { case 4: movement_game(); break; }` matched 100%.
- `src/GC2D/hx_wiper.c::Hx_StartWipe` — target gated `Hx_Warning(1)`
  with `beq body; b after`; the bare `if (hx.state == 2)` emitted
  `bne after`. A 1-case `switch (hx.state)` moved 97.2%→100%.
- `src/GC2D/hx_wiper.c::Hgx_ReadTexture` — same branch-layout lever on
  `hx.resFlag == 0` around the DVD read block, moving 96.8%→100%.

### Direct `this->[OFFSET]` field access vs static helper with hit_actor arg

**Rule:** When a non-static method calls a static helper that accesses
`((CastT*)arg)->field`, and the calling method's compiled body needs
to access `this->[OFFSET]` (not `arg->[OFFSET]`), the original source
likely does NOT call the helper — it directly accesses the field via
`this`. Even though the helper's body looks identical, calling
`helper(arg)` produces `lwz r?, OFFSET(r4)` (uses the arg in r4) while
direct `((CastT*)this)->field` produces `lwz r?, OFFSET(r3)` (uses
this).

Pattern: when target's body has `lwz r0, 0x68(r3)` (r3=this) but our
source calls a helper that takes a hit_actor arg, rewrite the body to
access the field directly via `((CastT*)this)->field`.

**Where observed:**
- `src/MoveBG/MapObjLib.cpp::getWaterPos/getWaterSpeed/getWaterPlane`
  — target accessed `this->[0x68]` (the TTakeActor::mHolder field
  slot, treated as a TWaterHitActor::unk68 water-ID). Previously we
  called `getWaterID(hit_actor)` which inlined to use the arg in r4.
  Replacing with `((TWaterHitActor*)this)->unk68` matched all three
  100%.

### 2-case fused switch produces midpoint-excluded bisection

**Rule:** A switch with exactly two case labels falling into one block:
```cpp
switch (x) {
case A:
case B:
    result = true;
}
```
compiles to a "midpoint-excluded" binary search:
```
cmpwi r, MID    ; MID = (A+B)/2
beq END         ; midpoint → false (since MID ∉ {A, B} when A,B differ by 2)
bge UPPER       ; > MID → check B
cmpwi r, A
bge SET         ; ≥ A and < MID → SET (only A matches here)
b END
UPPER:
cmpwi r, B+1
bge END         ; ≥ B+1 → false
SET: li r, 1    ; ≤ B and > MID → true (only B)
END:
```

Critically, this is **NOT** the same as `if (x == A || x == B)`, which
MWCC tends to compile to two flat `cmplwi`/`beq` pairs. The switch form
is what produces the bisection.

When the target asm shows the `cmpwi midpoint; beq END; bge UPPER; ...`
pattern, it's a 2-case switch — even if the two values look like they
could be a range (e.g. 0x88B/0x88D with 0x88C "missing" between them).
The midpoint is mechanically excluded; do not add it as a third case.

**Where observed:**
- `src/Camera/CameraMarioData.cpp::isMarioRocketing` — target had
  `cmpwi r3, 0x88c; beq END; bge UPPER; cmpwi 0x88b; bge SET; b END;
  UPPER: cmpwi 0x88e; bge END; SET: li r31, 1`. Looked like a range
  check 0x88B–0x88D but was actually `case 0x88B: case 0x88D:`. The
  3-case form (with 0x88C added) compiled to a clean range check
  `cmpwi 0x88e; bge END; cmpwi 0x88b; bge SET; b END` (85.4% match);
  removing 0x88C gave the midpoint-excluded bisection → 100%.

### Callee return type `bool` → callers emit `clrlwi r0, r3, 24` on the result

**Rule:** Helper / template functions declared to return `bool` (u8 in this
ABI) cause every caller that consumes the return value in an integer or
BOOL context to mask the result with `clrlwi r0, r3, 24` before testing it.
Target asm typically uses the raw `r3` directly (e.g. `cmpwi r3, 0` or
`mr/cmpw`-style chains), meaning the original declaration was `BOOL`/`int`,
not `bool`.

If multiple callers exist, change both the helper's return type AND any
forwarding function that returns its result. A helper that returns `BOOL`
but is wrapped by a `bool`-returning function still emits a BOOL→bool
convert at the wrapper's `return` (`neg r3, r3; subic r0, r3, 0x1;
subfe r0, r0, r3; clrlwi r3, r0, 24`).

**How to detect:** target shows the call followed by a direct cmpwi/cmpw,
ours shows `bl helper; clrlwi r0, r3, 24; ...`. Or the wrapper function
shows the four-instruction BOOL→bool convert sequence right after a call.

**Where observed:**
- `include/Camera/cameralib.hpp::CLBChaseGeneralConstantSpecifySpeed<T>`
  was declared `bool`. Changing it to `BOOL` (and matching its forwarder
  `CLBChaseSpecialDecrease`) closed `clrlwi` mismatches in
  `TBaseNPC::execTurnToFirstState` (97.98% → 99.04%),
  `TBaseNPC::execUTurn` (97.97% → 98.84%), and
  `CPolarSubCamera::execHeightPan_` (51.4% → 52.3%).
  Confirmed in two TUs (NpcWalkTurn, CameraHeightPan).

**Inverse direction — per-call-site `(u8)` narrowing on a `BOOL` predicate,
and the frame-collapse it triggers.** The *opposite* shape also occurs: a
`BOOL`-declared predicate whose result the **target narrows** with
`clrlwi. r0, r3, 24` (8-bit test) while **our** build does the raw
`cmpwi r3, 0x0`. Here the fix is NOT to change the predicate's return type
(other callers want the wide test) but to add a per-call-site `(u8)` cast:
`if ((u8)pred())`. The original author wrote the cast at exactly the sites
that narrow. Critically, this co-occurred with what looked like a
**phantom-inline frame-pad** (target frame *larger*: -0x48 vs -0x28): adding
`(u8)` fixed the `clrlwi` AND collapsed the frame to match. So a frame-size
delta sitting next to a `clrlwi.`-vs-`cmpwi` diff is a *downstream effect of
the wide-int test path*, not an independent inline — don't write it off as
currently-hard before checking for this cast. `Player/MarioRun`:
`getSlideStickMult` 98.5→99.9, `getSlopeNormalAccele`/`getSlopeSlideAccele`
99.2→99.9, `getChangeAngleSpeed` 98.0→98.6 — all `(u8)isForceSlip()`,
matching the pre-existing `(u8)isForceSlip()` sites in `MarioMove.cpp`.
Scan for the signature with `tools/agent/find_bool_narrow.py`.

### `switch` + `default:` wraps follow-up logic when target jumps past it

**Rule:** When source has the shape

```cpp
switch (x) { case A: case B: result = FALSE; break; }
if (some_other_condition) result = FALSE;
return result;
```

— and target's case bodies end with `b END_OF_FUNCTION` (skipping the
follow-up `if` entirely) — the original source moved the trailing
`if (some_other_condition)` into the `default:` branch. With:

```cpp
switch (x) {
case A: case B:
    result = FALSE;
    break;
default:
    if (some_other_condition) result = FALSE;
    break;
}
return result;
```

the case bodies emit `li r6, 0; b end` instead of `li r6, 0` followed by
fall-through into the always-run condition. This is the difference between
"case sets result, conditionally also runs follow-up" and "case OR
default, never both" — semantically equivalent when the case body sets
result FALSE (since the follow-up only ever sets FALSE too) but
structurally distinct in MWCC's output.

**How to detect:** target's last switch case ends with `li rN, 0; b END;`
where END is past a conditional block your build runs unconditionally.

**Where observed:**
- `TBaseNPC::isTurnToMarioWhenTalk` — switch on 5 actor-type cases
  followed by an `mActionFlag & 0xC01` check. Moving the flag check
  into `default:` closed 96.55% → 100%.
- `TBaseNPC::isTurnToMarioWhenApproach` — range `0x16..0x18` enumerated
  as cases, follow-up flag check (`mActionFlag & 0x7E7F`) moved to
  `default:`. 87.96% → 100%.

### `bool` vs `BOOL` local must match the function's return type

**Rule:** When a function returns `bool` (u8) and uses a local result variable,
declare it as `bool result = false` — NOT `BOOL result = FALSE`. The latter
forces MWCC to emit an int→bool cast at return:
`neg r3, r0; addic r0, r3, -1; subfe r0, r0, r3; clrlwi r3, r0, 24`.
With `bool result = false`, the function returns the local directly
(`mr r3, r0; blr` or equivalent) with no cast.

Symmetrically, a `BOOL`-returning function should use `BOOL result = FALSE` —
mismatched bool↔BOOL in either direction creates a cast at the boundary.

**Where observed:**
- `src/Camera/CameraMarioData.cpp::isMarioGoDown` — function returns `bool`.
  With `BOOL result = FALSE`, ended at 76% (4 extra instructions for the
  cast). Changing to `bool result = false` → 100%.
- `src/NPC/NpcChange.cpp::TBaseNPC::isNerveWalk` — after changing the function
  return type to `bool`, leaving `BOOL result = FALSE` inserted the same
  int→bool cast and dropped the owner to 80.1%; changing the local to
  `bool result = false` restored the owner to 100%.

### Ternary `? true : false` produces intermediate-byte sequence

**Rule:** `return cond ? true : false` (with `bool` literals) compiles to
`li r0, 1 / b set / li r0, 0 / set: clrlwi r3, r0, 24` — an
intermediate-byte materialisation followed by a u8 mask. The cleaner
`if (cond) return TRUE; return FALSE;` (with `BOOL` literals and a
`BOOL`-returning signature) compiles to two separate `li r3, K; blr`
return blocks — direct int returns.

Choose the form by matching the target's exit pattern:
- Target has shared exit with mask → use ternary with `bool`.
- Target has split exit blocks with `li r3, 0/1; blr` directly → use
  `if/return` with `BOOL`.

**Where observed:**
- `src/Enemy/bossgesso.cpp::is2ndFightNow` — `return cond ? true : false`
  gave 78.7%. Switching to `if (cond) return TRUE; return FALSE;` (and
  matching `BOOL` return type behaviour) → 100%.

### `__fabsf` intrinsic survives `#pragma dont_inline on`

**Rule:** Under TU-global `#pragma dont_inline on`, MWCC does NOT inline
the `fabsf(x)` library wrapper from `<math.h>` — it emits a `bl fabsf`
call instead of the single `fabs f1, f1` PowerPC instruction. The
`__fabsf` compiler intrinsic, by contrast, is *not* an inline function
but a builtin: it lowers directly to `fabs f1, f1` regardless of any
inlining pragmas.

When a TU has `dont_inline on` set, always use `__fabsf` rather than
`fabsf` (and likewise `__fabs` for double). Without it, callers gain a
`bl` call plus extra stack frame for the call.

**Where observed:**
- `src/NPC/NpcBase.cpp::isInMadSearchRange`/`isInBodyTurnSearchRange`
  with `fabsf` and `dont_inline on` produced a `bl ` call and 48-byte
  stack frame. Switching to `__fabsf` produced the inline `fabs`
  instruction and dropped to 24-byte stack frame, jumping from ~24% to
  98% match.
- Cross-confirmed in `src/Animal/Bird.cpp` and several Enemy/ files —
  they all use `__fabsf` explicitly, suggesting this is a known
  workaround.

**Related:** This is a special case of the broader rule that `.get()`
accessors on `TParamRT<T>` get blocked from inlining under
`dont_inline on`. Bypass it by accessing `.value` directly.

### TParamRT<T>::value direct access under `#pragma dont_inline`

**Rule:** Under TU-global `#pragma dont_inline on`, the inline
`get()` accessor on `TParamT<T>` cannot be inlined — MWCC emits a
`bl` call to a stub returning the address of `value`. Accessing
`param->mField.value` directly skips the accessor and produces the
expected `lfs f0, OFFSET(rN)` instruction.

**Where observed:**
- `src/NPC/NpcBase.cpp::isInMadSearchRange` — `param->mMadSearchHeight.get()`
  with `dont_inline on` produced a `bl ...` call; switching to
  `param->mMadSearchHeight.value` eliminated the call.

### Intermediate `bool match` local produces target's r0→r31 pattern

**Rule:** When target uses an intermediate result register
(typically r0) for an inner expression's outcome, then `mr r31, r0`
at the end — declaring the result as a SEPARATE local that is
assigned in both branches of an if/else and then copied to the
final result produces this code shape. The simpler
`bool result = false; if (cond) result = true;` form (without the
intermediate temp) produces `li r31, 1` direct stores instead.

**Pattern (target asm):**
```
... bunch of compares + beq to TRUE block ...
TRUE:  li r0, 1; b L_SET
FALSE: li r0, 0
L_SET: mr r31, r0
```

**Source that matches:**
```cpp
bool match;
if (test) {
    match = true;
} else {
    match = false;
}
result = match;
```

**Where observed:**
- `src/Camera/CameraMarioData.cpp::isMarioSlider` — adding the
  intermediate `bool match` after the OR-chain test took the
  function from 95.8% (with direct `result = true` in the
  if-block) to 100%.
- Implicitly used as the standard NPC-predicate pattern in
  `src/NPC/NpcBase.cpp` for all isXxx functions added this tick
  (isPollutionNpc, isChild, isBehaveToWaterNpc, etc).

### Single-equality `switch` defeats subis fusion too

**Rule:** Even when there's only ONE equality check against a
constant with shared high bits (e.g. `mActorType == 0x04000013`),
MWCC will compile `if (x == K) ...` with subis-fusion
(`subis r4, r5, 0x400; cmplwi r4, 0x13`) instead of the target's
direct `addi r0, r4, 0x13; cmpw r5, r0`. Wrapping the single case
in a `switch (x) { case K: ... }` defeats the fusion and produces
the target's two-instruction comparison.

This is a refinement of the broader switch-defeats-fusion rule
below — even single-case switches help when the constant has
high bits that would otherwise trigger fusion.

**Where observed:**
- `src/NPC/NpcBase.cpp` — `isNormalMareW`, `isNormalMareM`,
  `isSpecialMonteW` each check one mActorType value (0x04000013,
  0x0400000E, 0x0400000D). Direct `if` form gave 22.5%–59% match
  after the bool-result rewrite; wrapping in a 1-case switch took
  them all to 100%.

### `switch` defeats fusion of multiple equality compares

**Rule:** When source has multiple equality compares of an int against
distinct constants — whether the high bits share (`subis` fusion) or the
constants are consecutive (`subi + cmplwi range` fusion) — MWCC's optimiser
will fold them into range-bisection arithmetic that doesn't match target's
clean `cmpwi + beq/bge` tree. Rewriting the dispatch as
`switch (x) { case a: case b: ...: result = true; } return result;`
reliably defeats the fusion and produces target's binary-search comparison
pattern.

**Companion rule — initializing a `bool result = false` upfront** hoists
`li r3, 0` to BEFORE the comparison tree and unlocks MWCC's
conditional-return idiom: false paths emit `beqlr`/`bltlr`/`bgelr` instead
of a unified `li r3, 0; blr` block. Target consistently uses this pattern
for predicate functions.

**Where observed:**
- `src/Camera/CameraTalk.cpp::makeMtxForTalk` — high-bits-shared constants
  `0x400001A`, `0x4000007` → `subis r4, r5, 0x400 ; cmplwi r4, 0x1a ; ...
  cmplwi r4, 0x7`. Switch rewrite produced `cmpw + beq + bge`. 74.7% → 84.4%.
- `src/Camera/CameraMode.cpp` — six independent predicate functions
  (`isTalkCameraSpecifyMode`, `isFixCameraSpecifyMode`,
  `isDefiniteCameraSpecifyMode`, `isFollowCameraSpecifyMode`,
  `isTowerCameraSpecifyMode`, `isLButtonCameraSpecifyMode`) all went from
  39–55% → 100% by converting `if (x == A) return true; ...` chains to
  `bool result = false; switch (x) { case A: ...: result = true; } return result;`.
  Consecutive-value fusion (`subi + cmplwi range`) and range checks
  (`mode >= A && mode < B` rewritten as explicit case enumeration) also
  matched after the switch rewrite.

**Consequences:**
- When target shows a binary-search `cmpwi/beq/bge` tree and our build
  emits `subi*+cmplwi range`, the switch rewrite is the correct source
  structure — even when the function "looks like" an if/else chain in
  intent. Range checks `x >= A && x < B` are best expressed as explicit
  `case A: case A+1: ... case B-1:` enumeration if they are part of a
  switch-like dispatch.
- For boolean predicates whose target loads `li r3, 0` upfront and uses
  `*lr` conditional returns, write `bool result = false; ... result = true; return result;`
  rather than `if (...) return true; ... return false;`.

## Hypotheses under investigation

### Direct const-reference `TVec3` temporaries can recover right-to-left argument stack construction

**Hypothesis.** When a target call takes several `const TVec3<float>&`
arguments and the asm constructs stack temporaries in reverse argument order
(for example `scale`, then `rot`, then `pos`) while still passing the normal
argument registers (`r4 = pos`, `r5 = rot`, `r6 = scale`), spelling the call
with direct `TVec3` temporaries can match better than naming locals first.
Named locals may preserve semantics but encourage MWCC to allocate and store
the three vectors in declaration/order-of-use shapes that differ from the
target.

**Observed.** `mario/Enemy/hanasambo` (2026-06-11 MNL):
`TSamboFlower::loadAfter()` changed from named `pos`, `rot`, and `scale`
locals to
`newAndRegisterObj("coin", TVec3(0,0,0), TVec3(0,0,0), TVec3(1,1,1))`.
MWCC then constructed the const-reference temporaries in the target stack
order and the function moved `98.7 -> 100.0`. Merely reordering named local
declarations improved store order (`98.7 -> 99.5`) but put the argument
registers on the wrong stack slots versus target.

**Experiment to confirm/refute.** Find a second TU with a `const TVec3&`
factory/helper call and target stack temps constructed right-to-left. Compare
named locals, declaration-order permutations, and direct temporaries; confirm
whether only the direct temporary spelling preserves both stack construction
order and final argument-register slots.

### `TVec3::set(x, y, z)` can recover batched matrix-translation copies

**Hypothesis.** When target asm loads three matrix translation components
(`mtx[2][3]`, `mtx[1][3]`, `mtx[0][3]`) before storing a `TVec3` destination,
component-by-component assignments may interleave each load with its store.
Writing the copy as `dst.set(mtx[0][3], mtx[1][3], mtx[2][3])` can force MWCC
to evaluate the three arguments first and then store `x/y/z`, matching the
target's batched load/store shape. Expect possible FPR naming residue.

**Observed.** `mario/MoveBG/MapObjCorona` (2026-06-11 MNL):
`TBathtubGrip::control()` improved 98.7 -> 99.3 after replacing three stores
to `unk144` with `unk144.set(...)`; `TBathtub::control()` improved
89.8 -> 91.6 after replacing three stores to `unk200` with `unk200.set(...)`.
Both target blocks used the same batched translation-copy shape.

**Experiment to confirm/refute.** Find a second TU with a nonmatching
matrix-translation-to-`TVec3` copy where target loads all three components
before storing. Toggle only component assignments vs. `TVec3::set(...)` and
verify whether the batched argument evaluation repeats.

### `.cpp` definition plus scoped `dont_inline` can recover same-TU static template-member call boundaries, but may emit the wrong owner kind/frame

**Hypothesis.** When target asm owns a standalone helper for a header-visible
static template member and same-TU callers branch to it, moving only that
specialization's body out of the class/header and wrapping the `.cpp`
definition in `#pragma dont_inline on/off` can make MWCC emit and call the
helper. This is weaker than a settled rule because the recovered owner may be
strong rather than weak and may still have frame-size residue; treat it as a
call-boundary lever, not a final weak-owner explanation.

**Observed.** `mario/Enemy/koopajr` (2026-06-10 MNL):
`JGeometry::TUtil<f32>::mod(float, float)` was target-owned in this TU and
called by `TDirectionCalc::absDirection`, `calcTurnDirection`, and `sub`, while
the header in-class body inlined at all three sites and left the helper
missing. Declaring `mod` in `JGUtil.hpp`, defining it in `koopajr.cpp` between
`checkNerve` and `makeRoundVelocity`, and wrapping that definition in
`#pragma dont_inline` emitted the owner and changed the callers to `bl mod`,
moving `absDirection` 67.7 -> 85.7, `calcTurnDirection` 73.7 -> 83.5, and
`sub` 66.8 -> 85.3. The body itself is 99.7%; residue is target 0x28 frame vs
current 0x20 and weak target owner vs strong current owner. The helper body
also needs a signed quotient cast, `(s64)(value / modulus)`, to get
`__cvt_sll_flt`; `(u64)` emits `__cvt_ull_flt`.

**Refuted local variants.** Header-only `#pragma dont_inline` around the
in-class body still emitted no owner. Marking the `.cpp` definition `inline`
also dropped the owner entirely. An explicit-specialization spelling compiled
but still emitted a strong owner and did not change the frame.

**Experiment to confirm/refute.** Find another target-owned static template
member with same-TU callers where the current header body inlines. Try the
`.cpp` definition + scoped `dont_inline` shape, then inspect whether the owner
kind/frame mismatch repeats. If a second TU gains call boundaries but keeps a
strong/small-frame owner, split this into two rules: call-boundary recovery is
real, weak-owner/frame reproduction remains open.

### Direct member expressions can avoid preserving owner locals across calls

**Hypothesis.** If target asm reloads an owner/member pointer after a call, but
current source keeps it live in a callee-saved GPR, removing the cached owner
local and spelling the accesses as direct member expressions (`unk0->...`) can
make MWCC treat the pointer as call-clobberable and reload it later. This can
also reduce extra saved GPR pressure, but may shrink the stack frame and leave
frame-size residue.

**Observed.** `mario/Enemy/tinkoopa`
`TTinKoopaLaunchOrder::checkOrder()` (2026-06-10 MNL): replacing
`TTinKoopa* koopa = unk0` with direct `unk0->...` expressions, then writing the
null actor branch as `if (!unk0->unk164) ... else ...`, matched the target's
actor load/call branch shape and owner reloads. Combined with explicit branch
temps for the count clamps, the function moved 66.9 -> 96.2. Remaining residue
is stack frame size and register coloring.

**Experiment to confirm/refute.** Find a second member-helper function where
target reloads a member owner/pointer after an intervening call and current
source preserves a local owner across that call. Toggle only the cached local
to direct member expressions and verify whether MWCC drops the saved owner
register without introducing repeated-load mismatches.

### Naming repeated float field reads can promote them into saved FPR lifetimes

**Hypothesis.** When a hot expression repeats several struct float fields across
multiple arithmetic predicates, writing those fields as named `f32` locals can
make MWCC keep the earliest values in saved FPRs (`f31/f30`) instead of using
volatile temporaries. This can match targets that load the fields once, reuse
them across adjacent tests, and spill saved FPRs even though the direct repeated
member-expression source is semantically identical.

**Observed.** `mario/Map/MapCheck` (2026-06-10 MNL):
`TMapCollisionData::checkRoofList()` and `checkGroundList()` both repeat
triangle X/Z coordinates across three containment tests. Naming
`point1x/point1z/point2z/point2x/point3z/point3x` matched the target
`f31/f30` lifetimes and load order, moving `checkRoofList()` 97.5 -> 99.0 and
`checkGroundList()` 97.7 -> 98.9. Remaining residue is frame size and
base-register allocation.

**Experiment to confirm/refute.** Find a second TU with repeated scalar float
field reads across adjacent geometric predicates where the target holds the
first fields in saved FPRs and current source uses volatile FPRs. Toggle only
the repeated member expressions into named locals and verify whether saved FPR
allocation and load order move toward target without requiring stack padding.

### Caching a member loop bound plus binding each struct-array record can unlock MWCC's 8-way unroller

**Hypothesis.** For loops over a struct array where the target has the
`srwi/mtctr/bdnz` 8-way unroller, source that tests directly against a mutable
member bound (for example `i < this->count`) may keep a single-body loop because
MWCC must reload the bound. Copying the member to a local before the zero check
can expose an invariant bound and trigger the unroller. If the loop body reads
multiple fields from the same record, also bind a per-iteration record reference
(`const T& rec = arr[i];`) so MWCC computes one record pointer and uses regular
`lfs 0/4/8(rec)` loads instead of separate `lfsx` component offsets.

**Observed.** `mario/Animal/BeeHive` `TBeeHive::getCenterOfGravity() const`
(2026-06-09): caching `mWaitTimer` as `count` changed the loop from a
single-body member-reload loop to the target 8-way unroll family. Adding
`const TBoid& boid = mBoidLeader->mBoidData[i]` changed the component loads
from `lfsx` to the target record-pointer `lfs 0/4/8` pattern, moving the
function `30.9% -> 88.1%`.

**Experiment to confirm/refute.** Find a second struct-array accumulation or
copy loop where target has the 8-way unroller and current source either reloads
a member bound or uses `lfsx` per component. Apply the bound-local and
record-reference parts separately, then promote only if both effects reproduce.

### A placeholder data member before first virtuals reproduces nonzero vptr offsets in lightweight local declarations

**Hypothesis.** For local "header substitute" class declarations, MWCC places
the vptr at the point where the first virtual methods appear in the class
declaration, not necessarily at offset 0. If target asm calls through
`lwz r12, N(r3); lwz r12, slot(r12); blrl`, and the real class has data before
the vptr, a lightweight declaration with only virtual methods emits the wrong
`lwz r12, 0(r3)`. Add a real placeholder byte array before the virtuals to put
the vptr at the observed offset.

**Observed.** `mario/Player/Yoshi` `YoshiHeadCtrl` calls
`TWaterGun::getCurrentNozzle()` then dispatches `TNozzleBase::getGunAngle()`
through `lwz r12, 0x364(r3); lwz r12, 0x10(r12)`. The local placeholder class
in `Yoshi.cpp` had only virtual declarations and emitted `lwz r12, 0(r3)`.
Adding `u8 _0[0x364];` before the virtuals reproduced the target vptr load and
moved `YoshiHeadCtrl` `99.7% -> 99.8%`.

**Experiment.** Find a second local placeholder class where target virtual
dispatch loads a vptr from a nonzero offset, preferably one already using raw
offsets elsewhere. Add placeholder data before the first virtual declarations
and verify that MWCC moves only the vptr load without changing call semantics.
Promote only if the declaration-order effect repeats.

### Widening a narrowed byte loop bound can preserve conversion scheduling while keeping byte compare codegen

**Hypothesis.** When a target computes a byte-sized loop bound from float math
near surrounding byte-table copies, spelling the local as `u32 n` while assigning
through an explicit byte cast (`n = (u8)(int)(...)`) can keep the target's
`clrlwi` byte compare but let MWCC schedule the float conversion early enough to
interleave with independent byte loads/stores. A source `u8 n` may make MWCC
home the narrowed value in a way that delays the conversion until after the
whole byte-copy block, even though the final loop uses the same byte range.

**Current symptom.** `mario/GC2D/hx_wiper` `Hx_SetVFilter` target interleaves
the seven-byte `vtable_org -> vtable` copy with
`lfs/fmuls/fctiwz/stfd/lwz` for `(u8)(64.0f * ratio)`. With `u8 n`, current
MWCC scheduled the full table copy before the conversion and the function was
86.3%. Changing only the local to `u32 n` while keeping
`n = (u8)(int)(64.0f * ratio)` restored the target interleaving and moved the
function to 98.9%; remaining residue is frame/loop-cleanup shape.

**Experiment.** Find a second TU where a narrowed `u8`/`u16` loop bound is
computed from float or integer math around independent byte/halfword table
copies and the target interleaves the conversion with those copies. Toggle only
the local's storage type wider while preserving an explicit narrow cast at
assignment. Promote if the scheduling and narrowed compare repeat; refute if
the effect is specific to `Hx_SetVFilter`'s unrolled copy/loop body.

### Post-call block-local `GXColor` aggregate initialization can direct-fill the final stack slot

**Hypothesis.** When a target initializes a `GXColor` after one or more calls by
loading a packed color constant from sdata, storing it directly into the mutable
color stack slot, then byte-patching channels before a by-value GX call, place
the declaration in a nested block after those calls:

```c
{
	GXColor color = { 0xff, 0xff, 0xff, 0xff };
	color.a = alpha;
	GXSetTevColor(GX_TEVREG0, color);
}
```

This preserves C89 declaration ordering while letting MWCC direct-initialize
the real local. A top-level `GXColor color;` followed by
`color = (GXColor) { ... };` can emit a compound-literal temp and an extra
copy; a packed `*(u32*)&color = ...` assignment can fold the constant to an
immediate instead of using the target sdata load.

**Observed.** `mario/GC2D/hx_wiper` `Hxs_Logo_TexSetup` and
`Hxs_Logo_ExtraDraw` (2026-06-08 1:30am MNL): moving the `GXColor`
declarations into post-`Hgx_init_tobj_resource` nested blocks removed the
compound-literal temp, restored the target frame sizes, and made both functions
exact after the callee was kept out of line.

**Experiment.** Find a second TU where target asm loads a packed `GXColor`
constant from sdata immediately after calls and byte-patches it before a GX API
call. Compare top-level declaration plus compound assignment, packed `u32`
store, and nested-block aggregate initialization. Promote if the nested-block
form consistently direct-fills the final color slot.

### Splitting repeated call arguments into distinct locals can force earlier register-to-register argument copies

**Hypothesis.** When a call passes the same scalar local to two argument
positions and another argument needs a late conversion, MWCC may delay the
duplicate register copy until after that conversion. Introducing a second local
assigned from the first can force the duplicate argument copy earlier while
preserving the visible call and value flow.

**Current symptom.** `mario/Player/MarioWait`
`TMario::waitingCommonEvents()` target sets up `IConverge(diff, 0, speed,
speed)` as `lha speed -> r5; addi r6,r5,0; extsh r3,diff; bl IConverge`.
The direct repeated-argument spelling emitted the `addi r6,r5,0` after the
`extsh`. Adding `s32 turnSpeed2 = turnSpeed` and passing `turnSpeed2` as the
fourth argument moved the copy before the conversion and took the function
`98.5% -> 100.0%` fuzzy.

**Experiment.** Find a second TU where target copies a repeated scalar argument
before a first-argument conversion or address calculation, while current source
passes the same local twice and copies later. Split only the repeated argument
into a second local; promote to Settled if the copy moves without changing
other scheduling, refute if this was specific to `IConverge` or this frame
shape.

### Positive-arm-first repeated-expression integer abs can force duplicated arithmetic in both branches

**Hypothesis.** When target asm computes an integer difference, branches on the
condition-code result, then recomputes the same difference in both the
non-negative and negative arms before an optional `neg`, avoid a precomputed
`diff` local. Instead, write the absolute value as a positive-arm-first
`if/else` over the repeated source expression:

```cpp
if (cur - prev >= 0)
	diff = cur - prev;
else
	diff = -(cur - prev);
```

The order of the source locals still matters for load order. This is a
natural spelling for preserving the original expression in both arms; it should
not be generalized to artificial recomputation unless the target clearly shows
duplicated arithmetic.

**Observed.** `mario/Camera/CameraSecureView`
`CPolarSubCamera::execSecureView_` (2026-06-07 12:41am MNL): replacing
`diff = cur - prev; if (diff < 0) diff = -diff;` with previous/current `s16`
locals and the positive-arm-first repeated expression matched the target
`subf.` / positive `subf` / negative `subf; neg` block and moved the function
`88.6% -> 91.1%`.

**Experiment to confirm/refute.** Find a second TU where target asm shows a
duplicated integer-difference absolute value and current source precomputes the
difference. Toggle only the precomputed-local form vs the repeated-expression
positive-arm-first `if/else`; confirm whether MWCC consistently preserves the
duplicated arithmetic and whether local declaration order controls the adjacent
load order.

### Direct-initialize a `TVec3` from a friend operator result to avoid an extra pre-copy temp

**Hypothesis.** When using the fabricated friend `TVec3` operators to preserve
an out-of-line `add`/`sub`/`scale` call boundary, a two-statement spelling can
materialize an extra source local before the by-value operator parameter:

```cpp
JGeometry::TVec3<f32> pos = source;
pos = pos + offset;
```

If the target copies `source` directly into the by-value operator parameter,
calls the helper, then copies the result into the named local, spell it as direct
initialization:

```cpp
JGeometry::TVec3<f32> pos = source + offset;
```

This keeps the known friend-operator call-boundary lever while avoiding the
additional pre-copy local that the two-statement form creates.

**Observed.** `mario/Map/MapWarp` `TMapWarp::watchToWarp` (2026-06-06):
`marioPos = SMS_GetMarioPos(); marioPos = marioPos + unk4[data].unk8;` kept the
target `TVec3::add` `bl` but emitted an extra local copy and reached 95.6%.
Direct-initializing `marioPos` from `SMS_GetMarioPos() + unk4[data].unk8`
removed that extra copy block and moved the function to 99.8%.

**Experiment to confirm/refute.** Find another TU already using a friend
`TVec3` operator to recover a helper call boundary, preferably one with an
extra pre-copy temp in the diff. Toggle only the two-statement vs direct-init
form and verify whether the direct-init spelling consistently copies the source
directly into the by-value operator parameter.

### Automatic const arrays can recover unnamed rodata copy-in constants where scalar bit-casts become immediates

**Hypothesis.** When target asm copies an unnamed constant object such as
`@NN` into stack slots before using those slots as floats, source may need an
automatic `const` array. Direct scalar `make_float(0x...)` expressions can
instead synthesize the values with `lis/addi` immediates and leave the unnamed
object missing. If the later uses are float loads, prefer an automatic
`const float[]` with exact bit-rounded literals over `const unsigned long[]`
plus `make_float(...)`: the integer array can recover the object but adds extra
bit-cast stack traffic, while the float array lets MWCC copy the rodata and load
the copied slots directly.

**Observed.** `mario/PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/Math/Single_precision/exponentialsf`
`__log2f` inline inside `powf` (2026-06-05): replacing two scalar
`make_float(0xBF38AA80)` / `make_float(0x3EF637A6)` constants with an automatic
`const unsigned long __log2e_m2[] = { 0xBF38AA80, 0x3EF637A6 };` made target
`@93` match and moved `powf` `65.0% -> 70.3%`. Retyping that same automatic
array as `const float __log2e_m2[] = { -0.7213516235351562f,
0.4808933138847351f };` kept `@93` matched, removed the extra bit-cast copies,
reduced the inlined caller frame, and contributed to the later `powf`
`76.6% -> 87.4%` lift.

**Experiment to confirm/refute.** Find an independent TU where target asm
copies a small unnamed rodata constant array to stack and current source uses
scalar bit-cast literals or immediate constants. Rewrite only those constants
as an automatic `const` float array when target uses `lfs` from the copied
slots, or as an integer array only when target uses integer loads, and verify
that the unnamed object appears without introducing a named static.

### Union bit-pattern locals can force target store/reload/shift shapes for synthesized floats

**Hypothesis.** When target asm builds a float bit pattern through a stack
local, spell the source as a union local with separate integer assignments and
a final `.f` read. A one-expression `make_float((n + k) << shift)` can let MWCC
fold directly to `slwi; stw`, skipping target's store/reload of the unshifted
integer. The union sequence:

```c
union {
	unsigned long u;
	float f;
} scale;

scale.u = (unsigned long)(n + 127);
scale.u <<= 23;
return scale.f * poly;
```

reproduces the target's `stw n+k; lwz; slwi; stw; lfs` shape.

**Observed.** In
`mario/PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/Math/Single_precision/exponentialsf`
`two_to_x` inlined into `powf`, replacing
`make_float((unsigned long)(n + 127) << 23)` with a union `scale` local matched
the repeated target scale setup and moved `powf` `87.4% -> 92.7%`.

**Experiment to confirm/refute.** Find another C-mode MSL/math TU where target
stores an integer exponent/mantissa component before shifting it into a float
bit pattern, while current source uses one folded bit-cast expression. Rewrite
only that expression through a union local and verify the store/reload/shift
sequence appears without adding unrelated frame growth.

### Inline helper OR chains can share the true block where repeated early returns duplicate materialization

**Hypothesis.** When an inline helper calls the same predicate several times
and returns true if any call succeeds, the spelling controls result
materialization after the helper is inlined. Repeated early returns:

```cpp
if (A())
	return true;
if (B())
	return true;
if (C())
	return true;
return false;
```

can make MWCC materialize `li result, 1; b end` after each successful test.
A single short-circuit OR:

```cpp
if (A() || B() || C())
	return true;
return false;
```

can instead branch each successful predicate to one shared true block, matching
target asm that has `bne L_true` after the first two calls and only assigns the
bool once near the end of the inlined helper.

**Observed.** `mario/Map/MapArea`
`TMapCollisionData::polygonIsInGrid()` (t413): rewriting the inlined
`checkLinePolygonCollision()` helper from three repeated early-return checks to
one OR expression removed duplicate true materialization blocks in four
inlined call groups and moved the function `94.9% -> 95.9%`.

**Experiment to confirm/refute.** Find another inlined helper with repeated
same-result early returns where target branches all successful calls to one
shared assignment block. Rewrite only the helper as a short-circuit OR and
verify whether the duplicate `li/b` blocks disappear without changing argument
evaluation order.

### One short-circuit guard assigning a shared result block may avoid duplicate false/true assignment blocks

**Hypothesis.** When several predicates all assign the same result value and
target asm branches every successful predicate to one shared assignment block
(`li result, 0` or `li result, 1`), write the source as one short-circuit guard
with a single assignment body:

```cpp
bool result = true;
if (mode == BAD || (helper() && !isNow() ? true : false) || otherBad)
	result = false;
```

The nested equivalent:

```cpp
if (mode != BAD) {
	if (helperBad)
		result = false;
} else {
	result = false;
}
```

can make MWCC emit a duplicate assignment block for the outer `else` even when
the inner predicates already branch to a shared block. The same shape appears
for a `skip = true` guard.

**Observed.** `mario/Camera/CameraBGCheck` (t407): rewriting
`isNeedRoofCheck_()` as one OR guard moved `95.7% -> 100%`; rewriting the
analogous guard in `isNeedGroundCheck_()` moved `94.6% -> 96.9%`; rewriting
`calcInHouseNo_()`'s suppression predicate as one OR guard moved
`88.8% -> 90.2%`.

**Experiment to confirm/refute.** Find an independent TU where target has one
shared `li result, K` block for multiple guard predicates but source emits a
duplicate outer-else assignment. Rewrite only the guard as one short-circuit
condition with a single assignment body and verify whether the duplicate block
disappears without changing helper-call order.

### Fixed small vector-angle reductions may need scalar locals, not stack `TVec3` temps, to keep point deltas in saved FPRs

**Hypothesis.** When target asm computes a fixed number of vector
cross/dot/`atan2f` reductions as straight-line math with many saved FPRs,
source stack `TVec3` temporaries can force the wrong store/load schedule even
if the operations are equivalent. Writing explicit scalar deltas
(`p1x = point1.x - hit.x`, etc.) and scalar cross/dot products lets MWCC keep
the deltas in FPRs across `sqrt`/`atan2f` calls and can recover the target
saved-FPR shape.

**Observed.** `mario/Map/MapCheck` `bgIntersectLine()` (t401) moved
`51.1% -> 84.8%` when three point-minus-hit `TVec3` temps were replaced with
scalar deltas and scalar cross/dot math after the loop-unroll and
`TVec3::add` call-boundary fixes.

**Experiment to confirm/refute.** Find another fixed small vector-angle or
cross/dot reduction currently written with stack vector temps; rewrite only the
vector temps into named scalar deltas, and check whether target saved-FPR
allocation and call-adjacent product order improve.

### Getter-fed float multiplies may need a split assignment to preserve operand order while using the accessor as a frame lever

**Hypothesis.** Inline `TParamRT<T>::get()` accessors can be used as a stack
frame lever, but for `f32` values immediately multiplied by another live
`f32`, spelling the expression as `param.get() * strength` can make MWCC choose
the opposite `fmuls` operand order from target. Splitting the load and multiply:

```cpp
f32 phase = param.get();
phase *= strength;
```

can preserve the target `lfs phaseReg, value; fmuls phaseReg, phaseReg,
strengthReg` shape while keeping the accessor-boundary frame effect. Accessor
count can also have threshold behavior: one extra accessor may not move the
frame, while a second does.

**Observed.** `mario/Camera/camerashake`
`TCameraShake::{startShake,keepShake}` (t381): raw TParam offset reads emitted
the correct visible loads but a `0x38` frame versus target `0x50`. Switching
only the three `s16` velocity reads to `.get()` grew the frame to `0x48`.
Adding two `f32` amplitude getters reached the target `0x50`; the direct
`param.get() * strength` form had `fmuls strength,value` operand drift, while
splitting into `phase = get(); phase *= strength;` made both functions exact.

**Experiment to confirm/refute.** Find another TParam-heavy setup function with
near-exact field loads, a too-small frame, and immediate float multiplies. Test
raw value reads, direct `.get() * factor`, and split `tmp = .get(); tmp *=
factor`. Promote only if the split form again preserves target operand order
while the accessor count controls frame size.

### Fixed-count countdown cursor loops can select a compact `mtctr/bdnz` loop where forward loops unroll

**Hypothesis.** For a fixed trip count with cursor increments and no need to use
the induction value, a forward loop such as `for (int i = 0; i < N; ++i)` can
trigger MWCC's unroller, and a forward `do/while` can preserve the loop but use
an explicit `cmpwi/blt` controller. Writing the loop as a countdown over the
same cursor body:

```cpp
for (int i = N; i > 0; --i) {
	*p = ...;
	++p;
}
```

can make MWCC lower it to the target compact `li N; mtctr; body; bdnz` form
without hand-unrolling or introducing an explicit compare.

**Observed.** `mario/Camera/sunmodel` `TSunModel::TSunModel(bool, const char*)`
(t379): the 17-entry sample-reset loop as a forward pointer `for` selected an
8-way unroll plus remainder and left the constructor at `60.5%`. A forward
`do/while` stopped the unroll but emitted `cmpwi/blt`, raising it to `96.6%`.
Changing only the controller to `for (s32 i = 17; i > 0; --i)` produced the
target `mtctr/bdnz` loop and made the constructor exact.

**Experiment to confirm/refute.** Find a second fixed-count cursor loop where
target has a single `mtctr/bdnz` body and the current source either unrolls or
uses `cmpwi/blt`. Test forward `for`, forward `do/while`, and countdown
`for`; promote only if countdown again selects `mtctr/bdnz` without changing the
loop body.

### Tiny fixed nested loops may be inner-unrolled while preserving dead-looking constant-condition branches

**Hypothesis.** For tiny fixed nested loops such as a 3x3 stencil, MWCC may keep
the outer loop as a counted `bdnz` loop, fully unroll the inner loop, and still
emit constant-condition scaffolding from the original inner-loop predicate. A
hand-expanded source that samples the same fixed positions can remove that
scaffolding, hoist different address pieces, and increase register pressure.

**Observed.** `mario/Map/PollutionPos`
`TPollutionPos::getEdgeDegree(int, int) const` (t367): rewriting the hand
expanded left/center/right row samples back into
`for yOffset=-1..1; for xOffset=-1..1; if (xOffset != 0 || yOffset != 0)` made
MWCC reproduce the target's single outer `mtctr 3` loop, the unrolled three
inner samples, and the dead-looking `li r6,0; cmpwi r6,0` center-skip test. The
function moved `49.4% -> 95.7%`; changing the sentinel compare to `0xffU` then
matched the target unsigned `cmplwi` and moved it to `97.8%`.

**Experiment to confirm/refute.** Find a second fixed small stencil or
neighborhood loop where target has one counted loop and unrolled inner bodies
with constant-condition remnants. Compare structured nested-loop source against
hand-expanded samples; promote only if the structured form again restores both
the unroll shape and the preserved constant-condition branch.

### Countdown cursor loops may select MWCC's 4-way unroller where index loops select an 8-way unroller

**Hypothesis.** For loops over fixed-size records that only need to visit every
entry, an index form such as `for (int i = 0; i < count; ++i) data[i]...` can
make MWCC emit an 8-way unrolled preheader plus a remainder loop, while a
countdown cursor form

```cpp
T* p = base;
for (int i = count; i > 0; --i, ++p) {
	...
}
```

can make the compiler copy the count into the loop controller and choose the
target 4-way unroll (`srwi. count, 2`; four record visits per `bdnz`;
`andi. count, 3` remainder). This is not just register coloring: it changes the
unroll factor and removes the 8-way `count - 8` preheader family.

**Observed.** `mario/Camera/CameraMultiPlayer`
`CPolarSubCamera::ctrlMultiPlayerCamera_` (t363): changing the first player
position averaging loop from index/table access to a countdown cursor restored
the target's 4-way unrolled sum loop and moved the function `58.5% -> 88.5%`.

**Experiment to confirm/refute.** Find a second record-walk loop where target
has 4-way unroll plus `andi. count, 3`, while our indexed source emits an
8-way `count - 8` preheader. Compare indexed, forward cursor with separate
index, and countdown cursor forms; promote only if countdown cursor selects the
4-way unroller again without hand-unrolling the source.

### Assigning a loop index to the bound inside a match arm can reproduce target's bound-plus-one sentinel loop where `break` emits a different controller

**Hypothesis.** When target asm for a search loop does not branch out on match,
but instead assigns the induction variable to the loop bound (`mr idx, bound`)
and then still executes the loop increment, the original source likely avoided
`break` and wrote an in-loop sentinel assignment:

```cpp
for (idx = 0; idx < bound; ++idx) {
	if (match)
		idx = bound;
}
if (idx == bound) {
	// no match
}
```

This exits with `idx == bound` when no match is found and `idx == bound + 1`
when a match was found. A natural `break` can make MWCC choose a different loop
controller (`mtctr`/`bdnz` in the observed case) and changes the post-loop
equality test shape.

**Observed.** `mario/JSystem/JAudio/JAInterface/JAIGFrameSequence`
`JAIBasic::checkEntriedSeq` (t351): changing the auto-heap owner scan match arm
from `break` to `track = JAIGlobalParameter::seqPlayTrackMax` restored the
target index/multiply loop with `mr r9, r8`, removed the ctr loop, and moved the
function `91.9% -> 94.0%`.

**Experiment to confirm/refute.** Find a second search loop where target shows
`mr idx, bound` followed by the normal increment and a final `idx == bound`
test. Compare `break`, sentinel assignment, and an explicit boolean-found local.

### Cached `u8` loop bounds can trigger small-loop unrolling; reloading the bound expression may preserve a target top-tested loop

**Hypothesis.** For small byte-count loops over local tables, storing the loop
bound in a `u8` local can give MWCC enough range information to emit an unrolled
preheader plus a cleanup loop. If target asm keeps a simple top-tested loop that
reloads the bound expression each iteration, write the condition against the
source expression instead of a cached local. When the bound expression includes
an index scale such as `category * 2`, the helper parameter type also matters:
an `int` category can let MWCC strength-reduce the scaled offset across the
outer loop, while a `u8` helper argument tends to recompute the narrowed scale.
Keep the helper shape scoped because changing a shared helper signature can
regress other callers.

**Observed.** `mario/JSystem/JAudio/JAInterface/JAIGFrameSe`
`JAIBasic::checkNextFrameSe` (t349): replacing a cached
`u8 categoryLimit` with direct `getSeCategoryLimitInt(unk0, unk10, i)` loop
conditions changed the candidate initializer from an 8-way unrolled preheader
to the target plain loop and moved the function `77.7% -> 90.1%`. Changing the
shared `getSeCategoryLimit` helper to take `int` improved this function but
regressed `releaseSeRegist` `93.5% -> 91.4%`, so the old `u8` helper stayed in
place for existing callers.

**Experiment to confirm/refute.** Find another JAudio category/table loop where
target reloads a byte table limit inside a small initializer but source caches
it and MWCC unrolls. Compare cached-local, direct-expression, and scoped
`int`-helper forms; promote only if a second TU shows the same unroll/offset
behavior without caller regressions.

### For quadratic interpolation over integer control points, direct `start + mid + end` terms can preserve the target conversion schedule better than factored locals

**Hypothesis.** In Bezier-style interpolation expressions over `int` point
fields, MWCC's register and int-to-float conversion schedule is sensitive to
the written term tree. A direct expression in the source order

```cpp
start * ((1.0f - t) * (1.0f - t))
    + mid * (2.0f * (1.0f - t) * t)
    + end * (t * t)
```

can reproduce the target's load/convert/FMADD order, while precomputing
`t2`, `inv`, `blend`, and `inv2` as locals or writing the terms as
`mid + start + end` changes FPR allocation and scheduling. If the rounded
result is sign-extended before being stored to a 32-bit field, split the int
conversion and the short cast (`s32 x = value; field = (s16)x;`) so MWCC emits
`fctiwz` before the second coordinate's rounding branch and `extsh` at the
later store.

**Observed.** `mario/GC2D/BoundPane` `TBoundPane::update()` (t343): rewriting
both position and size interpolation blocks from `end + start + mid` with a
direct float-to-int store to direct `start + mid + end` terms plus the delayed
`(s16)` store moved the function from `55.0% -> 99.3%`. Factored locals
preserved behavior but regressed to `98.9%`; explicit integer component locals
inflated the frame to `0x78`.

**Experiment to confirm/refute.** Test another TU with integer point/control
interpolation, for example the GC2D shine/select spline code. If target uses
the same conversion and FMADD schedule, compare direct `start + mid + end`
terms against factored locals and alternate term orders.

### Direct packed integer expressions preserve shift/OR BP-word construction where field-setting macros lower to `rlwimi`

**Hypothesis.** When target asm builds a packed hardware register word as a
straight sequence of `slwi`/`or` instructions, source should spell the word as a
single packed expression:

```cpp
u32 word = (hi << 24) | (mid << 12) | lo;
```

Using a field-setting macro that masks and ORs each field into an accumulator can
make MWCC recognise insert-field idioms and emit `rlwimi`/`clrlwi` sequences
instead, even when the final value is equivalent. For signed half-index values,
write `/ 2` rather than `>> 1` when target shows `srawi` plus `addze`.

**Observed.** `mario/MarioUtil/PacketUtil`
`FifoSetFogRangeAdj(unsigned char, unsigned short, GXFogAdjTable*)` (t341):
rewriting both range-table and range-center BP words from `PACKET_SET_REG_FIELD`
calls to direct packed expressions changed the helper from `34.9% -> 100.0%`
and added 312 matched code bytes.

Same TU follow-up: `FifoSetFog(_GXFogType, float, float, float, float,
GXColor)` moved from `40.8% -> 95.4%` after using direct/staged packed
expressions for the BP words, writing each FIFO word immediately in target
order, starting the fog exponent at 1 instead of adding 1 later, and assigning
the `c` float in each branch before the normalization loops. This reinforces
the packing/order lever but is not an independent TU confirmation.

**Experiment to confirm/refute.** Find a second FIFO/BP helper where target has
manual `slwi`/`or` packing but current source uses a field-setting macro and
emits `rlwimi`; rewrite only the packed word expression. If the shift/OR pattern
returns without scheduling regressions, promote as a narrow integer-packing rule.

### In a non-polymorphic-base derived class, MWCC places the derived vptr at the first virtual declaration point

**Hypothesis.** When a class derives from a non-polymorphic base and introduces
its own virtual function, MWCC does not always place the implicit vptr at the
start of the derived tail independent of source order. Instead, the vptr appears
at the point where the first virtual member is declared relative to data members.
If data members are declared before the first virtual method, those fields occupy
the first bytes after the base subobject and the vptr is emitted after them; if
the virtual method is declared first, the vptr occupies the first derived-tail
word and shifts the fields later.

**Observed.** `mario/GC2D/BlendPane` (t340): moving
`TBlendPane::mStep/mCurrent/mActive` before `virtual update()` in
`include/GC2D/BlendPane.hpp` shifted the fields to target offsets
`0x68/0x6c/0x70`, shifted the vptr to `0x74`, made the constructor exact, and
added 80 matched code bytes.

**Experiment to confirm/refute.** Find a second derived class whose target
constructor stores the vptr after newly introduced data members while our build
stores it before them. Reorder only the class declaration so the data members
precede the first virtual declaration; if the vptr and field offsets move to
target without other changes, promote this as a settled MWCC class-layout rule.

### Put a float interval predicate in the `while` condition when target uses body-before-condition branch-back layout

**Hypothesis.** For loops that repeatedly call a body while a float interval
predicate remains true, MWCC's source shape matters:

```cpp
while (i < count && table[i].frame <= current + step) {
	body();
}
```

lowers to the canonical loop layout with an initial branch to the condition,
the body placed before the condition block, and a condition-time `cror` branch
back to the body. The equivalent inverse-break body:

```cpp
while (i < count) {
	if (table[i].frame > current + step)
		break;
	body();
}
```

keeps the body after the comparison and emits the wrong branch target/order,
even if rewriting the `if` as positive `<=` recovers the `cror` lattice.

**Observed.** `JSystem/JAudio/JAInterface/JAIAnimation`
`JAIAnimeSound::setAnimSoundActor` (t339): converting all four crossed-record
playback loops from inverse-break bodies to while-header predicates, plus
using a local cached table pointer, moved the function `71.7% -> 95.3%`.

**Experiment to confirm/refute.** Find a second TU with a nonmatching loop whose
target has `b condition; body; condition: fcmpo; cror; b<eq/ne> body` and whose
source currently uses `if (inverse) break; body();`. Move only the predicate
into the `while` header and check whether the body-before-condition layout and
branch-back target appear without other structural changes.

### Explicit template specializations can preserve same-TU template-member `bl` calls where generic template definitions inline despite `dont_inline`

**Hypothesis.** For a class template member used by non-template functions in
the same TU, a generic template definition plus explicit class instantiations
may still be inlined into the callers under `-inline deferred`, even if the
generic template definitions are below the callers and wrapped in
`#pragma dont_inline`. If the target keeps `bl` calls to the emitted template
member specializations, forward-declare the explicit specializations before the
callers, define the explicit specializations after the callers, and wrap those
specialization definitions in `#pragma dont_inline on/off`.

**Citation (1 TU).** `mario/Player/MarioRecord` (t338):
`TMarioInputReplay::{reset,play,init}` needed calls to
`TRecordValueManager<T>::reset/get`. Source order alone and `dont_inline`
around generic template bodies did not stop same-TU inlining. Explicit
specialization declarations plus `dont_inline` specialization definitions made
`reset` and `play` exact and moved the unit `52.58% -> 99.973%`; `init` kept
only a +8 target frame-size residue.

**Experiment to confirm/refute.** Find a second TU where a generic template
member body is emitted exactly but same-TU callers inline it despite target
`bl`s. Convert only that helper to predeclared explicit specializations under
`dont_inline`; if the call boundaries return without changing helper bodies or
symbol ownership, promote this as a narrow template call-boundary lever.

### Returning `bool` from a `BOOL` local through a ternary can force a full-width `cmpwi` rematerialization

**Hypothesis.** In a `bool`-returning function, a `bool result` local followed by
`return result ? true : false;` makes MWCC test the local as an 8-bit bool
(`clrlwi.`). If target instead tests the local with full-width `cmpwi`, use a
full-width `BOOL result` local while keeping the explicit bool ternary return:

```cpp
BOOL result = TRUE;
...
return result ? true : false;
```

Directly returning the `BOOL` local is a different shape: MWCC can lower the
conversion through `neg/subfe` instead of the branchy `cmpwi; beq; li; b; li`
materialization. Assigning `result = result ? TRUE : FALSE; return result;` can
also choose the `neg` path. The ternary must remain at the return expression.

**Citation (1 TU).** `mario/Camera/CameraBck`
`TCameraBck::updateDemo` (t335): after vector-copy and offset-cache fixes, a
`bool result` plus ternary return left the final test as `clrlwi.` and the
function at `97.4%`. Changing only the local to `BOOL result`/`TRUE`/`FALSE`
kept the ternary return, restored the target `cmpwi r31, 0` final test, and
moved the function to `97.9%`. Direct `return result;` and assignment
normalization probes both regressed to `neg/subfe`.

**Experiment to confirm/refute.** Search for bool-returning near-matches where
the target has a full-width `cmpwi` on a 0/1 local before final return while
ours has `clrlwi.` or `neg/subfe`. Flip only the local type to `BOOL` and keep
the return expression as `return result ? true : false;`. A second independent
hit can promote this as a narrow return-materialization rule.

### A typed `this` local before a virtual call can make MWCC load the vtable before setting `r3`

**Hypothesis.** For a virtual call on `this`, spelling the call through an
explicit same-type pointer local can shift MWCC's setup order from
`mr r3,this; lwz r12,0(this)` to `lwz r12,0(this); mr r3,this` without changing
the call target or adding instructions:

```cpp
TShineFader* fader = this;
fader->draw(rect);
```

This is source-visible and not just register coloring: the natural
`draw(rect)` form already has the same live values and emits the opposite order.

**Citation (1 TU).** `mario/GC2D/ShineFader`
`TShineFader::perform` (t333): the target's second virtual call loads the vtable
pointer before moving `this` into `r3`. Natural `draw(graphics->mViewportRect)`
left the function at `93.5%` with only `mr r3` / `lwz r12` swapped. Routing the
call through `TShineFader* fader = this; fader->draw(...)` matched the
instruction stream exactly (`100.0%`).

**Experiment to confirm/refute.** Search near-match virtual-call residues where
the only mismatch is `mr r3,this` scheduled before a vtable load. Apply the
same typed-local call spelling. If a second independent TU responds, promote
this as a narrow virtual-call scheduling lever; if it only works after a prior
virtual call on the same object, refine the precondition.

### Out-of-class template member definitions can remove in-class inline bias while preserving weak template emission

**Hypothesis.** For class template virtuals whose bodies are visible in a
header, defining the member inside the class body can make MWCC treat the body
as strongly inline-preferred at derived call sites. Moving the same template
member body out of the class body, while keeping it in the header, can still
provide the visible template definition/weak instance but lower the inline bias
enough for `-inline deferred` to keep a `bl` to the template member.

**Citation (1 TU).** `mario/JSystem/JDrama/JDRSmJ3DScn`
`JDrama::TSmJ3DScn::perform` (t331): the target calls
`TViewObjPtrListT<TViewObj, TViewObj>::perform` twice. With the base template
body defined in-class, MWCC recursively inlined the list walk and left
`perform` at `0.0%`. Moving only `TViewObjPtrListT::perform` and `loadSuper`
to out-of-class template definitions restored the two `bl` call sites without
regressing the exact ctor/dtor; a report compare showed only this unit/function
changed. A local `MtxPtr viewMtx` then fixed the `PSMTXCopy` argument order,
leaving `TSmJ3DScn::perform` at `99.9%` with only a +8B frame-size residue.

**Experiment to confirm/refute.** Find a second class-template virtual where a
derived function should call an emitted template instance but an in-class
template body inlines into the derived method. Move only that body out of class
and verify whether the call boundary appears without collateral constructor or
weak-owner regressions. If the pattern repeats, promote this as the less blunt
alternative to `#pragma dont_inline` for header template virtuals.

### Some SDK call sites may have a wider local prototype than the callee definition when the callee masks its arguments

**Hypothesis.** When a target caller passes an integer argument directly to an
SDK function whose callee body immediately masks that argument down to a smaller
width, the source TU may have seen a wider local prototype than the SDK callee
definition. A global signature change is wrong if it removes the callee-side
masking; the useful lever is a TU-local declaration that changes only caller
conversion.

**Citation (1 TU).** `mario/Player/MarioMain`
`TMario::drawSyncCallback` (t327): the target caller stores the `fctiwz` result
and passes it directly to `GXPeekARGB`, with no caller-side `clrlwi` narrowing.
The SDK `GXPeekARGB` body still needs `u16` parameters to emit its own
`clrlslwi` masks and keep `mario/dolphin/gx/GXMisc::GXPeekARGB` at 100%. A
global `GXPeekARGB(u32,u32,...)` probe matched the MarioMain caller but regressed
the SDK callee and failed the linked DOL checksum. A `MarioMain.cpp`-local
wide declaration, with the header's narrow declaration hidden in that TU, kept
the callee/link intact and matched the caller.

**Experiment to confirm/refute.** Search other `GXPoke*` / `GXPeek*` call sites
where target omits caller-side narrowing but the SDK callee masks its args. If a
second independent TU needs the same "wide caller, narrow callee" split, promote
this as a header/prototype matching rule. If no second site appears, keep it as
a MarioMain-specific prototype accident.

### A higher-level inline wrapper can force an out-of-line weak call that a direct inline method call would expand

**Hypothesis.** For template matrix helpers, calling a higher-level wrapper can
change MWCC's deferred-inline decision even when the wrapper itself is inline.
In `mario/System/TalkCursor::associateNPC` (t317), direct
`mtx.identity33(); mtx.setTrans(pos);` inlined the 3x3 identity stores, but
`mtx.translation(pos.x, pos.y, pos.z)` emitted a `bl` to
`TRotation3<TMatrix34<SMatrix34C<f>>>::identity33()` and moved the function
`41.5 -> 82.5`. The remaining residue is not solved: the call currently emits a
local weak `identity33` body in `TalkCursor.o`, while the target calls the weak
owner in `mario/MarioUtil/DrawUtil`, and scalar arguments are homed before the
call whereas the target loads `pos` after the identity call.

**Experiment to confirm/refute.** Find a second site where target calls
`identity33` but direct `identity33(); setTrans(...)` inlines. Test whether
`translation(...)` reliably restores the call boundary, then solve weak-owner
placement separately before promoting this pattern.

### Direct `this->memberPtr` access may force post-call reloads where a cached local pins the pointer across the call

**Hypothesis.** The settled "do not cache a global pointer across calls" rule
also appears to apply to pointer members loaded from `this`. A source local like
`TMario* mario = unk68;` encourages MWCC to keep the pointer live in a
callee-saved register across opaque calls, while repeated direct `unk68` member
accesses let the compiler reload the member after a call. In
`mario/Player/MarioEffect::setJumpIntoWaterEffect` (t317), dropping the cached
`TMario*` local made MWCC reload `unk68` after `PSMTXCopy`, matching the target.
The same rewrite split raw speed from absolute speed (`speed` + `absSpeed`),
which matched target `fmr f31,f1` followed by `fneg f31,f1` instead of negating
from the already-copied FPR. Together this moved the helper `89.8 -> 97.1`.

**Experiment to confirm/refute.** Find another function where target reloads a
`this` pointer member after an intervening call but our build keeps a cached
local pointer live. Remove only the cached local and use direct member accesses;
if the reload pattern appears without broad regressions, promote this as the
member-pointer companion to the global-pointer caching rule.

### Inline inherited virtuals referenced only by a derived vtable may need an external weak owner, not per-TU header emission

**Hypothesis.** When a derived class emits a vtable that reuses inherited virtual
methods whose bodies are inline in the header, MWCC may either emit local weak
copies in that TU or reference a weak copy owned by another TU depending on how
the original header/source split was written. In `M3DUtil/SDLModel` (t310),
`SDLMatPacket` inherits `J3DMatPacket::isSame` and `J3DMatPacket::entry`; our
header-inline bodies make `SDLModel.o` emit extra weak `isSame__12J3DMatPacket...`
and `entry__12J3DMatPacket...`, but the target `SDLModel.o` has only the
`SDLMatPacket` vtable and no local copies. The same methods already have weak
symbols elsewhere (`J3DPacket`/J3DGraphBase ownership).

**Experiment.** Try a narrow branch/worktree probe that moves only
`J3DMatPacket::isSame`/`entry` bodies out of `J3DPacket.hpp` into the known weak
owner TU, then full-report the project. Confirmation: `SDLModel.o` loses those
extras while existing `J3DPacket` weak symbols still match and no broad TUs
regress. Refutation: the move breaks mixed inline callers like the historical
`TUtil<f32>::sqrt` sweep, or the target expects local weak copies in other vtable
owners.

**Partial result (t311).** Moving `J3DMatPacket::isSame` into
`J3DGraphBase/J3DPacket.cpp` and `J3DMatPacket::entry` into
`J3DGraphAnimator/J3DJoint.cpp` did remove the SDLModel extras and the stray
`J3DDrawBuffer.o` `isSame` copy; `J3DPacket.o` then matched `isSame`. The probe
could not be kept because the linked `J3DJoint.o` weak `entry` body stayed at
99.8%: target stores MWCC's pointer-to-member-function temp at `r1+0x14`, while
the natural local-variable source emits `r1+0x10`. Inline-expression,
declaration-then-assignment, explicit sort-type local, and `const sortFunc`
variants either kept `r1+0x10` or changed the call shape. Remaining experiment:
find the original non-padding source structure that shifts the PTMF temp by four
bytes, then retry the weak-owner move.

### `(f32)(a - b)` (int subtract then convert) vs `(f32)a - (f32)b` (two converts then float subtract) are distinct, source-controlled codegen — and converting an unsigned/`u8` operand directly skips the signed-fixup `xoris`

**Symptom.** `Player/SplashManager::makeDL` computes a fade ratio from two
`u8` life counters: `(initLife - life) / initLife`. Two source spellings:

```cpp
// (A) int subtract first, then one int->float convert:
f32 ratio = (f32)(mInitLife - splash->mLife) / (f32)mInitLife;
// codegen: subf r0,life,init; xoris r3,r0,0x8000; stw r3,buf+4; lfd; fsubs (signed magic)

// (B) convert each operand to float, then float-subtract:
f32 ratio = ((f32)mInitLife - (f32)splash->mLife) / (f32)mInitLife;
// codegen: two unsigned int->double converts (stw r31=0x43300000 hi; stw <val> lo; lfd; fsubs),
//          NO xoris, then fsubs of the two floats.
```

Switching from (A) to (B) took makeDL **72.5% → 94.6%** — the target uses form
(B). The key tells in the asm: (A) does ONE `subf` + ONE `xoris r,r,0x8000`
(the signed int->float sign-bit flip) before a single conversion; (B) does TWO
conversions with **no `xoris`** and the subtraction happens in float (`fsubs`)
*after* both converts.

**Two sub-observations:**
1. **Subtract-then-convert vs convert-then-subtract is source-visible** and
   not reordered by MWCC. Read the asm: a single `subf`/`sub` feeding one
   conversion → write `(f32)(a-b)`; two independent conversions feeding an
   `fsubs` → write `(f32)a - (f32)b`.
2. **No `xoris` ⇒ unsigned conversion.** When the operand being converted is a
   `u8`/`u16`/unsigned value used *directly* (form B, each `(f32)u8field`),
   MWCC emits the unsigned int->double path (magic `0x4330000000000000`, hi
   word `0x43300000`, raw value in the low word, no sign flip). When the value
   is a *signed int* expression result (form A, `(int)(a-b)` can be negative),
   it emits the signed path with `xoris r,r,0x8000` and the
   `0x4330000080000000` magic. So the cast target type (and signedness of the
   thing being cast) controls the presence of `xoris`.

**Hypothesis (toward Settled).** This is deterministic source-shape codegen,
not a coloring artifact: the position of the cast relative to the arithmetic
operator decides convert-then-op vs op-then-convert, and the signedness of the
converted expression decides signed (`xoris`) vs unsigned conversion. Needs one
more independent TU computing a float ratio/scale from integer counters to
promote. Look for any `(f32)(intExpr)` near a `divw`/`subf` + `xoris`
combination in the asm where the target instead shows paired conversions.

**Citations:** `mario/Player/SplashManager::makeDL` (72.5→94.6, commit on
2026-05-31).

### A counted loop indexing `arr[(u16)i]` where the index is masked to 16 bits each use: target recomputes `((u16)i)<<scale` per-iteration (`clrlslwi`) instead of carrying a strength-reduced byte-offset induction variable

**Symptom.** `Camera/lensglow::__ct__` material-anm loop:
```cpp
int num = unk10->getMaterialNum();          // u16 count hoisted to int
for (u16 i = 0; i < num; i++) {
    J3DMaterialAnm* anm = new J3DMaterialAnm();
    unk10->getMaterialNodePointer(i)->change();      // mMaterials[(u16)i]
    unk10->getMaterialNodePointer(i)->unk38 = anm;   // mMaterials[(u16)i] again
}
```
`getMaterialNodePointer(u16 idx)` returns `mMaterials[idx]`, so the index is
masked to 16 bits (`idx` is `u16`). **Target** carries a single induction var
`i` in a callee-saved reg and recomputes the byte offset each iteration with
`clrlslwi rT, rI, 16, 2` (= `((u16)i) << 2`), reusing rT for both array
accesses within the iteration. **Our build** strength-reduces: it adds a
second induction variable holding the byte offset directly (`addi rB, rB, 4`,
no mask) alongside `i` (kept only for the `cmpw` bound check). Net: our loop is
+2 instructions and the frame is +8 bytes larger (target 0x160 vs ours 0x158).
Everything else in the ctor matched (65% → 97.1% after member-init order,
infectious strings, count hoist, and `int` loop bound).

**Hypothesis.** When the array index is *masked* (`(u16)i`) at each use, the
byte offset `((u16)i)*stride` is NOT a clean linear induction (it wraps at
2^16), so MWCC declines to introduce a strength-reduced offset variable and
recomputes it from `i` each iteration. Our build strength-reduced anyway —
suggesting some source-level difference makes MWCC treat the offset as a clean
induction (e.g. the index reaching the subscript without a u16 truncation, or a
different inline shape for `getMaterialNodePointer`).

**Experiment to run next.** (a) Try `for (int i = 0; …)` with an explicit
`(u16)i` cast only inside the subscript vs. (b) a `getMaterialNodePointer`
variant taking `int`. Watch whether the `clrlslwi`-recompute form appears.
Also diff the +8 stack: confirm it's purely the extra induction var's spill
slot vs. an inlined temp. If a lever is found, promote; this is a real
byte-count (base_size) difference, not just coloring.

### A local initialized BEFORE a preceding `bl` but first USED after it gets promoted to a callee-saved (non-volatile) FPR/GPR — and that NV-reg's save/restore inflates the frame; move the init AFTER the call to keep it volatile

**Symptom.** In `Enemy/BathtubKiller::resetBathtubKiller` the launch-offset
default `f32 off = 0.0f;` was declared *before* `int sel = (int)(MsRandF()*4);`.
Because `off`'s value (0.0f) is live across the `bl rand` inside `MsRandF`,
MWCC pinned it to **f31** (callee-saved), emitting `stfd f31, …` /
`lfd f31, …` in the prologue/epilogue AND bumping the frame. The target kept
`off` in **f4** (volatile) and loaded 0.0f only *after* rand, interleaved into
the float→int conversion.

**Lever.** Declare/initialize the local as late as possible — specifically
*after* the call whose result it doesn't depend on:

```cpp
// inflates frame, off → f31 (live across bl rand):
f32 off = 0.0f;
int sel = (int)(MsRandF() * 4.0f);
...

// off → f4 (volatile), no save/restore, frame collapses:
int sel = (int)(MsRandF() * 4.0f);
f32 off = 0.0f;
...
```

In resetBathtubKiller this single reorder took 97.1% → 99.9% AND eliminated
the f31 save pair (the only remaining diff is a pure phantom-inline frame pad).

**Why it's a hypothesis not Settled.** Observed cleanly once. It is the
*inverse* of the Settled entry "Hoist a struct-field read into a local INSIDE a
loop, BEFORE a function call, to lock it into a NV-FPR across the call" — that
entry deliberately uses the *same* mechanism to FORCE NV-FPR allocation. So the
mechanism is established; the open part is whether "move init after the call"
is reliably the right lever whenever the target uses a volatile reg + small
frame for a constant-init local. **Experiment:** find a near-match where our
build spills a constant/default local to f30/f31 (with save/restore + frame
inflation) but the target uses f1–f13; try sinking the local's initialization
past the nearest preceding `bl` and confirm the NV-reg pair disappears.

### A mid-function `return FALSE` identical to the tail `return FALSE`, with intervening calls, co-occurs with both an un-merged `li r3,0` AND a small frame UNDER-allocation (igaiga graph-tail nerves)

**Symptom.** In `igaiga`'s graph-walking nerve `execute` bodies the shape is
```cpp
if (self->isReachedToGoalXZ()) {
    if (self->jumpToNextGraphNode() >= 0) self->flagJump();
    if (self->getTracer()->getCurrent().checkFlag(0x40)) return FALSE; // mid
    self->goToRandomNextGraphNode();
}
self->walkBehavior(2, 1.0f);
return FALSE;                                                          // tail
```
The mid and tail returns are the **same constant** (`FALSE`). The target
branches the mid-return's condition **directly to the single tail `li r3,0`**
(`bne <epilogue-li>`), reusing one `li`. Our build instead materialises the
mid-return inline as `li r3,0; b <epilogue>` (a second `li r3,0`) and inverts
the branch polarity (`beq <body>`). Cross-jump/tail-merge that the target does,
ours doesn't.

**New correlation (t283).** This un-merged mid-return co-occurs with a
frame UNDER-allocation: `IgaigaRollOnGraph::execute` target frame 0x58 / ours
0x40 (−0x18); `WaterHit` 0x70 / 0x58 (−0x18). Crucially `RollOnGraph` inlines
**zero** `theNerve()` Meyers singletons, so its −0x18 is NOT the documented
nerve-base-hoist frame inflation — it tracks the mid-`return FALSE` structure
+ the `getTracer()->getCurrent().checkFlag` inline instead. The control
twin `GorogoroRollOnGraph::execute` (no mid-return, `goToShortest` instead of
`goToRandom`) matches **100%** with the correct frame. So on these two nerves
the block-ordering open question and the frame-under-allocation open question
are very likely the **same** root cause, not two.

**Experiment to try next.** Reduce to a minimal BOOL fn: `if(A){ if(B) f();
if(C) return FALSE; g(); } h(); return FALSE;` and vary (a) whether the mid
and tail return the same constant, (b) whether a call sits between the mid
return and the tail. Watch whether the `li r3,0` merges and the frame grows
together. If they move in lockstep, promote as one rule. Banned: goto. Cited:
`Enemy/igaiga` `execute__23TNerveIgaigaRollOnGraph` (94.24), `WaterHit`
(98.61), control `GorogoroRollOnGraph` (100). See `state/notes/igaiga.md`.

### `getMActor()->getModel()` inlines the model access; bare `getModel()` emits an out-of-line `bl` (per-site, diff-driven)

**Confirmed mechanism (igaiga, t279).** `TLiveActor::getModel()` is an
**out-of-line const** member (`src/Strategic/liveactor.cpp:130`, body
`return mMActor->unk4;`). When the original source wrote the model access as
`getMActor()->getModel()` (both inline: `getMActor()` returns `0x74`,
`MActor::getModel()` returns `unk4`), MWCC inlines it to
`lwz rX,0x74(rThis); lwz rX,4(rX)` — no `bl`. When the source wrote bare
`getModel()`, it emits `bl getModel__10TLiveActorCFv`. `getAnmMtx(N)` then
adds `N*0x30` stride after the `lwz 0x58` (mNodeMatrices) load — index 0 adds
nothing, index 1 emits `addi rX,rX,0x30`. So matching a particle-emit-on-joint
call requires getting BOTH right: the inline form AND the joint index.

- `TGorogoro::calcRootMatrix`: `getModel()->getAnmMtx(1)` →
  `getMActor()->getModel()->getAnmMtx(0)` (target had no 0x30 stride): 95.9% → 99.3%.
- `TGorogoro::setDeadAnm`: `getModel()->getAnmMtx(1)` → `getMActor()->getModel()->getAnmMtx(1)`
  (target keeps the 0x30 stride): 97.9% → 99.9%.
- `TGorogoro::behaveToWater` same emit fix applied (TU otherwise deep IMPL gap).

The asymmetry is real: within `calcRootMatrix`, the emit call inlines getModel
but the later `getModel()->getBaseTRMtx()` matches as an out-of-line `bl` on
**both** sides — i.e. the original wrote `getMActor()->getModel()` for the emit
and `getModel()` for the base matrix. So this is **per-site, not per-function**;
drive it by the diff, never blanket-replace.

**Negative sweep (t279).** A read-only detector (`tools/agent/scan_getmodel.sh`)
scanned 20 enemy TUs for the signature (our RIGHT-only `bl getModel__10TLiveActorCFv`
where target inlines). After filtering false positives — heavily-misaligned
functions show both `>` and a LEFT-side `bl getModel` on a `|` modified line
(e.g. popo `PopoRollCallback`/`PopoPossessedCallback`, whose real gap is the
bool-return `li 0/1; b` materialize + `init$` ordering) — **no other enemy TU
had a genuine applicable site.** The lever appears igaiga-specific so far.
Needs a 2nd independent TU before promotion to Settled. Detector caveat: a
purely textual `^<`/`^>` grep misses target `bl getModel` on `|` lines; always
eyeball the diff for a LEFT `lwz rX,4(rX)` inline pair before applying.

### Uniform per-TU +0x10 stack inflation on leaf functions (igaiga)

Several small leaf functions in `mario/Enemy/igaiga` whose bodies match the
target instruction-for-instruction still miss by exactly **0x10 bytes of
stack frame** (ours smaller): `TIgaiga::boundSE` (0x28 vs 0x38),
`TIgaigaPolluteModel::setAnm` (0x18 vs 0x28), `TIgaiga::behaveToWater`
(0x40 vs 0x50). No extra stores/temps appear in the target body, so it is
not an inlined-Vec argument temp — it reads as the MWCC 1.2.5 stack-padding
bug (an inlined call or register-resident local reserving unused stack).

Experiment to confirm/refute: find the inlined helper on the shared call
paths (`MActor::getFrameCtrl`, `MSoundSESystem::MSoundSE::startSoundActorWithInfo`)
or an extra named local in the original; reconstruct it (NOT a `_pad` hack)
and check whether all three frames grow by 0x10 together. If one source
construct fixes the whole cluster at once, that confirms a single shared
inline as the cause. See state/notes/igaiga.md (tick 277).

### Product local plus delayed base local can defeat `fnmsubs` while preserving duplicate constant loads

**Hypothesis.** For repeated neighbor impulse updates, source written as
`neighbor->mVelocity.y -= accel * coeff` encourages MWCC to fuse the
multiply and subtract into `fnmsubs`. If target emits `fmuls` followed
by `fsubs`, name the product before the store:

```cpp
self->mVelocity.y -= kAccel;
f32 accel = kAccel;
if (neighbor != nullptr) {
	f32 push = accel * coeff;
	neighbor->mVelocity.y -= push;
}
```

Putting the `f32 accel = kAccel` **after** the self-subtract preserves
target's duplicate load of the constant: one `lfs` for the self update,
then a second `lfs` hoisted before the neighbor null checks. Putting it
before the self-subtract reuses the first load and mismatches the
target's load order.

**Citation (1 TU).** `MoveBG/MapObjMonte::THangingBridgeBoard::control`
(t245): direct compound `-=` emitted `fnmsubs` and kept the function at
81.1%. Naming per-neighbor products fixed the arithmetic shape; moving
the base `accel` local after the self-subtract then matched the duplicate
constant-load shape. Combined with the target matrix pointer and repeated
width field reads, the function rose to 99.84%.

**Experiment to confirm/refute.** Find another map-object or enemy
propagation loop where target shows `lfs const; self -= const; lfs const;
if (neighbor) { fmuls; fsubs; store; }` but source emits `fnmsubs`.
Apply this delayed-base-local/product-local spelling and verify both the
arithmetic opcode and duplicate constant load.

### Degree-angle JMA helpers may need `#pragma dont_inline` to match a standalone helper and its callers

**Hypothesis.** For small matrix helpers that convert degrees to JMA
sin/cos table indices, spelling the body with `JMASin(angle)` /
`JMACos(angle)` can shrink the helper's own frame versus naming an
intermediate `s16 idx`, but the smaller body may cross MWCC's inline
threshold and get pulled into callers. If the target has both a
standalone helper body and caller-side `bl` instructions, wrap only the
helper definition in `#pragma dont_inline on/off`.

**Citation (1 TU).** `MoveBG/MapObjPinna::MsMtxSetRotX` (t237):
`s16 idx = angle * (65536.0f / 360.0f); JMASSin(idx); JMASCos(idx);`
kept callers out-of-line but left the helper at a `-0x28` frame versus
target `-0x20`. Rewriting to `JMASin(angle)` / `JMACos(angle)` made the
helper exact but inlined it into `TShellCup::perform` and regressed that
caller from 98.6% to 53.6%. Adding `#pragma dont_inline` around only
`MsMtxSetRotX` kept the exact helper while restoring the caller's `bl`
shape.

**Experiment to confirm/refute.** Find a second TU-local matrix or
angle helper where the target emits a standalone helper plus caller
`bl`s, while our named short-angle local inflates the helper frame.
Apply the direct degree-helper spelling with and without
`#pragma dont_inline` and check both the helper and at least one caller.

### Taking the address of a `TParamRT<T>::value` field forces `addi field_addr; lfs 0(field_addr)` even for a single load

**Hypothesis.** When target materializes a parameter value's address
before loading it (`lwz params; addi rN, params, VALUE_OFF; lfs fX,
0(rN)`), direct source `params->field.value` collapses the address into
the load immediate (`lfs fX, VALUE_OFF(params)`). Introduce a typed
pointer to the value and dereference that pointer:

```cpp
TBossHanachanChangeSaveParams* params = mChangeParams;
f32* fallDecideRotateZ = &params->mSLFallDecideRotateZ.value;
if (absRot > *fallDecideRotateZ) { ... }
```

This preserves the explicit field-address node long enough for MWCC to
emit the target's `addi + lfs 0` form. It is distinct from the settled
"hoist param->field.member to a local pointer across multiple `bl`
calls" rule: here the pointer is consumed once in the same basic block.

**Citation (1 TU).** `Enemy/BossHanachanMain::checkFallDecideAndSetup`
(t219): direct `.value` kept `lfs f0, 0xb8(r5)` and the function was
93.0%; adding `f32* fallDecideRotateZ = &...value` produced
`addi r5, r5, 0xb8; lfs f0, 0(r5)` and lifted the function to 94.9%.

**Experiment to confirm/refute.** Find a second near-match with target
`addi param_base, value_off` feeding one `lfs`/`lwz`, especially in an
Enemy TParams block. Apply the address-local form and check whether the
address materialization appears without regressing register allocation.

**Negative transfer test (t221).** `Enemy/BossHanachanMain::
goToInitialRecoverGraphNode` has target `addi r6, params, 0x1f8; lfs f3,
0(r6)` while setting up a call to `findNearestVisibleIndex`. Introducing
`f32* recoverSearchDegree = &params->mSLRecoverSearchDegree.value` did
not survive; MWCC collapsed it back to `lfs f3, 0x1f8(params)`. Adding a
named scalar loaded through the pointer moved the load earlier and
regressed the function. The current hypothesis should be narrowed: this
lever is confirmed only when the pointer is consumed in a local
compare/use block like `checkFallDecideAndSetup`, not when it is just a
float call argument.

### `MsRandF(l, r)` 2-arg interval helper: the original took args **by const reference**, so literal `0.0f`/`1.0f` arguments do NOT constant-fold

**Hypothesis.** The 2-arg `MsRandF(f32 l, f32 r)` in
`include/MarioUtil/RandomUtil.hpp` (flagged `// fake!!!`) was originally
declared `MsRandF(const f32& l, const f32& r)`. With by-value params,
a call `MsRandF(0.0f, 1.0f)` lets MWCC fold `(r - l)` → a single `1.0f`
constant and `l` → `0.0f`, and fuse the multiply-add into one `fmadds`.
The target instead loads the TU's shared `0.0f` (`@3243`) and `1.0f`
(`@3518`) sdata2 constants separately, computes `(r - l)` at **runtime
before the `rand()` call** (hoisting it into callee-saved `f31`), and
emits `fmuls; fmuls; fadds` (no fusion). That non-folding, pre-call
`(r-l)` hoist is exactly what passing the literals through `const f32&`
parameters produces.

**Observed.** `mario/Enemy/killer`: `TKiller::reset`'s
`if (MsRandF(0.0f, 1.0f) < 0.05f)` block. By-value 2-arg form → 78.4%.
Flipping the shared header to `const f32&` → **83.6%** (frame and the
entire rand expansion line up). BUT the global flip net-regressed
overall fuzzy (71.10129 → 71.09631) — other TUs' `MsRandF(l,r)`
callsites (namekuri/hamukuri/poihana/smallEnemy/effectObj/bombhei/
mameGesso/telesa, 24 sites) match *better* with by-value, implying either
those originals genuinely used by-value, the helper was overloaded, or
those callsites pass runtime variables (where the signature is codegen-
neutral). Reverted the global change; left killer at the by-value 2-arg
form.

**Experiment to confirm/resolve.** Check what the 24 other callsites
pass: if they pass *variables* (not literals), const-ref is codegen-
neutral there and the regression came from elsewhere — re-measure. If
they pass *literals*, the two behaviors are irreconcilable with one
signature → the smallEnemy "random interval" family likely had its own
distinct helper (the `// fake!!!` TODO hints at a `random interval`
class). A TU-local const-ref helper in killer.cpp (or a class static
holding the 0/1 bounds, forcing a memory load at the callsite) would let
killer reach 83.6% without touching the shared header. Worth a focused
INVESTIGATION pass.

**Experiment run (t267).** `mario/Enemy/chuuhana` strengthens the
dedicated-helper/class theory. Target `TChuuHana::setGoal` explicitly
stores `-30.0f` and `30.0f` to stack, computes `(max-min)` into
callee-saved `f31` before `rand()`, then uses `fmuls; fadds`. Spelling the
source as local `minYaw/maxYaw` with the raw `rand()` expression, and then
as `MsRandF(minYaw, maxYaw)`, both failed to produce the target f31
lifetime or improve the function. The same stack-interval symptom appears
in ChuuHana `reset`, `willFall`, and `initSetEnemies` for graph-node
selection. This is not solved by the current global 2-arg `MsRandF`; revisit
with a TU-local/helper-class experiment rather than changing
`RandomUtil.hpp` globally.

**Experiment run (t210).** Surveyed all 24 2-arg callsites: a mix of
literal args (`0.0f,100.0f`, `0.0f,1.0f`, `0.0f,360.0f`, `10.0f,20.0f`,
`16.0f,8.0f`, `2,3`, `-500.0f,500.0f`) and variable args
(`minR,maxR`, `params1->unk2C4,...`, `trapJumpMinSpXZ,...`,
`unk194->unk458,...`). The variable-arg sites are codegen-neutral
(memory load either way). The *literal* sites are the conflict: a global
const-ref flip net-regressed overall fuzzy, so at least one other TU's
literal callsite matches the *folding* (by-value) form while killer's
matches the *non-folding* (const-ref) form. One signature cannot satisfy
both → the original almost certainly had a dedicated `random interval`
helper/class distinct from the simple 2-arg `MsRandF`, used by the
smallEnemy family. Recommended fix when revisited: a killer-TU-local
(or smallEnemy-family-shared) inline taking `const f32&`, leaving the
global `MsRandF(f32,f32)` by-value. Do NOT re-flip the global header.

### Naming shared sub-products as locals forces CSE and defeats `fnmsubs`/`fmsubs` fusion in matrix/quaternion math

**Hypothesis.** When a block computes several outputs that share
sub-products (e.g. a quaternion→matrix conversion where each diagonal
term is `1 - 2*a*a - 2*b*b` and the products `2*a*a`, `2*b*b` recur
across two diagonals), writing each output as a single inline expression
lets MWCC fuse `const - prod` into one `fnmsubs`/`fmsubs` — which
*consumes* the product, so it cannot be CSE'd into the other output that
needs it. The target instead computes every product once with a plain
`fmuls` and uses `fadds`/`fsubs`, never fusing. Naming each shared
product as its own `f32` local forces MWCC to materialize it once into a
register and reuse it, exactly reproducing the no-fusion schedule.

```cpp
// fuses into fnmsubs, fails to share x2*x across m11 and m22 (WRONG):
m[1][1] = 1.0f - (x2 * x) - (z2 * z);
m[2][2] = 1.0f - (x2 * x) - (y2 * y);

// name the products → one fmuls each, shared, plain fsubs (RIGHT):
f32 xx = x2 * x; f32 yy = y2 * y; f32 zz = z2 * z; /* ... */
m[1][1] = 1.0f - xx - zz;
m[2][2] = 1.0f - xx - yy;
```

**Symptom that signals this lever.** Target shows a long run of `fmuls`
into distinct FPRs followed by `fsubs`/`fadds`, with NO `fnmsubs`/
`fmsubs`/`fmadds`; our build shows one or more fused `fnmsubs` where the
target had a separate multiply + subtract. Usually accompanied by a base
vs target frame/instruction-count mismatch that vanishes once the
products are named.

**Citation (1 TU).** `Enemy/Kazekun::calcRootMatrix` (t204): the
quat→matrix block was 85.83% with one `fnmsubs`; naming all 9 products
(xx/yy/zz/xy/xz/yz/wx/wy/wz) → **91.21%**, sizes byte-exact (580=580).
Residue is pure FPR coloring (ours spills f31 where target reuses f3).

**Promote to Settled** once confirmed on a second matrix/quaternion TU
(candidates: any `getQuat`/`setRotate`/`SMS_CalcToDirMatrix` consumer, or
a `TQuat4::setMatrix`-style conversion elsewhere).

### "Set/force current nerve NOW" idiom = `reset()` + `setNext(nerve)` + `pushAfterCurrent(nerve-or-default)`, guarded by `getCurrentNerve() != nerve`

**Hypothesis.** Enemy `kill`/`forceKill`/terminal-`execute` bodies that
"switch to a specific nerve immediately" (not queue-after-current) compile
to a compound inlined block, NOT `pushNerve`:

```cpp
if (mSpine->getCurrentNerve() != &TNerveX::theNerve()) {
    mSpine->reset();                              // mVertebrae.clear()  -> stw 0, 0x8(spine)
    mSpine->setNext(&TNerveX::theNerve());        // prev=cur; mTime=0; mCurrent=X  (0x1c,0x20,0x14)
    mSpine->pushAfterCurrent(&TNerveX::theNerve()); // or getDefault() — push onto cleared stack
}
```

Each `theNerve()` is spelled textually (the comparison, the setNext arg, and
the push arg are *separate* expressions), so the asm shows one Meyers
static-init guard **per** reference — 3 guards when the push uses the nerve,
2 when it uses `getDefault()`/`unk18` (no guard for the default load).
Key distinguisher from `becomeNerve(nerve)`: a single inlined helper taking
`nerve` as a param would evaluate `theNerve()` **once** (1 guard); the
multi-guard asm proves the calls are written out.

`setNext` vs `setDefaultNext`: `setDefaultNext()` inlines `setNext(unk18)`,
which loads `mCurrent` (0x14) for the if-test *before* loading `unk18`
(0x18); `setNext(getDefault())` evaluates the arg first → `unk18` load
*before* `mCurrent`. Match the 0x14-vs-0x18 load order to choose.

**Promote to Settled** once confirmed in a second TWalkerEnemy/TSmallEnemy
(e.g. another enemy's `kill`/`forceKill`). Currently single-TU.

**Citations (1 TU, bombhei t198).**
- `TBombHei::kill` 41.9% → 89.5% (reset+setNext+pushAfterCurrent(nerve)).
- `TBombHei::forceKill` 47.2% → 72% (push uses `getDefault()`, 2 guards).
- `TNerveBombHeiExplosion::execute` terminal block 76.8% → 92.6%
  (`reset()+setDefaultNext()+pushAfterCurrent(getDefault())` before `return true`).

Residual on all three: a TU-wide static-data-layout difference — target
hoists `@NNNN`/`@1431` data bases into callee-saved r30/r31 and addresses
nerve instances/registration strings as `base+off`, while our build emits a
fresh `lis/addi` per reference and names the block `.bss.N` instead of
`@NNNN`. Same family as the moveObject +8 / kill +4 / behaveToRelease 99%
residuals. See Open questions.

### Big-function deferred-inline budget declines to inline even the *first* operand's `theNerve()` Meyers singleton

**Hypothesis.** The accumulator-shape rule (see Settled) says a
materialized `bool b = (a == X::theNerve() || a == Y::theNerve())`
inlines the first operand's `theNerve()` and emits a `bl` for the
second. But in a *large* function MWCC declines to inline **even the
first** — both become `bl theNerve__...Fv`. Observed: `Enemy/Kukku`
target calls `theNerve__15TNerveKukkuFallFv` 2× and
`theNerve__19TNerveKukkuPostFallFv` 4×. `behaveToWater` (small) inlines
Fall + calls PostFall — matches with our source. `updateRotation`
(large, float-heavy, two such disjunctions) target calls **both** Fall
and PostFall, but our build still inlines Fall (the function is
oversized at base 720 vs target 584 *because* of the 4× inline
expansion). Suspected cause: the `-inline deferred` per-function
expansion budget for `updateRotation` is exhausted by its float math,
so theNerve never inlines there.

**Experiment to confirm/refute.** Complete the TU (implement `dropCoins`
+ `calcRootMatrix`, the two unwritten float giants) and rebuild — this
shifts the TU-global deferred-inline accounting and sdata/static-guard
symbol numbering. Re-diff `updateRotation`: if Fall's theNerve flips to
a `bl` and the frame shrinks toward 0x70, the budget hypothesis holds.
If not, the lever is something more local (no source-level control over
the first-operand inline in a big function — would become an Open
question / currently-hard). Also worth trying: an artificial second
disjunction or extra inline calls to push `behaveToWater`-sized
functions over the threshold and watch the first theNerve flip to `bl`.
See `state/notes/Kukku.md`.

### `#pragma dont_inline on/off` around an out-of-line accessor's `.cpp` definition forces a `bl` at call sites where `-O4` auto-inlines it

**Hypothesis.** When a small accessor is *declared* in the header but
*defined* in the `.cpp` (so it is not a header inline), `-O4`'s
auto-inliner will still inline it into earlier callers in the same TU
during the deferred pass. If the target keeps it as a real `bl` call,
wrapping **only that definition** in `#pragma dont_inline on` /
`#pragma dont_inline off` suppresses the auto-inline at every call site
while leaving the standalone body (and every *other* function's
inlining) untouched.

```cpp
#pragma dont_inline on
const JGeometry::TVec3<f32>& TWireTrap::getWireDir() const
{
	return getWireBinder()->getDir();   // standalone still emits 100%
}
#pragma dont_inline off
```

This is the *inverse* lever to the header-inline accessors: instead of
forcing a reload via inlining, it forces a call by *forbidding*
inlining. Useful when the target calls a trivial wrapper accessor
(`getWireDir()` = `getWireBinder()->getDir()`) out-of-line at one site
while inlining the underlying `getWireBinder()->getDir()` at another —
the wrapper's call survives, the inner accessors still inline.

**Symptom that signals this lever.** Target shows `addi r3, rThis, 0`
(set up `this`) followed by `bl <accessor>` then a 3×`lwz/stw` copy of
the returned ref to a stack TVec3; our build instead shows the accessor
body inlined (`lwz rBase, off(rThis); lwz/stw ...`) with no `bl`.

**Citations (single TU so far — needs a 2nd TU to settle, t188 wireTrap):**
- `getWireDir()` inlined → `bl` at 3 nerve velocity sites:
  - TNerveWireTrapSearch::execute 81.6% → 86.8%
  - TNerveWireTrapReturnMove::execute 77.4% → 80.9%
  - TNerveWireTrapOnewayMove::execute 89.8% → 91.2%
- Standalone `getWireDir__9TWireTrapCFv` stayed 100% throughout.
- Caveat: `#pragma dont_inline` is documented as a TU-global toggle for
  *everything defined while it is on* — scope it tightly to the single
  definition (on immediately before, off immediately after) so it
  doesn't suppress unrelated inlines.

### Returning a bare integer comparison (`return a < b;`) emits branchless materialization; `if (a < b) return TRUE; return FALSE;` emits the cmpw/branch/li form

**Hypothesis.** For a `BOOL`/`bool`-returning function whose body is just
`return a < b;` (signed ints), MWCC materializes the result *branchlessly*:

```
eqv   r0, r3, r4
subfc r3, r3, r4
srwi  r0, r0, 31
addze r3, r0
clrlwi r3, r3, 31
```

The target, however, often wants the simple branch form:

```
cmpw  r3, r0
bge   END        ; a >= b → return 0
li    r3, 0x1
blr
END: li r3, 0x0
blr
```

The source-level lever is to write an explicit `if`/return instead of
returning the comparison directly:

```cpp
// branchless (eqv/subfc/srwi/addze):
return self->mWaitTime < spine->getTime();

// branch form (cmpw/bge/li 1/li 0):
if (self->mWaitTime < spine->getTime())
    return TRUE;
return FALSE;
```

This also applies to `return done;` where `done` is a bool set in a
prior if/else — rewriting as `if (done) return TRUE; return FALSE;`
adds the target's second `clrlwi.`/branch widening.

**Diagnostic signature.** Target shows `cmpw`/`bge`(or other ordered
branch)/`li r3,1`/`li r3,0`; our build shows the `eqv`/`subfc`/`srwi`/
`addze` quintet (no branch).

**Citations (all `Enemy/wireTrap`, tick 186).**
- `TNerveWireTrapWait::execute`: 64.4% → 100% (sole change).
- `TNerveWireTrapGoWait::execute`: 50.8% → 100% (sole change).
- `TNerveWireTrapOnewayMoveStart::execute`: the `return done;` →
  `if (done) return TRUE; return FALSE;` change was part of 86.5% → 99.8%.

Needs a citation outside wireTrap to promote to Settled.

### Source comparison operand order maps directly to `fcmpo`/`cmpw` operand order; flip the source operands to flip the emitted compare and its branch/cror condition

**Hypothesis.** MWCC preserves the left/right order of a comparison's
operands when emitting `fcmpo`/`cmpw`. Writing `CONST > var` emits
`fcmpo cr0, fCONST, fVar` whereas the equivalent `var < CONST` emits
`fcmpo cr0, fVar, fCONST`. The branch/cror condition follows from the
operator (`<` vs `>`, `<=` vs `>=`) on the chosen ordering. When the
target loads the constant into the *first* compare register, the source
wrote the constant on the left.

```cpp
// target: fcmpo cr0, f(0.0), f(mWireDir)  → write const on left:
f32 sign = 0.0f > self->mWireDir ? -1.0f : 1.0f;   // not  mWireDir < 0.0f

// target: fcmpo cr0, f(1.0), f(mScaleRate); cror eq, lt, eq
if (1.0f <= self->mScaleRate) { ... }              // not  mScaleRate >= 1.0f
```

**Citations (all `Enemy/wireTrap`, tick 186).**
- `TNerveWireTrapOnewayMoveEnd::execute`: flipping `mWireDir < 0.0f` →
  `0.0f > mWireDir` in the sign ternary: 95.6% → 97.5%.
- `TNerveWireTrapOnewayMoveStart::execute`: flipping `mScaleRate >= 1.0f`
  → `1.0f <= mScaleRate` matched the `fcmpo f2,f0; cror eq,lt,eq` pair
  (part of 86.5% → 99.8%).

Distinct from the `!(dist < K)` hypothesis below (which is about NaN/
ordered-branch *opcode* choice); this one is about operand *order*.

### `if (!(dist < K))` emits `bge`, while `if (dist >= K)` emits the `cror eq, gt, eq; beq` lattice (inverse of the !(>=) hypothesis)

**Hypothesis.** Mirror of the `!(>=)` lever below. For a "fail-if-dist-too-big"
test where the target asm shows a single ordered branch (`bge SKIP`) instead
of the cror lattice, the source uses `!(dist < K)` rather than `dist >= K`:

```cpp
// BAD: emits cror lattice
if (dist >= 160000.0f) return;
// fcmpo cr0, fDist, fK
// cror eq, gt, eq
// beq EXIT

// GOOD: emits single bge
if (!(dist < 160000.0f)) return;
// fcmpo cr0, fDist, fK
// bge EXIT
```

Semantically equivalent for non-NaN floats. MWCC compiles `>=` through
the cror-lattice path (treating it as ordered-strict), but `!(<)` through
the simpler `bge` (which for floats includes "unordered"). The two produce
different NaN behavior but identical results for real arithmetic.

**Diagnostic signature.** Target's `fcmpo cr0, fX, fY` is immediately
followed by a single `bge LBL`, while our build emits `cror eq, gt, eq;
beq LBL` (one extra instruction).

**Citation.**

- `Camera/sunmgr::perform` (tick 144) Mario-distance gate
  (`dx*dx + dz*dz >= 160000.0f` → `!(dx*dx + dz*dz < 160000.0f)`):
  +1pp on the function (within the cumulative 77.8 → 98.07 above).
  Confirmed the rewrite produces target's `bge EXIT` instead of
  `cror eq, gt, eq; beq EXIT`.

**Note vs the `!(>=)` Hypothesis.** This is the polarity-flipped
sibling: `!(>=)` SUMMONS the cror lattice (for abs-by-sign blocks),
while `!(<)` BANISHES it (for distance-threshold returns). Same
underlying MWCC mechanism — the explicit `!` around the strict
comparison forces a different branch-form lowering than the
non-negated convex form.

**Promote to Settled when.** A second TU confirms the `!(<)` rewrite
swaps cror+beq for a single bge.

### `if (!(x >= 0.0f))` triggers `cror eq, gt, eq` lattice, while `if (x < 0.0f)` emits a plain `bge`

**Hypothesis.** When the target asm shows a `cror eq, gt, eq` followed by
`bne`/`beq` between a `fcmpo` and the body of an "abs-by-sign-flip" block
(`if-neg then -x`), the source uses `!(x >= 0.0f)` rather than `x < 0.0f`.
The two are semantically equivalent for non-NaN floats, but MWCC compiles
them with different CR-bit patterns:

```cpp
// Plain less-than — emits bge:
if (dx < 0.0f) {                // fcmpo; bge skip; fneg
    dx = -dx;
}

// Negated >= — emits cror lattice:
if (!(dx >= 0.0f)) {            // fcmpo; cror eq, gt, eq; bne fneg_lbl; b skip; fneg_lbl: fneg
    dx = -dx;
}
```

**Diagnostic signature.** Target's `fcmpo cr0, fX, f0` is immediately
followed by `cror eq, gt, eq; bne LBL_FNEG; b SKIP; LBL_FNEG: fneg fX, fX`,
while our build emits a single `bge SKIP; fneg fX, fX`. The cror lattice
produces an extra `b` instruction so any function with this pattern will
have a small (8B) instruction count delta in the abs block.

**Experiment.** Rewrite the `if (x < 0.0f)` to `if (!(x >= 0.0f))`. If the
cror pattern emerges, the hypothesis holds for that TU. To promote to
Settled, need a 2nd independent confirmation in another TU.

**Citations.**

- `Camera/CameraInbetween::execCameraInbetween` (tick 142): 87.6 → **87.8%**
  (+0.2pp). Both the dx and dz fabs blocks switched to the `!(>=)` form
  and the cror lattice appeared, matching target.
- `Enemy/BossHanachanMain::checkFallDecideAndSetup` (tick 221): the original
  source already used `if (!(absRot >= 0.0f)) absRot = -absRot;` but MWCC chose
  the opposite branch layout (`cror; beq skip; fneg`) and left the function at
  94.7%. Rewriting the field-load abs as a ternary assignment,
  `absRot = absRot >= 0.0f ? absRot : -absRot;`, produced the target
  `cror; bne fneg; b merge` layout, restored size parity, and lifted the
  function to 96.3%. Caveat: applying the same ternary shape to the later
  expression abs (`diff = body->unk13C - body->mRotation.z`) regressed to a
  276B function with extra `fmr`s, so this is a field-load/source-shape lever,
  not a blanket replacement.
- `Camera/CameraSecureView::calcSecureViewTarget_` (2026-06-07 12:43am MNL):
  rewriting expression abs from `if (sum < 0.0f) sum = -sum;` to
  `sum = sum >= 0.0f ? sum : -sum;` recovered the target
  `cror; bne fneg; b merge` branch lattice and moved `84.0% -> 85.0%`, but
  still introduced one extra `fmr f0,f1` before the compare. This supports the
  branch-layout half of the ternary lever while preserving the warning that
  expression abs may carry extra FPR residue.

**Counter-evidence (don't blindly apply).** Same lever applied to
`Camera/CameraSecureView::execSecureView_` (tick 142) **reduced**
match 71.9 → 71.6% even though the cror pattern emerged. Reason:
the `bne` (target) vs `beq` (ours after rewrite) branch direction
differed, and MWCC chose different FPR coloring for `sum`, adding
a wasteful `fmr f0, f2` and inverting the fnmadds path. Confirms
the rule only helps when the **surrounding register coloring stays
stable**. Symptom of cascade failure: extra `fmr fA, fB` after the
fcmpo in our build vs target.

**Where to try it next.** Any function with target asm showing
`cror eq, gt, eq; bne/beq` after a float comparison-to-zero, where the
sign-flip is "make positive". Common in distance/displacement code,
clamp-to-positive predicates, and CLBChase-style angle wrap helpers.
**Verify** that ours's current build already uses the same FPR for
sum as target; if there's an existing `fmr` shuffle right before the
fcmpo, the rewrite likely won't help.

### Inline `obj->member.value` at use sites (no intermediate locals) lands the load directly in the consumer FPR/GPR

**Hypothesis.** When the same `obj->member.value` (or any non-trivial
member chain) is read for both a comparison and a subsequent function
call argument, **inlining the chain at each use site** lets MWCC pick
the FPR/GPR for each load based on its immediate next consumer.
Caching the value into a local f32/int forces MWCC to allocate a
"neutral" register first and then `fmr`/`mr` to the consumer FPR,
adding an instruction per use.

**Cached-local symptom (our build):**
```
lwz   r3, OFFSET(this)        ; r3 = mParams
lfs   f0, MEMBER_OFFSET(r3)   ; load into f0 (the "neutral" reg)
fmr   f1, f0                  ; copy to the FPR the next compare wants
fcmpo cr0, fDist, f1          ; compare
```

**Inline-at-use form (target):**
```
lwz   r3, OFFSET(this)        ; r3 = mParams, kept alive across the if/elseif
lfs   f1, MEMBER_OFFSET(r3)   ; load straight into f1 (next consumer)
fcmpo cr0, fDist, f1
```
The same `r3` survives across the if/else-if branches so the next
`lfs fM, OTHER_OFFSET(r3)` for a different member can happen without
reloading the base.

**Counterpart.** This is the mirror of the existing
`feedback_lazy_local_for_post_call_load` (in `state/memory/`) which
introduces a typed local *LATE* specifically to defer a load past a
call. Both rules emerge from the same MWCC truth: the register where
a value ends up is determined by the next consuming instruction, not
by where the C++ declared it.

**Observation.** `Enemy/BossHanachanEffect::TBossHanachan::emitCamShake_`
(tick 130): removed `f32 maxDist = mParams->mSLCamShakeMaxDist.value;`
and `f32 zeroDist = mParams->mSLCamShakeZeroDist.value;` locals,
inlined the chain at the comparison sites and at the
`CLBCalcRatio<f32>(…ZeroDist.value, …MaxDist.value, dist)` call.
Combined with using a separate `f32 r` for the clamp chain (so the
join's `fmr f31, f1` happens once at the end instead of immediately
after the call), match rose 95.46% → 97.79%. Single-TU; needs second
confirmation to promote.

**Experiment to confirm/refute.** Sweep functions stuck at 90-99% match
where the diff shows an `lfs f0, …; fmr fN, f0` pair for a chained
member load. Candidates: anywhere using `TParamRT<T>::value` plus a
comparison plus a downstream call with the same value. Try inlining,
measure the delta. If the rule holds across ≥2 independent TUs,
promote to Settled. If counterexamples appear (e.g. inlining
*regresses* match because MWCC doesn't have an inline-call lever to
keep the base pointer alive), document the boundary condition here.

**Caveats observed so far.**

- Only safe if no calls fall between the inline reads. A call would
  force MWCC to spill/restore the base pointer, defeating the trick.
- Multi-arg template helpers like `CLBCalcRatio<f32>(a, b, c)` need
  each inline load to land in the right FPR (f1/f2/f3). If `c`
  is already in f3 (or f4 with an `fmr` planned), inlining `a` into
  f1 and `b` into f2 lets the call go through with zero arg shuffles.

### Comparison operand order schedules static-init guard inline expansion

**Hypothesis:** When a `!=` (or `==`) comparison contains an inline call
that expands to a static-init guard block (e.g. `DEFINE_NERVE`'s
`theNerve()` accessor), MWCC schedules the guard at the source position
of the call. Swapping the operands of the comparison can move the
guard relative to the other operand's evaluation. This is independent
of the C++ "evaluation order is unspecified" rule — MWCC consistently
places the inline expansion at the source position.

**Observed:** `Enemy/seal::receiveMessage` had
`mSpine->getLatestNerve() != &TNerveSealDie::theNerve()`. Target's
asm shows theNerve's init guard BEFORE the getLatestNerve load. Our
build had the guard AFTER, with getLatestNerve's result cached in r30
across the guard (extra lwz/cmplwi/beq/b/lwz). Swapping operands
(`&theNerve() != mSpine->getLatestNerve()`) moved our guard before
getLatestNerve, lifting match 87.91% → 97.25% (+9.34pp).

**Counter-example:** `Enemy/Amenbo::forceKill` has the same pattern
(`mSpine->getLatestNerve() != &TNerveSmallEnemyDie::theNerve()`) and
matches at 100% with the ORIGINAL operand order. So the swap is
context-specific, not universal.

**Experiment to confirm/refute:** Sweep all `getLatestNerve() != &T::theNerve()`
callsites in non-matching functions. For each, check whether target's
asm shows the init-guard-before or init-guard-after pattern. If
before, try the swap. If the rule holds, this should be a reliable
+5-15pp lever in similar cases. If it doesn't (e.g. the swap regresses
some), need a finer rule for what triggers MWCC to put the guard
first.

**Cost when applied:** 1 instruction difference (the `cmplw` operand
order flips), but trades that for 5-15 instructions of scheduling
match. Usually worthwhile.

**Cited:** `Enemy/seal::receiveMessage` (this file, tick 92).
Counter-example: `Enemy/Amenbo::forceKill` (matched at 100% without).

### `new T(args)` expression may spill the result to a stack-0x10 scratch slot

**Hypothesis:** When a `T* p = new T(args)` expression is followed by
method calls that take `p` as `this`, MWCC sometimes emits a
spill-and-reload sequence using a stack slot at +0x10 even though `p`
already lives in a callee-saved register (e.g. r29):

```
mr.  r29, r3        ; r29 = new() return value
beq  SKIP
stw  r29, 0x10(r1)  ; spill to stack 0x10
... arg setup ...
lwz  r3,  0x10(r1)  ; reload to r3 — but r29 already has it!
bl   __ct__T...
lwz  r3,  0x10(r1)  ; reload again
... use ...
bl   method
SKIP:
```

Our build for the same source skips the spill and uses
`mr r3, r29` / `addi r3, r29, 0x0` directly. This costs us 5pp
match in `Enemy/DebuTelesa::load` (94.57% with no spill, can't
reach 100% without it). Adds +8 bytes to the stack frame (slot
0x10 reserved).

**Observed symptoms:**
- Target stack frame is +8 bytes vs ours
- Target has `stw rN, 0x10(r1)` immediately after the `beq` of the
  null-check
- Two subsequent `lwz r3, 0x10(r1)` reloads before the ctor and
  before the method call
- After the method call, target uses `r29` directly for field
  accesses (so r29 was always live; the spill is redundant)

**Why might MWCC do this:** Possibly exception-safety paranoia
(if ctor throws, the unconstructed pointer must remain reachable
for cleanup) — but SMS has exceptions off. Or possibly a specific
source-level pattern triggers it.

**Experiment to test:** Find another TU where this `new T(...)` spill
pattern appears in target and try source variants:
1. Cast the new result to base class: `TParams* p = new TFoo(...)`.
2. Use `void* mem = ::operator new(sizeof(T)); new(mem) T(...);
   T* p = static_cast<T*>(mem); ...`
3. Move the local declaration outside the function (impossible).
4. Add a redundant `volatile` qualifier to the local.
5. Place the call in a try-block (if exceptions can be locally
   enabled).

Citation: `Enemy/DebuTelesa::load` 94.57% (stuck — spill not
reproducible from any straightforward source pattern tested in
tick 82).

### Inline pointer-arithmetic computes per-iteration; hoisted `(this + OFFSET)` reserves a register

**Hypothesis:** Two equivalent source forms for indexing into a field-offset
array generate different MWCC codegen. Form A hoists `this + OFFSET` into a
callee-saved register before the loop; Form B computes `(OFFSET + i*4)`
inline each iteration without a hoist.

```cpp
// Form A — base-pointer cast then array index:
//   addi rH, r3, OFFSET   ; outside loop
//   lwzx r4, rH, rI       ; inside loop (rI = i*4)
TBoundPane* p = ((TBoundPane**)((u8*)this + OFFSET))[i];

// Form B — single-expression pointer arithmetic:
//   addi r0, rI, OFFSET   ; inside loop
//   lwzx r4, r3, r0       ; inside loop
TBoundPane* p = *(TBoundPane**)((u8*)this + OFFSET + i * 4);
```

Form A consumes one extra callee-saved register (extending `stmw`/`stwu`);
Form B uses no extra register but adds one `addi` per iteration.

**Observed:** `GC2D/Guide::checkPoint` — two loops at `_168` and `_44C`.
Form A → Form B closed codegen 95.44% → 99.80% (only phantom stack
padding left).

**Experiment to confirm/refute:** Find another TU with a small loop indexing
a field-offset array where target's loop body shows
`addi r0, rIdx, OFFSET; lwzx`. Test Form B vs Form A; confirm Form B
matches when target uses the inline form.

### Copy-construct on declaration skips the default constructor's body

**Hypothesis:** When a class with a non-trivial default constructor (e.g.
`JUTRect() { set(0, 0, 0, 0); }`) is used as a local, copy-construction
emits only the copy ctor's body, while default-ctor + assignment emits both.

```cpp
// Form A — emits `bl set__7JUTRectFiiii` (default ctor) then `bl copy`:
JUTRect rect;
rect.copy(*src);

// Form B — emits only `bl copy__7JUTRectFRC7JUTRect`:
JUTRect rect(*src);
```

(This is standard C++ semantics, but the matching lever is worth tracking.)

**Observed:** `GC2D/Guide::checkPoint` — two `JUTRect` locals in two loops.
Switching to Form B removed the upfront `bl set__7JUTRectFiiii` call and
contributed to 87.65% → 99.80%.

**Experiment to confirm/refute:** Find another near-100% function where
target lacks a default-ctor call that our build emits. Switch the local's
declaration to copy-construction; confirm the call drops.

### Function returning its pointer argument: declare the return type even when callers ignore it

**Hypothesis:** When the target's epilogue is `mr r3, rN; blr` where rN is
a callee-saved register holding a pointer arg or pointer field, the source
declared the function with that pointer as its return type. A `void`
declaration generates no such `mr`. C++ mangling doesn't encode return
type, so callers that ignore the return value still match.

**Observed:** `GC2D/Guide::setup(JKRMemArchive*)` — target ends
`mr r3, r31; blr` where r31 = saved `archive` arg. Declaring
`JKRMemArchive* setup(JKRMemArchive*)` with `return archive;` closed
77.09% → 100%. Sole caller `MarDirectorDirect` ignores the return value.

**Experiment to confirm/refute:** Find another `void`-declared function
whose epilogue has `mr r3, rN; blr` with rN holding an arg or
field-of-this. Re-declare and add `return rN;` to all paths; confirm the
function moves toward 100%.

### MWCC -O4,p folds `0.0f * (expr)` to constant 0 in some TUs, not others

**Hypothesis:** Under `-O4,p`, MWCC's constant folder eliminates
multiply-by-zero terms (`0.0f * X`) including the entire subexpression
`X`, even when X is non-trivial. The target's compile of the same TU
sometimes PRESERVES the `0.0 * X` term, emitting `fmadds f0, 0.0, X, acc`
as a no-op addition. The trigger appears TU-local, not just expression-
local.

**Symptom (target vs ours), `move__12TSelectShineFv` spline block:**
Source:
```cpp
splineY = 0.0f * (omt * omt) + amp9 * (2.0f * omt * t) + amp * (t * t);
```
Target asm (preserves all terms):
```
fmuls f2, f2, f2       ; f2 = omt^2
... compute amp9 * (2*omt*t) into f0 ...
fmadds f0, f3, f2, f0  ; f0 = 0.0 * omt^2 + acc   <-- preserves the 0 term
fmadds f0, f5, f4, f0  ; f0 = amp * t^2 + acc
```
Our asm (folds away the 0 term and its dead operand):
```
... compute amp9 * (2*omt*t) into f0 ...
fmadds f0, f2, f1, f0  ; only amp * t^2 + amp9*(2*omt*t)   <-- no omt^2 anywhere
```
The entire `omt * omt` computation is eliminated as dead code.

**What I tried that did NOT change folding:**
- Reordering source terms (amp9 first, 0 term last). Same fold.
- All four spline branches show identical fold behavior.

**Experiment to confirm/refute:** Find another GC2D / Player TU where
target asm shows a literal `0.0 * X` survives constant folding under
`-O4,p`. Compare TU-level pragmas / inline directives. If folding
correlates with `-inline deferred` vs `-inline auto`, write a minimal
test to confirm.

Alternatively: try a non-literal zero, e.g. `(amp - amp)` or a `volatile
const f32 z = 0;` — does that preserve the `fmadds 0, X, acc`?

**Cost:** `move__12TSelectShineFv` is now `87.8%` after t343 restored the
out-of-line `bl add__TVec3` call boundary with `world = mPos + unk18`. It is
still capped partly by this zero-folding residue, with many mismatched
instructions in the spline cascading through to register coloring downstream.

### `addi rN, rM, OFFSET` field-address caching across calls

**Hypothesis:** When a `this`-relative field at a given offset is accessed
≥ 2 times AND at least one of the accesses follows a `bl` call, MWCC
hoists `addi rN, rM, offset` into a non-volatile register and uses
`offset 0` for subsequent loads/stores. This is net-zero or net-negative
on instruction count (the `addi` setup costs +1 vs. using the immediate
offset directly each time), but consumes a callee-saved register and
inflates the stack frame by 4 bytes per cached address.

**Symptom (target vs ours):** Target reads `lwz rA, 0xOFF(rThis)` two
times across a `bl` boundary. Ours does:
```
addi rN, rThis, 0xOFF       ; cache
lwz rA, 0xOFF(rThis)        ; (still uses immediate offset! quirky)
bl ...
lwz rA, 0x0(rN)              ; uses the cache
```
Net: +1 `addi`, +1 callee-save spill, +8 bytes stack (1 reg save + 1
restore + frame padding). Match drops 1-5pp per cached address.

**Where observed:**
- `src/Camera/CameraDemo.cpp::execDeadDemoProc_` — 4 accesses to
  `(this + 0x27c)`; ours caches into r5 (`addi r5, r3, 0x27c`), target
  doesn't. The cache also indirectly forces shouldReturn/firstMatches
  into different registers (r4/r0 vs target's r4/r5), preventing the
  multi-bool `addi r5, r4, 0` copy pattern.
- `src/Camera/CameraDemo.cpp::endDemoCamera` — 3 accesses to
  `(this + 0x2B4)`; ours caches into r3 (`addi r3, r31, 0x2b4`), target
  doesn't. Cost: ~1.5pp.
- `src/Camera/CameraDemo.cpp::updateGateDemoCamera_` — accesses to
  `(this + 0x2B4)` and `(this + 0x70)` both cached (`addi r31, r29, 0x2b4;
  addi r30, r29, 0x70`); target uses just r31 = this. Cost: ~19pp; stack
  grows from 0x30 → 0x40 to spill the extra regs.
- `src/Camera/CameraDemo.cpp::startGateDemoCamera` — `(this + 0x2B0)`
  and `(this + 0x2B4)` cached. Target uses 2 non-volatile regs; ours
  uses 4 (r28-r31). Cost: ~21pp.
- `src/Camera/CameraDemo.cpp::ctrlNormalDeadDemo_` — most aggressive
  case observed: 5 cached field addresses (`&this[0x3c]`, `&this[0x40]`,
  `&this[0x44]`, `&this[0x10]`, `&this[0x280]`) in r26-r29. Uses `stmw r26`
  to save 6 callee-saved registers; target uses just `stw r30, r31`.

**What I tried that did NOT prevent caching:**
- Replacing `void* save = ...; ... void* save2 = ...;` with reassign
  to the same variable.
- Using scoped `{ }` blocks to limit each local's lifetime.
- Adding a typed cast in the second access.

The cache decision appears to be made before instruction scheduling
and isn't affected by source-level variable lifetimes. It correlates
with **number of accesses ≥ 2** plus **at least one access following a
`bl` call** (so a non-volatile register is what's needed to survive
the call).

**Experiment to confirm/refute:** Reduce one of the `(this + OFFSET)`
access pairs to a single access by hoisting the value into a local
variable that's used across both call sites. If the cache disappears,
the trigger is access count. If it persists, the trigger is something
else (maybe address-of computations in the AST?).

Alternatively: write a minimal test where a struct field is accessed
exactly twice across a call. If MWCC caches in our test but target's
version (if reconstructable) doesn't, we may be hitting a compiler
flag difference (`-inline auto` vs `-inline deferred` heuristic?).

**Consequence if true:** Many Camera/ functions are capped 75-95%
because of this single MWCC quirk. If we find the lever, a sweep
could lift dozens of functions at once.

### MWCC sometimes inlines a call selectively within the same TU

**Hypothesis:** Even with `#pragma dont_inline on` set TU-wide (or the
helper otherwise visible only via the same flag environment), MWCC's
`-inline deferred,auto` (or `-inline deferred` for game code) can inline
*some* call sites of a function while keeping others as `bl` calls.

**Where observed:**
- `mario/Camera/CameraMode::isNormalCameraCompletely` — target has the
  first `isNormalCameraSpecifyMode(mMode)` as `bl`, but the second
  `isNormalCameraSpecifyMode(prevMode)` *inlined* as a jump-table
  switch. With our `#pragma dont_inline on`, both calls become `bl`,
  capping the function at 94.3% match.
- `mario/Camera/CameraMode::isTalkCameraInbetween` and `isLButtonCameraInbetween`
  — target inlines the dispatch helper twice (once for `mMode`, once for
  `prevMode`); with `dont_inline on`, both become `bl` and these
  functions stay at 30% / 0%.
- `mario/Camera/CameraMode::isOverHipAttackSpecifyMode` — target keeps
  both helper calls as `bl` (no inlining); without `dont_inline on`, MWCC
  auto-inlines the now-tiny helpers, dropping it to 20%; with
  `dont_inline on`, matches.
- `mario/Enemy/BathtubKiller` `behaveToWater` (30%), `attackToMario`
  (66%), `receiveMessage` (36%) — t297. All three guard with
  `getCurrentNerve() != &Explosion::theNerve() && != &Break::theNerve()`
  then `pushNerve(&XNerve::theNerve())`. **Target emits `bl theNerve` at
  the `==`/`!=` COMPARISON sites but INLINES the `theNerve()` inside the
  `pushNerve()` block.** Our build inlines `theNerve()` at the comparison
  sites too, bloating each function +90..190 bytes. (Note the cited
  Camera `dont_inline` lever does NOT apply: it's all-or-nothing and
  would force `bl` at the push site too, which target inlines.) The bl is
  accompanied by the `subf;cntlzw;extrwi.;bne` bool-equality idiom (a
  consequence of comparing against a `bl` result, not an independent
  signal). One asymmetry inside the same fn: in `attackToMario` the
  pushed nerve (Explosion) is ALSO the first guard clause, and its
  comparison DOES inline (sharing the push's `instance$` via CSE, init
  guard hoisted to first occurrence) while Break (compare-only) stays
  `bl`. In `behaveToWater` the pushed nerve (Break) is the *second*
  clause and its comparison stays `bl` — so position-of-the-pushed-nerve
  in the guard, not which nerve, decides whether the comparison inlines.

**Refuted source levers (t297, BathtubKiller):**
- *Definition order.* Target defines all `execute`/`theNerve` nerve
  bodies BEFORE the methods that compare them; our `.cpp` defined them
  after. Moving the `DEFINE_NERVE` block above `receiveMessage`/
  `attackToMario`/`behaveToWater` produced **zero** change (identical
  match %, identical asm). Under `-inline auto`, definition order does
  not gate this comparison-site inlining.
- *BOOL helper wrapper.* Hypothesized the guard went through an inlined
  `BOOL TSpineBase::isNerve(Nerve n) const { return mCurrent == n; }`
  (the `&theNerve()` passed as an arg → `bl` to evaluate, `==` → cntlzw).
  Added the helper and rewrote `behaveToWater` to `!isNerve(...) && ...`:
  MWCC **still inlined** `theNerve()` through the argument. No effect on
  match %. Reverted.

**Experiment to confirm:** Determine what source-level cue distinguishes
inline-here from no-inline-there. Candidates:
- The first call is at the top of the function and is the gating
  condition for an early return; the second is buried deeper. Position-
  in-CFG hint to inliner?
- Calls inside an `if (X)` short-circuit chain might inline more readily
  than calls in standalone `if (X) { ... }` blocks.
- Number of remaining call sites at deferred-inlining time (the first
  one may be on a "hot" CFG path; the second on a "cold" path).

**Consequence if true:** When a TU exhibits this pattern, write the
*inlined* call sites with the dispatch logic in-place (explicit switch
or comparison) rather than calling the helper; keep the *non-inlined*
calls as proper helper invocations. This avoids the `dont_inline`
all-or-nothing trap.

### `#pragma dont_inline` is TU-global, not lexical

**Hypothesis:** `#pragma dont_inline on/off` in MWCC 1.2.5 (GameCube) is applied
to every function in the TU using the **final** pragma state seen by the parser,
not lexically scoped around the function definition. Multiple positions of `on`/`off`
in the same file all produced the same compilation outcome — the state at
end-of-file is what stuck.

**Where observed:**
- `src/JSystem/JDrama/JDRDStageGroup.cpp`. Tried wrapping only `perform()` in
  `#pragma dont_inline on ... off`. Either both functions saw `on` (when the
  file ended with `on`) or both saw `off`. Could not pin one function on and
  another off.
- `src/NPC/NpcBase.cpp` (this tick, supporting evidence). Adding
  `#pragma dont_inline on` at top of file made isMadNpc's call to
  `isNormalMonteW` a `bl` (matching target). The previously-100%-matching
  isXxx predicates (which had no calls) were unaffected. The pragma applied
  TU-wide as expected.
- `src/System/TalkCursor.cpp` (t317, supporting evidence). Wrapping only
  `TTalkCursor::associateNPC` in `#pragma dont_inline on/off` did **not** force
  the desired `TRotation3::identity33` call. Leaving final TU state as
  `dont_inline on` did force out-of-line inline helpers, but it applied broadly:
  `associateNPC` regressed `41.5 -> 17.5`, `loadAfter` `99.6 -> 93.3`, and the
  dtor `100 -> 86` due to extra emitted/called helpers (`TPosition3`/`TRotation3`
  ctors, `setTrans`, `TFlagT::on/off`, `MActor::getModel`). Reverted.

**Experiment to confirm a different scope mechanism:** A TU where two
functions need different inlining decisions (one needs `on`, the other an
auto-generated dtor that needs `off`) and we successfully control them
independently using e.g. `__noinline` attribute, declaration trick, or
moving inline source out of header.

**Consequence if true:** When a TU needs both a `dont_inline on` function and an
auto-generated dtor that inlines empty base dtors, **we cannot have both match**
through pragma alone. A different mechanism (e.g. `__noinline` attribute,
declaration trick, or moving inline source out of header) is required.

## Open questions

_Seeded from the "currently-hard patterns" list in `CLAUDE.md` — promote to *Hypotheses
under investigation* the moment you have a testable theory, and to *Settled* once
confirmed in ≥2 TUs._

- **What source or visibility cue selects the constructor depth MWCC emits for
  nested empty geometry wrappers?** In `mario/Enemy/enemyMario`
  `TEnemyMario::consider`, target state `0x1B` constructs a stack
  `JDrama::TGraphics` but emits weak no-op calls to
  `TRotation3<TMatrix44<SMatrix44C<f32>>>::TRotation3()` and
  `TMatrix34<SMatrix34C<f32>>::TMatrix34()`. Current natural source
  `JDrama::TGraphics graphics; TMario::checkController(&graphics);` with
  existing `JDRGraphics.hpp` instead emits/uses the neighboring low-level
  `TMatrix44<SMatrix44C<f32>>::TMatrix44()` and
  `SMatrix34C<f32>::SMatrix34C()` constructors; changing `TGraphics` member
  types globally to `TRotation3<TMtx44f>`/`TMtx34f` removed those extras but
  still failed to call the target wrapper ctors and regressed other
  `TGraphics` owners such as `JDRDirector`. Next experiment: compare
  target/source include visibility and inline-depth pragmas for the small set
  of stack `TGraphics` owners (`JDRDirector`, `Application`,
  `MarDirectorSetup2`, `MovieDirector`, `enemyMario`) before attempting any
  header-level change.

- **How can a header-visible constructor inline in one TU while still emitting
  a matching standalone weak in its owner TU?** In
  `mario/System/MarDirectorInitECT`, moving
  `JDrama::TCamera::TCamera(float,float,const char*)` from
  `JDRNameRefGen.cpp` into `JDRCamera.hpp` made `TOrthoProj` construction
  inline like target (`initECTGft` `69.8% -> 90.2%`, `initECDisp`
  `79.3% -> 95.2%`) and emitted the missing
  `JGeometry::TVec3<float>::set<float>` helper. But the standalone 204B weak
  `TCamera` constructor disappeared from `mario/JSystem/JDrama/JDRNameRefGen`,
  where target's `TPolarCamera` construction calls it. In-class and
  out-of-class header `inline` definitions behaved the same. Re-adding the
  `.cpp` definition is a hard redefinition error. `#pragma inline_depth(1)`
  around `TPolarCamera` forced a constructor symbol but made its body
  nonmatching (18.2%) and regressed the campaign TU; `inline_depth(8)` restore
  compiled but did not preserve the weak. A narrower TU-local `inline`
  definition in `src/System/MarDirectorInitECT.cpp` preserves the 204B
  `JDRNameRefGen` owner and recovers the same `TOrthoProj` expansion plus the
  local `TVec3<float>::set<float>` owner, but it also emits target-absent
  `TViewObj::TViewObj(const char*)` and `TPlacement::~TPlacement()` helpers in
  the campaign TU. Next experiment: find a source cue that keeps the base
  constructor body visible for `TOrthoProj` while routing the header-defined
  `TViewObj`/`TPlacement` helpers to their existing owners.

- **What source shape preserves an out-of-line
  `TVec3<float>::set(const Vec&)` call when the copied temp is only read back
  as scalars?** In `mario/Camera/CameraWarp`
  `CPolarSubCamera::addMoveCameraAndMario(const Vec&)`, target updates four
  camera vectors, loads `gpCameraMario`, calls the existing weak
  `JGeometry::TVec3<float>::set(const Vec&)` into a stack temp at `r1+0x38`,
  then adds that temp to Mario's cached position. Natural source
  `JGeometry::TVec3<f32> tmp; tmp.set(delta); mario->mPosX += tmp.x; ...`
  scalarizes the copy, shrinks the frame from target `0xd0` to `0x28`, and
  falls through directly to `TCameraInbetween::addMoveCameraAndMario`. Moving
  the temp declaration before the first vector updates was neutral; routing the
  temp through a typed pointer (`tmpPtr->set(delta); tmpPtr->x`) regressed
  `61.3% -> 60.5%`; a local inline wrapper around `set` was also neutral. Other
  Camera TUs such as `mario/Camera/lensflare` and `mario/Camera/sunmodel`
  naturally keep the same weak call when the temp later escapes to another
  function, so the missing condition may be "address escapes after set" rather
  than declaration order. Next experiment should compare those call sites'
  lifetime/escape shapes before trying another local spelling.

- **What source shape preserves JGadget list iterator constructor call
  boundaries without forcing broad weak emission?** Several list-insertion
  targets keep explicit iterator constructor/copy calls that current natural
  source shapes inline away. In `mario/System/PerformList`,
  `TPerformList::load()` target goes
  `TSingleNodeLinkList::end()` -> typed
  `TSingleLinkList<TPerformLink,0>::iterator(base)` ->
  `Element_getNode<TPerformLink>()` -> base `Insert()`, and the TU owns the
  4B/12B typed weak helpers. Current `Push_back(new TPerformLink(...))`
  preserves behavior and scores best, but inlines the typed constructor and
  `Element_getNode`, leaving both helpers missing. A named base iterator plus
  named typed iterator still inlined both helpers and regressed `load()`
  `82.6% -> 77.9%`; explicit base insertion in the public `push_back` overload
  also inlined the base iterator constructor and regressed `85.8% -> 69.2%`.
  Similar symptoms exist in `mario/MoveBG/sunmodel` and `mario/Map/Map` with
  `TList_pointer_void` iterator setup. The next experiment should compare a
  naturally matching JGadget list owner, not another named-local spelling.

- **What natural source condition makes a TU own an inline virtual destructor
  and vtable without a target-absent dummy caller?** In `mario/JSystem/JKernel/
  JKRFileCache` (2026-06-05), the source dummy
  `reinterpret_cast<JKRFileFinder*>(0)->~JKRFileFinder()` emits the target weak
  `JKRFileFinder::~JKRFileFinder()`, `JKRFileFinder::__vtable`, and a 16B data
  companion, but also emits an extra 52B `dummy()`. Directly removing the dummy
  compiled but made all three target owner artifacts missing and failed the DOL
  hash. Marking the helper `static inline` also dropped the destructor/vtable
  owner artifacts, so an unused inline helper is not enough to instantiate the
  weak owner. Similar extra-only dtor/vtable forcing shapes appear in small
  JSystem / JDrama TUs (`JUTDbPrint`, `JDRDStageGroup`). Contrast
  `mario/JSystem/JKernel/JKRAramStream`: its dummy emitted a duplicate
  `JSURandomInputStream::getAvailable()` weak body, while the target owner is
  `mario/JSystem/JSupport/JSUInputStream`, so removing that dummy was safe and
  left the real owner exact. Also contrast `mario/JSystem/J2D/J2DScreen`: the
  TU has a real `J2DSetScreen` constructor, and removing the explicit
  `J2DSetScreen` destructor dummy left the target weak destructor and vtable
  exact. Next experiment should compare a TU where MWCC naturally owns an inline
  virtual dtor/vtable against these dummy-forced TUs, looking at class
  declaration location, first odr-use, vtable key-function ownership, and
  `-inline deferred` ordering before trying another local deletion.

- **What source shape makes C-mode MSL `__log2f` load the `@93`
  bit-pattern coefficient table without inflating every inlined caller
  frame?** In `mario/PowerPC_EABI_Support/.../exponentialsf` (t405/t419), target
  `powf` has three inlined `__log2f` blocks that load two coefficient bit
  patterns from `@93` into stack slots before the fractional-tail branch. A
  block-scope `static const unsigned long __log2e_coeff[]` recovered the
  `.sdata2` bytes but regressed `powf`: using the table directly in the
  correction branch moved `34.7% -> 33.0%`, while materializing `coeff0` /
  `coeff1` locals before the branch moved `34.7% -> 31.2%` and inflated the
  frame. An automatic `const unsigned long[]` later recovered target `@93`
  but still added bit-cast stack traffic; an automatic `const float[]` with
  the exact bit-rounded values plus union bit-cast locals moved `powf` to
  `92.7%` and cut the frame to `0xd8`, but target is still `0x90`. Remaining
  open residue: what natural source shape gets the target's earlier
  `clrlwi r?, bits, 9` before the low-16 branch and the target
  `delta2 = delta * delta` scheduling before the coefficient `fmadds`, without
  re-inflating the frame? A `mantissaBits` local recovered the early `clrlwi`
  but regressed `powf` `87.4% -> 87.3%`, and `static inline __log2f` worsened
  the local-static symbol to `__log2e_m1$8`, so those variants are refuted.

- **What source shape forces MWCC to narrow a casted `s16` local before an
  intervening call when the target does `addi; extsh; bl` (t359,
  `CameraSecureView`)?** In both secure-view functions, target computes Mario's
  backward angle as `lha gpMarioAngleY; addi -0x8000; extsh` before the first
  `CLBLinearInbetween<f32>` call, then uses the narrowed value later. The natural
  `s16 marioBack = (s16)(*gpMarioAngleY - 0x8000);` keeps the un-narrowed value
  live across the call and emits `extsh` only at the later delta expression.
  Tried and reverted: `const s16` (no effect), splitting into
  `marioBack = *gpMarioAngleY; marioBack -= 0x8000;` (MWCC delayed the `addi`
  until after the call, regressing `execSecureView_` 82.6% -> 77.6% and
  `calcSecureViewTarget_` 84.0% -> 70.1%). Next experiment should test a real
  inline/helper boundary or an earlier signed use, not another assignment
  spelling.

- **What remaining source shape gives `TWarpInCallBack::execute` the target
  random-scale scheduling and 0x110 stack frame (t323,
  `Player/MarioParticle`)?** The original "why do the three scale calls stay
  out-of-line" part is answered: friend `operator*(TVec3, f32)` forces the
  `bl scale` calls (0.0% → 60.0%). The remaining residue is narrower: target
  schedules `extrwi/xoris` after saving r31, computes randomScale with the
  signed-conversion path, and allocates a 0x110 frame; current best source keeps
  the scale calls and FPR lifetimes but uses a 0xd0 frame, schedules part of the
  random conversion earlier/later, and needs extra vector-copy temps. The signed
  `(s32)` cast produced `xoris` but scheduled it before the frame setup and
  regressed 60.0% → 55.2%, so the next experiment should change expression
  boundaries/lifetimes rather than simply casting.

- **Frustum-clip-over-actor-array loops co-mismatch on (a) a small phantom-inline
  frame inflation AND (b) a loop-counter ↔ in-loop-pointer NV-register swap —
  confirmed in ≥2 TUs but no source lever found (t301).** The canonical shape:
  `SetViewFrustumClipCheckPerspective(...); for (i=0;i<num;i++){ T* p=arr[i];
  Vec pos=p->mPosition; ... ViewFrustumClipCheck(gfx,&pos,r); set/clear a flag }`.
  Two instances:
  - `Enemy/DebuTelesa::clipEnemies` (98.47%): target frame 0x58, ours 0x50 (+8B);
    target `i`→r29, `actor`→r30; ours `i`→r30, `actor`→r29 (swapped). Source has
    `s32 num = mObjNum;` hoist (correct — both load num once into r31).
  - `Animal/fishoid::clipBoids` (97.2%): target frame 0x70, ours 0x48 (+0x28B);
    target inits IVs in order index(r31)/mActors-off(r28)/mBoidData-off(r27); ours
    inits mBoidData-off(r31)/mActors-off(r30)/index(r29) — reverse order.
  In both, the persistent loop counter and the in-loop-derived pointer/offset IVs
  end up in *different relative NV-register slots* than the target, and the target
  reserves more stack (8–40B) than our (logically-identical) body needs — the
  classic "inlined function inflates frame" residual with the inline's body fully
  DCE'd. The frame delta varies (8 vs 0x28) so it's not a fixed per-call constant;
  likely tied to which of getFovy/getAspect/SetViewFrustumClipCheckPerspective/
  ViewFrustumClipCheck had an inlined wrapper in the original. **Experiment ideas
  (none tried yet):** (1) probe whether the IV register order follows source
  declaration/first-use order by reordering the in-loop accesses; (2) check
  whether the +8/+0x28 frame comes from an inlined accessor by diffing a matched
  sibling clip loop (none found yet at 100%). Skip register-only ratholes; only
  worth revisiting if a *matched* frustum-clip loop surfaces to copy its idiom.

- **Inlined `(Vec){1.0f,1.0f,1.0f}` compound literals leave DEAD, unreferenced
  `(1,1,1)` aggregate constants at the FRONT of `.rodata`, shifting the
  infectious-string base by 0xC each and breaking every string-offset match in
  the TU (t299, `Enemy/BathtubKiller`).** Symptom: `setMActorAndKeeper` 99.88%
  with every `addi rX, rRodataBase, 0xNNN` off by exactly +0x18 vs target
  (our `0x154/0x170/...` vs target `0x13c/0x158/...`). Root cause: our `.rodata`
  begins with two 12-byte `0x3F800000 x3` objects (`@692`,`@705`) that the target
  does NOT have — confirmed *unreferenced by any code or data relocation* in our
  `.o` (grep of dtk disasm finds only their `.obj`/`.endobj`, zero load/branch/
  data refs). They push the dummy 12-zero string from `.rodata:0x0` to `0x18`.
  The target materialises `(1,1,1)` component-wise (three `lfs` of scalar `1.0f`
  from `@sda21`), so it has no rodata triple at all. These come from an inlined
  header function containing a `(Vec){1.0f,1.0f,1.0f}` compound literal — repo
  has these in `J3DJoint.hpp` (`J3DMtxCalcBasic::init`, `J3DMtxCalcMaya::init`,
  both virtual) and `MapCollisionEntry.hpp:44` (`setUpTrans`, virtual). MWCC
  apparently inlines a (de-virtualised?) call, optimises away the *use* of the
  aggregate, but cannot DCE the rodata const it spawned. **Open:** which exact
  inline site in `bind`/`calcRootMatrix` spawns the two triples, and what
  source-level change suppresses the dead const without changing logic. Until
  solved this caps `setMActorAndKeeper` at 99.88 and bloats `.data` (we also emit
  weak `__vt__TParamRT/TParamT` + `MtxCalcTypeName[]` array + `__vt__TNerveBase`
  that the target's `.data` lacks: our `.data` 736B vs target 656B, `.rodata`
  +0x18). Low individual payoff (one fn from 99.88→100), but the mechanism is
  general — any TU whose infectious strings sit at the wrong offset should be
  checked for leading dead `(1,1,1)` rodata aggregates first.

- **A `TVec3<f32>::set<int>(0,0,0)` template call is emitted OUT-OF-LINE by the
  target but INLINED by our build, even at 2 call sites (t299,
  `Enemy/BathtubKiller`).** `set<i>__Q29JGeometry8TVec3<f>Fiii` is `base missing`
  in report.json — the target instantiates it as a local out-of-line function
  (the int→double `xoris`/`lfd @sda21`/`fsubs` magic ×3) and calls it via `bl`
  from both `TNerveBathtubKillerExplosion::execute` and `...Break::execute`
  (where `killBathtubKiller`'s `vel.set(0,0,0); setVelocity(vel)` is inlined).
  Our `template <class TY> void set(TY,TY,TY)` (JGVec3.hpp) gets fully inlined at
  each site instead. This is the only TU in the whole tree whose target emits an
  out-of-line *int* `set<i>` (grep of `build/GMSJ01/asm`). Caps both execute
  nerves (~48-52%) and the `set<i>` fn at 0%. **Open:** what makes MWCC keep the
  int-conversion template out-of-line here — inline-budget at this nesting depth,
  or a `-inline` interaction. No source lever found; part of the
  kill/break/explode "helper inline" cluster already noted in campaign_tu.md.

- **(ANSWERED t285 — moved to *Hypotheses under investigation*.)** The int-range
  rand stack-homing in `reset__7TIgaiga` is reproduced by `volatile int min/max`
  (defeats constant-folding so they stay in memory) + an explicit `int range =
  max - min;` temp evaluated *before* the `MsRandF()` call (so MWCC holds the subf
  result in callee-saved r29 across `rand()`). 74.7% → 99.6%, instruction stream
  byte-identical; residual is +8 frame pad. The fabricated `MsRand(int,int)` inline
  helper was REFUTED — it folded the literals like the inline expression. See the
  Hypotheses entry.

- **(ANSWERED t279 — moved to *Hypotheses under investigation*.)** The
  `getModel()->getAnmMtx(N)` lowering vs target inline joint access is now
  understood: bare `getModel()` calls out-of-line `TLiveActor::getModel`;
  `getMActor()->getModel()` inlines `mMActor->unk4`. Per-site, diff-driven.
  Confirmed in igaiga (calcRootMatrix 95.9→99.3, setDeadAnm 97.9→99.9); a
  20-TU enemy sweep found no other applicable site. See the Hypotheses entry.

- **Nerve-heavy fns: target names the static-nerve/registration-string
  block `@NNNN` and hoists its base into a callee-saved reg (r30/r31),
  addressing every inlined `theNerve()` instance + `__register_global_object`
  name string as `base+off`; our build emits the block as `.bss.N` and
  re-materializes a fresh `lis/addi` per reference (t198, bombhei).** When a
  function inlines ≥3 `theNerve()` Meyers singletons (e.g. `kill`,
  `forceKill`, `moveObject`, `behaveToRelease`, `Explosion::execute`), the
  target's prologue does `lis rX, @NNNN@ha; addi r30, rX, @NNNN@l` once and
  then `addi r0, r30, 0x48` / `addi r5, r31, 0x18` etc. for each instance,
  allocating an extra callee-saved register and (often) +4/+8/+0x20 frame.
  Our build addresses each `instance$K@sda21` / vtable directly with inline
  `lis/addi`, uses one fewer saved register, and a smaller frame — so these
  functions land at 72–99% with the *logic byte-identical* but every data
  reference and the stack/register prologue shifted. Symptom: in the diff,
  every real mismatch line is a `@NNNN` vs `...bss.N` symbol, an
  `instance$<big>` vs `instance$<small>` renumber, or an `addi rN,r3X,off`
  base+offset vs our inline `lis/addi`. Question: what source/layout
  property decides whether the anonymous static cluster gets a named `@NNNN`
  base worth hoisting? Suspect ordering/count of nerve `DEFINE_NERVE`
  statics relative to the TU's other anonymous data; possibly an
  infectious-data declaration (cf. the `dummy1431` `.data` rule) is missing.
  Experiment: try adding the bombhei-specific infectious statics the target
  emits near `@2904`/`@3009` and check whether the block renames to `@NNNN`
  and the base hoists.

- **`TQuat4::rotate` inline spills its internal `q`/`q2` quats to stack
  (+0x20 frame); target keeps the whole rotation in FPRs (t196, Kukku).**
  `JGQuat4.hpp::rotate(const TVec3&, TVec3&)` computes two intermediate
  `TQuat4 q; TQuat4 q2;` (the sandwich `q*v*q^-1`). The cleanest isolated
  reproduction is `calcMomentum__6TKukkuFf` (92.86%, `#pragma dont_inline
  on`, body = `q=SMS_Eular2Quat(mRotation); v(0,0,speed); q.rotate(v,v);
  return v;`). Target frame **0x78**, ours **0x98** — a uniform +0x20
  shift of every local. The +0x20 is exactly two 16-byte `TQuat4` slots:
  target keeps `q`/`q2` live in f0–f13 and reads `v`(0x48–0x50) and the
  named `q`(0x54–0x60) straight from their stack slots in the fmadds
  chain, writing the 3 results back over `v`'s slots — it never
  materializes the rotate intermediates to memory. Our build allocates
  0x38–0x57 for them. The fmadds *shape* is identical (71 instrs each);
  only the offsets (+0x20) and FPR numbers differ, so this is pure
  register-allocation/scheduling, not a logic diff. **The inline's own
  comment already says "Incollect regalloc".** Tried (REFUTED this
  tick): rewriting `q`/`q2` as plain `f32 q2x,q2y,q2z` locals instead of
  `TQuat4` structs — NET REGRESSION across all 7 rotate users (Kukku
  81.427→81.424, Bird 87.293→87.238, BathtubKiller 30.424→30.200,
  Kumokun, coasterkiller, fireWanwan all down; overall 70.14171→
  70.14004). The struct form is what the original used. Experiment to
  try next: match the target's fmadds *operand order* exactly in the
  rotate body (the target schedules `fneg`s early and interleaves the
  two-quat products in a specific order) — the spill may be a scheduling
  artifact of our expression order, fixable without touching the struct
  decls. Blocks `calcMomentum`, `dropCoins`, `calcRootMatrix`,
  `perform__10TKukkuBall` in Kukku, and every quat-rotate enemy. Distinct
  from the frame-UNDER-allocation entry below (this is OVER-allocation).
  Risk: any rotate-body edit is cross-TU (7 users) — always run a
  before/after report over all of them.

- **Frame UNDER-allocation: our build emits a stack frame *smaller* than
  the target (t192 triage).** The documented MWCC padding bug usually
  makes OUR frame BIGGER. But several near-matches show the opposite —
  ours is 8–24 bytes *smaller* than target, with the saved-reg block and
  every local sitting at a lower r1 offset (uniform shift):
  `System/MarDirector::loadParticle` (target 0x40, ours 0x28, −0x18),
  `Player/ModelWaterManager::drawWaterVolume` (target 0xa8, ours 0xa0, −8),
  `System/MarioGamePad::updateMeaning` (target 0x190, ours 0x188, −8). In
  updateMeaning the missing 8 bytes coincide with an `lfd f0` reading a
  freshly-`stw`'d int pair (an 8-byte double temp materialized from two
  GPR stores) — target reserves a dedicated 8-byte slot for it, ours
  appears to overlap/elide it. Hypothesis to test: the int→double bit-cast
  temp (the `stw;stw;lfd` idiom, e.g. from `(f64)(int<<2 | int)` or a
  packed-pair-to-double) gets its own non-overlapping stack slot in the
  target, and source that forces a distinct named `f64`/`double` local
  for that value would reserve the slot. Experiment: on updateMeaning,
  introduce an explicit `f64` local for whatever feeds the `lfd` and see
  if the frame grows by 8. Distinct from the +N padding-bug family;
  citations are read-only diffs only so far (no fix yet).

- **`cror eq, lt, eq; bne; b` ternary form for `(f > 0.0f) ? CALL : 0.0f`
  (MSHandle calcDolby/calcPan, t176).** Target compiles the ternary
  `f32 angle = (param > 0.0f) ? MSACos(-vec.z / param) : 0.0f;` as
  `lfs f1, 0.0; fcmpo f31, f1; cror eq, lt, eq; bne <call_block>;
  b <merge>; <call block>; <merge>:`. Our build emits the natural
  `lfs f1, 0.0; fcmpo f31, f1; ble <merge>; <call block>; <merge>:`
  (single conditional branch, no cror). The `cror eq, lt, eq` form
  ORs LT and EQ into EQ-bit; the subsequent `bne` is equivalent to
  "branch on (LT||EQ) == 0", i.e. branch on GT-or-UN (NaN-tolerant
  GT). Tried: rewriting as `!(param <= 0.0f) ? CALL : 0.0f` — produces
  the cror correctly but also adds `mfcr+extrwi.` BOOL-conversion
  cruft, so net match drops. Tried: `if (param <= 0.0f) angle = 0;
  else angle = MSACos(...);` swap — MWCC inlined MSACos under the
  else branch (0% match). The structural form that triggers cror+bne
  without inlining MSACos is unknown. Pattern repeats in two MSHandle
  functions; closing it would shift both calcDolby (84.67) and calcPan
  (89.10) by ~5-10pp each.

- **TU-local .cpp method inlined despite single call site under -inline
  deferred (Player/Tongue::movement, t170).** `canGo()` is defined in
  Tongue.cpp (not header-inline) and is the only caller in `movement()`.
  Target shows `bl canGo`; ours fully inlines canGo's body (incl. its
  bl checkGround/checkRoof sub-calls). Tried: `BOOL res = canGo();` then
  `if (!res)`, `#pragma inline_max_size 0`, `#pragma inline_max_total_size 0`,
  `#pragma dont_inline on/off` wrap. None inhibited the inline. canGo also
  appears as standalone in symbol list, so it IS emitted — just also
  inlined at call site. The lever to inhibit single-call-site cpp-defined
  function inlining is unknown. Same TU also has `__ami__TVec3` weakly
  emitted by target but never by ours, suggesting the same root cause:
  some inline-inhibitor in target's source we haven't found.

- **Templated `push(const T&)` inlines wrapper but not template body
  (NPC/NpcEvent, t160).** TSpcInterp's `push(const TSpcSlice&)` forwards
  to `mProcessStack.push(slice)`. In `evIsNpcSinkBottom`,
  `evCheckCurNerve4Npc`, `evCheckLatestNerve4Npc` (and likely more
  short-bodied SPC handlers), target inlines the outer wrapper but
  keeps `bl push__21TSpcStack<9TSpcSlice>FRC9TSpcSlice` as an explicit
  call. Our build inlines BOTH layers. In `evGetAddressFromViewObjName`,
  `evSetFruitType`, `evCheckMonteClear`, `evConnectDummyNpc`, both
  target and ours inline both layers (matching). What controls the
  template's selective non-inlining? Same TU, same template, ~4 sites
  inlined, ~3 sites out-of-line. Tried: `TSpcSlice slice((int)x);
  interp->push(slice);` — no change. Possibly related to: function body
  size at the call site, number of locals live across the push, or the
  -inline deferred pass's order of expansion. The `getLatestNerve` weak
  symbol emission (a separate 28-byte instantiation of TSpine's getter)
  may be related — both involve template instantiation choices the
  inliner makes per-call-site.

- **NPC/NpcEvent 8-byte stack-frame inflation in target (t160).**
  Multiple BOOL-handler functions have target's stack frame 8 bytes
  *larger* than ours (e.g. evIsGameModeNormal 0x30 vs 0x20,
  evCheckMonteClear 0x90 vs 0x88, evIsDemoMode 0x28 vs 0x20, ev_-
  ForceStartTalk 0x90 vs 0x68). Code/insn count is identical, only
  stack-slot offsets shift by 4-8 bytes. Suspect: missing rodata
  symbols inflate target's NpcEvent.o rodata by ~0xCF bytes (0x330 vs
  ours 0x261), affecting MWCC's spill scheduling. Or some inlined
  function with an extra stack slot we're not reproducing. Several
  twelve-byte BSS entries (@2640–@2654) appear in target but only two
  in ours — could be uninitialized TVec3 statics, default-arg
  storage, etc. Need to identify what causes target's extra rodata.


- **Binary-search-of-value-ranges branching pattern (TNpcParams init
  loop, t152).** Target's asm for `__ct__10TNpcParamsFv` has a strange
  test cascade that looks like a switch lowered to **branching** mode
  rather than jump table:
  ```
  cmpwi i, 10; bge L_inner_10   ; >= 10 → forward to inner test
  cmpwi i, 5;  bge L_default    ; 5..9 → default
  cmpwi i, 1;  bge L_alias_0    ; 1..4 → alias_0 (forward to body block!)
  b L_default                    ; i == 0 → default
  L_inner_10: cmpwi i, 12; bge L_default; b L_alias_9
  L_alias_0: ... ; b L_loop_end
  L_alias_9: ... ; b L_loop_end
  L_default: ... ; falls through to L_loop_end
  ```
  This is a **binary-search of the transition points** (1, 5, 10, 12)
  with the case bodies laid out as **separate blocks below the test
  cascade**, reached via `bge LBL` forward jumps. Standard if-else
  chains INTERLEAVE tests and bodies — the tests-then-bodies layout is
  switch-style branching. **What we can't reproduce**: levers tried in
  t151–t152:
  - `switch (i) { case 1..4: ...; case 10..11: ...; default: ...; }`
    — MWCC picks JUMP TABLE (range 0..28, 6 cases, ~21% density)
    regressing to 28.19%.
  - Continue-based nested if-else (`if (i<10) { if (i>=1 && i<5)
    {...; continue;} } else { if (i<12) {...; continue;} } default;`)
    — gives 32.90% with correct body layout order but in-line
    alias_0/alias_9 (not as separate `bge`-target blocks).
  - Explicit binary-tree with 3 separate `default` blocks — MWCC fails
    to merge the new() calls → 7% regression.
  - Test direction tweaks (`i < 5 && i >= 1` vs `i >= 1 && i < 5`) —
    no movement.
  - **Coupled register-allocation residue**: target keeps `r28=this`
    fixed, `r31=i*4` advancing, `r30=&sSaveFileName` fixed (uses
    `add r3, r30, r31`); mine strength-reduces both to running
    pointers `r31=this+i*4`, `r30=&sSaveFileName[i]` (extra
    `addi r30, r30, 0x4` in loop bottom). Likely linked to the same
    switch lowering that drives the test cascade pattern.
  - Filed by: t152 INV, citation `NPC/NpcSave::TNpcParams::TNpcParams`
    (32.90% best). See also `state/notes/NpcSave.md`.

- **`JGeometry::TUtil<f32>::inv_sqrt` inlined in our build but emitted
  as `bl` in 50 target TUs.** Symptom (tick 150,
  `NPC/NpcCollision::execNpcObjCollision_`): target asm has
  `bl inv_sqrt__Q29JGeometry8TUtil<f>Ff`; ours inlines the 7-instruction
  Newton-Raphson body (`frsqrte; frsp; fmuls; fmuls; fnmsubs; fmuls`)
  at every call site, plus 2 extra FPR constants (`@420 = 3.0`,
  `@421 = 0.5`) hoisted into callee-saved registers in the prologue.
  Net cost: per-call function gains ~12-15 bytes and adds 2 extra
  callee-saved FPR slots.
  - JGUtil.hpp defines `inline f32 TUtil<f32>::inv_sqrt(f32)` wrapped
    in `#pragma dont_inline on/off`. Target's nm shows 50 TUs with
    `U inv_sqrt__Q29JGeometry8TUtil<f>Ff` (undefined references) and
    exactly one TU (`JSystem/JParticle/JPAEmitter.o`) with the weak
    `W` definition at offset 0x800. Our build emits ZERO TUs with
    either `U` or `W` — all 50 inline the body.
  - The `#pragma dont_inline on` should prevent inlining, but
    apparently the `inline` keyword on the function overrides it
    under `-inline auto` (the default flag for game TUs).
  - Hypothesized fix: move the function body out of the header (drop
    `inline` keyword) into a `.cpp` file. Then only that TU has the
    body; all other 50 TUs see only the declaration and emit
    `bl inv_sqrt`. Risk: a handful of TUs may currently be relying
    on the inlined form for their match — sweep all 50 before/after
    to verify net positive.
  - Affected: `NPC/NpcCollision::execNpcObjCollision_` (85% — would
    likely gain ~5pp if `bl` form is achieved), and dozens of other
    TUs using `inv_sqrt` in distance-normalization paths.
  - High-value cross-TU sweep target.

- **`gekko_ps_copy12` inlined in our build but emitted as a callable
  function in target.** Symptom (tick 115, `MoveBG/MapObjTree`):
  target asm calls `bl gekko_ps_copy12__9JGeometryFPvPv` at 3 sites
  (controlLeaf, initMapObj, plus another in controlLeaf). Our build
  inlines the 12 `psq_l` + 12 `psq_st` op pairs at every call site,
  ballooning function size by ~120 bytes and dragging controlLeaf
  to 69.82%, initMapObj to 78.28%.
  - Function definition is in `include/JSystem/JGeometry/JGMatrix34.hpp`,
    marked `inline`, and uses an asm block (~24 paired-single ops).
  - Both our and target compilation use same flags: `-O4,p
    -inline auto -inline deferred`. Same compiler version (MWCC 1.2.5).
  - Other TUs (MapObjFlag, MapObjFence) also call `gekko_ps_copy12`
    as a function in target — pattern is consistent.
  - Tried in this TU: `#pragma inline_max_size 0` at TU-top — no
    effect; gekko_ps_copy12 still inlines.
  - Suspect causes (untested): (a) reference / address-of in target
    forces emission; (b) some other TU in the build references it
    non-inline and the linker pulls a deferred version; (c) the
    `register` keyword on params + `asm` block plays differently with
    `-inline deferred` than expected; (d) total TU code-size threshold
    in `-inline deferred` heuristic.
  - Next experiment ideas: try `void (*p)(void*, void*) =
    &JGeometry::gekko_ps_copy12;` somewhere to force address-of; try
    `#pragma inline_depth(0)` (different from inline_max_size); try
    defining a TU-local non-inline wrapper that calls the inline (but
    that creates a new symbol — won't match unless target also has one).
- **Vtables in target asm get per-vtable @ha/@l individual relocations;
  ours coalesces multiple vtables under one `...data.0@ha` base + offset.**
  Symptom: dtors at 86-89% diff at the vtable-chain unwind:
  target `lis r3, __vt__TDerived@ha; addi r3, r3, __vt__TDerived@l; stw r3, 0(this); addi r0, r3, 0x24; stw r0, 0x20(this); ... lis r3, __vt__TParent@ha; ...`
  ours: `lis r3, ...data.0@ha; addi r3, r3, ...data.0@l; addi r0, r3, 0x714; stw r0, 0(this); addi r0, r3, 0x738; ...; addi r0, r3, 0x10; ...`
  Both objects have `.obj __vt__TFence, global` etc. as separate symbol
  declarations. The difference is the relocation TYPE chosen by the
  compiler: target uses `R_PPC_ADDR16_HA` + `R_PPC_ADDR16_LO` per-vtable;
  ours uses section-relative `...data.0@ha`. Also target's vtables are
  ordered RailFence, WaterH, Water, Inner, Outer, TFence in `.data`;
  ours: TFence, Outer, Inner, Water, WaterH, RailFence. Affected: 4
  MapObjFence dtors at 86-89%, likely affects MANY other dtor-heavy
  TUs. Suspect cause: section attribute (each vtable should go to its
  own COMDAT-style group). Lever search: tried no source-level fixes
  yet. Worth investigating whether a per-class flag, a `__weak__`
  attribute, or a specific class structure forces separate sections.
- **`MsWrap<f>__Ffff` template instantiation inlined despite all defeat
  attempts.** Symptom: target asm `.fn "MsWrap<f>__Ffff", local` with
  4 callers via `bl`. Our build inlines the body at each call site,
  no out-of-line emission. Tried: (a) `#pragma dont_inline on/off`
  around caller (controlWall); (b) `#pragma dont_inline on/off` around
  explicit specialization `template <> f32 MsWrap<f32>(...)` at TU
  scope; (c) calling with explicit template arg `MsWrap<f32>(...)`.
  All ineffective — MWCC continues to inline. Hypothesis: the explicit
  specialization is dead-code-stripped because no external reference
  forces emission. Possible fix to try: `static MsWrap_fn dummy = &MsWrap<f32>;`
  taking the address forces emission. Affected: `MoveBG/MapObjFence::controlWall`
  (loses ~5pp from 4 inlined calls). Other TUs that use MsWrap inline
  it too (e.g. `Enemy/enemy.cpp::moveTo*`) and DO get the weak-emit
  pattern — so something about the *caller* context controls it.

- **`JGeometry::gekko_ps_copy12` inlined in our build but called via `bl` in
  target.** Symptom: target asm has `bl gekko_ps_copy12__9JGeometryFPvPv`,
  ours emits 12 `psq_l`/`psq_st` instructions at the call site. Confirmed
  on `MoveBG/MapObjFlag::draw` and `MoveBG/MapObjBall::calcCurrentMtx`;
  asm files for MapObjBall/Block/Cloud/Fence/Flag/Monte/Pinna/RailBlock/Tree
  all contain `bl` calls; MapObjBlock emits the weak symbol. The function
  is `inline` in `include/JSystem/JGeometry/JGMatrix34.hpp` and uses an
  `asm {}` block with `register f32` locals. Our build inlines despite
  `-inline deferred` flag for the `MoveBG` lib. Tried (all no effect):
  (a) `#pragma inline_max_size(0)` + `inline_max_total_size(0)` before
  draw definition; (b) wrapping arg as `j3dSys.getViewMtx()` to add an
  inline forwarder; (c) explicit `(void*)` casts at call site. Hypotheses:
  (1) MWCC under `-inline deferred` decides per-TU based on something
  about parse order that we're not replicating; (2) a specific local-decl
  ordering before the call inhibits inlining; (3) the original source uses
  a non-inline wrapper we haven't found. Affected: `MoveBG/MapObjFlag::draw`
  at 71.82%, `MoveBG/MapObjBall::calcCurrentMtx` at 62.32%. Risk: if the
  lever is found, it could lift 8+ TUs simultaneously.
- **MAnmSoundNPC::startAnimSound: `lwzx` indexed-load vs target's
  `add+lwz`.** Target loads v as `add r3, r31, r5; lwz r3, 0x18(r3)`
  (mData held in NV r31, regular lwz at constant offset 0x18). Ours
  emits `slwi r3, r0, 5; addi r0, r3, 0x18; lwzx r5, r4, r0` (mData
  in temp r4, indexed lwzx). Tried: (a) `u8* base = (u8*)mData +
  mDataCounter*0x20; u32 v = *(u32*)(base+0x18)` — MWCC still picks
  lwzx; (b) `u8 (*entries)[0x20] = ...; *(u32*)(entries[idx]+0x18)`
  — also still lwzx. Both forms get re-merged. The lwzx pattern
  prevents mData from being NV-promoted to r31 (target saves r26..r31,
  6 regs; ours saves r27..r31, 5 regs). 70.3% → 72.9% after b2/b3
  mask-test rewrite (the remaining 27% gap is mostly this + cascading
  reg-coloring downstream).
- **MAnmSound::startAnimSound switch fall-through body order.** Target
  asm orders switch case bodies as: `case 0; case 7; default`. Target's
  case 0 body ends with `bne default_label; b end_label` — it's an
  explicit goto-to-default, not a fall-through. With C++98 + no-goto
  constraint, this can't be reproduced exactly. Tried in tick 102:
  duplicating the default body inline in case 0 (instead of fall-through
  to default) — this produces order `case 0 + duplicate-body; case 7;
  default` (78.9% → 85.5%). MWCC does NOT tail-merge the duplicate bl,
  so case 0 emits its own `addi r3, r28; addi r4, r29; ... bl
  startSoundActorInner` instead of jumping to default. Last ~14% gap
  is this duplication + cascading frame-size delta. Currently-hard.
- **MWCC per-call-site inline decision is *not* TU-wide.** Symptom: in
  `Enemy/BossHanachanSub::moveHead`, target inlines `TVec3::add` and
  `TVec3::scale` in loop1 (`mPos.y += g; mPos.add(mVel)`) and the first
  half of loop2's `dir.scale()`, but emits `bl sub`/`bl add`/`bl scale`
  for the same TVec3 helpers in loop2's `diff = mPos - mPrev`, loop2's
  `newPos.add(dir)`, and loop3's `(mPos-mPrev)*m08`. Our build inlines
  ALL of them, losing the bl-with-stack-temp pattern target uses. The
  same TU's `setDegreeZAndRevisionPosXZ` also has `bl sub` for its one
  `delta.sub(p.mPos)`. No `#pragma dont_inline` in source. Other TUs
  (cameralib, bossgesso, amiNoko) call out-of-line `sub` without any
  pragma either. Hypotheses worth testing: (a) MWCC's inline-cost
  budget is *per function* and gets exhausted at later call sites once
  enough size has been emitted (loop1 came first, was cheap → inlined;
  loop2/3 added bl's because the budget was used up); (b) register
  pressure at the call site shifts the budget calculation; (c) the
  decision is influenced by what's on the live-range list at that
  point. A clean experiment: take a single function with two identical
  `vec.add(other)` calls; insert dummy bloat between them; check
  whether the second flips from inline → bl. Affected TUs:
  `Enemy/BossHanachanSub::moveHead` at 61.74%, `setDegreeZ` at 79.59%
  (tick 100).
- **Dead-code preservation in `Hino2Landing::execute` (`fctiwz; stfd;
  lwz` without consumer).** Symptom: target asm contains the canonical
  `lfs; fctiwz; stfd; lwz` f32→s32 conversion sequence with the loaded
  int going into `r0` which is immediately overwritten by the next
  unrelated `lwz` — i.e. the conversion result is dead but emitted.
  Same with an inline `checkLiveFlag(LIVE_FLAG_HIDDEN)` expansion
  (`lwz; rlwinm.`) whose flag result is unused. Our source has bare
  `self->getMActor()->getFrameCtrl(0)->getFrame();` and
  `self->checkLiveFlag(LIVE_FLAG_HIDDEN);` (called but result
  discarded); our build DCE's them. Adding `(s32)` cast alone is
  insufficient. Adding `volatile s32 frame = (s32)...; (void)frame;`
  emits both ops AND match size goes from 168 → 192 (matching target),
  but volatile forces an extra `stw r0, 0x2c(r1)` write-back that
  target doesn't have. What source pattern produces the in-register
  conversion *without* a write-back? Likely candidates: register-only
  storage class (not supported in MWCC C++), an inline empty function
  taking the value by value, or a missing `JUT_ASSERT_F` whose body got
  optimized but the eval kept. Affected TU: `Enemy/hinokuri2`
  `execute__TNerveHino2LandingC...` at 87.40% (tick 100).
- **Multi-stream-register-copy in stream-read loops.** Symptom: target's
  `load__17TMarioPositionObjFR20JSUMemoryInputStream` copies the stream
  parameter `r4` into SIX callee-saved registers (r24..r29 all = r4)
  before entering its read loop, then dispatches the 10 per-iteration
  `stream.read(...)` calls across all six registers in a non-obvious
  pattern (24,24,26,29,24,25,28,24,24,27). Our build with plain
  `stream.read(...)` calls keeps `stream` in a single register
  (matching JDRActor::load, TCubeStreamInfo::load which are non-looped
  and match cleanly). Hypotheses to test: (a) the source had an
  inlined `TVec3<f32>::read(JSUInputStream&)` helper that rebound
  `stream` to a fresh reference at each expansion, forcing the
  allocator to spill into fresh callee-saved regs; (b) chained
  `operator>>(JSUInputStream&, JGeometry::TVec3<f32>&)` returning
  fresh refs at each link; (c) some macro that captured stream
  into a local. A t423 probe with a `static inline readVec(stream, vec)`
  helper containing two local stream references **did not confirm** the
  helper theory: it improved the base `this`/stream register choices but
  introduced extra vector-address temporaries, still coalesced the final
  dummy stream, and regressed `load` from 95.6% to 92.5%. Need a different
  minimal test case, likely for operator-chaining or macro capture rather
  than a normal helper. A 2026-06-06 `mario/Map/MapWarp`
  `TMapWarp::init` probe with explicit stream reference aliases for the y/z
  coordinate reads and selected dummy reads partially confirmed that aliases can
  force additional callee-saved stream copies: with corrected local-array stack
  order, `init` moved 93.8% -> 95.7% and emitted four stream copies instead of
  one. It still missed one target copy, used different registers, and the
  explicit duplicate aliases were reverted as not source-natural. Affected TUs:
  `Player/MarioPositionObj` load at 95.6% (updated t423),
  `Map/MapWarp::init` at 93.8% — see `state/notes/MarioPositionObj.md` and
  `state/notes/MapWarp.md`.
- **Dead-but-kept TMatrix34 stack stores under MWCC -O4,p.** Symptom: target
  `watch__26TDolpicEventRiccoMammaGateFv` (and equivalent code shapes in other
  TUs) emits 12 stfs stores at `r1+0x60..+0x8C` initialising a `TMatrix34`
  identity + `mMtx[1][1] = scale`, but no subsequent read or pass-by-reference
  occurs — the next instruction is `lwz r3, 0x24(r31); ... vtbl[6] = setUp()`,
  which takes no args. The matrix is purely dead from source view. Yet MWCC
  emits the stores. Our build of the same source pattern DCE's the stores
  entirely. What forces MWCC to keep them? Theories worth testing: (1) the
  matrix variable was the `this` of an inlined helper whose body got DCE'd
  but the prologue/epilogue ctor stores survived (e.g. `auto coll = ...; (void)
  coll;` with `coll`'s ctor non-trivial); (2) `volatile` somewhere in the
  matrix's source path; (3) the matrix was passed-by-reference to an inline
  empty body and the compiler kept the prep stores. Affected TU:
  `mario/Map/MapEventDolpic` watch__Ricco at 77.55% (filed tick 71).
- **Local-static `$localstatic0$<fn>` mangling on a non-inlined function.**
  Symptom: target's `calcTowerCenterPos___15CPolarSubCameraFP3Vec`
  is a 296-byte OUT-OF-LINE function (not inlined into its sole
  caller) yet its local static is mangled with the
  `<varname>$localstatic0$<funcname>` form — the same suffix used
  by symbols inside *successfully-inlined* functions like
  `drawJetCoasterBalloonMessage_`'s `sFixCameraPos`. In our build,
  the same source produces `<varname>$<counter>` (e.g.
  `sPositionNameTable$562`) for a non-inlined function. Adding
  `inline` to the declaration **does** flip mangling to
  `$localstatic0$<fn>`, but it ALSO causes MWCC to fully inline the
  function into its caller — losing the out-of-line copy and
  regressing the caller's match by ~20pp. `#pragma dont_inline on`
  around the definition does not prevent the inlining when `inline`
  is on the declaration. So we have a forced choice between
  *correct symbol mangling* and *correct codegen layout*. What
  source pattern in the target preserves both? Candidates worth
  trying: `__inline` (MWCC-extension keyword?), inline-then-extern-template,
  defining within an anonymous namespace, or some `-inline` flag we
  haven't tested. Affected TUs: CameraNormal (calcTowerCenterPos +
  caller ctrlNormalOrTowerCamera_, ~20-byte rodata/data mismatch).
- **`TTimeRec::startTimer` callers gain +16 bytes of stack frame.** Symptom: any
  function that calls `startTimer__9TTimeRecFv` (or `endTimer`) inflates its frame by
  exactly 0x10. The offending bytes don't correspond to any local in the source. Is
  this an inline-expansion artifact in MWCC's frame allocator? Does the inflation
  disappear if `startTimer` is reconstructed as a non-inline call?
- **`addi rN, rM, 0` vs `mr rN, rM`.** Two ways to encode the same operation; the
  target picks one, our build often picks the other. What source-level cue drives the
  choice? Is it register-class hinting? Lifetime analysis? Has anyone reproduced the
  flip by changing variable order alone?
- **f30/f31 register swap.** In some floating-point-heavy functions, target and ours
  agree on every instruction but the two callee-saved FPRs are swapped. Likely an
  interaction between expression order, live-range overlap, and MWCC's allocator
  preferences — but the exact lever is unknown.
- **Redundant field reloads under inline expansion.** When an inline accesses a field
  twice, MWCC sometimes emits two loads where common-subexpression elimination should
  have collapsed them. What inhibits CSE here? Is it the inline's parameter aliasing
  assumption? A `this`-pointer barrier?
- **Block ordering in boolean-return functions.** Compiler picks fall-through-through-true
  vs fall-through-through-false. The choice may correlate with branch-prediction hints,
  expression nesting, or the position of the `return` in the source AST. Reduce to a
  rule by varying source structure on a minimal test case.

- **Large functions stop inlining cheap helpers (`dot`, `sqrt`) that inline
  fine elsewhere (t201, Kazekun/flyAroundMario).** In the 916-byte
  `flyAroundMario`, the target emits `bl dot__...TVec3` and
  `bl sqrt__...TUtil<f>` for `TUtil<f32>::sqrt(dir.dot(dir))`, while our build
  inlines both (the squared() fmuls/fadds + the frsqrte NR sqrt expand inline).
  Both are normally trivially-inlined header templates — and the SAME TU inlines
  squared() inside the smaller `SMS_CalcToDirMatrix` (matched 91%). So the
  un-inlining is *function-size/inline-budget* driven, not a per-helper
  property. Combined with the vector-temp frame inflation
  ([[project_quat_cascade_0pct]] / the `TQuat4::rotate` +0x20 open question and
  the frame-UNDER-allocation entry), this drops the whole function to 0% objdiff
  despite the opening ~40 instrs being byte-exact. Question: is there a
  source-level lever to force `dot`/`sqrt` out-of-line at a single call site in
  a large fn (a `dont_inline`-stubbed wrapper? a named helper?), and does the
  frame then collapse to the target's 0x140? Experiment: on flyAroundMario,
  replace `TUtil<f32>::sqrt(dir.dot(dir))` with calls through `dont_inline`
  forwarders and re-diff; if the frame shrinks and alignment jumps, the lever is
  inline-budget control, not expression order.

## Refuted / wrong turns

### `#line` does not force MWCC local-class `$line` mangling

**Symptom (`mario/MarioUtil/ShadowUtil`).** Target owns local `TGDLStatic`
subclasses named from original source lines, e.g.
`TMBindShadowManager::TCylinder$2171ShadowUtil_cpp` and
`TSetup1$2172ShadowUtil_cpp`.

**Tried & REFUTED:** wrapping local class declarations in
`#line 2171 "ShadowUtil.cpp"` / `#line 2190 "ShadowUtil.cpp"` compiled, but
MWCC still emitted class names from the physical current source lines
(`TCylinder$625ShadowUtil_cpp`, `TSetup1$626ShadowUtil_cpp`, etc.). The target
`$2171` / `$2172` helpers remained missing and the probe only added extras.

**Conclusion.** Do not use `#line` as a local-class symbol-name lever. Recover
these helpers by placing the local class declarations at matching physical
lines or by accepting nonmatching helper names until source layout is ready.

### Explicit specialization declarations do not suppress local JGadget list iterator helper ownership in `MarDirectorInitECT`

**Symptom.** `mario/System/MarDirectorInitECT` matches the target list-insert
call shape but still locally emits extra 12B helpers for
`JGadget::TList<void*>::end()`,
`JGadget::TList<void*>::iterator` copy construction, and
`JGadget::TList_pointer<JDrama::TViewObj*>::iterator(Base::iterator)`.

**Tried & REFUTED:** adding TU-scope explicit specialization declarations for
those three class-template member/constructor helpers compiled cleanly but was
byte-identical: helper extras remained, and `setupPerformList_console`,
`initECDisp`, and `initECTGft` scores did not move.

**Conclusion.** The free-function template specialization routing rule does
not mechanically apply to these JGadget member/constructor helpers. Future
work needs a different owner-routing mechanism or a naturally matching list
owner to copy; do not retry declaration-only specializations for this cluster.

### MWCC rejects explicit specialization declarations for `TVec3<f32>::set<f32>`

**Symptom (`mario/Animal/AnimalBase`).** Target owns a local 16B
`set<f>__Q29JGeometry8TVec3<f>Ffff` body and calls it from the late
`TAnimalBase::execWalk` quaternion-rotate block; current source inlines
`rDest.set(q2.x, q2.y, q2.z)` from `TQuat4::rotate` and leaves the local owner
missing.

**Tried & REFUTED:** adding a TU-scope declaration
`namespace JGeometry { template <> void TVec3<f32>::set<f32>(f32, f32, f32); }`
does not compile under MWCC 1.2.5 (`unimplemented C++ feature`). This is not an
available call-boundary lever for the member-template `set` overload.

**Conclusion.** Future `TVec3::set<f32>` owner experiments need a different
source shape, such as a caller-side rotate wrapper or inline-budget lever; do
not retry the explicit member-template specialization declaration.

### An explicit specialization declaration does not force tiny `MsClamp<f32>` local-owner emission

**Symptom (`mario/Animal/AnimalBase`).** Target owns a local 32B
`MsClamp<f>__Ffff` body and calls it from `TAnimalBase::execWalk`; direct
`MsClamp<f32>` source inlines the compare/select body and leaves the local
owner missing.

**Tried & REFUTED:** adding a TU-scope
`template <> f32 MsClamp<f32>(f32, f32, f32);` declaration before the call
compiled cleanly but produced byte-identical output: `execWalk` stayed 58.5%
after the `MsWrap` wrapper fix, and `MsClamp<f>` remained missing. A tiny
file-local `callMsClamp` wrapper also inlined completely and did not emit the
target owner.

**Conclusion.** The `MsWrap` static-wrapper rule does not mechanically apply to
very small branch-only templates. Future `MsClamp` experiments need a different
inline-budget or call-boundary lever; do not retry the explicit specialization
declaration or a trivial wrapper alone.

### File-scope or inline-static arrays do not replace dummy-local anonymous rodata in `JKRCompArchive`

**Symptom (`mario/JSystem/JKernel/JKRCompArchive`).** The target TU owns three
anonymous rodata objects `@1210` (16B int table), `@1411` (12B float table),
and `@1431` (12B float table) but no standalone `dummy()` text. Current source
keeps those anonymous rodata symbols by defining the three automatic `const`
arrays inside an otherwise target-absent `static void dummy()`.

**Tried & REFUTED:** deleting `dummy()` removed the 4B extra text but also made
all three target rodata objects missing. Replacing it with file-scope
`static const` arrays kept the bytes but renamed them to `unknownTable1/2/3`
extras. Replacing it with an uncalled `inline static void dummy()` containing
function-local `static const` arrays similarly emitted
`unknownTable1$84/2$85/3$86` extras instead of `@1210/@1411/@1431`.

**Conclusion.** The emitted dummy is currently the least-wrong shape for this
TU because it preserves anonymous rodata ownership. Do not retry file-scope or
inline-static array variants here; the real fix needs a natural owner that
emits local automatic const-pool data without an out-of-line dummy body.

### Out-of-class redeclaration of `TUtil<f32>::sqrt` does not inhibit header-body inlining

**Symptom (t397, `mario/Player/MarioSound`
`TMario::soundTorocco`).** Target calls
`sqrt__Q29JGeometry8TUtil<f>Ff` after computing the squared distance, while the
current source's `distVec.length()` inlines the `frsqrte` Newton-Raphson body.

**Tried & REFUTED:** adding a TU-local out-of-class redeclaration
`namespace JGeometry { f32 TUtil<f32>::sqrt(f32); }` after including
`JGUtil.hpp` compiled cleanly but produced byte-identical output: `soundTorocco`
stayed 75.1% and still inlined the `frsqrte` body. A plain redeclaration of an
already-defined static member does not hide the inline class-body definition
from MWCC's call-site inline decision.

**Conclusion.** This is not a per-TU declaration-order lever. Future sqrt-call
experiments need a different call-site source shape or an inline-budget lever;
do not retry the out-of-class redeclaration alone.

### Explicit specialization declaration for `TRotation3<TMatrix33>::setRotate` is not a drop-in fix for `camerashake`

**Symptom (t380, `mario/Camera/camerashake`
`TCameraShake::execShake`).** Target calls the existing weak owner
`setRotate__Q29JGeometry64TRotation3<Q29JGeometry38TMatrix33<Q29JGeometry13SMatrix33C<f>>>FRCQ29JGeometry8TVec3<f>f`
from `Camera/cameralib`; current source keeps a TU-local `fakeSetRotate`
wrapper and emits an extra local 0x154-byte `setRotate` body, but preserves
`execShake` at 75.8%.

**Tried & REFUTED:** adding a TU-scope explicit specialization declaration for
`TRotation3<TMatrix33<SMatrix33C<f32> > >::setRotate` and calling
`rot.setRotate(axis, angleRad)` directly removed the 0x154-byte local
`setRotate` extra, but made `execShake` worse (75.8% -> 68.1%) and caused local
40-byte `TVec3<f32>::dot` / `scale` extras to appear. Additional out-of-class
declarations for those two `TVec3` members did not remove the extras or recover
the function shape.

**Follow-up (t381) also REFUTED:** wrapping `axis.normalize()` in a one-level
static inline helper improved `execShake` to 82.0% by restoring the target
`TVec3::dot` / `inv_sqrt` / `TVec3::scale` call sequence, but combining that
wrapper with the direct `rot.setRotate(axis, angleRad)` specialization still
regressed the function to 74.8%. It removed the local `setRotate` extra, but
MWCC expanded the rotation body at the call site and kept local `dot`/`scale`
extras.

**Conclusion.** The explicit-specialization declaration rule works for several
free/header template helpers, but this class-template member case is coupled to
MWCC's inline-budget decisions around `TVec3::normalize`. Do not apply the
specialization mechanically here; the remaining fix needs a per-call-site weak
owner lever that preserves `execShake`'s normalize and frame shape together.

### Source-level levers do NOT force MWCC to hoist/materialize a 32-bit compare constant into a callee-saved reg (the `0x80000001` mario-type check)

**Symptom (t301, `Animal/fishoid::checkHitActors`).** Target materializes the
literal `0x80000001` once before the loop into a callee-saved reg
(`lis r3,0x8000; addi r31,r3,1`) and compares register-register with `cmpw r0,r31`
each iteration — using a 4th NV reg (r28..r31) and shifting `this` into r28. Our
build instead folds the compare to the unsigned-wrap peephole
`addis r0,r3,0x8000; cmplwi r0,1` per iteration (only 3 NV regs, `this` in r29),
and emits a single `bne` instead of the target's `beq inside; b skip`.

**Tried & REFUTED:**
1. `(s32)field == (s32)0x80000001` (force signed compare) — no change; MWCC still
   applies the unsigned-wrap `addis/cmplwi` trick for equality regardless of cast.
2. `u32 marioType = 0x80000001; ... == marioType` (local variable) — no change;
   MWCC constant-propagates the local back into the same `addis/cmplwi` fold.

**Conclusion.** The folded `addis/cmplwi` form is MWCC's chosen lowering for an
equality compare against a constant near 0x80000000 whenever the constant is
known at the fold site — and it's load-invariant-but-NOT-hoisted. Whether the
target hoists+materializes instead is a register-allocation/LICM decision driven
by the surrounding loop's register pressure, not by a source-level expression of
the constant. (Note the matched `gesso::sendMessage` also uses the `addis/cmplwi`
fold — but via the bool-returning `isActorType` path with index iteration; the
fishoid target uses direct pointer iteration with the materialized form.) No
source lever found. Leave checkHitActors at 48%; this is the constant-hoist
member of the currently-hard register-coloring family.

### Prepending `isPool() ||` to the shared `TBGCheckData::isWaterSurface()` header inline is NET NEGATIVE

**Hypothesis tested (t281).** `TGorogoro::forceKill` (igaiga) showed the target
evaluating its water guard as TWO materialized sub-predicates: `isPool()`
({0x104,0x105,0x4104}) first, then the full 7-type flat chain. The natural read
was that the original `isWaterSurface()` *body* began with `isPool() ||`. Changed
the header (`include/Map/MapData.hpp:112`) to prepend `isPool() ||` and full-rebuilt.

**Result: REFUTED.** Project fuzzy 77.00741 → 76.96908 (**-0.038pp**),
matched_functions 8340 → 8334 (**-6 fns**). The flat form is correct at the ~15
other call sites; only forceKill wanted the isPool-first shape.

**Right answer:** the `isPool()` materialization is a **call-site composition**,
not a header property. Writing the guard as `(mGroundPlane->isPool() ||
mGroundPlane->isWaterSurface())` at the forceKill site reproduced the
materialized pair and took forceKill 73.8% → 82.7% (→94.2% after the separate
nerve-transition fix) with zero collateral. **Lesson: never edit a shared
predicate header to match one site — compose the extra term at the call site and
verify net with a full report.json diff.** See [[the spine kill/forceKill
nerve-transition note]] and `state/notes/igaiga.md`.

### `JGeometry::TUtil<f32>::sqrt` out-of-header sweep is NET NEGATIVE at project level

**Hypothesis tested (t174).** Mirror the inv_sqrt fix described in
*Hypotheses under investigation*: move sqrt body from JGUtil.hpp into
src/MarioUtil/RumbleMgr.cpp (the existing weak owner) and replace
header definition with a forward declaration. Expectation: 71 target
TUs with `bl sqrt__Q29JGeometry8TUtil<f>Ff` would gain.

**Result: REFUTED.** Project regressed **-0.1046pp**
(68.8276 → 68.7230). 11 TUs gained (biggest: ModelGate +10.12pp,
BathtubBinder +9.28pp, emario +2.98pp), but 22 TUs lost (biggest:
BathWaterManager -7.07pp, feetinv -1.83pp, bossgesso -1.74pp,
MarioPhysics -1.61pp, coasterkiller -1.27pp, fireWanwan -1.18pp,
bossManta -1.17pp, namekuri -1.12pp).

**Why it backfired.** Target's sqrt usage is **mixed per call site**:
some TUs got the inline body, others got `bl sqrt`. Forcing
project-wide `bl` resolves the bl-side TUs but breaks the inline-side
ones. This is the "MIXED inline+bl per site" risk anticipated in
[[feedback_move_inline_body_to_cpp]] and in the parallel inv_sqrt
hypothesis above.

**Implication.** A pure `inline keyword override of dont_inline`
explanation is incomplete: MWCC's `-inline auto` makes per-call-site
inline decisions even for header-defined functions, and target's
match% depends on the resulting mix. Cross-TU symbol movement is the
WRONG axis to optimize — we need a per-call-site lever (volatile
intermediate? helper-with-side-effect? source-level "this call may
have side effects"? distinct typedef? unknown.).

**Note for the inv_sqrt hypothesis above.** That fix is ALREADY
applied (body in `src/JSystem/JParticle/JPAEmitter.cpp`), and target's
inv_sqrt usage is documented as uniformly `bl`. If the inv_sqrt
distribution were also mixed, that fix would have produced a similar
regression. Verify before promoting any "move out of header" rule to
Settled. The current Settled state of inv_sqrt is uncited; treat with
caution.

**Don't re-run** this exact experiment without a new lever idea.
Reverted in t174.

---

---

## Conventions for entries

- **Lead with the rule, not the story.** "MWCC does X when Y" is more useful than "I
  spent 30 min on Z and figured out X". Save the story for the journal.
- **Cite specific symbols.** `SMS_IsMarioOnWire__Fv`, `flip__8TTimeRecFv`, etc. —
  fully mangled names so anyone (human or future agent) can grep the codebase.
- **Quantify when you can.** "+16 bytes of phantom stack inflation" beats "extra
  stack". "lwz then lwz at the same offset" beats "two loads".
- **Distinguish confirmed from suspected.** In _Settled_, only put things observed
  in two or more places. In _Hypotheses_, lead with "Hypothesis:".
- **Link related entries.** When extending an entry, cross-reference rather than
  duplicate. Use `(see also: <heading>)`.
- **Date entries are not needed** — git history serves that purpose. The structural
  order (newest first within each section) gives readers the sense of "what's been
  on the bot's mind recently".
