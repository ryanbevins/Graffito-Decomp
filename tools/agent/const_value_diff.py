#!/usr/bin/env python3
# usage: python3 tools/agent/const_value_diff.py mario/Enemy/amiNoko [FUNCTION ...]
"""Find aligned anonymous .sdata2 loads with different bytes, using one TU diff.

Candidates only: inspect the full diff, target relocations, and raw retail DOL
before changing source. Reconstructed object data can contain false relocations.
"""
import argparse
import json
import re
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BINUTILS = ROOT / "build/binutils/powerpc-eabi-"
LOAD = re.compile(r"^(lfs|lfd|lwz) [fr]\d+, (@\d+)@sda21$")
SYMBOL = re.compile(r"^([0-9a-f]+)\s+\w+\s+O\s+\.sdata2\s+"
                    r"[0-9a-f]+\s+(@\d+)$", re.MULTILINE)


def run(argv):
    return subprocess.check_output(argv, cwd=ROOT, stderr=subprocess.PIPE)


def literals(path):
    table = run([str(BINUTILS) + "objdump", "-t", str(path)]).decode()
    data = run([str(BINUTILS) + "objcopy", "-O", "binary", "-j", ".sdata2",
                str(path), "/dev/stdout"])
    return data, {name: int(offset, 16) for offset, name in SYMBOL.findall(table)}


def load_bytes(entry, data, symbols):
    inst = entry.get("instruction", {})
    match = LOAD.fullmatch(inst.get("formatted", ""))
    if not match or match[2] not in symbols:
        return None
    op, name = match.groups()
    size = 8 if op == "lfd" else 4
    start = symbols[name]
    value = data[start:start + size]
    if len(value) != size:
        return None
    return op, name, value


def display(op, value):
    if op == "lwz":
        return "0x" + value.hex()
    return repr(struct.unpack(">d" if op == "lfd" else ">f", value)[0])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("unit")
    parser.add_argument("functions", nargs="*", help="name substrings")
    args = parser.parse_args()
    rel = args.unit.removeprefix("mario/")
    left_data, left_literals = literals(ROOT / f"build/GMSJ01/obj/{rel}.o")
    right_data, right_literals = literals(ROOT / f"build/GMSJ01/src/{rel}.o")
    diff = json.loads(run([str(ROOT / "build/tools/objdiff-cli"), "diff", "-c",
                           "functionRelocDiffs=data_value", "-u", args.unit,
                           "-o", "-", "--format", "json"]))
    right_symbols = diff["right"]["symbols"]
    for left in diff["left"]["symbols"]:
        if not left.get("instructions") or not left.get("target_symbol"):
            continue
        name = left.get("demangled_name", left["name"])
        if args.functions and not any(
                pattern.lower() in (name + " " + left["name"]).lower()
                for pattern in args.functions):
            continue
        right = right_symbols[left["target_symbol"] - 1]
        # Objdiff arrays include alignment gaps: pair rows, not raw addresses.
        for lrow, rrow in zip(left["instructions"], right.get("instructions", [])):
            lv = load_bytes(lrow, left_data, left_literals)
            rv = load_bytes(rrow, right_data, right_literals)
            if not lv or not rv or lv[0] != rv[0] or lv[2] == rv[2]:
                continue
            address = int(lrow["instruction"]["address"])
            print(f"{name} @{address:x}: retail {lv[1]}={display(lv[0], lv[2])} "
                  f"ours {rv[1]}={display(rv[0], rv[2])}")


if __name__ == "__main__":
    main()
