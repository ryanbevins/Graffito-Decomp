# usage: python tools/agent/tvec3_copy_sweep.py <unit> <src.cpp> [--apply] [--reverse]
# Greedy per-site probe of the TVec3 copy-form lever (fact:tvec3-copy-form-selects-fusion-2372):
# rewrites `TVec3<f32> v = <lvalue>;` to `TVec3<f32> v; v = <lvalue>;` one site at a time,
# rebuilds the object, and keeps the rewrite only if the containing function's match % rises.
# Without --apply the source is restored at the end (report only).
# --reverse probes the opposite rewrite: `TVec3<f32> v;` + `v = <lvalue>;` -> `TVec3<f32> v = <lvalue>;`.
import re, subprocess, sys

unit, path = sys.argv[1], sys.argv[2]
apply = '--apply' in sys.argv
reverse = '--reverse' in sys.argv
obj = 'build/GMSJ01/src/' + path[len('src/'):].rsplit('.', 1)[0] + '.o'
DECL = re.compile(r'^(\s*)((?:const\s+)?JGeometry::TVec3<f32>) (\w+)\s*=\s*([\w\->\.\[\]\*\(\)]+);\s*$')
BARE = re.compile(r'^(\s*)((?:const\s+)?JGeometry::TVec3<f32>) (\w+);\s*$')
FN = re.compile(r'^[A-Za-z][\w\s\*&:<>,]*?\b(\w+)::(~?\w+)\(')


def build():
    r = subprocess.run(['ninja', obj], capture_output=True, text=True)
    return r.returncode == 0


def score(fn):
    out = subprocess.run(['python', 'tools/decomp-diff.py', '-u', unit, '-d', fn],
                         capture_output=True, text=True).stdout
    m = re.search(r'(\d+\.\d+)% match', out)
    return float(m.group(1)) if m else None


orig = open(path).read()
lines = orig.split('\n')


def fn_of(i):
    for j in range(i, -1, -1):
        m = FN.match(lines[j])
        if m:
            return m.group(1) + '::' + m.group(2)


build()
cur = {}
i = 0
while i < len(lines):
    if reverse:
        m = BARE.match(lines[i])
        if m and i + 1 < len(lines):
            a = re.match(r'^\s*' + m.group(3) + r'\s*=\s*([\w\->\.\[\]\*\(\)]+);\s*$', lines[i + 1])
            if a and '(' not in a.group(1):
                fn = fn_of(i)
                if fn and fn not in cur:
                    cur[fn] = score(fn)
                if fn and cur[fn] is not None and cur[fn] < 100.0:
                    saved = lines[i:i + 2]
                    lines[i:i + 2] = [f'{m.group(1)}{m.group(2)} {m.group(3)} = {a.group(1)};']
                    open(path, 'w').write('\n'.join(lines))
                    new = score(fn) if build() else None
                    if new is not None and new > cur[fn]:
                        print(f'KEEP {i+1} {fn} {cur[fn]} -> {new}', flush=True)
                        cur[fn] = new
                    else:
                        print(f'drop {i+1} {fn} {cur[fn]} -> {new}', flush=True)
                        lines[i:i + 1] = saved
        i += 1
        continue
    m = DECL.match(lines[i])
    # skip temporaries / calls: only plain lvalues (no parentheses unless a deref cast)
    if not m or '(' in m.group(4):
        i += 1
        continue
    fn = fn_of(i)
    if fn is None:
        i += 1
        continue
    if fn not in cur:
        cur[fn] = score(fn)
    if cur[fn] is None or cur[fn] >= 100.0:
        i += 1
        continue
    ind, ty, name, src = m.groups()
    saved = lines[i]
    lines[i] = f'{ind}{ty} {name};\n{ind}{name} = {src};'
    open(path, 'w').write('\n'.join(lines))
    new = score(fn) if build() else None
    if new is not None and new > cur[fn]:
        print(f'KEEP {i+1} {fn} {cur[fn]} -> {new}', flush=True)
        cur[fn] = new
    else:
        print(f'drop {i+1} {fn} {cur[fn]} -> {new}', flush=True)
        lines[i] = saved
    i += 1

final = '\n'.join(lines)
if not apply:
    final = orig
open(path, 'w').write(final)
build()
