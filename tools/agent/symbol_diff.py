#!/usr/bin/env python3
# usage: python3 tools/agent/symbol_diff.py <unit_path_under_obj> <symbol>
#   example: python3 tools/agent/symbol_diff.py Player/MarioMove checkPlayerAround__6TMarioFif
# Prints a side-by-side instruction diff for a single symbol, drawn from the
# two object files under build/GMSJ01/obj/<unit>.o and build/GMSJ01/src/<unit>.o.
# Avoids the multi-MB JSON dump from objdiff-cli when invoking the whole TU.
import json
import os
import subprocess
import sys


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        sys.exit(2)
    unit = sys.argv[1]
    sym = sys.argv[2]
    target = f"build/GMSJ01/obj/{unit}.o"
    base = f"build/GMSJ01/src/{unit}.o"
    if not os.path.exists(target) or not os.path.exists(base):
        print(f"Missing: {target} or {base}", file=sys.stderr)
        sys.exit(1)
    out = subprocess.run(
        ["build/tools/objdiff-cli", "diff", "-1", target, "-2", base, sym,
         "--format", "json", "-o", "-"],
        capture_output=True, text=True)
    if out.returncode != 0:
        print(out.stderr, file=sys.stderr)
        sys.exit(1)
    d = json.loads(out.stdout)
    left_fn = find_sym(d.get("left", {}).get("symbols", []), sym)
    right_fn = find_sym(d.get("right", {}).get("symbols", []), sym)
    if not left_fn:
        print(f"Symbol not found in target (left): {sym}", file=sys.stderr)
        sys.exit(1)
    if not right_fn:
        print(f"Symbol not found in base (right): {sym}", file=sys.stderr)
        sys.exit(1)
    print(f"=== {sym} ===")
    print(f"left  (target): {left_fn.get('match_percent', 0):.2f}%  "
          f"{len(left_fn.get('instructions', []))} insns  size={left_fn.get('size')}")
    print(f"right (ours):   {right_fn.get('match_percent', 0):.2f}%  "
          f"{len(right_fn.get('instructions', []))} insns  size={right_fn.get('size')}")
    print()
    li = left_fn.get("instructions", [])
    ri = right_fn.get("instructions", [])
    n = max(len(li), len(ri))
    for i in range(n):
        l = li[i] if i < len(li) else None
        r = ri[i] if i < len(ri) else None
        ls = fmt_insn(l)
        rs = fmt_insn(r)
        marker = " "
        if ls.strip() != rs.strip():
            marker = "*"
        print(f"{marker} {ls:54s} | {rs}")


def find_sym(syms, name):
    for s in syms:
        if s.get("name") == name:
            return s
    return None


def fmt_insn(insn):
    if insn is None:
        return ""
    ins = insn.get("instruction", {}) or {}
    addr_raw = ins.get("address", 0)
    try:
        addr = int(addr_raw)
    except (TypeError, ValueError):
        addr = 0
    formatted = ins.get("formatted", "")
    return f"{addr:5x}: {formatted}"


if __name__ == "__main__":
    main()
