#!/usr/bin/env python3
"""Byte-level SHP1 metadata comparison for batch 9 between original and export."""

import struct

def read_u8(data, pos): return data[pos]
def read_u16(data, pos): return struct.unpack_from('>H', data, pos)[0]
def read_u32(data, pos): return struct.unpack_from('>I', data, pos)[0]
def read_f32(data, pos): return struct.unpack_from('>f', data, pos)[0]
def read_s16(data, pos): return struct.unpack_from('>h', data, pos)[0]

class SHP1Info:
    def __init__(self, data, label):
        self.data = data
        self.label = label
        self.base = data.find(b'SHP1')
        assert self.base != -1, f"SHP1 not found in {label}"

        self.size = read_u32(data, self.base + 4)
        self.batch_count = read_u16(data, self.base + 8)
        self.pad = read_u16(data, self.base + 10)

        self.off_batches    = read_u32(data, self.base + 0x0C)
        self.off_unknown    = read_u32(data, self.base + 0x10)  # remapTable
        self.off_zero       = read_u32(data, self.base + 0x14)
        self.off_attribs    = read_u32(data, self.base + 0x18)
        self.off_mtx_table  = read_u32(data, self.base + 0x1C)
        self.off_data       = read_u32(data, self.base + 0x20)
        self.off_mtx_init   = read_u32(data, self.base + 0x24)  # matrixData / matrixInit
        self.off_draw_init  = read_u32(data, self.base + 0x28)  # packetLocations / drawInit

    def abs(self, rel_off):
        return self.base + rel_off

    def get_batch_header(self, bi):
        off = self.abs(self.off_batches) + bi * 40
        raw = self.data[off:off+40]
        return {
            'raw': raw,
            'mattype': read_u16(self.data, off + 0),
            'packetCount': read_u16(self.data, off + 2),
            'attribOffset': read_u16(self.data, off + 4),
            'firstMatrixData': read_u16(self.data, off + 6),
            'firstDrawInit': read_u16(self.data, off + 8),
            'padding': read_u16(self.data, off + 10),
            'bbox': [read_f32(self.data, off + 12 + i*4) for i in range(7)],
        }

    def get_mtx_init(self, idx):
        """Read matrixData entry (8 bytes): useMtxIndex(u16), count(u16), firstIndex(u32)"""
        off = self.abs(self.off_mtx_init) + idx * 8
        return {
            'useMtxIndex': read_u16(self.data, off + 0),
            'count': read_u16(self.data, off + 2),
            'firstIndex': read_u32(self.data, off + 4),
            'raw': self.data[off:off+8],
        }

    def get_draw_init(self, idx):
        """Read packetLocation entry (8 bytes): size(u32), offset(u32)"""
        off = self.abs(self.off_draw_init) + idx * 8
        return {
            'size': read_u32(self.data, off + 0),
            'offset': read_u32(self.data, off + 4),
            'raw': self.data[off:off+8],
        }

    def get_mtx_table_entries(self, first_index, count):
        entries = []
        off = self.abs(self.off_mtx_table) + first_index * 2
        for i in range(count):
            entries.append(read_u16(self.data, off + i * 2))
        return entries

    def get_prim_data_start(self, draw_init_offset, nbytes=32):
        abs_off = self.abs(self.off_data) + draw_init_offset
        return self.data[abs_off:abs_off+nbytes], abs_off

    def get_attribs(self, attrib_byte_offset):
        """Read attribute list starting at byte offset from attribs section."""
        attribs = []
        off = self.abs(self.off_attribs) + attrib_byte_offset
        while True:
            attr = read_u32(self.data, off)
            dtype = read_u32(self.data, off + 4)
            if attr == 0xFF:
                break
            attribs.append((attr, dtype))
            off += 8
        return attribs


def hex_bytes(raw):
    return ' '.join(f'{b:02x}' for b in raw)


def compare_field(name, orig_val, exp_val, indent="    "):
    match = "OK" if orig_val == exp_val else "MISMATCH!"
    if isinstance(orig_val, float):
        print(f"{indent}{name}: orig={orig_val:.6f}  exp={exp_val:.6f}  [{match}]")
    elif isinstance(orig_val, list):
        if orig_val == exp_val:
            print(f"{indent}{name}: {orig_val}  [OK]")
        else:
            print(f"{indent}{name}:")
            print(f"{indent}  orig={orig_val}")
            print(f"{indent}  exp ={exp_val}  [MISMATCH!]")
    else:
        if isinstance(orig_val, int) and orig_val > 255:
            print(f"{indent}{name}: orig={orig_val:#06x}  exp={exp_val:#06x}  [{match}]")
        else:
            print(f"{indent}{name}: orig={orig_val}  exp={exp_val}  [{match}]")


def main():
    with open(r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd", 'rb') as f:
        orig_data = f.read()
    with open(r"C:\Users\ryana\Downloads\reconstruct_test.bmd", 'rb') as f:
        exp_data = f.read()

    orig = SHP1Info(orig_data, "ORIG")
    exp = SHP1Info(exp_data, "EXP")

    print("=" * 70)
    print("SHP1 HEADER COMPARISON")
    print("=" * 70)
    for name, ov, ev in [
        ("sectionSize", orig.size, exp.size),
        ("batchCount", orig.batch_count, exp.batch_count),
        ("pad", orig.pad, exp.pad),
        ("off_batches", orig.off_batches, exp.off_batches),
        ("off_remapTable", orig.off_unknown, exp.off_unknown),
        ("off_zero", orig.off_zero, exp.off_zero),
        ("off_attribs", orig.off_attribs, exp.off_attribs),
        ("off_mtxTable", orig.off_mtx_table, exp.off_mtx_table),
        ("off_primData", orig.off_data, exp.off_data),
        ("off_mtxInit", orig.off_mtx_init, exp.off_mtx_init),
        ("off_drawInit", orig.off_draw_init, exp.off_draw_init),
    ]:
        compare_field(name, ov, ev, "  ")

    # BATCH 9 header
    BATCH = 9
    print(f"\n{'='*70}")
    print(f"BATCH {BATCH} HEADER (40 bytes)")
    print(f"{'='*70}")

    ob = orig.get_batch_header(BATCH)
    eb = exp.get_batch_header(BATCH)

    print(f"  ORIG raw: {hex_bytes(ob['raw'])}")
    print(f"  EXP  raw: {hex_bytes(eb['raw'])}")

    for field in ['mattype', 'packetCount', 'attribOffset', 'firstMatrixData',
                   'firstDrawInit', 'padding']:
        compare_field(field, ob[field], eb[field])

    print(f"  bbox:")
    for i in range(7):
        label = ['unk', 'minX', 'minY', 'minZ', 'maxX', 'maxY', 'maxZ'][i]
        compare_field(f"  [{i}] {label}", ob['bbox'][i], eb['bbox'][i], "    ")

    # Attribs check
    print(f"\n  Attribs at offset {ob['attribOffset']}:")
    orig_attribs = orig.get_attribs(ob['attribOffset'])
    exp_attribs = exp.get_attribs(eb['attribOffset'])
    print(f"    ORIG: {orig_attribs}")
    print(f"    EXP:  {exp_attribs}")
    if orig_attribs == exp_attribs:
        print(f"    [OK]")
    else:
        print(f"    [MISMATCH!]")

    # Compute stride from attribs
    stride = sum(1 if dt == 1 else 2 for _, dt in orig_attribs)
    print(f"    Vertex stride: {stride} bytes")

    # Per-packet comparison
    orig_pkt_count = ob['packetCount']
    exp_pkt_count = eb['packetCount']

    print(f"\n{'='*70}")
    print(f"BATCH {BATCH} PACKETS: orig={orig_pkt_count}, exp={exp_pkt_count}")
    print(f"{'='*70}")

    max_pkts = max(orig_pkt_count, exp_pkt_count)
    for pi in range(max_pkts):
        print(f"\n  --- Packet {pi} ---")

        # Matrix init
        if pi < orig_pkt_count:
            omi = orig.get_mtx_init(ob['firstMatrixData'] + pi)
            print(f"  ORIG mtxInit: raw={hex_bytes(omi['raw'])}")
            print(f"    useMtxIndex={omi['useMtxIndex']:#06x}  count={omi['count']}  firstIndex={omi['firstIndex']}")
        else:
            omi = None
            print(f"  ORIG mtxInit: N/A (packet {pi} doesn't exist in orig)")

        if pi < exp_pkt_count:
            emi = exp.get_mtx_init(eb['firstMatrixData'] + pi)
            print(f"  EXP  mtxInit: raw={hex_bytes(emi['raw'])}")
            print(f"    useMtxIndex={emi['useMtxIndex']:#06x}  count={emi['count']}  firstIndex={emi['firstIndex']}")
        else:
            emi = None
            print(f"  EXP  mtxInit: N/A (packet {pi} doesn't exist in exp)")

        if omi and emi:
            compare_field("useMtxIndex", omi['useMtxIndex'], emi['useMtxIndex'], "    ")
            compare_field("count", omi['count'], emi['count'], "    ")

        # Draw init
        if pi < orig_pkt_count:
            odi = orig.get_draw_init(ob['firstDrawInit'] + pi)
            print(f"  ORIG drawInit: raw={hex_bytes(odi['raw'])}")
            print(f"    size={odi['size']}  offset={odi['offset']:#x}")
        else:
            odi = None

        if pi < exp_pkt_count:
            edi = exp.get_draw_init(eb['firstDrawInit'] + pi)
            print(f"  EXP  drawInit: raw={hex_bytes(edi['raw'])}")
            print(f"    size={edi['size']}  offset={edi['offset']:#x}")
        else:
            edi = None

        if odi and edi:
            compare_field("displayListSize", odi['size'], edi['size'], "    ")
            compare_field("displayListOffset", odi['offset'], edi['offset'], "    ")
            # Check 32-byte alignment
            if odi['size'] % 32 != 0:
                print(f"    ORIG size NOT 32-aligned! ({odi['size'] % 32})")
            if edi['size'] % 32 != 0:
                print(f"    EXP  size NOT 32-aligned! ({edi['size'] % 32})")
            if odi['offset'] % 32 != 0:
                print(f"    ORIG offset NOT 32-aligned! ({odi['offset'] % 32})")
            if edi['offset'] % 32 != 0:
                print(f"    EXP  offset NOT 32-aligned! ({edi['offset'] % 32})")

        # Matrix table entries
        if omi:
            entries = orig.get_mtx_table_entries(omi['firstIndex'], omi['count'])
            print(f"  ORIG mtxTable[{omi['firstIndex']}..{omi['firstIndex']+omi['count']-1}]: {entries}")
        if emi:
            entries = exp.get_mtx_table_entries(emi['firstIndex'], emi['count'])
            print(f"  EXP  mtxTable[{emi['firstIndex']}..{emi['firstIndex']+emi['count']-1}]: {entries}")

        # Primitive data first bytes
        if odi:
            prim_raw, abs_off = orig.get_prim_data_start(odi['offset'], 32)
            gx_op = prim_raw[0]
            vert_count = struct.unpack_from('>H', prim_raw, 1)[0] if len(prim_raw) >= 3 else 0
            print(f"  ORIG primData @{abs_off:#x}: op={gx_op:#04x} vertCount={vert_count}")
            print(f"    first 20 bytes: {hex_bytes(prim_raw[:20])}")

        if edi:
            prim_raw, abs_off = exp.get_prim_data_start(edi['offset'], 32)
            gx_op = prim_raw[0]
            vert_count = struct.unpack_from('>H', prim_raw, 1)[0] if len(prim_raw) >= 3 else 0
            print(f"  EXP  primData @{abs_off:#x}: op={gx_op:#04x} vertCount={vert_count}")
            print(f"    first 20 bytes: {hex_bytes(prim_raw[:20])}")

    # BATCH 10 quick comparison
    BATCH2 = 10
    print(f"\n{'='*70}")
    print(f"BATCH {BATCH2} HEADER")
    print(f"{'='*70}")
    ob2 = orig.get_batch_header(BATCH2)
    eb2 = exp.get_batch_header(BATCH2)
    print(f"  ORIG raw: {hex_bytes(ob2['raw'])}")
    print(f"  EXP  raw: {hex_bytes(eb2['raw'])}")
    for field in ['mattype', 'packetCount', 'attribOffset', 'firstMatrixData',
                   'firstDrawInit', 'padding']:
        compare_field(field, ob2[field], eb2[field])

    for pi in range(max(ob2['packetCount'], eb2['packetCount'])):
        print(f"\n  --- Batch {BATCH2} Packet {pi} ---")
        if pi < ob2['packetCount']:
            omi = orig.get_mtx_init(ob2['firstMatrixData'] + pi)
            print(f"  ORIG mtxInit: useMtx={omi['useMtxIndex']:#06x} count={omi['count']} firstIdx={omi['firstIndex']}")
            entries = orig.get_mtx_table_entries(omi['firstIndex'], omi['count'])
            print(f"  ORIG mtxTable: {entries}")
        if pi < eb2['packetCount']:
            emi = exp.get_mtx_init(eb2['firstMatrixData'] + pi)
            print(f"  EXP  mtxInit: useMtx={emi['useMtxIndex']:#06x} count={emi['count']} firstIdx={emi['firstIndex']}")
            entries = exp.get_mtx_table_entries(emi['firstIndex'], emi['count'])
            print(f"  EXP  mtxTable: {entries}")

    # SuperBMD comparison note
    print(f"\n{'='*70}")
    print("useMtxIndex SUMMARY (batch 9)")
    print(f"{'='*70}")
    print(f"  {'Pkt':>3}  {'ORIG':>8}  {'EXP':>8}  {'Match':>6}")
    for pi in range(max_pkts):
        o_val = "N/A"
        e_val = "N/A"
        if pi < orig_pkt_count:
            omi = orig.get_mtx_init(ob['firstMatrixData'] + pi)
            o_val = f"0x{omi['useMtxIndex']:04x}"
        if pi < exp_pkt_count:
            emi = exp.get_mtx_init(eb['firstMatrixData'] + pi)
            e_val = f"0x{emi['useMtxIndex']:04x}"
        match = "YES" if o_val == e_val else "NO"
        if o_val == "N/A" or e_val == "N/A":
            match = "-"
        print(f"  {pi:>3}  {o_val:>8}  {e_val:>8}  {match:>6}")

    print(f"\n  Note: SuperBMD always writes 0x0000 for useMtxIndex.")
    print(f"  Our export writes matrixTable[0] as fallback.")
    print(f"  Original has specific values including 0xFFFF.")


if __name__ == '__main__':
    main()
