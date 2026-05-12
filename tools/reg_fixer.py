#!/usr/bin/env python3
"""
Register Fixer v2 - Automatically fixes GPR/FPR register allocation mismatches
by reordering variable declarations, expressions, and code structure.

Default strategies (fast):
1. All-pairs declaration swaps (not just adjacent)
2. Expression operand reordering (swap a+b to b+a)
3. Statement reordering (swap independent assignments)
4. Comparison operand swaps (a <= b -> b >= a)
5. Subtraction rewrites (a - b -> -(b - a) or a + (-b))

Deep strategies (--deep flag, slower):
6. Temporary variable introduction/removal
7. Ternary <-> if/else conversion
8. Expression splitting/merging (a = x*y+z -> tmp = x*y; a = tmp+z)
9. Function argument pre-evaluation (extract arg to temp)

For each function with register swaps:
1. Identifies which registers are swapped
2. Finds variable declarations and expressions
3. Tries each strategy, measures diff improvement
4. Keeps improvements, reverts failures

Usage:
  python tools/reg_fixer.py Player/MarioMove                    # analyze all
  python tools/reg_fixer.py Player/MarioMove --fix               # analyze + auto-fix
  python tools/reg_fixer.py Player/MarioMove functionName        # single function
  python tools/reg_fixer.py Player/MarioMove functionName --fix  # single + fix
  python tools/reg_fixer.py Player/MarioMove --deep              # try all strategies (slower)
"""

import subprocess
import re
import sys
import os
import time
import copy
import itertools

OBJDUMP = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        'build', 'binutils', 'powerpc-eabi-objdump.exe')

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__))))
from code_differ import parse_function, classify_insn_diff, get_all_functions


# ============================================================
# REGISTER ANALYSIS
# ============================================================

def find_reg_swaps(orig_insns, comp_insns):
    """Find register swaps between original and compiled.
    Returns list of (orig_reg, comp_reg)."""
    if len(orig_insns) != len(comp_insns):
        return []

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
            if creg in reg_map and oreg in reg_map[creg]:
                pair = tuple(sorted([oreg, creg]))
                if pair not in seen:
                    seen.add(pair)
                    swaps.append((oreg, creg))

    return swaps


# ============================================================
# SOURCE ANALYSIS
# ============================================================

def find_function_in_source(src_lines, func_mangled):
    """Find the source line range for a function."""
    base = func_mangled.split('__')[0]
    start = None
    brace_depth = 0

    for i, line in enumerate(src_lines):
        if start is None:
            if re.search(r'\b' + re.escape(base) + r'\s*\(', line):
                if '{' in line or (i + 1 < len(src_lines) and '{' in src_lines[i + 1]):
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
    type_pattern = (
        r'^((?:const\s+)?'
        r'(?:u8|s8|u16|s16|u32|s32|int|f32|f64|BOOL|bool|'
        r'void\s*\*|TMario\s*\*|TLiveActor\s*\*|THitActor\s*\*|'
        r'TWaterGun\s*\*|TYoshi\s*\*|TBGCheckData\s*\*|'
        r'MtxPtr|Mtx|Vec|'
        r'JGeometry::TVec3<f32>|TVec3<f32>|'
        r'(?:[A-Z]\w+\s*\*?))'
        r'\s*\*?)\s+(\w+)\s*[;=\(]'
    )

    for i in range(start, min(end + 1, len(src_lines))):
        line = src_lines[i].strip()
        if not line or line.startswith('//') or line.startswith('/*') or line in ('{', '}'):
            continue

        m = re.match(type_pattern, line)
        if m:
            decls.append((i, m.group(1).strip(), m.group(2), src_lines[i]))

    return decls


def find_expressions(src_lines, start, end):
    """Find binary expressions that could have operands swapped.
    Returns list of (line_index, left, op, right, full_match)."""
    exprs = []
    # Match: a OP b patterns (arithmetic, comparison)
    expr_pattern = r'(\b\w+(?:\.\w+)*(?:\(\))?)\s*([+\-*])\s*(\b\w+(?:\.\w+)*(?:\(\))?)'

    for i in range(start, min(end + 1, len(src_lines))):
        line = src_lines[i].strip()
        if not line or line.startswith('//'):
            continue

        for m in re.finditer(expr_pattern, line):
            left, op, right = m.group(1), m.group(2), m.group(3)
            # Only commutative ops
            if op in ('+', '*'):
                exprs.append((i, left, op, right, m.group(0)))

    return exprs


def find_assignments(src_lines, start, end):
    """Find assignments where we could introduce/remove temp variables.
    Returns list of (line_index, var, expression)."""
    assigns = []

    for i in range(start, min(end + 1, len(src_lines))):
        line = src_lines[i].strip()
        if not line or line.startswith('//'):
            continue

        # Match: var = expr;
        m = re.match(r'^(\w+(?:\.\w+)*)\s*=\s*(.+);$', line)
        if m:
            assigns.append((i, m.group(1), m.group(2)))

    return assigns


def find_function_calls(src_lines, start, end):
    """Find function calls with multiple arguments that could be reordered.
    Returns list of (line_index, func_name, args_str)."""
    calls = []

    for i in range(start, min(end + 1, len(src_lines))):
        line = src_lines[i].strip()
        if not line or line.startswith('//'):
            continue

        # Match: funcName(arg1, arg2, ...)
        m = re.search(r'(\b\w+)\s*\(([^)]+)\)', line)
        if m and ',' in m.group(2):
            calls.append((i, m.group(1), m.group(2)))

    return calls


# ============================================================
# TRANSFORMATION STRATEGIES
# ============================================================

class Transform:
    """A source transformation to try."""
    def __init__(self, name, apply_fn, revert_fn):
        self.name = name
        self.apply_fn = apply_fn
        self.revert_fn = revert_fn


def make_line_swap(lines, line_a, line_b):
    """Create a transform that swaps two lines."""
    def apply(ls):
        ls[line_a], ls[line_b] = ls[line_b], ls[line_a]
    def revert(ls):
        ls[line_a], ls[line_b] = ls[line_b], ls[line_a]
    return apply, revert


def make_expr_swap(lines, line_idx, old_expr, new_expr):
    """Create a transform that swaps expression operands."""
    orig = lines[line_idx]
    def apply(ls):
        ls[line_idx] = ls[line_idx].replace(old_expr, new_expr, 1)
    def revert(ls):
        ls[line_idx] = orig
    return apply, revert


def make_temp_intro(lines, assign_line, var, expr, temp_name='_tmp'):
    """Insert a temp variable before an assignment: T _tmp = expr; var = _tmp;"""
    orig_line = lines[assign_line]
    indent = len(orig_line) - len(orig_line.lstrip())
    ind = orig_line[:indent]

    # Try to infer type from the expression/context
    # Default to f32 for float-looking exprs, else auto
    if any(c in expr for c in ('.0f', 'sin', 'cos', 'sqrt', 'fabs')):
        vtype = 'f32'
    else:
        vtype = 'auto'

    def apply(ls):
        new_lines = f'{ind}{vtype} {temp_name} = {expr};\n{ind}{var} = {temp_name};'
        ls[assign_line] = new_lines
    def revert(ls):
        ls[assign_line] = orig_line
    return apply, revert


def make_temp_remove(lines, temp_line, use_line, temp_name, expr):
    """Remove a temp variable by inlining: replace temp with its expression."""
    orig_temp = lines[temp_line]
    orig_use = lines[use_line]

    def apply(ls):
        ls[temp_line] = ''  # remove temp decl
        ls[use_line] = ls[use_line].replace(temp_name, f'({expr})')
    def revert(ls):
        ls[temp_line] = orig_temp
        ls[use_line] = orig_use
    return apply, revert


def make_decl_move(lines, from_line, to_line):
    """Move a declaration from one position to another."""
    content = lines[from_line]
    def apply(ls):
        line = ls[from_line]
        ls[from_line] = ''  # blank the original
        if to_line > from_line:
            ls.insert(to_line, line)
        else:
            ls.insert(to_line, line)
    def revert(ls):
        # Reconstruct from scratch - simpler than tracking shifts
        if to_line > from_line:
            ls.pop(to_line)
        else:
            ls.pop(to_line)
        ls[from_line] = content
    return apply, revert


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
    """Count differing instructions for a specific function."""
    orig_out = subprocess.run([OBJDUMP, '-d', f'build/GMSJ01/obj/{tu_name}.o'],
                              capture_output=True, text=True, timeout=60).stdout
    comp_out = subprocess.run([OBJDUMP, '-d', f'build/GMSJ01/src/{tu_name}.o'],
                              capture_output=True, text=True, timeout=60).stdout

    orig = parse_function(orig_out, func_name)
    comp = parse_function(comp_out, func_name)

    if not orig or not comp:
        return 999

    if len(orig) != len(comp):
        return abs(len(orig) - len(comp)) + \
               sum(1 for a, b in zip(orig[:min(len(orig), len(comp))],
                                      comp[:min(len(orig), len(comp))]) if a.raw != b.raw)

    return sum(1 for a, b in zip(orig, comp) if a.raw != b.raw)


def build():
    subprocess.run(['python', 'configure.py'], capture_output=True, text=True, timeout=30)
    r = subprocess.run(['python', '-m', 'ninja'], capture_output=True, text=True, timeout=60)
    return r.returncode == 0


def try_transform(tu_name, src_file, func_name, transform_name, apply_fn, revert_fn):
    """Try a transform and measure diff change. Returns (new_diffs, baseline_diffs)."""
    with open(src_file) as f:
        lines = f.readlines()

    baseline_diffs = get_func_diff_count(tu_name, func_name)

    # Apply transform
    line_list = [l.rstrip('\n') for l in lines]
    apply_fn(line_list)

    with open(src_file, 'w') as f:
        f.write('\n'.join(line_list) + '\n')

    if not build():
        # Revert on build failure
        with open(src_file, 'w') as f:
            f.writelines(lines)
        build()
        return None, baseline_diffs

    new_diffs = get_func_diff_count(tu_name, func_name)

    # Revert
    with open(src_file, 'w') as f:
        f.writelines(lines)

    return new_diffs, baseline_diffs


# ============================================================
# STRATEGY GENERATORS
# ============================================================

def gen_all_pair_swaps(src_lines, func_start, func_end):
    """Generate all pairs of declaration swaps (not just adjacent)."""
    decls = find_local_declarations(src_lines, func_start, func_end)
    transforms = []

    for i in range(len(decls)):
        for j in range(i + 1, len(decls)):
            line_a = decls[i][0]
            line_b = decls[j][0]
            name_a = decls[i][2]
            name_b = decls[j][2]

            apply_fn, revert_fn = make_line_swap(src_lines, line_a, line_b)
            transforms.append(Transform(
                f'swap_decl:{name_a}<->{name_b}',
                apply_fn, revert_fn
            ))

    return transforms


def gen_expr_swaps(src_lines, func_start, func_end):
    """Generate expression operand swaps (a+b -> b+a, a*b -> b*a)."""
    exprs = find_expressions(src_lines, func_start, func_end)
    transforms = []

    for line_idx, left, op, right, full in exprs:
        if left == right:
            continue
        new_expr = f'{right} {op} {left}'
        apply_fn, revert_fn = make_expr_swap(src_lines, line_idx, full, new_expr)
        transforms.append(Transform(
            f'expr_swap:{left}{op}{right}->{right}{op}{left}',
            apply_fn, revert_fn
        ))

    return transforms


def gen_temp_intros(src_lines, func_start, func_end):
    """Generate temporary variable introductions for complex RHS expressions."""
    assigns = find_assignments(src_lines, func_start, func_end)
    transforms = []

    for line_idx, var, expr in assigns:
        # Only for complex expressions (has operators)
        if not any(op in expr for op in ['+', '-', '*', '/', '(', '->']):
            continue
        # Skip if already a simple variable
        if re.match(r'^\w+$', expr.strip()):
            continue
        # Skip if already has a temp pattern
        if '_tmp' in expr:
            continue

        temp_name = f'_rf_{var.replace(".", "_")}'
        apply_fn, revert_fn = make_temp_intro(src_lines, line_idx, var, expr, temp_name)
        transforms.append(Transform(
            f'temp_intro:{var}={temp_name}',
            apply_fn, revert_fn
        ))

    return transforms


def gen_stmt_reorders(src_lines, func_start, func_end):
    """Generate statement reorderings for consecutive assignments to different vars."""
    assigns = find_assignments(src_lines, func_start, func_end)
    transforms = []

    for i in range(len(assigns)):
        for j in range(i + 1, min(i + 4, len(assigns))):
            line_a = assigns[i][0]
            line_b = assigns[j][0]
            var_a = assigns[i][1]
            var_b = assigns[j][1]

            # Only swap if they don't depend on each other
            if var_a in assigns[j][2] or var_b in assigns[i][2]:
                continue

            # Only if close together (within 5 lines)
            if abs(line_b - line_a) > 5:
                continue

            apply_fn, revert_fn = make_line_swap(src_lines, line_a, line_b)
            transforms.append(Transform(
                f'stmt_reorder:{var_a}<->{var_b}',
                apply_fn, revert_fn
            ))

    return transforms


def gen_comparison_swaps(src_lines, func_start, func_end):
    """Generate comparison operand swaps: (a <= b) -> (b >= a), etc.
    MWCC evaluates the left operand first, so swapping changes register allocation."""
    transforms = []
    # Map: swap operands AND flip the operator
    op_flip = {'<=': '>=', '>=': '<=', '<': '>', '>': '<', '==': '==', '!=': '!='}
    cmp_pattern = re.compile(
        r'if\s*\(\s*'
        r'([\w][\w.()>\-]*)'    # left operand (identifiers, member access, calls, ->)
        r'\s*([<>=!]=?)\s*'     # comparison operator
        r'([\w][\w.()>\-]*)'    # right operand
        r'\s*\)'
    )

    for i in range(func_start, min(func_end + 1, len(src_lines))):
        line = src_lines[i]
        stripped = line.strip()
        if not stripped or stripped.startswith('//'):
            continue

        for m in cmp_pattern.finditer(line):
            left, op, right = m.group(1), m.group(2), m.group(3)
            if left == right:
                continue
            if op not in op_flip:
                continue
            flipped_op = op_flip[op]
            old_expr = m.group(0)
            # Rebuild the if(...) with swapped operands
            new_expr = old_expr[:old_expr.index(left)] + right + ' ' + flipped_op + ' ' + left + ')'

            # Use a simpler replacement: just swap within the matched region
            orig_cmp = f'{left} {op} {right}'
            new_cmp = f'{right} {flipped_op} {left}'

            orig_line = src_lines[i]
            line_idx = i

            def _make_apply(idx, old_c, new_c, orig_l):
                def apply(ls):
                    ls[idx] = ls[idx].replace(old_c, new_c, 1)
                return apply
            def _make_revert(idx, orig_l):
                def revert(ls):
                    ls[idx] = orig_l
                return revert

            transforms.append(Transform(
                f'cmp_swap:{left}{op}{right}->{right}{flipped_op}{left}',
                _make_apply(line_idx, orig_cmp, new_cmp, orig_line),
                _make_revert(line_idx, orig_line)
            ))

    return transforms


def gen_subtraction_rewrites(src_lines, func_start, func_end):
    """Generate subtraction rewrites: a - b -> -(b - a) or a + (-b).
    Flips which operand goes to f1 vs f2 in MWCC."""
    transforms = []
    # Match: expr = X - Y; or similar with subtraction
    sub_pattern = re.compile(
        r'([\w][\w.()>\-]*)\s*-\s*([\w][\w.()>\-]*)'
    )

    for i in range(func_start, min(func_end + 1, len(src_lines))):
        line = src_lines[i]
        stripped = line.strip()
        if not stripped or stripped.startswith('//'):
            continue
        # Skip lines with string literals
        if '"' in stripped:
            continue

        for m in sub_pattern.finditer(stripped):
            left, right = m.group(1), m.group(2)
            if left == right:
                continue
            # Skip pointer arithmetic / -> patterns
            if '->' in m.group(0):
                continue
            # Skip if left is a keyword
            if left in ('return', 'if', 'while', 'for', 'case', 'sizeof'):
                continue

            old_sub = f'{left} - {right}'
            orig_line = src_lines[i]
            line_idx = i

            # Strategy A: a - b -> -(b - a)
            new_sub_neg = f'-({right} - {left})'
            def _make_apply_a(idx, old_s, new_s, orig_l):
                def apply(ls):
                    ls[idx] = ls[idx].replace(old_s, new_s, 1)
                return apply
            def _make_revert(idx, orig_l):
                def revert(ls):
                    ls[idx] = orig_l
                return revert

            transforms.append(Transform(
                f'sub_negate:{left}-{right}->-({right}-{left})',
                _make_apply_a(line_idx, old_sub, new_sub_neg, orig_line),
                _make_revert(line_idx, orig_line)
            ))

            # Strategy B: a - b -> a + (-b)
            new_sub_add = f'{left} + (-{right})'
            transforms.append(Transform(
                f'sub_addneg:{left}-{right}->{left}+(-{right})',
                _make_apply_a(line_idx, old_sub, new_sub_add, orig_line),
                _make_revert(line_idx, orig_line)
            ))

    return transforms


def gen_ternary_transforms(src_lines, func_start, func_end):
    """Generate ternary <-> if/else conversions.
    x = cond ? a : b; <-> if(cond) x = a; else x = b;
    These produce different register allocation in MWCC."""
    transforms = []

    # Pattern 1: ternary -> if/else
    ternary_pattern = re.compile(
        r'^(\s*)'                            # indent
        r'(\w[\w.>\-]*)\s*=\s*'              # lhs =
        r'(.+?)\s*\?\s*'                     # condition ?
        r'(.+?)\s*:\s*'                      # true_val :
        r'(.+?)\s*;'                         # false_val ;
    )

    for i in range(func_start, min(func_end + 1, len(src_lines))):
        line = src_lines[i]
        m = ternary_pattern.match(line)
        if m:
            indent, lhs, cond, true_val, false_val = (
                m.group(1), m.group(2), m.group(3), m.group(4), m.group(5)
            )
            orig_line = src_lines[i]
            line_idx = i

            new_code = (
                f'{indent}if ({cond}) {lhs} = {true_val}; '
                f'else {lhs} = {false_val};'
            )

            def _make_apply(idx, new_c, orig_l):
                def apply(ls):
                    ls[idx] = new_c
                return apply
            def _make_revert(idx, orig_l):
                def revert(ls):
                    ls[idx] = orig_l
                return revert

            transforms.append(Transform(
                f'ternary_to_ifelse:{lhs}=cond?...',
                _make_apply(line_idx, new_code, orig_line),
                _make_revert(line_idx, orig_line)
            ))

    # Pattern 2: single-line if/else assignment -> ternary
    ifelse_pattern = re.compile(
        r'^(\s*)'                                    # indent
        r'if\s*\((.+?)\)\s*'                        # if (cond)
        r'(\w[\w.>\-]*)\s*=\s*(.+?);\s*'            # lhs = true_val;
        r'else\s+\3\s*=\s*(.+?);'                   # else lhs = false_val;
    )

    for i in range(func_start, min(func_end + 1, len(src_lines))):
        line = src_lines[i]
        m = ifelse_pattern.match(line)
        if m:
            indent, cond, lhs, true_val, false_val = (
                m.group(1), m.group(2), m.group(3), m.group(4), m.group(5)
            )
            orig_line = src_lines[i]
            line_idx = i

            new_code = f'{indent}{lhs} = ({cond}) ? {true_val} : {false_val};'

            def _make_apply_t(idx, new_c, orig_l):
                def apply(ls):
                    ls[idx] = new_c
                return apply
            def _make_revert_t(idx, orig_l):
                def revert(ls):
                    ls[idx] = orig_l
                return revert

            transforms.append(Transform(
                f'ifelse_to_ternary:{lhs}=...',
                _make_apply_t(line_idx, new_code, orig_line),
                _make_revert_t(line_idx, orig_line)
            ))

    # Pattern 3: multi-line if/else assignment -> ternary
    for i in range(func_start, min(func_end - 2, len(src_lines))):
        line = src_lines[i].strip()
        # Match: if (cond)
        m_if = re.match(r'if\s*\((.+)\)\s*$', line)
        if not m_if:
            continue
        cond = m_if.group(1)

        # Next lines: assignment in if-block, else, assignment in else-block
        # Handle both braced and unbraced forms
        lines_ahead = []
        for k in range(i + 1, min(i + 8, func_end + 1)):
            lines_ahead.append((k, src_lines[k].strip()))

        # Try to find: lhs = val1; ... else ... lhs = val2;
        lhs = None
        true_val = None
        false_val = None
        else_seen = False
        true_line = None
        false_line = None
        last_line = None

        for k, la in lines_ahead:
            if la in ('{', '}'):
                continue
            if la == 'else' or la == 'else {':
                else_seen = True
                continue
            if not else_seen:
                ma = re.match(r'(\w[\w.>\-]*)\s*=\s*(.+?);$', la)
                if ma:
                    lhs = ma.group(1)
                    true_val = ma.group(2)
                    true_line = k
            else:
                ma = re.match(re.escape(lhs or '') + r'\s*=\s*(.+?);$', la)
                if ma and lhs:
                    false_val = ma.group(1)
                    false_line = k
                    last_line = k
                    break

        if lhs and true_val is not None and false_val is not None and last_line:
            indent = src_lines[i][:len(src_lines[i]) - len(src_lines[i].lstrip())]
            new_code = f'{indent}{lhs} = ({cond}) ? {true_val} : {false_val};'

            orig_lines_saved = {k: src_lines[k] for k in range(i, last_line + 1)}
            start_idx = i
            end_idx = last_line

            def _make_apply_ml(start, end, new_c, saved):
                def apply(ls):
                    ls[start] = new_c
                    for k in range(start + 1, end + 1):
                        ls[k] = ''
                return apply
            def _make_revert_ml(saved):
                def revert(ls):
                    for k, v in saved.items():
                        ls[k] = v
                return revert

            transforms.append(Transform(
                f'multiline_ifelse_to_ternary:{lhs}=...',
                _make_apply_ml(start_idx, end_idx, new_code, orig_lines_saved),
                _make_revert_ml(orig_lines_saved)
            ))

    return transforms


def gen_expression_splits(src_lines, func_start, func_end):
    """Generate expression splitting/merging transforms.
    Split: a = x * y + z; -> f32 _t = x * y; a = _t + z;
    Merge: f32 _t = x * y; a = _t + z; -> a = x * y + z;
    Changes which FPR gets the intermediate result."""
    transforms = []

    # --- SPLIT: find compound expressions and split at each operator ---
    # Match: var = exprA OP exprB;  where exprA or exprB is itself compound
    compound_pattern = re.compile(
        r'^(\s*)'                           # indent
        r'(\w[\w.>\-]*)\s*=\s*'             # lhs =
        r'(.+?)\s*([+\-*/])\s*'            # left_expr OP
        r'(\w[\w.()>\-]*)\s*;'              # right_expr ;
    )
    # Also try splitting the other way: simple OP compound
    compound_pattern2 = re.compile(
        r'^(\s*)'
        r'(\w[\w.>\-]*)\s*=\s*'
        r'(\w[\w.()>\-]*)\s*([+\-*/])\s*'  # simple OP
        r'(.+?)\s*;'                        # compound ;
    )

    split_count = 0
    for i in range(func_start, min(func_end + 1, len(src_lines))):
        line = src_lines[i]
        stripped = line.strip()
        if not stripped or stripped.startswith('//'):
            continue
        # Need at least 2 operators to be worth splitting
        ops_count = sum(1 for c in stripped if c in '+-*/')
        if ops_count < 2:
            continue
        # Skip lines with function calls (complex to split safely)
        if '(' in stripped and ')' in stripped and not stripped.startswith('('):
            # Allow if it's just parenthesized subexpr, not a func call
            if re.search(r'\w\s*\(', stripped.split('=', 1)[-1] if '=' in stripped else ''):
                continue

        m = re.match(r'^(\s*)(\w[\w.>\-]*)\s*=\s*(.+);$', stripped)
        if not m:
            continue

        lhs = m.group(2)
        expr = m.group(3).strip()
        indent = line[:len(line) - len(line.lstrip())]
        orig_line = src_lines[i]
        line_idx = i

        # Try splitting at each top-level binary operator
        # Simple approach: find +/- at the top level (not inside parens)
        depth = 0
        split_points = []
        for ci, ch in enumerate(expr):
            if ch == '(':
                depth += 1
            elif ch == ')':
                depth -= 1
            elif depth == 0 and ch in '+-*/' and ci > 0 and ci < len(expr) - 1:
                split_points.append((ci, ch))

        for sp_idx, (sp_pos, sp_op) in enumerate(split_points):
            left_part = expr[:sp_pos].strip()
            right_part = expr[sp_pos + 1:].strip()

            if not left_part or not right_part:
                continue
            # Skip if either side is just a simple identifier
            if re.match(r'^\w+$', left_part) and re.match(r'^\w+$', right_part):
                continue

            # Determine type heuristic
            if any(hint in orig_line for hint in ('f32', 'f64', '.0f', 'sin', 'cos', 'sqrt')):
                tmp_type = 'f32'
            elif any(hint in orig_line for hint in ('s32', 'int', 'u32')):
                tmp_type = 's32'
            else:
                tmp_type = 'f32'

            tmp_name = f'_sp{split_count}'
            split_count += 1

            # Split left part into temp
            new_code = (
                f'{indent}{tmp_type} {tmp_name} = {left_part};\n'
                f'{indent}{lhs} = {tmp_name} {sp_op} {right_part};'
            )

            def _make_apply_sp(idx, new_c, orig_l):
                def apply(ls):
                    ls[idx] = new_c
                return apply
            def _make_revert_sp(idx, orig_l):
                def revert(ls):
                    ls[idx] = orig_l
                return revert

            transforms.append(Transform(
                f'expr_split:{lhs}=({left_part}){sp_op}{right_part}',
                _make_apply_sp(line_idx, new_code, orig_line),
                _make_revert_sp(line_idx, orig_line)
            ))

    # --- MERGE: find temp = expr; var = temp OP ...; and inline ---
    assigns = find_assignments(src_lines, func_start, func_end)
    for i in range(len(assigns)):
        temp_line, temp_var, temp_expr = assigns[i]
        # Check if this looks like a temp (used exactly once in the next few lines)
        for j in range(i + 1, min(i + 4, len(assigns))):
            use_line, use_var, use_expr = assigns[j]
            if temp_var in use_expr:
                # Check it's used as a simple token, not part of another name
                if re.search(r'\b' + re.escape(temp_var) + r'\b', use_expr):
                    # Merge: inline temp_expr into use_expr
                    merged_expr = re.sub(
                        r'\b' + re.escape(temp_var) + r'\b',
                        f'({temp_expr})',
                        use_expr
                    )
                    orig_temp_line = src_lines[temp_line]
                    orig_use_line = src_lines[use_line]

                    def _make_apply_mg(t_idx, u_idx, new_expr, orig_t, orig_u, u_var):
                        indent = orig_u[:len(orig_u) - len(orig_u.lstrip())]
                        def apply(ls):
                            ls[t_idx] = ''
                            ls[u_idx] = f'{indent}{u_var} = {new_expr};'
                        return apply
                    def _make_revert_mg(t_idx, u_idx, orig_t, orig_u):
                        def revert(ls):
                            ls[t_idx] = orig_t
                            ls[u_idx] = orig_u
                        return revert

                    transforms.append(Transform(
                        f'expr_merge:{temp_var}->inline_in_{use_var}',
                        _make_apply_mg(temp_line, use_line, merged_expr,
                                      orig_temp_line, orig_use_line, use_var),
                        _make_revert_mg(temp_line, use_line,
                                       orig_temp_line, orig_use_line)
                    ))

    return transforms


def gen_func_arg_reorders(src_lines, func_start, func_end):
    """Generate function argument pre-evaluation transforms.
    MWCC evaluates arguments right-to-left. Wrapping an argument in a temp
    forces its evaluation before the call, changing register allocation.

    func(exprA, exprB) -> { auto _arg = exprA; func(_arg, exprB); }
    """
    transforms = []
    calls = find_function_calls(src_lines, func_start, func_end)
    arg_count = 0

    for line_idx, func_name, args_str in calls:
        # Skip control flow keywords
        if func_name in ('if', 'while', 'for', 'switch', 'return'):
            continue

        # Split args (simple comma split - won't handle nested parens perfectly)
        args = []
        depth = 0
        current = ''
        for ch in args_str:
            if ch == '(':
                depth += 1
                current += ch
            elif ch == ')':
                depth -= 1
                current += ch
            elif ch == ',' and depth == 0:
                args.append(current.strip())
                current = ''
            else:
                current += ch
        args.append(current.strip())

        if len(args) < 2:
            continue

        orig_line = src_lines[line_idx]
        indent = orig_line[:len(orig_line) - len(orig_line.lstrip())]

        # For each argument that's an expression (not simple identifier/literal),
        # try extracting it to a temp
        for arg_idx, arg in enumerate(args):
            # Skip simple identifiers and literals
            if re.match(r'^[\w.>\-]+$', arg):
                continue
            if re.match(r'^[\d.]+f?$', arg):
                continue
            # Must have some operator or function call
            if not any(c in arg for c in '+-*/()'):
                continue

            # Determine type heuristic
            if any(hint in arg for hint in ('.0f', 'sin', 'cos', 'sqrt')):
                tmp_type = 'f32'
            elif any(hint in arg for hint in ('0x', '<<', '>>')):
                tmp_type = 'u32'
            else:
                tmp_type = 'auto'

            tmp_name = f'_arg{arg_count}'
            arg_count += 1

            new_args = list(args)
            new_args[arg_idx] = tmp_name
            new_args_str = ', '.join(new_args)

            # Rebuild the line with the temp extraction
            new_call = orig_line.replace(args_str, new_args_str, 1)
            new_code = f'{indent}{tmp_type} {tmp_name} = {arg};\n{new_call}'

            def _make_apply_fa(idx, new_c, orig_l):
                def apply(ls):
                    ls[idx] = new_c
                return apply
            def _make_revert_fa(idx, orig_l):
                def revert(ls):
                    ls[idx] = orig_l
                return revert

            transforms.append(Transform(
                f'func_arg_extract:{func_name}:arg{arg_idx}',
                _make_apply_fa(line_idx, new_code, orig_line),
                _make_revert_fa(line_idx, orig_line)
            ))

    return transforms


# ============================================================
# MAIN
# ============================================================

def analyze_function(tu_name, func_name, src_file, fix_mode=False, deep_mode=False):
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

    swaps = find_reg_swaps(orig_insns, comp_insns)
    if not swaps:
        return

    gpr_swaps = [(a, b) for a, b in swaps if a.startswith('r')]
    fpr_swaps = [(a, b) for a, b in swaps if a.startswith('f')]

    short_name = func_name.split('__')[0]
    total_diffs = sum(1 for a, b in zip(orig_insns, comp_insns) if a.raw != b.raw) \
                  if len(orig_insns) == len(comp_insns) else -1

    print(f"\n  {short_name}: {total_diffs} diffs")
    print(f"    GPR swaps: {gpr_swaps}")
    print(f"    FPR swaps: {fpr_swaps}")

    if not fix_mode:
        return

    # Load source
    with open(src_file) as f:
        src_content = f.read()
    src_lines = src_content.split('\n')

    func_start, func_end = find_function_in_source(src_lines, func_name)
    if func_start is None:
        print(f"    Could not find function in source")
        return

    print(f"    Function at lines {func_start+1}-{func_end+1}")

    # Generate transforms
    transforms = []

    # Strategy 1: All-pairs declaration swaps
    transforms.extend(gen_all_pair_swaps(src_lines, func_start, func_end))

    # Strategy 2: Expression operand swaps
    transforms.extend(gen_expr_swaps(src_lines, func_start, func_end))

    # Strategy 3: Statement reordering
    transforms.extend(gen_stmt_reorders(src_lines, func_start, func_end))

    # Strategy 4: Comparison operand swaps (fast)
    transforms.extend(gen_comparison_swaps(src_lines, func_start, func_end))

    # Strategy 5: Subtraction rewrites (fast)
    transforms.extend(gen_subtraction_rewrites(src_lines, func_start, func_end))

    if deep_mode:
        # Strategy 6: Temp variable introduction (slower)
        transforms.extend(gen_temp_intros(src_lines, func_start, func_end))

        # Strategy 7: Ternary <-> if/else conversion (slower)
        transforms.extend(gen_ternary_transforms(src_lines, func_start, func_end))

        # Strategy 8: Expression splitting/merging (slower)
        transforms.extend(gen_expression_splits(src_lines, func_start, func_end))

        # Strategy 9: Function argument pre-evaluation (slower)
        transforms.extend(gen_func_arg_reorders(src_lines, func_start, func_end))

    if not transforms:
        print(f"    No transforms to try")
        return

    print(f"    Generated {len(transforms)} transforms to try")

    baseline_diffs = total_diffs
    best_diffs = baseline_diffs
    best_transform = None

    for t in transforms:
        # Re-read source each time (in case of line number issues)
        with open(src_file) as f:
            current = f.read()
        current_lines = current.split('\n')

        try:
            t.apply_fn(current_lines)
        except (IndexError, ValueError) as e:
            continue

        with open(src_file, 'w') as f:
            f.write('\n'.join(current_lines))

        if build():
            new_diffs = get_func_diff_count(tu_name, func_name)

            # Check for regressions in other functions
            new_total = get_exact_match_count(tu_name)

            if new_diffs < best_diffs:
                print(f"    IMPROVED: {t.name}: {baseline_diffs} -> {new_diffs} diffs")
                best_diffs = new_diffs
                best_transform = t.name

                # Save this improved version
                with open(src_file) as f:
                    improved_content = f.read()
            elif new_diffs == best_diffs:
                pass  # no change
            else:
                pass  # worse, will revert
        else:
            pass  # build failed

        # Revert to original
        with open(src_file, 'w') as f:
            f.write(src_content)

    # Apply best if found
    if best_transform and best_diffs < baseline_diffs:
        with open(src_file, 'w') as f:
            f.write(improved_content)
        build()

        # Verify no regressions
        new_total = get_exact_match_count(tu_name)
        baseline_total = get_exact_match_count(tu_name)

        print(f"    APPLIED: {best_transform} ({baseline_diffs} -> {best_diffs} diffs)")

        if best_diffs == 0:
            print(f"    *** PERFECT MATCH! ***")

        return True
    else:
        # Make sure we're back to original
        with open(src_file, 'w') as f:
            f.write(src_content)
        build()
        print(f"    No improvements found ({len(transforms)} transforms tried)")
        return False


def run(tu_name, target_func=None, fix_mode=False, deep_mode=False):
    src_file = f'src/{tu_name}.cpp'

    print(f"{'='*70}")
    print(f"REGISTER FIXER v2: {tu_name}")
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
        result = analyze_function(tu_name, func, src_file, fix_mode, deep_mode)
        if result:
            improved += 1

    elapsed = time.time() - start

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
        print("  python tools/reg_fixer.py Player/MarioMove --fix --deep")
        print("  python tools/reg_fixer.py Player/MarioMove funcName --fix")
        sys.exit(1)

    tu = sys.argv[1]
    fix_mode = '--fix' in sys.argv
    deep_mode = '--deep' in sys.argv

    target = None
    for arg in sys.argv[2:]:
        if not arg.startswith('--'):
            target = arg
            break

    run(tu, target, fix_mode, deep_mode)
