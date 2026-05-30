#!/bin/bash
# usage: tools/agent/scan_getmodel.sh <EnemyTU> [<EnemyTU> ...]
#   e.g. tools/agent/scan_getmodel.sh popo Kukku bossgesso
#
# Read-only detector for the "getMActor()->getModel() inline vs bare getModel()
# out-of-line bl" matching lever (docs/MWCC.md, Hypotheses). For each nonmatching
# function in mario/Enemy/<TU>, flags it FIXABLE if our build emits a RIGHT-only
# `bl getModel__10TLiveActorCFv` (target inlined it) AND target has zero LEFT-only
# such bl. Requires a current build (objdiff reads build/GMSJ01).
#
# CAVEAT: a purely textual ^</^> grep MISSES a target `bl getModel` printed on a
# `|` (modified) line, producing false positives on heavily-misaligned functions
# (e.g. popo callbacks, whose real gap is the bool-return li 0/1;b materialize).
# Always eyeball the flagged function's diff for a LEFT `lwz rX,4(rX)` inline pair
# (mMActor->unk4) before applying the fix. See docs/MWCC.md for the full rule.
for tu in "$@"; do
  unit="mario/Enemy/$tu"
  python3 tools/decomp-diff.py -u "$unit" -t function -s nonmatching 2>/dev/null \
    | sed -n 's/^nonmatching *[0-9.]*% *[0-9]*B *\.text *//p' \
    | while IFS= read -r fn; do
      [ -z "$fn" ] && continue
      out=$(python3 tools/decomp-diff.py -u "$unit" -d "$fn" 2>/dev/null)
      rcnt=$(echo "$out" | grep -cE '^> .*bl getModel__10TLiveActorCFv')
      lcnt=$(echo "$out" | grep -cE '^< .*bl getModel__10TLiveActorCFv')
      if [ "$rcnt" -gt 0 ] && [ "$lcnt" -eq 0 ]; then
        pct=$(echo "$out" | head -1 | grep -oE '[0-9.]+%')
        echo "MAYBE-FIXABLE (verify by eye): $unit :: $fn ($pct) [R=$rcnt L=$lcnt]"
      fi
    done
done
