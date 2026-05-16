"""Swap in upstream version of a file, build, capture match%, restore ours."""
import json
import subprocess
import sys
import os

CONTESTED = [
    'src/Player/MarioJump.cpp',
    'src/Player/MarioAction.cpp',
    'src/Player/MarioAutodemo.cpp',
    'src/Player/MarioCap.cpp',
    'src/Player/MarioDraw.cpp',
    'src/Player/MarioAccess.cpp',
    'src/Player/MarioCollision.cpp',
    'src/Player/Yoshi.cpp',
    'src/System/MarDirectorDirect.cpp',
    'src/System/EventWatcher.cpp',
]

def read_match(path):
    with open('build/GMSJ01/report.json') as f:
        r = json.load(f)
    key = 'mario/' + path[4:-4]
    for u in r.get('units', []):
        if u['name'] == key:
            m = u['measures']
            mc = int(m.get('matched_code', '0'))
            tc = int(m.get('total_code', '0'))
            pct = (100.0 * mc / tc) if tc else 0.0
            return pct, mc, tc, m.get('matched_functions', 0), m.get('total_functions', 0)
    return None

def main():
    baseline = {}
    for f in CONTESTED:
        baseline[f] = read_match(f)

    print(f"{'file':50s} {'ours%':>8s} {'theirs%':>8s} {'winner':>8s}")
    print('-' * 80)

    results = []
    for f in CONTESTED:
        # Save our version
        with open(f, 'rb') as fp:
            ours_content = fp.read()
        # Get upstream version
        try:
            upstream = subprocess.check_output(['git', 'show', f'origin/main:{f}'])
        except subprocess.CalledProcessError:
            print(f'{f}: not in origin/main')
            continue
        # Write upstream
        with open(f, 'wb') as fp:
            fp.write(upstream)
        # Build
        r = subprocess.run(['python', '-m', 'ninja'], capture_output=True, text=True)
        if r.returncode != 0:
            print(f'{f}: BUILD FAILED with upstream version')
            # Restore
            with open(f, 'wb') as fp:
                fp.write(ours_content)
            continue
        theirs_pct = read_match(f)
        # Restore ours
        with open(f, 'wb') as fp:
            fp.write(ours_content)
        # Print
        ours_pct = baseline[f][0]
        winner = 'OURS' if ours_pct >= theirs_pct[0] else 'THEIRS'
        print(f'{f:50s} {ours_pct:7.2f}% {theirs_pct[0]:7.2f}% {winner:>8s}  fns ours={baseline[f][3]}/{baseline[f][4]} theirs={theirs_pct[3]}/{theirs_pct[4]}')
        results.append((f, ours_pct, theirs_pct[0], winner))

    # Final rebuild to restore baseline
    subprocess.run(['python', '-m', 'ninja'], capture_output=True)

    print('\n=== Summary ===')
    for f, op, tp, w in results:
        print(f'{w:>8s}  {f}  ({op:.2f}% vs {tp:.2f}%)')

if __name__ == '__main__':
    main()
