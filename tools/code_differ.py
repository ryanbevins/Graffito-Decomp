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

Patterns detected:
1-7. (original patterns)
8. INLINED_CALL: Original calls X out-of-line but compiled inlines it
9. MISSING_CALL: Compiled calls X but original doesn't (should be inlined)
10. BLOCK_REORDER: Code blocks are reordered vs structurally different
"""

import subprocess
import re
import sys
import os
import difflib
import hashlib
from collections import Counter

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
        'INLINED_CALL': 'Original calls this function out-of-line but compiled inlines it. Use #pragma dont_inline on/off around the called function, or ensure it is not defined in a header visible to this TU.',
        'MISSING_CALL': 'Compiled calls this function but original inlines it. Move the function definition into a header visible to this TU, or define it in the same TU so MWCC can inline it.',
    }
    return suggestions.get(diff_type, 'Unknown difference type.')


# ============================================================
# RELOCATION-AWARE DIFFING
# ============================================================

def parse_relocations(objdump_dr_output, func_name):
    """Extract relocation entries for a function from `objdump -dr` output.

    Returns a dict with:
      'calls': list of (offset, target_symbol) for R_PPC_REL24 (bl calls)
      'data_refs': list of (offset, reloc_type, target_symbol) for addr refs
    """
    calls = []
    data_refs = []
    in_func = False

    lines = objdump_dr_output.split('\n')
    last_insn_offset = None

    for line in lines:
        # Function header: "00000abc <funcName>:"
        m = re.match(r'^([0-9a-f]+) <(.+)>:', line)
        if m:
            if in_func:
                break  # hit next function
            if m.group(2) == func_name:
                in_func = True
            continue

        if not in_func:
            continue

        # Empty line ends function
        if line.strip() == '':
            break

        # Instruction line
        m_insn = re.match(r'\s+([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s+(\S+)\s*(.*)', line)
        if m_insn:
            last_insn_offset = m_insn.group(1)
            continue

        # Relocation line: "    offset: R_PPC_REL24  symbol_name"
        # or: "                         offset: R_PPC_ADDR16_HA  symbol_name"
        m_reloc = re.match(r'\s+([0-9a-f]+):\s+(R_PPC_\S+)\s+(\S+)', line)
        if m_reloc:
            rel_offset = m_reloc.group(1)
            rel_type = m_reloc.group(2)
            rel_target = m_reloc.group(3)

            if rel_type == 'R_PPC_REL24':
                calls.append((rel_offset, rel_target))
            elif rel_type in ('R_PPC_ADDR16_HA', 'R_PPC_ADDR16_LO',
                              'R_PPC_ADDR16_HI', 'R_PPC_ADDR32'):
                data_refs.append((rel_offset, rel_type, rel_target))

    return {'calls': calls, 'data_refs': data_refs}


def analyze_relocations(tu_name, func_name, verbose=True):
    """Compare relocations between original and compiled object files.

    Detects:
      INLINED_CALL: original calls X out-of-line but compiled inlines it
      MISSING_CALL: compiled calls X but original doesn't (should be inlined)

    Returns list of (diff_type, detail_string) tuples.
    """
    orig_o = f'build/GMSJ01/obj/{tu_name}.o'
    comp_o = f'build/GMSJ01/src/{tu_name}.o'

    try:
        orig_dr = subprocess.run(
            [OBJDUMP, '-dr', orig_o],
            capture_output=True, text=True, timeout=60
        ).stdout
        comp_dr = subprocess.run(
            [OBJDUMP, '-dr', comp_o],
            capture_output=True, text=True, timeout=60
        ).stdout
    except (subprocess.TimeoutExpired, FileNotFoundError):
        return []

    orig_relocs = parse_relocations(orig_dr, func_name)
    comp_relocs = parse_relocations(comp_dr, func_name)

    # Compare call targets (bl instructions via R_PPC_REL24)
    orig_call_targets = [target for _, target in orig_relocs['calls']]
    comp_call_targets = [target for _, target in comp_relocs['calls']]

    # Build multisets (a symbol can be called multiple times)
    orig_counts = Counter(orig_call_targets)
    comp_counts = Counter(comp_call_targets)

    diffs = []

    # Calls in original but not compiled = we're inlining something that should be out-of-line
    for sym in sorted(set(orig_counts.keys()) | set(comp_counts.keys())):
        o_count = orig_counts.get(sym, 0)
        c_count = comp_counts.get(sym, 0)

        if o_count > c_count:
            diff = o_count - c_count
            detail = f'Original calls {sym} {o_count}x but compiled only {c_count}x (inlining {diff} call(s) that should be out-of-line)'
            diffs.append(('INLINED_CALL', detail))
        elif c_count > o_count:
            diff = c_count - o_count
            detail = f'Compiled calls {sym} {c_count}x but original only {o_count}x ({diff} call(s) should be inlined)'
            diffs.append(('MISSING_CALL', detail))

    if verbose and diffs:
        print(f"\n  Relocation diffs:")
        for dtype, detail in diffs:
            suggestion = suggest_fix(dtype, detail, None, None)
            print(f"    [{dtype}] {detail}")
            print(f"      FIX: {suggestion}")

    return diffs


# ============================================================
# BLOCK-REORDERING DETECTION
# ============================================================

def split_basic_blocks(insns):
    """Split an instruction list into basic blocks.

    A new block starts after a branch instruction, or at a branch target.
    Returns list of lists of Insn.
    """
    if not insns:
        return []

    # Collect branch target offsets
    branch_targets = set()
    for insn in insns:
        if insn.opcode in BRANCHES and insn.operands:
            # Extract hex target from operands like "0x1234" or "0xabc <label>"
            m = re.search(r'(?:^|,\s*)([0-9a-f]+)\b', insn.operands)
            if m:
                branch_targets.add(m.group(1))

    blocks = []
    current = []

    for insn in insns:
        # Start new block if this offset is a branch target (and current block non-empty)
        if insn.offset in branch_targets and current:
            blocks.append(current)
            current = []

        current.append(insn)

        # End block after a branch instruction
        if insn.opcode in BRANCHES:
            blocks.append(current)
            current = []

    if current:
        blocks.append(current)

    return blocks


def hash_block(block):
    """Hash a basic block by its opcodes only (ignoring registers/operands).

    This allows detecting reordered blocks that have the same structure.
    """
    opcodes = ' '.join(insn.opcode for insn in block)
    return hashlib.md5(opcodes.encode()).hexdigest()


def detect_block_reorder(orig_insns, comp_insns):
    """Detect if compiled code is a permutation of original basic blocks.

    Returns (is_reordered, summary_string) tuple.
    """
    if not orig_insns or not comp_insns:
        return False, ''

    orig_blocks = split_basic_blocks(orig_insns)
    comp_blocks = split_basic_blocks(comp_insns)

    # If block counts differ significantly, not a simple reorder
    if len(orig_blocks) != len(comp_blocks):
        return False, ''

    # Need at least 2 blocks to have a reordering
    if len(orig_blocks) < 2:
        return False, ''

    orig_hashes = [hash_block(b) for b in orig_blocks]
    comp_hashes = [hash_block(b) for b in comp_blocks]

    # Check if it's a permutation (same multiset of hashes)
    if sorted(orig_hashes) == sorted(comp_hashes) and orig_hashes != comp_hashes:
        # Find which blocks moved
        moved = []
        for i, (oh, ch) in enumerate(zip(orig_hashes, comp_hashes)):
            if oh != ch:
                moved.append(i)

        summary = (f'Code blocks appear reordered — likely if/else or branch ordering difference. '
                   f'{len(moved)} of {len(orig_blocks)} blocks in different positions.')
        return True, summary

    return False, ''


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

    # Block-reorder detection
    is_reordered, reorder_summary = detect_block_reorder(orig_insns, comp_insns)
    if verbose and is_reordered:
        print(f"\n  [BLOCK_REORDER] {reorder_summary}")

    # Relocation-aware diffing
    reloc_diffs = analyze_relocations(tu_name, func_name, verbose=verbose)
    for dtype, detail in reloc_diffs:
        type_counts[dtype] = type_counts.get(dtype, 0) + 1

    return {
        'status': 'DIFFERS',
        'orig_count': len(orig_insns),
        'comp_count': len(comp_insns),
        'diffs': diffs,
        'type_counts': type_counts,
        'reloc_diffs': reloc_diffs,
        'block_reorder': is_reordered,
        'block_reorder_summary': reorder_summary if is_reordered else '',
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

    # Get objdump -dr output for relocation analysis
    try:
        orig_dr = subprocess.run(
            [OBJDUMP, '-dr', f'build/GMSJ01/obj/{tu_name}.o'],
            capture_output=True, text=True, timeout=60
        ).stdout
        comp_dr = subprocess.run(
            [OBJDUMP, '-dr', f'build/GMSJ01/src/{tu_name}.o'],
            capture_output=True, text=True, timeout=60
        ).stdout
    except (subprocess.TimeoutExpired, FileNotFoundError):
        orig_dr = ''
        comp_dr = ''

    # Aggregate all diff types across all functions
    global_types = {}
    actionable = []
    block_reordered = []

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

        # Relocation analysis (inline vs out-of-line call mismatches)
        if orig_dr and comp_dr:
            orig_relocs = parse_relocations(orig_dr, func)
            comp_relocs = parse_relocations(comp_dr, func)

            orig_call_counts = Counter(t for _, t in orig_relocs['calls'])
            comp_call_counts = Counter(t for _, t in comp_relocs['calls'])

            for sym in sorted(set(orig_call_counts.keys()) | set(comp_call_counts.keys())):
                o_count = orig_call_counts.get(sym, 0)
                c_count = comp_call_counts.get(sym, 0)
                if o_count > c_count:
                    dtype = 'INLINED_CALL'
                    type_counts[dtype] = type_counts.get(dtype, 0) + (o_count - c_count)
                    global_types[dtype] = global_types.get(dtype, 0) + (o_count - c_count)
                elif c_count > o_count:
                    dtype = 'MISSING_CALL'
                    type_counts[dtype] = type_counts.get(dtype, 0) + (c_count - o_count)
                    global_types[dtype] = global_types.get(dtype, 0) + (c_count - o_count)

        # Block-reorder detection
        is_reordered, reorder_summary = detect_block_reorder(orig, comp)
        if is_reordered:
            block_reordered.append((func, reorder_summary))

        # Determine if actionable
        fixable_types = {'SIGN_MISMATCH', 'TYPE_NARROW_COMP', 'EXTRA_NARROW_COMP',
                         'BRANCH_FLIP', 'FABS_MISSING', 'INLINED_CALL', 'MISSING_CALL'}
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

    if block_reordered:
        print(f"\n{'='*70}")
        print(f"BLOCK-REORDERED FUNCTIONS ({len(block_reordered)})")
        print(f"{'='*70}")
        for func, summary in block_reordered:
            print(f"\n  {func}:")
            print(f"    {summary}")

    return non_matching, global_types, actionable


# ============================================================
# AUTO-FIX ENGINE
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


def build():
    subprocess.run(['python', 'configure.py'], capture_output=True, text=True, timeout=30)
    r = subprocess.run(['python', '-m', 'ninja'], capture_output=True, text=True, timeout=60)
    return r.returncode == 0


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
        return abs(len(orig) - len(comp)) + \
               sum(1 for a, b in zip(orig[:min(len(orig), len(comp))],
                                      comp[:min(len(orig), len(comp))]) if a.raw != b.raw)

    return sum(1 for a, b in zip(orig, comp) if a.raw != b.raw)


def try_sign_fix(tu_name, src_file, func_name, diffs):
    """Try fixing SIGN_MISMATCH by changing s16 <-> u16, s32 <-> u32."""
    with open(src_file) as f:
        original = f.read()

    src_lines = original.split('\n')
    func_start, func_end = find_function_in_source(src_lines, func_name)
    if func_start is None:
        return False

    baseline = get_func_diff_count(tu_name, func_name)

    # For each SIGN_MISMATCH diff, look for the variable type
    sign_diffs = [d for d in diffs if d[0] == 'SIGN_MISMATCH']
    if not sign_diffs:
        return False

    # Try type swaps in the function region
    type_swaps = [
        ('s16', 'u16'), ('u16', 's16'),
        ('s32', 'u32'), ('u32', 's32'),
        ('s8', 'u8'), ('u8', 's8'),
    ]

    best_content = None
    best_diffs = baseline

    for old_type, new_type in type_swaps:
        modified = original
        # Only modify within the function
        for i in range(func_start, func_end + 1):
            line = src_lines[i]
            # Match type declarations: "s16 varName" or "(s16)" casts
            new_line = re.sub(r'\b' + old_type + r'\b', new_type, line)
            if new_line != line:
                modified = modified.replace(line, new_line, 1)

        if modified == original:
            continue

        with open(src_file, 'w') as f:
            f.write(modified)

        if build():
            new_diffs = get_func_diff_count(tu_name, func_name)
            if new_diffs < best_diffs:
                print(f"      IMPROVED: {old_type}->{new_type}: {baseline}->{new_diffs} diffs")
                best_diffs = new_diffs
                best_content = modified

    if best_content and best_diffs < baseline:
        with open(src_file, 'w') as f:
            f.write(best_content)
        build()
        return True

    # Revert
    with open(src_file, 'w') as f:
        f.write(original)
    return False


def try_branch_flip_fix(tu_name, src_file, func_name, diffs):
    """Try fixing BRANCH_FLIP by swapping if/else blocks."""
    with open(src_file) as f:
        original = f.read()

    src_lines = original.split('\n')
    func_start, func_end = find_function_in_source(src_lines, func_name)
    if func_start is None:
        return False

    baseline = get_func_diff_count(tu_name, func_name)

    # Find if/else blocks and try inverting the condition + swapping blocks
    # Look for simple if (!cond) -> if (cond) inversions
    improved = False
    for i in range(func_start, func_end + 1):
        line = src_lines[i].strip()

        # Simple negation: if (!x) -> if (x) and vice versa
        m = re.match(r'(\s*)if\s*\(\s*!(.*)\s*\)', src_lines[i])
        if m:
            indent = m.group(1)
            cond = m.group(2).strip()
            new_line = f'{indent}if ({cond})'
            old_line = src_lines[i]
            src_lines[i] = new_line

            test = '\n'.join(src_lines)
            with open(src_file, 'w') as f:
                f.write(test)

            if build():
                new_diffs = get_func_diff_count(tu_name, func_name)
                if new_diffs < baseline:
                    print(f"      IMPROVED: negate condition at line {i+1}: {baseline}->{new_diffs} diffs")
                    baseline = new_diffs
                    improved = True
                    continue

            # Revert this attempt
            src_lines[i] = old_line

        # Also try adding negation: if (x) -> if (!x)
        m = re.match(r'(\s*)if\s*\(\s*([^!].*)\s*\)', src_lines[i])
        if m and 'else' not in src_lines[i]:
            indent = m.group(1)
            cond = m.group(2).strip()
            if not cond.startswith('!'):
                new_line = f'{indent}if (!({cond}))'
                old_line = src_lines[i]
                src_lines[i] = new_line

                test = '\n'.join(src_lines)
                with open(src_file, 'w') as f:
                    f.write(test)

                if build():
                    new_diffs = get_func_diff_count(tu_name, func_name)
                    if new_diffs < baseline:
                        print(f"      IMPROVED: add negation at line {i+1}: {baseline}->{new_diffs} diffs")
                        baseline = new_diffs
                        improved = True
                        continue

                # Revert
                src_lines[i] = old_line

    if improved:
        with open(src_file, 'w') as f:
            f.write('\n'.join(src_lines))
        build()
        return True

    # Revert
    with open(src_file, 'w') as f:
        f.write(original)
    return False


def try_type_narrow_fix(tu_name, src_file, func_name, diffs):
    """Try fixing TYPE_NARROW_COMP by widening variable types (u8->s32, s16->s32)."""
    with open(src_file) as f:
        original = f.read()

    src_lines = original.split('\n')
    func_start, func_end = find_function_in_source(src_lines, func_name)
    if func_start is None:
        return False

    baseline = get_func_diff_count(tu_name, func_name)

    # Try widening narrow types to s32/u32
    widen_map = {
        'u8': ['s32', 'u32', 'int'],
        's8': ['s32', 'int'],
        's16': ['s32', 'int'],
        'u16': ['u32', 's32'],
    }

    best_content = None
    best_diffs = baseline

    for narrow_type, wide_types in widen_map.items():
        for wide_type in wide_types:
            modified_lines = src_lines[:]
            changed = False
            for i in range(func_start, func_end + 1):
                line = modified_lines[i].strip()
                # Only modify LOCAL VARIABLE DECLARATIONS (type varname = ...; or type varname;)
                # Do NOT modify casts like (u8*), pointer types, or other uses
                decl_pattern = r'^(\s*)' + narrow_type + r'(\s+\w+\s*[;=])'
                new_line = re.sub(decl_pattern, r'\1' + wide_type + r'\2', modified_lines[i])
                if new_line != modified_lines[i]:
                    modified_lines[i] = new_line
                    changed = True

            if not changed:
                continue

            modified = '\n'.join(modified_lines)
            with open(src_file, 'w') as f:
                f.write(modified)

            if build():
                new_diffs = get_func_diff_count(tu_name, func_name)
                if new_diffs < best_diffs:
                    print(f"      IMPROVED: {narrow_type}->{wide_type}: {baseline}->{new_diffs} diffs")
                    best_diffs = new_diffs
                    best_content = modified

    if best_content and best_diffs < baseline:
        with open(src_file, 'w') as f:
            f.write(best_content)
        build()
        return True

    with open(src_file, 'w') as f:
        f.write(original)
    return False


def autofix_function(tu_name, func_name, verbose=True):
    """Auto-fix actionable diffs in a function."""
    src_file = f'src/{tu_name}.cpp'
    result = analyze_function(tu_name, func_name, verbose=False)
    if not result or result['status'] != 'DIFFERS':
        return False

    type_counts = result['type_counts']
    diffs = result['diffs']

    fixable = set(type_counts.keys()) & {'SIGN_MISMATCH', 'TYPE_NARROW_COMP',
                                          'EXTRA_NARROW_COMP', 'BRANCH_FLIP'}
    if not fixable:
        if verbose:
            short = func_name.split('__')[0]
            print(f"  {short}: no auto-fixable patterns ({list(type_counts.keys())})")
        return False

    short = func_name.split('__')[0]
    print(f"  {short}: trying to fix {fixable}")

    improved = False

    if 'SIGN_MISMATCH' in fixable:
        if try_sign_fix(tu_name, src_file, func_name, diffs):
            improved = True

    if 'BRANCH_FLIP' in fixable:
        if try_branch_flip_fix(tu_name, src_file, func_name, diffs):
            improved = True

    if 'TYPE_NARROW_COMP' in fixable or 'EXTRA_NARROW_COMP' in fixable:
        if try_type_narrow_fix(tu_name, src_file, func_name, diffs):
            improved = True

    return improved


def autofix_all(tu_name):
    """Auto-fix all actionable functions in a TU."""
    print(f"\n{'='*70}")
    print(f"AUTO-FIX: {tu_name}")
    print(f"{'='*70}")

    if not build():
        print("ERROR: baseline build failed!")
        return

    baseline_matches = get_exact_match_count(tu_name)
    print(f"Baseline: {baseline_matches} exact matches")

    non_matching, global_types, actionable = analyze_all(tu_name)

    if not actionable:
        print("\nNo auto-fixable functions found.")
        return

    print(f"\n{'='*70}")
    print(f"ATTEMPTING FIXES ON {len(actionable)} FUNCTIONS")
    print(f"{'='*70}")

    fixed = 0
    for func, counts, fixable in actionable:
        if autofix_function(tu_name, func):
            # Regression check
            new_matches = get_exact_match_count(tu_name)
            if new_matches < baseline_matches:
                short = func.split('__')[0]
                print(f"  ** REGRESSION: {short} broke {baseline_matches - new_matches} matches! Reverting...")
                subprocess.run(['git', 'checkout', '--', f'src/{tu_name}.cpp'],
                              capture_output=True, text=True)
                build()
            else:
                fixed += 1
                if new_matches > baseline_matches:
                    print(f"  ** NEW MATCHES: {baseline_matches} -> {new_matches}")
                    baseline_matches = new_matches

    final_matches = get_exact_match_count(tu_name)
    print(f"\n{'='*70}")
    print(f"RESULTS: {fixed}/{len(actionable)} functions improved")
    print(f"Matches: {baseline_matches} -> {final_matches}")
    print(f"{'='*70}")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python tools/code_differ.py Player/MarioMove --all")
        print("  python tools/code_differ.py Player/MarioMove functionName")
        print("  python tools/code_differ.py Player/MarioMove --fix        # auto-fix all")
        print("  python tools/code_differ.py Player/MarioMove funcName --fix")
        sys.exit(1)

    tu = sys.argv[1]
    fix_mode = '--fix' in sys.argv

    if fix_mode:
        target = None
        for arg in sys.argv[2:]:
            if not arg.startswith('--'):
                target = arg
                break
        if target:
            if not build():
                print("Build failed!")
                sys.exit(1)
            autofix_function(tu, target)
        else:
            autofix_all(tu)
    elif '--all' in sys.argv:
        analyze_all(tu)
    elif len(sys.argv) >= 3 and not sys.argv[2].startswith('--'):
        analyze_function(tu, sys.argv[2])
    else:
        analyze_all(tu)
