#include <Camera/LensFlare.hpp>
#include <Camera/SunMgr.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <stdio.h>

extern const char* cSunVolumeName;

TLensFlare::TLensFlare(const char* name)
    : JDrama::TViewObj(name)
{
	unk10 = nullptr;
	unk14 = nullptr;
	unk18 = 0.0f;
	unk1C = 0.0f;
	unk20 = 1.0f;
	unk24 = 0.0f;
	unk28 = 0.0f;
	unk2C = 1.0f;
	unk30 = 0.5f;
	unk34 = 1.0f;
	unk38 = 1.0f;
	unk3C = 1.0f;
	unk40 = 1.5f;
	unk44 = 1.0f;
	unk48 = 0.0f;

	if ((gpSunMgr->unk15 & 2) != 0) {
		return;
	}

	char buf[0x100];
	snprintf(buf, 0x100, "%s/%s", cSunVolumeName, "lensflare.bmd");
	void* res = JKRFileLoader::getGlbResource(buf);
	unk10     = (J3DModelData*)J3DModelLoaderDataBase::load(res, 0x10020000);
	J3DModel* model = new J3DModel();
	if (model) {
		model = new (model) J3DModel(unk10, 0, 1);
	}
	unk14 = model;
}

void TLensFlare::perform(u32 flags, JDrama::TGraphics* gfx)
{
	(void)flags;
	(void)gfx;
	// TODO: draw lens flare (complex, ~1500 bytes)
}

template <class T> T CLBEaseOutInbetween(T a, T b, T t)
{
	return CLBTwoDegreeGeneralInbetween<T>(a, b, t, a - b);
}

template f32 CLBEaseOutInbetween<f32>(f32, f32, f32);
