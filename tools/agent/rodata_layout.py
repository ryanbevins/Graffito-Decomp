#!/usr/bin/env python3
# usage: python tools/agent/rodata_layout.py <unit path e.g. GC2D/ConsoleStr> [N]
"""Compare the first N .rodata object (offset,size) pairs of retail asm vs our built object.
Useful for spotting missing infectious strings / rodata-ordering residue."""
import re, subprocess, sys
rel = sys.argv[1]; n = int(sys.argv[2]) if len(sys.argv) > 2 else 8
t = open(f'build/GMSJ01/asm/{rel}.s', errors='ignore').read()
ret = [(int(o, 16), int(s, 16)) for o, s in re.findall(r'# \.rodata:(0x[0-9A-Fa-f]+) \| \S+ \| size: (0x[0-9A-Fa-f]+)', t) if int(s, 16)]
out = subprocess.run(['build/binutils/powerpc-eabi-objdump', '-t', f'build/GMSJ01/src/{rel}.o'], capture_output=True, text=True).stdout
ours = sorted((int(l.split()[0], 16), int(l.split()[4], 16)) for l in out.splitlines() if ' O .rodata' in l)
ok = True
for i in range(n):
    a = ret[i] if i < len(ret) else None; b = ours[i] if i < len(ours) else None
    if a is None and b is None: break
    mark = ' ' if a == b else '!'; ok &= a == b
    print(f'{mark} retail {a}  ours {b}')
sys.exit(0 if ok else 1)
