# Manual Equivalence Audit Log

_A strict, hand-audited registry of translation units whose every instruction-level
difference from the original has been verified, line by line, as a **non-behavioral
compiler artifact** (or is byte-identical). This is the trustworthy source of truth for
"is this TU actually equivalent" — it deliberately supersedes the older
`.agent_verdicts/*.md` "equivalent" stamps, which were granted on hand-wave
("remaining differences are codegen-class") without an instruction-by-instruction
behavioral proof and in several cases flip-flopped between verdicts._

If a TU is listed here as `EQUIVALENT`, it means a human (or an agent under direct human
review) has looked at **every differing instruction** and shown it cannot change observable
program behavior. If a TU is not in this log, it has **not** been audited to this bar — its
`report.json` percentage and any old `.agent_verdicts` verdict say nothing about whether its
residual diff is behavioral.

---

## The bar

A TU is `EQUIVALENT` only if **all** of the following hold:

1. **Every** differing instruction is individually accounted for and classified as one of the
   allowed non-behavioral artifact classes:
   - scratch register numbering (GPR/FPR allocation — e.g. `r6` vs `r4`, f30/f31 swap),
   - dead-parameter-register reuse (ours reuses a dead incoming-arg reg the target reserves),
   - stack-frame size / local offsets (MWCC 1.2.5 stack-padding inflation),
   - instruction encoding choices (`addi rN,rN,0` vs `mr`, `subi` vs `addi` of a negative),
   - constant-pool / local-label numbering (`@1234` vs `dummy*`), rodata/sdata label naming
     and ordering **with identical values**,
   - weak inline/dtor bodies emitted from rogue includes that don't change `.text` behavior.
2. **Zero** differences in anything behavioral: memory **stores** (address, value, width,
   order), loads that feed control flow, branch structure / conditions, function calls (target,
   args, order), or any computed value. Same program state, same observable effects, every path.
3. The source contains **no fake-matching hacks**: no `_pad[N]` / char-array padding, no
   `(void)x` sinks, no `goto` control flow, no forced weak-symbol emission, no behavior-changing
   stores or `volatile` locals added purely to steer codegen.
4. The verdict cites the **specific** residual artifact(s) and, where a reusable mechanism was
   found, links to the `docs/MWCC.md` entry that explains it.

`MATCH` = byte-identical (100%), trivially passes the bar.

A TU that is close but has **any** unexplained or behavioral diff is **not** logged here. Record
it instead as in-progress in your notes; only promote it once it clears the bar.

---

## How to add an entry (living doc)

- Append a row to the summary table and a detail block below, newest-first within the detail
  section.
- Do the audit against the **live object**, not stale data: disassemble
  `build/GMSJ01/obj/<TU>.o` (target) and `build/GMSJ01/src/<TU>.o` (ours), diff per function,
  and walk every diff line.
- Re-audit (and update the date) if the source changes. An `EQUIVALENT` verdict is pinned to a
  source state; note the commit or describe the source if it drifts.
- Never downgrade silently — if a re-audit fails the bar, move the row out and say why.

---

## Summary

| TU | Function(s) | Status | Match% | Residual artifact (cause) | Audited |
|----|-------------|--------|--------|---------------------------|---------|
| `JSystem/JDrama/JDRFrmGXSet` | `perform` (`.text`) | EQUIVALENT | 96.76 | dead-`r4` reuse + stack padding | 2026-06-17 |
| `JSystem/JDrama/JDRFrmGXSet` | `__dt` dtor (`.text`) | MATCH | 100 | — | 2026-06-17 |
| `JSystem/JDrama/JDRFrmGXSet` | `.data` / weak bodies | EQUIVALENT* | 59 | **systemic** `TViewObj` COMDAT vtable over-emission (see SYS-1) | 2026-06-17 |
| `JSystem/JDrama/JDREfbCtrl` | 6 of 8 fns | MATCH | 100 | — (`setTexAttb`, `TEfbCtrlTex::perform` newly matched) | 2026-06-17 |
| `JSystem/JDrama/JDREfbCtrl` | `TEfbCtrlDisp::perform` | EQUIVALENT | 99.69 | stack padding +0x30 (unreferenced dead stack) | 2026-06-17 |
| `JSystem/JDrama/JDREfbCtrl` | `TEfbCtrl::perform` | EQUIVALENT | 95.43 | dead-param **saved**-reg reservation (dual of dead-`r4`) | 2026-06-17 |
| `GC2D/ScrnFader` | 15 of 16 fns | MATCH | 100 | — (`draw_wipe_box`, `load` newly matched) | 2026-06-17 |
| `GC2D/ScrnFader` | `update` | EQUIVALENT | 99.94 | inline-stack-padding +16 (body byte-identical) | 2026-06-17 |

`*` Correct **raw-compiler output** — the `.data` shortfall is a decomp-toolkit **split / COMDAT-fold
representation artifact** (our raw `.o` weak-emits a vtable the original *also* weak-emitted; the
linker folds it and dtk reassigns the survivor), **not a defect and not source-fixable**. See SYS-1.

---

## Details

### `JSystem/JDrama/JDRFrmGXSet` — 2026-06-17

Audited against `build/GMSJ01/obj/JSystem/JDrama/JDRFrmGXSet.o` vs
`build/GMSJ01/src/JSystem/JDrama/JDRFrmGXSet.o`, instruction by instruction.

**`__dt__Q26JDrama9TFrmGXSet` — MATCH.** Byte-identical.

**`perform__Q26JDrama9TFrmGXSetFUlPQ26JDrama9TGraphics` — EQUIVALENT (96.76%).**
The body is instruction-identical to target: same opcodes, same order, and — verified
explicitly — **every memory store matches** (`mFrameBuffer`@4, the GXRenderModeObj field
copies @8..0x44, the `unkFC` setBit `sth`s @0xFC, `mFBClamp`@0xF0, `mClearColor`@0xF4 via its
stack-temp materialization, `mClearZ`@0xF8), same branch structure for all 7 setBit gates.
Only two residual, provably non-behavioral diffs remain:

- **Stack frame size:** target `stwu r1,-0x130` (304) vs ours `-0x90` (144), and the dependent
  `beq`/epilogue/`getClearColor` temp offset. MWCC 1.2.5 stack-padding inflation. No behavior.
- **Scratch-register coloring:** the field-copy/setBit blocks use `{r4,r6,r7}` (ours) where
  target uses `{r6,r7,r8}` — ours **reuses the dead parameter register `r4`** as scratch; target
  leaves `r4` reserved. `param_1` is used exactly once (the entry gate) in both. This is a
  parameter-liveness register-allocation artifact: keeping `param_1` live flips ours to the exact
  target coloring but always costs one instruction the target lacks, so the target's reservation
  comes from a `param_1` use eliminated **after** allocation — not reproducible from C++ under
  `-inline auto`. Full mechanism + proof in `docs/MWCC.md` → "Dead-parameter scratch-register
  reservation".

**`.data` (59%) — EQUIVALENT by dedup, but flagged.** Our `JDRFrmGXSet.o` emits an **extra weak**
`__vt__Q26JDrama8TViewObj` (in `.data`, +0x24) and a weak `__dt__Q26JDrama8TViewObjFv` (in `.text`,
the source of the `.text` size delta too). The **target references `__vt__TViewObj` as UND** and
emits neither. Cause: `TViewObj`'s only own virtual is the **pure** `perform` (no out-of-line key
function), so its vtable is COMDAT/weak — our build emits it in every TU that materialises a
derived dtor; the original restricts emission to its single home, `JDRActor.o`. The linker dedups
weak symbols, so the final DOL is byte-correct and behaviour is identical — but the `.o` is not
byte-clean. This is **not a `JDRFrmGXSet` bug**: it is the same over-emission across dozens of
`TViewObj`-derived TUs. Tracked as **SYS-1** below. JDRFrmGXSet cannot be byte-clean in `.data`
until SYS-1 is resolved at the `TViewObj` level.

**Verdict basis:** `.text` has no store/branch/call/value difference on any path — residual is
strictly scratch-GPR numbering + frame padding. `.data` residual is a linker-deduped weak vtable
(non-behavioral). Behaviorally identical end-to-end. **Not** matched to 100% on purpose — the only
`.text` sub-100% "wins" available (a behavior-changing `param_2->unk0 = param_1` store, or a
`volatile` forcing local) are fake matches and were rejected; the `.data` gap is systemic (SYS-1).

---

### `JSystem/JDrama/JDREfbCtrl` — 2026-06-17

8 functions; **6 byte-identical** (`.text` 96.34% → 99.41%), 2 verified non-behavioral residuals.

**Newly matched (clean levers, no hacks):**
- `setTexAttb` (82.56% → **100%**): the `GXGetTexObjAll` width/height now route through a 2-word temp and are written to the adjacent `mWidth`(u32 @0x24)/`mHeight`(u32 @0x28) pair in one batched store via `*(Dims*)&mWidth = dims;` — the documented `*(T*)&dst` batched-store lever. This also **deleted the banned `char trash[0x10]` padding hack** that was there before.
- `TEfbCtrlTex::perform` (was already byte-identical; the report's raw diff was only the `.o` offset line).
- `TEfbCtrlDisp::perform` (92.83% → **99.69%**): `param_2->unkFC.get()` (single-read accessor) coalesced a redundant `lhz` reload, and `fbClamp`/`clearZ` intermediate locals fixed the trailing-arg eval order.

**Residual 1 — `TEfbCtrlDisp::perform` 99.69% EQUIVALENT (stack padding +0x30).** Instruction-identical except the frame is `-0x60` (target) vs `-0x30` (ours): 0x30 of fully-DCE'd, *never-referenced* stack (the materialization at offsets 72/76 is identical, just placed higher). **Verified not source-reconstructable** by trying the inline levers: by-value `GXRenderModeObj` local *overshoots* (+0x38) and isn't DCE-clean (the render mode is passed **by reference**, read twice — unlike `JDRFrmGXSet` where it's *assigned*); materializing the args adds no stack; materializing the inline `check()` results inflates but emits stores the target lacks. The dead space comes from the register allocator, not a source local. Behaviorally identical.

**Residual 2 — `TEfbCtrl::perform` 95.43% EQUIVALENT (dead-parameter saved-register reservation).** Target frame `-40` + saves an extra `r29` (`this` in r29), ours `-32` + 2 saved regs (`this` in r30). **2nd confirmed TU of the dead-parameter register pattern** (the *saved-reg dual* of `JDRFrmGXSet`'s scratch-reuse form): target keeps `param_1` live through allocation (reserving a GPR + 8 frame) then DCEs the use after allocation. A reference local (`TFlagT& flag = unk20;`) inflates the frame to the exact `-40` but reads `unk20` through a materialized `&unk20` reg where the target reads `32(r29)` directly — i.e. it adds a *different* artifact, not the target's. The only thing that reaches the exact allocation is a behavior-changing `param_2->unk0 = param_1` store (rejected as fake matching). See `docs/MWCC.md` → "Dead-parameter scratch-register reservation" (now 2 TUs — promote toward Settled).

---

### `GC2D/ScrnFader` — 2026-06-17

16 functions; **15 byte-identical** (`.text` ~100% via objdiff-cli), 1 residual.
- `draw_wipe_box` (93.77% → **100%**): replaced the 2 `w`/`h` float locals with the 4×-inline per-arg `JUTRect` construction form (4 `fctiwz`, frame 0x90) to match the target's instruction stream + frame.
- `load` (96.59% → **100%**): stack-slot layout via un-named `startFadein(stream.readS32())`, a `u32 local` for the color word, and per-shift `TColor((u8)(v>>24)...)`.
- `update` (99.66% → **99.94%** EQUIVALENT): body byte-identical (`updateRequest`/`updateFadeinout` fully inlined, kept `fVar1` in f2 via the minuend split); residual is +16 inline-stack-padding (the original frame has 16 unused bytes; not reproducible without a banned `_pad`). Left honest.

**Runtime bug found during this investigation (fixed in `src/System/Application.cpp`, NOT ScrnFader):**
`gameLoop`'s `C_MTXOrtho(m, top, bottom, left, right, …)` for the root fader had **`bottom` and `right`
swapped** — `bottom=fbWidth(640)`, `right=efbHeight(448)` — so the fade quad spanned a 640-tall ortho
in a 448-line viewport and reached only `448/640 ≈ 70%` down, leaving the bottom ~1/3 unfaded. Fixed to
`bottom=efbHeight, right=fbWidth`; **verified against the original `.o`** (orig loads `efbHeight`→f2/bottom,
`fbWidth`→f4/right) — so the fix also moves `gameLoop` toward 100%. The fader fill itself
(`perform`/`draw`/`fill_rect`) was already matched and innocent; the wrong constant lived in the caller.

---

## Systemic issues (cross-TU)

These are non-behavioral artifacts whose fix lives outside any single TU. They cap the `report.json`
percentage of many TUs at once, so they're worth a dedicated pass.

### SYS-1 — `TViewObj` abstract-base vtable is COMDAT-emitted in every derived TU (target homes it once in `JDRActor.o`)

**Symptom.** Target `*.o` for `TViewObj`-derived classes reference `__vt__Q26JDrama8TViewObj` as
**UND**; only `JDRActor.o` emits it (weak). Our build emits the weak vtable (and a weak
`__dt__TViewObj`) in *every* derived TU that emits a destructor — observed in `JDRFrmGXSet`, and
weak `__vt__TViewObj` shows up in `boid`, `cameragc`, `CubeManagerBase`, `lensflare`, `lensglow`,
`sunmgr`, `sunmodel`, `areacylinder`, `beam`, `bgpoldrop`, … (dozens). Inflates `.data` (and `.text`)
on all of them.

**Cause (confirmed).** `TViewObj` (`include/JSystem/JDrama/JDRViewObj.hpp`) declares only a **pure**
virtual `perform` plus an inline ctor and an *implicit* (inline) dtor — no non-inline virtual to act
as the vtable key function — so MWCC treats `__vt__TViewObj` as COMDAT and emits it in **126** TUs
(every one whose derived dtor materialises the base subobject). The target's `__dt__TFrmGXSet`
**inlines** `~TViewObj` (it sets `__vt__TViewObj` inline — `lis/addi/stw` — and calls
`__dt__TNameRef` directly), and **imports** `__vt__TViewObj` UND; the single weak copy lives in
`JDRActor.o` (pulled in by `TActor`'s out-of-line key functions).

**Fix attempted + MEASURED (2026-06-17) — REJECTED.** Gave `TViewObj` an out-of-line virtual dtor
(`virtual ~TViewObj();` + def in `JDRViewObj.cpp`), then full-rebuilt. Result, verified on disk:
- `.data` over-emission removed (JDRFrmGXSet `.data` 59% → 100%; weak-`__vt__TViewObj` count
  126 → ~1 as TUs rebuilt) — the *intended* effect.
- BUT it makes the dtor a non-inline key function, so **every derived dtor now CALLS
  `__dt__TViewObj` instead of inlining it** — diverging from the target, which inlines. Confirmed:
  our `__dt__TFrmGXSet` lost the target's inline `lis/addi/stw; bl __dt__TNameRef` and gained
  `bl __dt__TViewObj`. The dtor went from byte-identical (100%) to mismatched.
- It also homes the vtable **strong in `JDRViewObj.o`** vs the target's **weak in `JDRActor.o`**.
- Overall `fuzzy_match_percent` did **not** improve (61.44091 → 61.44091).

**ROOT CAUSE FOUND (2026-06-17, ultracode workflow + independently verified) — it is NOT a source
issue.** The reference objects in `build/GMSJ01/obj/` are **decomp-toolkit reconstructions split out
of the linked DOL**, not raw compiler output (each has a `.note.split` ELF section). The original
build, with this same MWCC 1.2.5, emitted the weak COMDAT `__vt__TViewObj` / `__dt__TViewObj` in
**every** derived-dtor TU — exactly like ours. The **linker folds** those duplicate weak COMDAT copies
to one in the final DOL; **decomp-toolkit then assigns the single survivor to `JDRActor.o` and rewrites
all other TUs' references to `*UND`**. Proof: (1) target `.o` carry `.note.split`; (2) target
`JDRLighting.o` *references* `__vt__TViewObj` as `*UND` while *using* it in relocations (its out-of-line
ctor stores the vtable address) — impossible for a raw per-TU compile, which would emit a weak copy;
(3) game-wide the vtable is weak-defined in **exactly 1** object and `*UND` in **115**, the
COMDAT-fold + dtk-survivor signature. Our baseline already homes it weak in `JDRActor.o`
byte-identically to the target, `JDRViewObj.o` is clean, and all derived dtors already inline
`~TViewObj`.

**Why no source change works (and the earlier attempt is strictly worse).** For a class whose only
own non-pure virtual is the destructor, the two requirements — (a) derived dtors *inline* `~TViewObj`
and (b) the vtable *imported UND* in non-owner `.o` — are coupled through dtor-body visibility in
**opposite** directions: any visible dtor body ⇒ derived inline it ✔ but the vtable is COMDAT-emitted
locally ✘; a decl-only dtor ⇒ vtable UND ✔ but derived **call** `__dt__TViewObj` (`.text` regression) ✘.
The target satisfies both only because its `.o` are post-link/split, not compiler output. The
"non-inline body" variant additionally emits **global** (ODR) vtables in non-owners — worse than weak.

**Impact / behavior.** None. The final DOL is byte-correct (the weak duplicates fold exactly as the
original's did). The `.data` shortfall on `TViewObj`-derived TUs is purely a **report.json/objdiff
representation artifact** of diffing our *raw* `.o` against *dtk-split* `.o`.

**Status:** RESOLVED — **not a source defect; do not "fix".** Our raw output is the faithful
maximally-correct compiler result. Any source change either regresses derived-dtor `.text` or
introduces ODR-global vtables. The only place this could ever be reconciled is in the *diff tooling*
(teach objdiff/report to fold COMDAT weak duplicates the way the linker does), not in the source —
tracked separately if ever worth it. Reopen only as a tooling task, never as a source change.
