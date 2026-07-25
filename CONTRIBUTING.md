# Contributing to Graffito

Graffito is a matching decompilation project. A useful contribution must be
grounded in the target binary and preserve runtime behavior; source that merely
looks plausible is not enough.

## Before you start

- Read `README.md`, `AGENTS.md`, and the relevant notes under `docs/`.
- Build the unmodified project once so your toolchain and game input are known
  to work.
- Keep changes focused on one translation unit or one clearly related issue.
- Do not submit game assets, retail binaries, generated assembly, build
  products, compiler downloads, diff dumps, or local agent/editor state.

## Matching standards

- Prefer ordinary, readable C or C++ that explains the target instructions.
- Do not use fake stack padding, `goto`-based control-flow shaping,
  behavior-changing stores, forced weak-symbol emission, or other codegen-only
  hacks.
- Do not mark a translation unit `Matching` unless the relevant output is
  byte-identical.
- Do not mark a translation unit `Equivalent` without the instruction-level
  behavioral proof required by `docs/AUDIT.md`.
- If the evidence is ambiguous, leave the code non-matching and document what
  remains.

## Validation

At minimum, run:

```sh
python configure.py
ninja
```

For changes that affect functionally equivalent or source-linked units, also
run:

```sh
python configure.py --non-matching
ninja
```

Use `tools/decomp-diff.py` or objdiff to record the before/after result for the
affected translation unit. Run `git diff --check` before submitting.

Build success proves compilation and the configured DOL hash; it does not
replace runtime testing when a change affects behavior.

## Commits and pull requests

- Explain what evidence led to the source shape.
- Include the affected unit and symbol names.
- Report match-percentage changes accurately and call out any remaining
  differences.
- Separate unrelated cleanup or formatting from matching work.
- Never bypass repository checks with `--no-verify`, and never force-push over
  shared project history.

By contributing, you agree that your contribution is provided under the
repository's CC0 1.0 Universal dedication.
