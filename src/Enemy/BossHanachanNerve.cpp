#include <Enemy/BossHanachan.hpp>
#include <Enemy/BossHanachanSaveParams.hpp>

#include <GC2D/GCConsole2.hpp>
#include <MSound/MSModBgm.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <Strategic/Spine.hpp>
#include <System/MarDirector.hpp>

DEFINE_NERVE(TNerveBossHanachanDead, TLiveActor)
{
	TBossHanachan* boss = (TBossHanachan*)spine->getBody();
	boss->considerSetAnm(BHANM_NERVE_5);
	if (!(boss->mLiveFlag & 0x40000) && boss->isAllBckAlreadyEnd(BHANM_KIND_0F)) {
		boss->mLiveFlag |= 0x40000;
		boss->removeAllMapCollision();
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBossHanachanSnort, TLiveActor)
{
	TBossHanachan* boss = (TBossHanachan*)spine->getBody();
	if (spine->getTime() == 200 && boss->checkLiveFlag(0x20000)) {
		boss->offLiveFlag(0x20000);
		MSBgm::startBGM(0x80010029);
		switch (boss->mTempo) {
		case 2:
			((MSModBgm*)gpMSound->unk98)->changeTempo(0, 1);
			break;
		case 1:
			((MSModBgm*)gpMSound->unk98)->changeTempo(1, 1);
			break;
		}
	}
	boss->considerSetAnm(BHANM_NERVE_4);
	if (boss->isAllBckAlreadyEnd(BHANM_KIND_0E)) {
		boss->goToInitialRecoverGraphNode();
		spine->pushAfterCurrent(&TNerveBossHanachanGraphWander::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBossHanachanDamage, TLiveActor)
{
	TBossHanachan* boss = (TBossHanachan*)spine->getBody();
	boss->considerSetAnm(BHANM_NERVE_3);
	boss->execSlip();
	if (0.0f == boss->unk140
	    && spine->getTime() >= boss->mChangeParams->mSLDamageFrames.value) {
		boss->setAnmTimerWhenGetUp();
		spine->pushAfterCurrent(&TNerveBossHanachanGetUp::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBossHanachanGetUp, TLiveActor)
{
	TBossHanachan* boss = (TBossHanachan*)spine->getBody();
	boss->considerSetAnm(BHANM_NERVE_2);
	if (boss->isFinishedGetUp()) {
		boss->setRandomWeakBodyIndex();
		boss->setAnmTimerWhenSnort();
		spine->pushAfterCurrent(&TNerveBossHanachanSnort::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBossHanachanDown, TLiveActor)
{
	TBossHanachan* boss = (TBossHanachan*)spine->getBody();
	boss->considerSetAnm(BHANM_NERVE_1);
	if (spine->getTime() >= boss->mChangeParams->mSLDownFrames.value) {
		boss->setAnmTimerWhenGetUp();
		spine->pushAfterCurrent(&TNerveBossHanachanGetUp::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBossHanachanTumble, TLiveActor)
{
	TBossHanachan* boss = (TBossHanachan*)spine->getBody();
	if (spine->getTime() == 0)
		boss->setTumbleAnm(BHANM_STOP_ON);
	else
		boss->considerSetAnm(BHANM_NERVE_0);
	boss->execSlip();
	if (0.0f == boss->unk140 && boss->isTumbleCompletelyAllBody()) {
		gpMarDirector->getConsole()->startAppearBalloon(0xE0007, true);
		spine->pushAfterCurrent(&TNerveBossHanachanDown::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBossHanachanGraphWander, TLiveActor)
{
	TBossHanachan* boss = (TBossHanachan*)spine->getBody();
	if (spine->getTime() == 0)
		boss->setHeadAndBodyAnm(BHANM_KIND_00, BHANM_STOP_ON);
	boss->execWalk(true);
	if (boss->checkFallDecideAndSetup()) {
		spine->pushAfterCurrent(&TNerveBossHanachanTumble::theNerve());
		return TRUE;
	}
	return FALSE;
}
