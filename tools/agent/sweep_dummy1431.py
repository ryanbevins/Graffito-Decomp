#!/usr/bin/env python3
"""
Sweep all TUs in the project to find those where target's .data starts
with the dummy1431/dummy1411/dummy1210 40-byte infectious pattern and
our base does not.

Usage: python3 tools/agent/sweep_dummy1431.py
"""
import json
import os
import subprocess
import sys
import base64

script_dir = os.path.dirname(os.path.realpath(__file__))
root_dir = os.path.abspath(os.path.join(script_dir, "..", ".."))
OBJDIFF = os.path.join(root_dir, "build", "tools", "objdiff-cli")
REPORT = os.path.join(root_dir, "build", "GMSJ01", "report.json")

EXPECTED_HEX = "3f8000003f8000003f8000003f8000003f8000003f80000000000000000000020000000100000003"


def check_unit(unit):
    try:
        out = subprocess.check_output(
            [OBJDIFF, "diff", "-c", "functionRelocDiffs=data_value",
             "-u", unit, "-o", "-", "--format", "json"],
            cwd=root_dir, stderr=subprocess.DEVNULL,
        )
    except Exception:
        return None
    d = json.loads(out)
    L_data = next((s for s in d["left"]["sections"] if s["name"] == ".data"), None)
    if L_data is None:
        return None
    diffs = L_data.get("data_diff", [])
    if not diffs:
        return None
    first = diffs[0]
    if first.get("kind") == "DIFF_DELETE" and "data" in first:
        bs = base64.b64decode(first["data"])
        if bs[:40].hex().startswith(EXPECTED_HEX):
            return (int(L_data["size"]), int(first.get("size", "0")))
    return None


def main():
    r = json.load(open(REPORT))
    candidates = []
    for u in r["units"]:
        if "functions" not in u:
            continue
        result = check_unit(u["name"])
        if result:
            L_size, delete_size = result
            # measure of TU
            m = int(u["measures"].get("matched_code", 0))
            t = int(u["measures"].get("total_code", 1))
            candidates.append((m/t*100, u["name"], L_size, delete_size, m, t))
    candidates.sort(key=lambda x: x[0])
    for pct, unit, L, dsize, m, t in candidates:
        print(f"  {pct:6.2f}% L.data={L:>5} delete={dsize:>4} m={m:>5} t={t:>5} {unit}")


if __name__ == "__main__":
    main()
