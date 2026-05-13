#include <System/MSoundMainSide.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>

bool MSMainProc::MSStageInfo::bossNotDamaged;
bool MSMainProc::MSStageInfo::bossLives;
bool MSMainProc::MSStageInfo::bossLives2;

void MSStage::setPosPtr(Vec*) { }

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

void MSMainProc::fromTalkingCameraDemo(bool) { gpMSound->talkModeIn(false); }

void MSMainProc::fromInnerCameraDemo() { gpMSound->unkCA = 0; }
void MSMainProc::toInnerCameraDemo() { gpMSound->unkCA = 1; }
