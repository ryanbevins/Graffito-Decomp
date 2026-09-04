#!/usr/bin/env python3
# usage: python3 tools/agent/report_delta.py BASELINE CURRENT
"""Compare canonical objdiff reports, including exact-match regressions."""
import argparse
import json
from pathlib import Path


KEYS = ("fuzzy_match_percent", "matched_code", "matched_functions", "matched_data")


def values(measures):
    return tuple(float(measures.get(k, 0)) for k in KEYS)


def show(name, before, after):
    old, new = values(before), values(after)
    if old != new:
        delta = tuple(b - a for a, b in zip(old, new))
        print(f"{name}: fuzzy {old[0]:.6f} -> {new[0]:.6f}; "
              f"exact code {delta[1]:+.0f}B, functions {delta[2]:+.0f}, "
              f"data {delta[3]:+.0f}B")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("baseline", type=Path)
    parser.add_argument("current", type=Path)
    args = parser.parse_args()
    before = json.loads(args.baseline.read_text())
    after = json.loads(args.current.read_text())
    for key in ("total_code", "total_functions", "total_units"):
        if int(before["measures"][key]) != int(after["measures"][key]):
            parser.error(f"report populations differ: {key}")
    show("Overall", before["measures"], after["measures"])
    old_units = {u["name"]: u["measures"] for u in before["units"]}
    new_units = {u["name"]: u["measures"] for u in after["units"]}
    if old_units.keys() != new_units.keys():
        parser.error("report unit sets differ")
    for name in sorted(old_units):
        show(name, old_units[name], new_units[name])
    regression = any(int(after["measures"][k]) < int(before["measures"][k])
                     for k in ("matched_code", "matched_functions"))
    print("Exact code/function regression" if regression else
          "Exact code/function totals preserved")
    return int(regression)


if __name__ == "__main__":
    raise SystemExit(main())
