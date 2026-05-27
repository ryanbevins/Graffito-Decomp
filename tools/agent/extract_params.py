#!/usr/bin/env python3
"""
usage: extract_params.py <asm-file> <ctor-symbol>

Reads a TParams ctor from a .s file and prints:
  param_offset (hex)  default_label  default_value  name_label
Plus mPrmPath string label.

The pattern recognized for each PARAM_INIT is:
  lfs f0, "@NNNN"@sda21(r0)       -> default float ref
  ... (some addi for next name) ...
  stfs f0, 0xMM(rXX)              -> stored at value offset
For int/short params, lfs/stfs may be lwz/stw or li/sth/stw.

For TParamRT<s32>/<s16>/<u8>:
  li r0, N                        -> immediate value
  stw/sth/stb r0, 0xMM(rXX)       -> stored at value offset

The PARAM_INIT name is extracted from the addi to @1490+offset preceding
each bl calcKeyCode__...
"""

import sys, re, pathlib

def main():
    if len(sys.argv) != 3:
        print(__doc__); sys.exit(1)
    asm_path = pathlib.Path(sys.argv[1])
    sym = sys.argv[2]
    lines = asm_path.read_text().splitlines()
    # find fn block
    start = None; end = None
    for i, line in enumerate(lines):
        if line.startswith(f".fn {sym},"):
            start = i
        elif start is not None and line.startswith(".endfn"):
            end = i; break
    if start is None or end is None:
        print(f"symbol not found: {sym}"); sys.exit(1)
    block = lines[start:end]

    rod_base = None     # the @1490 base (string table base name)
    # Find which @label is used for rodata strings:
    for line in block:
        m = re.search(r'lis\s+r\d+,\s*"(@\d+)"@ha', line)
        if m:
            rod_base = m.group(1); break

    # Extract pattern: each PARAM_INIT triggers
    #   addi rN, rRod, 0xNNN   ; (name ptr)
    #   bl calcKeyCode__...
    #   ...
    #   bl __ct__10TBaseParam...
    #   lfs/li ... -> default
    #   stfs/stw/sth/stb ... 0xMM(rXX) -> value at offset
    # Or the first one has mPrmPath store at the top:
    #   addi r0, rRod, 0xNNN
    #   stw r0, 0x0(r3)        -> mPrmPath
    name_re = re.compile(r'addi\s+r\d+,\s*r\d+,\s*0x([0-9a-fA-F]+)')
    value_load_re = re.compile(r'(lfs|li|lwz)\s+\S+,\s*"?(@\d+|0x[0-9a-fA-F]+|[0-9]+)"?')
    value_store_re = re.compile(r'(stfs|stw|sth|stb)\s+\S+,\s*0x([0-9a-fA-F]+)\(r\d+\)')
    bl_re = re.compile(r'bl\s+(\S+)')

    # Walk through. After each bl __ct__10TBaseParam, capture the next
    # store-to-value-offset (the one with stfs/stw/sth/stb to offset >= 0x10).
    print(f"# {sym}")
    print(f"# rod_base = {rod_base}")
    # 1) mPrmPath: first `stw rN, 0x0(r3)` where rN was set by `addi rN, rod, 0xMMM`
    state = "scan_prmpath"
    last_addi = {}       # reg -> offset
    reg_values = {}      # reg -> ("float", label) or ("imm", value); reset on overwrite
    pending_value_for_store = None  # the value to attribute to next param store
    saw_first_basector = False
    for line in block:
        m = re.match(r'\s*/\*[^/]*\*/\s+(\S.*?)\s*$', line)
        if not m:
            continue
        instr = m.group(1)
        # track addi rN, rRod, IMM
        am = re.match(r'addi\s+r(\d+),\s*r\d+,\s*0x([0-9a-fA-F]+)', instr)
        if am:
            last_addi[am.group(1)] = int(am.group(2), 16)
        # mPrmPath: stw rN, 0x0(r3)
        if state == "scan_prmpath":
            sm = re.match(r'stw\s+r(\d+),\s*0x?0\(r3\)', instr)
            if sm:
                if sm.group(1) in last_addi:
                    print(f"mPrmPath = {rod_base}+0x{last_addi[sm.group(1)]:x}")
                else:
                    print(f"mPrmPath = (r{sm.group(1)} — likely ctor arg)")
                state = "scan_params"
                continue
        # capture default value loads/immediates into a register
        lm = re.match(r'lfs\s+f(\d+),\s*"(@\d+)"@sda21', instr)
        if lm:
            reg_values["f"+lm.group(1)] = ("float-sda21", lm.group(2))
            pending_value_for_store = reg_values["f"+lm.group(1)]
        lim = re.match(r'li\s+r(\d+),\s*0x([0-9a-fA-F]+)', instr)
        if lim:
            reg_values["r"+lim.group(1)] = ("imm", int(lim.group(2), 16))
            pending_value_for_store = reg_values["r"+lim.group(1)]
        lim2 = re.match(r'li\s+r(\d+),\s*(-?\d+)$', instr)
        if lim2:
            reg_values["r"+lim2.group(1)] = ("imm", int(lim2.group(2)))
            pending_value_for_store = reg_values["r"+lim2.group(1)]
        # Track stfs/stw/sth/stb to offset >= 0x10 as a param value store
        sm = re.match(r'(stfs|stw|sth|stb)\s+(f|r)(\d+),\s*0x([0-9a-fA-F]+)\(r\d+\)', instr)
        if sm and state == "scan_params":
            opcode = sm.group(1)
            srcreg = sm.group(2)+sm.group(3)
            off = int(sm.group(4), 16)
            if off >= 0x10:
                # value attribution: prefer the most recent value loaded into srcreg
                val = reg_values.get(srcreg, pending_value_for_store)
                shown = val[1] if val else "?"
                print(f"  +0x{off-0x10:x} <- {shown}    (store {opcode} {srcreg} at 0x{off:x})")
                pending_value_for_store = None
                continue

if __name__ == "__main__":
    main()
