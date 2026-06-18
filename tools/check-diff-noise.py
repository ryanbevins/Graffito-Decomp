#!/usr/bin/env python3

"""
Report objdiff instruction differences that are not obvious match noise.

This is intentionally conservative. By default it ignores:
  - register-only operand differences
  - stack-offset-only differences relative to r1
  - branch-destination-only differences
  - swapped source operands for commutative ops such as fadds

Anything else is reported as structural.
"""

import argparse
import json
import os
import re
import subprocess
import sys
from collections import Counter


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))
OBJDIFF_CLI = os.environ.get(
    "OBJDIFF_CLI", os.path.join(ROOT_DIR, "build", "tools", "objdiff-cli")
)

REGISTER_RE = re.compile(r"^(?:r|f|fr|cr)\d+$")
COMMUTATIVE_OPS = {
    "add",
    "add.",
    "fadd",
    "fadds",
    "fmul",
    "fmuls",
    "or",
    "or.",
    "xor",
    "xor.",
    "and",
    "and.",
}


def run_objdiff(unit):
    result = subprocess.run(
        [
            OBJDIFF_CLI,
            "diff",
            "-c",
            "functionRelocDiffs=data_value",
            "-u",
            unit,
            "-o",
            "-",
            "--format",
            "json",
        ],
        cwd=ROOT_DIR,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        print(result.stderr, file=sys.stderr)
        sys.exit(result.returncode)
    return json.loads(result.stdout)


def fuzzy_match(pattern, name):
    return pattern.lower() in name.lower()


def symbol_name(symbol):
    return symbol.get("demangled_name", symbol.get("name", "?"))


def find_left_symbol(data, pattern):
    matches = []
    for symbol in data.get("left", {}).get("symbols", []):
        if "instructions" not in symbol:
            continue
        if fuzzy_match(pattern, symbol_name(symbol)):
            matches.append(symbol)

    if not matches:
        raise SystemExit("no function matches pattern: %s" % pattern)
    if len(matches) > 1:
        print("multiple functions match; using first:", file=sys.stderr)
        for symbol in matches:
            print("  " + symbol_name(symbol), file=sys.stderr)
    return matches[0]


def paired_right_symbol(data, left_symbol):
    target_symbol = left_symbol.get("target_symbol")
    if target_symbol is None:
        raise SystemExit("left function has no paired right function")

    index = target_symbol - 1
    right_symbols = data.get("right", {}).get("symbols", [])
    if index < 0 or index >= len(right_symbols):
        raise SystemExit("paired right function index is out of range")

    right_symbol = right_symbols[index]
    if "instructions" not in right_symbol:
        raise SystemExit("paired right symbol has no instructions")
    return right_symbol


def instruction_text(entry):
    inst = entry.get("instruction")
    if inst is None:
        return "<missing>"
    return inst.get("formatted", "<unformatted>")


def opcode(entry):
    inst = entry.get("instruction")
    if inst is None:
        return None
    for part in inst.get("parts", []):
        if "opcode" in part:
            return part["opcode"].get("mnemonic")
    return None


def arg_value(arg):
    if "opaque" in arg:
        return ("opaque", str(arg["opaque"]))
    if "signed" in arg:
        return ("number", int(arg["signed"]))
    if "unsigned" in arg:
        return ("number", int(arg["unsigned"]))
    if "branch_dest" in arg:
        return ("branch", int(arg["branch_dest"]))
    if "reloc" in arg:
        return ("reloc", json.dumps(arg["reloc"], sort_keys=True))
    return ("unknown", json.dumps(arg, sort_keys=True))


def args(entry):
    inst = entry.get("instruction")
    if inst is None:
        return []
    values = []
    for part in inst.get("parts", []):
        if "arg" in part:
            values.append(arg_value(part["arg"]))
    return values


def is_register(value):
    return value[0] == "opaque" and REGISTER_RE.match(value[1]) is not None


def is_r1(value):
    return value == ("opaque", "r1")


def is_stack_offset_difference(index, left_args, right_args):
    left = left_args[index]
    right = right_args[index]
    if left[0] != "number" or right[0] != "number":
        return False

    # Memory operands appear as offset, base-register in objdiff parts.
    if index + 1 < len(left_args) and is_r1(left_args[index + 1]):
        return index + 1 < len(right_args) and is_r1(right_args[index + 1])

    # addi/addis stack-address materialization appears as rD, r1, imm.
    if index > 0 and is_r1(left_args[index - 1]):
        return is_r1(right_args[index - 1])

    return False


def is_commutative_source_swap(left_op, left_args, right_args):
    if left_op not in COMMUTATIVE_OPS:
        return False
    if len(left_args) != 3 or len(right_args) != 3:
        return False
    return (
        left_args[0] == right_args[0]
        and left_args[1] == right_args[2]
        and left_args[2] == right_args[1]
    )


def classify_pair(left_entry, right_entry):
    left_inst = left_entry.get("instruction")
    right_inst = right_entry.get("instruction")
    if left_inst is None or right_inst is None:
        return "structural:insert-delete"

    left_op = opcode(left_entry)
    right_op = opcode(right_entry)
    if left_op != right_op:
        return "structural:opcode"

    if left_entry.get("diff_kind") is None and right_entry.get("diff_kind") is None:
        return "exact"

    left_args = args(left_entry)
    right_args = args(right_entry)
    if len(left_args) != len(right_args):
        return "structural:arg-count"

    if is_commutative_source_swap(left_op, left_args, right_args):
        return "ignored:commutative-source-swap"

    categories = set()
    for index, (left, right) in enumerate(zip(left_args, right_args)):
        if left == right:
            continue
        if is_register(left) and is_register(right):
            categories.add("register")
            continue
        if is_stack_offset_difference(index, left_args, right_args):
            categories.add("stack")
            continue
        if left[0] == "branch" and right[0] == "branch":
            categories.add("branch")
            continue
        return "structural:operand"

    if not categories:
        return "exact"
    return "ignored:" + "+".join(sorted(categories))


def compare(left_symbol, right_symbol):
    left_insts = left_symbol.get("instructions", [])
    right_insts = right_symbol.get("instructions", [])
    limit = max(len(left_insts), len(right_insts))
    rows = []

    for index in range(limit):
        left_entry = left_insts[index] if index < len(left_insts) else {}
        right_entry = right_insts[index] if index < len(right_insts) else {}
        category = classify_pair(left_entry, right_entry)
        if category == "exact":
            continue

        left_inst = left_entry.get("instruction") or right_entry.get("instruction") or {}
        address = int(left_inst.get("address", 0))
        rows.append(
            {
                "index": index,
                "address": address,
                "category": category,
                "left": instruction_text(left_entry),
                "right": instruction_text(right_entry),
            }
        )
    return compact_instruction_moves(rows)


def compact_instruction_moves(rows):
    result = []
    used = set()

    for index, row in enumerate(rows):
        if index in used or row["category"] != "structural:insert-delete":
            if index not in used:
                result.append(row)
            continue

        if row["left"] == "<missing>":
            text = row["right"]
            opposite_side = "right"
        elif row["right"] == "<missing>":
            text = row["left"]
            opposite_side = "left"
        else:
            result.append(row)
            continue

        pair_index = None
        for candidate_index in range(index + 1, len(rows)):
            candidate = rows[candidate_index]
            if (
                candidate_index not in used
                and candidate["category"] == "structural:insert-delete"
                and candidate["left"] != row["left"]
                and candidate["right"] != row["right"]
                and (candidate["left"] == text or candidate["right"] == text)
            ):
                pair_index = candidate_index
                break

        if pair_index is None:
            result.append(row)
            continue

        pair = rows[pair_index]
        used.add(index)
        used.add(pair_index)
        result.append(
            {
                "index": min(row["index"], pair["index"]),
                "address": row["address"] or pair["address"],
                "category": "structural:instruction-order",
                "left": "%s at #%04d" % (text, pair["index"] if opposite_side == "right" else row["index"]),
                "right": "%s at #%04d" % (text, row["index"] if opposite_side == "right" else pair["index"]),
            }
        )

    return result


def main():
    parser = argparse.ArgumentParser(
        description="Check objdiff output for structural differences after ignoring register/stack noise."
    )
    parser.add_argument("-u", "--unit", required=True, help="objdiff unit name")
    parser.add_argument("-d", "--diff", required=True, help="function name substring")
    parser.add_argument(
        "--show-ignored",
        action="store_true",
        help="also print ignored register/stack/branch differences",
    )
    args_ns = parser.parse_args()

    data = run_objdiff(args_ns.unit)
    left_symbol = find_left_symbol(data, args_ns.diff)
    right_symbol = paired_right_symbol(data, left_symbol)
    rows = compare(left_symbol, right_symbol)

    structural = [row for row in rows if row["category"].startswith("structural:")]
    ignored = [row for row in rows if row["category"].startswith("ignored:")]

    print(symbol_name(left_symbol))
    print(
        "match: %.4g%%, size: %sB, instructions: %d"
        % (
            left_symbol.get("match_percent", 0.0),
            left_symbol.get("size", "?"),
            len(left_symbol.get("instructions", [])),
        )
    )

    counts = Counter(row["category"] for row in rows)
    if counts:
        print("difference categories:")
        for category, count in sorted(counts.items()):
            print("  %-36s %d" % (category, count))
    else:
        print("no instruction differences")

    printable = structural
    if args_ns.show_ignored:
        printable = rows

    if printable:
        print()
        print("reported differences:")
        for row in printable:
            print(
                "  #%04d 0x%04x %-32s L: %s"
                % (row["index"], row["address"], row["category"], row["left"])
            )
            print(" " * 47 + "R: " + row["right"])

    if structural:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
