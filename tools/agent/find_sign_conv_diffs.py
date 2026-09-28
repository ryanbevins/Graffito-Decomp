#!/usr/bin/env python3
# usage: python3 tools/agent/find_sign_conv_diffs.py [--min 50] [--grep REGEX] [--const]
"""List non-matching functions whose count of signed int->float conversion
markers (xoris rX, rY, 0x8000) differs between retail and ours. A retail-only
xoris usually means a field/local is s32 in the original but u32 in our source
(e.g. TShine::unk168/unk170), or vice versa.
--const counts only xoris of a register set by `li` shortly before: a retail-only
hit means an int constant was held in a local (not folded), cf. ConsoleStr startCloseWipe."""
import argparse, json, os, re, subprocess
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OBJDIFF = os.path.join(ROOT, "build", "tools", "objdiff-cli")
X = re.compile(r'xoris r\d+, r\d+, 0x8000')
ap = argparse.ArgumentParser(); ap.add_argument("--min", type=float, default=50)
ap.add_argument("--grep", default="")
ap.add_argument("--const", action="store_true")
a = ap.parse_args()
LI = re.compile(r'li (r\d+), ')
XS = re.compile(r'xoris r\d+, (r\d+), 0x8000')
def count_const(sym):
    ins = [i.get("instruction", {}).get("formatted", "") for i in sym.get("instructions", [])]
    n = 0
    for k, t in enumerate(ins):
        m = XS.search(t)
        if not m:
            continue
        for back in ins[max(0, k - 12):k][::-1]:
            if re.search(r'\b' + m.group(1) + r'\b', back.split(",")[0] if " " in back else ""):
                n += 1 if LI.match(back) and LI.match(back).group(1) == m.group(1) else 0
                break
    return n
rep = json.load(open(os.path.join(ROOT, "build/GMSJ01/report.json")))
for u in rep["units"]:
    if a.grep and not re.search(a.grep, u["name"]):
        continue
    fns = {f["name"]: f.get("fuzzy_match_percent", 0) for f in u.get("functions", [])
           if a.min <= f.get("fuzzy_match_percent", 0) < 100}
    if not fns:
        continue
    try:
        d = json.loads(subprocess.check_output(
            [OBJDIFF, "diff", "-u", u["name"], "-o", "-", "--format", "json"],
            cwd=ROOT, stderr=subprocess.DEVNULL, timeout=120))
    except Exception:
        continue
    right = {s.get("name"): s for s in d["right"]["symbols"]}
    for lt in d["left"]["symbols"]:
        n = lt.get("name")
        if n not in fns or n not in right:
            continue
        cnt = count_const if a.const else (lambda s: sum(1 for i in s.get("instructions", [])
                            if X.search(i.get("instruction", {}).get("formatted", ""))))
        l, r = cnt(lt), cnt(right[n])
        if l != r:
            print(f"{u['name']}\t{n}\t{fns[n]:.2f}\tretail={l}\tours={r}")
