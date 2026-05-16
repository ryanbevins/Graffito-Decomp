#!/usr/bin/env python3
"""Diff a function in original vs compiled obj."""
import subprocess
import sys
from pathlib import Path

def disasm(objdump, path, func):
    out = subprocess.run([str(objdump), '-d', '-r', str(path)], capture_output=True, text=True).stdout
    lines = out.split('\n')
    in_f = False
    res = []  # list of (raw_bytes, instr_text)
    for l in lines:
        if f'<{func}>:' in l:
            in_f = True
            continue
        elif in_f:
            if l.strip() == '':
                break
            # Function header lines like "00000000 <name>:" — break only on those
            if l and not l[0].isspace() and '<' in l and l.rstrip().endswith(':'):
                break
            parts = l.split('\t')
            if len(parts) >= 3:
                raw_bytes = parts[1].strip()  # "7c 08 02 a6 "
                instr = '\t'.join(parts[2:]).strip()
                if ' <' in instr:
                    instr = instr.split(' <')[0].strip()
                res.append((raw_bytes, instr))
            elif l.strip().startswith('R_'):
                # relocation row — attach to previous instruction
                if res:
                    raw_bytes, instr = res[-1]
                    reloc = l.strip()
                    res[-1] = (raw_bytes, f'{instr} ; {reloc}')
            else:
                res.append(('', l.strip()))
    return res

def main():
    if len(sys.argv) < 3:
        print("Usage: diff_func.py <path> <func>")
        sys.exit(1)

    path = sys.argv[1].replace('\\', '/')
    func = sys.argv[2]
    root = Path(__file__).parent.parent.parent
    objdump = root / 'build' / 'binutils' / 'powerpc-eabi-objdump.exe'
    orig_obj = root / 'build' / 'GMSJ01' / 'obj' / f'{path}.o'
    comp_obj = root / 'build' / 'GMSJ01' / 'src' / f'{path}.o'

    orig = disasm(objdump, orig_obj, func)
    comp = disasm(objdump, comp_obj, func)
    print(f'Orig: {len(orig)} instr  Comp: {len(comp)} instr')

    n = max(len(orig), len(comp))
    diffs = 0
    for i in range(n):
        o = orig[i] if i < len(orig) else (None, '<missing>')
        c = comp[i] if i < len(comp) else (None, '<missing>')
        ob, ot = o
        cb, ct = c
        # Compare raw bytes when available; otherwise fall back to text.
        if (ob is not None and cb is not None) and ob.split() == cb.split():
            continue
        if ob is None and cb is None and ot == ct:
            continue
        print(f'  {i*4:#06x}: ORIG: {ot:50}  COMP: {ct}')
        diffs += 1
        if diffs > 50:
            print('... truncated')
            break
    if diffs == 0:
        print('MATCH')

if __name__ == '__main__':
    main()
