Verdict: equivalent

Date: 2026-06-13 9:44am MNL
Unit: `mario/JSystem/JKernel/JKRHeap`
Source: `src/JSystem/JKernel/JKRHeap.cpp`
Classification: `Object(Equivalent, "JSystem/JKernel/JKRHeap.cpp")`

Reason:
- Re-verified during the audit sweep. The overview has no missing target
  symbols. Target text/data symbols are exact or codegen-class equivalent.
- `freeAll()` and `dispose()` run the same disposer-list loop, reload the first
  link after each virtual dispose call, pass `-1`, and return once the list is
  empty. Remaining differences are stack-frame/local-slot size.
- `dispose_subroutine(unsigned long, unsigned long)` preserves the same range
  checks, virtual disposer call, first-link reload path, previous-link next
  path, and loop exits. The diff is frame/local-slot size only.
- `copyMemory(void*, void*, unsigned long)` preserves the same rounded word
  count, eight-word unrolled loop, tail loop, loads, stores, and pointer
  increments. The diff is register-copy spelling (`mr` versus `addi`) and
  register choice.
- Extra `JSUList`/`JSUTree`/`JSULink` destructor symbols are weak ownership
  residue and do not represent missing target behavior.
- Recheck at 7:10am: current full diffs for `freeAll`, `dispose_subroutine`,
  `dispose`, and `copyMemory` still show only frame/local-slot, `mr`/`addi`,
  and register-choice residue. No structural diffs or missing rows were found.
- Recheck at 9:44am: fresh full diffs for `freeAll`,
  `dispose_subroutine`, `dispose`, and `copyMemory` still preserve the same
  disposer-list iteration, virtual delete calls with `-1`, range checks,
  previous-link reload path, unrolled eight-word copy loop, tail loop, loads,
  stores, and pointer increments. Residue remains frame/local-slot,
  `mr`/`addi`, and register-choice drift. Source-link and normal hash proof
  passed again.

Proof:
- `python configure.py --non-matching && ninja` linked
  `build/GMSJ01/mario.dol` from source.
- `python configure.py && ninja` restored the matching config and passed
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-14 5:58pm MNL safety-net recheck: verdict remains `equivalent`.
Current overview has no missing target symbols. Fresh full diffs for
`freeAll`, `dispose_subroutine`, `dispose`, and `copyMemory` still preserve the
same disposer-list iteration, virtual destructor calls with `-1`, range
checks, reload/next-link behavior, rounded word count, eight-word unrolled copy
loop, tail loop, loads, stores, and pointer increments. Residue is frame/local
slot, `mr`/`addi`, and register-choice drift; this tick's source-link and
normal proof builds covered the current object.
