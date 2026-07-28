#include <MoveBG/MapObjWave.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <Camera/CubeMapTool.hpp>
#include <System/MarDirector.hpp>
#include <Player/MarioAccess.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <dolphin/gx.h>
#include <math.h>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <JSystem/JAudio/JALibrary/JALModSe.hpp>
#include <MSound/MSoundBGM.hpp>

TMapObjWave* gpMapObjWave;

static GXColor sColor;

static u8 sAlphaCompLarge = 0x55;
static u8 sAlphaCompSmall = 0x23;

TMapObjWave::TMapObjWave(const char* name)
    : JDrama::TViewObj(name)
{
	mWaveSize        = 0.0f;
	mHalfWaveSize    = 0.0f;
	mInvHalfWaveSize = 0.0f;
	mGridCount       = 0;
	unk24            = 0.0f;
	unk28            = 0.0f;
	unk2C            = 0.0f;
	unk30            = 0.0f;
	unk34            = 0.0f;
	unk38            = 0.0f;
	unk3C            = 0.0f;
	unk40            = 0.0f;
	unk44            = 0.0f;
	unk48            = 0.1f;
	unk4C            = 0.0f;
	unk50            = 0.0f;
	unk54            = 255.0f;
	unk58            = 255.0f;
	unk5C            = 0.0f;
	unk60            = 0.0f;
	unk64            = MsRandF() * 360.0f;
	unk68            = MsRandF() * 360.0f;
	unk6C            = MsRandF();
	unk70            = MsRandF();
	unk74            = 0.0f;
	unk78            = 0.0f;
	mTexInfo         = nullptr;
	unk98            = 0;

	sColor.r = 0xc8;
	sColor.g = 0xc8;
	sColor.b = 0xff;
	sColor.a = 0;

	unk7C.r = 0xc2;
	unk7C.g = 0xf2;
	unk7C.b = 0xbe;
	unk7C.a = 0;

	unk84.r = 0;
	unk84.g = 0;
	unk84.b = 0;
	unk84.a = 0x48;

	unk8C.r = 0;
	unk8C.g = 0;
	unk8C.b = 0;
	unk8C.a = 0x90;

	gpMapObjWave = this;
}

void TMapObjWave::load(JSUMemoryInputStream& stream)
{
	JDrama::TNameRef::load(stream);

	mWaveSize        = 5200.0f;
	mGridSize        = 200.0f;
	mHalfWaveSize    = mWaveSize / 2.0f;
	mInvHalfWaveSize = 1.0f / mHalfWaveSize;
	mGridCount       = (int)(mWaveSize / mGridSize);

	mTexInfo = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    "/scene/map/map/wave.bti");

	unk60 = 0.0015f;
	unk74 = 0.0012f;
	unk78 = 0.0015f;
	unk4C = 400.0f;
	unk50 = 150.0f;
	unk24 = 0.02f;
	unk28 = 0.03f;

	switch (gpMarDirector->mMap) {
	case 3:
	case 0x1e:
		unk2C = 25.0f;
		unk30 = 20.0f;
		unk34 = 0.0f;
		unk38 = 0.0f;
		unk3C = unk2C;
		unk40 = unk30;
		break;
	case 4:
		unk2C = 40.0f;
		unk30 = 30.0f;
		unk34 = 5.0f;
		unk38 = 0.0f;
		break;
	case 0xd:
		unk2C = 30.0f;
		unk30 = 25.0f;
		unk34 = 5.0f;
		unk38 = 0.0f;
		break;
	case 9:
	case 0x34:
		unk2C = 10.0f;
		unk30 = 15.0f;
		unk34 = 0.0f;
		unk38 = 0.0f;
		break;
	default:
		unk2C = 30.0f;
		unk30 = 25.0f;
		unk34 = 0.0f;
		unk38 = 0.0f;
		break;
	}
	unk3C = unk2C;
	unk40 = unk30;
}

void TMapObjWave::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (mTexInfo == nullptr)
		return;

	if (flags & 1) {
		updateTime();
		u8 map = gpMarDirector->mMap;
		if (map == 4 || map == 6) {
			updateHeightAndAlpha();
		}
	}

	if (flags & 8) {
		initDraw();
		draw();
	}
}

#pragma dont_inline on
void TMapObjWave::updateTime()
{
	unk64 += unk24;
	if (unk64 > 6.28318f)
		unk64 -= 6.28318f;

	unk68 += unk28;
	if (unk68 > 6.28318f)
		unk68 -= 6.28318f;

	unk6C += unk60;
	if (unk6C > 1.0f)
		unk6C -= 1.0f;

	unk70 += unk60;
	if (unk70 > 1.0f)
		unk70 -= 1.0f;
}
#pragma dont_inline off

static bool isWaterBg(u16 code)
{
	if (code == 0x100)
		return true;
	if (code == 0x101)
		return true;
	if ((u16)(code - 0x102) <= 3)
		return true;
	if (code == 0x4104)
		return true;
	return false;
}

void TMapObjWave::updateHeightAndAlpha()
{
	const TBGCheckData* groundBelow;
	const TBGCheckData* groundExact;
	gpMap->checkGround(*gpMarioPos, &groundBelow);
	gpMap->checkGroundExactY(gpMarioPos->x, 10.0f, gpMarioPos->z,
	                         &groundExact);

	bool inWater = SMS_CheckMarioFlag(0x10000);

	if (inWater || isWaterBg(groundExact->mBGType)
	    || isWaterBg(groundBelow->mBGType)) {
		f32 groundY = gpMap->checkGroundIgnoreWaterSurface(
		    gpMarioPos->x, 0.0f, gpMarioPos->z, &groundExact);
		f32 dist = unk4C + groundY;
		if (dist < 0.0f || groundExact->mBGType == 0x700) {
			unk3C = unk2C;
			unk40 = unk30;
		} else {
			f32 ratio = 1.0f - dist / unk4C;
			unk3C     = (unk2C - unk34) * ratio + unk34;
			unk40     = (unk30 - unk38) * ratio + unk38;
		}

		f32 alphaDist = unk50 + groundY;
		if (alphaDist < 0.0f || groundExact->mBGType == 0x700) {
			unk54 = unk58;
		} else {
			unk54
			    = (unk58 - unk5C) * (1.0f - alphaDist / unk50) + unk5C;
		}
	} else {
		unk3C = unk34;
		unk40 = unk38;
		unk54 = unk5C;
	}

	if (gpMarDirector->mMap == 4) {
		if (-4950.0f < gpMarioPos->x && -4340.0f > gpMarioPos->x
		    && 7660.0f < gpMarioPos->z && 8040.0f > gpMarioPos->z) {
			unk3C = unk34;
			unk40 = unk38;
			unk54 = unk5C;
		}
	}

	s32 cubeNo = gpCubeStream->getInCubeNo(*gpMarioPos);
	if (cubeNo != -1) {
		TCubeStreamInfo* info
		    = (TCubeStreamInfo*)&(*gpCubeStream->unk14)[cubeNo];
		if (unk44 < info->unk3C) {
			unk44 += unk48;
		}
	} else if (unk44 > 0.0f) {
		unk44 -= unk48;
	} else {
		unk44 = 0.0f;
	}

	if (unk44 > 0.0f) {
		unk3C = unk2C + unk44;
		unk40 = unk30 + unk44;
	}
}

void TMapObjWave::draw()
{
	f32 invTwoPi = 0.15915507f;

	for (f32 outerZ = -mHalfWaveSize;
	     outerZ <= mHalfWaveSize - mGridSize; outerZ += mGridSize) {
		f32 z0 = outerZ + gpMarioPos->z;
		f32 z1 = z0 + mGridSize;
		GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, (u16)(mGridCount * 2));
		f32 invTwoPiZ0 = invTwoPi * z0;
		f32 invTwoPiZ1 = invTwoPi * z1;
		f32 absOuter   = fabsf(outerZ);

		for (f32 innerX = -mHalfWaveSize;
		     innerX <= mHalfWaveSize - mGridSize; innerX += mGridSize) {
			f32 absInner = fabsf(innerX);
			f32 x0       = innerX + gpMarioPos->x;
			int alpha0;
			if (absInner > absOuter) {
				alpha0 = (int)(unk54
				               * (1.0f - mInvHalfWaveSize * absInner));
			} else {
				alpha0 = (int)(unk54
				               * (1.0f - mInvHalfWaveSize * absOuter));
			}

			f32 absOuterNext = fabsf(outerZ + mGridSize);
			int alpha1;
			if (absInner > absOuterNext) {
				alpha1 = (int)(unk54
				               * (1.0f - mInvHalfWaveSize * absInner));
			} else {
				alpha1 = (int)(unk54
				               * (1.0f - mInvHalfWaveSize * absOuterNext));
			}

			f32 y0;
			if (mTexInfo == nullptr) {
				y0 = 0.0f;
			} else {
				f32 wave1
				    = unk3C * sinf(unk24 * (invTwoPi * x0) + unk64);
				f32 wave2 = unk40 * sinf(unk28 * invTwoPiZ0 + unk68);
				y0        = wave1 + wave2;
			}
			GXPosition3f32(x0, y0, z0);
			GXColor4u8(sColor.r, sColor.g, sColor.b, alpha0);
			f32 texX0 = x0 * unk74;
			f32 texZ0 = z0 * unk74;
			f32 texX1 = x0 * unk78;
			f32 texZ1 = z0 * unk78;
			GXTexCoord2f32(unk6C + texX0, texZ0);
			GXTexCoord2f32(0.8f * texX1, unk70 + texZ1);

			f32 y1;
			if (mTexInfo == nullptr) {
				y1 = 0.0f;
			} else {
				f32 wave1
				    = unk3C * sinf(unk24 * (invTwoPi * x0) + unk64);
				f32 wave2 = unk40 * sinf(unk28 * invTwoPiZ1 + unk68);
				y1        = wave1 + wave2;
			}
			GXPosition3f32(x0, y1, z1);
			GXColor4u8(sColor.r, sColor.g, sColor.b, alpha1);
			f32 texZNext0 = z1 * unk74;
			f32 texZNext1 = z1 * unk78;
			GXTexCoord2f32(unk6C + texX0, texZNext0);
			GXTexCoord2f32(0.8f * texX1, unk70 + texZNext1);
		}
	}
}

void TMapObjWave::noWave()
{
	unk34 = 0.0f;
	unk38 = 0.0f;
	unk2C = 0.0f;
	unk30 = 0.0f;
	unk3C = 0.0f;
	unk40 = 0.0f;
}

f32 TMapObjWave::getHeight(f32 x, f32 y, f32 z) const
{
	const TBGCheckData* ground;
	f32 height = gpMap->checkGroundExactY(x, y + 50.0f, z, &ground);
	u16 code   = ground->mBGType;
	if (!isWaterBg(code))
		return y;
	if (code != 0x102 && code != 0x103)
		return height;
	if (mTexInfo == nullptr)
		return 0.0f;

	f32 wave1 = unk3C * sinf(unk24 * (0.15915507f * x) + unk64);
	f32 wave2 = unk40 * sinf(unk28 * (0.15915507f * z) + unk68);
	return wave1 + wave2;
}

f32 TMapObjWave::getWaveHeight(f32 x, f32 z) const
{
	if (mTexInfo == nullptr)
		return 0.0f;

	f32 wave1 = unk3C * sinf(unk24 * (0.15915507f * x) + unk64);
	f32 wave2 = unk40 * sinf(unk28 * (0.15915507f * z) + unk68);
	return wave1 + wave2;
}

void TMapObjWave::initDraw()
{
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);

	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX1, GX_DIRECT);

	GXLoadPosMtxImm(j3dSys.mViewMtx, 0);
	GXSetCurrentMtx(0);

	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);

	GXSetNumTexGens(2);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY,
	                  GX_FALSE, GX_PTIDENTITY);
	GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX1, GX_IDENTITY,
	                  GX_FALSE, GX_PTIDENTITY);

	JUTTexture texture(mTexInfo);
	texture.load(GX_TEXMAP0);

	GXSetTevColorS10(GX_TEVREG0, unk7C);
	GXSetTevColorS10(GX_TEVREG1, unk84);
	GXSetTevColorS10(GX_TEVREG2, unk8C);

	GXSetNumTevStages(2);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
	                GX_CC_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);

	GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP0, GX_COLOR0A0);
	GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO,
	                GX_CC_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_2,
	                GX_TRUE, GX_TEVPREV);

	GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_SRCCLR, GX_LO_NOOP);
	GXSetAlphaCompare(GX_GEQUAL, sAlphaCompLarge, GX_AOP_OR, GX_LEQUAL,
	                  sAlphaCompSmall);
	GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
	GXSetCullMode(GX_CULL_NONE);
}
