#!/usr/bin/env python3
# usage: python3 tools/agent/find_const_symbol_swaps.py [--min 90] [--grep REGEX]
"""Find non-matching functions where one side loads an anonymous literal
(@NNNN@sda21) and the other loads a named variable (e.g. a static member such as
mBlockXZScale) at the same aligned instruction. This usually means the source
uses a named static where retail used a literal (or vice versa) -- a cheap fix.
Example: TTobiPuku::scalingChangeActor used mBlockXZScale, retail used 3.0f."""
import argparse, json, os, re, subprocess

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OBJDIFF = os.path.join(ROOT, "build", "tools", "objdiff-cli")
ANON = re.compile(r'@\d+@sda21')
NAMED = re.compile(r'(?<![@\w])([A-Za-z_][\w$<>,]*)@sda21')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--min", type=float, default=90.0)
    ap.add_argument("--grep", default="")
    a = ap.parse_args()
    rep = json.load(open(os.path.join(ROOT, "build/GMSJ01/report.json")))
    for u in rep["units"]:
        if a.grep and not re.search(a.grep, u["name"]):
            continue
        fns = {f["name"] for f in u.get("functions", [])
               if a.min <= f.get("fuzzy_match_percent", 0) < 100}
        if not fns:
            continue
        try:
            out = subprocess.check_output(
                [OBJDIFF, "diff", "-u", u["name"], "-o", "-", "--format", "json"],
                cwd=ROOT, stderr=subprocess.DEVNULL, timeout=120)
        except Exception:
            continue
        d = json.loads(out)
        right = {s.get("name"): s for s in d["right"]["symbols"]}
        for lt in d["left"]["symbols"]:
            name = lt.get("name")
            if name not in fns or name not in right:
                continue
            li = lt.get("instructions", [])
            ri = right[name].get("instructions", [])
            for i in range(min(len(li), len(ri))):
                l = li[i].get("instruction", {}).get("formatted", "")
                r = ri[i].get("instruction", {}).get("formatted", "")
                if l == r:
                    continue
                if (ANON.search(l) and NAMED.search(r)) or (NAMED.search(l) and ANON.search(r)):
                    print(f"{u['name']}\t{name}\t#{i}\tretail: {l}\tours: {r}")


if __name__ == "__main__":
    main()
