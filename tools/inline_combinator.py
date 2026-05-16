#!/usr/bin/env python3
"""
Inline Combinator - Find exact stack-gap solutions via accessor combinations.

Reads the inline_solver cache (or does a fresh scan) and finds subsets of
safe accessors whose combined stack delta exactly matches the gap for each
non-matching function. Tests the best combinations by applying them, building,
and checking for regressions.

Each fabricated inline accessor adds -8 bytes to the compiled stack frame.
If a function's gap is -N, we need N/8 accessors whose effects on that
function sum to exactly -N.

Usage:
  python tools/inline_combinator.py Player/MarioMove              # analyze gaps
  python tools/inline_combinator.py Player/MarioMove --apply      # find and apply solutions
  python tools/inline_combinator.py Player/MarioMove funcName     # single function
"""

import subprocess
import re
import sys
import os
import json
import time
import hashlib
from itertools import combinations

OBJDUMP = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        'build', 'binutils', 'powerpc-eabi-objdump.exe')


# ============================================================
# HELPERS (shared with inline_solver.py)
# ============================================================

def get_stack_frames(obj_path):
    r = subprocess.run([OBJDUMP, '-d', obj_path], capture_output=True, text=True, timeout=60)
    frames = {}
    func = None
    for line in r.stdout.split('\n'):
        m = re.match(r'^[0-9a-f]+ <(.+)>:', line)
        if m:
            func = m.group(1)
        if func and 'stwu' in line:
            m2 = re.search(r'stwu\s+r1,(-?\d+)\(r1\)', line)
            if m2:
                frames[func] = int(m2.group(1))
    return frames


def get_exact_matches(tu_name):
    """Compare original vs compiled object files, return dict func->bool."""
    orig_obj = f'build/GMSJ01/obj/{tu_name}.o'
    comp_obj = f'build/GMSJ01/src/{tu_name}.o'

    orig_out = subprocess.run([OBJDUMP, '-d', orig_obj], capture_output=True, text=True, timeout=60).stdout
    comp_out = subprocess.run([OBJDUMP, '-d', comp_obj], capture_output=True, text=True, timeout=60).stdout

    def extract_functions(text):
        funcs = {}
        current = None
        insns = []
        for line in text.split('\n'):
            m = re.match(r'^[0-9a-f]+ <(.+)>:', line)
            if m:
                if current:
                    funcs[current] = insns
                current = m.group(1)
                insns = []
            elif current:
                bm = re.match(r'\s+[0-9a-f]+:\s+((?:[0-9a-f]{2} )+)', line)
                if bm:
                    insns.append(bm.group(1).strip())
        if current:
            funcs[current] = insns
        return funcs

    orig_funcs = extract_functions(orig_out)
    comp_funcs = extract_functions(comp_out)

    exact = {}
    for func in orig_funcs:
        if func in comp_funcs:
            exact[func] = (orig_funcs[func] == comp_funcs[func])
    return exact


def build():
    subprocess.run(['python', 'configure.py'], capture_output=True, text=True, timeout=30)
    r = subprocess.run(['python', '-m', 'ninja'], capture_output=True, text=True, timeout=60)
    return r.returncode == 0


def replace_reads(source, field, accessor):
    """Replace all READ occurrences of field with accessor call."""
    lines = source.split('\n')
    result = []
    count = 0

    for idx, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith('//') or stripped.startswith('/*'):
            result.append(line)
            continue
        if '/* 0x' in line or 'PARAM_INIT' in line or 'const {' in line:
            result.append(line)
            continue
        if 'return ' + field in line and '()' not in line.split(field)[0][-5:]:
            if 'get' in line.lower() and '{' in line:
                result.append(line)
                continue

        next_stripped = lines[idx + 1].lstrip() if idx + 1 < len(lines) else ''

        if '.' in field:
            pattern = re.escape(field)
        else:
            pattern = r'(?<![.\w])' + re.escape(field) + r'(?![.\w(])'

        new_line = line
        offset = 0
        for m in re.finditer(pattern, line):
            start, end = m.start(), m.end()

            if start >= 2 and line[start-2:start] == '->':
                continue

            rest = line[end:].lstrip()
            is_write = False
            if rest and rest[0] == '=' and (len(rest) < 2 or rest[1] != '='):
                is_write = True
            elif len(rest) >= 2 and rest[0] in '+-|&*' and rest[1] == '=':
                is_write = True
            elif not rest:
                if next_stripped.startswith('=') and not next_stripped.startswith('=='):
                    is_write = True
                elif len(next_stripped) >= 2 and next_stripped[0] in '+-|&*' and next_stripped[1] == '=':
                    is_write = True

            if is_write:
                continue

            adj_start = start + offset
            adj_end = end + offset
            new_line = new_line[:adj_start] + accessor + new_line[adj_end:]
            offset += len(accessor) - (end - start)
            count += 1

        result.append(new_line)

    return '\n'.join(result), count


def detect_header(tu_name):
    src_file = f'src/{tu_name}.cpp'
    with open(src_file) as f:
        for line in f:
            m = re.match(r'#include\s*<(.+\.hpp)>', line)
            if m:
                path = f'include/{m.group(1)}'
                if os.path.exists(path):
                    return path
    return 'include/Player/MarioMain.hpp'


def detect_class_name(header_file):
    with open(header_file) as f:
        for line in f:
            m = re.match(r'\s*class\s+(\w+)\s*[:{]', line)
            if m:
                return m.group(1)
    return 'TMario'


# ============================================================
# CACHE LOADING
# ============================================================

def load_cache(tu_name):
    """Load the inline_solver cache file. Returns None if not found/stale."""
    cache_file = f'tools/inline_cache_{tu_name.replace("/", "_")}.json'
    if not os.path.exists(cache_file):
        return None
    with open(cache_file) as f:
        return json.load(f)


def load_or_scan(tu_name, header_file):
    """Load cache or run inline_solver to generate it."""
    cache = load_cache(tu_name)
    if cache is not None:
        print(f"Loaded cache: {len(cache.get('match_makers', {}))} match-makers, "
              f"{len(cache.get('safe', {}))} safe accessors")
        return cache

    print("No cache found. Running inline_solver to generate...")
    r = subprocess.run(
        ['python', 'tools/inline_solver.py', tu_name, f'--header={header_file}'],
        capture_output=False, text=True, timeout=600
    )
    cache = load_cache(tu_name)
    if cache is None:
        print("ERROR: inline_solver did not produce a cache file!")
        sys.exit(1)
    return cache


# ============================================================
# COMBINATION FINDER
# ============================================================

def build_accessor_index(cache):
    """Build a per-function index: func -> list of (accessor_name, delta).

    Each entry means: applying accessor_name changes func's stack by delta.
    Only includes accessors from 'match_makers' and 'safe' pools.
    """
    all_accessors = {}
    for name, info in cache.get('match_makers', {}).items():
        all_accessors[name] = info
    for name, info in cache.get('safe', {}).items():
        all_accessors[name] = info

    # func -> [(accessor_name, delta_for_this_func)]
    func_index = {}
    for acc_name, info in all_accessors.items():
        for func, delta in info['stacks'].items():
            if func not in func_index:
                func_index[func] = []
            func_index[func].append((acc_name, delta))

    return all_accessors, func_index


def find_exact_subsets(func_name, gap, candidates, max_size=8):
    """Find all subsets of candidates whose deltas sum exactly to the gap.

    candidates: list of (accessor_name, delta) for this function
    gap: target sum (negative number, e.g. -48)
    max_size: maximum subset size to try

    Returns list of subsets, sorted by size (smallest first).
    Each subset is a frozenset of accessor names.
    """
    # Filter to candidates that contribute in the right direction
    # gap is negative (we need MORE stack), deltas are negative (each adds stack)
    useful = [(name, d) for name, d in candidates if d < 0]

    if not useful:
        return []

    solutions = []

    # Try increasing subset sizes, smallest first (greedy)
    for size in range(1, min(max_size, len(useful)) + 1):
        for combo in combinations(useful, size):
            total = sum(d for _, d in combo)
            if total == gap:
                names = frozenset(n for n, _ in combo)
                solutions.append(names)

        # If we found solutions at this size, no need for larger ones
        if solutions:
            break

    return solutions


def find_all_solutions(cache, target_func=None):
    """Find exact-match subsets for all gapped functions.

    Returns dict: func_name -> list of solution subsets (frozensets of accessor names)
    """
    gaps = cache.get('gaps', {})
    all_accessors, func_index = build_accessor_index(cache)

    if target_func:
        # Find the full mangled name
        matched = [f for f in gaps if target_func in f]
        if not matched:
            print(f"No gapped function matching '{target_func}'")
            return {}
        gaps = {f: gaps[f] for f in matched}

    solutions = {}
    for func, gap in sorted(gaps.items(), key=lambda x: abs(x[1])):
        candidates = func_index.get(func, [])
        if not candidates:
            continue

        subsets = find_exact_subsets(func, gap, candidates)
        if subsets:
            solutions[func] = subsets

    return solutions


# ============================================================
# CROSS-FUNCTION ANALYSIS
# ============================================================

def check_combo_side_effects(combo_names, cache):
    """Check what side effects a combination of accessors has on OTHER functions.

    Returns dict: func -> total_delta for functions not in the target.
    """
    all_accessors = {}
    for name, info in cache.get('match_makers', {}).items():
        all_accessors[name] = info
    for name, info in cache.get('safe', {}).items():
        all_accessors[name] = info

    side_effects = {}
    for acc_name in combo_names:
        if acc_name not in all_accessors:
            continue
        for func, delta in all_accessors[acc_name]['stacks'].items():
            side_effects[func] = side_effects.get(func, 0) + delta

    return side_effects


def rank_solutions(func, solutions, cache):
    """Rank solutions for a function by desirability.

    Criteria (in order):
    1. Fewer accessors (less risk)
    2. Fewer side effects on other functions
    3. More side effects that HELP other gapped functions
    """
    gaps = cache.get('gaps', {})

    ranked = []
    for combo in solutions:
        side = check_combo_side_effects(combo, cache)

        # Count how many other gapped functions are helped/hurt
        helps = 0
        hurts = 0
        neutral_side_effects = 0
        for f, d in side.items():
            if f == func:
                continue
            if f in gaps:
                remaining = gaps[f] - d
                if abs(remaining) < abs(gaps[f]):
                    helps += 1
                elif abs(remaining) > abs(gaps[f]):
                    hurts += 1
            else:
                # Non-gapped function affected -- potential regression
                neutral_side_effects += 1

        score = (
            len(combo),                # fewer is better
            hurts,                     # fewer hurts is better
            neutral_side_effects,      # fewer side effects is better
            -helps,                    # more helps is better (negative for sort)
        )
        ranked.append((score, combo, side))

    ranked.sort(key=lambda x: x[0])
    return ranked


# ============================================================
# APPLY ENGINE
# ============================================================

def apply_combination(tu_name, src_file, header_file, combo_names, cache):
    """Apply a combination of accessors, build, and check results.

    Returns (success, new_exact, details) or (False, None, reason).
    """
    all_accessors = {}
    for name, info in cache.get('match_makers', {}).items():
        all_accessors[name] = info
    for name, info in cache.get('safe', {}).items():
        all_accessors[name] = info

    with open(src_file) as f:
        src_orig = f.read()
    with open(header_file) as f:
        hdr_orig = f.read()

    try:
        # Add all accessors to header
        hdr = hdr_orig
        cls_name = detect_class_name(header_file)
        cls_idx = hdr.find(f'class {cls_name}')
        if cls_idx == -1:
            return False, None, "class not found in header"

        fab_idx = hdr.find('// Fabricated', cls_idx)
        if fab_idx == -1:
            fab_idx = hdr.find('};', cls_idx)
        pos = hdr.rfind('\n', 0, fab_idx) + 1

        block = '\t// Inline combinator accessors\n'
        for name in sorted(combo_names):
            if name not in all_accessors:
                return False, None, f"accessor '{name}' not in cache"
            block += all_accessors[name]['header'] + '\n'

        hdr = hdr[:pos] + block + '\n' + hdr[pos:]

        # Replace reads in source for each accessor
        src = src_orig
        total_replacements = 0
        for name in combo_names:
            info = all_accessors[name]
            src, count = replace_reads(src, info['field'], info['call'])
            total_replacements += count

        if total_replacements == 0:
            return False, None, "no replacements made"

        with open(header_file, 'w') as f:
            f.write(hdr)
        with open(src_file, 'w') as f:
            f.write(src)

        if not build():
            return False, None, "build failed"

        new_exact = get_exact_matches(tu_name)
        return True, new_exact, f"{total_replacements} replacements"

    finally:
        # Always revert
        with open(src_file, 'w') as f:
            f.write(src_orig)
        with open(header_file, 'w') as f:
            f.write(hdr_orig)


def apply_and_keep(tu_name, src_file, header_file, combo_names, cache):
    """Apply a combination permanently (do NOT revert). Returns new_exact or None."""
    all_accessors = {}
    for name, info in cache.get('match_makers', {}).items():
        all_accessors[name] = info
    for name, info in cache.get('safe', {}).items():
        all_accessors[name] = info

    with open(header_file) as f:
        hdr = f.read()
    with open(src_file) as f:
        src = f.read()

    cls_name = detect_class_name(header_file)
    cls_idx = hdr.find(f'class {cls_name}')
    if cls_idx == -1:
        return None

    fab_idx = hdr.find('// Fabricated', cls_idx)
    if fab_idx == -1:
        fab_idx = hdr.find('};', cls_idx)
    pos = hdr.rfind('\n', 0, fab_idx) + 1

    block = '\t// Inline combinator accessors\n'
    for name in sorted(combo_names):
        if name not in all_accessors:
            continue
        block += all_accessors[name]['header'] + '\n'

    hdr = hdr[:pos] + block + '\n' + hdr[pos:]

    total_replacements = 0
    for name in combo_names:
        info = all_accessors[name]
        src, count = replace_reads(src, info['field'], info['call'])
        total_replacements += count

    if total_replacements == 0:
        return None

    with open(header_file, 'w') as f:
        f.write(hdr)
    with open(src_file, 'w') as f:
        f.write(src)

    if not build():
        return None

    return get_exact_matches(tu_name)


# ============================================================
# MAIN
# ============================================================

def print_analysis(cache, target_func=None):
    """Print gap analysis and candidate solutions."""
    gaps = cache.get('gaps', {})
    all_accessors, func_index = build_accessor_index(cache)

    print(f"\n{'='*70}")
    print(f"STACK GAP ANALYSIS")
    print(f"{'='*70}")

    if target_func:
        matched = [f for f in gaps if target_func in f]
        if not matched:
            print(f"No gapped function matching '{target_func}'")
            return
        gaps = {f: gaps[f] for f in matched}

    for func in sorted(gaps, key=lambda x: abs(gaps[x]), reverse=True):
        gap = gaps[func]
        n_needed = abs(gap) // 8
        candidates = func_index.get(func, [])
        max_available = sum(abs(d) for _, d in candidates if d < 0) if candidates else 0

        short = func.split('__')[0]
        status = "SOLVABLE" if max_available >= abs(gap) else f"NEED MORE ({max_available}/{abs(gap)})"
        print(f"\n  {short}")
        print(f"    gap={gap} ({n_needed} accessors needed), "
              f"available={len(candidates)} candidates, {status}")

        if candidates:
            for name, d in sorted(candidates, key=lambda x: x[1]):
                print(f"      {name:<25} {d:+d}")

    # Find solutions
    solutions = find_all_solutions(cache, target_func)

    print(f"\n{'='*70}")
    print(f"EXACT SOLUTIONS FOUND: {len(solutions)} functions")
    print(f"{'='*70}")

    for func, subsets in sorted(solutions.items()):
        gap = gaps[func]
        short = func.split('__')[0]
        ranked = rank_solutions(func, subsets, cache)

        print(f"\n  {short} (gap={gap}):")
        for i, (score, combo, side) in enumerate(ranked[:3]):
            names = sorted(combo)
            # Show side effects on other gapped functions
            side_notes = []
            for f, d in side.items():
                if f != func and f in cache.get('gaps', {}):
                    fs = f.split('__')[0]
                    side_notes.append(f"{fs}:{d:+d}")

            side_str = f"  side:[{', '.join(side_notes)}]" if side_notes else ""
            print(f"    #{i+1} ({len(combo)} accessors): {', '.join(names)}{side_str}")

    return solutions


def run_apply(tu_name, cache, target_func=None):
    """Find and apply exact solutions, checking for regressions."""
    src_file = f'src/{tu_name}.cpp'
    header_file = detect_header(tu_name)
    gaps = cache.get('gaps', {})

    print(f"\n{'='*70}")
    print(f"APPLYING COMBINATIONS: {tu_name}")
    print(f"{'='*70}")

    # Ensure baseline build
    if not build():
        print("ERROR: baseline build failed!")
        return

    baseline_exact = get_exact_matches(tu_name)
    baseline_count = sum(1 for v in baseline_exact.values() if v)
    print(f"Baseline: {baseline_count} exact matches")

    solutions = find_all_solutions(cache, target_func)
    if not solutions:
        print("No exact solutions found.")
        return

    # Track which accessors we've committed to applying
    applied_accessors = set()
    applied_count = 0
    new_match_funcs = []

    # Process solutions in order of gap size (smallest first = easiest to solve)
    for func in sorted(solutions, key=lambda f: abs(gaps.get(f, 0))):
        subsets = solutions[func]
        ranked = rank_solutions(func, subsets, cache)
        short = func.split('__')[0]

        print(f"\n  Testing {short} (gap={gaps[func]})...")

        solved = False
        for score, combo, side in ranked[:3]:  # Try top 3 ranked solutions
            # Include any previously applied accessors
            test_combo = applied_accessors | combo
            names = sorted(combo - applied_accessors)
            if not names:
                # All accessors in this combo are already applied
                print(f"    All accessors already applied, checking...")

            print(f"    Trying: {', '.join(sorted(combo))}...", end='', flush=True)

            success, new_exact, details = apply_combination(
                tu_name, src_file, header_file, test_combo, cache
            )

            if not success:
                print(f" FAILED ({details})")
                continue

            # Check for regressions
            new_count = sum(1 for v in new_exact.values() if v)
            regressions = []
            improvements = []
            for f in baseline_exact:
                if f in new_exact:
                    if baseline_exact[f] and not new_exact[f]:
                        regressions.append(f)
                    elif not baseline_exact[f] and new_exact[f]:
                        improvements.append(f)

            if regressions:
                reg_short = [r.split('__')[0] for r in regressions]
                print(f" REGRESSIONS: {reg_short}")
                continue

            # Check if the target function now matches
            target_matches = new_exact.get(func, False)
            if target_matches:
                print(f" MATCH! (+{len(improvements)} total new matches)")
                applied_accessors |= combo
                applied_count += 1
                new_match_funcs.append(func)
                solved = True
                break
            elif improvements:
                imp_short = [i.split('__')[0] for i in improvements]
                print(f" no match for target, but {imp_short} improved")
                # Still worth applying if it helps other functions
                applied_accessors |= combo
                applied_count += 1
                solved = True
                break
            else:
                print(f" no improvement")

        if not solved:
            print(f"    No working solution for {short}")

    # Final application
    if applied_accessors:
        print(f"\n{'='*70}")
        print(f"FINAL APPLICATION: {len(applied_accessors)} accessors")
        print(f"{'='*70}")
        print(f"  Accessors: {', '.join(sorted(applied_accessors))}")

        # Restore to baseline first
        build()

        final_exact = apply_and_keep(tu_name, src_file, header_file, applied_accessors, cache)
        if final_exact is None:
            print("  FAILED to apply! Files may need manual restore.")
            return

        final_count = sum(1 for v in final_exact.values() if v)

        # Final regression check
        regressions = []
        improvements = []
        for f in baseline_exact:
            if f in final_exact:
                if baseline_exact[f] and not final_exact[f]:
                    regressions.append(f)
                elif not baseline_exact[f] and final_exact[f]:
                    improvements.append(f)

        if regressions:
            print(f"  FINAL REGRESSION DETECTED! Reverting...")
            reg_short = [r.split('__')[0] for r in regressions]
            print(f"  Broken: {reg_short}")
            # Revert via git
            subprocess.run(['git', 'checkout', '--', src_file], capture_output=True, text=True)
            subprocess.run(['git', 'checkout', '--', header_file], capture_output=True, text=True)
            build()
            return

        print(f"\n  Exact matches: {baseline_count} -> {final_count} ({final_count - baseline_count:+d})")
        if improvements:
            for f in improvements:
                print(f"    NEW MATCH: {f.split('__')[0]}")
        print(f"\n  Changes kept in:")
        print(f"    {header_file}")
        print(f"    {src_file}")
    else:
        print("\nNo combinations applied.")


def run(tu_name, apply_mode=False, target_func=None):
    header_file = detect_header(tu_name)

    print(f"{'='*70}")
    print(f"INLINE COMBINATOR: {tu_name}")
    print(f"{'='*70}")
    print(f"Header: {header_file}")

    cache = load_or_scan(tu_name, header_file)

    gaps = cache.get('gaps', {})
    print(f"Functions with stack gaps: {len(gaps)}")
    print(f"Total gap: {sum(abs(g) for g in gaps.values())} bytes")

    solutions = print_analysis(cache, target_func)

    if apply_mode:
        run_apply(tu_name, cache, target_func)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python tools/inline_combinator.py Player/MarioMove              # analyze gaps")
        print("  python tools/inline_combinator.py Player/MarioMove --apply      # find and apply solutions")
        print("  python tools/inline_combinator.py Player/MarioMove funcName     # single function")
        sys.exit(1)

    tu_name = sys.argv[1]
    apply_mode = '--apply' in sys.argv

    target_func = None
    for arg in sys.argv[2:]:
        if not arg.startswith('--'):
            target_func = arg
            break

    run(tu_name, apply_mode, target_func)
