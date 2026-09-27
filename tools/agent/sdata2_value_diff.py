#!/usr/bin/env python3
# usage: python tools/agent/sdata2_value_diff.py [--prefix Player/] [--limit 50]
"""Compare the multiset of .sdata2 float/double values per TU (retail asm vs our object).

Values present in retail but missing from ours usually mean a wrong literal in source
(e.g. 3.0f where retail uses 0.1f) -- objdiff only shows these as relocation-name rows.
"""
import argparse, collections, glob, os, re, struct, subprocess

B = "build/binutils/powerpc-eabi-"
ap = argparse.ArgumentParser()
ap.add_argument("--prefix", default="")
ap.add_argument("--limit", type=int, default=60)
args = ap.parse_args()


def ours(obj):
    syms = []
    out = subprocess.run([B + "objdump", "-t", obj], capture_output=True, text=True).stdout
    for l in out.splitlines():
        m = re.match(r"([0-9a-f]+)\s+l\s+O\s+\.sdata2\s+([0-9a-f]+)\s+(@\S+)", l)
        if m:
            syms.append((int(m.group(1), 16), int(m.group(2), 16)))
    if not syms:
        return collections.Counter()
    data = subprocess.run([B + "objcopy", "-O", "binary", "-j", ".sdata2", obj, "/dev/stdout"],
                          capture_output=True).stdout
    c = collections.Counter()
    for o, s in syms:
        b = data[o:o + s]
        if s == 4 and len(b) == 4:
            c[round(struct.unpack(">f", b)[0], 5)] += 1
        elif s == 8 and len(b) == 8:
            c[round(struct.unpack(">d", b)[0], 5)] += 1
    return c


def retail(asm):
    txt = open(asm, encoding="utf-8", errors="replace").read()
    i = txt.find(".section .sdata2")
    if i < 0:
        return collections.Counter()
    c = collections.Counter()
    for m in re.finditer(r'\.obj "?@\d+"?, local\s*\n\s*\.(float|double) (\S+)', txt[i:]):
        try:
            c[round(float(m.group(2)), 5)] += 1
        except ValueError:
            pass
    return c


rows = []
for asm in glob.glob("build/GMSJ01/asm/**/*.s", recursive=True):
    rel = os.path.relpath(asm, "build/GMSJ01/asm")[:-2]
    if not rel.startswith(args.prefix):
        continue
    obj = f"build/GMSJ01/src/{rel}.o"
    if not os.path.exists(obj):
        continue
    r, o = retail(asm), ours(obj)
    if not r or not o:
        continue
    miss, extra = r - o, o - r
    if miss or extra:
        rows.append((len(miss) + len(extra), rel, dict(miss), dict(extra)))
rows.sort(key=lambda x: x[0])
for n, rel, miss, extra in rows[: args.limit]:
    print(f"{rel}: retail-only={miss} ours-only={extra}")
