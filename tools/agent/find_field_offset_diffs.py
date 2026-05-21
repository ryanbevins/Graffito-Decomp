#!/usr/bin/env python3
# usage: python3 tools/agent/find_field_offset_diffs.py [--min-pct 95] [--max-results 30]
#
# Scans report.json for near-match functions whose only mismatches are
# load/store instructions with different field offsets (e.g.
# `lha r4, 0x1d90(r3)` vs `lha r4, 0x1da4(r3)`). These typically signal
# a struct-layout bug in headers — a missing, extra, or reordered field
# in a class chain.
#
# Prints top candidates with the differing offsets so you can grep
# headers for the right field/struct.
import json
import os
import re
import subprocess
import sys

MIN_PCT = 95.0
MAX_RESULTS = 30


def normalize(ins):
    if ins is None:
        return None
    o = ins.get('instruction', {}) or {}
    s = o.get('formatted', '')
    # mnemonic and operands
    return s


def insn_key(ins):
    """Return (mnemonic, base_register_pattern) for matching same instruction shape."""
    if not ins:
        return None
    o = ins.get('instruction', {}) or {}
    f = o.get('formatted', '')
    parts = f.split(None, 1)
    if not parts:
        return None
    mnem = parts[0]
    return mnem


def extract_offsets(formatted):
    """Find hex offsets like 0x123(rN)."""
    return re.findall(r'0x[0-9a-fA-F]+\(r\d+\)', formatted)


def main():
    args = sys.argv[1:]
    min_pct = MIN_PCT
    max_results = MAX_RESULTS
    if '--min-pct' in args:
        i = args.index('--min-pct')
        min_pct = float(args[i+1])
    if '--max-results' in args:
        i = args.index('--max-results')
        max_results = int(args[i+1])

    d = json.load(open('build/GMSJ01/report.json'))

    def iter_units(items):
        for u in items:
            if u.get('functions') or u.get('measures', {}).get('fuzzy_match_percent') is not None:
                yield u
            sub = u.get('units', u.get('children', []))
            if sub:
                yield from iter_units(sub)

    candidates = []
    for u in iter_units(d.get('units', d.get('children', []))):
        unit_name = u.get('name', '')
        for f in u.get('functions', []):
            pct = f.get('fuzzy_match_percent', 0)
            if pct < min_pct or pct >= 100.0:
                continue
            name = f.get('name', '')
            if '__sinit' in name or name.startswith('@'):
                continue
            candidates.append((unit_name, name, pct, f.get('size', 0)))

    candidates.sort(key=lambda x: (-x[2], x[3]))

    print(f"Scanning {len(candidates)} candidates at >= {min_pct}% fuzzy...\n")
    found = 0
    for unit_name, sym, pct, sz in candidates:
        # Strip "mario/" prefix to match what symbol_diff expects
        unit_path = unit_name
        for prefix in ('mario/', 'sdk/', 'jsystem/'):
            if unit_path.startswith(prefix):
                unit_path = unit_path[len(prefix):]
                break
        target = f"build/GMSJ01/obj/{unit_path}.o"
        base = f"build/GMSJ01/src/{unit_path}.o"
        if not (os.path.exists(target) and os.path.exists(base)):
            continue
        try:
            out = subprocess.run(
                ["build/tools/objdiff-cli", "diff", "-1", target, "-2", base, sym,
                 "--format", "json", "-o", "-"],
                capture_output=True, text=True, timeout=8)
            if out.returncode != 0:
                continue
            dd = json.loads(out.stdout)
        except Exception:
            continue

        def find_sym(syms, name):
            for s in syms:
                if s.get('name') == name:
                    return s
            return None

        left = find_sym(dd.get('left', {}).get('symbols', []), sym)
        right = find_sym(dd.get('right', {}).get('symbols', []), sym)
        if not left or not right:
            continue

        li = left.get('instructions', [])
        ri = right.get('instructions', [])
        if len(li) != len(ri):
            continue
        offset_diffs = []
        other_diffs = 0
        for i, (l, r) in enumerate(zip(li, ri)):
            lf = (l.get('instruction', {}) or {}).get('formatted', '')
            rf = (r.get('instruction', {}) or {}).get('formatted', '')
            if lf == rf:
                continue
            # check if it's just an offset diff
            l_norm = re.sub(r'0x[0-9a-fA-F]+\(r\d+\)', 'OFFSET(rN)', lf)
            r_norm = re.sub(r'0x[0-9a-fA-F]+\(r\d+\)', 'OFFSET(rN)', rf)
            l_offsets = extract_offsets(lf)
            r_offsets = extract_offsets(rf)
            if l_norm == r_norm and l_offsets != r_offsets:
                # Skip stack-frame-size differences (rN(r1)) — those are
                # phantom stack inflation, not field offsets.
                is_stack = all('(r1)' in o for o in l_offsets + r_offsets)
                if is_stack:
                    other_diffs += 1  # treat as non-fixable
                else:
                    offset_diffs.append((lf, rf))
            else:
                other_diffs += 1
        if offset_diffs and other_diffs <= 2:
            found += 1
            print(f"{unit_name:40s} {sym[:55]:55s} {pct:6.2f}% other_diffs={other_diffs}")
            for lf, rf in offset_diffs[:3]:
                print(f"  target: {lf}")
                print(f"  ours:   {rf}")
            print()
            if found >= max_results:
                break


if __name__ == "__main__":
    main()
