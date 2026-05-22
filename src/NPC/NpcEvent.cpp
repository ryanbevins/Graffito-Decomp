#include <NPC/NpcEvent.hpp>
#include <MSound/MSoundBGM.hpp>
#include <NPC/NpcBase.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <Strategic/SpcInterp.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>

static void evIsDemoMode(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	BOOL isDemo;
	if (gpMarDirector->unk124 == 3) isDemo = TRUE;
	else if (gpMarDirector->unk124 == 4) isDemo = TRUE;
	else isDemo = FALSE;
	interp->push(TSpcSlice((int)isDemo));
}

static void evIsGameModeNormal(TSpcTypedInterp<TEventWatcher>* interp,
                               u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	BOOL isNormal;
	if (gpMarDirector->unk124 == 0) isNormal = TRUE;
	else isNormal = FALSE;
	interp->push(TSpcSlice((int)isNormal));
}

u32 TNpcEvent::mDownSunflowerNum;

void TNpcEvent::initDownSunflowerNum()
{
	if (TFlagManager::smInstance->getBool(0x50003)) {
		mDownSunflowerNum = 5;
	} else {
		mDownSunflowerNum = 0;
	}
}

static bool ReviveSunflowerCallBack(TBaseNPC* npc, u32 phase)
{
	if (phase == 0) {
		npc->sunflowerReviveIn();
		u32 soundID = 0x8808;
		if (TNpcEvent::mDownSunflowerNum == 0)
			soundID = 0x4847;
		if (gpMSound->gateCheck(soundID)) {
			MSoundSESystem::MSoundSE::startSoundNpcActor(
			    soundID, (const Vec*)((u8*)npc + 0x10), 0, (JAISound**)NULL, 0, 4);
		}
	}
	return true;
}

void TNpcEvent::reviveOneSunflower() { (void)ReviveSunflowerCallBack; }

void TNpcEvent::initNpcBuiltin(TSpcTypedBinary<TEventWatcher>* binary)
{
	(void)binary;
}
