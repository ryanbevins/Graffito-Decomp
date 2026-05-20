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

_(empty — populate as observations confirm)_

## Hypotheses under investigation

### `#pragma dont_inline` is TU-global, not lexical

**Hypothesis:** `#pragma dont_inline on/off` in MWCC 1.2.5 (GameCube) is applied
to every function in the TU using the **final** pragma state seen by the parser,
not lexically scoped around the function definition. Multiple positions of `on`/`off`
in the same file all produced the same compilation outcome — the state at
end-of-file is what stuck.

**Where observed:** `src/JSystem/JDrama/JDRDStageGroup.cpp`. Tried wrapping only
`perform()` in `#pragma dont_inline on ... off`. Either both functions saw `on`
(when the file ended with `on`) or both saw `off` (when the file ended with `off`
or `reset`). Could not pin one function to `on` and another to `off`.

**Experiment to confirm:** Reproduce in a second TU. Best candidate would be a TU
where two functions need different inlining decisions (one calls a large inline
that should *not* expand, the other has an auto-generated dtor that *should* inline
its base subobject dtors). If both follow the file-end pragma state, this is
confirmed.

**Consequence if true:** When a TU needs both a `dont_inline on` function and an
auto-generated dtor that inlines empty base dtors, **we cannot have both match**
through pragma alone. A different mechanism (e.g. `__noinline` attribute,
declaration trick, or moving inline source out of header) is required.

## Open questions

## Open questions

_Seeded from the "currently-hard patterns" list in `CLAUDE.md` — promote to *Hypotheses
under investigation* the moment you have a testable theory, and to *Settled* once
confirmed in ≥2 TUs._

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

## Refuted / wrong turns

_(empty — populate when a former hypothesis is disproved)_

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
