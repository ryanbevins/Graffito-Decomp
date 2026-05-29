#include <Enemy/Kukku.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/ObjModel.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <System/Application.hpp>
#include <System/MarDirector.hpp>
#include <Map/PollutionManager.hpp>
#include <JSystem/JDrama/JDRGraphics.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

static char* tori_bastable[] = {
	(char*)"/scene/tori/bas/tori_back.bas",
	(char*)"/scene/tori/bas/tori_down.bas",
	(char*)0,
	(char*)"/scene/tori/bas/tori_hit.bas",
	(char*)"/scene/tori/bas/tori_wait.bas",
	(char*)"/scene/tori/bas/tori_fall_end.bas",
};

DEFINE_NERVE(TNerveKukkuFall, TLiveActor) { return FALSE; }
DEFINE_NERVE(TNerveKukkuPostFall, TLiveActor) { return FALSE; }
DEFINE_NERVE(TNerveKukkuRecoverGraph, TLiveActor) { return FALSE; }
DEFINE_NERVE(TNerveKukkuGraphWander, TLiveActor) { return FALSE; }

void TKukkuManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
	    { "tori.bmd", 0x10210000, 0 },
	    { nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TKukkuManager::TKukkuManager(const char* name)
    : TSmallEnemyManager(name)
{
}

const char** TKukku::getBasNameTable() const
{
	return (const char**)tori_bastable;
}

void TKukku::setDeadAnm()
{
	getMActor()->setBck("tori_down");
	setCurAnmSound();
}

void TKukku::setAfterDeadEffect()
{
	TSmallEnemy::setAfterDeadEffect();
	gpPollution->stamp(((TSmallEnemyManager*)mManager)->getUnk58(), mPosition.x,
	                   mPosition.y, mPosition.z, 1000.0f);
}

void TKukku::bind() { TLiveActor::bind(); }

void TKukku::control()
{
	if (unk1A4 > 0)
		unk1A4 -= 1;
	TLiveActor::control();
}

void TKukku::reset()
{
	unk1A4 = 0;
	unk1AC = 0;
	mGravity = 0.0f;
	unk1B0 = 0;
	onLiveFlag(LIVE_FLAG_AIRBORNE);
	mScaledBodyRadius = 75.0f;
}

BOOL TKukku::receiveMessage(THitActor* sender, u32 message)
{
	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return FALSE;

	if (message < 2 && (s32)message >= 0) {
		mSpine->reset();
		mSpine->setNext(&TNerveSmallEnemyDie::theNerve());
		return TRUE;
	}
	return TSmallEnemy::receiveMessage(sender, message);
}

void TKukku::perform(u32 action, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(action, graphics);

	for (void** ball = &unk194[0]; ball != &unk1A0; ++ball) {
		THitActor* a = (THitActor*)*ball;
		a->perform(action, graphics);
	}
}

TKukku::TKukku(const char* name)
    : TSmallEnemy(name)
{
	unk1A0 = nullptr;
}
