#!/usr/bin/env python3
"""Dump MWCC compiler internals for a function, to crack register-allocation and
stack-frame matching problems with evidence instead of guesswork.

Wraps cadmic's mwcc-debugger (which runs the real MWCC executable under
retrowin32 + gdb) and points it at the right compiler. Given a source file and a
mangled symbol, it derives the exact mwcc command line from ninja, swaps in a
*debugger-supported* compiler version, runs the debugger, and prints the two
highest-value dumps: the stack-allocation map (variables.txt) and the
register-allocation assignments.

The debugger only has breakpoint offsets for GC/1.1 and GC/2.6. GC/1.1 is
"good enough" for GC/1.0-1.2.5 (our game code) and GC/2.6 for GC/1.3.2-2.7
(SDK) — the regalloc algorithm and stack-allocation order are shared across each
range, so the dumps are representative even though the binary isn't byte-identical
to 1.2.5. See the generated MWCC Tooling brain page for current usage notes.

Usage:
    python tools/agent/mwcc_dump.py <source.cpp> <mangled_symbol> [--out DIR] [--full]

    python tools/agent/mwcc_dump.py src/Player/MarioMove.cpp move__9TMarioFv
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(os.environ.get("GRAFFITO_REPO", Path.cwd()))
MWCC_DEBUGGER = Path(os.environ.get("MWCC_DEBUGGER", "/opt/mwcc-debugger/mwcc_debugger.py"))
RETROWIN32 = Path(os.environ.get("RETROWIN32", "/opt/retrowin32/retrowin32"))

# Which debugger-supported compiler approximates each real version.
#   GC/1.0 - 1.2.5  -> GC/1.1   (game code)
#   GC/1.3.2 - 2.7  -> GC/2.6   (Dolphin SDK)
_V11 = {"1.0", "1.1", "1.1p1", "1.2.5", "1.2.5n"}
_V26 = {"1.3", "1.3.2", "1.3.2r", "2.0", "2.0p1", "2.5", "2.6", "2.7"}


def die(msg: str) -> "None":
    print("mwcc_dump: " + msg, file=sys.stderr)
    sys.exit(1)


def ninja_command_for(source: str) -> str:
    """Return the full ninja build command for the object built from `source`."""
    try:
        out = subprocess.check_output(
            ["ninja", "-t", "commands"], cwd=str(REPO), text=True,
            stderr=subprocess.DEVNULL,
        )
    except Exception as e:  # noqa: BLE001
        die("could not run `ninja -t commands` (build configured?): %s" % e)
    base = os.path.basename(source)
    cands = [ln for ln in out.splitlines() if base in ln and "mwcceppc.exe" in ln]
    if not cands:
        die("no mwcceppc.exe command found for %s (is it in configure.py?)" % source)
    return cands[0]


def detect_compiler_version(cmd: str) -> str:
    m = re.search(r"compilers/GC/([0-9][^/\s]*)/mwcceppc\.exe", cmd)
    return m.group(1) if m else ""


def supported_compiler(real_version: str) -> Path:
    if real_version in _V26:
        gc = "2.6"
    else:
        gc = "1.1"  # default; covers 1.0-1.2.5
    p = REPO / "build" / "compilers" / "GC" / gc / "mwcceppc.exe"
    if not p.exists():
        die("debugger-supported compiler not present: %s" % p)
    return p


def build_mwcc_cmdline(ninja_cmd: str, compiler: Path) -> str:
    """Take the mwcceppc.exe invocation out of the ninja command (dropping the
    wibo/sjiswrap prefix and any '&& transform_dep.py' suffix) and swap the
    compiler path for a debugger-supported one."""
    toks = ninja_cmd.split()
    ci = next((i for i, t in enumerate(toks) if t.endswith("mwcceppc.exe")), None)
    if ci is None:
        die("could not locate mwcceppc.exe token in ninja command")
    args = []
    for t in toks[ci + 1:]:
        if t in ("&&", "||", "|", ";", ">", ">>"):
            break
        args.append(t)
    return " ".join([str(compiler)] + args)


def summarize(outdir: Path) -> None:
    def show(name: str, title: str) -> None:
        p = outdir / name
        if p.exists():
            print("\n===== %s (%s) =====" % (title, name))
            print(p.read_text(encoding="utf-8", errors="replace").rstrip())

    show("variables.txt", "STACK ALLOCATION")
    show("regalloc-gpr-pass-1-assigned.txt", "GPR ASSIGNMENTS")
    show("regalloc-fpr-pass-1-assigned.txt", "FPR ASSIGNMENTS")
    print("\nFull dumps in %s" % outdir)
    print("  backend-05-before-regalloc.txt  — cross-ref virtual regs (r35+) to source")
    print("  backend-0X-*.txt                — PCode after each backend pass")
    print("  regalloc-*-all.txt              — includes coalesced vars (extra neighbors)")


def main() -> None:
    ap = argparse.ArgumentParser(description="Dump MWCC internals for a function.")
    ap.add_argument("source", help="source file, e.g. src/Player/MarioMove.cpp")
    ap.add_argument("symbol", help="mangled symbol, e.g. move__9TMarioFv")
    ap.add_argument("--out", default=None, help="output dir (default: a temp dir)")
    ap.add_argument("--keep-version", action="store_true",
                    help="use the TU's own compiler instead of mapping to GC/1.1/2.6 "
                         "(only works if it's already 1.1 or 2.6)")
    args = ap.parse_args()

    if not MWCC_DEBUGGER.exists():
        die("mwcc_debugger.py not found at %s (set MWCC_DEBUGGER)" % MWCC_DEBUGGER)
    if not RETROWIN32.exists():
        die("retrowin32 not found at %s (set RETROWIN32)" % RETROWIN32)

    ninja_cmd = ninja_command_for(args.source)
    real_v = detect_compiler_version(ninja_cmd)
    if args.keep_version:
        compiler = REPO / "build" / "compilers" / "GC" / real_v / "mwcceppc.exe"
    else:
        compiler = supported_compiler(real_v)
    approx = "" if str(compiler).endswith("GC/%s/mwcceppc.exe" % real_v) else \
        "  (approximating real GC/%s)" % real_v
    cmdline = build_mwcc_cmdline(ninja_cmd, compiler)

    outdir = Path(args.out) if args.out else Path(tempfile.mkdtemp(prefix="mwccdbg-"))
    outdir.mkdir(parents=True, exist_ok=True)

    print("source   : %s" % args.source)
    print("symbol   : %s" % args.symbol)
    print("compiler : GC/%s%s" % (
        re.search(r"GC/([^/]+)/", str(compiler)).group(1), approx))
    print("cmdline  : %s" % cmdline)
    print("running mwcc-debugger (emulated, ~1 min)...")

    rc = subprocess.call(
        [sys.executable, str(MWCC_DEBUGGER), "-e", str(RETROWIN32),
         "-a", cmdline, args.symbol, str(outdir)],
        cwd=str(REPO),
    )
    if rc != 0:
        die("mwcc_debugger.py exited %d (wrong symbol name? SJIS source? "
            "try a different function)" % rc)
    if not (outdir / "variables.txt").exists() and not list(outdir.glob("regalloc-*")):
        die("no dumps produced — symbol %r may not match the compiled name" % args.symbol)
    summarize(outdir)


if __name__ == "__main__":
    main()
