#!/usr/bin/env python3
"""
Inline Matrix Builder - Differential Compilation Approach

For a target TU, systematically analyzes stack frame gaps, relocation
differences, and weak symbol mismatches to identify missing inline functions.

Usage:
  python tools/inline_matrix.py Player/MarioMove          # full analysis
  python tools/inline_matrix.py Player/MarioMove --test    # run differential tests
"""

import subprocess
import json
import re
import sys
import os

OBJDUMP = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        'build', 'binutils', 'powerpc-eabi-objdump.exe')


def run_cmd(args, timeout=120):
    """Run a command and return stdout."""
    r = subprocess.run(args, capture_output=True, text=True, timeout=timeout)
    return r.stdout


def get_stack_frames(obj_path):
    """Extract function name -> stack frame size from an object file."""
    output = run_cmd([OBJDUMP, '-d', obj_path])
    frames = {}
    current_func = None
    for line in output.split('\n'):
        m = re.match(r'^[0-9a-f]+ <(.+)>:', line)
        if m:
            current_func = m.group(1)
        if current_func and 'stwu' in line:
            m2 = re.search(r'stwu\s+r1,(-?\d+)\(r1\)', line)
            if m2:
                frames[current_func] = int(m2.group(1))
    return frames


def get_relocations(obj_path, func_name):
    """Get relocation targets for a specific function."""
    output = run_cmd([OBJDUMP, '-dr', obj_path])
    in_func = False
    relocs = []
    for line in output.split('\n'):
        if f'<{func_name}>' in line and ':' in line:
            in_func = True
            continue
        if in_func:
            if line.strip() == '' or (re.match(r'^[0-9a-f]+ <', line) and func_name not in line):
                break
            m = re.search(r'R_PPC_REL24\s+(.+)', line)
            if m:
                relocs.append(m.group(1).strip())
    return relocs


def get_all_relocations(obj_path):
    """Get all relocations grouped by function."""
    output = run_cmd([OBJDUMP, '-dr', obj_path])
    func_relocs = {}
    current_func = None
    for line in output.split('\n'):
        m = re.match(r'^[0-9a-f]+ <(.+)>:', line)
        if m:
            current_func = m.group(1)
            if current_func not in func_relocs:
                func_relocs[current_func] = []
        if current_func:
            m2 = re.search(r'R_PPC_REL24\s+(.+)', line)
            if m2:
                func_relocs[current_func].append(m2.group(1).strip())
    return func_relocs


def get_weak_symbols(obj_path):
    """Get all weak function symbols: name -> size."""
    output = run_cmd([OBJDUMP, '-t', obj_path])
    weaks = {}
    for line in output.split('\n'):
        if '  w  ' in line and ' F ' in line:
            parts = line.split()
            if len(parts) >= 6:
                size = int(parts[4], 16)
                name = parts[5]
                weaks[name] = size
    return weaks


def get_instruction_counts(obj_path):
    """Get function name -> instruction count."""
    output = run_cmd([OBJDUMP, '-d', obj_path])
    counts = {}
    current_func = None
    count = 0
    for line in output.split('\n'):
        m = re.match(r'^[0-9a-f]+ <(.+)>:', line)
        if m:
            if current_func:
                counts[current_func] = count
            current_func = m.group(1)
            count = 0
        elif current_func and re.match(r'\s+[0-9a-f]+:', line):
            count += 1
    if current_func:
        counts[current_func] = count
    return counts


def analyze_tu(tu_name):
    """Run complete analysis on a translation unit."""
    orig_obj = f'build/GMSJ01/obj/{tu_name}.o'
    comp_obj = f'build/GMSJ01/src/{tu_name}.o'

    if not os.path.exists(orig_obj):
        print(f"ERROR: {orig_obj} not found")
        return
    if not os.path.exists(comp_obj):
        print(f"ERROR: {comp_obj} not found")
        return

    print(f"{'='*70}")
    print(f"INLINE ANALYSIS: {tu_name}")
    print(f"{'='*70}")

    # Stack frames
    orig_frames = get_stack_frames(orig_obj)
    comp_frames = get_stack_frames(comp_obj)

    # Instruction counts
    orig_icounts = get_instruction_counts(orig_obj)
    comp_icounts = get_instruction_counts(comp_obj)

    # Weak symbols
    orig_weaks = get_weak_symbols(orig_obj)
    comp_weaks = get_weak_symbols(comp_obj)

    # Relocations
    orig_relocs = get_all_relocations(orig_obj)
    comp_relocs = get_all_relocations(comp_obj)

    # --- Report ---

    # 1. Stack gaps
    print(f"\n{'-'*70}")
    print("STACK FRAME GAPS (original - compiled)")
    print(f"{'-'*70}")
    print(f"{'Function':<45} {'Orig':>6} {'Comp':>6} {'Gap':>6} {'#Inl':>5}")
    print(f"{'-'*45} {'-'*6} {'-'*6} {'-'*6} {'-'*5}")

    gap_funcs = []
    for func in sorted(orig_frames.keys()):
        if func in comp_frames:
            orig_s = orig_frames[func]
            comp_s = comp_frames[func]
            gap = orig_s - comp_s  # negative means original is larger (needs more stack)
            if gap != 0:
                n_inlines = abs(gap) // 8
                direction = "TOO BIG" if gap > 0 else ""
                print(f"  {func:<43} {orig_s:>6} {comp_s:>6} {gap:>+6} {n_inlines:>5} {direction}")
                gap_funcs.append((func, orig_s, comp_s, gap))

    # 2. Instruction count diffs
    print(f"\n{'-'*70}")
    print("INSTRUCTION COUNT DIFFS")
    print(f"{'-'*70}")
    icount_diffs = []
    for func in sorted(orig_icounts.keys()):
        if func in comp_icounts:
            diff = comp_icounts[func] - orig_icounts[func]
            if diff != 0:
                print(f"  {func:<50} orig={orig_icounts[func]:>4} comp={comp_icounts[func]:>4} diff={diff:>+4}")
                icount_diffs.append((func, diff))

    if not icount_diffs:
        print("  (all instruction counts match)")

    # 3. Relocation diffs
    print(f"\n{'-'*70}")
    print("RELOCATION DIFFS (function call mismatches)")
    print(f"{'-'*70}")
    has_reloc_diff = False
    for func in sorted(orig_relocs.keys()):
        if func in comp_relocs:
            orig_set = sorted(orig_relocs[func])
            comp_set = sorted(comp_relocs[func])
            if orig_set != comp_set:
                has_reloc_diff = True
                print(f"  {func}:")
                orig_counter = {}
                comp_counter = {}
                for r in orig_set:
                    orig_counter[r] = orig_counter.get(r, 0) + 1
                for r in comp_set:
                    comp_counter[r] = comp_counter.get(r, 0) + 1
                all_targets = set(list(orig_counter.keys()) + list(comp_counter.keys()))
                for t in sorted(all_targets):
                    oc = orig_counter.get(t, 0)
                    cc = comp_counter.get(t, 0)
                    if oc != cc:
                        if oc > cc:
                            print(f"    MISSING call (orig has {oc}x, we have {cc}x): {t}")
                        else:
                            print(f"    EXTRA call (orig has {oc}x, we have {cc}x): {t}")
    if not has_reloc_diff:
        print("  (all relocations match)")

    # 4. Weak symbol diffs
    print(f"\n{'-'*70}")
    print("WEAK SYMBOL DIFFS (inline function emission)")
    print(f"{'-'*70}")
    missing = set(orig_weaks.keys()) - set(comp_weaks.keys())
    extra = set(comp_weaks.keys()) - set(orig_weaks.keys())

    if missing:
        print("  MISSING (original emits, we don't):")
        for w in sorted(missing):
            print(f"    {w} ({orig_weaks[w]} bytes)")
    if extra:
        print("  EXTRA (we emit, original doesn't):")
        for w in sorted(extra):
            print(f"    {w} ({comp_weaks[w]} bytes)")
    if not missing and not extra:
        print("  (all weak symbols match)")

    # 5. Summary
    print(f"\n{'-'*70}")
    print("SUMMARY")
    print(f"{'-'*70}")
    total_gap = sum(abs(g) for _, _, _, g in gap_funcs)
    print(f"  Functions with stack gaps: {len(gap_funcs)}")
    print(f"  Total absolute gap: {total_gap} bytes")
    print(f"  Estimated missing inline calls: {total_gap // 8}")
    print(f"  Functions with instruction diffs: {len(icount_diffs)}")
    print(f"  Functions with relocation diffs: {1 if has_reloc_diff else 0}")
    print(f"  Missing weak symbols: {len(missing)}")
    print(f"  Extra weak symbols: {len(extra)}")

    return {
        'gaps': gap_funcs,
        'icount_diffs': icount_diffs,
        'missing_weaks': list(missing),
        'extra_weaks': list(extra),
    }


def scan_all_tus():
    """Scan all original .o files for weak symbols to build global inline database."""
    print("Scanning all TUs for weak symbols...")
    obj_dir = 'build/GMSJ01/obj'
    all_weaks = {}  # symbol -> list of TUs

    for root, dirs, files in os.walk(obj_dir):
        for f in files:
            if f.endswith('.o'):
                obj_path = os.path.join(root, f)
                tu_name = os.path.relpath(obj_path, obj_dir).replace('.o', '').replace('\\', '/')
                weaks = get_weak_symbols(obj_path)
                for sym, size in weaks.items():
                    if sym not in all_weaks:
                        all_weaks[sym] = {'size': size, 'tus': []}
                    all_weaks[sym]['tus'].append(tu_name)

    # Sort by number of TUs (most shared first)
    print(f"\n{'='*70}")
    print(f"GLOBAL INLINE DATABASE ({len(all_weaks)} unique weak symbols)")
    print(f"{'='*70}")
    print(f"{'Symbol':<60} {'Size':>6} {'TUs':>4}")
    print(f"{'-'*60} {'-'*6} {'-'*4}")

    for sym, info in sorted(all_weaks.items(), key=lambda x: -len(x[1]['tus'])):
        if len(info['tus']) >= 3:  # Only show widely-shared inlines
            print(f"  {sym:<58} {info['size']:>6} {len(info['tus']):>4}")

    return all_weaks


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python tools/inline_matrix.py <TU_NAME> [--scan-all]")
        print("  e.g.: python tools/inline_matrix.py Player/MarioMove")
        print("        python tools/inline_matrix.py --scan-all")
        sys.exit(1)

    if sys.argv[1] == '--scan-all':
        scan_all_tus()
    else:
        analyze_tu(sys.argv[1])
