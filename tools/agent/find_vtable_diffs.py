#!/usr/bin/env python3
# usage: python3 tools/agent/find_vtable_diffs.py
# Scans all non-100% functions across report.json and flags any where the
# only difference between target and our build is a single virtual-call vtable
# offset (lwz r12, OFFSET(r12)). These are nearly always 1-line source fixes
# (calling the wrong virtual method).
import json
import os
import re
import subprocess


def main():
    with open("build/GMSJ01/report.json") as f:
        d = json.load(f)
    candidates = []
    for unit in d.get("units", []):
        m = unit.get("measures", {})
        fuzzy = m.get("fuzzy_match_percent", 0)
        if 85.0 <= fuzzy < 100:
            for fn in unit.get("functions", []):
                pct = fn.get("fuzzy_match_percent", 0)
                sz = int(fn.get("size", 0))
                if 85.0 <= pct < 100 and sz <= 600:
                    candidates.append((unit["name"], fn["name"], pct, sz))
    print(f"Scanning {len(candidates)} candidate functions...")
    print()
    for unit_name, sym, pct, sz in candidates:
        unit_path = unit_name.replace("mario/", "")
        target = f"build/GMSJ01/obj/{unit_path}.o"
        base = f"build/GMSJ01/src/{unit_path}.o"
        if not os.path.exists(target) or not os.path.exists(base):
            continue
        out = subprocess.run(
            ["build/tools/objdiff-cli", "diff", "-1", target, "-2", base, sym,
             "--format", "json", "-o", "-"],
            capture_output=True, text=True)
        if out.returncode != 0:
            continue
        try:
            j = json.loads(out.stdout)
        except json.JSONDecodeError:
            continue
        left_fn = find_sym(j.get("left", {}).get("symbols", []), sym)
        right_fn = find_sym(j.get("right", {}).get("symbols", []), sym)
        if not left_fn or not right_fn:
            continue
        li = left_fn.get("instructions", [])
        ri = right_fn.get("instructions", [])
        if len(li) != len(ri):
            continue
        diffs = []
        for l, r in zip(li, ri):
            ls = (l.get("instruction") or {}).get("formatted", "").strip()
            rs = (r.get("instruction") or {}).get("formatted", "").strip()
            if ls != rs:
                diffs.append((ls, rs))
        # Check for vtable offset diffs (the pattern: target and our both load via lwz r12)
        # We want EXACTLY this pattern: lwz r12, OFFSET_TARGET(r12) vs lwz r12, OFFSET_OURS(r12)
        vt_re = re.compile(r"^lwz r12, 0x[0-9a-f]+\(r12\)$")
        vt_diffs = [d for d in diffs if vt_re.match(d[0]) and vt_re.match(d[1])]
        if vt_diffs and len(diffs) <= 4:
            print(f"{unit_name} :: {sym} ({pct:.2f}%, {sz}B)  -- {len(diffs)} diffs, {len(vt_diffs)} vtable")
            for ls, rs in diffs:
                marker = "V" if vt_re.match(ls) else " "
                print(f"  {marker} target: {ls}")
                print(f"  {marker} ours:   {rs}")
            print()


def find_sym(syms, name):
    for s in syms:
        if s.get("name") == name:
            return s
    return None


if __name__ == "__main__":
    main()
