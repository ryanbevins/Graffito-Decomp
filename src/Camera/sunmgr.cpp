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

static const char dummyMactorStringValue1[] = "\0\0\0\0\0\0\0\0\0\0\0";
static const char SMS_NO_MEMORY_MESSAGE[]   = "メモリが足りません\n";
static const char cSunSceneName[]           = "/scene/sun";
static const char cSunsetSceneName[]        = "/scene/sunset";
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
	unk24    = 0.0f;
	unk28    = 0.0f;
	unk2C    = 0.0f;
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

	*(u32*)&unk18 = (r << 8) | g;
	*(u32*)&unk1C = (b << 8) | a;

	if (JDrama::TNameRefGen::search<JDrama::TNameRef>("太陽モデル")) {
		unk14 = 1;
	} else if (JDrama::TNameRefGen::search<JDrama::TNameRef>("夕日モデル")) {
		unk14 = 1;
		unk15 |= 0x2;
	}

	if (unk14 && gpMarDirector->mMap == 1
	    && TFlagManager::smInstance->getBool(0x50004)) {
		unk15 |= 0x1;
		TStagePositionInfo* p = static_cast<TStagePositionInfo*>(
		    gpPositionHolder->search(cSunWarpPointName));
		unk24 = p->unkC.x;
		unk28 = p->unkC.y;
		unk2C = p->unkC.z;
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

	BOOL inMode = FALSE;
	if (gpCamera->isLButtonCameraSpecifyMode(gpCamera->mMode)
	    && !gpCamera->isNowInbetween())
		inMode = TRUE;
	if (!inMode)
		return;

	f32 dz = gpMarioPos->z - unk2C;
	f32 dx = gpMarioPos->x - unk24;
	if (dx * dx + dz * dz >= 160000.0f)
		return;

	TSunModel* sm = gpSunModel;
	BOOL a = FALSE;
	BOOL b = FALSE;
	BOOL c = FALSE;
	if (sm->unkF8 >= -0.3f && sm->unkF8 <= 0.3f)
		a = TRUE;
	if (a && sm->unkFC >= -0.3f)
		b = TRUE;
	if (b && sm->unkFC <= 0.3f)
		c = TRUE;
	if (!c)
		return;

	gpMarDirector->setNextStage(9, nullptr);
	JAISound* snd = (JAISound*)gpMSound->unk7C;
	if (snd == nullptr)
		return;
	snd->setVolume(0.0f, 0x64, 0);
	((JAISound*)gpMSound->unk7C)->setPitch(1.3f, 0x64, 0);
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
