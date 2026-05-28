#!/usr/bin/env python3
"""
Find functions at 99.X% fuzzy where the gap is small. Useful for
INVESTIGATION-mode polish targets.

Usage: python3 tools/agent/find_near_match.py [min_pct] [max_pct] [min_size] [max_size]
"""
import json
import os
import sys

script_dir = os.path.dirname(os.path.realpath(__file__))
root_dir = os.path.abspath(os.path.join(script_dir, "..", ".."))
REPORT = os.path.join(root_dir, "build", "GMSJ01", "report.json")


def main():
    min_pct = float(sys.argv[1]) if len(sys.argv) > 1 else 99.0
    max_pct = float(sys.argv[2]) if len(sys.argv) > 2 else 99.99
    min_size = int(sys.argv[3]) if len(sys.argv) > 3 else 100
    max_size = int(sys.argv[4]) if len(sys.argv) > 4 else 1000
    r = json.load(open(REPORT))
    results = []
    for u in r["units"]:
        for fn in u["functions"]:
            mp = float(fn.get("fuzzy_match_percent", 0))
            sz = int(fn["size"])
            if min_pct <= mp < max_pct and min_size <= sz <= max_size:
                results.append((mp, sz, u["name"], fn["name"]))
    results.sort(key=lambda x: (-x[0], -x[1]))
    for mp, sz, unit, fn in results[:50]:
        print(f"  {mp:6.2f}% {sz:>5}B {unit:30} {fn}")


if __name__ == "__main__":
    main()
