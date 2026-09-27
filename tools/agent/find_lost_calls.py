#!/usr/bin/env python3
# usage: python tools/agent/find_lost_calls.py [--prefix Enemy/]
"""List same-TU functions that retail calls via `bl` more often than our object does.

Typical cause: MWCC auto-inlined the callee in our build, while retail reached it
through another inline (nested auto-inline stays out-of-line). See Koopa changeAnm.
"""
import argparse, collections, glob, os, re, subprocess

B = "build/binutils/powerpc-eabi-objdump"
ap = argparse.ArgumentParser()
ap.add_argument("--prefix", default="")
args = ap.parse_args()

for asm in sorted(glob.glob(f"build/GMSJ01/asm/{args.prefix}**/*.s", recursive=True)):
    rel = os.path.relpath(asm, "build/GMSJ01/asm")[:-2]
    obj = f"build/GMSJ01/src/{rel}.o"
    if not os.path.exists(obj):
        continue
    text = open(asm, errors="ignore").read()
    defined = set(re.findall(r"^\.fn (\S+), global", text, re.M))
    retail = collections.Counter()
    for m in re.finditer(r"\tbl (\S+)$", text, re.M):
        name = m.group(1).strip('"')
        if name in defined:
            retail[name] += 1
    if not retail:
        continue
    out = subprocess.run([B, "-dr", obj], capture_output=True, text=True).stdout
    ours = collections.Counter()
    for m in re.finditer(r"R_PPC_REL24\s+(\S+)", out):
        ours[m.group(1)] += 1
    for name, n in retail.items():
        if ours[name] < n:
            print(f"{rel}: {name} retail={n} ours={ours[name]}")
