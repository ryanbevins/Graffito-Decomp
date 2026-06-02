#include <System/MSoundMainSide.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <Player/MarioAccess.hpp>
#include <System/MarDirector.hpp>

static const u32 cMSBgmNone = 0xfffffff0;

u32 MSMainProc::MSStageInfo::msStg;
u32 MSMainProc::MSStageInfo::demoBgm;
u32 MSMainProc::MSStageInfo::stageBgm;
u32 MSMainProc::MSStageInfo::stageBgmSilent;
u8 MSMainProc::MSStageInfo::stageBgmSilentStartStatus;
u8 MSMainProc::MSStageInfo::flags;
u16 MSMainProc::MSStageInfo::volOffCategory;
u8 MSMainProc::MSStageInfo::fadeEvent;
u32 MSMainProc::MSStageInfo::switchBgm;
u32 MSMainProc::MSStageInfo::switchBgm2;
f32 MSMainProc::MSStageInfo::cubeFadeRatio;
bool MSMainProc::MSStageInfo::cubeFadeUsePan;
bool MSMainProc::MSStageInfo::bossLives;
bool MSMainProc::MSStageInfo::bossLives2;
bool MSMainProc::MSStageInfo::bossNotDamaged;
bool MSMainProc::MSStageInfo::distFadeStageToKage;
MSStage* MSStage::smMSStage;

void MSStage::setPosPtr(Vec*) { }

void MSSTageSimpleEnvironment::proc()
{
	if (mSoundID != 0xffffffff)
		MSoundSESystem::MSoundSE::startSoundSystemSE(mSoundID, 0, nullptr, 0);
}

MSStageDistFade::MSStageDistFade(const Vec* pos, float near_dist,
                                 float far_dist, u32 bgm, bool use_pan)
    : unk4(0)
    , unk8(near_dist == 0.0f ? 2000.0f : near_dist)
    , unkC(far_dist == 0.0f ? 500.0f : far_dist)
    , unk10(pos)
    , unk14(bgm)
    , unk18(use_pan)
{
}

void MSStage::stageLoop() { proc(); }

void MSMainProc::setBossNotDamagedFlag(bool flag)
{
	MSStageInfo::bossNotDamaged = flag;
}

void MSMainProc::setBossLivesFlagOnlyFlag(bool flag)
{
	MSStageInfo::bossLives = flag;
}

void MSMainProc::setBossLivesFlag2(bool flag)
{
	MSBgm::stopTrackBGM(1, 10);
	MSBgm::setTrackVolume(0, 1.0f, 18, 0);
	MSStageInfo::bossLives2 = flag;
}

void MSMainProc::setBossLivesFlag(bool flag)
{
	MSBgm::stopTrackBGM(1, 10);
	MSBgm::setTrackVolume(0, 1.0f, 18, 0);
	MSStageInfo::bossLives = flag;
}

void MSMainProc::startStageBGM(u8, u8)
{
	gpMSound->initSound();
	gpMSound->unkA0 = 0;
	gpMSound->unkC9 = 0;

	if (MSStageInfo::stageBgm != cMSBgmNone) {
		bool shouldStart = false;
		if (MSStageInfo::flags & 2)
			shouldStart = true;

		if (shouldStart) {
			if (MSStageInfo::flags & 1) {
				JAISound* sound = MSBgm::startBGM(MSStageInfo::stageBgm);
				if (sound != nullptr)
					sound->setVolume(0.0f, 0, 0);
			} else {
				MSBgm::startBGM(MSStageInfo::stageBgm);
			}
		}
	}

	if (MSStageInfo::stageBgmSilent != cMSBgmNone
	    && MSStageInfo::stageBgmSilentStartStatus == 2
	    && gpMSound->unkCF != 0) {
		JAISound* sound = MSBgm::startBGM(MSStageInfo::stageBgmSilent);
		if (sound != nullptr)
			sound->setVolume(0.0f, 0, 0);
	}
}

void MSMainProc::endStageEntranceDemo(u8, u8)
{
	if (MSStageInfo::demoBgm != cMSBgmNone) {
		bool stopDemo = false;
		if (MSStageInfo::flags & 8)
			stopDemo = true;

		if (stopDemo)
			MSBgm::stopBGM(MSStageInfo::demoBgm, 20);
	}

	bool silentDemo = false;
	if (MSStageInfo::flags & 4)
		silentDemo = true;

	if (silentDemo)
		MSBgm::setVolume(MSStageInfo::demoBgm, 0.0f, 20, 0);

	gpMSound->demoModeOut(false);
	gpMSound->unkCC = 0;
}

void MSMainProc::entranceDemoLoop(u32) { }

void MSMainProc::startStageEntranceDemo(u8, u8)
{
	if (MSStageInfo::demoBgm != cMSBgmNone)
		MSBgm::startBGM(MSStageInfo::demoBgm);

	if (MSStageInfo::stageBgmSilent != cMSBgmNone
	    && MSStageInfo::stageBgmSilentStartStatus == 1
	    && gpMSound->unkCF != 0) {
		JAISound* sound = MSBgm::startBGM(MSStageInfo::stageBgmSilent);
		if (sound != nullptr)
			sound->setVolume(0.0f, 0, 0);
	}

	gpMSound->demoModeIn(MSStageInfo::volOffCategory, false);
	gpMSound->unkA4 = 0;
	gpMSound->unkC9 = 1;
}

void MSMainProc::fromTalkingCameraDemo(bool) { gpMSound->talkModeIn(false); }

void MSMainProc::toTalkingCameraDemo()
{
	u16 category = 0;
	switch (gpMarDirector->mMap) {
	case 1:
	case 4:
	case 9:
		category = 4;
		break;
	}
	gpMSound->setCategoryVOLs(category, 1.0f);
}

void MSMainProc::fromInnerCameraDemo() { gpMSound->unkCA = 0; }
void MSMainProc::toInnerCameraDemo() { gpMSound->unkCA = 1; }

u32 MSMainProc::getMonteVillageActorArea(const Vec& pos)
{
	u32 area = 4;
	if (MSGMSound->unkCD == 8) {
		Vec checkPos = pos;
		checkPos.y += 75.0f;
		switch (gpCubeFastC->getInCubeNo(checkPos)) {
		case 0:
			area = 2;
			break;
		case 1:
			area = 0;
			break;
		default:
			area = 3;
			break;
		}
	}
	return area;
}
