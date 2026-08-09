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

static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";
static const char* SMS_NO_MEMORY_MESSAGE    = "メモリが足りません\n";
static const char* cSunSceneName            = "/scene/sun";
static const char* cSunsetSceneName         = "/scene/sunset";
const char* cSunWarpPointName               = "太陽ワープポイント";

TSunMgr* gpSunMgr;

TSunMgr::TSunMgr(const char* name)
    : JDrama::TViewObj(name)
    , TDrawSyncCallback()
{
	unk14    = 0;
	unk15    = 0;
	*(s32*)&unk18 = -1;
	*(s32*)&unk1C = -1;
	unk20    = 0.0f;
	unk24.x  = 0.0f;
	unk24.y  = 0.0f;
	unk24.z  = 0.0f;
	gpSunMgr = this;
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

	u32 color1    = (r << 8) | g;
	u32 color2    = (b << 8) | a;
	*(u32*)&unk18 = color1;
	*(u32*)&unk1C = color2;

	if (JDrama::TNameRefGen::search<JDrama::TNameRef>("太陽モデル")) {
		unk14 = 1;
	} else if (JDrama::TNameRefGen::search<JDrama::TNameRef>("夕日モデル")) {
		unk14 = 1;
		unk15 |= 0x2;
	}

	if (unk14 && gpMarDirector->mMap == 1
	    && TFlagManager::smInstance->getBool(0x50004)) {
		unk15 |= 0x1;
		const char* warpName = cSunWarpPointName;
		TStagePositionInfo* p = static_cast<TStagePositionInfo*>(
		    gpPositionHolder->searchF(
		        JDrama::TNameRef::calcKeyCode(warpName), warpName));
		unk24 = p->unkC;
	}
}

void TSunMgr::perform(unsigned long flags, JDrama::TGraphics* gfx)
{
	if (!(unk15 & 1))
		return;
	if (!(flags & 1))
		return;
	if (!(gfx->unk0 & 0x2))
		return;

	CPolarSubCamera* cam = gpCamera;
	bool inMode = false;
	if (cam->isLButtonCameraSpecifyMode(cam->mMode)
	    && !cam->isNowInbetween())
		inMode = true;
	if (!(inMode ? true : false))
		return;

	f32 dz = gpMarioPos->z - unk24.z;
	f32 dx = gpMarioPos->x - unk24.x;
	if (!(dx * dx + dz * dz < 160000.0f))
		return;

	TSunModel* sm = gpSunModel;
	f32* uv = &sm->mFPos[0].x;
	bool c = false;
	bool b = c;
	bool a = c;
	if (-0.3f <= uv[0] && uv[0] <= 0.3f)
		a = true;
	if (a && -0.3f <= uv[1])
		b = true;
	if (b && uv[1] <= 0.3f)
		c = true;
	if (!(c ? true : false))
		return;

	gpMarDirector->setNextStage(9, nullptr);
	MSound* ms = gpMSound;
	if (ms->unk7C == nullptr)
		return;
	((JAISound*)ms->unk7C)->setVolume(0.0f, 0x64, 0);
	((JAISound*)ms->unk7C)->setPitch(1.3f, 0x64, 0);
}

s32 TSunMgr::getAddColor() const
{
	s32 alpha = 0;
	if (unk14)
		alpha = (s32)gpSunModel->unkAC;
	return alpha;
}

void TSunMgr::drawSyncCallback(unsigned short token)
{
	(void)token;
	if (unk14)
		gpSunModel->getZBufValue();
}

TSunMgr::~TSunMgr() { }
