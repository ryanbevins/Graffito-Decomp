#include <NPC/NpcEvent.hpp>
#include <System/FlagManager.hpp>

u32 TNpcEvent::mDownSunflowerNum;

void TNpcEvent::initDownSunflowerNum()
{
	if (TFlagManager::smInstance->getBool(0x50003)) {
		mDownSunflowerNum = 5;
	} else {
		mDownSunflowerNum = 0;
	}
}

void TNpcEvent::reviveOneSunflower() { }

void TNpcEvent::initNpcBuiltin(TSpcTypedBinary<TEventWatcher>* binary)
{
	(void)binary;
}
