#!/usr/bin/env python3
# usage: python3 tools/agent/audit_candidates.py [--min-pct 85] [--open-only] [--check-missing]
# Lists NonMatching objects ranked by fuzzy match percent, with any recorded
# audit verdict from the configured state root marked beside each row.
import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_STATE = ROOT.parent / "state"
REPORT = ROOT / "build" / "GMSJ01" / "report.json"
CONFIGURE = ROOT / "configure.py"

BLOCKING_VERDICTS = (
    "not_equivalent",
    "needs_impl",
    "needs_investigation",
    "blocked",
)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--min-pct", type=float, default=85.0)
    parser.add_argument("--limit", type=int, default=80)
    parser.add_argument("--open-only", action="store_true")
    parser.add_argument("--check-missing", action="store_true",
                        help="run decomp-diff missing-symbol checks")
    parser.add_argument("--no-missing-only", action="store_true",
                        help="only print rows with no missing target symbols")
    parser.add_argument("--state-root", default=os.environ.get(
        "GRAFFITO_STATE", str(DEFAULT_STATE)))
    args = parser.parse_args()

    nonmatching = read_nonmatching_units(CONFIGURE)
    report = json.loads(REPORT.read_text())
    state_root = Path(args.state_root)

    rows = []
    for unit in report["units"]:
        name = unit["name"]
        if not name.startswith("mario/"):
            continue
        rel = name[len("mario/"):]
        if rel not in nonmatching:
            continue

        measures = unit.get("measures", {})
        pct = float(measures.get("fuzzy_match_percent") or 0)
        if pct < args.min_pct:
            continue

        verdict = read_verdict(state_root / "audit" / (rel + ".md"))
        blocked = is_blocked(verdict)
        if args.open_only and blocked:
            continue

        matched = to_int(measures.get("matched_code"))
        total = to_int(measures.get("total_code"))
        rows.append((blocked, -pct, rel, pct, matched, total, verdict))

    rows.sort()
    if not rows:
        print("No candidates matched the filters.")
        return

    if args.no_missing_only:
        args.check_missing = True

    checked_rows = []
    rows_to_check = rows if args.no_missing_only else rows[:args.limit]
    for row in rows_to_check:
        missing = ""
        if args.check_missing:
            missing = check_missing_symbols(row[2])
        checked_rows.append(row + (missing,))

    if args.no_missing_only:
        checked_rows = [
            row for row in checked_rows if row[-1] == "no-missing"
        ][:args.limit]
    else:
        checked_rows = checked_rows[:args.limit]

    if not checked_rows:
        print("No candidates matched the filters.")
        return

    if args.check_missing:
        print("FUZZY    CODE        MISSING      STATUS                         UNIT")
        print("-------------------------------------------------------------------------------------")
    else:
        print("FUZZY    CODE        STATUS                         UNIT")
        print("------------------------------------------------------------------------")

    for blocked, _sort_pct, rel, pct, matched, total, verdict, missing in checked_rows:
        status = verdict
        if blocked:
            status = "blocked:" + verdict
        if args.check_missing:
            print(
                f"{pct:6.3f}% {matched:6}/{total:<6} {missing:<12} "
                f"{status:<30} {rel}"
            )
        else:
            print(f"{pct:6.3f}% {matched:6}/{total:<6} {status:<30} {rel}")


def read_nonmatching_units(configure_path):
    text = configure_path.read_text()
    result = set()
    pattern = re.compile(r'Object\(NonMatching,\s*"([^"]+\.(?:c|cpp|s))"')
    for match in pattern.finditer(text):
        result.add(str(Path(match.group(1)).with_suffix("")))
    return result


def read_verdict(path):
    if not path.exists():
        return "open"
    text = path.read_text(errors="replace")
    match = re.search(r"(?im)^\s*(?:#+\s*)?verdict\s*:\s*([^\n]+)", text)
    if not match:
        return "note"
    verdict = match.group(1).strip().replace("`", "")
    return verdict.replace(" / ", "/")


def is_blocked(verdict):
    lowered = verdict.lower()
    return any(token in lowered for token in BLOCKING_VERDICTS)


def to_int(value):
    if value is None:
        return 0
    if isinstance(value, int):
        return value
    return int(str(value).replace(",", ""))


def check_missing_symbols(rel):
    unit = "mario/" + rel
    cmd = [
        sys.executable,
        str(ROOT / "tools" / "decomp-diff.py"),
        "-u",
        unit,
        "-s",
        "missing",
    ]
    proc = subprocess.run(
        cmd,
        cwd=str(ROOT),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if proc.returncode != 0:
        return "error"
    if "No symbols match the given filters." in proc.stdout:
        return "no-missing"

    count = 0
    for line in proc.stdout.splitlines():
        if line.startswith("missing"):
            count += 1
    return "missing:%d" % count


if __name__ == "__main__":
    main()
