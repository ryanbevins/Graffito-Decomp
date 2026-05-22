#include <GC2D/SunGlass.hpp>
#include <Camera/SunMgr.hpp>
#include <Player/MarioAccess.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JParticle/JPAEmitterManager.hpp>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>

extern JPAEmitterManager* gpEmitterManager4D2;

void TSunGlass::startFade(int param_1, bool param_2)
{
	TFlagManager::getInstance()->getFlag(0x40000);

	if (param_1 == 2) {
		s32 alpha = 0;
		if (gpMarDirector->mMap == 1) {
			s32 flag = TFlagManager::smInstance->getFlag(0x40000);
			alpha    = (s32)((f32)(unk1E - unk1F)
			               * (1.0f - (f32)flag / 120.0f));
		}
		unk1D = alpha;
		unk1C = 100;
	} else {
		unk1D     = 100;
		s32 alpha = 0;
		if (gpMarDirector->mMap == 1) {
			s32 flag = TFlagManager::smInstance->getFlag(0x40000);
			alpha    = (s32)((f32)(unk1E - unk1F)
			               * (1.0f - (f32)flag / 120.0f));
		}
		unk1C = alpha;
	}

	unk26 = param_2;
	unk24 = 0;
}

void TSunGlass::draw(const JDrama::TRect& param_1, JUtility::TColor param_2)
{
	if (param_2.a == 0)
		return;

	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);

	Mtx mtx;
	MTXTrans(mtx, 0.0f, 0.0f, 0.0f);
	GXLoadPosMtxImm(mtx, 0);
	GXSetCurrentMtx(0);
	GXSetZMode(0, GX_LEQUAL, 0);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3c, 0, 0x7d);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_CLR_RGBA, GX_RGBA4, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GXSetNumChans(1);
	GXSetNumTexGens(0);
	GXSetNumTevStages(1);
	GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);

	if (unk1A & 0x2)
		GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
	else
		GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA,
		               GX_LO_NOOP);

	u32 color = param_2;
	GXBegin(GX_QUADS, GX_VTXFMT0, 4);
	GXPosition3s16(param_1.x1, param_1.y1, 0);
	GXColor1u32(color);
	GXPosition3s16(param_1.x2, param_1.y1, 0);
	GXColor1u32(color);
	GXPosition3s16(param_1.x2, param_1.y2, 0);
	GXColor1u32(color);
	GXPosition3s16(param_1.x1, param_1.y2, 0);
	GXColor1u32(color);
}

void TSunGlass::perform(u32 param_1, JDrama::TGraphics* param_2)
{
	if (param_1 & 1) {
		if (unk26) {
			unk14.a = (s32)((f32)unk1D
			              + (f32)(unk24 * (unk1C - unk1D))
			                    / (f32)unk22);
			if ((int)unk24 < (int)unk22) {
				unk24++;
			} else {
				unk26 = 0;
			}
		}
	}

	if (param_1 & 8) {
		draw(param_2->getViewport(), unk14);
	}
}

void TSunGlass::loadAfter()
{
	s32 alpha = 0;
	if (gpMarDirector->mMap == 1) {
		s32 flag = TFlagManager::smInstance->getFlag(0x40000);
		alpha    = (s32)((f32)(unk1E - unk1F)
		               * (1.0f - (f32)flag / 120.0f));
	}
	unk14.a = alpha;
}

void TSunGlass::load(JSUMemoryInputStream& stream)
{
	JDrama::TNameRef::load(stream);
	unk10 = gpMarDirector->unk18[1];
}

void TSunShine::perform(u32 param_1, JDrama::TGraphics* param_2)
{
	if (param_1 & 8) {
		draw(param_2->getViewport(), unk14);
	}

	if (param_1 & 1) {
		unk14.a = (u8)gpSunMgr->getAddColor();
		if (unk28) {
			if (!SMS_IsMarioStatusElecDamage()) {
				unk28 = 0;
			}
		} else {
			if (SMS_IsMarioStatusElecDamage()) {
				unk28 = 1;
				JGeometry::TVec3<f32> pos(300.0f, 224.0f, 0.0f);
				gpEmitterManager4D2->createEmitter(pos, 0x200, nullptr,
				                                   nullptr);
			}
		}
	}
}

void TSunShine::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	if (gpMarDirector->mMap == 6) {
		unk14.r = 72;
		unk14.g = 48;
		unk14.b = 0;
		unk14.a = 255;
	}
}
