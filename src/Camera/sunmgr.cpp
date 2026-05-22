#include <Camera/SunMgr.hpp>
#include <Camera/SunModel.hpp>
#include <Camera/Camera.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <System/MarDirector.hpp>
#include <System/FlagManager.hpp>
#include <System/PositionHolder.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <Player/MarioAccess.hpp>
#include <JSystem/JAudio/JAInterface/JAISound.hpp>

TSunMgr* gpSunMgr;
extern const char* cSunWarpPointName;

TSunMgr::TSunMgr(const char* name)
    : JDrama::TViewObj(name)
    , TDrawSyncCallback()
{
	unk14    = 0;
	unk15    = 0;
	*(s32*)&unk18 = -1;
	*(s32*)&unk1C = -1;
	unk20    = 0.0f;
	unk24    = 0.0f;
	unk28    = 0.0f;
	unk2C    = 0.0f;
	gpSunMgr = this;
}

TSunMgr::~TSunMgr() { }

void TSunMgr::drawSyncCallback(unsigned short token) { (void)token; }

s32 TSunMgr::getAddColor() const
{
	s32 alpha = 0;
	if (unk14)
		alpha = (s32)gpSunModel->unkAC;
	return alpha;
}

void TSunMgr::perform(unsigned long flags, JDrama::TGraphics* gfx)
{
	if (!(unk15 & 1))
		return;
	if (!(flags & 1))
		return;
	(void)gfx;

	if (!(gpCamera->isLButtonCameraSpecifyMode(gpCamera->mMode)
	      && !gpCamera->isNowInbetween()))
		return;

	f32 dx = gpMarioPos->x - unk24;
	f32 dz = gpMarioPos->z - unk2C;
	if (dx * dx + dz * dz >= 0.0f)
		return;

	(void)dx;
	(void)dz;

	// Sun warp logic (TODO match exactly)
}

void TSunMgr::load(JSUMemoryInputStream& stream)
{
	JDrama::TNameRef::load(stream);
	u32 r, g, b, a;
	stream.read(&r, 4);
	stream.read(&g, 4);
	stream.read(&b, 4);
	stream.read(&a, 4);
	stream.read(&unk20, 4);

	*(u32*)&unk18 = (r << 8) | g;
	*(u32*)&unk1C = (b << 8) | a;

	if (JDrama::TNameRefGen::search<JDrama::TNameRef>(
	        "\x90\xBC\x82\xCC\x91\xBE\x97\x7A")) {
		unk14 = 1;
	}
	// TODO sun warp lookup
}
