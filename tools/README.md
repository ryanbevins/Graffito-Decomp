# Tools

`tools/agent/audit_candidates.py` ranks `NonMatching` objects from
`build/GMSJ01/report.json` and annotates them with audit verdicts from
`/opt/graffito/state/audit`. Use it during AUDIT ticks to avoid reopening TUs
that already have `needs_impl` or `not_equivalent` blockers recorded.
