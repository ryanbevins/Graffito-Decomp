#!/usr/bin/env python3
"""
Inline Solver v2 - Differential Compilation Matrix Builder + Solver

Tests each candidate inline accessor by replacing ALL occurrences
in the source, compiling, and measuring stack frame changes across
every function simultaneously.

Usage:
  python tools/inline_solver.py Player/MarioMove
"""

import subprocess
import re
import sys
import os
import time
import json

OBJDUMP = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        'build', 'binutils', 'powerpc-eabi-objdump.exe')


def get_stack_frames(obj_path):
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
    subprocess.run(['python', 'configure.py'], capture_output=True, text=True, timeout=30)
    r = subprocess.run(['python', '-m', 'ninja'], capture_output=True, text=True, timeout=60)
    return r.returncode == 0


def get_gaps(tu_name):
    orig = get_stack_frames(f'build/GMSJ01/obj/{tu_name}.o')
    comp = get_stack_frames(f'build/GMSJ01/src/{tu_name}.o')
    gaps = {}
    for func in orig:
        if func in comp and orig[func] != comp[func]:
            gap = orig[func] - comp[func]
            if gap < 0:
                gaps[func] = gap
    return gaps


# ============================================================
# CANDIDATES
# ============================================================

def generate_candidates():
    """Generate candidate inline accessor replacements."""
    candidates = []

    # Each candidate: (name, field_pattern, replacement, header_line)
    # field_pattern: regex to find in source (must match exactly the field access)
    # replacement: what to replace it with
    # header_line: line to add to TMario class

    # TMario field accessors
    tmario = [
        # Field name       Accessor call         Return type   Header definition
        ('mHealth',        'getHealth()',         's16',        's16 getHealth() const { return mHealth; }'),
        ('mWaterGun',      'getWaterGun()',       'TWaterGun*', 'TWaterGun* getWaterGun() const { return (TWaterGun*)mModel; }'),
        ('mYoshi',         'getYoshiPtr()',       'TYoshi*',    'TYoshi* getYoshiPtr() const { return mYoshi; }'),
        ('mGamePad',       'getGamePad()',        'TMarioGamePad*', 'TMarioGamePad* getGamePad() const { return mGamePad; }'),
        ('mHeldObject',    'getHeldObj()',        'void*',      'void* getHeldObj() const { return mHeldObject; }'),
        ('mHolder',        'getHolderPtr()',      'void*',      'void* getHolderPtr() const { return mHolder; }'),
        ('unk15C',         'getColRadius()',      'f32',        'f32 getColRadius() const { return unk15C; }'),
        ('mWallPlane',     'getWallPl()',         'const TBGCheckData*', 'const TBGCheckData* getWallPl() const { return mWallPlane; }'),
        ('mRoofPlane',     'getRoofPl()',         'const TBGCheckData*', 'const TBGCheckData* getRoofPl() const { return mRoofPlane; }'),
        ('mWaterFloor',    'getWaterFl()',        'const TBGCheckData*', 'const TBGCheckData* getWaterFl() const { return mWaterFloor; }'),
        ('mFloorPosition.y', 'getFloorY()',       'f32',        'f32 getFloorY() const { return mFloorPosition.y; }'),
        ('mFloorPosition.z', 'getFloorZ()',       'f32',        'f32 getFloorZ() const { return mFloorPosition.z; }'),
        ('mFloorPosition.x', 'getFloorX()',       'f32',        'f32 getFloorX() const { return mFloorPosition.x; }'),
        ('mDashSpeed',     'getDashSp()',         'f32',        'f32 getDashSp() const { return mDashSpeed; }'),
        ('mDashTimer',     'getDashTm()',         's16',        's16 getDashTm() const { return mDashTimer; }'),
        ('mLastGroundY',   'getLastGndY()',       'f32',        'f32 getLastGndY() const { return mLastGroundY; }'),
        ('mRidingActor',   'getRideAct()',        'TLiveActor*','TLiveActor* getRideAct() const { return mRidingActor; }'),
        ('mHolderHeightDiff','getHolderHDiff()',  'f32',        'f32 getHolderHDiff() const { return mHolderHeightDiff; }'),
        ('unk350',         'getPollType()',       's32',        's32 getPollType() const { return unk350; }'),
        ('unk370',         'getHeightAbv()',      'f32',        'f32 getHeightAbv() const { return unk370; }'),
        ('unk36C',         'getMaxHeight()',      'f32',        'f32 getMaxHeight() const { return unk36C; }'),
        ('unk134',         'getDirtyAmt()',       'f32',        'f32 getDirtyAmt() const { return unk134; }'),
        ('unk368',         'getSpeedBonus()',     'f32',        'f32 getSpeedBonus() const { return unk368; }'),
        ('unk360',         'getFootTimer()',      's16',        's16 getFootTimer() const { return unk360; }'),
        ('unk362',         'getWetTimer()',       's16',        's16 getWetTimer() const { return unk362; }'),
        ('unk2BA',         'getDmgTimer()',       's16',        's16 getDmgTimer() const { return unk2BA; }'),
        ('mSlideAngle',    'getSlideAng()',       's16',        's16 getSlideAng() const { return mSlideAngle; }'),
        ('unk78',          'getFlags78()',        'u32',        'u32 getFlags78() const { return unk78; }'),
    ]

    for field, accessor, ret_type, header_def in tmario:
        candidates.append({
            'name': accessor.replace('()', ''),
            'field': field,
            'accessor': accessor,
            'header_line': '\t' + header_def,
        })

    return candidates


def test_candidate(tu_name, src_file, header_file, candidate, baseline_frames):
    """Test a candidate by replacing ALL occurrences in source."""
    with open(src_file, 'r') as f:
        src_orig = f.read()
    with open(header_file, 'r') as f:
        hdr_orig = f.read()

    try:
        # 1. Add accessor to header
        hdr_new = hdr_orig
        tmario_idx = hdr_new.find('class TMario')
        if tmario_idx == -1:
            return None
        fab_idx = hdr_new.find('// Fabricated', tmario_idx)
        if fab_idx == -1:
            return None
        insert_pos = hdr_new.rfind('\n', 0, fab_idx) + 1
        hdr_new = hdr_new[:insert_pos] + candidate['header_line'] + '\n' + hdr_new[insert_pos:]

        # 2. Replace ALL occurrences of field with accessor in source
        field = candidate['field']
        accessor = candidate['accessor']
        src_new = src_orig

        # Build regex: match the field as a standalone read (not assignment target)
        # Avoid replacing inside strings, comments, or when field is on LHS of =
        # Simple approach: replace 'field' with 'accessor()' when preceded by
        # common read patterns and NOT followed by ' =' or '+=' etc.

        # For simple fields like mHealth, mWallPlane, unk350:
        # Replace reads but not writes
        # A "read" is when field appears and is NOT immediately followed by assignment ops

        lines = src_new.split('\n')
        new_lines = []
        count = 0
        for line in lines:
            # Skip comments and the header_line itself
            stripped = line.strip()
            if stripped.startswith('//') or stripped.startswith('/*'):
                new_lines.append(line)
                continue

            # Don't replace on lines where field is being assigned to
            # (field = ..., field +=, field -=, field |=, field &=, field *=)
            assign_pattern = re.compile(r'\b' + re.escape(field) + r'\s*[+\-|&*]?=')
            if assign_pattern.search(line):
                new_lines.append(line)
                continue

            # Don't replace field declarations or struct member definitions
            if '/* 0x' in line or 'PARAM_INIT' in line:
                new_lines.append(line)
                continue

            # Replace standalone reads
            pattern = r'(?<![.\w])' + re.escape(field) + r'(?![.\w(])'
            new_line, n = re.subn(pattern, accessor, line)
            if n > 0:
                count += n
            new_lines.append(new_line)

        if count == 0:
            return {}  # field not used in source

        src_new = '\n'.join(new_lines)

        # Write
        with open(header_file, 'w') as f:
            f.write(hdr_new)
        with open(src_file, 'w') as f:
            f.write(src_new)

        # Build
        if not build():
            return None

        # Measure
        new_frames = get_stack_frames(f'build/GMSJ01/src/{tu_name}.o')
        deltas = {}
        for func in baseline_frames:
            if func in new_frames and baseline_frames[func] != new_frames[func]:
                deltas[func] = new_frames[func] - baseline_frames[func]

        return deltas

    finally:
        with open(src_file, 'w') as f:
            f.write(src_orig)
        with open(header_file, 'w') as f:
            f.write(hdr_orig)


def run(tu_name):
    src_file = f'src/{tu_name}.cpp'
    header_file = 'include/Player/MarioMain.hpp'

    print(f"{'='*70}")
    print(f"INLINE SOLVER v2: {tu_name}")
    print(f"{'='*70}")

    # Baseline
    if not build():
        print("ERROR: baseline build failed!")
        return

    baseline = get_stack_frames(f'build/GMSJ01/src/{tu_name}.o')
    orig = get_stack_frames(f'build/GMSJ01/obj/{tu_name}.o')

    gaps = {}
    for func in orig:
        if func in baseline and orig[func] != baseline[func]:
            gap = orig[func] - baseline[func]
            if gap < 0:
                gaps[func] = gap

    print(f"\nFunctions with stack gaps: {len(gaps)}")
    for func, gap in sorted(gaps.items(), key=lambda x: x[1]):
        n = abs(gap) // 8
        print(f"  {func}: {gap} ({n} inlines needed)")

    candidates = generate_candidates()
    print(f"\nTesting {len(candidates)} candidates (replacing ALL occurrences)...\n")

    matrix = {}
    start = time.time()

    for i, cand in enumerate(candidates):
        elapsed = time.time() - start
        eta = (elapsed / max(i, 1)) * (len(candidates) - i)
        print(f"  [{i+1:>2}/{len(candidates)}] {cand['name']:<25}", end='', flush=True)

        deltas = test_candidate(tu_name, src_file, header_file, cand, baseline)

        if deltas is None:
            print(f" FAILED")
            matrix[cand['name']] = 'FAILED'
        elif deltas:
            affected = {f: d for f, d in deltas.items() if f in gaps}
            if affected:
                print(f" -> {len(affected)} gap functions affected!")
                for f, d in sorted(affected.items()):
                    print(f"         {f}: {d:+d}")
                matrix[cand['name']] = affected
            else:
                print(f" -> affects {len(deltas)} non-gap functions")
                matrix[cand['name']] = {}
        else:
            print(f" -> no effect")
            matrix[cand['name']] = {}

    # Rebuild baseline
    build()

    total = time.time() - start
    print(f"\nMatrix built in {total:.1f}s ({total/len(candidates):.1f}s per candidate)")

    # Solve
    useful = {k: v for k, v in matrix.items() if isinstance(v, dict) and v}
    if not useful:
        print("\nNo useful candidates found.")
        return

    print(f"\n{'='*70}")
    print(f"RESULTS: {len(useful)} useful accessors found")
    print(f"{'='*70}")

    for cand, deltas in sorted(useful.items(), key=lambda x: -len(x[1])):
        print(f"\n  {cand}:")
        for func, delta in sorted(deltas.items()):
            target = gaps.get(func, 0)
            remaining = target - delta
            print(f"    {func}: {delta:+d} (gap {target} -> {remaining})")

    # Summary: what would applying ALL useful candidates do?
    print(f"\n{'='*70}")
    print(f"PROJECTED IMPACT (all useful candidates applied)")
    print(f"{'='*70}")

    combined = {}
    for cand, deltas in useful.items():
        for func, delta in deltas.items():
            combined[func] = combined.get(func, 0) + delta

    for func in sorted(gaps.keys(), key=lambda x: gaps[x]):
        orig_gap = gaps[func]
        reduction = combined.get(func, 0)
        new_gap = orig_gap - reduction
        status = "SOLVED!" if new_gap == 0 else f"{new_gap} remaining"
        print(f"  {func}: {orig_gap} -> {new_gap} ({status})")

    # Cache
    cache = {'tu': tu_name, 'gaps': gaps, 'matrix': matrix, 'useful': useful}
    cache_file = f'tools/inline_matrix_{tu_name.replace("/", "_")}.json'
    with open(cache_file, 'w') as f:
        json.dump(cache, f, indent=2)
    print(f"\nCached to {cache_file}")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python tools/inline_solver.py <TU_NAME>")
        sys.exit(1)
    run(sys.argv[1])
