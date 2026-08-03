#include <Enemy/EffectEnemy.hpp>
#include <Enemy/WalkerEnemy.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <Player/MarioAccess.hpp>
#include <System/EmitterViewObj.hpp>
#include <Strategic/ObjModel.hpp>
#include <M3DUtil/MActor.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <dolphin/mtx.h>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

TEffectEnemy::TEffectEnemy(const char* name)
    : TWalkerEnemy(name)
    , mUnk194(0)
{
}

void TEffectEnemy::init(TLiveManager* mgr)
{
	TWalkerEnemy::init(mgr);
	mActorType = 0x10000005;
}

void TEffectEnemy::setMActorAndKeeper()
{
	mMActorKeeper       = new TMActorKeeper(mManager, 1);
	mMActor             = mMActorKeeper->createMActor("default.bmd", 3);
}

void TEffectEnemy::reset()
{
	TWalkerEnemy::reset();
}

void TEffectEnemy::behaveToWater(THitActor* hit)
{
	if (mHitPoints > 1) {
		TSmallEnemy::behaveToWater(hit);
	} else {
		kill();
	}
}

void TEffectEnemy::sendAttackMsgToMario()
{
	switch (mUnk194) {
	case 0:
		SMS_SendMessageToMario(this, 0xA);
		kill();
		break;
	case 1: SMS_SendMessageToMario(this, 9); break;
	default: SMS_SendMessageToMario(this, 0xE); break;
	}
}

void TEffectEnemy::setDeadAnm()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0x8B, &mPosition, 0, nullptr);
	if (gpMSound->gateCheck(0x28C5)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x28C5, (const Vec*)&mPosition,
		                                          0, nullptr, 0, 4);
	}
	mLiveFlag |= 0x20000;
}

void TEffectEnemy::kill()
{
	setDeadAnm();
	mLiveFlag |= LIVE_FLAG_DEAD;
	unk64 |= 1;
}

void TEffectEnemy::forceKill()
{
	if (!(mGroundPlane->isIllegalData()
	      || (!mGroundPlane->isDeathPlane() && !mGroundPlane->isPool()
	          && !mGroundPlane->isWaterSurface())
	      || isAirborne() || checkLiveFlag(LIVE_FLAG_UNK10))
	    || !gpMap->isInArea(mPosition.x, mPosition.z)) {
		kill();
	}
}

void TEffectEnemy::perform(u32 param_1, JDrama::TGraphics* gfx)
{
	if (!(mLiveFlag & LIVE_FLAG_DEAD)) {
		if (param_1 & 1) {
			TWalkerEnemy::moveObject();
		}
		if ((param_1 & 2) && !(mLiveFlag & LIVE_FLAG_CLIPPED_OUT)) {
			u8 maxHp
			    = getSaveParam() ? getSaveParam()->mSLHitPointMax.get() : 1;
			u8 hitPoints = mHitPoints;
			Vec scaled;
			PSVECScale((Vec*)&mScaling, &scaled,
			           (f32)((int)hitPoints / (int)maxHp));
			gpMarioParticleManager->emitAndBindToPosPtr(0x1ED, &mPosition, 3,
			                                            this);
			gpMarioParticleManager->emitAndBindToPosPtr(0x135, &mPosition, 1,
			                                            this);
			gpMarioParticleManager->emitAndBindToPosPtr(0x136, &mPosition, 1,
			                                            this);
			gpMarioParticleManager->emitAndBindToPosPtr(0x137, &mPosition, 1,
			                                            this);
		}
		THitActor::perform(param_1, gfx);
	}
}

void TEffectEnemyManager::initSetEnemies() { }

TSpineEnemy* TEffectEnemyManager::createEnemyInstance()
{
	return new TEffectEnemy("\x83\x47\x83\x74\x83\x46\x83\x4E\x83\x67\x93\x47");
}

void TEffectEnemyManager::loadAfter() { JDrama::TNameRef::loadAfter(); }

void TEffectEnemyManager::load(JSUMemoryInputStream& in)
{
	TSmallEnemyManager::load(in);
	unk38 = new TWalkerEnemyParams("/enemy/moveFireEffect.prm");
}
