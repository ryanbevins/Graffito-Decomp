#include <Camera/LensGlow.hpp>

extern const char* cSunVolumeName;
extern const char* cSunsetVolumeName;

TLensGlow::TLensGlow(bool sunset, const char* name)
    : JDrama::TViewObj(name)
{
	unk10 = nullptr;
	unk14 = nullptr;
	unk18 = nullptr;
	// unk1C is J3DFrameCtrl with its own ctor
	unk30 = nullptr;

	unk48 = 0.0f;
	unk4C = 0.0f;
	unk50 = 1.0f;
	unk54 = 1.0f;
	unk58 = 1.0f;
	unk5C = 0xFF;
	unk5D = 2;
	unk68 = 0.0f;
	unk6C = 0.0f;
	unk70 = 1.0f;
	unk74 = 0.0f;
	unk78 = 0.0f;
	unk7C = 0.0f;
	unk80 = 0.0f;
	unk84 = 0.0f;
	unk88 = 0.0f;
	unk8C = 0.0f;
	unk90 = 1.0f;
	unk94 = 100.0f;

	if (sunset) {
		unk68 = 0.5f;
		unk6C = 0.0f;
	}

	// TODO: load model + 2 anims from cSunVolumeName (or cSunsetVolumeName if
	// sunset) + create J3DMaterialAnm array per material. Approximately 600 bytes
	// of asm.
}

void TLensGlow::perform(u32 flags, JDrama::TGraphics* gfx)
{
	(void)flags;
	(void)gfx;
	// TODO: draw lens glow with animations
}
