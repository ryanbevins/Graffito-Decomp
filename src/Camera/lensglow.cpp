#include <Camera/LensGlow.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DMaterialAnm.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DAnmLoader.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <stdio.h>

static const char dummyMactorStringValue1[] = "\0\0\0\0\0\0\0\0\0\0\0";
static const char* SMS_NO_MEMORY_MESSAGE     = "メモリが足りません\n";

const char* cSunVolumeName    = "/scene/sun";
const char* cSunsetVolumeName = "/scene/sunset";

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
	unk18 = (J3DAnmBase*)J3DAnmLoaderDataBase::load(
	    JKRFileLoader::getGlbResource(buf));
	((J3DAnmTextureSRTKey*)unk18)->searchUpdateMaterialID(unk10);

	snprintf(buf, 0x100, "%s/%s", volume, "glow.brk");
	unk30 = (J3DAnmBase*)J3DAnmLoaderDataBase::load(
	    JKRFileLoader::getGlbResource(buf));
	((J3DAnmTevRegKey*)unk30)->searchUpdateMaterialID(unk10);

	int num = unk10->getMaterialNum();
	for (u16 i = 0; i < num; i++) {
		J3DMaterialAnm* anm = new J3DMaterialAnm();
		unk10->getMaterialNodePointer(i)->change();
		unk10->getMaterialNodePointer(i)->unk38 = anm;
	}

	unk10->entryTexMtxAnimator((J3DAnmTextureSRTKey*)unk18);
	unk10->entryTevRegAnimator((J3DAnmTevRegKey*)unk30);

	unk1C.init(unk18->getFrameMax());
	unk1C.setRate(SMSGetAnmFrameRate());
	unk1C.setAttribute(2);

	unk34.init(unk30->getFrameMax());
	unk34.setRate(SMSGetAnmFrameRate());
	unk34.setAttribute(2);

	unk74 = 500.0f;
	unk78 = 500.0f;
	unk7C = 0.0f;
	unk64 = unk6C;
	unk60 = unk6C;
}

void TLensGlow::perform(u32 flags, JDrama::TGraphics* gfx)
{
	(void)flags;
	(void)gfx;
	// TODO: draw lens glow with animations
}
