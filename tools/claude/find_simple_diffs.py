#!/usr/bin/env python3
"""Find non-matching functions with simple diffs (few instruction-level differences)."""
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).parent.parent.parent
OBJDUMP = ROOT / 'build' / 'binutils' / 'powerpc-eabi-objdump.exe'

def disasm(path, func):
    out = subprocess.run([str(OBJDUMP), '-d', '-r', str(path)], capture_output=True, text=True).stdout
    in_f = False
    res = []
    for l in out.split('\n'):
        if f'<{func}>:' in l:
            in_f = True
            continue
        elif in_f:
            if l.strip() == '':
                break
            if l and not l[0].isspace() and '<' in l and l.rstrip().endswith(':'):
                break
            parts = l.split('\t')
            if len(parts) >= 3:
                raw_bytes = parts[1].strip()
                res.append(raw_bytes)
    return res

def count_diffs(unit_name, fn_name):
    relpath = unit_name.replace('mario/', '') + '.o'
    orig = ROOT / 'build' / 'GMSJ01' / 'obj' / relpath
    comp = ROOT / 'build' / 'GMSJ01' / 'src' / relpath
    if not orig.exists() or not comp.exists():
        return None, None
    o = disasm(orig, fn_name)
    c = disasm(comp, fn_name)
    if len(o) != len(c) or len(o) == 0:
        return None, None
    diffs = sum(1 for a, b in zip(o, c) if a.split() != b.split())
    return diffs, len(o)

def main():
    with open(ROOT / 'build/GMSJ01/report.json') as f:
        r = json.load(f)

    results = []
    for u in r.get('units', []):
        name = u.get('name', '')
        if 'JSystem' in name or '/sdk/' in name or 'PowerPC_EABI' in name or 'TRK_' in name:
            continue
        for func in u.get('functions', []):
            fuzzy = func.get('fuzzy_match_percent', 0)
            size = int(func.get('size', 0) or 0)
            if 95 <= fuzzy < 100 and 50 < size < 600:
                results.append((fuzzy, size, name, func['name']))

    # Process in order of highest fuzzy first
    results.sort(key=lambda c: (-c[0], c[1]))

    simple = []
    for fuzzy, size, unit, fname in results[:300]:
        diffs, total = count_diffs(unit, fname)
        if diffs is not None and 0 < diffs <= 3:
            simple.append((diffs, fuzzy, total, size, unit, fname))

    simple.sort(key=lambda c: (c[0], -c[1]))
    print(f'{"diffs":>5} {"fuzzy":>6} {"instr":>5} {"size":>4}  unit  ::  func')
    for diffs, fuzzy, total, size, unit, fname in simple[:50]:
        print(f'  {diffs:>3}  {fuzzy:>6.2f}  {total:>5}  {size:>4}  {unit}  ::  {fname}')

if __name__ == '__main__':
    main()
