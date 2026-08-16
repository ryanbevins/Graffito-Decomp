#include <Camera/LensGlow.hpp>
#include <Camera/cameralib.hpp>
#include <Camera/SunModel.hpp>
#include <Camera/CameraMarioData.hpp>
#include <System/Resolution.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DMaterialAnm.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3D/J3DGraphBase/Components/J3DGXColorS10.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DAnmLoader.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <stdio.h>

template <> f32 CLBLinearInbetween<f32>(f32, f32, f32);
template <> f32 CLBEaseOutInbetween<f32>(f32, f32, f32);
template <> f32 CLBTwoDegreeGeneralInbetween<f32>(f32, f32, f32, f32);

static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";
static const char* SMS_NO_MEMORY_MESSAGE    = "メモリが足りません\n";
static const char* cSunVolumeNameLiteral    = "/scene/sun";
static const char* cSunsetVolumeNameLiteral = "/scene/sunset";

extern const char* cSunVolumeName;
extern const char* cSunsetVolumeName;

TLensGlow::TLensGlow(bool sunset, const char* name)
    : JDrama::TViewObj(name)
    , unk10(nullptr)
    , unk14(nullptr)
    , unk18(nullptr)
    // unk1C is J3DFrameCtrl with its own ctor (constructed here in decl order)
    , unk30(nullptr)
    // unk34 is J3DFrameCtrl with its own ctor (constructed here in decl order)
{
	unk48 = 0.0f;
	unk4C = 0.0f;
	unk50 = 0.5f;
	unk54 = 0.5f;
	unk58 = 0.5f;
	unk5C = 0xFF;
	unk5D = 2;
	unk68 = 10.0f;
	unk6C = 10.0f;
	unk70 = 0.1f;
	unk80 = 0.0f;
	unk84 = 0.0f;
	unk88 = 0.0f;
	unk8C = 0.0f;
	unk90 = 0.1f;
	unk94 = 1.5f;

	const char* volume = cSunVolumeName;
	if (sunset) {
		volume = cSunsetVolumeName;
		unk68  = 15.0f;
		unk6C  = 10.0f;
	}

	char buf[0x100];

	snprintf(buf, 0x100, "%s/%s", volume, "glow.bmd");
	unk10 = (J3DModelData*)J3DModelLoaderDataBase::load(
	    JKRFileLoader::getGlbResource(buf), 0x10020000);
	unk14 = new J3DModel(unk10, 0, 1);

	snprintf(buf, 0x100, "%s/%s", volume, "glow.btk");
	unk18 = (J3DAnmTextureSRTKey*)J3DAnmLoaderDataBase::load(
	    JKRFileLoader::getGlbResource(buf));
	unk18->searchUpdateMaterialID(unk10);

	snprintf(buf, 0x100, "%s/%s", volume, "glow.brk");
	unk30 = (J3DAnmTevRegKey*)J3DAnmLoaderDataBase::load(
	    JKRFileLoader::getGlbResource(buf));
	unk30->searchUpdateMaterialID(unk10);

	int num = unk10->getMaterialNum();
	for (u16 i = 0; i < num; i++) {
		J3DMaterialAnm* anm = new J3DMaterialAnm();
		unk10->getMaterialNodePointer(i)->change();
		unk10->getMaterialNodePointer(i)->unk38 = anm;
	}

	unk10->entryTexMtxAnimator(unk18);
	unk10->entryTevRegAnimator(unk30);

	unk1C.init(unk18->getFrameMax());
	unk1C.setRate(SMSGetAnmFrameRate());
	unk1C.setAttribute(2);

	unk34.init(unk30->getFrameMax());
	unk34.setRate(SMSGetAnmFrameRate());
	unk34.setAttribute(2);

	unk74 = 500.0f;
	unk78 = 500.0f;
	unk7C = 0.0f;
	unk60 = unk64 = unk6C;
}

void TLensGlow::perform(u32 param, JDrama::TGraphics* gfx)
{
	(void)gfx;

	u8 visible;
	if (gpCameraMario->isMarioIndoor()) {
		visible = 0;
	} else {
		TSunModel* sun = gpSunModel;
		f32 lim        = unk94;
		visible        = -lim <= sun->mFPos[0].x && sun->mFPos[0].x <= lim
		          && -lim <= sun->mFPos[0].y && sun->mFPos[0].y <= lim;
	}

	if (param & 1) {
		u8 visCount  = gpSunModel->mVisibleCount;
		f32 sunRatio = gpSunModel->mUnk194;

		if (visCount <= unk5D) {
			unk4C = 0.0f;
		} else {
			f32 t = (f32)(visCount - unk5D) * (1.0f / (f32)(17 - unk5D));
			unk4C = CLBEaseOutInbetween(0.0f, (f32)unk5C,
			                            CLBLinearInbetween(0.0f, 1.0f, t));
		}

		f32 rate;
		if (unk48 < unk4C)
			rate = unk50;
		else if (sunRatio == 0.0f)
			rate = unk58;
		else
			rate = unk54;
		CLBChaseDecrease(&unk48, unk4C, rate, 0.0f);

		unk64 = CLBLinearInbetween(0.002f * unk6C, 0.002f * unk68, sunRatio);
		CLBChaseDecrease(&unk60, unk64, unk70, 0.0f);

		f32 sunX  = gpSunModel->mFPos[0].x;
		f32 baseX = sunX * (f32)((u16)SMSGetGameRenderWidth() >> 1);
		f32 sunY  = gpSunModel->mFPos[0].y;
		f32 baseY = sunY * (f32)((u16)SMSGetGameRenderHeight() >> 1);

		if (visCount == 0) {
			unk8C = 0.0f;
			unk88 = 0.0f;
		} else if (sunRatio >= 0.5f) {
			unk8C = 0.0f;
			unk88 = 0.0f;
		} else {
			f32 sumX = 0.0f;
			f32 sumY = 0.0f;
			for (int i = 0; i < 17; i++) {
				if (gpSunModel->mZBufVisible[i]) {
					sumX += gpSunModel->mFPos[i].x;
					sumY += gpSunModel->mFPos[i].y;
				}
			}
			f32 inv = 1.0f / (f32)visCount;
			f32 tt  = 2.0f * sunRatio;
			f32 rx
			    = CLBLinearInbetween(sumX * inv, gpSunModel->mFPos[0].x, tt);
			f32 ry
			    = CLBLinearInbetween(sumY * inv, gpSunModel->mFPos[0].y, tt);
			u32 width  = SMSGetGameRenderWidth();
			u32 height = SMSGetGameRenderHeight();
			f32 x      = rx * (f32)((u16)width >> 1);
			f32 y      = ry * (f32)((u16)height >> 1);
			unk88      = x - baseX;
			unk8C      = y - baseY;
		}

		CLBChaseDecrease(&unk80, unk88, unk90, 0.0f);
		CLBChaseDecrease(&unk84, unk8C, unk90, 0.0f);
		unk74 = baseX + unk80;
		unk78 = baseY + unk84;
	}

	if (param & 2) {
		unk1C.update();
		unk34.update();
		if (visible) {
			Mtx mtx;
			Vec scale;
			scale.x = unk60;
			scale.y = unk60;
			scale.z = 1.0f;
			CLBCalcScaleTranslateMatrix(mtx, scale, (Vec&)unk74);
			PSMTXCopy(mtx, unk14->unk20);
			unk14->calc();
		}
	}

	if (param & 0x200) {
		if (visible) {
			int num = unk10->getMaterialNum();
			for (int i = 0; (u16)i < num; i++) {
				J3DGXColorS10 c;
				c.color =
				    unk10->getMaterialNodePointer(i)->getTevColor(0)->color;
				c.color.a = (s16)unk48;
				unk10->getMaterialNodePointer(i)->getTevBlock()->setTevColor(
				    0, &c);
			}
			unk18->setFrame(unk1C.getFrame());
			unk30->setFrame(unk34.getFrame());
			unk14->entry();
		}
	}

	if (param & 4) {
		if (visible)
			unk14->viewCalc();
	}
}
