#!/usr/bin/env python3
# usage: python3 tools/agent/find_double_liveflag_load.py [glob]
# Scans build/GMSJ01/asm/**.s for the pattern:
#   lwz rX, 0xf0(rY)
#   rlwinm. ...
#   beq ...
#   lwz rX, 0xf0(rY)   <- double load of mLiveFlag at offset 0xf0 (LiveActor mLiveFlag)
# Indicates the target uses checkLiveFlag()/offLiveFlag() inline helper pair.
import glob
import os
import re
import sys

ASM_ROOT = "build/GMSJ01/asm"
PAT = re.compile(r"lwz\s+r\d+,\s*0xf0\(r\d+\)")
RLWINM_DOT = re.compile(r"rlwinm\.\s")
BEQ_BNE = re.compile(r"^\s*[*/0-9a-fA-F\s]*\b(beq|bne)\b")


def scan(path):
    hits = []
    with open(path) as f:
        lines = f.readlines()
    fn = None
    for i, ln in enumerate(lines):
        m = re.match(r'\.fn\s+"?([^",]+)"?', ln)
        if m:
            fn = m.group(1)
        if PAT.search(ln):
            # check next 1-4 instr window for rlwinm. + beq + lwz 0xf0 again
            window = lines[i + 1 : i + 6]
            text = "\n".join(window)
            if RLWINM_DOT.search(text) and "beq" in text:
                # see if 0xf0 lwz appears again
                for w in window:
                    if PAT.search(w):
                        hits.append((fn, path, i + 1))
                        break
    return hits


def main():
    pats = sys.argv[1:] or ["**/*.s"]
    seen = set()
    for p in pats:
        for path in glob.glob(os.path.join(ASM_ROOT, p), recursive=True):
            for fn, fp, ln in scan(path):
                key = (fn, fp)
                if key in seen:
                    continue
                seen.add(key)
                rel = os.path.relpath(fp, ASM_ROOT)
                print(f"{rel}:{ln}  {fn}")


if __name__ == "__main__":
    main()
