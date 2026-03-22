#!/usr/bin/env python3
"""
Inline Solver v3 - Differential Compilation Matrix Builder + Solver

Tests each candidate inline accessor by replacing ALL read occurrences
in the source, compiling, and measuring stack frame changes.

Usage:
  python tools/inline_solver.py Player/MarioMove
  python tools/inline_solver.py Player/MarioMove --apply  # apply found solutions
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


def build():
    subprocess.run(['python', 'configure.py'], capture_output=True, text=True, timeout=30)
    r = subprocess.run(['python', '-m', 'ninja'], capture_output=True, text=True, timeout=60)
    return r.returncode == 0


def replace_reads(source, field, accessor):
    """Replace all READ occurrences of field with accessor, skip writes."""
    lines = source.split('\n')
    result = []
    count = 0

    for idx, line in enumerate(lines):
        stripped = line.strip()

        # Skip comments
        if stripped.startswith('//') or stripped.startswith('/*'):
            result.append(line)
            continue

        # Skip declarations, PARAM_INIT, header markers
        if '/* 0x' in line or 'PARAM_INIT' in line or 'const {' in line:
            result.append(line)
            continue

        # For dotted fields like mFloorPosition.y, we need exact match
        # Replace only READ occurrences, skip writes
        # A write is: field immediately followed by assignment op (=, +=, -=, etc.)
        # A read is everything else (comparisons, function args, RHS of assignments)

        # Join with next line to detect multi-line assignments like:
        #   mVel.y
        #       = expr;
        next_stripped = ''
        if idx + 1 < len(lines):
            next_stripped = lines[idx + 1].lstrip()

        if '.' in field:
            pattern = re.escape(field)
        else:
            pattern = r'(?<![.\w])' + re.escape(field) + r'(?![.\w(])'

        new_line = line
        offset = 0
        for m in re.finditer(pattern, line):
            start, end = m.start(), m.end()

            # Skip if preceded by -> (other object's field)
            if start >= 2 and line[start-2:start] == '->':
                continue

            # Check what follows on THIS line
            rest = line[end:].lstrip()

            # Check for assignment on same line
            is_write = False
            if rest and rest[0] == '=' and (len(rest) < 2 or rest[1] != '='):
                is_write = True
            elif len(rest) >= 2 and rest[0] in '+-|&*' and rest[1] == '=':
                is_write = True

            # Check for assignment on NEXT line (multi-line pattern)
            if not is_write and not rest:
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


def test_candidate(tu_name, src_file, header_file, candidate, baseline):
    """Test a candidate accessor. Returns {func: delta} or None on failure."""
    with open(src_file) as f:
        src_orig = f.read()
    with open(header_file) as f:
        hdr_orig = f.read()

    try:
        # Add accessor to header
        hdr = hdr_orig
        tmario_idx = hdr.find('class TMario')
        fab_idx = hdr.find('// Fabricated', tmario_idx)
        if tmario_idx == -1 or fab_idx == -1:
            return None
        pos = hdr.rfind('\n', 0, fab_idx) + 1
        hdr = hdr[:pos] + candidate['header'] + '\n' + hdr[pos:]

        # Replace reads in source
        src, count = replace_reads(src_orig, candidate['field'], candidate['call'])

        if count == 0:
            return {}

        # Write and build
        with open(header_file, 'w') as f:
            f.write(hdr)
        with open(src_file, 'w') as f:
            f.write(src)

        if not build():
            return None

        # Measure
        new_frames = get_stack_frames(f'build/GMSJ01/src/{tu_name}.o')
        deltas = {}
        for func in baseline:
            if func in new_frames and baseline[func] != new_frames[func]:
                deltas[func] = new_frames[func] - baseline[func]
        return deltas

    finally:
        with open(src_file, 'w') as f:
            f.write(src_orig)
        with open(header_file, 'w') as f:
            f.write(hdr_orig)


def generate_candidates():
    """All candidate inline accessors."""
    C = []

    def add(name, field, ret, body=None):
        if body is None:
            body = f'return {field};'
        C.append({
            'name': name,
            'field': field,
            'call': name + '()',
            'header': f'\t{ret} {name}() const {{ {body} }}',
        })

    # TMario fields
    add('getHealth', 'mHealth', 's32', 'return (s32)mHealth;')
    add('getAnimId', 'mAnimationId', 'u16')
    add('getYoshiPtr', 'mYoshi', 'TYoshi*')
    add('getGamePad', 'mGamePad', 'TMarioGamePad*')
    add('getHeldObj', 'mHeldObject', 'void*')
    add('getColRadius', 'unk15C', 'f32')
    add('getWallPl', 'mWallPlane', 'const TBGCheckData*')
    add('getRideAct', 'mRidingActor', 'TLiveActor*')
    add('getDashSp', 'mDashSpeed', 'f32')
    add('getDashTm', 'mDashTimer', 's16')
    add('getLastGndY', 'mLastGroundY', 'f32')
    add('getHolderHDiff', 'mHolderHeightDiff', 'f32')
    add('getPollType', 'unk350', 's32')
    add('getHeightAbv', 'unk370', 'f32')
    add('getMaxHeight', 'unk36C', 'f32')
    add('getDirtyAmt', 'unk134', 'f32')
    add('getSpeedBonus', 'unk368', 'f32')
    add('getFootTimer', 'unk360', 's16')
    add('getWetTimer', 'unk362', 's16')
    add('getDmgTimer', 'unk2BA', 's16')
    add('getSlideAng', 'mSlideAngle', 's16')
    add('getFlags78', 'unk78', 'u32')
    add('getWaterFl', 'mWaterFloor', 'const TBGCheckData*')

    # Dotted field accessors (struct member access)
    add('getFloorY', 'mFloorPosition.y', 'f32')
    add('getFloorZ', 'mFloorPosition.z', 'f32')
    add('getFloorX', 'mFloorPosition.x', 'f32')
    add('getPosX', 'mPosition.x', 'f32')
    add('getPosY', 'mPosition.y', 'f32')
    add('getPosZ', 'mPosition.z', 'f32')
    add('getVelX', 'mVel.x', 'f32')
    add('getVelY', 'mVel.y', 'f32')
    add('getVelZ', 'mVel.z', 'f32')
    add('getFaceY', 'mFaceAngle.y', 's16')
    add('getFaceX', 'mFaceAngle.x', 's16')

    # unk fields commonly accessed
    add('getUnk12C', 'unk12C', 'f32')
    add('getUnk130', 'unk130', 'f32')
    add('getUnk138', 'unk138', 'f32')
    add('getUnk13C', 'unk13C', 'f32')
    add('getUnk374', 'unk374', 'f32')
    add('getUnk378', 'unk378', 'f32')
    add('getUnk104', 'unk104', 'f32')
    add('getUnk9C', 'unk9C', 's16')
    add('getUnkA0', 'unkA0', 's16')

    return C


def run(tu_name, apply_mode=False):
    src_file = f'src/{tu_name}.cpp'
    header_file = 'include/Player/MarioMain.hpp'

    print(f"{'='*70}")
    print(f"INLINE SOLVER v3: {tu_name}")
    print(f"{'='*70}")

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

    print(f"\nFunctions with gaps: {len(gaps)}")
    total_gap = sum(abs(g) for g in gaps.values())
    for func, gap in sorted(gaps.items(), key=lambda x: x[1]):
        print(f"  {func}: {gap} ({abs(gap)//8} inlines)")
    print(f"  Total: {total_gap} bytes ({total_gap//8} inlines)")

    candidates = generate_candidates()
    print(f"\nTesting {len(candidates)} candidates...\n")

    matrix = {}
    useful = {}
    start = time.time()

    for i, cand in enumerate(candidates):
        elapsed = time.time() - start
        eta = (elapsed / max(i, 1)) * (len(candidates) - i)
        print(f"  [{i+1:>2}/{len(candidates)}] {cand['name']:<20}", end='', flush=True)

        deltas = test_candidate(tu_name, src_file, header_file, cand, baseline)

        if deltas is None:
            print(f" FAILED")
        elif deltas:
            affected = {f: d for f, d in deltas.items() if f in gaps}
            if affected:
                total_reduction = sum(abs(d) for d in affected.values())
                print(f" -> {len(affected)} funcs, -{total_reduction} bytes")
                useful[cand['name']] = {'deltas': affected, 'candidate': cand}
            else:
                other = len(deltas)
                print(f" -> {other} non-gap funcs only")
        else:
            print(f" -> no effect")

    build()  # restore baseline
    total_time = time.time() - start
    print(f"\nDone in {total_time:.0f}s ({total_time/len(candidates):.1f}s each)")

    if not useful:
        print("\nNo useful candidates found.")
        return

    # Results
    print(f"\n{'='*70}")
    print(f"FOUND {len(useful)} USEFUL ACCESSORS")
    print(f"{'='*70}")

    for name, info in sorted(useful.items(), key=lambda x: -sum(abs(d) for d in x[1]['deltas'].values())):
        total_d = sum(abs(d) for d in info['deltas'].values())
        print(f"\n  {name} (-{total_d} total):")
        for func, d in sorted(info['deltas'].items()):
            remain = gaps[func] - d
            print(f"    {func}: {d:+d} (gap {gaps[func]} -> {remain})")

    # Projection
    print(f"\n{'='*70}")
    print(f"PROJECTED IMPACT")
    print(f"{'='*70}")

    combined = {}
    for name, info in useful.items():
        for func, d in info['deltas'].items():
            combined[func] = combined.get(func, 0) + d

    solved = 0
    reduced = 0
    for func in sorted(gaps.keys(), key=lambda x: gaps[x]):
        g = gaps[func]
        r = combined.get(func, 0)
        new_g = g - r
        if new_g == 0:
            print(f"  {func}: {g} -> SOLVED!")
            solved += 1
        else:
            print(f"  {func}: {g} -> {new_g}")
        reduced += abs(r)

    print(f"\n  Solved: {solved}/{len(gaps)} functions")
    print(f"  Reduced: {reduced}/{total_gap} bytes ({100*reduced/total_gap:.0f}%)")

    # Cache
    cache_file = f'tools/inline_cache_{tu_name.replace("/", "_")}.json'
    cache = {
        'tu': tu_name, 'gaps': gaps, 'useful': {
            k: {'deltas': v['deltas'], 'header': v['candidate']['header'],
                'field': v['candidate']['field'], 'call': v['candidate']['call']}
            for k, v in useful.items()
        }
    }
    with open(cache_file, 'w') as f:
        json.dump(cache, f, indent=2)
    print(f"\nCached to {cache_file}")

    # Apply mode
    if apply_mode and useful:
        print(f"\n{'='*70}")
        print(f"APPLYING {len(useful)} ACCESSORS")
        print(f"{'='*70}")

        with open(header_file) as f:
            hdr = f.read()
        with open(src_file) as f:
            src = f.read()

        # Add all headers
        tmario_idx = hdr.find('class TMario')
        fab_idx = hdr.find('// Fabricated', tmario_idx)
        pos = hdr.rfind('\n', 0, fab_idx) + 1
        header_block = '\t// Auto-discovered inline accessors\n'
        for name, info in useful.items():
            header_block += info['candidate']['header'] + '\n'
        hdr = hdr[:pos] + header_block + '\n' + hdr[pos:]

        # Apply all replacements
        total_replacements = 0
        for name, info in useful.items():
            src, n = replace_reads(src, info['candidate']['field'], info['candidate']['call'])
            total_replacements += n
            print(f"  {name}: {n} replacements")

        with open(header_file, 'w') as f:
            f.write(hdr)
        with open(src_file, 'w') as f:
            f.write(src)

        print(f"\n  Total: {total_replacements} replacements")
        print(f"  Building...")

        if build():
            print(f"  BUILD OK!")
            new_frames = get_stack_frames(f'build/GMSJ01/src/{tu_name}.o')
            print(f"\n  Updated gaps:")
            new_total = 0
            for func in sorted(gaps.keys(), key=lambda x: gaps[x]):
                if func in new_frames and func in orig:
                    new_gap = orig[func] - new_frames[func]
                    old_gap = gaps[func]
                    if new_gap < 0:
                        new_total += abs(new_gap)
                    status = "SOLVED!" if new_gap == 0 else f"{new_gap}"
                    if new_gap != old_gap:
                        print(f"    {func}: {old_gap} -> {new_gap} {status}")
                    else:
                        print(f"    {func}: {old_gap} (unchanged)")
            print(f"\n  Remaining gap: {new_total} bytes (was {total_gap})")
        else:
            print(f"  BUILD FAILED! Reverting...")
            with open(header_file, 'w') as f:
                f.write(cache['hdr_orig'])
            with open(src_file, 'w') as f:
                f.write(cache['src_orig'])


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python tools/inline_solver.py <TU> [--apply]")
        sys.exit(1)
    apply_mode = '--apply' in sys.argv
    run(sys.argv[1], apply_mode)
