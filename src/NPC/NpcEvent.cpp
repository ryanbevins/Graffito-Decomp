#include <NPC/NpcEvent.hpp>
#include <NPC/NpcBase.hpp>
#include <System/FlagManager.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>

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
