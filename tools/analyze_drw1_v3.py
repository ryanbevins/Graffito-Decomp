"""Analyze DRW1 + Batch 9 raw data. V3 - debug attribs."""
import struct
from pathlib import Path

def r8(d,o): return struct.unpack_from('>B',d,o)[0]
def r16(d,o): return struct.unpack_from('>H',d,o)[0]
def r32(d,o): return struct.unpack_from('>I',d,o)[0]

def find_section(data, tag):
    off = 0x20
    while off < len(data):
        if data[off:off+4] == tag:
            return off, r32(data, off+4)
        off += r32(data, off+4)
    return None, None

def parse_drw1(data):
    off, _ = find_section(data, b'DRW1')
    if off is None: return []
    count = r16(data, off + 8)
    wo = r32(data, off + 12) + off
    do = r32(data, off + 16) + off
    return [(r8(data, wo+i), r16(data, do+i*2)) for i in range(count)]

def parse_shp1(data):
    off, size = find_section(data, b'SHP1')
    if off is None: return None
    # Print raw header offsets for debugging
    print(f"  SHP1 at 0x{off:X}, size=0x{size:X}")
    print(f"  batch_count = {r16(data, off+8)}")
    fields = ['batches', 'unused0x10', 'unused0x14', 'attribs', 'mtx_table', 'prim_data', 'mtx_init', 'draw_init']
    offsets = {}
    for i, name in enumerate(fields):
        raw = r32(data, off + 0x0C + i*4)
        abs_off = raw + off
        print(f"  {name}: rel=0x{raw:X} abs=0x{abs_off:X}")
        offsets[name] = abs_off
    offsets['off'] = off
    offsets['batch_count'] = r16(data, off+8)
    return offsets

def dump_batch_raw(data, shp1, idx):
    """Dump raw batch header bytes."""
    b = shp1['batches'] + idx * 0x28
    print(f"\n  Batch {idx} raw header at 0x{b:X}:")
    for i in range(0, 0x28, 4):
        print(f"    +0x{i:02X}: {data[b+i:b+i+4].hex()}")

    mat_type = r8(data, b)
    num_packets = r16(data, b + 2)
    attrib_offset = r16(data, b + 4)  # This is index into attrib list
    first_mtx = r16(data, b + 6)
    first_pkt = r16(data, b + 8)

    print(f"  matType={mat_type}, numPackets={num_packets}")
    print(f"  attribOffset={attrib_offset} (index into attrib table)")
    print(f"  firstMtxData={first_mtx}, firstPacketLoc={first_pkt}")

    # Dump attrib table area - the offset is a BYTE offset into the indexed table
    # Actually in BMD, attribOffset indexes into a separate per-batch attrib list
    # Let's dump raw bytes around the attrib area
    # The attrib table is a flat array of (GXAttr:u32, GXCompCnt:u32, GXCompType:u32, padding:u8?) ...
    # Actually BMD SHP1 attribs: pairs of (GXAttr:u16, GXAttrType:u16) terminated by (0xFF, 0xFF)
    # Wait - some formats use u32 pairs. Let me check.

    # Try reading as u16 pairs first
    a_base = shp1['attribs']
    print(f"\n  Attrib table base: 0x{a_base:X}")

    # Dump first 64 bytes of attrib table
    print(f"  Raw attrib table (first 64 bytes):")
    for i in range(0, 64, 16):
        hex_str = data[a_base+i:a_base+i+16].hex()
        print(f"    0x{a_base+i:X}: {hex_str}")

    # Now read attribs at the offset for this batch
    # attrib_offset is typically a count of u32 entries (or byte offset?)
    print(f"\n  Trying attribs at byte offset {attrib_offset} from attrib base:")
    a_off = a_base + attrib_offset
    print(f"    Reading from 0x{a_off:X}:")
    for i in range(10):
        if a_off + i*4 + 4 > len(data): break
        attr = r16(data, a_off + i*4)
        dtype = r16(data, a_off + i*4 + 2)
        print(f"    [{i}] attr=0x{attr:04X}({attr}) dtype=0x{dtype:04X}({dtype})")
        if attr == 0xFFFF: break

    # Try interpreting attrib_offset as entry index (each entry = 4 bytes)
    print(f"\n  Trying attribs at entry index {attrib_offset} (offset {attrib_offset*4}):")
    a_off2 = a_base + attrib_offset * 4
    print(f"    Reading from 0x{a_off2:X}:")
    for i in range(10):
        if a_off2 + i*4 + 4 > len(data): break
        attr = r16(data, a_off2 + i*4)
        dtype = r16(data, a_off2 + i*4 + 2)
        print(f"    [{i}] attr=0x{attr:04X}({attr}) dtype=0x{dtype:04X}({dtype})")
        if attr == 0xFFFF: break

    return mat_type, num_packets, first_mtx, first_pkt

def main():
    orig = Path(r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd").read_bytes()
    exp = Path(r"C:\Users\ryana\Downloads\reconstruct_test.bmd").read_bytes()

    print("=== ORIGINAL ===")
    orig_shp1 = parse_shp1(orig)

    # Dump batch 0 for reference (simple batch)
    print("\n--- Batch 0 (simple, for reference) ---")
    dump_batch_raw(orig, orig_shp1, 0)

    print("\n--- Batch 9 (target) ---")
    dump_batch_raw(orig, orig_shp1, 9)

    print("\n\n=== EXPORT ===")
    exp_shp1 = parse_shp1(exp)

    print("\n--- Batch 0 ---")
    dump_batch_raw(exp, exp_shp1, 0)

    print("\n--- Batch 9 ---")
    dump_batch_raw(exp, exp_shp1, 9)

if __name__ == '__main__':
    main()
