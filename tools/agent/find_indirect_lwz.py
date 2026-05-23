#!/usr/bin/env python3
# usage: python3 tools/agent/find_indirect_lwz.py
# For each near-match function, find target asm patterns of form
#   `lwz rN, GLOBAL@sda21; mr r3, rN; bl <method>`  (indirect)
# AND check if our build's disasm uses the direct
#   `lwz r3, GLOBAL@sda21; bl <method>` form.
# These mismatches are candidates for the "Bind a global pointer to a typed
# local" lever in docs/MWCC.md.
import os
import re
import subprocess
import json
import sys

DTK = "build/tools/dtk"
OBJDIR_TARGET = "build/GMSJ01/asm"  # .s files (target)
OBJDIR_OURS_OBJ = "build/GMSJ01/src"  # .o files (ours)

LWZ_RE = re.compile(r"lwz r(\d+), ([A-Za-z0-9_]+)@sda21")


def normalize_unit(unit):
    if unit.startswith("mario/"):
        return unit[len("mario/"):]
    if unit.startswith("sdk/"):
        return unit[len("sdk/"):]
    return unit


def parse_asm_file(path):
    """Return dict fn_name -> list of (addr_hex, instr_text)."""
    fn_map = {}
    cur_fn = None
    cur_lines = []
    if not os.path.exists(path):
        return fn_map
    with open(path) as f:
        for line in f:
            mfn = re.match(r"\.fn (\S+),", line)
            if mfn:
                if cur_fn:
                    fn_map[cur_fn] = cur_lines
                cur_fn = mfn.group(1)
                cur_lines = []
                continue
            am = re.search(
                r"/\*\s*([0-9A-Fa-f]+)\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f ]+\*/\s+(.*)$",
                line,
            )
            if am and cur_fn:
                cur_lines.append((am.group(1), am.group(2).strip()))
    if cur_fn:
        fn_map[cur_fn] = cur_lines
    return fn_map


def find_indirect(fn_lines):
    """Return list of (addr, global) for indirect lwz patterns."""
    out = []
    for i, (addr, instr) in enumerate(fn_lines):
        m = LWZ_RE.search(instr)
        if not m:
            continue
        reg = m.group(1)
        glob = m.group(2)
        if reg == "3":
            continue  # already direct
        # check next 1-2 instructions for "mr r3, rN" then "bl"
        for j in range(1, min(3, len(fn_lines) - i)):
            _, nxt = fn_lines[i + j]
            mrm = re.match(r"mr r3, r(\d+)\b", nxt)
            if mrm and mrm.group(1) == reg:
                # check next is a bl
                if i + j + 1 < len(fn_lines):
                    _, bl_line = fn_lines[i + j + 1]
                    if bl_line.startswith("bl "):
                        out.append((addr, glob, bl_line[3:].split()[0]))
                break
    return out


def find_direct(fn_lines):
    """Return list of (addr, global, callee) for direct lwz r3 + bl patterns."""
    out = []
    for i, (addr, instr) in enumerate(fn_lines):
        m = LWZ_RE.search(instr)
        if not m:
            continue
        reg = m.group(1)
        glob = m.group(2)
        if reg != "3":
            continue
        # check within next 3 instructions for a bl
        for j in range(1, min(5, len(fn_lines) - i)):
            _, nxt = fn_lines[i + j]
            if nxt.startswith("bl "):
                out.append((addr, glob, nxt[3:].split()[0]))
                break
            if "lwz r3" in nxt or "li r3" in nxt or "addi r3" in nxt or "mr r3" in nxt:
                break  # r3 reassigned, no longer holding global
    return out


def disasm_ours(unit):
    """Use dtk to disasm our build's .o; return parsed fn_map."""
    unit = normalize_unit(unit)
    o_path = f"{OBJDIR_OURS_OBJ}/{unit}.o"
    if not os.path.exists(o_path):
        return {}
    # output to a temp file
    tmp_s = f"/tmp/ours_{unit.replace('/', '_')}.s"
    if not os.path.exists(tmp_s):
        try:
            subprocess.run(
                [DTK, "elf", "disasm", o_path, tmp_s],
                check=True, capture_output=True
            )
        except Exception as e:
            print(f"# disasm failed for {unit}: {e}", file=sys.stderr)
            return {}
    return parse_asm_file(tmp_s)


def scan(unit, near_match_names):
    unit = normalize_unit(unit)
    target_path = f"{OBJDIR_TARGET}/{unit}.s"
    target_fns = parse_asm_file(target_path)
    if not target_fns:
        return []
    ours_fns = disasm_ours(unit)
    candidates = []
    for fn_name in near_match_names:
        if fn_name not in target_fns:
            continue
        target_indirects = find_indirect(target_fns[fn_name])
        if not target_indirects:
            continue
        ours_lines = ours_fns.get(fn_name, [])
        ours_directs = find_direct(ours_lines)
        # find target indirect globals that our build is doing direct for
        ours_direct_globals = {g for (_, g, _) in ours_directs}
        for (addr, glob, callee) in target_indirects:
            if glob in ours_direct_globals:
                candidates.append((fn_name, glob, callee))
    return candidates


def main():
    report = json.load(open("build/GMSJ01/report.json"))
    near_by_unit = {}
    near_pct = {}
    for u in report["units"]:
        for f in u.get("functions", []) or []:
            mp = f.get("fuzzy_match_percent", 0)
            if 70.0 < mp < 100.0:
                near_by_unit.setdefault(u["name"], []).append(f["name"])
                near_pct[(u["name"], f["name"])] = mp
    print("# Functions where target has `lwz rN; mr r3, rN; bl` but ours has `lwz r3; bl`:")
    print("# (these are candidates for the typed-local lever)")
    rows = []
    for unit, fns in near_by_unit.items():
        cands = scan(unit, fns)
        for (fn, glob, callee) in cands:
            rows.append((near_pct[(unit, fn)], unit, fn, glob, callee))
    rows.sort(key=lambda r: -r[0])
    for (pct, unit, fn, glob, callee) in rows:
        print(f"{pct:6.2f}%  {unit}::{fn}  {glob} -> {callee}")


if __name__ == "__main__":
    main()
