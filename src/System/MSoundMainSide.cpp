#include <System/MSoundMainSide.hpp>

bool MSMainProc::MSStageInfo::bossNotDamaged;
bool MSMainProc::MSStageInfo::bossLives;

void MSStage::setPosPtr(Vec*) { }

void MSMainProc::setBossNotDamagedFlag(bool flag)
{
	MSStageInfo::bossNotDamaged = flag;
}

void MSMainProc::setBossLivesFlagOnlyFlag(bool flag)
{
	MSStageInfo::bossLives = flag;
}
