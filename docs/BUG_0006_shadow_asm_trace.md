# BUG 0006 ShadowUtil ASM trace

This file traces the kind-1 custom square-shadow path used by the save-file
blocks that produced the large dark rectangles.

Compared objects:

- Original: `build/GMSJ01/obj/MarioUtil/ShadowUtil.o`
- Pre-fix source build: `build/GMSJ01/src/MarioUtil/ShadowUtil.o`

Post-fix deployed DOL:

- SHA1: `90CD092CD6D9A6A5E4783EC6C0E66D5686172397`

Function of interest:

- `TMBindShadowManager::drawShadowVolume(bool, TAlphaShadowQuad*)`

Useful symbols:

- `quad` = `TAlphaShadowQuad*`
- `base` = `quad->unk64`, the `TSquareShadowInfo::unk0` vertex array
- `a` = current square vertex `base[i]`
- `b` = next square vertex `base[(i + 1) % 5]`
- `aT` = `(a.x, a.y + 50.0f, a.z)`
- `aB` = `(a.x, a.y - 50.0f, a.z)`
- `bT` = `(b.x, b.y + 50.0f, b.z)`
- `bB` = `(b.x, b.y - 50.0f, b.z)`

## Index Tables

Retail `.rodata`:

```asm
0040 00000002 00000001 00000000 00000003  // top[0..3] = 2,1,0,3
0050 00000002 00000000 00000004 00000003  // top[4..7] = 2,0,4,3
0060 00000000 00000000 00000001 00000002  // top[8] = 0, bottom[0..2] = 0,1,2
0070 00000000 00000002 00000003 00000000  // bottom[3..6] = 0,2,3,0
0080 00000003 00000004                    // bottom[7..8] = 3,4
```

Current `.rodata`:

```asm
0048 00000002 00000001 00000000 00000003  // top[0..3] = 2,1,0,3
0058 00000002 00000000 00000004 00000003  // top[4..7] = 2,0,4,3
0068 00000000 00000000 00000001 00000002  // top[8] = 0, bottom[0..2] = 0,1,2
0078 00000000 00000002 00000003 00000000  // bottom[3..6] = 0,2,3,0
0088 00000003 00000004                    // bottom[7..8] = 3,4
```

Conclusion: the cap index values are equal. The offset differs only because our
TU has extra rodata before these arrays.

## Square Vertex Setup In calcVtx

The square vertex write block is value-equivalent. The compiler emits it very
differently, but both sides calculate:

```cpp
dx = request.x - original.x;
dz = request.z - original.z;

if (original.x >= request.x && original.z >= request.z) {
    v0 = ( reqC, 0, -req10);
    v1 = ( dx + reqC, 0, dz - req10);
    v2 = ( dx - reqC, 0, dz - req10);
    v3 = ( dx - reqC, 0, dz + req10);
    v4 = (-reqC, 0,  req10);
} else {
    v0 = ( 1, 0,  1);
    v1 = ( dx + 1, 0, dz - 1);
    v2 = ( dx - 1, 0, dz - 1);
    v3 = ( dx - 1, 0, dz + 1);
    v4 = (-1, 0,  1);
}
```

Retail selected lines:

```asm
4e8: lfs f1,368(r1)        // f1 = original.x
4f0: lfs f0,256(r1)        // f0 = request.x
4f8: fcmpo cr0,f1,f0       // compare original.x >= request.x
520: lfs f1,376(r1)        // f1 = original.z
524: lfs f9,264(r1)        // f9 = request.z
528: fcmpo cr0,f1,f9       // compare original.z >= request.z
538: lfs f6,256(r1)        // f6 = request.x
548: lfs f1,204(r1)        // f1 = original.x
550: lfs f3,212(r1)        // f3 = original.z
558: fsubs f1,f6,f1        // f1 = dx
564: fsubs f2,f9,f3        // f2 = dz
560: stfsx f2,r4,r3        // v0.x = reqC
580: stfsx f3,r4,r3        // v0.z = -req10
59c: stfsx f3,r4,r3        // v1.x = dx + reqC
5b8: stfsx f3,r4,r3        // v1.z = dz - req10
5d4: stfsx f3,r4,r3        // v2.x = dx - reqC
5f0: stfsx f3,r4,r3        // v2.z = dz - req10
60c: stfsx f1,r4,r3        // v3.x = dx - reqC
628: stfsx f1,r4,r3        // v3.z = dz + req10
644: stfsx f1,r4,r3        // v4.x = -reqC
65c: stfsx f1,r4,r3        // v4.z = req10
670: stfsx f4,r4,r3        // v0.y = 0.0f
684: stfsx f4,r4,r3        // v1.y = 0.0f
698: stfsx f4,r4,r3        // v2.y = 0.0f
6ac: stfsx f4,r4,r3        // v3.y = 0.0f
6c0: stfsx f4,r4,r3        // v4.y = 0.0f
```

Current selected lines:

```asm
480: lfs f1,0(r27)         // f1 = request.x
488: lfs f2,76(r1)         // f2 = original.x
490: fcmpo cr0,f2,f1       // compare original.x >= request.x
494: lfs f3,8(r27)         // f3 = request.z
498: lfs f6,84(r1)         // f6 = original.z
49c: fsubs f1,f1,f2        // f1 = dx
4a4: fsubs f2,f3,f6        // f2 = dz
4b0: fcmpo cr0,f6,f3       // compare original.z >= request.z
4c0: stfs f3,0(r3)         // v0.x = reqC
4cc: stfs f3,8(r3)         // v0.z = -req10
4d8: stfs f3,12(r3)        // v1.x = dx + reqC
4e4: stfs f3,20(r3)        // v1.z = dz - req10
4f0: stfs f3,24(r3)        // v2.x = dx - reqC
4fc: stfs f3,32(r3)        // v2.z = dz - req10
508: stfs f1,36(r3)        // v3.x = dx - reqC
514: stfs f1,44(r3)        // v3.z = dz + req10
520: stfs f1,48(r3)        // v4.x = -reqC
528: stfs f1,56(r3)        // v4.z = req10
568: stfs f23,4(r3)        // v0.y = 0.0f
56c: stfs f23,16(r3)       // v1.y = 0.0f
570: stfs f23,28(r3)       // v2.y = 0.0f
574: stfs f23,40(r3)       // v3.y = 0.0f
578: stfs f23,52(r3)       // v4.y = 0.0f
580: stw r0,100(r29)       // quad->unk64 = base
```

Conclusion: the square points are not the current value divergence.

## drawShadowVolume Side-Wall Divergence

Retail starts the side-wall primitive with exactly 60 vertices:

```asm
2b0c: li r3,144            // GX_TRIANGLES
2b10: li r4,0              // GX_VTXFMT0
2b14: li r5,60             // 5 edges * 12 vertices
2b18: bl GXBegin           // begin side-wall triangles
2b1c: lwz r4,100(r31)      // r4 = base = quad->unk64
2b20: lis r3,-13311        // r3 = FIFO base 0xCC010000
2b24: lfs f4,@4014         // f4 = 50.0f
```

Retail first edge, where `a = v0` and `b = v1`:

```asm
2b28: lfs f0,4(r4)         // f0 = a.y
2b2c: lfs f6,8(r4)         // f6 = a.z
2b30: lfs f3,0(r4)         // f3 = a.x
2b34: fadds f5,f0,f4       // f5 = a.y + 50 = aT.y
2b38: fsubs f10,f0,f4      // f10 = a.y - 50 = aB.y
2b3c: stfs f3,FIFO         // vertex 01 x = a.x
2b40: stfs f5,FIFO         // vertex 01 y = aT.y
2b44: stfs f6,FIFO         // vertex 01 z = a.z       => aT
2b48: lfs f0,16(r4)        // f0 = b.y
2b4c: lfs f8,20(r4)        // f8 = b.z
2b50: lfs f2,12(r4)        // f2 = b.x
2b54: fadds f7,f0,f4       // f7 = b.y + 50 = bT.y
2b58: fsubs f9,f0,f4       // f9 = b.y - 50 = bB.y
2b5c: stfs f2,FIFO         // vertex 02 x = b.x
2b60: stfs f7,FIFO         // vertex 02 y = bT.y
2b64: stfs f8,FIFO         // vertex 02 z = b.z       => bT
2b68: stfs f2,FIFO         // vertex 03 x = b.x
2b6c: stfs f9,FIFO         // vertex 03 y = bB.y
2b70: stfs f8,FIFO         // vertex 03 z = b.z       => bB
2b74: stfs f2,FIFO         // vertex 04 x = b.x
2b78: stfs f9,FIFO         // vertex 04 y = bB.y
2b7c: stfs f8,FIFO         // vertex 04 z = b.z       => bB
2b80: stfs f3,FIFO         // vertex 05 x = a.x
2b84: stfs f10,FIFO        // vertex 05 y = aB.y
2b88: stfs f6,FIFO         // vertex 05 z = a.z       => aB
2b8c: stfs f3,FIFO         // vertex 06 x = a.x
2b90: stfs f5,FIFO         // vertex 06 y = aT.y
2b94: stfs f6,FIFO         // vertex 06 z = a.z       => aT
2b98: stfs f2,FIFO         // vertex 07 x = b.x
2b9c: stfs f7,FIFO         // vertex 07 y = bT.y
2ba0: stfs f8,FIFO         // vertex 07 z = b.z       => bT
2ba4: stfs f3,FIFO         // vertex 08 x = a.x
2ba8: stfs f5,FIFO         // vertex 08 y = aT.y
2bac: stfs f6,FIFO         // vertex 08 z = a.z       => aT
2bb0: stfs f2,FIFO         // vertex 09 x = b.x
2bb4: stfs f9,FIFO         // vertex 09 y = bB.y
2bb8: stfs f8,FIFO         // vertex 09 z = b.z       => bB
2bbc: stfs f2,FIFO         // vertex 10 x = b.x
2bc0: stfs f9,FIFO         // vertex 10 y = bB.y
2bc4: stfs f8,FIFO         // vertex 10 z = b.z       => bB
2bc8: stfs f3,FIFO         // vertex 11 x = a.x
2bcc: stfs f5,FIFO         // vertex 11 y = aT.y
2bd0: stfs f6,FIFO         // vertex 11 z = a.z       => aT
2bd4: stfs f3,FIFO         // vertex 12 x = a.x
2bd8: stfs f10,FIFO        // vertex 12 y = aB.y
2bdc: stfs f6,FIFO         // vertex 12 z = a.z       => aB
```

Retail side-wall order for each edge is therefore:

```text
triangle 1: aT, bT, bB
triangle 2: bB, aB, aT
triangle 3: bT, aT, bB
triangle 4: bB, aT, aB
```

Pre-fix source build side loop:

```asm
1090: li r0,5              // loop count = 5 edges
1094: lfs f3,@1134         // f3 = 50.0f
1098: lis r3,0x6666        // magic division by 5 for modulo
109c: mtctr r0             // CTR = 5
10a0: addi r6,r3,0x6667    // r6 = 0x66666667
10a4: li r9,0              // i = 0
10a8: li r3,0              // byte offset = i * sizeof(Vec)
10b0: addi r7,r9,1         // r7 = i + 1
10b4: lwz r8,100(r30)      // r8 = base = quad->unk64
10b8: mulhw r0,r6,r7       // q-temp for signed divide by 5
10bc: srawi r0,r0,1        // q-temp >>= 1
10c0: add r10,r8,r3        // r10 = &base[i] = a
10c4: lfs f0,4(r10)        // f0 = a.y
10c8: srwi r5,r0,31        // sign adjust
10cc: add r0,r0,r5         // q = floor((i + 1) / 5)
10d0: lfs f5,8(r10)        // f5 = a.z
10d4: mulli r0,r0,5        // q * 5
10d8: lfs f2,0(r10)        // f2 = a.x
10dc: fadds f4,f3,f0       // f4 = a.y + 50 = aT.y
10e0: fsubs f8,f0,f3       // f8 = a.y - 50 = aB.y
10e4: stfs f2,FIFO         // vertex 01 x = a.x
10e8: subf r0,r0,r7        // r0 = (i + 1) % 5
10ec: stfs f4,FIFO         // vertex 01 y = aT.y
10f0: mulli r0,r0,12       // next byte offset
10f4: stfs f5,FIFO         // vertex 01 z = a.z       => aT
10f8: add r5,r8,r0         // r5 = &base[(i + 1) % 5] = b
10fc: lfs f1,4(r5)         // f1 = b.y
1100: addi r9,r9,1         // ++i
1104: lfs f7,8(r5)         // f7 = b.z
1108: addi r3,r3,12        // next a byte offset
110c: lfs f0,0(r5)         // f0 = b.x
1110: fadds f6,f3,f1       // f6 = b.y + 50 = bT.y
1114: stfs f0,FIFO         // vertex 02 x = b.x
1118: fsubs f1,f1,f3       // f1 = b.y - 50 = bB.y
111c: stfs f6,FIFO         // vertex 02 y = bT.y
1120: stfs f7,FIFO         // vertex 02 z = b.z       => bT
1124: stfs f0,FIFO         // vertex 03 x = b.x
1128: stfs f1,FIFO         // vertex 03 y = bB.y
112c: stfs f7,FIFO         // vertex 03 z = b.z       => bB
1130: stfs f2,FIFO         // vertex 04 x = a.x
1134: stfs f4,FIFO         // vertex 04 y = aT.y      => WRONG: retail wants bB first
1138: stfs f5,FIFO         // vertex 04 z = a.z       => current vertex 04 = aT
113c: stfs f0,FIFO         // vertex 05 x = b.x
1140: stfs f1,FIFO         // vertex 05 y = bB.y      => current vertex 05 = bB
1144: stfs f7,FIFO         // vertex 05 z = b.z
1148: stfs f2,FIFO         // vertex 06 x = a.x
114c: stfs f8,FIFO         // vertex 06 y = aB.y      => current vertex 06 = aB
1150: stfs f5,FIFO         // vertex 06 z = a.z
1154: stfs f0,FIFO         // vertex 07 x = b.x
1158: stfs f6,FIFO         // vertex 07 y = bT.y
115c: stfs f7,FIFO         // vertex 07 z = b.z       => bT
1160: stfs f2,FIFO         // vertex 08 x = a.x
1164: stfs f4,FIFO         // vertex 08 y = aT.y
1168: stfs f5,FIFO         // vertex 08 z = a.z       => aT
116c: stfs f0,FIFO         // vertex 09 x = b.x
1170: stfs f1,FIFO         // vertex 09 y = bB.y
1174: stfs f7,FIFO         // vertex 09 z = b.z       => bB
1178: stfs f0,FIFO         // vertex 10 x = b.x
117c: stfs f1,FIFO         // vertex 10 y = bB.y
1180: stfs f7,FIFO         // vertex 10 z = b.z       => bB
1184: stfs f2,FIFO         // vertex 11 x = a.x
1188: stfs f4,FIFO         // vertex 11 y = aT.y
118c: stfs f5,FIFO         // vertex 11 z = a.z       => aT
1190: stfs f2,FIFO         // vertex 12 x = a.x
1194: stfs f8,FIFO         // vertex 12 y = aB.y
1198: stfs f5,FIFO         // vertex 12 z = a.z       => aB
119c: bdnz 10b0            // next edge
```

Pre-fix side-wall order:

```text
triangle 1: aT, bT, bB
triangle 2: aT, bB, aB   <-- diverges at vertices 04-06
triangle 3: bT, aT, bB
triangle 4: bB, aT, aB
```

## Root Divergence Found

The root divergence is not the cap index arrays and not the square vertex
coordinates. It is the second side-wall triangle in `drawShadowVolume()`.

Retail emits triangle 2:

```text
bB, aB, aT
```

Pre-fix source emitted triangle 2:

```text
aT, bB, aB
```

For normal color geometry this can look like the same triangle with a different
starting vertex, but in this pass the winding/culling interaction matters. The
source should match the retail FIFO order exactly.

## Post-Fix Verification

After reordering the second triangle in `src/MarioUtil/ShadowUtil.cpp`, the
current source object now emits the corrected FIFO order for vertices 04-06:

```asm
1130: stfs f0,FIFO         // vertex 04 x = b.x
1134: stfs f1,FIFO         // vertex 04 y = bB.y
1138: stfs f7,FIFO         // vertex 04 z = b.z       => bB
113c: stfs f2,FIFO         // vertex 05 x = a.x
1140: stfs f8,FIFO         // vertex 05 y = aB.y
1144: stfs f5,FIFO         // vertex 05 z = a.z       => aB
1148: stfs f2,FIFO         // vertex 06 x = a.x
114c: stfs f4,FIFO         // vertex 06 y = aT.y
1150: stfs f5,FIFO         // vertex 06 z = a.z       => aT
```

Post-fix side-wall order:

```text
triangle 1: aT, bT, bB
triangle 2: bB, aB, aT
triangle 3: bT, aT, bB
triangle 4: bB, aT, aB
```
