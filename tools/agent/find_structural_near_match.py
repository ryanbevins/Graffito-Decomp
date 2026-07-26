#!/usr/bin/env python3
# usage: python tools/agent/find_structural_near_match.py --min-pct 95 --prefix mario/MoveBG
"""Rank near-matching functions that still have structural diff categories."""

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REPORT = ROOT / "build" / "GMSJ01" / "report.json"
CHECKER = ROOT / "tools" / "check-diff-noise.py"
CATEGORY_RE = re.compile(r"^\s+(structural:[\w+-]+)\s+(\d+)\s*$", re.MULTILINE)


def main():
	parser = argparse.ArgumentParser(
	    description="Find report functions whose live diff still has structural rows."
	)
	parser.add_argument("--min-pct", type=float, default=95.0)
	parser.add_argument("--max-pct", type=float, default=99.99)
	parser.add_argument("--min-size", type=int, default=64)
	parser.add_argument("--max-size", type=int, default=1200)
	parser.add_argument("--prefix", default="mario/")
	parser.add_argument("--scan", type=int, default=100,
	                    help="maximum ranked functions to check")
	parser.add_argument("--limit", type=int, default=30,
	                    help="maximum structural candidates to print")
	args = parser.parse_args()

	report = json.loads(REPORT.read_text())
	candidates = []
	for unit in report["units"]:
		unit_name = unit["name"]
		if not unit_name.startswith(args.prefix):
			continue
		for function in unit.get("functions", []):
			pct = float(function.get("fuzzy_match_percent") or 0.0)
			size = int(function["size"])
			if not (args.min_pct <= pct <= args.max_pct):
				continue
			if not (args.min_size <= size <= args.max_size):
				continue
			name = function.get("metadata", {}).get(
			    "demangled_name", function["name"]
			)
			candidates.append((-pct, -size, unit_name, name))

	candidates.sort()
	found = 0
	for neg_pct, neg_size, unit_name, name in candidates[: args.scan]:
		proc = subprocess.run(
		    [
		        sys.executable,
		        str(CHECKER),
		        "-u",
		        unit_name,
		        "-d",
		        name,
		    ],
		    cwd=str(ROOT),
		    text=True,
		    stdout=subprocess.PIPE,
		    stderr=subprocess.STDOUT,
		)
		categories = CATEGORY_RE.findall(proc.stdout)
		if not categories:
			continue
		category_text = ",".join(
		    "%s=%s" % (category, count) for category, count in categories
		)
		print(
		    "%6.2f%% %5dB %-42s %-36s %s"
		    % (-neg_pct, -neg_size, unit_name, category_text, name)
		)
		found += 1
		if found >= args.limit:
			break

	if found == 0:
		print("No structural near-match candidates found in the scanned range.")


if __name__ == "__main__":
	main()
