#!/usr/bin/env python3
# usage: python3 tools/agent/tu_delta.py BASE_REPORT NEW_REPORT <unit-suffix>
"""Print TU fuzzy/matched_code and per-function fuzzy changes between reports."""
import json
import sys


def load(path, suffix):
    for u in json.load(open(path))["units"]:
        if u["name"].endswith(suffix):
            m = u["measures"]
            print(path, m.get("fuzzy_match_percent"), m.get("matched_code"))
            return {f["name"]: f.get("fuzzy_match_percent", 0) for f in u.get("functions", [])}
    return {}


a = load(sys.argv[1], sys.argv[3])
b = load(sys.argv[2], sys.argv[3])
for k in a:
    if abs(a[k] - b.get(k, 0)) > 1e-6:
        print(k[:70], round(a[k], 3), "->", round(b.get(k, 0), 3))
