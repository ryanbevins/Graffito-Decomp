"""Debug matrix init data for weighted batches."""
import struct

BMD_PATH = r"C:\Users\ryana\documents\mario_extracted\bmd\ma_mdl1.bmd"

def read_u8(data, off): return struct.unpack_from('>B', data, off)[0]
def read_u16(data, off): return struct.unpack_from('>H', data, off)[0]
def read_u32(data, off): return struct.unpack_from('>I', data, off)[0]

with open(BMD_PATH, 'rb') as f:
    data = f.read()

shp1_off = data.find(b'SHP1')
shp1_mtx_init_off = shp1_off + read_u32(data, shp1_off + 0x24)
shp1_draw_init_off = shp1_off + read_u32(data, shp1_off + 0x28)

print(f"mtx_init @ 0x{shp1_mtx_init_off:X}")
print(f"draw_init @ 0x{shp1_draw_init_off:X}")
print(f"gap = {shp1_draw_init_off - shp1_mtx_init_off} bytes")

# Dump all matrix init entries (there should be 22: batches 0-8 have 1 pkt each = 9, batch 9 has 11, batch 10 has 2 = 22)
# At 8 bytes each, that's 176 bytes
# The gap is draw_init - mtx_init
gap = shp1_draw_init_off - shp1_mtx_init_off
print(f"Gap = {gap} bytes, fits {gap // 8} entries of 8 bytes")

# Dump first 22 entries as 8-byte structs
print("\nMatrix init as 8-byte entries:")
for i in range(22):
    off = shp1_mtx_init_off + i * 8
    raw = ' '.join(f'{read_u8(data, off+j):02X}' for j in range(8))
    u16_0 = read_u16(data, off)
    u16_1 = read_u16(data, off + 2)
    u32_0 = read_u32(data, off + 4)
    print(f"  [{i:2d}]: {raw}  | count={u16_0}, u16_1={u16_1}, u32={u32_0}")

# What if it's 4-byte entries? u16 count, u16 firstIndex
print("\nMatrix init as 4-byte entries (u16 count, u16 firstIdx):")
for i in range(22):
    off = shp1_mtx_init_off + i * 4
    count = read_u16(data, off)
    first = read_u16(data, off + 2)
    print(f"  [{i:2d}]: count={count}, firstIdx={first}")
