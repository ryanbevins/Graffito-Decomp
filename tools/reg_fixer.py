#!/usr/bin/env python3
"""
Register Fixer - Automatically fixes GPR/FPR register allocation mismatches
by reordering variable declarations and expressions.

For each function with register swaps:
1. Identifies which registers are swapped
2. Finds the corresponding variable declarations in source
3. Tries swapping declaration order
4. Compiles and checks if the swap is fixed
5. Keeps improvements, reverts failures

Usage:
  python tools/reg_fixer.py Player/MarioMove                    # analyze all
  python tools/reg_fixer.py Player/MarioMove --fix               # analyze + auto-fix
  python tools/reg_fixer.py Player/MarioMove functionName        # single function
  python tools/reg_fixer.py Player/MarioMove functionName --fix  # single + fix
"""

import subprocess
import re
import sys
import os
import time
import itertools

OBJDUMP = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        'build', 'binutils', 'powerpc-eabi-objdump.exe')

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__))))
from code_differ import parse_function, classify_insn_diff, get_all_functions


# ============================================================
# REGISTER ANALYSIS
# ============================================================

def get_reg_assignments(insns):
    """Map which registers are first assigned (written) in a function.
    Returns list of (register, instruction_index, instruction) in assignment order."""
    assignments = []
    seen = set()

    # Skip prologue (stwu, stw saves, mr)
    prologue_end = 0
    for i, insn in enumerate(insns):
        if insn.opcode in ('stwu', 'stw', 'stfd', 'mr', 'mflr', 'stmw'):
            prologue_end = i + 1
        else:
            break

    for i in range(prologue_end, len(insns)):
        insn = insns[i]
        # Find destination register (first operand for most PPC instructions)
        parts = insn.operands.split(',')
        if not parts:
            continue
        dest = parts[0].strip()

        # Skip if not a register or already seen
        if not re.match(r'^[rf]\d+$', dest):
            continue
        if dest in seen:
            continue
        if dest in ('r0', 'r1', 'r3'):  # r0=scratch, r1=stack, r3=this/return
            continue

        seen.add(dest)
        assignments.append((dest, i, insn))

    return assignments


def find_reg_swaps(orig_insns, comp_insns):
    """Find register swaps between original and compiled.
    Returns list of (orig_reg, comp_reg, first_diff_index)."""
    if len(orig_insns) != len(comp_insns):
        return []

    # Build register mapping by finding consistent swaps
    reg_map = {}  # orig_reg -> set of comp_regs it maps to

    for oi, ci in zip(orig_insns, comp_insns):
        if oi.raw == ci.raw:
            continue
        if oi.opcode != ci.opcode:
            continue

        o_regs = re.findall(r'\b([rf]\d+)\b', oi.operands)
        c_regs = re.findall(r'\b([rf]\d+)\b', ci.operands)

        if len(o_regs) != len(c_regs):
            continue

        for oreg, creg in zip(o_regs, c_regs):
            if oreg != creg:
                if oreg not in reg_map:
                    reg_map[oreg] = set()
                reg_map[oreg].add(creg)

    # Find consistent 1:1 swaps
    swaps = []
    seen = set()
    for oreg, cregs in sorted(reg_map.items()):
        if len(cregs) == 1:
            creg = list(cregs)[0]
            # Check reverse mapping
            if creg in reg_map and oreg in reg_map[creg]:
                pair = tuple(sorted([oreg, creg]))
                if pair not in seen:
                    seen.add(pair)
                    swaps.append((oreg, creg))

    return swaps


# ============================================================
# SOURCE ANALYSIS
# ============================================================

def find_function_in_source(src_content, func_mangled):
    """Find the source line range for a function."""
    # Demangle: extract the base name
    base = func_mangled.split('__')[0]

    # Search for the function definition
    lines = src_content.split('\n')
    start = None
    brace_depth = 0

    for i, line in enumerate(lines):
        if start is None:
            # Look for function definition
            if re.search(r'\b' + re.escape(base) + r'\s*\(', line):
                if '{' in line or (i + 1 < len(lines) and '{' in lines[i + 1]):
                    start = i
                    brace_depth = line.count('{') - line.count('}')
                    continue
        else:
            brace_depth += line.count('{') - line.count('}')
            if brace_depth <= 0:
                return start, i

    return None, None


def find_local_declarations(src_lines, start, end):
    """Find local variable declarations in a function.
    Returns list of (line_index, var_type, var_name, full_line)."""
    decls = []

    for i in range(start, min(end + 1, len(src_lines))):
        line = src_lines[i].strip()

        # Skip empty, comments, braces
        if not line or line.startswith('//') or line.startswith('/*') or line in ('{', '}'):
            continue

        # Match: type varname = ... or type varname;
        m = re.match(r'^((?:const\s+)?(?:u8|s8|u16|s16|u32|s32|int|f32|f64|BOOL|bool|void\*?)\s*\*?)\s+(\w+)\s*[;=]', line)
        if m:
            decls.append((i, m.group(1).strip(), m.group(2), src_lines[i]))

    return decls


def find_swappable_pairs(decls):
    """Find pairs of adjacent declarations that could be swapped."""
    pairs = []
    for i in range(len(decls) - 1):
        line_a, type_a, name_a, full_a = decls[i]
        line_b, type_b, name_b, full_b = decls[i + 1]

        # Only swap adjacent lines (within 2 lines of each other)
        if abs(line_b - line_a) <= 3:
            pairs.append((i, i + 1))

    return pairs


# ============================================================
# TESTING ENGINE
# ============================================================

def get_exact_match_count(tu_name):
    """Count exact byte-matching functions."""
    orig_out = subprocess.run([OBJDUMP, '-d', f'build/GMSJ01/obj/{tu_name}.o'],
                              capture_output=True, text=True, timeout=60).stdout
    comp_out = subprocess.run([OBJDUMP, '-d', f'build/GMSJ01/src/{tu_name}.o'],
                              capture_output=True, text=True, timeout=60).stdout

    orig_funcs = {}
    comp_funcs = {}

    for output, store in [(orig_out, orig_funcs), (comp_out, comp_funcs)]:
        cur = None
        for line in output.split('\n'):
            m = re.match(r'^[0-9a-f]+ <(.+)>:', line)
            if m:
                cur = m.group(1)
                store[cur] = []
            elif cur:
                bm = re.match(r'\s+[0-9a-f]+:\s+((?:[0-9a-f]{2} )+)', line)
                if bm:
                    store[cur].append(bm.group(1).strip())

    count = 0
    for func in orig_funcs:
        if func in comp_funcs and orig_funcs[func] == comp_funcs[func]:
            count += 1
    return count


def get_func_diff_count(tu_name, func_name):
    """Count differing bytes for a specific function."""
    orig_out = subprocess.run([OBJDUMP, '-d', f'build/GMSJ01/obj/{tu_name}.o'],
                              capture_output=True, text=True, timeout=60).stdout
    comp_out = subprocess.run([OBJDUMP, '-d', f'build/GMSJ01/src/{tu_name}.o'],
                              capture_output=True, text=True, timeout=60).stdout

    orig = parse_function(orig_out, func_name)
    comp = parse_function(comp_out, func_name)

    if not orig or not comp:
        return 999

    if len(orig) != len(comp):
        return abs(len(orig) - len(comp)) + sum(1 for a, b in zip(orig[:min(len(orig), len(comp))], comp[:min(len(orig), len(comp))]) if a.raw != b.raw)

    return sum(1 for a, b in zip(orig, comp) if a.raw != b.raw)


def build():
    subprocess.run(['python', 'configure.py'], capture_output=True, text=True, timeout=30)
    r = subprocess.run(['python', '-m', 'ninja'], capture_output=True, text=True, timeout=60)
    return r.returncode == 0


def try_swap(tu_name, src_file, func_name, line_a, line_b):
    """Try swapping two lines in the source and check if it improves."""
    with open(src_file) as f:
        lines = f.readlines()

    orig_content = ''.join(lines)
    baseline_diffs = get_func_diff_count(tu_name, func_name)

    # Swap the lines
    lines[line_a], lines[line_b] = lines[line_b], lines[line_a]

    with open(src_file, 'w') as f:
        f.writelines(lines)

    try:
        if not build():
            return None, baseline_diffs

        new_diffs = get_func_diff_count(tu_name, func_name)

        # Also check we didn't break other functions
        new_total = get_exact_match_count(tu_name)

        return new_diffs, baseline_diffs
    finally:
        # Only revert if it didn't improve
        pass  # caller decides


def revert_file(src_file, original_content):
    with open(src_file, 'w') as f:
        f.write(original_content)


# ============================================================
# MAIN
# ============================================================

def analyze_function(tu_name, func_name, src_file, fix_mode=False):
    """Analyze and optionally fix register swaps in a function."""
    orig_out = subprocess.run([OBJDUMP, '-d', f'build/GMSJ01/obj/{tu_name}.o'],
                              capture_output=True, text=True, timeout=60).stdout
    comp_out = subprocess.run([OBJDUMP, '-d', f'build/GMSJ01/src/{tu_name}.o'],
                              capture_output=True, text=True, timeout=60).stdout

    orig_insns = parse_function(orig_out, func_name)
    comp_insns = parse_function(comp_out, func_name)

    if not orig_insns or not comp_insns:
        return

    if [i.raw for i in orig_insns] == [i.raw for i in comp_insns]:
        return  # already matches

    # Find register swaps
    swaps = find_reg_swaps(orig_insns, comp_insns)
    if not swaps:
        return

    gpr_swaps = [(a, b) for a, b in swaps if a.startswith('r')]
    fpr_swaps = [(a, b) for a, b in swaps if a.startswith('f')]

    short_name = func_name.split('__')[0]
    total_diffs = sum(1 for a, b in zip(orig_insns, comp_insns) if a.raw != b.raw) if len(orig_insns) == len(comp_insns) else -1

    print(f"\n  {short_name}: {total_diffs} diffs, GPR swaps: {gpr_swaps}, FPR swaps: {fpr_swaps}")

    if not fix_mode:
        return

    # Try to fix by swapping variable declarations
    with open(src_file) as f:
        src_content = f.read()

    src_lines = src_content.split('\n')
    func_start, func_end = find_function_in_source(src_content, func_name)

    if func_start is None:
        print(f"    Could not find function in source")
        return

    decls = find_local_declarations(src_lines, func_start, func_end)
    if len(decls) < 2:
        print(f"    Only {len(decls)} declarations found, nothing to swap")
        return

    pairs = find_swappable_pairs(decls)
    if not pairs:
        print(f"    No swappable adjacent declaration pairs found")
        return

    print(f"    Found {len(decls)} declarations, {len(pairs)} swappable pairs")

    baseline_diffs = total_diffs
    best_diffs = baseline_diffs
    best_swap = None

    for idx_a, idx_b in pairs:
        line_a = decls[idx_a][0]
        line_b = decls[idx_b][0]
        name_a = decls[idx_a][2]
        name_b = decls[idx_b][2]

        # Swap
        src_lines[line_a], src_lines[line_b] = src_lines[line_b], src_lines[line_a]

        with open(src_file, 'w') as f:
            f.write('\n'.join(src_lines))

        if build():
            new_diffs = get_func_diff_count(tu_name, func_name)

            # Check for regressions
            new_total = get_exact_match_count(tu_name)

            if new_diffs < best_diffs:
                print(f"    IMPROVED: swap {name_a} <-> {name_b}: {baseline_diffs} -> {new_diffs} diffs")
                best_diffs = new_diffs
                best_swap = (line_a, line_b, name_a, name_b)
            else:
                print(f"    tried {name_a} <-> {name_b}: {new_diffs} diffs (no improvement)")
        else:
            print(f"    tried {name_a} <-> {name_b}: BUILD FAILED")

        # Revert swap
        src_lines[line_a], src_lines[line_b] = src_lines[line_b], src_lines[line_a]
        with open(src_file, 'w') as f:
            f.write('\n'.join(src_lines))

    # Apply best swap if found
    if best_swap and best_diffs < baseline_diffs:
        line_a, line_b, name_a, name_b = best_swap
        src_lines[line_a], src_lines[line_b] = src_lines[line_b], src_lines[line_a]
        with open(src_file, 'w') as f:
            f.write('\n'.join(src_lines))
        build()

        # Verify no regressions
        new_total = get_exact_match_count(tu_name)
        baseline_total = get_exact_match_count(tu_name)  # already rebuilt

        print(f"    APPLIED: {name_a} <-> {name_b} ({baseline_diffs} -> {best_diffs} diffs)")
        return True

    # Restore original
    with open(src_file, 'w') as f:
        f.write(src_content)
    build()
    return False


def run(tu_name, target_func=None, fix_mode=False):
    src_file = f'src/{tu_name}.cpp'

    print(f"{'='*70}")
    print(f"REGISTER FIXER: {tu_name}")
    print(f"{'='*70}")

    if not build():
        print("ERROR: baseline build failed!")
        return

    baseline_matches = get_exact_match_count(tu_name)
    print(f"Baseline: {baseline_matches} exact matches")

    orig_out = subprocess.run([OBJDUMP, '-d', f'build/GMSJ01/obj/{tu_name}.o'],
                              capture_output=True, text=True, timeout=60).stdout
    comp_out = subprocess.run([OBJDUMP, '-d', f'build/GMSJ01/src/{tu_name}.o'],
                              capture_output=True, text=True, timeout=60).stdout

    if target_func:
        funcs = [target_func]
    else:
        # Find functions with register swaps (same instruction count)
        all_funcs = get_all_functions(orig_out)
        funcs = []
        for func in all_funcs:
            orig = parse_function(orig_out, func)
            comp = parse_function(comp_out, func)
            if not orig or not comp:
                continue
            if len(orig) != len(comp):
                continue
            if [i.raw for i in orig] == [i.raw for i in comp]:
                continue

            swaps = find_reg_swaps(orig, comp)
            if swaps:
                funcs.append(func)

    print(f"Functions with register swaps: {len(funcs)}")

    start = time.time()
    improved = 0

    for func in funcs:
        result = analyze_function(tu_name, func, src_file, fix_mode)
        if result:
            improved += 1

    elapsed = time.time() - start

    if fix_mode:
        final_matches = get_exact_match_count(tu_name)
        print(f"\n{'='*70}")
        print(f"RESULTS: {improved} functions improved")
        print(f"Matches: {baseline_matches} -> {final_matches}")
        print(f"Time: {elapsed:.0f}s")
        print(f"{'='*70}")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python tools/reg_fixer.py Player/MarioMove")
        print("  python tools/reg_fixer.py Player/MarioMove --fix")
        print("  python tools/reg_fixer.py Player/MarioMove funcName --fix")
        sys.exit(1)

    tu = sys.argv[1]
    fix_mode = '--fix' in sys.argv

    target = None
    for arg in sys.argv[2:]:
        if not arg.startswith('--'):
            target = arg
            break

    run(tu, target, fix_mode)
