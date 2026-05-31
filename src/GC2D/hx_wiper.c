#include <GC2D/hx_wiper.h>
#include <dolphin/gx.h>
#include <dolphin/dvd.h>
#include <dolphin/os/OSCache.h>

void ReInitializeGX();

// Shared work struct for the screen-wipe ("Hx") effect system.
// Derived from offset accesses across all functions (see notes/hx_wiper.md).
typedef struct HxWork {
	/* 0x00 */ u32 imgW;
	/* 0x04 */ u32 imgH;
	/* 0x08 */ u32 imgWHalf;
	/* 0x0C */ u32 imgHHalf;
	/* 0x10 */ u8 state;
	/* 0x11 */ u8 wipeNo;
	/* 0x12 */ u8 type;
	/* 0x13 */ u8 pad13;
	/* 0x14 */ f32 timer;
	/* 0x18 */ f32 speed;
	/* 0x1C */ u32 unk1C;
	/* 0x20 */ void (*handler)();
	/* 0x24 */ u32 resFlag;
	/* 0x28 */ u32 unk28;
	/* 0x2C */ void* buffer;
	/* 0x30 */ void* resource;
	/* 0x34 */ u32 bufSize;
	/* 0x38 */ u32 unk38;
	/* 0x3C */ u32 unk3C;
	/* 0x40 */ u8 rest[0x64 - 0x40];
} HxWork;

// Texture resource header (.bti / ResTIMG-style); only the fields used here.
typedef struct HxTexRes {
	/* 0x00 */ u8 format;
	/* 0x01 */ u8 pad01;
	/* 0x02 */ u16 width;
	/* 0x04 */ u16 height;
	/* 0x06 */ u8 wrapS;
	/* 0x07 */ u8 wrapT;
	/* 0x08 */ u8 pad08[0x14 - 0x08];
	/* 0x14 */ u8 minFilter;
	/* 0x15 */ u8 magFilter;
	/* 0x16 */ u8 pad16[0x1C - 0x16];
	/* 0x1C */ u32 imageOffset;
} HxTexRes;

// 9-float motion descriptor used by Hx_MotionSet / Hx_MotionUpdate.
typedef struct HxMotion {
	/* 0x00 */ f32 unk00;
	/* 0x04 */ f32 unk04;
	/* 0x08 */ f32 unk08;
	/* 0x0C */ f32 unk0C;
	/* 0x10 */ f32 unk10;
	/* 0x14 */ f32 unk14;
	/* 0x18 */ f32 unk18;
	/* 0x1C */ f32 unk1C;
	/* 0x20 */ f32 unk20;
} HxMotion;

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
static HxWork hx;
static u8 hx_buffer[0x3300];

static int hxs_logo_resetflag;
static int hxs_logodraw_resetflag;

static u16 img_wx;
static u16 img_wy;

// forward declarations (handlers referenced by handle_table / Hx_UpdateWipe)
static void Hx_Test5();
static void Hx_Test4();
static void Hx_Test2R();
static void Hx_Test2();
static void Hx_Test1();
static void Hx_Logo();
static void Hx_GameOver();
static void Hx_Door();
static void Hx_Circle();
static void Hx_Warning(int code);
static void Hx_CameraInit();
static void Hx_GxInit(int, int);
static void Frb2_InitBlackBox();
static void Frb2_RendBox(u32 color, f32 x0, f32 y0, f32 x1, f32 y1);
static void dummy_handler();

static void (*const handle_table[15])() = {
	dummy_handler, Hx_Circle, Hx_Circle, Hx_Test1, Hx_Test1,
	Hx_Test5, Hx_Test5, Hx_Test4, Hx_Test4, Hx_Test2R,
	Hx_Test2, Hx_Door, Hx_Logo, Hx_GameOver, dummy_handler,
};

static const u8 handle_type[15] = {
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0,
};

// ---------------------------------------------------------------------------
// Wipe-effect handlers (GX-heavy; reconstruction pending - see notes/hx_wiper.md)
// ---------------------------------------------------------------------------
static void Hx_Test5() {}
static void Hx_Test4() {}
static void Hx_Test2R() {}
static void Hx_Test2() {}
static void Hx_Test1() {}

int Hx_MovieStartSyncEx() {
	if (hx.wipeNo != 12)
		return 0;

	if (hx.unk38 >= 2 && hx.unk38 <= 5) {
		if (hxs_logodraw_resetflag == 0)
			return 0;
		hxs_logodraw_resetflag = 0;
		return 1;
	}

	if (hx.unk38 >= 6) {
		if (hxs_logo_resetflag == 0)
			return 0;
		if (hx.unk38 == 6 && hx.unk3C > 0xC0)
			return 0;
		hxs_logo_resetflag = 0;
		return 2;
	}

	return 0;
}

static void Hx_Logo() {}
static void Hx_GameOver() {}
static void Hx_Door() {}
static void Hx_Circle() {}

// ---------------------------------------------------------------------------
// Motion solver
// ---------------------------------------------------------------------------
f32 Hx_MotionUpdate(HxMotion* m) {
	if (m->unk00 > m->unk1C) {
		m->unk18 += m->unk0C;
	} else if (m->unk04 <= m->unk1C) {
		m->unk18 += m->unk14;
	}
	m->unk1C += 1.0f;
	m->unk20 += m->unk18;
	return m->unk20;
}

void Hx_MotionSet(HxMotion* m, f32 dist, f32 t1, f32 t2, f32 t3) {
	f32 v;
	m->unk00 = t1;
	m->unk04 = t1 + t2;
	m->unk08 = m->unk04 + t3;
	v = 2.0f * dist / (t3 + (t1 + 2.0f * t2));
	if (t1 != 0.0f)
		m->unk0C = v / t1;
	if (t3 != 0.0f)
		m->unk14 = -v / t3;
	m->unk10 = 0.0f;
	m->unk18 = 0.0f;
	m->unk20 = 0.0f;
	m->unk1C = 0.0f;
}

u32 Hx_TimerCountDown() {
	if (hx.unk3C != 0)
		hx.unk3C--;
	return hx.unk3C;
}

// ---------------------------------------------------------------------------
// Wipe driver
// ---------------------------------------------------------------------------
u32 Hx_UpdateWipe(f32 step) {
	ReInitializeGX();

	switch (hx.state) {
	case 1:
		if (hx.type != 1) {
			Hx_CameraInit();
			Hx_GxInit(0, 0);
			Frb2_InitBlackBox();
			Frb2_RendBox(0xFF, 0.0f, 0.0f, (f32)hx.imgW, (f32)hx.imgH);
		}
		break;
	case 3:
		hx.handler = handle_table[hx.wipeNo];
		hx.type = handle_type[hx.wipeNo];
		hx.state = 2;
		hx.unk38 = 0;
		/* fallthrough */
	case 2:
		hx.speed = step;
		GXDrawDone();
		hx.handler();
		GXDrawDone();
		hx.timer += step;
		break;
	}

	return hx.state;
}

int Hx_GetWipeType(int no) {
	return handle_type[no];
}

static void dummy_handler() {}

void Hx_StartWipe(int no, int arg) {
	if (hx.resFlag == 0) {
		hx.buffer = hx_buffer;
		hx.bufSize = 0x3300;
	}
	if (hx.state == 2)
		Hx_Warning(1);
	hx.state = 1;
	hx.wipeNo = no;
	hx.timer = 0.0f;
	hx.unk28 = arg;
}

void Hx_RemoveResource() {
	if (hx.state == 2)
		Hx_Warning(1);
	if (hx.resFlag == 0)
		Hx_Warning(2);
	hx.resFlag = 0;
	hx.unk28 = 0;
}

void Hx_ProvideResourceEx(void* res) {
	if (hx.state == 2)
		Hx_Warning(1);
	hx.unk28 = 1;
	hx.resource = res;
}

void Hx_ProvideResource(void* res, int size) {
	if (hx.state == 2)
		Hx_Warning(1);
	if (hx.resFlag != 0)
		Hx_Warning(3);
	hx.resFlag = 1;
	hx.buffer = res;
	hx.bufSize = size;
}

void Hx_ResetWipe(u32 w, u32 h) {
	hx.state = 0;
	hx.imgW = w;
	hx.imgH = h;
	hx.imgWHalf = hx.imgW >> 1;
	hx.imgHHalf = hx.imgH >> 1;
	hx.resFlag = 0;
	hx.unk28 = 0;
}

static void Hx_Warning(int code) {}

// ---------------------------------------------------------------------------
// GX helpers
// ---------------------------------------------------------------------------
static void Frb2_RendBox(u32 color, f32 x0, f32 y0, f32 x1, f32 y1) {
	GXBegin(GX_QUADS, GX_VTXFMT0, 4);
	GXPosition3f32(x0, y0, 0.0f);
	GXColor1u32(color);
	GXPosition3f32(x1, y0, 0.0f);
	GXColor1u32(color);
	GXPosition3f32(x1, y1, 0.0f);
	GXColor1u32(color);
	GXPosition3f32(x0, y1, 0.0f);
	GXColor1u32(color);
}

static void Frb2_InitBlackBox() {
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
}

static void Hgx_ReadTexture(char* fileName, void* addr) {
	DVDFileInfo fi;
	if (hx.resFlag == 0) {
		if (DVDOpen(fileName, &fi)) {
			long len = DVDReadPrio(&fi, addr, fi.length, 0, 2);
			DVDClose(&fi);
			DCStoreRange(addr, len);
		}
	}
}

static void Hgx_init_tobj_resource(GXTexObj* obj, HxTexRes* res) {
	void* image = (u8*)res + res->imageOffset;
	img_wx = res->width;
	img_wy = res->height;
	GXInitTexObj(obj, image, img_wx, img_wy, res->format, res->wrapS, res->wrapT,
	             GX_FALSE);
	GXInitTexObjLOD(obj, res->minFilter, res->magFilter, 0.0f, 0.0f, 0.0f,
	                GX_FALSE, GX_FALSE, GX_ANISO_1);
}

static void Hx_CameraInit() {}

static void Hx_GxInit(int mode, int blend) {
	switch (mode) {
	case 0:
		GXSetNumTexGens(0);
		GXSetNumTevStages(1);
		GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		break;
	case 1:
		GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY,
		                  GX_FALSE, GX_PTIDENTITY);
		GXSetNumTexGens(1);
		GXSetNumTevStages(1);
		GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
		GXClearVtxDesc();
		GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
		GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
		GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
		GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
		GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
		GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
		break;
	}
	switch (blend) {
	case 0:
		GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR);
		break;
	case 1:
		GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
		break;
	}
}
