#!/usr/bin/env python3
"""
Check whether a TU's target .data section starts with the 40-byte
dummy1431/dummy1411/dummy1210 pattern (1.0f x 6 + {0,2,1,3}).

If target has the pattern AND our base does not, adding the three
file-scope static dummies usually closes the .data offset gap.

Usage: python3 tools/agent/check_dummy1431.py <unit>
"""
import json
import os
import subprocess
import sys
import base64

script_dir = os.path.dirname(os.path.realpath(__file__))
root_dir = os.path.abspath(os.path.join(script_dir, "..", ".."))
OBJDIFF = os.path.join(root_dir, "build", "tools", "objdiff-cli")

EXPECTED_HEX = "3f8000003f8000003f8000003f8000003f8000003f80000000000000000000020000000100000003"


def check(unit):
    out = subprocess.check_output(
        [OBJDIFF, "diff", "-c", "functionRelocDiffs=data_value",
         "-u", unit, "-o", "-", "--format", "json"],
        cwd=root_dir,
    )
    d = json.loads(out)
    L_data = next((s for s in d["left"]["sections"] if s["name"] == ".data"), None)
    R_data = next((s for s in d["right"]["sections"] if s["name"] == ".data"), None)
    if L_data is None:
        return ("NO_TARGET_DATA", 0, 0)
    L_size = int(L_data["size"])
    R_size = int(R_data["size"]) if R_data else 0
    # find first delete block at offset 0 in .data_diff
    diffs = L_data.get("data_diff", [])
    if not diffs:
        return ("NO_DIFF", L_size, R_size)
    first = diffs[0]
    if first.get("kind") == "DIFF_DELETE" and "data" in first:
        bs = base64.b64decode(first["data"])
        if bs[:40].hex() == EXPECTED_HEX:
            return ("HAS_DUMMY1431", L_size, R_size)
        else:
            return (f"OTHER_DELETE:{bs[:40].hex()}", L_size, R_size)
    return ("FIRST_NOT_DELETE", L_size, R_size)


def main():
    units = sys.argv[1:]
    if not units:
        print(__doc__, file=sys.stderr)
        sys.exit(1)
    for u in units:
        try:
            status, L, R = check(u)
            print(f"{u}: status={status} L.data={L} R.data={R}")
        except Exception as e:
            print(f"{u}: ERROR {e}")


if __name__ == "__main__":
    main()
