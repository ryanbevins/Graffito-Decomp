#!/usr/bin/env python3
# usage: python tools/agent/find_bool_narrow.py [min_fp] [max_fp] [max_size]
#
# Scans near-match functions for the "missing (u8) narrowing cast on a BOOL
# predicate result" signature: at the same aligned position the TARGET emits
#   clrlwi. r0, r3, 24   (an 8-bit narrow-then-test of a call result in r3)
# while OUR build emits
#   cmpwi r3, 0x0        (a full-width int test)
# right after a `bl <predicate>`. Adding `(u8)` (or changing the predicate's
# return type to bool) at that call site forces the narrowing and frequently
# also collapses an apparent "phantom-inline" frame-size inflation that is
# really a downstream effect of the wide-int test path.
#
# Prints: fp% size unit :: sym  (clrlwi-vs-cmpwi after bl <callee>)
import json
import os
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DIFF = os.path.join(ROOT, "tools", "agent", "diff_sym.py")


def diff_lines(unit, sym):
    try:
        out = subprocess.run(
            [sys.executable, DIFF, unit, sym],
            capture_output=True, text=True, cwd=ROOT, timeout=60,
        ).stdout
    except Exception:
        return []
    return out.splitlines()


def has_signature(lines):
    # Look for a line where target side has `clrlwi. r0, r3, 24` and our side
    # has `cmpwi r3, 0x0`, preceded (within a few lines) by a `bl`.
    last_bl = None
    for ln in lines:
        if " bl " in ln:
            m = re.search(r" bl (\S+)", ln)
            last_bl = m.group(1) if m else "?"
        # split on the | column separator
        if "|" not in ln:
            continue
        left, right = ln.split("|", 1)
        if "clrlwi. r0, r3, 24" in left and "cmpwi r3, 0x0" in right:
            return last_bl or "?"
    return None


def main():
    min_fp = float(sys.argv[1]) if len(sys.argv) > 1 else 90.0
    max_fp = float(sys.argv[2]) if len(sys.argv) > 2 else 99.5
    max_sz = int(sys.argv[3]) if len(sys.argv) > 3 else 400
    rep = json.load(open(os.path.join(ROOT, "build", "GMSJ01", "report.json")))
    cands = []
    for u in rep["units"]:
        for f in u.get("functions") or []:
            fp = f.get("fuzzy_match_percent", 0)
            sz = int(f.get("size", 0))
            if min_fp <= fp <= max_fp and 0 < sz <= max_sz:
                cands.append((fp, sz, u["name"], f["name"]))
    cands.sort(reverse=True)
    print(f"scanning {len(cands)} candidates ({min_fp}-{max_fp}%, <={max_sz}B)...",
          file=sys.stderr)
    hits = 0
    for fp, sz, unit, sym in cands:
        callee = has_signature(diff_lines(unit, sym))
        if callee:
            hits += 1
            print(f"{fp:6.2f} {sz:4d} {unit} :: {sym}  (after bl {callee})")
    print(f"done: {hits} hits", file=sys.stderr)


if __name__ == "__main__":
    main()
