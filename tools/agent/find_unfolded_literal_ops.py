#!/usr/bin/env python3
# usage: python3 tools/agent/find_unfolded_literal_ops.py [--min 50]
"""List non-matching functions whose retail asm performs float arithmetic on two
registers both loaded straight from anonymous sdata2 literals (e.g. -400 - -700).
MWCC folds such expressions when they are plain locals; retail keeping them means
the constants flowed through inline/struct parameters (e.g. TVec2 ctor + sub),
see SMS_AddDamageFogEffect and TCogwheel::initMapObj."""
import argparse, glob, json, os, re
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
FN = re.compile(r'^\.fn (\S+),')
LIT = re.compile(r'\blfs (f\d+), "?@\d+"?@sda21')
OP = re.compile(r'\b(fsubs|fadds|fmuls|fdivs) (f\d+), (f\d+), (f\d+)')
ap = argparse.ArgumentParser(); ap.add_argument("--min", type=float, default=50)
a = ap.parse_args()
rep = json.load(open(os.path.join(ROOT, "build/GMSJ01/report.json")))
bad = {}
for u in rep["units"]:
    for f in u.get("functions", []):
        p = f.get("fuzzy_match_percent", 0)
        if a.min <= p < 100:
            bad[f["name"]] = (u["name"], p)
for path in glob.glob(os.path.join(ROOT, "build/GMSJ01/asm/**/*.s"), recursive=True):
    fn = None; lits = {}
    for line in open(path, errors="replace"):
        m = FN.match(line)
        if m:
            fn = m.group(1); lits = {}; continue
        if fn not in bad:
            continue
        m = LIT.search(line)
        if m:
            lits[m.group(1)] = True; continue
        m = OP.search(line)
        if m:
            d, s1, s2 = m.group(2), m.group(3), m.group(4)
            if lits.get(s1) and lits.get(s2):
                print(f"{bad[fn][0]}\t{fn}\t{bad[fn][1]:.2f}\t{line.split('*/')[-1].strip()}")
                fn = None
                continue
            lits.pop(d, None)
        elif re.search(r'\b(bl|b|blr|bctr)\b', line):
            lits = {}
