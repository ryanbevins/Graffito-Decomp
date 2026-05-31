export const meta = {
  name: 'hx-wiper-decode',
  description: 'Decode remaining GX-heavy hx_wiper functions into functional C, grouped by static-sharing family',
  phases: [{ title: 'Decode' }],
}

const CONTRACT = `
You are decoding functions from the original PowerPC assembly of Super Mario
Sunshine's screen-wipe TU into functional C, for a matching decompilation
(MWCC 1.2.5, GameCube). The source file is compiled as **C** (-lang=c), NOT C++.

ASM FILE: build/GMSJ01/asm/GC2D/hx_wiper.s  (read your function's line range with the Read tool)

The bar is FUNCTIONAL CORRECTNESS, not byte-perfect match. Read the asm carefully,
write C that does the same thing. Use real GX API names (look them up in
include/dolphin/gx/*.h via Grep — e.g. GXBegin, GXPosition3f32, GXColor1u32,
GXTexCoord2f32, GXSetVtxDesc, GXSetVtxAttrFmt, GXInitTexObj, C_MTXOrtho, etc).
Matrix/vector types: Mtx (f32[3][4]), Mtx44 (f32[4][4]), Vec/Point3d ({f32 x,y,z}) from
<dolphin/mtx.h> / <dolphin/vec.h>. GXColor is {u8 r,g,b,a}.

SHARED WORK STRUCT (already defined in the .c; reference fields by these names):
  typedef struct HxWork {
    u32 imgW; u32 imgH; u32 imgWHalf; u32 imgHHalf;        // 0x00,04,08,0C
    u8 state; u8 wipeNo; u8 type; u8 pad13;                // 0x10,11,12,13
    f32 timer; f32 speed; u32 unk1C;                       // 0x14,18,1C
    void (*handler)(); u32 resFlag; u32 unk28;             // 0x20,24,28
    void* buffer; void* resource; u32 bufSize;             // 0x2C,30,34
    u32 unk38; u32 unk3C; u8 rest[0x24];                   // 0x38,3C,40..63
  } HxWork;
  static HxWork hx;          // the global instance
  static u8 hx_buffer[0x3300];
Motion struct (9 floats): HxMotion { f32 unk00..unk20 } at 0x00..0x20.

GX FIFO immediate-write idiom: a sequence "stfs fA,-0x8000(r=0xcc01..); stfs fB; stfs fC; stw r"
where r3=lis 0xcc01 is GXPosition3f32(A,B,C) then GXColor1u32(color). 3 floats+1 word per vertex
inside a GXBegin(prim,fmt,nverts)... block. "lhz"/"sth" pairs into the FIFO are GXPosition2u16/
GXTexCoord2u16 etc. Match the GXBegin primitive/count from the li r3 before "bl GXBegin".

The "@NNN"@sda21 symbols are float/double constants (look at the .obj "@NNN" definition near
the bottom of the .s for the value). The "$NNN" suffixed .obj symbols (e.g. r$181, boundtable$262,
camLoc$96) are FILE-STATIC locals/data tables belonging to specific functions — declare them as
static C variables/arrays with the right initializer (read the .obj data in the .s).

OUTPUT: For each function in your group, return complete C source (signature + body).
Also return a deduplicated list of any file-static variable/array/const declarations your
functions need (with initializers, read from the .s data section). Be precise about types.
`

const SCHEMA = {
  type: 'object',
  additionalProperties: false,
  required: ['functions', 'statics', 'notes'],
  properties: {
    functions: {
      type: 'array',
      items: {
        type: 'object', additionalProperties: false,
        required: ['name', 'code'],
        properties: {
          name: { type: 'string' },
          code: { type: 'string', description: 'complete C function: signature + body' },
        },
      },
    },
    statics: {
      type: 'array',
      items: { type: 'string', description: 'a C file-static declaration with initializer' },
    },
    notes: { type: 'string', description: 'confidence, uncertainties, anything the integrator must know' },
  },
}

const GROUPS = [
  { label: 'cam+gx-setup', fns: 'Hx_CameraInit (asm line 5361), Hx_GxInit (line 5261), Frb2_InitGx (line 4666)' },
  { label: 'texcopy', fns: 'Hx_GetFrBuffer (line 5129), Hgx_ReadTexture (line 5177), Hgx_init_tobj_resource (line 5217)' },
  { label: 'vfilter', fns: 'Hx_SetVFilter (line 4929) — has an unrolled loop over a copy-filter vtable using mulhwu reciprocal-divide-by-3; reconstruct the LOGICAL loop (a simple for-loop with /3 and %3), do not unroll' },
  { label: 'frbuffermorf', fns: 'Hx_FrBufferMorf (line 4750), __Hx_FrBufferMorf (line 4769)' },
  { label: 'test1', fns: 'Hx_Test1 (line 1482), Hxs1_Test1 (line 1338)' },
  { label: 'test2', fns: 'Hx_Test2 (line 1074), Hx_Test2R (line 795), Hxs1_Test2 (line 597)' },
  { label: 'test45', fns: 'Hx_Test4 (line 350), Hx_Test5 (line 9)' },
  { label: 'logo', fns: 'Hx_Logo (line 1648), Hxs_PenDraw (line 1924), Hxs_Logo_MagDraw (line 2050), Hxs_Logo_TexDraw (line 2160), Hxs_Logo_TexSetup (line 2305), Hxs_Logo_ExtraDraw (line 2411). NOTE: Hx_Logo has a jump-table @943 (9 entries) over hx.unk38 — model as a switch on hx.unk38.' },
  { label: 'gameover', fns: 'Hx_GameOver (line 2513), Hxs_GameOver (line 2795)' },
  { label: 'door', fns: 'Hx_Door (line 3142), Hxs_FrBufferMorf2 (line 3411), Hxs_FrBufferMorf2B (line 3273)' },
  { label: 'circle', fns: 'Hx_Circle (line 4030), Hxs1_Circle (line 3825), Hxs2_Circle (line 3571). NOTE: uses boundtable$262 data table.' },
]

phase('Decode')
const results = await parallel(GROUPS.map(g => () =>
  agent(
    CONTRACT +
    `\n\nYOUR GROUP: ${g.label}\nDecode these functions: ${g.fns}\n` +
    `Read the exact line ranges from build/GMSJ01/asm/GC2D/hx_wiper.s. ` +
    `Return functional C for each, plus the file-static declarations they need.`,
    { label: `decode:${g.label}`, phase: 'Decode', schema: SCHEMA }
  ).then(r => ({ group: g.label, ...r }))
))

return results.filter(Boolean)
