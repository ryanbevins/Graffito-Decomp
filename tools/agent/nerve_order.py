# usage: python tools/agent/nerve_order.py [src/Enemy/foo.cpp ...]  (no args: scan all src/**/*.cpp)
# Compares the retail numbering order of nerve statics (instance$NNNN inside theNerve__*)
# with the DEFINE_NERVE order in our source. Parse order drives the numbering, so a
# mismatch usually shows up as addi r5,rX,<bss offset> operand diffs before
# __register_global_object. Prints the retail order for TUs that disagree.
import glob
import os
import re
import sys


def retail_order(asm):
    # Identify each instance$NNNN by the nerve vtable stored into it, so fully
    # inlined theNerve bodies (no out-of-line theNerve__ symbol) are covered too.
    lines = open(asm, encoding="utf-8", errors="replace").read().splitlines()
    seen = {}
    for i, line in enumerate(lines):
        m = re.search(r"stw r0, instance\$(\d+)@sda21", line)
        if not m:
            continue
        for near in lines[max(0, i - 3):i + 4]:
            v = re.search(r"__vt__\d+(TNerve\w+?)@l", near)
            if v:
                seen.setdefault(int(m.group(1)), v.group(1))
                break
    return [seen[k] for k in sorted(seen)]


def main():
    srcs = sys.argv[1:] or glob.glob("src/**/*.cpp", recursive=True)
    for src in sorted(srcs):
        text = open(src, encoding="utf-8", errors="replace").read()
        ours = re.findall(r"^DEFINE_NERVE\((\w+)\s*,", text, re.M)
        if len(ours) < 2:
            continue
        rel = os.path.relpath(src, "src")[:-4]
        asm = os.path.join("build/GMSJ01/asm", rel + ".s")
        if not os.path.exists(asm):
            continue
        retail = [n for n in retail_order(asm) if n in ours]
        mine = [n for n in ours if n in retail]
        if retail != mine:
            print(f"{rel}: {len(retail)} nerves out of order")
            print("  retail:", " ".join(retail))
            print("  source:", " ".join(mine))


if __name__ == "__main__":
    main()
