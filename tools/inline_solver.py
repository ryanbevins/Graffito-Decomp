#!/usr/bin/env python3
"""
Inline Solver - Differential Compilation Matrix Builder + Constraint Solver

Phase 3+4 of the inline analysis pipeline:
1. Generates candidate inline accessors for TMario fields
2. Tests each candidate by compiling and measuring stack frame changes
3. Builds a matrix: candidate × function → stack delta
4. Solves for the minimum set of inlines that matches original stack frames

Usage:
  python tools/inline_solver.py Player/MarioMove          # build matrix + solve
  python tools/inline_solver.py Player/MarioMove --solve   # solve from cached matrix
"""

import subprocess
import json
import re
import sys
import os
import time
import copy

# Path to objdump
OBJDUMP = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        'build', 'binutils', 'powerpc-eabi-objdump.exe')


def get_stack_frames(obj_path):
    """Extract function name -> stack frame size from an object file."""
    r = subprocess.run([OBJDUMP, '-d', obj_path], capture_output=True, text=True, timeout=60)
    frames = {}
    current_func = None
    for line in r.stdout.split('\n'):
        m = re.match(r'^[0-9a-f]+ <(.+)>:', line)
        if m:
            current_func = m.group(1)
        if current_func and 'stwu' in line:
            m2 = re.search(r'stwu\s+r1,(-?\d+)\(r1\)', line)
            if m2:
                frames[current_func] = int(m2.group(1))
    return frames


def build():
    """Build the project. Returns True if successful."""
    subprocess.run(['python', 'configure.py'], capture_output=True, text=True, timeout=30)
    r = subprocess.run(['python', '-m', 'ninja'], capture_output=True, text=True, timeout=60)
    return r.returncode == 0


def get_gaps(tu_name):
    """Get functions with stack frame gaps: func -> (original, compiled, gap)."""
    orig = get_stack_frames(f'build/GMSJ01/obj/{tu_name}.o')
    comp = get_stack_frames(f'build/GMSJ01/src/{tu_name}.o')
    gaps = {}
    for func in orig:
        if func in comp and orig[func] != comp[func]:
            gap = orig[func] - comp[func]  # negative = original is larger
            if gap < 0:  # only care about functions where original has MORE stack
                gaps[func] = {'original': orig[func], 'compiled': comp[func], 'gap': gap}
    return gaps


# ============================================================
# CANDIDATE INLINE GENERATORS
# ============================================================

def generate_tmario_candidates():
    """Generate candidate inline accessor replacements for TMario fields."""
    candidates = []

    # Pattern: fabricated accessor on TMario
    # Each adds an inline function call that inflates stack by +8
    tmario_fields = [
        ('getHealth', 's16', 'mHealth', 'return mHealth;'),
        ('getAnimId', 'u16', 'mAnimationId', 'return mAnimationId;'),
        ('getModel', 'void*', 'unk10C', 'return unk10C;'),
        ('getAnmSound', 'void*', 'unk110', 'return unk110;'),
        ('getController', 'void*', 'mController', 'return mController;'),
        ('getWireSag', 'f32', 'mWireSag', 'return mWireSag;'),
        ('getWireBounceVel', 'f32', 'mWireBounceVel', 'return mWireBounceVel;'),
        ('getWirePosRatio', 'f32', 'mWirePosRatio', 'return mWirePosRatio;'),
        ('getYoshi', 'TYoshi*', 'mYoshi', 'return mYoshi;'),
        ('getGamePad', 'TMarioGamePad*', 'mGamePad', 'return mGamePad;'),
        ('getHeldObject', 'void*', 'mHeldObject', 'return mHeldObject;'),
        ('getHolder', 'void*', 'mHolder', 'return mHolder;'),
        ('getCollisionRadius', 'f32', 'unk15C', 'return unk15C;'),
        ('getMarioCap', 'void*', 'mCap', 'return mCap;'),
        ('getFloorHeight', 'f32', 'mFloorPosition.y', 'return mFloorPosition.y;'),
        ('getWaterFloor', 'const TBGCheckData*', 'mWaterFloor', 'return mWaterFloor;'),
        ('getRoofPlane', 'const TBGCheckData*', 'mRoofPlane', 'return mRoofPlane;'),
        ('getWallPlane', 'const TBGCheckData*', 'mWallPlane', 'return mWallPlane;'),
    ]

    for name, ret_type, field, body in tmario_fields:
        # Check if accessor already exists
        candidates.append({
            'name': name,
            'class': 'TMario',
            'return_type': ret_type,
            'field': field,
            'header_accessor': f'\t{ret_type} {name}() const {{ {body} }}',
            'search_pattern': field,  # what to look for in source to know if this is applicable
        })

    return candidates


def test_candidate(tu_name, src_file, header_file, candidate, baseline_frames):
    """
    Test a single candidate inline accessor.
    Returns dict of function -> stack delta, or None if build failed.
    """
    # Read original files
    with open(src_file, 'r') as f:
        src_orig = f.read()
    with open(header_file, 'r') as f:
        hdr_orig = f.read()

    try:
        # Add accessor to header inside TMario class
        hdr_new = hdr_orig
        marker = '\t// Fabricated accessors for inline stack inflation'
        if marker not in hdr_new:
            # Find TMario class, then find existing "// Fabricated" section
            tmario_idx = hdr_new.find('class TMario')
            if tmario_idx == -1:
                return None
            fab_idx = hdr_new.find('// Fabricated', tmario_idx)
            if fab_idx == -1:
                return None
            # Insert before the first Fabricated comment
            insert_pos = hdr_new.rfind('\n', 0, fab_idx) + 1
            hdr_new = hdr_new[:insert_pos] + marker + '\n' + candidate['header_accessor'] + '\n\n' + hdr_new[insert_pos:]
        else:
            # Append after marker
            marker_end = hdr_new.find('\n', hdr_new.find(marker)) + 1
            hdr_new = hdr_new[:marker_end] + candidate['header_accessor'] + '\n' + hdr_new[marker_end:]

        # Replace ONE instance of direct field access with accessor call
        # This makes the compiler "see" the inline and inflate the stack
        src_new = src_orig
        field = candidate['field']
        accessor = candidate['name'] + '()'

        # Strategy: replace first occurrence of the field in the source
        # For fields like 'mHealth', replace 'mHealth' with 'getHealth()'
        # But be careful not to replace inside comments, strings, or the accessor itself
        # Simple approach: replace the first standalone occurrence
        import re
        # Match field access as a standalone word (not part of another identifier)
        pattern = r'(?<![.\w])' + re.escape(field) + r'(?!\w)'
        match = re.search(pattern, src_new)
        if match:
            src_new = src_new[:match.start()] + accessor + src_new[match.end():]

        # Write modified files
        with open(header_file, 'w') as f:
            f.write(hdr_new)
        with open(src_file, 'w') as f:
            f.write(src_new)

        # Build
        if not build():
            return None

        # Get new stack frames
        new_frames = get_stack_frames(f'build/GMSJ01/src/{tu_name}.o')

        # Compare with baseline
        deltas = {}
        for func in baseline_frames:
            if func in new_frames and baseline_frames[func] != new_frames[func]:
                deltas[func] = new_frames[func] - baseline_frames[func]

        return deltas

    finally:
        # ALWAYS restore originals
        with open(src_file, 'w') as f:
            f.write(src_orig)
        with open(header_file, 'w') as f:
            f.write(hdr_orig)


def build_matrix(tu_name, src_file, header_file):
    """Build the full differential compilation matrix."""
    print(f"Building inline matrix for {tu_name}...")
    print(f"Source: {src_file}")
    print(f"Header: {header_file}")

    # Get baseline
    if not build():
        print("ERROR: baseline build failed!")
        return None

    baseline = get_stack_frames(f'build/GMSJ01/src/{tu_name}.o')
    orig_frames = get_stack_frames(f'build/GMSJ01/obj/{tu_name}.o')

    # Get gaps
    gaps = {}
    for func in orig_frames:
        if func in baseline and orig_frames[func] != baseline[func]:
            gap = orig_frames[func] - baseline[func]
            if gap < 0:
                gaps[func] = gap

    if not gaps:
        print("No stack frame gaps found!")
        return None

    print(f"\nFunctions with gaps: {len(gaps)}")
    for func, gap in sorted(gaps.items(), key=lambda x: x[1]):
        print(f"  {func}: {gap}")

    # Generate candidates
    candidates = generate_tmario_candidates()
    print(f"\nTesting {len(candidates)} candidates...")

    # Test each candidate
    matrix = {}
    start = time.time()

    for i, cand in enumerate(candidates):
        elapsed = time.time() - start
        eta = (elapsed / (i + 1)) * (len(candidates) - i - 1) if i > 0 else 0
        print(f"  [{i+1}/{len(candidates)}] {cand['name']:<30} (ETA: {eta:.0f}s)", end='', flush=True)

        deltas = test_candidate(tu_name, src_file, header_file, cand, baseline)

        if deltas is None:
            print(" BUILD FAILED")
            matrix[cand['name']] = 'FAILED'
        elif deltas:
            print(f" -> affects {len(deltas)} functions")
            matrix[cand['name']] = deltas
        else:
            print(" -> no effect")
            matrix[cand['name']] = {}

    # Restore baseline build
    build()

    total = time.time() - start
    print(f"\nMatrix built in {total:.1f}s ({total/len(candidates):.1f}s per candidate)")

    return {
        'tu': tu_name,
        'gaps': gaps,
        'original_frames': {k: v for k, v in orig_frames.items() if k in gaps},
        'baseline_frames': {k: v for k, v in baseline.items() if k in gaps},
        'matrix': matrix,
    }


def solve(data):
    """
    Solve for the minimum set of inlines that matches original stack frames.
    Uses greedy set cover approach.
    """
    gaps = data['gaps']  # func -> gap (negative)
    matrix = data['matrix']  # candidate -> {func -> delta}

    # Filter to candidates that actually affect gap functions
    useful = {}
    for cand, deltas in matrix.items():
        if isinstance(deltas, dict) and deltas:
            relevant = {f: d for f, d in deltas.items() if f in gaps}
            if relevant:
                useful[cand] = relevant

    if not useful:
        print("No useful candidates found!")
        return []

    print(f"\nUseful candidates: {len(useful)}")
    for cand, deltas in sorted(useful.items(), key=lambda x: -len(x[1])):
        print(f"  {cand}: affects {len(deltas)} gap functions")
        for func, delta in sorted(deltas.items()):
            target_gap = gaps[func]
            print(f"    {func}: {delta:+d} (need {target_gap})")

    # Greedy solver: pick the candidate that closes the most gap
    remaining_gaps = dict(gaps)
    solution = []

    while remaining_gaps:
        best_cand = None
        best_score = 0

        for cand, deltas in useful.items():
            if cand in [s[0] for s in solution]:
                continue  # already used
            score = 0
            for func, delta in deltas.items():
                if func in remaining_gaps:
                    # Does this delta move us closer to 0?
                    old_gap = remaining_gaps[func]
                    new_gap = old_gap - delta  # gap is negative, delta is negative
                    if abs(new_gap) < abs(old_gap):
                        score += abs(old_gap) - abs(new_gap)

            if score > best_score:
                best_score = score
                best_cand = cand

        if best_cand is None or best_score == 0:
            break

        solution.append((best_cand, useful[best_cand]))

        # Update remaining gaps
        for func, delta in useful[best_cand].items():
            if func in remaining_gaps:
                remaining_gaps[func] -= delta
                if remaining_gaps[func] == 0:
                    del remaining_gaps[func]

    print(f"\n{'='*60}")
    print(f"SOLUTION: {len(solution)} inlines needed")
    print(f"{'='*60}")
    for cand, deltas in solution:
        print(f"  {cand}")

    if remaining_gaps:
        print(f"\nUNSOLVED gaps ({len(remaining_gaps)} functions):")
        for func, gap in sorted(remaining_gaps.items(), key=lambda x: x[1]):
            print(f"  {func}: {gap}")

    return solution


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python tools/inline_solver.py <TU_NAME>")
        print("  e.g.: python tools/inline_solver.py Player/MarioMove")
        sys.exit(1)

    tu = sys.argv[1]

    cache_file = f'tools/inline_matrix_{tu.replace("/", "_")}.json'

    if '--solve' in sys.argv and os.path.exists(cache_file):
        with open(cache_file) as f:
            data = json.load(f)
        solve(data)
    else:
        src_file = f'src/{tu}.cpp'
        header_file = 'include/Player/MarioMain.hpp'

        data = build_matrix(tu, src_file, header_file)
        if data:
            with open(cache_file, 'w') as f:
                json.dump(data, f, indent=2)
            print(f"\nMatrix cached to {cache_file}")
            solve(data)
