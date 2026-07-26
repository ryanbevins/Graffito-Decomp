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
