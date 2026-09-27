#!/usr/bin/env python3
# usage: python3 tools/agent/find_frame_diffs.py [--min 80] [--grep REGEX]
"""List non-matching functions whose stack frame size (prologue stwu r1,-N)
differs between the retail target object and our compiled object.
--grep filters to units whose source file matches REGEX (e.g. TParamRT/mSaveParam)."""
import json, os, re, subprocess, sys, argparse

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OBJDUMP = os.path.join(ROOT, "build", "binutils", "powerpc-eabi-objdump")
SYM_RE = re.compile(r"^[0-9a-f]+ <(.+)>:$")
STWU_RE = re.compile(r"stwu\s+r1,(-?\d+)\(r1\)")


def frames(path):
    out = {}
    if not os.path.exists(path):
        return out
    txt = subprocess.run([OBJDUMP, "-d", path], capture_output=True, text=True).stdout
    cur, n = None, 0
    for line in txt.splitlines():
        m = SYM_RE.match(line)
        if m:
            cur, n = m.group(1), 0
            continue
        if cur and cur not in out:
            n += 1
            m = STWU_RE.search(line)
            if m:
                out[cur] = -int(m.group(1))
            elif n > 6:
                out[cur] = 0
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--min", type=float, default=80.0)
    ap.add_argument("--grep", default=None)
    a = ap.parse_args()
    rep = json.load(open(os.path.join(ROOT, "build/GMSJ01/report.json")))
    for u in rep["units"]:
        fns = [f for f in u.get("functions", [])
               if a.min <= f.get("fuzzy_match_percent", 0) < 100]
        if not fns:
            continue
        src = u.get("metadata", {}).get("source_path")
        if a.grep:
            if not src or not os.path.exists(os.path.join(ROOT, src)):
                continue
            if not re.search(a.grep, open(os.path.join(ROOT, src), errors="ignore").read()):
                continue
        rel = u["name"].split("/", 1)[1]
        t = frames(os.path.join(ROOT, "build/GMSJ01/obj", rel + ".o"))
        o = frames(os.path.join(ROOT, "build/GMSJ01/src", rel + ".o"))
        for f in fns:
            n = f["name"]
            if n in t and n in o and t[n] != o[n]:
                print(f"{u['name']}\t{n}\t{f['fuzzy_match_percent']:.2f}\ttgt=0x{t[n]:x}\tours=0x{o[n]:x}\tdelta={t[n]-o[n]}")


if __name__ == "__main__":
    main()
