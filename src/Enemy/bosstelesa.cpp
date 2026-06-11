#include <Enemy/BossTelesa.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/HamuKuri.hpp>
#include <GC2D/GCConsole2.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MSound/MSound.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/Spine.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarDirector.hpp>
#include <dolphin/mtx.h>
#include <math.h>
#include <stdlib.h>

static const char* btelesa_bastable[] = {
	"/scene/btelesa/bas/btelesa_appear.bas",
	"/scene/btelesa/bas/btelesa_bero_hit.bas",
	"/scene/btelesa/bas/btelesa_damage.bas",
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	"/scene/btelesa/bas/btelesa_lick.bas",
	nullptr,
	nullptr,
	nullptr,
	"/scene/btelesa/bas/btelesa_roll.bas",
	"/scene/btelesa/bas/btelesa_spicy.bas",
	nullptr,
	nullptr,
	"/scene/btelesa/bas/btelesa_wait.bas",
	"/scene/btelesa/bas/btelesa_wet.bas",
};

f32 TBossTelesa::mEnemyGenRate           = 0.5f;
f32 TBossTelesa::mItemGenRate            = 0.1f;
u8 TBossTelesa::mNormalAlpha             = 0x96;
f32 TBossTelesa::mBaseHoseiPosY          = -300.0f;
f32 TBossTelesa::mRouletteUpRate         = 0.03f;
u32 TBossTelesa::mTelesaGenerateInterval = 400;
f32 TBossTelesa::mCameraMoveLimit        = 1000.0f;
f32 TBossTelesa::mCameraMoveSp           = 0.02f;

static inline TBossTelesa* getBoss(TSpineBase<TLiveActor>* spine)
{
	return (TBossTelesa*)spine->getBody();
}

static inline TBubble* getBubble(TSpineBase<TLiveActor>* spine)
{
	return (TBubble*)spine->getBody();
}

static inline TBossTelesa* getSlotOwner(TTelesaSlot* slot)
{
	return (TBossTelesa*)slot->unk1A0;
}

TBossTelesaSaveLoadParams::TBossTelesaSaveLoadParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mSLDamageRadius, 200)
    , PARAM_INIT(mSLDamageHeight, 100)
    , PARAM_INIT(mSLAttackRadius, 220)
    , PARAM_INIT(mSLAttackHeight, 120)
    , PARAM_INIT(mSLGenAttackerTime, 500)
    , PARAM_INIT(mSLGenBubbleTime, 600)
    , PARAM_INIT(mSLHitAngle, 20.0f)
    , PARAM_INIT(mSLNumGenBubble, 5)
    , PARAM_INIT(mSL1stBubbleSp, 10.0f)
    , PARAM_INIT(mSLHideAreaRadius, 500.0f)
    , PARAM_INIT(mSLSlotItemNum, 5)
    , PARAM_INIT(mSLSlotFruitNum, 10)
    , PARAM_INIT(mSLSlotFirstHitCollectRate, 0.1f)
    , PARAM_INIT(mSLSlotHitCollectRate, 0.1f)
    , PARAM_INIT(mSLTransYOffset, 350.0f)
    , PARAM_INIT(mSLStopSlotTime0, 3000)
    , PARAM_INIT(mSLStopSlotTime1, 2000)
    , PARAM_INIT(mSLStopSlotTime2, 1000)
    , PARAM_INIT(mSLSpicyTime, 2000)
{
	TParams::load(mPrmPath);
}

TBossTelesa::TBossTelesa(const char* name)
    : TSpineEnemy(name)
    , unk150(1)
    , unk154(nullptr)
    , unk158(nullptr)
    , unk15C(nullptr)
    , unk160(-1)
    , unk164(-1)
    , unk168(0.0f)
    , unk16C(nullptr)
    , unk184(nullptr)
    , unk188(nullptr)
    , unk18C(0)
    , unk1A8(0)
    , unk274(0)
    , unk350(0)
    , unk354(0)
    , unk358(0)
    , unk35A(1)
    , unk35B(1)
    , unk35C(0)
    , unk360(0.0f)
    , unk364(0.0f)
    , unk368(0)
    , unk36C(0)
    , unk370(3)
    , unk384(0)
{
}

void TBossTelesa::loadAfter() { JDrama::TNameRef::loadAfter(); }

void TBossTelesa::perform(u32 flags, JDrama::TGraphics* gfx)
{
	if ((flags & 0x200) && !(mLiveFlag & (LIVE_FLAG_DEAD | LIVE_FLAG_CLIPPED_OUT))) {
		if (mSpine->getCurrentNerve() == &TNerveBossTelesaDie::theNerve()
		    && unk350) {
			mMActor->offMakeDL();
			SMS_AddDamageFogEffect(mMActor->unk4->mModelData, mPosition, gfx);
		}
	}

	offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
	TSpineEnemy::perform(flags, gfx);

	unk16C->THitActor::perform(flags, gfx);
	unk170->THitActor::perform(flags, gfx);
	unk174->THitActor::perform(flags, gfx);
	if (unk184)
		unk184->testPerform(flags, gfx);
	if (unk188)
		unk188->testPerform(flags, gfx);
}

BOOL TBossTelesa::receiveMessage(THitActor*, u32) { return FALSE; }

MtxPtr TBossTelesa::getTakingMtx()
{
	unk278.set(unk178->mMActor->unk4->mNodeMatrices[1]);
	unk278.mMtx[1][3] = unk178->mPosition.y - 120.0f;
	return unk278.mMtx;
}

void TBossTelesa::init(TLiveManager* manager)
{
	TSpineEnemy::init(manager);
	initHitActor(0x08000024, 1, 0x80000000, mBodyRadius, mHeadHeight,
	             mBodyRadius, mHeadHeight);
	mActorType = 0x08000024;

	if (!unk16C)
		unk16C = new TBossTelesaBody("ボステレサ体コリジョン");
	if (!unk170)
		unk170 = new TBossTelesaTongue("ボステレサ舌コリジョン");
	if (!unk174)
		unk174
		    = new TBossTelesaKillSmallEnemy("ボステレサ雑魚敵死コリジョン");

	((TBossTelesaBody*)unk16C)->unk68            = this;
	((TBossTelesaTongue*)unk170)->unk68          = this;
	((TBossTelesaKillSmallEnemy*)unk174)->unk68  = this;

	if (mSpine)
		mSpine->initWith(&TNerveBossTelesaFallDemo::theNerve());
}

void TBossTelesa::calcRootMatrix()
{
	TSpineEnemy::calcRootMatrix();
	if (unk154)
		unk154->mPosition.y = mPosition.y + mBaseHoseiPosY;
}

void TBossTelesa::moveObject()
{
	TSpineEnemy::moveObject();
	if (unk154 && unk154->isRollDrum()) {
		int result = unk154->getSlotResult();
		if (result >= 0)
			flashItem(result);
	}
}

void TBossTelesa::kill()
{
	if (mSpine->getCurrentNerve() != &TNerveBossTelesaDie::theNerve())
		mSpine->pushNerve(&TNerveBossTelesaDie::theNerve());
}

const char** TBossTelesa::getBasNameTable() const { return btelesa_bastable; }

void TBossTelesa::reset()
{
	TSpineEnemy::reset();
	onHitFlag(HIT_FLAG_NO_COLLISION);
	onLiveFlag(LIVE_FLAG_UNK8);
	onLiveFlag(LIVE_FLAG_UNK10);
	onLiveFlag(LIVE_FLAG_HIDDEN);

	unk18C = 0;

	TBossTelesaSaveLoadParams* params = (TBossTelesaSaveLoadParams*)unk15C;
	setHitParams(params->mSLAttackRadius.get(), params->mSLAttackHeight.get(),
	             params->mSLDamageRadius.get(), params->mSLDamageHeight.get());

	gpMarDirector->fireStartDemoCamera("btelesa_roll_camera", nullptr, -1,
	                                    0.0f, true, nullptr, 0, nullptr,
	                                    JDrama::TFlagT<u16>(0));
}

void TBossTelesa::forceHide()
{
	if (mSpine->getCurrentNerve() == &TNerveBossTelesaDie::theNerve())
		return;
	if (mMActor->checkCurBckFromIndex(4))
		return;
	if (mMActor->checkCurBckFromIndex(0))
		return;

	forceAllItemKill();
	unk368 = 0;

	if (unk350) {
		if (gpMSound->gateCheck(0x2968)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x2968, &mPosition, 0, nullptr, 0, 4);
		}
	} else {
		if (gpMSound->gateCheck(0x28D5)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x28D5, &mPosition, 0, nullptr, 0, 4);
		}
	}

	mSpine->reset();
	mSpine->setNext(&TNerveBossTelesaHide::theNerve());
}

void TBossTelesa::forceAllItemKill()
{
	f32 zero = 0.0f;

	for (int i = 0; i < unk274; ++i) {
		if (unk1AC[i]->mHolder) {
			SMS_SendMessageToMario(unk1AC[i], HIT_MESSAGE_UNK8);
			unk1AC[i]->mHolder = nullptr;
		}

		unk1AC[i]->mPosition.set(zero, zero, zero);
		unk1AC[i]->onHitFlag(HIT_FLAG_NO_COLLISION);

		if (!(unk1AC[i]->mLiveFlag & LIVE_FLAG_DEAD)) {
			unk1AC[i]->kill();
			gpMarioParticleManager->emit(0xCD, &unk1AC[i]->mPosition, 0,
			                             nullptr);
		}
	}
}

void TBossTelesa::generateSlotItem()
{
	if (!unk154)
		return;

	switch (unk154->getSlotResult()) {
	case 0:
		genAttacker();
		break;
	case 1:
		if (gpItemManager)
			gpItemManager->makeObjAppear(mPosition.x, mPosition.y,
			                             mPosition.z, 0x2000000E, false);
		break;
	case 2:
		if (gpItemManager)
			gpItemManager->makeObjAppear(mPosition.x, mPosition.y,
			                             mPosition.z, 0x20000002, true);
		break;
	default:
		break;
	}
}

void TBossTelesa::rouletteStart()
{
	unk350 = 1;
	unk168 = 0.0f;
	if (unk154)
		unk154->moveStart();
}

void TBossTelesa::genAttacker()
{
	TConductor* conductor
	    = JDrama::TNameRefGen::search<TConductor>("TConductor");
	if (conductor)
		conductor->makeOneEnemyAppear(mPosition, "テレサ", 1);
	unk36C = mTelesaGenerateInterval;
}

void TBossTelesa::flashItem(int result)
{
	unk160 = result;
	unk164 = result;
	unk168 = 0.0f;
}

void TBossTelesa::slotFall()
{
	if (unk154)
		unk154->mPosition.y -= 5.0f;
}

void TBossTelesa::rouletteFall()
{
	unk168 += mRouletteUpRate;
	if (unk154)
		unk154->mPosition.y -= unk168;
}

void TBossTelesa::damageRecover()
{
	if (mHitPoints < getMaxHitPoints())
		mHitPoints += 1;
	offLiveFlag(LIVE_FLAG_UNK200);
}

void TBossTelesa::setSpicy(TLiveActor* actor)
{
	if (actor)
		actor->onLiveFlag(LIVE_FLAG_UNK200);
	unk35C = 1;
}

void TBossTelesa::checkHitObject(THitActor* actor)
{
	unk380 = -1;

	if ((actor->mActorType & ACTOR_TYPE_MASK) != ACTOR_TYPE_UNK40000000)
		return;
	if (mSpine->getCurrentNerve() != &TNerveBossTelesaPrepareSlot::theNerve())
		return;

	switch (actor->mActorType) {
	case 0x40000390:
		unk348 = 0xE6;
		unk349 = 0x64;
		unk34A = 0xB4;
		unk380 = 0xD8;
		kill();
		break;
	case 0x40000391:
	case 0x40000392:
		unk348 = 0xE6;
		unk349 = 0xB4;
		unk34A = 0;
		unk380 = 0xDA;
		kill();
		break;
	case 0x40000393:
		unk348 = 0x96;
		unk349 = 0x32;
		unk34A = 0xE6;
		unk380 = 0xD9;
		kill();
		break;
	case 0x40000395:
		break;
	default:
		return;
	}

	if (!unk350 && actor->mActorType != 0x40000395) {
		if (unk35A) {
			unk35A = 0;
			gpMarDirector->mConsole->startAppearBalloon(0xE000F, true);
		}
		unk35C++;
		if (unk35C > 2)
			gpMarDirector->mConsole->startAppearBalloon(0xE0010, true);
	} else {
		unk35C = 0;
	}

	unk374 = actor->mPosition;
	gpMarioParticleManager->emit(0xD7, &unk374, 0, nullptr);
	if (unk380 >= 0)
		gpMarioParticleManager->emit(unk380, &unk374, 0, nullptr);

	if (unk350) {
		gpMarioParticleManager->emit(0xDB, &unk374, 0, nullptr);
	} else {
		if (gpMSound->gateCheck(0x2944)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x2944, &mPosition, 0, nullptr, 0, 4);
		}
	}

	((TMapObjBase*)actor)->makeObjDead();
}

int TTelesaSlot::getResultFromAng(f32 ang)
{
	if (ang < 44.0f)
		return 0;
	if (ang < 89.0f)
		return 1;
	if (ang < 134.0f)
		return 3;
	if (ang < 179.0f)
		return 2;
	if (ang < 224.0f)
		return 0;
	if (ang < 269.0f)
		return 1;
	if (ang < 314.0f)
		return 3;
	if (ang < 359.0f)
		return 2;
	return 2;
}

int TTelesaSlot::getForcastResult(int idx)
{
	f32 ang = unk13C[idx];
	f32 cur = unk138[idx];

	do {
		while (fabsf(cur) > unk160) {
			ang += cur;
			if (cur > 0.0f)
				cur -= unk15C;
			else
				cur += unk15C;

			if (ang >= 360.0f)
				ang -= 360.0f;
			if (ang < 0.0f)
				ang += 360.0f;
		}

		ang += cur;
		if (ang >= 360.0f)
			ang -= 360.0f;
		if (ang < 0.0f)
			ang += 360.0f;

	} while ((int)fabsf(ang) % unk168 != 0);

	return getResultFromAng(unk168 * (int)(ang / (f32)unk168));
}

int TTelesaSlot::getSlotResult()
{
	int result = getResultFromAng(unk13C[0]);
	for (int i = 1; i < 3; ++i) {
		if (getResultFromAng(unk13C[i]) != result)
			return -1;
	}
	return result;
}

BOOL TTelesaSlot::isRollDrum()
{
	if (unk198)
		return TRUE;
	if (unk199)
		return TRUE;
	if (unk19A)
		return TRUE;

	unk19B = 0;
	return FALSE;
}

void TTelesaSlot::forceStopSlot(int idx)
{
	if (!unk19C)
		return;

	f32 min = 0.0f;
	f32 max = 1.0f;

	TBossTelesa* owner                = getSlotOwner(this);
	TBossTelesaSaveLoadParams* params = (TBossTelesaSaveLoadParams*)owner->unk15C;
	f32 collectRate                  = params->mSLSlotFirstHitCollectRate.get();

	if (SMS_GetMarioHP() == 1)
		collectRate = 0.9f;

	if (min + (max - min) * (rand() * 0.000030517578f) <= collectRate) {
		unk1A4 = 2;
		if (SMS_GetMarioHP() <= 3)
			unk1A4 = 0;
		*(&unk1A8 + idx) = 1;
	} else {
		unk1A4           = getForcastResult(idx);
		*(&unk198 + idx) = 0;
	}

	if (unk1A4 == owner->unk1A8)
		unk1A4 = 3;

	if (unk1A4 == 0) {
		if (owner->unk370 == 0)
			unk1A4 = 1;
		else if (SMS_GetMarioHP() >= 6)
			unk1A4 = 3;
	}

	unk19C = 0;
}

u32 TTelesaSlot::touchWater(THitActor*) { return FALSE; }

void TTelesaSlot::moveStart()
{
	unk19C = 1;
	unk19B = 1;
	unk198 = 1;
	unk199 = 1;
	unk19A = 1;
	unk1A8 = 0;
	unk1A9 = 0;
	unk1AA = 0;

	unk138[0] = -unk158;
	unk138[1] = -0.8f * unk158;
	unk138[2] = unk158;
}

void TTelesaSlot::moveObject()
{
	TSlotDrum::moveObject();
	for (int i = 0; i < 3; ++i) {
		if (*(&unk198 + i) && getForcastResult(i) == unk1A4)
			forceStopSlot(i);
	}
}

void TTelesaSlot::calcRootMatrix() { TSlotDrum::calcRootMatrix(); }

void TTelesaSlot::randomReset()
{
	for (int i = 0; i < 3; ++i) {
		unk138[i] = 0.0f;
		unk13C[i] = (f32)(rand() % 8) * 45.0f;
	}
	unk198 = 0;
	unk199 = 0;
	unk19A = 0;
	unk19B = 0;
	unk19C = 0;
	unk1A8 = 0;
	unk1A9 = 0;
	unk1AA = 0;
}

void TTelesaSlot::initMapObj()
{
	TSlotDrum::initMapObj();
	unk168 = 45;
	unk1A4 = 0;
	randomReset();
}

void TBossTelesaKillSmallEnemy::checkHit()
{
	unk6C = 0;

	for (int i = 0; i < mColCount; ++i) {
		THitActor* actor = mCollisions[i];
		if (actor->mActorType & ACTOR_TYPE_ENEMY) {
			if (actor->mActorType == (ACTOR_TYPE_ENEMY | 0x13))
				((THamuKuri*)actor)->selectCapHolder();
			((TLiveActor*)actor)->kill();
		}
	}

	JGeometry::TVec3<f32> diff = *gpMarioPos;
	diff -= mPosition;
	diff.y = 0.0f;
	if (MsVECMag2(&diff) < 300.0f) {
		unk68->forceHide();
		unk6C = 1;
	}
}

BOOL TBossTelesaTongue::receiveMessage(THitActor*, u32 message)
{
	if (message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		TBossTelesa* boss = unk68;
		if (boss->mSpine->getCurrentNerve()
		    == &TNerveBossTelesaAppear::theNerve())
			boss->mSpine->pushNerve(&TNerveBossTelesaSlotStart::theNerve());
	}
	return TRUE;
}

BOOL TBossTelesaBody::receiveMessage(THitActor* sender, u32 message)
{
	if (message == HIT_MESSAGE_TRAMPLE
	    && sender->mActorType != (ACTOR_TYPE_PLAYER | 1)) {
		TBossTelesa* boss = unk68;
		if (boss->mSpine->getCurrentNerve()
		    == &TNerveBossTelesaPrepareSlot::theNerve())
			boss->mSpine->pushNerve(&TNerveBossTelesaSpit::theNerve());
	}

	if (message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		TBossTelesa* boss = unk68;
		if (boss->mSpine->getCurrentNerve()
		    == &TNerveBossTelesaPrepareSlot::theNerve())
			boss->mSpine->pushNerve(&TNerveBossTelesaFreeze::theNerve());
	}

	return TRUE;
}

TBossTelesaManager::TBossTelesaManager(const char* name)
    : TEnemyManager(name)
{
}

void TBossTelesaManager::perform(u32 flags, JDrama::TGraphics* gfx)
{
	TEnemyManager::perform(flags, gfx);
}

void TBossTelesaManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "btelesa.bmd", 0x15300000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TSpineEnemy* TBossTelesaManager::createEnemyInstance()
{
	return new TBossTelesa("ボステレサ");
}

void TBossTelesaManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TBossTelesaSaveLoadParams("/enemy/bosstelesa.prm");
	TEnemyManager::load(stream);
}

MtxPtr TBubble::getTakingMtx() { return mMActor->unk4->unk20; }

void TBubble::init(TLiveManager* manager)
{
	TWalkerEnemy::init(manager);
	unk194     = (TBubbleSaveLoadParams*)getSaveParam();
	mActorType = 0x10000020;
	if (mSpine)
		mSpine->initWith(&TNerveBubbleLive::theNerve());
}

void TBubble::calcRootMatrix()
{
	MsMtxSetXYZRPH(getModel()->getBaseTRMtx(), mPosition.x, mPosition.y,
	               mPosition.z, mRotation.x, mRotation.y, mRotation.z);
	getModel()->setBaseScale(mScaling);
}

void TBubble::kill()
{
	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return;

	onLiveFlag(LIVE_FLAG_DEAD);
	if (mSpine)
		mSpine->pushNerve(&TNerveSmallEnemyDie::theNerve());
}

f32 TBubble::getGravityY() const
{
	if (unk1D0) {
		if (unk1D1)
			return 0.001f;
		return 0.0f;
	}
	return mGravity;
}

const char** TBubble::getBasNameTable() const { return btelesa_bastable; }

void TBubble::reset()
{
	TWalkerEnemy::reset();
	unk198 = 0;
	unk1CC = 0.0f;
	unk1D0 = 0;
	unk1D1 = 0;
}

void TBubble::behaveToWater(THitActor*)
{
	if (mSpine)
		mSpine->pushNerve(&TNerveBubbleSplit::theNerve());
}

void TBubble::setDeadAnm() { setBckAnm(9); }

void TBubble::attackToMario()
{
	sendAttackMsgToMario();
	kill();
}

void TBubble::appendEnemy()
{
	TConductor* conductor
	    = JDrama::TNameRefGen::search<TConductor>("TConductor");
	if (conductor)
		conductor->makeOneEnemyAppear(mPosition, "パブル", 1);
}

void TBubble::split()
{
	if (unk194 == nullptr)
		return;

	for (int i = 0; i < unk194->mSLNumDivision.get(); ++i)
		appendEnemy();
	kill();
}

TBubbleManager::TBubbleManager(const char* name)
    : TSmallEnemyManager(name)
{
}

void TBubbleManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "btelesa_osenbubbles_ind.bmd", 0x11020000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TSmallEnemy* TBubbleManager::createEnemyInstance() { return new TBubble; }

void TBubbleManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TBubbleSaveLoadParams("/enemy/bubble.prm");
	TSmallEnemyManager::load(stream);
}

DEFINE_NERVE(TNerveBossTelesaFallDemo, TLiveActor)
{
	TBossTelesa* boss = getBoss(spine);
	if (spine->getTime() == 0)
		boss->offLiveFlag(LIVE_FLAG_HIDDEN);
	if (spine->getTime() > 180) {
		spine->setNext(&TNerveBossTelesaHideWait::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBossTelesaFreeze, TLiveActor)
{
	TBossTelesa* boss = getBoss(spine);
	if (spine->getTime() > 120) {
		boss->damageRecover();
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBossTelesaPrepareSlot, TLiveActor)
{
	TBossTelesa* boss = getBoss(spine);
	if (spine->getTime() == 0)
		boss->rouletteStart();
	if (boss->unk154 && !boss->unk154->isRollDrum()) {
		spine->setNext(&TNerveBossTelesaSpitSlotItem::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBossTelesaSpitSlotItem, TLiveActor)
{
	TBossTelesa* boss = getBoss(spine);
	if (spine->getTime() == 0)
		boss->generateSlotItem();
	if (spine->getTime() > 90) {
		spine->setNext(&TNerveBossTelesaHideWait::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBossTelesaSlotStart, TLiveActor)
{
	TBossTelesa* boss = getBoss(spine);
	if (spine->getTime() == 0)
		boss->rouletteStart();
	if (spine->getTime() > 60) {
		spine->setNext(&TNerveBossTelesaPrepareSlot::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBossTelesaAppear, TLiveActor)
{
	TBossTelesa* boss = getBoss(spine);
	if (spine->getTime() == 0)
		boss->offLiveFlag(LIVE_FLAG_HIDDEN);
	if (spine->getTime() > 120)
		return TRUE;
	return FALSE;
}

DEFINE_NERVE(TNerveBossTelesaHideWait, TLiveActor)
{
	TBossTelesa* boss = getBoss(spine);
	if (spine->getTime() > boss->unk36C) {
		spine->setNext(&TNerveBossTelesaAppear::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBossTelesaHide, TLiveActor)
{
	TBossTelesa* boss = getBoss(spine);
	boss->onLiveFlag(LIVE_FLAG_HIDDEN);
	if (spine->getTime() > 60)
		return TRUE;
	return FALSE;
}

DEFINE_NERVE(TNerveBossTelesaSpit, TLiveActor)
{
	TBossTelesa* boss = getBoss(spine);
	if (spine->getTime() == 0)
		boss->genAttacker();
	if (spine->getTime() > 120)
		return TRUE;
	return FALSE;
}

DEFINE_NERVE(TNerveBossTelesaDie, TLiveActor)
{
	TBossTelesa* boss = getBoss(spine);
	boss->onLiveFlag(LIVE_FLAG_DEAD);
	return FALSE;
}

DEFINE_NERVE(TNerveBubbleSplit, TLiveActor)
{
	TBubble* bubble = getBubble(spine);
	if (spine->getTime() == 0)
		bubble->split();
	return TRUE;
}

DEFINE_NERVE(TNerveBubbleLive, TLiveActor)
{
	TBubble* bubble = getBubble(spine);
	if (bubble->unk194
	    && spine->getTime() > bubble->unk194->mSLLiveTime.get()) {
		bubble->kill();
		return TRUE;
	}
	return FALSE;
}
