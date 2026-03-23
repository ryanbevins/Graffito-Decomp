#!/usr/bin/env python3
"""
Code Differ - Instruction-level diff analyzer for MWCC decompilation

Compares original vs compiled assembly instruction-by-instruction,
identifies the TYPE of each difference, and suggests source fixes.

Patterns detected:
1. TYPE_MISMATCH: extra extsh/clrlwi from wrong variable type (s16 vs s32, u8 vs u32)
2. REG_SWAP: same instruction but different registers (declaration order issue)
3. BRANCH_POLARITY: beq vs bne (inverted if condition)
4. LOAD_ORDER: same loads in different order (expression/declaration order)
5. MISSING_INSN: instruction in original not in compiled (missing inline or logic)
6. EXTRA_INSN: instruction in compiled not in original (extra cast or unnecessary code)
7. STACK_OFFSET: same instruction with different stack-relative offset

Usage:
  python tools/code_differ.py Player/MarioMove funcName
  python tools/code_differ.py Player/MarioMove --all
"""

import subprocess
import re
import sys
import os
import difflib

OBJDUMP = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        'build', 'binutils', 'powerpc-eabi-objdump.exe')


# ============================================================
# INSTRUCTION PARSING
# ============================================================

class Insn:
    """Parsed PowerPC instruction."""
    def __init__(self, offset, raw, opcode, operands, full_asm):
        self.offset = offset
        self.raw = raw  # hex bytes
        self.opcode = opcode
        self.operands = operands
        self.full_asm = full_asm

    def __repr__(self):
        return f'{self.offset}: {self.full_asm}'


def parse_function(objdump_output, func_name):
    """Extract instructions for a function from objdump output."""
    insns = []
    in_func = False
    for line in objdump_output.split('\n'):
        m = re.match(r'^([0-9a-f]+) <(.+)>:', line)
        if m:
            if in_func:
                break  # hit next function
            if m.group(2) == func_name:
                in_func = True
            continue
        if in_func:
            if line.strip() == '':
                break
            m2 = re.match(r'\s+([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s+(\S+)\s*(.*)', line)
            if m2:
                offset = m2.group(1)
                raw = m2.group(2).strip()
                opcode = m2.group(3)
                operands = m2.group(4).strip()
                full = f'{opcode}\t{operands}' if operands else opcode
                insns.append(Insn(offset, raw, opcode, operands, full))
    return insns


def get_all_functions(objdump_output):
    """Get list of all function names."""
    funcs = []
    for line in objdump_output.split('\n'):
        m = re.match(r'^[0-9a-f]+ <(.+)>:', line)
        if m:
            funcs.append(m.group(1))
    return funcs


# ============================================================
# DIFF CLASSIFICATION
# ============================================================

# PPC register categories
GPR = set(f'r{i}' for i in range(32))
FPR = set(f'f{i}' for i in range(32))
CR = set(f'cr{i}' for i in range(8))

# Type-narrowing instructions
TYPE_NARROW = {'extsh', 'extsb', 'clrlwi', 'clrlwi.', 'rlwinm'}

# Branch instructions
BRANCHES = {'beq', 'bne', 'blt', 'bgt', 'ble', 'bge', 'beq+', 'bne+',
            'blt+', 'bgt+', 'ble+', 'bge+', 'beqlr', 'bnelr', 'blelr', 'bgelr',
            'beq-', 'bne-', 'blt-', 'bgt-', 'ble-', 'bge-',
            'b', 'bl', 'blr', 'bctr', 'bctrl'}

# Load/store instructions
LOADS = {'lwz', 'lhz', 'lha', 'lbz', 'lfs', 'lfd', 'lfsx', 'lwzx'}
STORES = {'stw', 'sth', 'stb', 'stfs', 'stfd', 'stwu', 'stfsu'}

# Branch polarity pairs
BRANCH_PAIRS = {
    'beq': 'bne', 'bne': 'beq',
    'blt': 'bge', 'bge': 'blt',
    'bgt': 'ble', 'ble': 'bgt',
    'beq+': 'bne+', 'bne+': 'beq+',
    'blt+': 'bge+', 'bge+': 'blt+',
    'bgt+': 'ble+', 'ble+': 'bgt+',
}


def classify_insn_diff(orig, comp):
    """Classify the difference between two aligned instructions."""
    if orig.raw == comp.raw:
        return None  # identical

    # Same opcode?
    if orig.opcode == comp.opcode:
        # Parse operands
        o_ops = [x.strip() for x in orig.operands.split(',')]
        c_ops = [x.strip() for x in comp.operands.split(',')]

        # Stack offset difference (same registers, different immediate with r1)
        if orig.opcode in LOADS | STORES:
            # Check if only the offset differs and base is r1
            if any('r1' in op for op in o_ops) and any('r1' in op for op in c_ops):
                # Extract just the register parts
                o_regs = [op for op in o_ops if op.startswith('r') or op.startswith('f')]
                c_regs = [op for op in c_ops if op.startswith('r') or op.startswith('f')]
                if o_regs == c_regs:
                    return ('STACK_OFFSET', f'{orig.opcode} offset differs')

        # Register swap (same opcode, different register numbers)
        o_regs = set(re.findall(r'\b([rf]\d+)\b', orig.operands))
        c_regs = set(re.findall(r'\b([rf]\d+)\b', comp.operands))
        if o_regs != c_regs:
            o_gpr = o_regs & GPR
            c_gpr = c_regs & GPR
            o_fpr = o_regs & FPR
            c_fpr = c_regs & FPR

            if o_gpr != c_gpr and o_fpr == c_fpr:
                return ('GPR_SWAP', f'{orig.opcode}: {o_gpr ^ c_gpr}')
            elif o_fpr != c_fpr and o_gpr == c_gpr:
                return ('FPR_SWAP', f'{orig.opcode}: {o_fpr ^ c_fpr}')
            elif o_gpr != c_gpr and o_fpr != c_fpr:
                return ('REG_SWAP', f'{orig.opcode}: GPR={o_gpr ^ c_gpr} FPR={o_fpr ^ c_fpr}')

        # Same registers but different immediate/offset
        return ('OPERAND_DIFF', f'{orig.opcode}: {orig.operands} vs {comp.operands}')

    # Different opcode
    # Branch polarity swap
    if orig.opcode in BRANCH_PAIRS and comp.opcode == BRANCH_PAIRS.get(orig.opcode):
        return ('BRANCH_FLIP', f'{orig.opcode} vs {comp.opcode}')

    # Load type mismatch (lha vs lhz = signed vs unsigned)
    if {orig.opcode, comp.opcode} == {'lha', 'lhz'}:
        return ('SIGN_MISMATCH', f'{orig.opcode} vs {comp.opcode} (s16 vs u16 load)')

    # Conditional return vs conditional branch
    if orig.opcode in ('beqlr', 'bnelr', 'blelr', 'bgelr') or \
       comp.opcode in ('beqlr', 'bnelr', 'blelr', 'bgelr'):
        return ('COND_RETURN', f'{orig.opcode} vs {comp.opcode}')

    # Type narrowing instruction present in one but not other
    if orig.opcode in TYPE_NARROW and comp.opcode not in TYPE_NARROW:
        return ('TYPE_NARROW_ORIG', f'original has {orig.opcode}, compiled has {comp.opcode}')
    if comp.opcode in TYPE_NARROW and orig.opcode not in TYPE_NARROW:
        return ('TYPE_NARROW_COMP', f'compiled has extra {comp.opcode}')

    # stwu vs stw (stack update difference)
    if {orig.opcode, comp.opcode} == {'stw', 'stwu'}:
        return ('STWU_DIFF', f'{orig.opcode} vs {comp.opcode}')

    # fabs vs manual abs pattern
    if orig.opcode == 'fabs' and comp.opcode != 'fabs':
        return ('FABS_MISSING', f'original uses fabs, compiled uses {comp.opcode}')

    # General opcode mismatch
    return ('OPCODE_DIFF', f'{orig.opcode} vs {comp.opcode}')


def analyze_unaligned(orig_insns, comp_insns):
    """Analyze functions with different instruction counts using sequence alignment."""
    # Use difflib to align instruction sequences by opcode
    orig_ops = [i.opcode + ' ' + i.operands for i in orig_insns]
    comp_ops = [i.opcode + ' ' + i.operands for i in comp_insns]

    matcher = difflib.SequenceMatcher(None, orig_ops, comp_ops)

    diffs = []
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == 'equal':
            continue
        elif tag == 'replace':
            for k in range(min(i2 - i1, j2 - j1)):
                oi = orig_insns[i1 + k]
                ci = comp_insns[j1 + k]
                cls = classify_insn_diff(oi, ci)
                if cls:
                    diffs.append((*cls, oi, ci))
        elif tag == 'delete':
            # Instructions in original but not compiled
            for k in range(i1, i2):
                oi = orig_insns[k]
                # Check if it's a type narrowing instruction
                if oi.opcode in TYPE_NARROW:
                    diffs.append(('EXTRA_NARROW_ORIG',
                                  f'original has extra {oi.opcode}', oi, None))
                else:
                    diffs.append(('MISSING_IN_COMP',
                                  f'compiled missing: {oi.full_asm}', oi, None))
        elif tag == 'insert':
            # Instructions in compiled but not original
            for k in range(j1, j2):
                ci = comp_insns[k]
                if ci.opcode in TYPE_NARROW:
                    diffs.append(('EXTRA_NARROW_COMP',
                                  f'compiled has extra {ci.opcode}', None, ci))
                else:
                    diffs.append(('EXTRA_IN_COMP',
                                  f'compiled extra: {ci.full_asm}', None, ci))

    return diffs


# ============================================================
# SUGGESTION ENGINE
# ============================================================

def suggest_fix(diff_type, detail, orig_insn, comp_insn):
    """Suggest a source code fix for a classified difference."""
    suggestions = {
        'STACK_OFFSET': 'Stack frame size mismatch. Try adding/removing inline accessor calls.',
        'GPR_SWAP': 'GPR register allocation differs. Try reordering local variable declarations.',
        'FPR_SWAP': 'FPR register allocation differs. Try reordering float variable declarations or expressions.',
        'BRANCH_FLIP': 'Branch condition is inverted. Your if/else blocks may be swapped. MWCC never swaps true/false blocks.',
        'SIGN_MISMATCH': 'Signed vs unsigned load. Change variable type: lha=s16 field, lhz=u16 field.',
        'TYPE_NARROW_COMP': 'Compiled has extra type narrowing. Variable type is too narrow (u8/s16). Try s32 or u32.',
        'EXTRA_NARROW_COMP': 'Compiled has extra extsh/clrlwi. Variable declared as s16/u8 should be s32/int.',
        'EXTRA_NARROW_ORIG': 'Original has extra extsh/clrlwi that compiled optimized away. May need explicit cast.',
        'COND_RETURN': 'Conditional return (bXXlr) vs conditional branch. Block ordering issue.',
        'FABS_MISSING': 'Original uses fabs instruction. Use fabsf() or fabs() instead of manual if/negate pattern.',
        'STWU_DIFF': 'Store-with-update vs regular store. Compiler optimization for sequential struct writes.',
        'MISSING_IN_COMP': 'Instruction in original missing from compiled. May need additional logic or a different expression.',
        'EXTRA_IN_COMP': 'Extra instruction in compiled. May have unnecessary cast, variable, or expression.',
        'OPERAND_DIFF': 'Same instruction, different operands. Check constant values and field offsets.',
        'OPCODE_DIFF': 'Different instruction entirely. Structural code difference.',
        'REG_SWAP': 'Both GPR and FPR registers differ. Major register allocation mismatch.',
    }
    return suggestions.get(diff_type, 'Unknown difference type.')


# ============================================================
# MAIN ANALYSIS
# ============================================================

def analyze_function(tu_name, func_name, verbose=True):
    """Full analysis of a single function."""
    orig_out = subprocess.run(
        [OBJDUMP, '-d', f'build/GMSJ01/obj/{tu_name}.o'],
        capture_output=True, text=True, timeout=60
    ).stdout
    comp_out = subprocess.run(
        [OBJDUMP, '-d', f'build/GMSJ01/src/{tu_name}.o'],
        capture_output=True, text=True, timeout=60
    ).stdout

    orig_insns = parse_function(orig_out, func_name)
    comp_insns = parse_function(comp_out, func_name)

    if not orig_insns:
        print(f"  Function '{func_name}' not found in original")
        return None
    if not comp_insns:
        print(f"  Function '{func_name}' not found in compiled")
        return None

    if verbose:
        print(f"\n{'='*70}")
        print(f"FUNCTION: {func_name}")
        print(f"Original: {len(orig_insns)} insns, Compiled: {len(comp_insns)} insns")
        print(f"{'='*70}")

    if orig_insns == comp_insns:
        if verbose:
            print("  EXACT MATCH!")
        return {'status': 'MATCH', 'diffs': []}

    # Classify diffs
    if len(orig_insns) == len(comp_insns):
        # Same length — align 1:1
        diffs = []
        for oi, ci in zip(orig_insns, comp_insns):
            cls = classify_insn_diff(oi, ci)
            if cls:
                diffs.append((*cls, oi, ci))
    else:
        # Different length — use sequence alignment
        diffs = analyze_unaligned(orig_insns, comp_insns)

    if not diffs:
        if verbose:
            print("  No classifiable differences found")
        return {'status': 'UNKNOWN', 'diffs': []}

    # Aggregate by type
    type_counts = {}
    for d in diffs:
        t = d[0]
        type_counts[t] = type_counts.get(t, 0) + 1

    if verbose:
        print(f"\nDiff summary ({len(diffs)} differences):")
        for t, c in sorted(type_counts.items(), key=lambda x: -x[1]):
            print(f"  {t:<25} {c:>3}x  | {suggest_fix(t, '', None, None)}")

        # Show specific diffs
        print(f"\nDetailed diffs:")
        shown = set()
        for dtype, detail, oi, ci in diffs:
            if dtype in shown and type_counts[dtype] > 5:
                continue  # don't spam repeated types
            shown.add(dtype)

            orig_str = f'{oi.offset}: {oi.full_asm}' if oi else '(missing)'
            comp_str = f'{ci.offset}: {ci.full_asm}' if ci else '(missing)'

            print(f"\n  [{dtype}] {detail}")
            print(f"    ORIG: {orig_str}")
            print(f"    COMP: {comp_str}")
            print(f"    FIX:  {suggest_fix(dtype, detail, oi, ci)}")

    return {
        'status': 'DIFFERS',
        'orig_count': len(orig_insns),
        'comp_count': len(comp_insns),
        'diffs': diffs,
        'type_counts': type_counts,
    }


def analyze_all(tu_name):
    """Analyze all non-matching functions in a TU."""
    orig_out = subprocess.run(
        [OBJDUMP, '-d', f'build/GMSJ01/obj/{tu_name}.o'],
        capture_output=True, text=True, timeout=60
    ).stdout
    comp_out = subprocess.run(
        [OBJDUMP, '-d', f'build/GMSJ01/src/{tu_name}.o'],
        capture_output=True, text=True, timeout=60
    ).stdout

    all_funcs = get_all_functions(orig_out)

    print(f"{'='*70}")
    print(f"CODE DIFFER: {tu_name} ({len(all_funcs)} functions)")
    print(f"{'='*70}")

    # Quick pass: find non-matching functions
    non_matching = []
    for func in all_funcs:
        orig = parse_function(orig_out, func)
        comp = parse_function(comp_out, func)
        if orig and comp:
            orig_bytes = [i.raw for i in orig]
            comp_bytes = [i.raw for i in comp]
            if orig_bytes != comp_bytes:
                non_matching.append(func)

    print(f"\nNon-matching: {len(non_matching)}/{len(all_funcs)}")

    # Aggregate all diff types across all functions
    global_types = {}
    actionable = []

    for func in non_matching:
        orig = parse_function(orig_out, func)
        comp = parse_function(comp_out, func)

        if len(orig) == len(comp):
            diffs = []
            for oi, ci in zip(orig, comp):
                cls = classify_insn_diff(oi, ci)
                if cls:
                    diffs.append(cls[0])
        else:
            udiffs = analyze_unaligned(orig, comp)
            diffs = [d[0] for d in udiffs]

        type_counts = {}
        for d in diffs:
            type_counts[d] = type_counts.get(d, 0) + 1
            global_types[d] = global_types.get(d, 0) + 1

        # Determine if actionable
        fixable_types = {'SIGN_MISMATCH', 'TYPE_NARROW_COMP', 'EXTRA_NARROW_COMP',
                         'BRANCH_FLIP', 'FABS_MISSING'}
        fixable = set(type_counts.keys()) & fixable_types
        if fixable:
            actionable.append((func, type_counts, fixable))

    # Summary
    print(f"\n{'='*70}")
    print(f"GLOBAL DIFF TYPE DISTRIBUTION")
    print(f"{'='*70}")
    for t, c in sorted(global_types.items(), key=lambda x: -x[1]):
        fix = suggest_fix(t, '', None, None)
        print(f"  {t:<25} {c:>4}x  {fix[:60]}")

    if actionable:
        print(f"\n{'='*70}")
        print(f"ACTIONABLE FUNCTIONS ({len(actionable)})")
        print(f"{'='*70}")
        for func, counts, fixable in actionable:
            print(f"\n  {func}:")
            for t in fixable:
                print(f"    {t}: {counts[t]}x -> {suggest_fix(t, '', None, None)}")

    return non_matching, global_types, actionable


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python tools/code_differ.py Player/MarioMove --all")
        print("  python tools/code_differ.py Player/MarioMove functionName")
        sys.exit(1)

    tu = sys.argv[1]

    if '--all' in sys.argv:
        analyze_all(tu)
    elif len(sys.argv) >= 3 and not sys.argv[2].startswith('--'):
        analyze_function(tu, sys.argv[2])
    else:
        analyze_all(tu)
