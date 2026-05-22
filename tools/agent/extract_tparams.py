#!/usr/bin/env python3
"""Extract TParams field info (offsets, names, types, default values) from MWCC asm.

usage: python3 tools/agent/extract_tparams.py <asm_path> <ctor_symbol>

Walks the ctor body looking for `bl __ct__10TBaseParamFP7TParamsUsPCc`. For each
PARAM_INIT, recovers:
  - field offset (from `addi r3, r31, FIELD_OFF` immediately preceding the bl)
  - name string (via `addi r6, r30, X`  + @1490 base in rodata,  OR  `li r6, "@N"@sda21`)
  - default type & value (from the `stfs/stw/sth/stb f0|rN, FIELD_OFF+0x10(r31)`
    instruction that follows, plus the preceding load of that source register).
"""

import re
import sys
from pathlib import Path


def parse_asm(asm_path):
    return Path(asm_path).read_text().splitlines()


def build_label_map(lines):
    label_to_def = {}
    cur_label = None
    for line in lines:
        m_obj = re.match(r'\.obj "?(@\d+)"?, local', line)
        m_sym = re.match(r'\.sym "?(@\d+)"?, local', line)
        if m_obj or m_sym:
            cur_label = (m_obj or m_sym).group(1)
            continue
        if line.startswith('.endobj') or line.startswith('.endsym'):
            cur_label = None
            continue
        if cur_label is None:
            continue
        m_str = re.match(r'\s+\.string "(.*)"', line)
        if m_str:
            label_to_def[cur_label] = {"kind": "string", "value": m_str.group(1)}
            continue
        m_f = re.match(r'\s+\.float (\S+)', line)
        if m_f:
            try:
                v = float(m_f.group(1))
            except ValueError:
                v = m_f.group(1)
            label_to_def[cur_label] = {"kind": "float", "value": v}
            continue
    return label_to_def


def find_ctor_body(lines, sym):
    start = None
    for i, line in enumerate(lines):
        if line.startswith(f".fn {sym},"):
            start = i + 1
        elif start is not None and line.startswith(f".endfn {sym}"):
            return lines[start:i]
    raise SystemExit(f"symbol {sym} not found")


INSTR_RE = re.compile(r'/\*[^*]*\*/\s+([\w.]+)\s*(.*)')


def parse_instr(line):
    m = INSTR_RE.match(line.strip())
    if not m:
        return None
    return m.group(1), m.group(2).strip()


def build_addr_to_label(lines):
    addr_to_label = {}
    label_to_addr = {}
    for i, line in enumerate(lines):
        m_h = re.match(r'# \.rodata:0x([0-9A-Fa-f]+) \| 0x([0-9A-Fa-f]+) \| size:', line)
        if m_h:
            addr = int(m_h.group(2), 16)
            for j in range(i + 1, min(i + 5, len(lines))):
                m_o = re.match(r'\.obj "?(@\d+)"?, local', lines[j])
                m_s = re.match(r'\.sym "?(@\d+)"?, local', lines[j])
                if m_o or m_s:
                    lbl = (m_o or m_s).group(1)
                    addr_to_label[addr] = lbl
                    label_to_addr[lbl] = addr
                    break
    return addr_to_label, label_to_addr


def hex_to_int(s):
    try:
        return int(s, 0)
    except ValueError:
        return None


def extract_params(instrs):
    n = len(instrs)
    fields = []
    # Pre-compute register state at each instruction index by walking forward.
    # Only track immediate-load patterns (li rN, IMM and addi rN, r0, IMM); we
    # care only about callee-saved registers (r14..r31) since the volatile ones
    # are clobbered across each PARAM_INIT bl.
    reg_state = [{} for _ in range(n + 1)]
    state = {}
    for idx, (op, args) in enumerate(instrs):
        reg_state[idx] = dict(state)
        if op == 'li':
            parts = [x.strip() for x in args.split(',')]
            if len(parts) == 2:
                v = hex_to_int(parts[1])
                if v is not None:
                    state[parts[0]] = v
                else:
                    state.pop(parts[0], None)
        elif op == 'addi':
            parts = [x.strip() for x in args.split(',')]
            if len(parts) == 3 and parts[1] == 'r0':
                v = hex_to_int(parts[2])
                if v is not None:
                    state[parts[0]] = v
                else:
                    state.pop(parts[0], None)
            elif len(parts) == 3:
                state.pop(parts[0], None)
        elif op == 'lis':
            parts = [x.strip() for x in args.split(',', 1)]
            state.pop(parts[0], None)
        elif op == 'bl':
            # Clobber volatile regs
            for r in ['r0', 'r3', 'r4', 'r5', 'r6', 'r7', 'r8', 'r9', 'r10', 'r11', 'r12']:
                state.pop(r, None)
        # other ops: don't track

    for i, (op, args) in enumerate(instrs):
        if op != 'bl' or '__ct__10TBaseParamFP7TParamsUsPCc' not in args:
            continue
        # Backward window: find `addi r3, r31, FIELD_OFF` and `r6` source
        field_off = None
        name_kind = None
        name_value = None
        for j in range(i - 1, max(-1, i - 12), -1):
            op2, args2 = instrs[j]
            if op2 == 'addi':
                p2 = [x.strip() for x in args2.split(',')]
                if p2[0] == 'r3' and p2[1] == 'r31' and field_off is None:
                    field_off = hex_to_int(p2[2])
                if p2[0] == 'r6' and p2[1] == 'r30' and name_kind is None:
                    name_kind = "r30_off"
                    name_value = hex_to_int(p2[2])
            if op2 == 'li':
                p2 = [x.strip() for x in args2.split(',')]
                if p2[0] == 'r6' and name_kind is None:
                    m = re.match(r'"(@\d+)"@sda21', p2[1])
                    if m:
                        name_kind = "sda2_label"
                        name_value = m.group(1)

        # Forward window: find the value store at field_off+0x10
        default = None
        if field_off is not None:
            for j in range(i + 1, min(n, i + 30)):
                op2, args2 = instrs[j]
                if op2 == 'bl' and '__ct__10TBaseParamFP7TParamsUsPCc' in args2:
                    break
                if op2 in ('stfs', 'stw', 'sth', 'stb'):
                    m = re.match(r'(\w+),\s*(\S+)\(r31\)', args2)
                    if m:
                        store_off = hex_to_int(m.group(2))
                        if store_off == field_off + 0x10:
                            src = m.group(1)
                            tname = {'stfs': 'f32', 'stw': 's32', 'sth': 's16', 'stb': 's8'}[op2]
                            # walk back to find source
                            if op2 == 'stfs':
                                # find immediately preceding lfs src
                                for k in range(j - 1, max(-1, j - 6), -1):
                                    op3, args3 = instrs[k]
                                    if op3 == 'lfs':
                                        m2 = re.match(rf'{src},\s*"(@\d+)"@sda21\(r0\)', args3)
                                        if m2:
                                            default = (tname, ("sda2_f32", m2.group(1)))
                                            break
                                if default is None:
                                    default = (tname, ("unknown", "?"))
                            else:
                                # int/short/byte: look up register state at store
                                st = reg_state[j]
                                if src in st:
                                    default = (tname, ("imm", st[src]))
                                else:
                                    default = (tname, ("unknown", src))
                            break
        fields.append({
            "field_off": field_off,
            "name_kind": name_kind,
            "name_value": name_value,
            "default": default,
        })
    return fields


def resolve_name(f, labels, addr_to_label, label_to_addr):
    if f["name_kind"] == "sda2_label":
        d = labels.get(f["name_value"])
        if d and d["kind"] == "string":
            return d["value"]
    if f["name_kind"] == "r30_off":
        base = label_to_addr.get("@1490")
        if base is None or f["name_value"] is None:
            return "??"
        target = base + f["name_value"]
        lbl = addr_to_label.get(target)
        if lbl is None:
            return "??"
        d = labels.get(lbl)
        if d and d["kind"] == "string":
            return d["value"]
    return "??"


def resolve_default(f, labels):
    if not f["default"]:
        return ("?", "?")
    tname, raw = f["default"]
    kind, val = raw
    if kind == "imm":
        if tname == "s16":
            return (tname, str(val))
        return (tname, str(val))
    if kind == "sda2_f32":
        d = labels.get(val)
        if d and d["kind"] == "float":
            return (tname, str(d["value"]))
        return (tname, f"?({val})")
    if kind == "unknown":
        return (tname, f"?({val})")
    return (tname, "?")


def main():
    asm_path = sys.argv[1]
    sym = sys.argv[2]
    lines = parse_asm(asm_path)
    labels = build_label_map(lines)
    addr_to_label, label_to_addr = build_addr_to_label(lines)
    ctor = find_ctor_body(lines, sym)
    instrs = [parse_instr(line) for line in ctor]
    instrs = [x for x in instrs if x]
    fields = extract_params(instrs)
    print(f"# {sym}  ({len(fields)} PARAM_INIT)")
    print(f"# fieldOff  type   name                                       default")
    for f in fields:
        name = resolve_name(f, labels, addr_to_label, label_to_addr)
        tname, val = resolve_default(f, labels)
        fo = "?" if f["field_off"] is None else f"0x{f['field_off']:x}"
        print(f"  {fo:>6}    {tname:<4}   {name:<42}   {val}")


if __name__ == "__main__":
    main()
