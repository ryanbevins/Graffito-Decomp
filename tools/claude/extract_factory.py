#!/usr/bin/env python3
"""Extract strcmp -> new -> ctor chains from MarNameRefGen factory asm."""
import re
import sys
from pathlib import Path

asm_path = Path(sys.argv[1] if len(sys.argv) > 1 else
                "build/GMSJ01/asm/System/MarNameRefGen_MapObj.s")
text = asm_path.read_text()

# Find the rodata section and parse strings into addr -> str
rodata_match = re.search(r"\.rodata\n(.*?)(?=\n\.section|\Z)", text, re.S)
rodata_text = rodata_match.group(1) if rodata_match else ""

# Parse .obj "@NAME", local ... .endobj
# Track running offset in rodata
obj_pattern = re.compile(
    r"# \.rodata:0x([0-9A-Fa-f]+) \| 0x[0-9A-Fa-f]+ \| size: 0x([0-9A-Fa-f]+)\n"
    r"\.obj \"@(\d+)\", local\n(.*?)\.endobj",
    re.S,
)

# Build offset -> shift-jis bytes mapping
offset_to_bytes = {}
for m in obj_pattern.finditer(rodata_text):
    off = int(m.group(1), 16)
    size = int(m.group(2), 16)
    body = m.group(4)
    raw = bytearray()
    # collect .4byte values
    for hm in re.finditer(r"\.4byte 0x([0-9A-Fa-f]+)", body):
        v = int(hm.group(1), 16)
        raw.extend(v.to_bytes(4, "big"))
    # collect .byte sequences
    for bm in re.finditer(r"\.byte ([^\n]+)", body):
        for tok in bm.group(1).split(","):
            tok = tok.strip()
            if tok.startswith("0x"):
                raw.append(int(tok, 16))
    # .string "..."
    for sm in re.finditer(r'\.string "([^"]*)"', body):
        s = sm.group(1).encode("latin1").decode("unicode_escape").encode("latin1")
        raw.extend(s)
        raw.append(0)
    offset_to_bytes[off] = bytes(raw[:size])

def decode_sjis(b):
    try:
        s = b.split(b"\x00")[0].decode("shift_jis")
        return s
    except UnicodeDecodeError:
        return b.split(b"\x00")[0].decode("latin1", errors="replace")

# Find function body of getNameRef_MapObj
fn_pattern = re.compile(
    r"\.fn getNameRef_MapObj__14TMarNameRefGenCFPCc.*?\n(.*?)\n\.endfn",
    re.S,
)
fn_body = fn_pattern.search(text).group(1)

# Walk instructions and extract pattern:
# addi r4, r31, OFFSET   ; r4 = string ptr
# bl strcmp
# cmpwi r3, 0
# bne ...
# li r3, SIZE
# bl __nw__FUl
# ...
# bl __ct__CLASSFPCc

# We'll do a regex pass for the key milestones
instr_lines = []
for line in fn_body.splitlines():
    m = re.search(r"\*/\s*(\S.+?)$", line)
    if m:
        instr_lines.append(m.group(1).strip())

i = 0
entries = []  # list of (cmp_str, ctor_name, size)
while i < len(instr_lines):
    line = instr_lines[i]
    # find: addi r4, r31, OFFSET
    m = re.match(r"addi r4, r31, (0x[0-9A-Fa-f]+)", line)
    if m:
        off = int(m.group(1), 16)
        # next should be bl strcmp
        if i + 1 < len(instr_lines) and "bl strcmp" in instr_lines[i + 1]:
            cmp_off = off
            # search forward for li r3, SIZE then bl __nw__ then bl __ct__
            size = None
            ctor = None
            ctor_off = None  # string offset passed to ctor
            for j in range(i + 2, min(i + 30, len(instr_lines))):
                sl = instr_lines[j]
                sm = re.match(r"li r3, (0x[0-9A-Fa-f]+|\d+)", sl)
                if sm and size is None:
                    val = sm.group(1)
                    size = int(val, 0)
                cm = re.match(r"bl __ct__(\S+)", sl)
                if cm:
                    ctor = cm.group(1)
                    # find the addi r4, r31, OFFSET just before this ctor call (the constructor's name arg)
                    for k in range(j - 1, max(j - 8, i), -1):
                        am = re.match(r"addi r4, r31, (0x[0-9A-Fa-f]+)", instr_lines[k])
                        if am:
                            ctor_off = int(am.group(1), 16)
                            break
                    break
                # If we see a strcmp before ctor, this entry's ctor was inlined or something
                if "bl strcmp" in sl:
                    break
            cmp_str_bytes = offset_to_bytes.get(cmp_off, b"")
            ctor_str_bytes = offset_to_bytes.get(ctor_off, b"") if ctor_off is not None else b""
            entries.append({
                "cmp": decode_sjis(cmp_str_bytes),
                "cmp_off": cmp_off,
                "size": size,
                "ctor": ctor,
                "ctor_off": ctor_off,
                "ctor_str": decode_sjis(ctor_str_bytes),
            })
    i += 1

import io
out = io.StringIO()
print(f"Found {len(entries)} entries", file=out)
for e in entries:
    cmp_s = e["cmp"]
    ctor = e["ctor"] or "?"
    size = e["size"]
    sizes = f"{size:#x}" if size is not None else "?"
    ctor_s = e["ctor_str"]
    print(f'  "{cmp_s}" sz={sizes} -> {ctor}  ctor_str="{ctor_s}"', file=out)
sys.stdout.buffer.write(out.getvalue().encode("utf-8"))
