"""Check INF1 scene graph compatibility with SHP1 in BMD files."""
import struct
import sys

def read_bmd_sections(path):
    """Parse BMD and return dict of section_tag -> (offset, size, data)."""
    with open(path, 'rb') as f:
        data = f.read()

    magic = data[:8]
    if magic[:4] not in (b'J3D2', b'J3D1'):
        print(f"WARNING: Unexpected magic: {magic[:4]}")

    file_size = struct.unpack_from('>I', data, 8)[0]
    section_count = struct.unpack_from('>I', data, 12)[0]

    print(f"File: {path}")
    print(f"  Magic: {magic}")
    print(f"  File size: {file_size} (actual: {len(data)})")
    print(f"  Section count: {section_count}")

    sections = {}
    offset = 32  # After J3D header
    for i in range(section_count):
        if offset + 8 > len(data):
            print(f"  WARNING: Ran out of data at section {i}")
            break
        tag = data[offset:offset+4].decode('ascii', errors='replace')
        size = struct.unpack_from('>I', data, offset + 4)[0]
        sections[tag] = (offset, size, data[offset:offset+size])
        print(f"  Section {i}: {tag} at 0x{offset:X}, size 0x{size:X}")
        offset += size

    return sections, data


def parse_inf1(section_data):
    """Parse INF1 section."""
    tag = section_data[:4]
    size = struct.unpack_from('>I', section_data, 4)[0]

    # INF1 header fields
    scale_flag = struct.unpack_from('>H', section_data, 8)[0]
    pad = struct.unpack_from('>H', section_data, 10)[0]
    packet_count = struct.unpack_from('>I', section_data, 12)[0]
    vertex_count = struct.unpack_from('>I', section_data, 16)[0]
    hierarchy_offset = struct.unpack_from('>I', section_data, 20)[0]

    print(f"\nINF1 Header:")
    print(f"  Scale flag: {scale_flag}")
    print(f"  Pad: {pad}")
    print(f"  Packet count: {packet_count}")
    print(f"  Vertex count: {vertex_count}")
    print(f"  Hierarchy offset: 0x{hierarchy_offset:X}")

    # Parse scene graph entries starting at offset 24
    entries = []
    off = 24
    while off + 4 <= len(section_data):
        entry_type = struct.unpack_from('>H', section_data, off)[0]
        entry_value = struct.unpack_from('>H', section_data, off + 2)[0]
        entries.append((entry_type, entry_value))
        if entry_type == 0x00:  # end
            break
        off += 4

    return {
        'scale_flag': scale_flag,
        'packet_count': packet_count,
        'vertex_count': vertex_count,
        'hierarchy_offset': hierarchy_offset,
        'entries': entries,
        'raw': section_data
    }


def print_scene_graph(entries):
    """Print the scene graph tree with indentation."""
    type_names = {
        0x00: 'END',
        0x01: 'OPEN_CHILD',
        0x02: 'CLOSE_CHILD',
        0x10: 'JOINT',
        0x11: 'MATERIAL',
        0x12: 'SHAPE',
    }

    indent = 0
    shape_indices = []

    print("\nScene Graph Tree:")
    for etype, evalue in entries:
        name = type_names.get(etype, f'UNKNOWN(0x{etype:02X})')

        if etype == 0x02:  # close child - dedent before printing
            indent = max(0, indent - 1)

        prefix = "  " * indent
        if etype in (0x10, 0x11, 0x12):
            print(f"  {prefix}{name} [{evalue}]")
        elif etype == 0x00:
            print(f"  {prefix}{name}")
        else:
            print(f"  {prefix}{name}")

        if etype == 0x12:
            shape_indices.append(evalue)

        if etype == 0x01:  # open child - indent after printing
            indent += 1

    return shape_indices


def parse_shp1_batch_count(section_data):
    """Get batch count from SHP1."""
    batch_count = struct.unpack_from('>H', section_data, 8)[0]
    print(f"\nSHP1:")
    print(f"  Batch count: {batch_count}")

    # Count total packets across all batches
    # SHP1 layout: header (8) + batchCount(2) + pad(2) + batchesOffset(4) + ...
    # We need the batches offset to find packet info
    # Each batch entry has a packetCount field
    batches_offset = struct.unpack_from('>I', section_data, 12)[0]

    total_packets = 0
    for i in range(batch_count):
        # Each batch entry is 0x28 bytes
        batch_off = batches_offset + i * 0x28
        if batch_off + 2 > len(section_data):
            break
        # Batch structure: u8 matrixType, u8 pad, u16 packetCount, ...
        pkt_count = struct.unpack_from('>H', section_data, batch_off + 2)[0]
        total_packets += pkt_count

    print(f"  Total packets across all batches: {total_packets}")
    return batch_count, total_packets


def parse_vtx1_position_count(section_data):
    """Get position vertex count from VTX1."""
    # VTX1 is more complex - try to get array count
    # VTX1: tag(4) + size(4) + arrayFormatOffset(4) + offsets[13](52)
    fmt_offset = struct.unpack_from('>I', section_data, 8)[0]

    # Read array format entries starting at fmt_offset
    # Each is: u32 arrayType, u32 componentCount, u32 dataType, u8 decimalPoint, u8 pad[3]
    # arrayType 9 = position
    off = fmt_offset
    pos_count = None

    # Read data offsets (13 possible attribute arrays)
    data_offsets = []
    for i in range(13):
        doff = struct.unpack_from('>I', section_data, 12 + i * 4)[0]
        data_offsets.append(doff)

    print(f"\nVTX1:")
    print(f"  Format offset: 0x{fmt_offset:X}")
    print(f"  Data offsets: {['0x{:X}'.format(d) for d in data_offsets if d != 0]}")

    # Try to determine position count from data size
    # Position is usually the first array (index 0)
    # Each position is 3 floats = 12 bytes (for float type)
    if data_offsets[0] != 0:
        # Find end of position data (next non-zero offset or section end)
        pos_start = data_offsets[0]
        pos_end = len(section_data)
        for d in data_offsets[1:]:
            if d != 0 and d > pos_start:
                pos_end = min(pos_end, d)
                break
        pos_size = pos_end - pos_start
        # Assuming float xyz = 12 bytes each
        pos_count = pos_size // 12  # rough estimate
        print(f"  Position data: 0x{pos_start:X} - 0x{pos_end:X} ({pos_size} bytes, ~{pos_count} vertices)")

    return pos_count


def main():
    exported_path = r"C:\Users\ryana\Downloads\reconstruct_test.bmd"
    original_path = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"

    print("=" * 60)
    print("EXPORTED BMD")
    print("=" * 60)
    exp_sections, exp_data = read_bmd_sections(exported_path)

    if 'INF1' not in exp_sections:
        print("ERROR: No INF1 section found!")
        return

    exp_inf1 = parse_inf1(exp_sections['INF1'][2])
    exp_shapes = print_scene_graph(exp_inf1['entries'])

    print(f"\nShape node summary:")
    print(f"  Total shape nodes: {len(exp_shapes)}")
    print(f"  Shape indices referenced: {exp_shapes}")

    if 'SHP1' in exp_sections:
        batch_count, total_packets = parse_shp1_batch_count(exp_sections['SHP1'][2])

        # Verify shape indices are valid
        bad = [s for s in exp_shapes if s >= batch_count]
        if bad:
            print(f"\n  ERROR: Shape indices out of range (>= {batch_count}): {bad}")
        else:
            print(f"\n  OK: All shape indices are valid (< {batch_count})")

        if len(exp_shapes) != batch_count:
            print(f"  WARNING: Shape node count ({len(exp_shapes)}) != batch count ({batch_count})")
        else:
            print(f"  OK: Shape node count matches batch count ({batch_count})")

        # Check packet count
        if exp_inf1['packet_count'] != total_packets:
            print(f"  WARNING: INF1 packetCount ({exp_inf1['packet_count']}) != SHP1 total packets ({total_packets})")
        else:
            print(f"  OK: INF1 packetCount matches SHP1 total ({total_packets})")

    if 'VTX1' in exp_sections:
        vtx_count = parse_vtx1_position_count(exp_sections['VTX1'][2])
        if vtx_count is not None:
            if exp_inf1['vertex_count'] != vtx_count:
                print(f"  INFO: INF1 vertexCount ({exp_inf1['vertex_count']}) vs VTX1 position count (~{vtx_count})")
            else:
                print(f"  OK: INF1 vertexCount matches VTX1 position count ({vtx_count})")

    # Now compare with original
    print("\n" + "=" * 60)
    print("ORIGINAL BMD")
    print("=" * 60)
    try:
        orig_sections, orig_data = read_bmd_sections(original_path)
    except FileNotFoundError:
        print(f"WARNING: Original file not found at {original_path}")
        return

    if 'INF1' in orig_sections:
        orig_inf1 = parse_inf1(orig_sections['INF1'][2])
        orig_shapes = print_scene_graph(orig_inf1['entries'])

        print(f"\nShape node summary:")
        print(f"  Total shape nodes: {len(orig_shapes)}")
        print(f"  Shape indices referenced: {orig_shapes}")

        if 'SHP1' in orig_sections:
            orig_batch_count, orig_total_packets = parse_shp1_batch_count(orig_sections['SHP1'][2])

    # Byte-identical comparison of INF1
    print("\n" + "=" * 60)
    print("INF1 COMPARISON")
    print("=" * 60)

    if 'INF1' in exp_sections and 'INF1' in orig_sections:
        exp_inf1_raw = exp_sections['INF1'][2]
        orig_inf1_raw = orig_sections['INF1'][2]

        if exp_inf1_raw == orig_inf1_raw:
            print("RESULT: INF1 sections are BYTE IDENTICAL")
        else:
            print("RESULT: INF1 sections DIFFER!")
            print(f"  Exported size: {len(exp_inf1_raw)}")
            print(f"  Original size: {len(orig_inf1_raw)}")

            # Find first difference
            min_len = min(len(exp_inf1_raw), len(orig_inf1_raw))
            for i in range(min_len):
                if exp_inf1_raw[i] != orig_inf1_raw[i]:
                    print(f"  First difference at offset 0x{i:X}: exported=0x{exp_inf1_raw[i]:02X} original=0x{orig_inf1_raw[i]:02X}")
                    # Show surrounding context
                    start = max(0, i - 4)
                    end = min(min_len, i + 12)
                    print(f"  Exported [{start:X}:{end:X}]: {exp_inf1_raw[start:end].hex()}")
                    print(f"  Original [{start:X}:{end:X}]: {orig_inf1_raw[start:end].hex()}")
                    break

            if len(exp_inf1_raw) != len(orig_inf1_raw):
                print(f"  Size mismatch: exported has {abs(len(exp_inf1_raw) - len(orig_inf1_raw))} extra bytes")


if __name__ == '__main__':
    main()
