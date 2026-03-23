#!/usr/bin/env python3
"""
Inline Solver v4 - Automated Inline Discovery

Scans source code for field accesses, generates candidate accessors
automatically, tests each via differential compilation, validates
against match percentage and regressions.

Usage:
  python tools/inline_solver.py Player/MarioMove
  python tools/inline_solver.py Player/MarioMove --apply
"""

import subprocess
import re
import sys
import os
import time
import json
import hashlib

OBJDUMP = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        'build', 'binutils', 'powerpc-eabi-objdump.exe')


# ============================================================
# LOW-LEVEL HELPERS
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


def get_match_percentages(tu_name):
    """Get function match percentages from check_match.py."""
    r = subprocess.run(
        ['python', 'tools/claude/check_match.py', tu_name],
        capture_output=True, text=True, timeout=30
    )
    matches = {}
    for line in r.stdout.split('\n'):
        m = re.match(r'\s+(.+?):\s+(MATCH|[\d.]+%)\s+\((\d+) bytes\)', line)
        if m:
            func_name = m.group(1).strip()
            pct = 100.0 if m.group(2) == 'MATCH' else float(m.group(2).replace('%', ''))
            matches[func_name] = pct
    return matches


def get_exact_matches(tu_name):
    """Count exact byte-matching functions by comparing object files."""
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
                # Extract raw bytes
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
            if orig_funcs[func] == comp_funcs[func]:
                exact[func] = True
            else:
                exact[func] = False
    return exact


def build():
    subprocess.run(['python', 'configure.py'], capture_output=True, text=True, timeout=30)
    r = subprocess.run(['python', '-m', 'ninja'], capture_output=True, text=True, timeout=60)
    return r.returncode == 0


def get_weak_symbols(obj_path):
    r = subprocess.run([OBJDUMP, '-t', obj_path], capture_output=True, text=True, timeout=60)
    weaks = {}
    for line in r.stdout.split('\n'):
        if '  w  ' in line and ' F ' in line:
            parts = line.split()
            if len(parts) >= 6:
                weaks[parts[5]] = int(parts[4], 16)
    return weaks


def file_hash(path):
    with open(path, 'rb') as f:
        return hashlib.md5(f.read()).hexdigest()


# ============================================================
# FIELD SCANNER - Auto-discover fields from source
# ============================================================

def scan_fields(src_file):
    """Scan source file for all `this->field` access patterns.
    Returns dict: field_name -> {read_count, write_count, lines}
    """
    with open(src_file) as f:
        src = f.read()

    fields = {}

    # Pattern 1: Direct member access (mFieldName, unkXXX)
    # These are fields accessed as bare names (implicit this->)
    for m in re.finditer(r'\b(m[A-Z]\w+|unk[0-9A-Fa-f]+)\b', src):
        name = m.group(1)
        # Skip common non-field patterns
        if name in ('mPosition', 'mVel', 'mFaceAngle', 'mFloorPosition',
                     'mLastSafePos', 'mLastGroundPos', 'mRideLocalPos',
                     'mRidePrevLocalPos', 'mJointPos', 'mGroundMtx',
                     'mJointMtx0', 'mJointMtx1', 'mJointMtx2', 'mJointMtx3'):
            continue  # These are structs/arrays, not simple fields
        if name.startswith('mDe') or name.startswith('mJump') or name.startswith('mRun'):
            continue  # TParams blocks
        if name.startswith('mSlip') or name.startswith('mSwim') or name.startswith('mWire'):
            continue  # More TParams
        if name.startswith('mSurfing') or name.startswith('mHover') or name.startswith('mDiving'):
            continue
        if name.startswith('mGraffito') or name.startswith('mDirty') or name.startswith('mMotor'):
            continue
        if name.startswith('mController') or name.startswith('mYoshi') and name != 'mYoshi':
            continue
        if name.startswith('mBar') or name.startswith('mHanging') or name.startswith('mOption'):
            continue
        if name.startswith('mParticle') or name.startswith('mEffect') or name.startswith('mSound'):
            continue
        if name.startswith('mWaterEffect') or name.startswith('mBodyAngle') or name.startswith('mAttackParams'):
            continue
        if name.startswith('mPull') or name.startswith('mHangRoof') or name.startswith('mDmg'):
            continue
        if name.startswith('mUpper'):
            continue

        if name not in fields:
            fields[name] = {'reads': 0, 'writes': 0}

    # Pattern 2: Dotted member access (mPosition.y, mVel.x, etc.)
    for m in re.finditer(r'\b(m\w+\.[xyz])\b', src):
        name = m.group(1)
        if name not in fields:
            fields[name] = {'reads': 0, 'writes': 0}

    # Count reads vs writes for each field
    lines = src.split('\n')
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith('//') or stripped.startswith('/*'):
            continue
        if '/* 0x' in line or 'PARAM_INIT' in line:
            continue

        for name in list(fields.keys()):
            if name not in line:
                continue

            # Check next line for multi-line assignment
            next_line = lines[i + 1].lstrip() if i + 1 < len(lines) else ''

            for fm in re.finditer(re.escape(name), line):
                rest = line[fm.end():].lstrip()

                is_write = False
                if rest and rest[0] == '=' and (len(rest) < 2 or rest[1] != '='):
                    is_write = True
                elif len(rest) >= 2 and rest[0] in '+-|&*' and rest[1] == '=':
                    is_write = True
                elif not rest and next_line:
                    if next_line.startswith('=') and not next_line.startswith('=='):
                        is_write = True
                    elif len(next_line) >= 2 and next_line[0] in '+-|&*' and next_line[1] == '=':
                        is_write = True

                if is_write:
                    fields[name]['writes'] += 1
                else:
                    fields[name]['reads'] += 1

    # Filter: only fields with reads (worth making accessor for)
    return {k: v for k, v in fields.items() if v['reads'] > 0}


def infer_type(field_name, header_content):
    """Try to infer the type of a field from the header."""
    # Search for field declaration in header
    pattern = r'/\*\s*0x[0-9A-Fa-f]+\s*\*/\s+(\S+(?:\s*\*)?)\s+' + re.escape(field_name) + r'\b'
    m = re.search(pattern, header_content)
    if m:
        return m.group(1)

    # Dotted fields - infer from parent type
    if '.x' in field_name or '.y' in field_name or '.z' in field_name:
        return 'f32'

    # Common patterns
    if field_name.startswith('unk') and len(field_name) <= 6:
        return 'u32'  # safe default

    return None


def generate_candidates_auto(src_file, header_file):
    """Auto-generate candidates from field scan."""
    fields = scan_fields(src_file)

    with open(header_file) as f:
        header = f.read()

    candidates = []
    seen_names = set()

    # Check which accessors already exist in the header
    existing = set(re.findall(r'\b(get\w+)\s*\(', header))

    for field, info in sorted(fields.items(), key=lambda x: -x[1]['reads']):
        # Generate accessor name
        if '.' in field:
            parts = field.replace('.', '_').split('_')
            name = 'get' + ''.join(p.capitalize() for p in parts)
        else:
            name = 'get' + field[0].upper() + field[1:]
            if field.startswith('m'):
                name = 'get' + field[1:]
            elif field.startswith('unk'):
                name = 'get' + field.capitalize()

        if name in seen_names or name in existing:
            continue
        seen_names.add(name)

        # Infer type
        ret_type = infer_type(field, header)
        if ret_type is None:
            continue

        # Skip if only 1 read (unlikely to be useful)
        if info['reads'] < 2:
            continue

        candidates.append({
            'name': name,
            'field': field,
            'call': name + '()',
            'header': f'\t{ret_type} {name}() const {{ return {field}; }}',
            'reads': info['reads'],
            'writes': info['writes'],
        })

    return candidates


# ============================================================
# REPLACEMENT ENGINE
# ============================================================

def replace_reads(source, field, accessor):
    """Replace all READ occurrences of field with accessor."""
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
            # Likely inside the accessor definition itself
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


# ============================================================
# TESTING ENGINE
# ============================================================

def test_candidate(tu_name, src_file, header_file, candidate,
                   baseline_frames, baseline_exact):
    """Test a candidate. Returns result dict or None on build failure."""
    with open(src_file) as f:
        src_orig = f.read()
    with open(header_file) as f:
        hdr_orig = f.read()

    try:
        # Add accessor to header
        hdr = hdr_orig
        tmario_idx = hdr.find('class TMario')
        if tmario_idx == -1:
            return None
        fab_idx = hdr.find('// Fabricated', tmario_idx)
        if fab_idx == -1:
            fab_idx = hdr.find('};', tmario_idx)
        pos = hdr.rfind('\n', 0, fab_idx) + 1
        hdr = hdr[:pos] + candidate['header'] + '\n' + hdr[pos:]

        # Replace reads
        src, count = replace_reads(src_orig, candidate['field'], candidate['call'])
        if count == 0:
            return {'stacks': {}, 'new_matches': [], 'broken_matches': [], 'replacements': 0}

        with open(header_file, 'w') as f:
            f.write(hdr)
        with open(src_file, 'w') as f:
            f.write(src)

        if not build():
            return None

        # Measure stacks
        new_frames = get_stack_frames(f'build/GMSJ01/src/{tu_name}.o')
        stack_deltas = {}
        for func in baseline_frames:
            if func in new_frames and baseline_frames[func] != new_frames[func]:
                stack_deltas[func] = new_frames[func] - baseline_frames[func]

        # Byte-exact match comparison
        new_exact = get_exact_matches(tu_name)
        new_matches = []
        broken_matches = []
        for func in baseline_exact:
            if func in new_exact:
                if not baseline_exact[func] and new_exact[func]:
                    new_matches.append(func)
                elif baseline_exact[func] and not new_exact[func]:
                    broken_matches.append(func)

        return {
            'stacks': stack_deltas,
            'new_matches': new_matches,
            'broken_matches': broken_matches,
            'replacements': count,
        }

    finally:
        with open(src_file, 'w') as f:
            f.write(src_orig)
        with open(header_file, 'w') as f:
            f.write(hdr_orig)


# ============================================================
# MAIN SOLVER
# ============================================================

def run(tu_name, apply_mode=False):
    src_file = f'src/{tu_name}.cpp'
    header_file = 'include/Player/MarioMain.hpp'

    print(f"{'='*70}")
    print(f"INLINE SOLVER v4: {tu_name}")
    print(f"{'='*70}")

    if not build():
        print("ERROR: baseline build failed!")
        return

    # Baselines
    baseline_frames = get_stack_frames(f'build/GMSJ01/src/{tu_name}.o')
    orig_frames = get_stack_frames(f'build/GMSJ01/obj/{tu_name}.o')
    baseline_exact = get_exact_matches(tu_name)
    n_exact = sum(1 for v in baseline_exact.values() if v)
    print(f"Baseline: {n_exact} exact matches")

    gaps = {}
    for func in orig_frames:
        if func in baseline_frames and orig_frames[func] != baseline_frames[func]:
            gap = orig_frames[func] - baseline_frames[func]
            if gap < 0:
                gaps[func] = gap

    total_gap = sum(abs(g) for g in gaps.values())
    print(f"\nStack gaps: {len(gaps)} functions, {total_gap} bytes total")

    # Auto-generate candidates
    candidates = generate_candidates_auto(src_file, header_file)
    print(f"Auto-discovered {len(candidates)} candidate accessors from field scan")
    print(f"\nTop candidates by read count:")
    for c in candidates[:10]:
        print(f"  {c['name']:<25} {c['field']:<20} reads={c['reads']:>3} writes={c['writes']:>3}")

    print(f"\nTesting {len(candidates)} candidates...\n")

    results = {}
    start = time.time()

    for i, cand in enumerate(candidates):
        elapsed = time.time() - start
        eta = (elapsed / max(i, 1)) * (len(candidates) - i)
        print(f"  [{i+1:>2}/{len(candidates)}] {cand['name']:<25}", end='', flush=True)

        result = test_candidate(tu_name, src_file, header_file, cand,
                                baseline_frames, baseline_exact)

        if result is None:
            print(f" FAILED")
        elif result['replacements'] == 0:
            print(f" (no reads found)")
        else:
            gap_stacks = {f: d for f, d in result['stacks'].items() if f in gaps}
            new_m = result['new_matches']
            broken = result['broken_matches']

            parts = []
            if gap_stacks:
                total_d = sum(abs(d) for d in gap_stacks.values())
                parts.append(f"-{total_d}b stack in {len(gap_stacks)} funcs")
            if new_m:
                parts.append(f"NEW MATCH: {new_m}")
            if broken:
                parts.append(f"BREAKS: {broken}")

            if parts:
                print(f" -> {', '.join(parts)}")
                results[cand['name']] = {**result, 'candidate': cand}
            else:
                print(f" -> no effect ({result['replacements']} replacements)")

    build()  # restore baseline
    total_time = time.time() - start
    print(f"\nDone in {total_time:.0f}s ({total_time/len(candidates):.1f}s each)")

    # Categorize results
    # Priority 1: produces new MATCH without breaking anything
    match_makers = {}
    # Priority 2: closes stack gaps without breaking matches
    safe = {}
    # Priority 3: closes gaps but breaks some matches
    risky = {}

    for name, r in results.items():
        gap_stacks = {f: d for f, d in r['stacks'].items() if f in gaps}
        if r['new_matches'] and not r['broken_matches']:
            match_makers[name] = r
        elif gap_stacks and not r['broken_matches']:
            safe[name] = r
        elif gap_stacks and r['broken_matches']:
            risky[name] = r

    print(f"\n{'='*70}")
    print(f"RESULTS: {len(match_makers)} MATCH-MAKERS, {len(safe)} safe, {len(risky)} risky")
    print(f"{'='*70}")

    if match_makers:
        print(f"\n  *** MATCH-MAKERS (produce new 100% byte-matches!) ***")
        for name, r in match_makers.items():
            print(f"\n  {name} -> NEW MATCHES: {r['new_matches']}")
            gap_stacks = {f: d for f, d in r['stacks'].items() if f in gaps}
            for func, d in sorted(gap_stacks.items()):
                print(f"    {func}: stack {d:+d}")

    print(f"\n  --- Safe (stack improvement, no broken matches) ---")
    for name, r in sorted(safe.items(), key=lambda x: -sum(abs(d) for d in x[1]['stacks'].values())):
        gap_stacks = {f: d for f, d in r['stacks'].items() if f in gaps}
        total_d = sum(abs(d) for d in gap_stacks.values())
        nm = f" +MATCH:{r['new_matches']}" if r['new_matches'] else ""
        print(f"  {name}: -{total_d}b in {len(gap_stacks)} funcs{nm}")

    if risky:
        print(f"\n  --- Risky (breaks matches) ---")
        for name, r in risky.items():
            print(f"  {name}: breaks {r['broken_matches']}")

    # Projection using only match-makers + safe
    all_good = {**match_makers, **safe}
    combined_stack = {}
    for name, r in all_good.items():
        for func, d in r['stacks'].items():
            if func in gaps:
                combined_stack[func] = combined_stack.get(func, 0) + d

    all_new_matches = set()
    for name, r in all_good.items():
        all_new_matches.update(r['new_matches'])

    print(f"\n{'='*70}")
    print(f"PROJECTED (all {len(all_good)} safe accessors)")
    print(f"{'='*70}")

    solved = 0
    reduced = 0
    for func in sorted(gaps.keys(), key=lambda x: gaps[x]):
        g = gaps[func]
        r = combined_stack.get(func, 0)
        new_g = g - r
        if new_g == 0:
            print(f"  {func}: SOLVED!")
            solved += 1
        elif new_g > 0:
            print(f"  {func}: {g} -> OVERSHOOT +{new_g}")
        else:
            print(f"  {func}: {g} -> {new_g}")
        reduced += min(abs(r), abs(g))

    print(f"\n  Stack solved: {solved}/{len(gaps)}")
    print(f"  Stack reduced: {reduced}/{total_gap} bytes ({100*reduced//total_gap}%)")
    if all_new_matches:
        print(f"  New EXACT matches: {all_new_matches}")

    # Cache
    cache = {
        'tu': tu_name,
        'src_hash': file_hash(src_file),
        'header_hash': file_hash(header_file),
        'gaps': gaps,
        'match_makers': {k: {
            'stacks': v['stacks'],
            'new_matches': v['new_matches'],
            'header': v['candidate']['header'],
            'field': v['candidate']['field'],
            'call': v['candidate']['call'],
        } for k, v in match_makers.items()},
        'safe': {k: {
            'stacks': v['stacks'],
            'header': v['candidate']['header'],
            'field': v['candidate']['field'],
            'call': v['candidate']['call'],
        } for k, v in safe.items()},
    }
    cache_file = f'tools/inline_cache_{tu_name.replace("/", "_")}.json'
    with open(cache_file, 'w') as f:
        json.dump(cache, f, indent=2)

    # Apply mode - only apply match-makers (guaranteed to produce new matches)
    to_apply = match_makers if match_makers else (all_good if apply_mode else {})

    if apply_mode and to_apply:
        print(f"\n{'='*70}")
        print(f"APPLYING {len(to_apply)} ACCESSORS")
        print(f"{'='*70}")

        with open(header_file) as f:
            hdr = f.read()
        with open(src_file) as f:
            src = f.read()

        tmario_idx = hdr.find('class TMario')
        fab_idx = hdr.find('// Fabricated', tmario_idx)
        pos = hdr.rfind('\n', 0, fab_idx) + 1
        block = '\t// Auto-discovered inline accessors (inline_solver v4)\n'
        for name, info in to_apply.items():
            block += info['candidate']['header'] + '\n'
        hdr = hdr[:pos] + block + '\n' + hdr[pos:]

        total_rep = 0
        for name, info in to_apply.items():
            src, n = replace_reads(src, info['candidate']['field'], info['candidate']['call'])
            total_rep += n

        with open(header_file, 'w') as f:
            f.write(hdr)
        with open(src_file, 'w') as f:
            f.write(src)

        if build():
            new_matches = get_match_percentages(tu_name)
            print(f"\n  Applied {total_rep} replacements. Results:")
            for func in sorted(baseline_matches.keys()):
                old = baseline_matches.get(func, 0)
                new = new_matches.get(func, 0)
                if abs(new - old) > 0.01:
                    print(f"    {func}: {old:.1f}% -> {new:.1f}%")
        else:
            print("  BUILD FAILED!")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python tools/inline_solver.py <TU> [--apply]")
        sys.exit(1)
    apply_mode = '--apply' in sys.argv
    run(sys.argv[1], apply_mode)
