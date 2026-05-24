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
	if (mUnk194 == 1) {
		SMS_SendMessageToMario(this, 9);
	} else if (mUnk194 < 0 || mUnk194 > 1) {
		SMS_SendMessageToMario(this, 0xE);
	} else {
		SMS_SendMessageToMario(this, 0xA);
		kill();
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
	mActorType |= 1;
}

void TEffectEnemy::forceKill()
{
	bool flag = mGroundPlane->isIllegalData() ? true : false;
	if (!flag) {
		u16 t = mGroundPlane->mBGType;
		flag  = (t == BG_TYPE_DEATH_PLANE) ? true : false;
		if (!flag) {
			flag = (t == BG_TYPE_POOL || t == BG_TYPE_INDOOR_POOL
			        || t == BG_TYPE_SHADED_POOL)
			    ? true
			    : false;
			if (!flag) {
				flag = (t == BG_TYPE_WATER || t == BG_TYPE_DAMAGING_WATER
				        || (u16)(t - BG_TYPE_SEA_WATER) <= 3
				        || t == BG_TYPE_SHADED_POOL)
				    ? true
				    : false;
				if (!flag) {
					u32 lf       = mLiveFlag;
					bool airborn = (lf & LIVE_FLAG_AIRBORNE) ? true : false;
					if (!airborn) {
						if (!(lf & LIVE_FLAG_UNK10)) {
							kill();
							return;
						}
					}
				}
			}
		}
	}
	if (!gpMap->isInArea(mPosition.x, mPosition.z)) {
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
			u32 maxHp = getSaveParam() ? getSaveParam()->mSLHitPointMax.value
			                           : 1;
			Vec scaled;
			PSVECScale((Vec*)&mScaling, &scaled,
			           (f32)(int)((u32)mHitPoints / maxHp));
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
