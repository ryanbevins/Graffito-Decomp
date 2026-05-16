#!/usr/bin/env python3
"""Convert static byte-array string constants in MarNameRefGen_Enemy.cpp
to inline Shift-JIS literals (which MWCC handles with -multibyte).
"""
import re
import sys
from pathlib import Path

src_path = Path("src/System/MarNameRefGen_Enemy.cpp")
src = src_path.read_text(encoding="utf-8")

# Parse the byte arrays into a map of id -> bytes
str_table = {}
pattern = re.compile(
    r"^static const unsigned char s(\d+)\[\] = \{ ([^}]+) \};$",
    re.M,
)
for m in pattern.finditer(src):
    sid = m.group(1)
    body = m.group(2)
    bytes_list = []
    for tok in body.split(","):
        tok = tok.strip()
        if tok.startswith("0x"):
            bytes_list.append(int(tok, 16))
    raw = bytes(bytes_list)
    # Strip trailing null
    if raw.endswith(b"\x00"):
        raw = raw[:-1]
    str_table[sid] = raw

print(f"Parsed {len(str_table)} string entries", file=sys.stderr)

# Decode each to Shift-JIS then escape for C++ string literal
def to_cpp_literal(b):
    try:
        decoded = b.decode("shift_jis")
    except UnicodeDecodeError:
        # Should never happen in this file; fall back to raw escapes
        return '"' + "".join(f"\\x{c:02x}" for c in b) + '"'
    # Escape special chars
    escaped = decoded.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{escaped}"'

# Build replacement table: STR(NNNN) -> literal
# Some entries are referenced via the makeSimpleEffect / templates with STR()
# Replace STR(NNNN) usages.
def replace_str(match):
    sid = match.group(1)
    if sid in str_table:
        return to_cpp_literal(str_table[sid])
    return match.group(0)

# Drop the static byte-array declarations
src = pattern.sub("", src)

# Replace STR(NNNN) with literals
src = re.sub(r"STR\((\d+)\)", replace_str, src)

# Replace MATCH(NNNN) with strcmp(name, "LITERAL") == 0
def replace_match(match):
    sid = match.group(1)
    if sid in str_table:
        lit = to_cpp_literal(str_table[sid])
        return f"strcmp(name, {lit}) == 0"
    return match.group(0)
src = re.sub(r"MATCH\((\d+)\)", replace_match, src)

# Drop the now-unused MATCH/STR macros if they're only macros
src = re.sub(r"^#define STR\(id\).*\n", "", src, flags=re.M)
src = re.sub(r"^#define MATCH\(id\).*\n", "", src, flags=re.M)

# Collapse runs of 3+ blank lines into 2
src = re.sub(r"\n{3,}", "\n\n", src)

src_path.write_text(src, encoding="utf-8")
print(f"Rewrote {src_path}", file=sys.stderr)
print(f"New line count: {len(src.splitlines())}", file=sys.stderr)
