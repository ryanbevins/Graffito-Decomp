#!/usr/bin/env python3
# usage: python3 tools/agent/read_dol_word.py 0x804129ac [ADDRESS ...]
"""Read retail words by virtual address, without ELF relocation normalization."""

import argparse
import struct
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("addresses", nargs="+", type=lambda value: int(value, 0))
    parser.add_argument("--dol", type=Path, default=Path("orig/GMSJ01/sys/main.dol"))
    args = parser.parse_args()
    data = args.dol.read_bytes()
    if len(data) < 0x100:
        parser.error("DOL header is truncated")
    offsets = struct.unpack_from(">18I", data, 0)
    addresses = struct.unpack_from(">18I", data, 0x48)
    sizes = struct.unpack_from(">18I", data, 0x90)
    for address in args.addresses:
        for offset, start, size in zip(offsets, addresses, sizes):
            if start <= address and address + 4 <= start + size:
                file_offset = offset + address - start
                if file_offset + 4 > len(data):
                    parser.error("DOL section extends beyond the file")
                word = data[file_offset:file_offset + 4]
                integer, = struct.unpack(">I", word)
                real, = struct.unpack(">f", word)
                print(f"{address:#010x} file={file_offset:#x} "
                      f"bytes={word.hex()} u32={integer} f32={real!r}")
                break
        else:
            parser.error(f"{address:#x} is not a file-backed DOL word")


if __name__ == "__main__":
    main()
