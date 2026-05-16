"""Debug SHP1 batch layout."""
import struct

BMD_PATH = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"

def read_u8(data, off): return struct.unpack_from('>B', data, off)[0]
def read_u16(data, off): return struct.unpack_from('>H', data, off)[0]
def read_u32(data, off): return struct.unpack_from('>I', data, off)[0]

with open(BMD_PATH, 'rb') as f:
    data = f.read()

shp1_off = data.find(b'SHP1')
shp1_size = read_u32(data, shp1_off + 4)
shp1_batch_count = read_u16(data, shp1_off + 8)

# Print all SHP1 header offsets
print("SHP1 header offsets:")
for i in range(8):
    off = shp1_off + 0x08 + i * 4
    val = read_u32(data, off)
    print(f"  +0x{0x08+i*4:02X}: 0x{val:08X}")

shp1_batches_off = shp1_off + read_u32(data, shp1_off + 0x0C)

# Dump raw batch 0 bytes (0x28 bytes)
print(f"\nBatch 0 raw (0x28 bytes) @ 0x{shp1_batches_off:X}:")
for i in range(0x28):
    if i % 16 == 0:
        print(f"  +0x{i:02X}: ", end="")
    print(f"{read_u8(data, shp1_batches_off + i):02X} ", end="")
    if (i + 1) % 16 == 0:
        print()
print()

# Try different batch struct interpretations
# Standard: matType(u8), pad(u8), vtxCount(u16) or packetCount(u16), attribOffset(u16),
#           firstMatrixData(u16), firstPacketLocation(u16), pad(u16), boundingBox...
print("Batch 0 fields (standard interpretation):")
b = shp1_batches_off
print(f"  +00 matType: {read_u8(data, b)}")
print(f"  +01 pad: {read_u8(data, b+1)}")
print(f"  +02 packetCount: {read_u16(data, b+2)}")
print(f"  +04 attribOffset: {read_u16(data, b+4)}")
print(f"  +06 firstMtxData: {read_u16(data, b+6)}")
print(f"  +08 firstPacketLoc: {read_u16(data, b+8)}")
print(f"  +0A pad: {read_u16(data, b+0xA)}")

# Now check draw init data at different offsets
shp1_draw_init_off = shp1_off + read_u32(data, shp1_off + 0x28)
shp1_mtx_init_off = shp1_off + read_u32(data, shp1_off + 0x24)

print(f"\nDraw init table @ 0x{shp1_draw_init_off:X}:")
for i in range(5):
    off = shp1_draw_init_off + i * 8
    v1 = read_u32(data, off)
    v2 = read_u32(data, off + 4)
    print(f"  [{i}]: offset=0x{v1:08X}, size=0x{v2:08X}")

print(f"\nMatrix init table @ 0x{shp1_mtx_init_off:X}:")
for i in range(5):
    off = shp1_mtx_init_off + i * 8
    v1 = read_u16(data, off)
    v2 = read_u16(data, off + 2)
    v3 = read_u32(data, off + 4)
    print(f"  [{i}]: count={v1}, pad={v2}, firstIdx={v3}")

# Try alternate: draw init might be 4+4 or different structure
# Let's also check if packetLoc might index differently
print(f"\nChecking firstPacketLoc=0 with draw_init entries:")
print(f"  draw_init[0] as 4+4: {read_u32(data, shp1_draw_init_off):08X} {read_u32(data, shp1_draw_init_off+4):08X}")

# The issue might be that firstPacketLoc is not 0 for batch 0
# or the draw init struct is different
# Let me check all batch firstPacketLoc values
print("\nAll batch firstPacketLoc values:")
for i in range(shp1_batch_count):
    boff = shp1_batches_off + i * 0x28
    fpl = read_u16(data, boff + 8)
    fmd = read_u16(data, boff + 6)
    pktcnt = read_u16(data, boff + 2)
    print(f"  Batch[{i}]: packetCount={pktcnt}, firstMtxData={fmd}, firstPacketLoc={fpl}")

# Let me also try reading the prim data directly
shp1_prim_data_off = shp1_off + read_u32(data, shp1_off + 0x20)
print(f"\nPrim data @ 0x{shp1_prim_data_off:X}:")
print(f"  First 32 bytes: {' '.join(f'{read_u8(data, shp1_prim_data_off+x):02X}' for x in range(32))}")

# Maybe the draw init structure uses a different size
# Let's dump more of the draw init area
print(f"\nDraw init raw (first 64 bytes):")
for i in range(64):
    if i % 16 == 0:
        print(f"  +0x{i:02X}: ", end="")
    print(f"{read_u8(data, shp1_draw_init_off + i):02X} ", end="")
    if (i + 1) % 16 == 0:
        print()
print()
