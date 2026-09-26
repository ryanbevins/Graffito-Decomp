#!/usr/bin/env python3
# usage: python3 tools/agent/anchor_casts.py <src.cpp> <header.hpp> <ClassName> [--min OFF]
"""Rewrite `((u8*)this + 0xNN)` into member-anchored addresses.

MWCC hoists cast-pointer arithmetic like `(u8*)this + 0x42A0` out of loops
as a loop-invariant `addi` register, while member/array addressing folds the
offset into d-form loads (retail shape). Maps each offset to the containing
top-level member (from `/* 0xNN */` or `// 0xNN` header comments) and emits
`(u8*)&member [+ delta]`. Offsets in unions/nested structs or below --min
(base-class range) are left untouched. Always measure the TU before/after.
"""
import re
import sys


def members(header, cls):
    h = open(header, errors="ignore").read()
    st = h.index("class " + cls)
    body = h[st:h.index("\n};", st)]
    out = []
    for line in body.split("\n"):
        m = re.match(r"^\t/\* 0x([0-9A-Fa-f]+) \*/ (.*)$", line)
        if m:
            off, rest = int(m.group(1), 16), m.group(2)
        else:
            m = re.match(r"^\t(\S.*?);\s*// 0x([0-9A-Fa-f]+)", line)
            if not m:
                continue
            off, rest = int(m.group(2), 16), m.group(1) + ";"
        mm = re.match(r"[\w:<>*\s]+?[\s*](\w+)(\[[^\]]*\])?;", rest)
        if rest.startswith(("union", "struct")) or not mm:
            out.append((off, None, False))
        else:
            out.append((off, mm.group(1), bool(mm.group(2))))
    return sorted(out)


def main():
    src, header, cls = sys.argv[1:4]
    lo = int(sys.argv[5], 16) if len(sys.argv) > 5 and sys.argv[4] == "--min" else 0
    mem = members(header, cls)
    s = open(src, errors="ignore").read()
    cnt = [0, 0]

    def sub(m):
        off = int(m.group(1), 16)
        best = None
        for o, n, a in mem:
            if o <= off:
                best = (o, n, a)
        if off < lo or not best or best[1] is None:
            cnt[1] += 1
            return m.group(0)
        o, n, a = best
        cnt[0] += 1
        base = "(u8*)%s" % n if a else "(u8*)&%s" % n
        return "(%s)" % base if off == o else "(%s + 0x%X)" % (base, off - o)

    s = re.sub(r"\(\(u8\*\)this \+ 0x([0-9A-Fa-f]+)\)", sub, s)
    open(src, "w").write(s)
    print("rewrote %d, kept %d" % tuple(cnt))


if __name__ == "__main__":
    main()
