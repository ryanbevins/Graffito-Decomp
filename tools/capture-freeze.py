"""Read-only Windows Dolphin MEM1/thread capture; no emulator plugin required."""
import argparse
import bisect
import ctypes as c
from ctypes import wintypes as w
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time
import os


class Region(c.Structure):
    _fields_ = [("base", c.c_void_p), ("allocation", c.c_void_p),
                ("allocation_protect", w.DWORD), ("partition", w.WORD),
                ("size", c.c_size_t), ("state", w.DWORD),
                ("protect", w.DWORD), ("type", w.DWORD)]


class Reader:
    def __init__(self, pid):
        self.api = c.WinDLL("kernel32", use_last_error=True)
        self.api.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
        self.api.OpenProcess.restype = w.HANDLE
        self.api.ReadProcessMemory.argtypes = [w.HANDLE, c.c_void_p, c.c_void_p, c.c_size_t, c.POINTER(c.c_size_t)]
        self.api.VirtualQueryEx.argtypes = [w.HANDLE, c.c_void_p, c.POINTER(Region), c.c_size_t]
        self.api.VirtualQueryEx.restype = c.c_size_t
        self.api.CloseHandle.argtypes = [w.HANDLE]
        self.handle = self.api.OpenProcess(0x410, False, pid)
        if not self.handle:
            raise c.WinError(c.get_last_error())

    def read(self, address, size):
        buf = c.create_string_buffer(size)
        count = c.c_size_t()
        if not self.api.ReadProcessMemory(self.handle, address, buf, size, c.byref(count)) or count.value != size:
            raise OSError("Cannot read process address %#x" % address)
        return buf.raw

    def find_ram(self):
        address = 0
        while address < 0x7fffffffffff:
            region = Region()
            if not self.api.VirtualQueryEx(self.handle, address, c.byref(region), c.sizeof(region)):
                break
            base = region.base or 0
            if region.state == 0x1000 and region.size >= 0x1800000 and not region.protect & 0x101:
                try:
                    header = self.read(base, 0x40)
                    if header[:6] == b"GMSJ01":
                        return base
                except OSError:
                    pass
            address = base + region.size
        raise RuntimeError("No running GMSJ01 MEM1 found. Start Sunshine and retry.")

    def close(self):
        self.api.CloseHandle(self.handle)


def symbols(path):
    result = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        fields = line.split(maxsplit=4)
        if len(fields) == 5:
            try:
                result.append((int(fields[0], 16), int(fields[1], 16), fields[4]))
            except ValueError:
                pass
    return sorted(result)


def decode(ram, syms):
    starts = [s[0] for s in syms]
    def label(address):
        i = bisect.bisect_right(starts, address) - 1
        if i >= 0:
            start, size, name = syms[i]
            if start <= address < start + size:
                return "%s+0x%x" % (name, address - start)
        return "<unknown>"
    def u32(address):
        offset = address - 0x80000000
        if not 0 <= offset <= len(ram) - 4:
            raise ValueError("Invalid MEM1 address %#x" % address)
        return struct.unpack_from(">I", ram, offset)[0]
    def frame(address):
        return {"address": "%08x" % address, "symbol": label(address)}
    current = u32(0x800000e4)
    thread = u32(0x800000dc)
    threads, seen = [], set()
    while thread and thread not in seen and len(threads) < 64:
        seen.add(thread)
        try:
            state = u32(thread + 0x2c8) >> 16
            sp = u32(thread + 4)
            item = {"thread": "%08x" % thread, "current": thread == current,
                    "state": state, "suspend": u32(thread + 0x2cc),
                    "wait_queue": "%08x" % u32(thread + 0x2dc),
                    "registers": ["%08x" % u32(thread + i * 4) for i in range(32)],
                    "saved_pc": frame(u32(thread + 0x198)), "saved_lr": frame(u32(thread + 0x84)),
                    "stack_pointer": "%08x" % sp, "frames": []}
            stack_seen = set()
            low, high = u32(thread + 0x308), u32(thread + 0x304)
            while low <= sp < high and sp not in stack_seen and len(item["frames"]) < 80:
                stack_seen.add(sp)
                item["frames"].append(frame(u32(sp + 4)))
                sp = u32(sp)
            threads.append(item)
            thread = u32(thread + 0x2fc)
        except ValueError as error:
            threads.append({"error": str(error)})
            break
    result = {"current_thread": "%08x" % current, "threads": threads}
    by_name = {name: address for address, size, name in syms}
    try:
        graphics = {}
        if "DrawDone" in by_name:
            graphics["draw_done"] = ram[by_name["DrawDone"] - 0x80000000]
        for name in ("CPUFifo", "GPFifo"):
            if name in by_name:
                pointer = u32(by_name[name])
                if pointer:
                    graphics[name] = {key: "%08x" % u32(pointer + index * 4)
                                      for index, key in enumerate(("base", "top", "size", "high_watermark", "low_watermark", "saved_read", "saved_write", "saved_count"))}
        if "__GXCurrentBP" in by_name:
            graphics["breakpoint"] = hex(u32(by_name["__GXCurrentBP"]))
        if "gx" in by_name:
            gx = u32(by_name["gx"])
            if gx:
                graphics["saved_cp_enable"] = hex(u32(gx + 8))
                graphics["saved_cp_status"] = hex(u32(gx + 12))
        for cls in ("20TPollutionCounterObj", "22TPollutionCounterLayer"):
            name = "__vt__" + cls
            if name in by_name:
                table = by_name[name]
                actual = [u32(table + 12), u32(table + 16)]
                expected = [by_name.get("getCounterNo__" + cls + "CFUl"),
                            by_name.get("getTokenNo__" + cls + "CFi")]
                graphics[name] = {"conversion_slots": [frame(a) for a in actual],
                                  "retail_order_correct": actual == expected}
        result["graphics"] = graphics
        if "gpApplication" in by_name:
            app = by_name["gpApplication"] - 0x80000000
            result["application"] = {"state": ram[app + 8],
                                     "area": ram[app + 0xe], "episode": ram[app + 0xf]}
        if "gpMarDirector" in by_name:
            director = u32(by_name["gpMarDirector"])
            if 0x80000000 <= director < 0x81800000 - 0x264:
                offset = director - 0x80000000
                result["director"] = {"address": hex(director), "map": ram[offset + 0x7c],
                                      "episode": ram[offset + 0x7d], "state": ram[offset + 0x64],
                                      "setup_objects_complete": ram[offset + 0x260]}
    except (ValueError, IndexError) as error:
        result["global_decode_error"] = str(error)
    return result


def summary(report):
    lines = ["Sunshine freeze capture", "DOL SHA1: " + report["dol_sha1"], report["note"]]
    for index, sample in enumerate(report["samples"]):
        lines.append("\nSample %d: %s" % (index + 1, sample.get("application", {})))
        lines.append("Director: %s" % sample.get("director", {}))
        lines.append("DrawDone: %s" % sample.get("graphics", {}).get("draw_done", "unknown"))
        for name, value in sample.get("graphics", {}).items():
            if isinstance(value, dict) and value.get("retail_order_correct") is False:
                lines.append("ERROR: %s has incorrect pollution token conversion slots; GPU synchronization can deadlock." % name)
        mismatches = sum(section["different_bytes"] for section in sample["text_sections"])
        lines.append("Executable byte differences from supplied DOL: %d" % mismatches)
        if mismatches:
            lines.append("WARNING: code differs; check DOL/map identity or runtime patches before trusting symbols.")
        for thread in sample["threads"]:
            if "error" in thread:
                lines.append(thread["error"])
                continue
            lines.append("Thread %s state=%d saved PC=%s" % (thread["thread"], thread["state"], thread["saved_pc"]["symbol"]))
            lines.extend("  " + frame["address"] + " " + frame["symbol"] for frame in thread["frames"] if frame["symbol"] != "<unknown>")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pid", type=int)
    parser.add_argument("--dol", type=Path, default=Path.home() / "Downloads/SunshineJPExtract/sys/main.dol")
    parser.add_argument("--map", type=Path)
    parser.add_argument("--out", type=Path, default=Path("build/freezes") / time.strftime("%Y%m%d-%H%M%S"))
    parser.add_argument("--samples", type=int, default=3)
    parser.add_argument("--interval", type=float, default=1.0)
    args = parser.parse_args()
    if args.samples < 1 or args.interval < 0:
        parser.error("samples must be positive and interval nonnegative")
    if args.pid is None:
        rows = subprocess.check_output(["tasklist", "/fi", "IMAGENAME eq Dolphin.exe", "/fo", "csv", "/nh"], text=True)
        import csv
        pids = [int(row[1]) for row in csv.reader(rows.splitlines()) if row and row[0].lower() == "dolphin.exe"]
        if len(pids) != 1:
            parser.error("Specify --pid when zero or multiple Dolphin processes exist")
        args.pid = pids[0]
    map_path = args.map or args.dol.with_suffix(".map")
    syms = symbols(map_path)
    dol = args.dol.read_bytes()
    report = {"pid": args.pid, "dol": str(args.dol.resolve()),
              "dol_sha1": hashlib.sha1(dol).hexdigest(), "map": str(map_path.resolve()),
              "map_sha1": hashlib.sha1(map_path.read_bytes()).hexdigest(),
              "note": "Live non-atomic snapshots. Saved contexts are not the live CPU PC; compare repeated samples.", "samples": []}
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "symbols.map").write_bytes(map_path.read_bytes())
    log_path = Path(os.environ.get("APPDATA", "")) / "Dolphin Emulator/Logs/dolphin.log"
    if log_path.is_file():
        (args.out / "dolphin.log").write_bytes(log_path.read_bytes())
    reader = Reader(args.pid)
    try:
        base = reader.find_ram()
        report["host_mem1"] = hex(base)
        for index in range(args.samples):
            ram = reader.read(base, 0x1800000)
            (args.out / ("mem1-%02d.bin" % index)).write_bytes(ram)
            # Verify executable bytes, allowing Dolphin's runtime patches to be reported.
            sections = []
            for n in range(7):
                offset = struct.unpack_from(">I", dol, n * 4)[0]
                address = struct.unpack_from(">I", dol, 0x48 + n * 4)[0]
                size = struct.unpack_from(">I", dol, 0x90 + n * 4)[0]
                if size:
                    runtime = ram[address - 0x80000000:address - 0x80000000 + size]
                    sections.append({"address": hex(address), "size": size, "different_bytes": sum(a != b for a, b in zip(runtime, dol[offset:offset + size]))})
            sample = decode(ram, syms)
            sample["text_sections"] = sections
            report["samples"].append(sample)
            if index + 1 < args.samples:
                time.sleep(args.interval)
    finally:
        reader.close()
    (args.out / "report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    readable = summary(report)
    (args.out / "summary.txt").write_text(readable, encoding="utf-8")
    print(readable)
    print("Saved capture:", args.out.resolve())


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError) as error:
        raise SystemExit(str(error))
