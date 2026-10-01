# Tools

The build downloads its pinned toolchain into `build/`. The scripts in this
directory are project-authored helpers and can be run from the repository root.

## Common helpers

- `decomp-diff.py`: list unit symbols and display instruction-level diffs.
- `check-diff-noise.py`: classify residual instruction differences; use its
  output as evidence, not as a substitute for behavioral review.
- `decompctx.py`: generate source context for external decompilation tools.
- `download_tool.py`, `project.py`, `ninja_syntax.py`, and
  `transform_dep.py`: build-system support used by `configure.py`.

Run a helper with `--help` for its full command-line interface.

## Investigation helpers

For a frozen running Sunshine game on Windows, double-click
`capture-freeze.cmd` or run `python tools/capture-freeze.py`. This read-only helper
saves symbolized thread stacks and raw memory without rebuilding the game.
See [Bianco freeze diagnostics](../docs/BIANCO_FREEZE.md) for usage and findings.

`tools/agent/` contains optional scripts for ranking candidates, examining
symbols, and testing recurring MWCC code-generation patterns. They are not
required for a normal build.

For example, `audit_candidates.py` ranks `NonMatching` units from
`build/GMSJ01/report.json`:

```sh
python tools/agent/audit_candidates.py --min-pct 85 --check-missing
```

Audit annotations are optional. Pass `--state-root PATH` or set
`GRAFFITO_STATE` when they live outside the repository.

`find_structural_near_match.py` checks the highest-ranked near-matching
functions with the noise classifier and lists only candidates that still have
structural rows:

```sh
python tools/agent/find_structural_near_match.py --min-pct 95 --prefix mario/MoveBG
```

Use it only for target selection; read the complete function diff before
editing.

`tvec3_copy_sweep.py UNIT SRC [--apply]` tries, one site at a time, rewriting
`TVec3<f32> v = src;` as `TVec3<f32> v; v = src;` (assignment copy keeps the
local address-taken, so later squares stay unfused like retail). It rebuilds the
object and keeps a rewrite only when the containing function's match rises.
Check the full report afterwards, since only the containing function is scored:

```sh
python tools/agent/tvec3_copy_sweep.py mario/MoveBG/MapObjBall src/MoveBG/MapObjBall.cpp --apply
```

`report_delta.py BASELINE CURRENT` compares two canonical non-matching objdiff
JSON reports, showing overall and per-unit fuzzy, exact-code, function, and
data changes. It exits nonzero if overall exact code or functions decrease,
and rejects reports with different code/function populations or unit sets.
Function-level fuzzy regressions are always printed, including those hidden
by larger gains elsewhere in the same unit. Add `--functions` to also list
function gains. Fuzzy-only regressions do not change the exit status.
Generate fresh reports after the full non-matching build; this helper does
not build or replace the push gate.

`download_tool.py TOOL OUTPUT --tag TAG` downloads build dependencies. A GitHub
release request that returns HTTP 504 is retried once with `download=1` to avoid
a stale gateway response. Other errors and a failed retry remain build failures.

Relocation operands in objdiff JSON are placeholders, and their target IDs do
not index the exported symbol array. `decomp-diff.py` uses objdiff's resolved
instruction text for these rows (braces enclose the whole changed instruction).
`check-diff-noise.py` treats relocation argument difference flags as structural,
including changed constant values and callees. Run their regression tests with
`python3 -m unittest discover -s tools/tests`.

`tools/agent/nerve_order.py [src/...cpp]` prints TUs whose `DEFINE_NERVE` order
differs from the retail `instance$NNNN` numbering order (instances identified by
the nerve vtable store, so fully inlined `theNerve` bodies are covered). Parse
order sets the nerve-static bss offsets (`addi r5,rX,off` before
`__register_global_object`). `tools/agent/move_nerves.py <cpp> <TNerve...>`
moves those blocks to the end of the file in the given order (the usual fix for
`-inline deferred` TUs). Always measure: some TUs are byte-neutral or mixed.

`python3 tools/agent/read_dol_word.py ADDRESS [ADDRESS ...]` reads four-byte
words directly from the retail DOL by virtual address and prints the file
offset, raw bytes, unsigned integer, and float interpretations. Addresses may
use `0x` notation; `--dol PATH` selects another DOL. Use this to verify constants
before changing source values inferred from reconstructed objects. Check the
target object's relocations separately; this tool does not inspect ELF files.

`agent/const_value_diff.py UNIT [FUNCTION ...]` audits aligned anonymous
`.sdata2` loads using one structured objdiff invocation per unit. It compares
exact object bytes for float, double, and word loads (including colors), and
accepts function-name substring filters. Output is a candidate list, not proof:
read the full instruction diff and verify target relocations and raw retail DOL
values with `agent/read_dol_word.py` before editing source.

`decomp-diff.py --range START-END` selects target-object offsets. Current-only
insertions remain attached to the preceding target instruction (or the first
one for leading insertions), even when the two functions have different object
addresses. Printed insertion offsets still show their current-object address.
