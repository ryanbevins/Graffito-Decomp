"""Validate big-endian OS thread walking, stack cycles, and symbol bounds."""
import runpy
from pathlib import Path
import struct
import unittest

decode = runpy.run_path(str(Path(__file__).resolve().parents[1] / "capture-freeze.py"))["decode"]


class CaptureTests(unittest.TestCase):
    def test_reversed_pollution_conversion_slots_are_detected(self):
        ram = bytearray(0x1000)
        syms = [(0x80000200, 20, "__vt__20TPollutionCounterObj"),
                (0x80000300, 8, "getCounterNo__20TPollutionCounterObjCFUl"),
                (0x80000308, 8, "getTokenNo__20TPollutionCounterObjCFi")]
        struct.pack_into(">II", ram, 0x20c, 0x80000308, 0x80000300)
        result = decode(ram, syms)
        self.assertFalse(result["graphics"]["__vt__20TPollutionCounterObj"]["retail_order_correct"])
        struct.pack_into(">II", ram, 0x20c, 0x80000300, 0x80000308)
        result = decode(ram, syms)
        self.assertTrue(result["graphics"]["__vt__20TPollutionCounterObj"]["retail_order_correct"])

    def test_thread_stack_and_cycle_are_bounded(self):
        ram = bytearray(0x10000)
        def put(address, value):
            struct.pack_into(">I", ram, address - 0x80000000, value)
        thread = 0x80001000
        put(0x800000dc, thread)
        put(0x800000e4, thread)
        put(thread + 0x2c8, 4 << 16)
        put(thread + 4, 0x80004000)
        put(thread + 0x198, 0x80005004)
        put(thread + 0x304, 0x80004800)
        put(thread + 0x308, 0x80003000)
        put(thread + 0x2fc, thread)
        put(0x80004000, 0x80004000)
        put(0x80004004, 0x80005020)
        result = decode(ram, [(0x80005000, 0x20, "loadResource")])
        self.assertEqual(len(result["threads"]), 1)
        item = result["threads"][0]
        self.assertTrue(item["current"])
        self.assertEqual(item["state"], 4)
        self.assertEqual(item["saved_pc"]["symbol"], "loadResource+0x4")
        self.assertEqual(len(item["frames"]), 1)
        self.assertEqual(item["frames"][0]["symbol"], "<unknown>")

    def test_corrupt_thread_pointer_is_reported(self):
        ram = bytearray(0x1000)
        struct.pack_into(">I", ram, 0xdc, 0x12345678)
        result = decode(ram, [])
        self.assertIn("Invalid MEM1 address", result["threads"][0]["error"])


if __name__ == "__main__":
    unittest.main()
