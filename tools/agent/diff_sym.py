#!/usr/bin/env python3
"""
Diff a function by mangled name, looking up by name in both objects
to bypass objdiff's target_symbol mismatch when symbol order differs
between target and our build.

Usage: python3 tools/agent/diff_sym.py <unit> <symbol>
       python3 tools/agent/diff_sym.py mario/GC2D/PauseMenu2 loadAfter__11TPauseMenu2Fv
"""
import json
import os
import subprocess
import sys

script_dir = os.path.dirname(os.path.realpath(__file__))
root_dir = os.path.abspath(os.path.join(script_dir, "..", ".."))
OBJDIFF = os.path.join(root_dir, "build", "tools", "objdiff-cli")


def main():
    if len(sys.argv) < 3:
        print(__doc__, file=sys.stderr)
        sys.exit(1)
    unit = sys.argv[1]
    sym_name = sys.argv[2]
    out = subprocess.check_output(
        [OBJDIFF, "diff", "-c", "functionRelocDiffs=data_value",
         "-u", unit, "-o", "-", "--format", "json"],
        cwd=root_dir,
    )
    d = json.loads(out)
    syms_l = d["left"]["symbols"]
    syms_r = d["right"]["symbols"]
    lt = next((s for s in syms_l if s.get("name") == sym_name), None)
    rt = next((s for s in syms_r if s.get("name") == sym_name), None)
    if lt is None:
        print(f"target missing: {sym_name}", file=sys.stderr)
        sys.exit(2)
    if rt is None:
        print(f"base missing: {sym_name}", file=sys.stderr)
        sys.exit(2)
    print(
        f"{sym_name} target_size={lt.get('size')} base_size={rt.get('size')} mp={lt.get('match_percent')}"
    )
    li = lt.get("instructions", [])
    ri = rt.get("instructions", [])

    def fmt(inst):
        return inst.get("instruction", {}).get("formatted", "")

    n = max(len(li), len(ri))
    mis = 0
    for i in range(n):
        l = fmt(li[i]) if i < len(li) else ""
        r = fmt(ri[i]) if i < len(ri) else ""
        mark = " " if l == r else "*"
        if mark == "*":
            mis += 1
        print(f"{i:4d} {mark} {l:<48} | {r}")
    print(f"mismatched: {mis}/{n}")


if __name__ == "__main__":
    main()
