#include <GC2D/hx_wiper.h>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <dolphin/dvd.h>
#include <dolphin/os/OSCache.h>
#include <math.h>

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
static u8 hx_buffer[0x3300] __attribute__((aligned(32)));

static int hxs_logo_resetflag;
static int hxs_logodraw_resetflag;

static u16 img_wx;
static u16 img_wy;

static const u8 vtable_org[7] = { 0x10, 0x10, 0, 0, 0, 0x10, 0x10 };
static u8 vtable[7];
static const u8 dec_step[4] = { 0, 1, 5, 6 };
static const u8 inc_step[3] = { 2, 3, 4 };
static void* fbuf = hx_buffer;
static void* fbuf2 = hx_buffer;

// forward declarations (handlers referenced by handle_table / Hx_UpdateWipe)
static void Hx_Test5();
static void Hx_Test4();
static void Hxs1_Test2(u32, u32, f32, f32, f32, f32);
static void Hx_Test2R();
static void Hx_Test2();
static void Hxs1_Test1(f32, f32, f32);
static void Hx_Test1();
static void Hx_Logo();
static void Hx_GameOver();
static void Hx_Door();
static void Hxs_FrBufferMorf2B(f32);
static void Hxs_FrBufferMorf2(f32);
static void Hx_Circle();
static void Hx_Warning(int code);
static void Hx_CameraInit();
static void Hx_GxInit(int, int);
f32 Hx_MotionUpdate(HxMotion*);
void Hx_MotionSet(HxMotion*, f32, f32, f32, f32);
u32 Hx_TimerCountDown();
static void Frb2_InitBlackBox();
static void Frb2_RendBox(u32 color, f32 x0, f32 y0, f32 x1, f32 y1);
static void Frb2_InitGx(GXTexObj* tobj);
static void Hx_SetVFilter(f32 ratio);
static void __Hx_FrBufferMorf(u32 x, u32 y);
static void Hx_GetFrBuffer(void* dest, u32 left, u32 top, u32 wd, u32 ht);
static void dummy_handler();

// Camera vectors (mutable: Hx_CameraInit overwrites .x/.y with screen center).
static Vec camLoc = { 320.0f, 240.0f, -30.0f };
static Vec objPt = { 320.0f, 240.0f, 0.0f };
static Vec up = { 0.0f, -10.0f, 0.0f };

static void (*const handle_table[15])() = {
	dummy_handler, Hx_Circle, Hx_Circle, Hx_Test1, Hx_Test1,
	Hx_Test5, Hx_Test5, Hx_Test4, Hx_Test4, Hx_Test2R,
	Hx_Test2, Hx_Door, Hx_Logo, Hx_GameOver, dummy_handler,
};

static const u8 handle_type[15] = {
	0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0,
};

static f32 r_393;
static f32 r_416;
static f32 r_432;
static f32 thin;
static u32 rstep;
static f32 thin_d;
static f32 rstep_d;

// ---------------------------------------------------------------------------
// Wipe-effect handlers (GX-heavy; reconstruction pending - see notes/hx_wiper.md)
// ---------------------------------------------------------------------------
static void Hx_Test5() {
	GXTexObj tobj;
	u32 y;
	u32 x;

	Hx_CameraInit();
	Hx_GxInit(1, 0);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	switch (hx.unk38) {
	case 0:
		hx.unk3C = 20;
		hx.unk38++;
		/* fallthrough */
	case 1: {
		f32 t;
		f32 ratioA;
		f32 ratioB;

		GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0,
		                  GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
		GXSetNumTexGens(1);
		GXSetNumTevStages(1);
		GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
		GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0,
		              GX_COLOR_NULL);
		GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR);
		GXInitTexObj(&tobj, hx_buffer, 0x40, 0x40, GX_TF_RGB565, GX_CLAMP,
		             GX_CLAMP, GX_FALSE);
		GXInitTexObjLOD(&tobj, GX_LINEAR, GX_LINEAR, 0.0f, 10.0f, 0.0f,
		                GX_FALSE, GX_TRUE, GX_ANISO_1);

		t = (f32)hx.unk3C / 20.0f;
		ratioA = 1.41f - 1.41f * t;
		ratioB = 0.1f + 1.41f * t;

		y = 0;
		while (y < hx.imgH) {
			x = 0;
			while (x < hx.imgW) {
				u32 i;
				f32 xf;
				f32 yf;
				f32 ratio;
				f32 phase;
				f32 cx;
				f32 cy;
				f32 firstX;
				f32 firstY;
				f32 firstS;
				f32 firstT;

				xf = (f32)x;
				yf = (f32)y;
				Hx_GetFrBuffer(hx_buffer, x, y, 0x40, 0x40);
				GXInvalidateTexAll();
				GXLoadTexObj(&tobj, GX_TEXMAP0);

				if (hx.type != 0)
					ratio = ratioA;
				else
					ratio = ratioB;

				if (ratio < 1.0f)
					phase = 3.1415927f * (1.0f - ratio);
				else
					phase = 0.0f;

				cx = xf + 32.0f;
				cy = yf + 32.0f;
				GXBegin(0xA0, GX_VTXFMT0, 0x12);
				GXPosition3f32(cx, cy, 0.0f);
				GXColor1u32(0);
				GXTexCoord2f32(0.5f, 0.5f);

				i = 0;
				while (i < 0x10) {
					f32 angle;
					f32 s;
					f32 c;
					f32 px;
					f32 py;
					f32 tx;
					f32 ty;

					angle = 3.1415927f * (2.0f * (f32)i) * 0.0625f;
					s = sinf(angle);
					c = cosf(angle);
					tx = 0.5f * s + 0.5f;
					ty = 0.5f * c + 0.5f;

					px = ratio * sinf(angle + phase);
					py = ratio * cosf(angle + phase);

					if (ratio <= 1.0f) {
						tx = ratio * sinf(angle) * 0.5f + 0.5f;
						ty = ratio * cosf(angle) * 0.5f + 0.5f;
					}

					if (px < -1.0f) {
						py = -py / px;
						px = -1.0f;
						tx = 0.0f;
						ty = 0.5f * py + 0.5f;
					}
					if (px > 1.0f) {
						py = py / px;
						px = 1.0f;
						tx = 1.0f;
						ty = 0.5f * py + 0.5f;
					}
					if (py < -1.0f) {
						px = -px / py;
						py = -1.0f;
						ty = 0.0f;
						tx = 0.5f * px + 0.5f;
					}
					if (py > 1.0f) {
						px = px / py;
						py = 1.0f;
						ty = 1.0f;
						tx = 0.5f * px + 0.5f;
					}

					px *= 32.0f;
					py *= 32.0f;
					if (i == 0) {
						firstX = px;
						firstY = py;
						firstS = tx;
						firstT = ty;
					}

					GXPosition3f32(cx + px, cy + py, 0.0f);
					GXColor1u32(0);
					GXTexCoord2f32(tx, ty);
					i++;
				}

				GXPosition3f32(cx + firstX, cy + firstY, 0.0f);
				GXColor1u32(0);
				GXTexCoord2f32(firstS, firstT);

				x += 0x40;
			}
			y += 0x40;
		}

		if (Hx_TimerCountDown() == 0) {
			hx.unk38++;
			hx.state = 3;
		}
		break;
	}
	default:
		hx.state = 3;
		break;
	}
}
static void Hx_Test4() {
	u32 i;
	f32 center;
	f32 outer;
	f32 inner;
	f32 angle;
	f32 prevOuterX;
	f32 prevOuterY;
	f32 prevInnerX;
	f32 prevInnerY;
	f32 z;
	u32 color;

	switch (hx.unk38) {
	case 0:
		switch (hx.type) {
		case 0:
			rstep = 0;
			thin = 124.3f;
			rstep_d = 5.0f;
			thin_d = 0.15f;
			break;
		case 1:
			rstep = 230;
			thin = 100.0f;
			rstep_d = -5.0f;
			thin_d = -0.15f;
			break;
		}
		hx.unk3C = 38;
		hx.unk38++;
		/* fallthrough */
	case 1:
		rstep = (u32)((f32)rstep + rstep_d);
		thin += thin_d;
		center = (f32)((hx.imgW >> 1) + 200);
		outer = center + thin;
		angle = 0.0f;
		prevOuterX = outer * sinf(angle) + (f32)hx.imgWHalf;
		prevOuterY = outer * cosf(angle) + (f32)hx.imgHHalf;
		inner = center - thin;
		prevInnerX = inner * sinf(angle) + (f32)hx.imgWHalf;
		prevInnerY = inner * cosf(angle) + (f32)hx.imgHHalf;

		Hx_CameraInit();
		Hx_GxInit(0, 1);

		i = 0;
		z = 0.0f;
		color = 0xff;
		while (i < rstep) {
			f32 curOuterX;
			f32 curOuterY;
			f32 curInnerX;
			f32 curInnerY;

			center -= 2.4f;
			outer = center + thin;
			if (center < thin)
				inner = 0.0f;
			else
				inner = center - thin;

			angle += 0.12f;
			curOuterX = outer * sinf(angle) + (f32)hx.imgWHalf;
			curOuterY = outer * cosf(angle) + (f32)hx.imgHHalf;
			curInnerX = inner * sinf(angle) + (f32)hx.imgWHalf;
			curInnerY = inner * cosf(angle) + (f32)hx.imgHHalf;

			GXBegin(GX_QUADS, GX_VTXFMT0, 4);
			GXPosition3f32(prevOuterX, prevOuterY, z);
			GXColor1u32(color);
			GXPosition3f32(curOuterX, curOuterY, z);
			GXColor1u32(color);
			GXPosition3f32(curInnerX, curInnerY, z);
			GXColor1u32(color);
			GXPosition3f32(prevInnerX, prevInnerY, z);
			GXColor1u32(color);

			prevOuterX = curOuterX;
			prevOuterY = curOuterY;
			prevInnerX = curInnerX;
			prevInnerY = curInnerY;
			i++;
		}

		if (Hx_TimerCountDown() == 0) {
			hx.state = 3;
			hx.unk38++;
		}
		break;
	}
}

static void Hxs1_Test2(u32 count, u32 side, f32 x, f32 y, f32 r1, f32 r2) {
	s32 pos;
	s32 end;
	s32 step;
	f32 r1sq;
	f32 r2sq;
	u32 color;
	f32 z;

	Hx_CameraInit();
	Hx_GxInit(0, 1);

	r1sq = r1 * r1;
	r2sq = r2 * r2;

	if (side == 0) {
		step = 1;
		end = (s32)r1;
		pos = (s32)-y;
	} else {
		step = -1;
		pos = (s32)r1;
		end = (s32)-y;
	}

	color = 0xff;
	z = 1.0f;
	while (pos != end) {
		f32 lineY;

		lineY = y + (f32)pos;
		if (lineY >= 0.0f && lineY <= (f32)hx.imgH) {
			s32 sqi;
			f32 root1;
			f32 root2;
			volatile f32 rootOut;
			f32 x0;
			f32 x1;
			BOOL draw;

			if (count == 0)
				break;
			count--;

			sqi = pos * pos;
			root1 = r1sq - (f32)sqi;
			if (root1 > 0.0f) {
				f64 guess = __frsqrte(root1);
				guess = 0.5 * guess * (3.0 - root1 * guess * guess);
				guess = 0.5 * guess * (3.0 - root1 * guess * guess);
				guess = 0.5 * guess * (3.0 - root1 * guess * guess);
				rootOut = (f32)(root1 * guess);
				root1 = rootOut;
			}

			root2 = r2sq - (f32)sqi;
			if (root2 > 0.0f) {
				f64 guess = __frsqrte(root2);
				guess = 0.5 * guess * (3.0 - root2 * guess * guess);
				guess = 0.5 * guess * (3.0 - root2 * guess * guess);
				guess = 0.5 * guess * (3.0 - root2 * guess * guess);
				rootOut = (f32)(root2 * guess);
				root2 = rootOut;
			}

			draw = TRUE;
			if (x < (f32)hx.imgWHalf) {
				x1 = x + root1;
				x0 = x + root2;
				if (x1 < 0.0f)
					draw = FALSE;
			} else {
				x0 = x - root1;
				x1 = x - root2;
				if (x0 > (f32)hx.imgW)
					draw = FALSE;
			}

			if (draw) {
				GXBegin(0xA8, GX_VTXFMT0, 2);
				GXPosition3f32(x0, lineY, z);
				GXColor1u32(color);
				GXPosition3f32(x1, lineY, z);
				GXColor1u32(color);
			}
		}
		pos += step;
	}
}

static void Hx_Test2R() {
	switch (hx.unk38) {
	case 0:
		r_432 = 1.0f;
		Hx_MotionSet((HxMotion*)hx.rest, 500.0f, 2.0f, 8.0f, 1.0f);
		hx.unk38++;
		hx.unk3C = 11;
		/* fallthrough */
	case 1:
		Hxs1_Test2(600, 0, (f32)(hx.imgW + 200), 150.0f, 700.0f,
		           450.0f);
		Hxs1_Test2(600, 1, (f32)(hx.imgW + 250), 370.0f, 650.0f,
		           400.0f);
		Hxs1_Test2(600, 0, (f32)(hx.imgW + 250), 300.0f, 420.0f,
		           200.0f);
		r_432 = Hx_MotionUpdate((HxMotion*)hx.rest);
		r_432 = 500.0f - r_432;
		Hxs1_Test2((u32)r_432, 0, (f32)(hx.imgW + 200), 300.0f, 900.0f,
		           650.0f);
		if (Hx_TimerCountDown() == 0) {
			hx.unk38++;
			hx.unk3C = 11;
			Hx_MotionSet((HxMotion*)hx.rest, 500.0f, 2.0f, 8.0f, 1.0f);
		}
		break;
	case 2:
		Hxs1_Test2(600, 1, (f32)(hx.imgW + 250), 370.0f, 650.0f,
		           400.0f);
		Hxs1_Test2(600, 0, (f32)(hx.imgW + 250), 300.0f, 420.0f,
		           200.0f);
		r_432 = Hx_MotionUpdate((HxMotion*)hx.rest);
		r_432 = 500.0f - r_432;
		Hxs1_Test2((u32)r_432, 1, (f32)(hx.imgW + 200), 150.0f, 700.0f,
		           450.0f);
		if (Hx_TimerCountDown() == 0) {
			hx.unk38++;
			hx.unk3C = 10;
			Hx_MotionSet((HxMotion*)hx.rest, 500.0f, 2.0f, 7.0f, 1.0f);
		}
		break;
	case 3:
		Hxs1_Test2(600, 0, (f32)(hx.imgW + 250), 300.0f, 420.0f,
		           200.0f);
		r_432 = Hx_MotionUpdate((HxMotion*)hx.rest);
		r_432 = 500.0f - r_432;
		Hxs1_Test2((u32)r_432, 0, (f32)(hx.imgW + 250), 370.0f, 650.0f,
		           400.0f);
		if (Hx_TimerCountDown() == 0) {
			hx.unk38++;
			hx.unk3C = 12;
			Hx_MotionSet((HxMotion*)hx.rest, 500.0f, 2.0f, 9.0f, 1.0f);
		}
		break;
	case 4:
		r_432 = Hx_MotionUpdate((HxMotion*)hx.rest);
		r_432 = 500.0f - r_432;
		Hxs1_Test2((u32)r_432, 1, (f32)(hx.imgW + 250), 300.0f, 420.0f,
		           200.0f);
		if (Hx_TimerCountDown() == 0) {
			hx.unk38++;
			hx.state = 3;
		}
		break;
	default:
		hx.state = 3;
		break;
	}
}

static void Hx_Test2() {
	switch (hx.unk38) {
	case 0:
		r_416 = 1.0f;
		Hx_MotionSet((HxMotion*)hx.rest, 500.0f, 2.0f, 8.0f, 1.0f);
		hx.unk38++;
		hx.unk3C = 11;
		break;
	case 1:
		r_416 = Hx_MotionUpdate((HxMotion*)hx.rest);
		Hxs1_Test2((u32)r_416, 1, (f32)(hx.imgW + 200), 300.0f, 900.0f,
		           650.0f);
		if (Hx_TimerCountDown() == 0) {
			hx.unk38++;
			hx.unk3C = 11;
			Hx_MotionSet((HxMotion*)hx.rest, 500.0f, 2.0f, 8.0f, 1.0f);
		}
		break;
	case 2:
		Hxs1_Test2(600, 0, (f32)(hx.imgW + 200), 300.0f, 900.0f,
		           650.0f);
		r_416 = Hx_MotionUpdate((HxMotion*)hx.rest);
		Hxs1_Test2((u32)r_416, 0, (f32)(hx.imgW + 200), 150.0f, 700.0f,
		           450.0f);
		if (Hx_TimerCountDown() == 0) {
			hx.unk38++;
			hx.unk3C = 10;
			Hx_MotionSet((HxMotion*)hx.rest, 500.0f, 2.0f, 7.0f, 1.0f);
		}
		break;
	case 3:
		Hxs1_Test2(600, 0, (f32)(hx.imgW + 200), 300.0f, 900.0f,
		           650.0f);
		Hxs1_Test2(600, 0, (f32)(hx.imgW + 200), 150.0f, 700.0f,
		           450.0f);
		r_416 = Hx_MotionUpdate((HxMotion*)hx.rest);
		Hxs1_Test2((u32)r_416, 1, (f32)(hx.imgW + 250), 370.0f, 650.0f,
		           400.0f);
		if (Hx_TimerCountDown() == 0) {
			hx.unk38++;
			hx.unk3C = 12;
			Hx_MotionSet((HxMotion*)hx.rest, 500.0f, 2.0f, 9.0f, 1.0f);
		}
		break;
	case 4:
		Hxs1_Test2(600, 0, (f32)(hx.imgW + 200), 300.0f, 900.0f,
		           650.0f);
		Hxs1_Test2(600, 0, (f32)(hx.imgW + 200), 150.0f, 700.0f,
		           450.0f);
		Hxs1_Test2(600, 1, (f32)(hx.imgW + 250), 370.0f, 650.0f,
		           400.0f);
		r_416 = Hx_MotionUpdate((HxMotion*)hx.rest);
		Hxs1_Test2((u32)r_416, 0, (f32)(hx.imgW + 250), 300.0f, 420.0f,
		           200.0f);
		if (Hx_TimerCountDown() == 0) {
			hx.unk38++;
			hx.state = 3;
		}
		break;
	default:
		hx.state = 3;
		break;
	}
}

static void Hxs1_Test1(f32 x, f32 y, f32 r) {
	u32 i;
	f32 r2;
	f32 z;
	u32 color;

	Hx_CameraInit();
	Hx_GxInit(0, 1);
	GXSetLineWidth(7, GX_TO_ZERO);

	r2 = r * r;
	GXBegin(0xA8, GX_VTXFMT0, ((u32)r << 1) + 2);

	i = 0;
	z = 1.0f;
	color = 0xff;
	while (i <= (u32)r) {
		f32 root;
		f32 iy;
		f32 x0;
		f32 x1;
		f32 y0;
		volatile f32 rootOut;

		root = r2 - (f32)(i * i);
		if (root > 0.0f) {
			f64 guess = __frsqrte(root);
			guess = 0.5 * guess * (3.0 - root * guess * guess);
			guess = 0.5 * guess * (3.0 - root * guess * guess);
			guess = 0.5 * guess * (3.0 - root * guess * guess);
			rootOut = (f32)(root * guess);
			root = rootOut;
		}

		iy = (f32)i;
		if (y < (f32)hx.imgHHalf)
			y0 = y + iy;
		else
			y0 = y - iy;

		if (x < (f32)hx.imgWHalf) {
			x0 = x;
			x1 = x + root;
		} else {
			x0 = x - root;
			x1 = x;
		}

		GXPosition3f32(x0, y0, z);
		GXColor1u32(color);
		GXPosition3f32(x1, y0, z);
		GXColor1u32(color);
		i++;
	}
}

static void Hx_Test1() {
	switch (hx.unk38) {
	case 0:
		if (hx.type == 1) {
			r_393 = 400.0f;
			Hx_MotionSet((HxMotion*)hx.rest, 400.0f, 10.0f, 12.0f, 8.0f);
		} else {
			r_393 = 1.0f;
			Hx_MotionSet((HxMotion*)hx.rest, 400.0f, 5.0f, 10.0f, 10.0f);
		}
		hx.unk38++;
		hx.unk3C = 25;
		break;
	case 1:
		if (Hx_TimerCountDown() == 0)
			hx.unk38++;
		r_393 = Hx_MotionUpdate((HxMotion*)hx.rest);
		if (hx.type == 1)
			r_393 = 400.0f - r_393;
		break;
	default:
		hx.state = 3;
		break;
	}

	Hxs1_Test1(0.0f, 0.0f, r_393);
	Hxs1_Test1((f32)hx.imgW, 0.0f, r_393);
	Hxs1_Test1((f32)hx.imgW, (f32)hx.imgH, r_393);
	Hxs1_Test1(0.0f, (f32)hx.imgH, r_393);
}

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
static void Hx_Door() {
	u32 v;
	f32 f;
	u32 halfW;

	switch (hx.unk38) {
	case 0:
		hx.unk38++;
		Hx_MotionSet((HxMotion*)hx.rest, (f32)(hx.imgW >> 1), 5.0f, 6.0f,
		             5.0f);
		break;
	case 1:
		v = (u32)(int)Hx_MotionUpdate((HxMotion*)hx.rest);
		f = (f32)(int)v;
		Hxs_FrBufferMorf2(f);
		if (v >= (hx.imgW >> 1)) {
			hx.unk38++;
			Hx_MotionSet((HxMotion*)hx.rest, (f32)(hx.imgW >> 1), 5.0f,
			             6.0f, 5.0f);
		}
		break;
	case 2:
		halfW = hx.imgW >> 1;
		Hxs_FrBufferMorf2((f32)halfW);
		v = (u32)(int)Hx_MotionUpdate((HxMotion*)hx.rest);
		f = (f32)(int)v;
		Hxs_FrBufferMorf2B(f);
		if (v >= (hx.imgW >> 1))
			hx.unk38++;
		break;
	case 3:
		halfW = hx.imgW >> 1;
		Hxs_FrBufferMorf2((f32)halfW);
		Hxs_FrBufferMorf2B((f32)halfW);
		hx.state = 3;
		break;
	}
}

static void Hxs_FrBufferMorf2B(f32 x) {
	GXTexObj tobj;
	u32 srcX;
	f32 right;
	f32 y;
	f32 stripH;
	f32 zero;
	f32 one;
	f32 screenW;
	f32 screenH;

	srcX = (hx.imgW >> 1) + (hx.imgW >> 2);
	Frb2_InitGx(&tobj);
	screenW = (f32)hx.imgW;
	right = screenW - x;
	if (x < (f32)(hx.imgW >> 2)) {
		zero = 0.0f;
		one = 1.0f;
		stripH = 16.0f;
		y = 0.0f;
		while (y < (f32)hx.imgH) {
			f32 y1;
			f32 srcXF;

			Hx_GetFrBuffer(fbuf2, srcX, (u32)y, 0xA0, 0x10);
			GXInvalidateTexAll();
			GXLoadTexObj(&tobj, GX_TEXMAP0);
			GXBegin(GX_QUADS, GX_VTXFMT0, 4);
			y1 = y + stripH;
			srcXF = (f32)(int)srcX;
			GXPosition3f32(srcXF, y, zero);
			GXColor1u32(0);
			GXTexCoord2f32(zero, zero);
			GXPosition3f32(right, y, zero);
			GXColor1u32(0);
			GXTexCoord2f32(one, zero);
			GXPosition3f32(right, y1, zero);
			GXColor1u32(0);
			GXTexCoord2f32(one, one);
			GXPosition3f32(srcXF, y1, zero);
			GXColor1u32(0);
			GXTexCoord2f32(zero, one);
			GXDrawDone();
			y += stripH;
		}
	}

	Frb2_InitBlackBox();
	screenH = (f32)hx.imgH;
	Frb2_RendBox(0xFF, right, 0.0f, screenW, screenH);
}

static void Hxs_FrBufferMorf2(f32 x) {
	GXTexObj tobj;
	f32 y;
	f32 stripH;
	f32 zero;
	f32 one;
	f32 srcRight;
	f32 screenH;

	Frb2_InitGx(&tobj);
	if (x < (f32)(hx.imgW >> 2)) {
		y = 0.0f;
		zero = 0.0f;
		one = 1.0f;
		stripH = 16.0f;
		while (y < (f32)hx.imgH) {
			f32 y1;

			Hx_GetFrBuffer(fbuf2, 0, (u32)y, 0xA0, 0x10);
			GXInvalidateTexAll();
			GXLoadTexObj(&tobj, GX_TEXMAP0);
			GXBegin(GX_QUADS, GX_VTXFMT0, 4);
			y1 = y + stripH;
			srcRight = (f32)(hx.imgW >> 2);
			GXPosition3f32(x, y, zero);
			GXColor1u32(0);
			GXTexCoord2f32(zero, zero);
			GXPosition3f32(srcRight, y, zero);
			GXColor1u32(0);
			GXTexCoord2f32(one, zero);
			GXPosition3f32(srcRight, y1, zero);
			GXColor1u32(0);
			GXTexCoord2f32(one, one);
			GXPosition3f32(x, y1, zero);
			GXColor1u32(0);
			GXTexCoord2f32(zero, one);
			GXDrawDone();
			y += stripH;
		}
	}

	Frb2_InitBlackBox();
	screenH = (f32)hx.imgH;
	Frb2_RendBox(0xFF, 0.0f, 0.0f, x, screenH);
	GXBegin(GX_QUADS, GX_VTXFMT0, 4);
	GXPosition3f32(0.0f, 0.0f, 0.0f);
	GXColor1u32(0xFF);
	GXPosition3f32(x, 0.0f, 0.0f);
	GXColor1u32(0xFF);
	GXPosition3f32(x, screenH, 0.0f);
	GXColor1u32(0xFF);
	GXPosition3f32(0.0f, screenH, 0.0f);
	GXColor1u32(0xFF);
}

static void Hx_Circle() {}

// ---------------------------------------------------------------------------
// Motion solver
// ---------------------------------------------------------------------------
#pragma dont_inline on
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
#pragma dont_inline off

void Hx_MotionSet(HxMotion* m, f32 dist, f32 t1, f32 t2, f32 t3) {
	f32 t12;
	f32 stop;
	f32 denom;
	f32 v;
	m->unk00 = t1;
	t12 = t1 + t2;
	stop = m->unk00 + t2;
	denom = t3 + (t2 + t12);
	m->unk04 = stop;
	m->unk08 = m->unk04 + t3;
	v = 2.0f * dist / denom;
	if (t1 != 0.0f)
		m->unk0C = v / t1;
	if (t3 != 0.0f)
		m->unk14 = -v / t3;
	m->unk10 = 0.0f;
	m->unk18 = 0.0f;
	m->unk20 = 0.0f;
	m->unk1C = 0.0f;
}

#pragma dont_inline on
u32 Hx_TimerCountDown() {
	if (hx.unk3C != 0)
		hx.unk3C--;
	return hx.unk3C;
}
#pragma dont_inline off

// ---------------------------------------------------------------------------
// Wipe driver
// ---------------------------------------------------------------------------
u32 Hx_UpdateWipe(f32 step) {
	ReInitializeGX();

	switch (hx.state) {
	case 0:
		break;
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
	if ((int)hx.resFlag == 0) {
		hx.buffer = hx_buffer;
		hx.bufSize = 0x3300;
	}
	if ((int)hx.state == 2)
		Hx_Warning(1);
	hx.state = 1;
	hx.wipeNo = no;
	hx.timer = 0.0f;
	hx.unk1C = arg;
}

void Hx_RemoveResource() {
	if (hx.state == 2)
		Hx_Warning(1);
	if ((int)hx.resFlag == 0)
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
	if ((int)hx.resFlag != 0)
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

#pragma dont_inline on
static void Hx_Warning(int code) {}
#pragma dont_inline off

// ---------------------------------------------------------------------------
// GX helpers
// ---------------------------------------------------------------------------
#pragma dont_inline on
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
#pragma dont_inline off

static void Hx_SetVFilter(f32 ratio) {
	u32 i;
	u8 n;
	vtable[0] = vtable_org[0];
	n = (u8)(int)(64.0f * ratio);
	vtable[1] = vtable_org[1];
	vtable[2] = vtable_org[2];
	vtable[3] = vtable_org[3];
	vtable[4] = vtable_org[4];
	vtable[5] = vtable_org[5];
	vtable[6] = vtable_org[6];
	for (i = 0; i < n; i++) {
		vtable[dec_step[i & 3]]--;
		vtable[inc_step[i % 3]]++;
	}
	GXSetCopyFilter(GX_FALSE, NULL, GX_TRUE, vtable);
}

static void Frb2_InitGx(GXTexObj* tobj) {
	Hx_CameraInit();
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	Hx_SetVFilter(1.0f);
	GXSetNumTexGens(1);
	GXSetNumTevStages(1);
	GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR);
	GXInitTexObj(tobj, fbuf2, 0xA0, 0x10, GX_TF_RGB565, GX_CLAMP, GX_CLAMP,
	             GX_FALSE);
	GXInitTexObjLOD(tobj, GX_LINEAR, GX_LINEAR, 0.0f, 10.0f, 0.0f, GX_FALSE,
	                GX_TRUE, GX_ANISO_1);
	GXLoadTexObj(tobj, GX_TEXMAP0);
}

static void Hx_FrBufferMorf(f32 ratio) {
	Hx_SetVFilter(ratio);
	__Hx_FrBufferMorf(hx.imgWHalf - 0x18, hx.imgHHalf - 0x18);
}

static void __Hx_FrBufferMorf(u32 x, u32 y) {
	GXTexObj tobj;

	Hx_CameraInit();
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	Hx_GetFrBuffer(fbuf, x, y, 0x30, 0x30);
	GXInvalidateTexAll();
	GXSetNumTexGens(1);
	GXSetNumTevStages(1);
	GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR);
	GXInitTexObj(&tobj, fbuf, 0x30, 0x30, GX_TF_RGB565, GX_CLAMP, GX_CLAMP,
	             GX_FALSE);
	GXInitTexObjLOD(&tobj, GX_LINEAR, GX_LINEAR, 0.0f, 10.0f, 0.0f, GX_FALSE,
	                GX_TRUE, GX_ANISO_1);
	GXLoadTexObj(&tobj, GX_TEXMAP0);

	GXBegin(GX_QUADS, GX_VTXFMT0, 4);
	GXPosition3f32((f32)x, (f32)y, 0.0f);
	GXColor1u32(0);
	GXTexCoord2f32(0.0f, 0.0f);
	GXPosition3f32((f32)(x + 0x30), (f32)y, 0.0f);
	GXColor1u32(0);
	GXTexCoord2f32(1.0f, 0.0f);
	GXPosition3f32((f32)(x + 0x30), (f32)(y + 0x30), 0.0f);
	GXColor1u32(0);
	GXTexCoord2f32(1.0f, 1.0f);
	GXPosition3f32((f32)x, (f32)(y + 0x30), 0.0f);
	GXColor1u32(0);
	GXTexCoord2f32(0.0f, 1.0f);
}

#pragma dont_inline on
static void Hx_GetFrBuffer(void* dest, u32 left, u32 top, u32 wd, u32 ht) {
	GXColor clear = { 0, 0, 0, 0 };
	GXSetTexCopySrc(left, top, wd, ht);
	GXSetTexCopyDst(wd, ht, GX_TF_RGB565, GX_FALSE);
	GXGetTexBufferSize(wd, ht, GX_TF_RGB565, GX_FALSE, 0);
	GXSetCopyClear(clear, 0xFFFFFF);
	GXCopyTex(dest, GX_TRUE);
	GXPixModeSync();
}
#pragma dont_inline off

static void Hgx_ReadTexture(char* fileName, void* addr) {
	DVDFileInfo fi;
	if ((int)hx.resFlag == 0) {
		if (DVDOpen(fileName, &fi)) {
			long len = DVDReadPrio(&fi, addr, fi.length, 0, 2);
			DVDClose(&fi);
			DCStoreRange(addr, len);
		}
	}
}

static void Hgx_init_tobj_resource(GXTexObj* obj, HxTexRes* res) {
	u32 imageOffset = res->imageOffset;
	u8 format = res->format;
	u8 wrapS = res->wrapS;
	u8 wrapT = res->wrapT;
	u8 minFilter = res->minFilter;
	u8 magFilter = res->magFilter;
	void* image = (u8*)res + imageOffset;
	img_wx = res->width;
	img_wy = res->height;
	GXInitTexObj(obj, image, img_wx, img_wy, format, wrapS, wrapT, GX_FALSE);
	GXInitTexObjLOD(obj, minFilter, magFilter, 0.0f, 0.0f, 0.0f, GX_FALSE,
	                GX_FALSE, GX_ANISO_1);
}

static void Hx_CameraInit() {
	Mtx44 proj;
	Mtx posMtx;
	f32 hw = (f32)(hx.imgW >> 1);
	f32 hh = (f32)(hx.imgH >> 1);

	camLoc.x = hw;
	camLoc.y = hh;
	objPt.x = hw;
	objPt.y = hh;
	C_MTXOrtho(proj, hh, -hh, -hw, hw, 0.0f, 100.0f);
	GXSetProjection(proj, GX_ORTHOGRAPHIC);
	GXSetViewport(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
	C_MTXLookAt(posMtx, &camLoc, &up, &objPt);
	GXSetCullMode(GX_CULL_NONE);
	GXSetCoPlanar(GX_FALSE);
	GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	GXSetNumTexGens(0);
	GXSetNumTevStages(1);
	GXSetNumIndStages(0);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GXSetLineWidth(6, GX_TO_ZERO);
	GXLoadPosMtxImm(posMtx, GX_PNMTX0);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_VTX, GX_SRC_VTX, 0, GX_DF_NONE,
	              GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE,
	              GX_AF_NONE);
	GXSetNumChans(1);
}

#pragma dont_inline on
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
	case 1:
		GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
		break;
	case 0:
		GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR);
		break;
	}
}
#pragma dont_inline off
