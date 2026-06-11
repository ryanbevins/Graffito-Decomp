#include <Enemy/BossTelesa.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/HamuKuri.hpp>
#include <Enemy/Telesa.hpp>
#include <Camera/CameraShake.hpp>
#include <GC2D/GCConsole2.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <MarioUtil/ScreenUtil.hpp>
#include <MarioUtil/TexUtil.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/Item.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MSound/MSound.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioMain.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/Spine.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarDirector.hpp>
#include <System/MarioGamePad.hpp>
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
	TRoulette** roulettes = (TRoulette**)&unk178;
	TTelesaSlot* slot     = (TTelesaSlot*)unk184;

	f32 speedMin = 0.05f;
	f32 speedMax = 0.1f;
	f32 dirMin   = -1.0f;
	f32 dirMax   = 1.0f;
	f32 dirRange = dirMax - dirMin;
	f32 dir      = dirMin + dirRange * (rand() * 0.000030517578f);

	TSpineEnemyParams* params = getSaveParam();
	u8 maxHitPoints          = params ? getSaveParam()->mSLHitPointMax.get() : 1;
	f32 hpSpeed
	    = (maxHitPoints - mHitPoints) * TBossTelesa::mRouletteUpRate;

	for (int i = 0; i < 3; ++i) {
		f32 direction = 1.0f;
		if (dir > 0.0f)
			direction = -1.0f;
		if (i == 0 || i == 2)
			direction = -direction;

		f32 speedRange = speedMax - speedMin;
		f32 speed      = speedMin + speedRange * (rand() * 0.000030517578f);
		roulettes[i]->unk144 = direction * (speed + hpSpeed);

		speedRange     = speedMax - speedMin;
		speed          = speedMin + speedRange * (rand() * 0.000030517578f);
		slot->unk1E4[i] = direction * (speed + hpSpeed);
	}

	for (int i = 0; i < 3; ++i)
		roulettes[i]->setRollSp(slot->unk1E4[i]);

	SMSRumbleMgr->start(0x14, 0xf, (f32*)nullptr);
	gpCameraShake->startShake((EnumCamShakeMode)0x23, 1.0f);
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

BOOL TBossTelesa::slotFall()
{
	TRoulette* roulette0 = (TRoulette*)unk178;
	TRoulette* roulette1 = (TRoulette*)unk17C;
	TRoulette* roulette2 = (TRoulette*)unk180;
	TTelesaSlot* slot     = (TTelesaSlot*)unk184;

	if (slot->mPosition.y > roulette0->mPosition.y - 800.0f) {
		slot->mPosition.y -= 5.0f;
		return FALSE;
	}

	slot->mPosition.y -= 1.0f;

	if (slot->mPosition.y < roulette0->mPosition.y - 900.0f) {
		int rolling = 0;
		if (roulette0->unk13C != 0.0f)
			rolling = 1;
		if (roulette1->unk13C != 0.0f)
			++rolling;
		if (roulette2->unk13C != 0.0f)
			++rolling;

		if (rolling != 3)
			rouletteStart();
	}

	if (slot->mPosition.y < roulette0->mPosition.y - 1100.0f)
		return TRUE;

	THitActor* switchActor = roulette0->unk150;
	switchActor->mAttackRadius = 280.0f;
	switchActor->mAttackHeight = 100.0f;
	switchActor->mDamageRadius = 280.0f;
	switchActor->mDamageHeight = 100.0f;
	switchActor->calcEntryRadius();

	switchActor = roulette1->unk150;
	switchActor->mAttackRadius = 280.0f;
	switchActor->mAttackHeight = 100.0f;
	switchActor->mDamageRadius = 280.0f;
	switchActor->mDamageHeight = 100.0f;
	switchActor->calcEntryRadius();

	return FALSE;
}

BOOL TBossTelesa::rouletteFall()
{
	TRoulette* roulette0 = (TRoulette*)unk178;
	TRoulette* roulette1 = (TRoulette*)unk17C;

	if (roulette0->mPosition.y > roulette1->mPosition.y) {
		roulette0->mPosition.y -= 1.0f;
		roulette0->mMActor->setBck("rulet00");
	} else {
		roulette0->mPosition.y = roulette1->mPosition.y;
		return TRUE;
	}

	if (roulette0->mPosition.y > roulette1->mPosition.y + 3.0f) {
		if (SMS_SendMessageToMario(this, HIT_MESSAGE_TAKE))
			mHeldObject = (TTakeActor*)SMS_GetMarioHitActor();
	} else {
		if (SMS_SendMessageToMario(this, HIT_MESSAGE_UNK8)) {
			if (gpMSound->gateCheck(0x2926)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x2926, &mPosition, 0, nullptr, 0, 4);
			}
			mHeldObject = nullptr;
		}
	}

	if (gpMSound->gateCheck(0x28DC)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x28DC, &mPosition, 0, nullptr, 0, 4);
	}

	if (gpMSound->gateCheck(0x2125)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x2125, &mPosition, 0, nullptr, 0, 4);
	}

	gpMarioOriginal->mGamePad->onNeutralMarioKey();
	return FALSE;
}

void TBossTelesa::damageRecover()
{
	f32 zero = 0.0f;

	for (int i = 0; i < 20; ++i) {
		TLiveActor* actor = unk2A8[i];
		if (!(actor->mLiveFlag & LIVE_FLAG_DEAD)) {
			if (actor->mHolder == nullptr)
				SMS_SendMessageToMario(actor, HIT_MESSAGE_UNK8);

			((TMapObjBase*)actor)->makeObjDead();
			gpMarioParticleManager->emit(0xCD, &actor->mPosition, 0,
			                             nullptr);
			actor->mPosition.set(zero, zero, zero);
		}
	}

	for (int i = 0; i < 10; ++i) {
		TLiveActor* actor = unk2F8[i];
		if (!(actor->mLiveFlag & LIVE_FLAG_DEAD)) {
			if (actor->mHolder == nullptr)
				SMS_SendMessageToMario(actor, HIT_MESSAGE_UNK8);

			((TMapObjBase*)actor)->makeObjDead();
			gpMarioParticleManager->emit(0xCD, &actor->mPosition, 0,
			                             nullptr);
			actor->mPosition.set(zero, zero, zero);
		}

		actor = unk320[i];
		if (!(actor->mLiveFlag & LIVE_FLAG_DEAD)) {
			((TMapObjBase*)actor)->makeObjDead();
			gpMarioParticleManager->emit(0xCD, &actor->mPosition, 0,
			                             nullptr);
			actor->mPosition.set(zero, zero, zero);
		}
	}

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

	mSpine->pushAfterCurrent(&TNerveBossTelesaHide::theNerve());
	unk368 = 0;
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

	for (int i = 0; i < 3; ++i) {
		*(&unk198 + i) = 1;
		*(&unk1A8 + i) = 0;

		f32 speed = 1.0f;
		if (i == 0)
			speed = -1.0f;
		if (i == 1)
			speed = -0.8f;

		unk138[i] = speed * unk158;
	}
}

void TTelesaSlot::moveObject()
{
	TLiveActor::moveObject();

	for (int i = 0; i < unk148; ++i) {
		u8* isRolling = &unk198 + i;
		u8* doStop    = &unk1A8 + i;

		if (*doStop) {
			if (getForcastResult(i) == unk1A4) {
				*isRolling = 0;
				*doStop    = 0;
			}
		}

		f32 speed = unk138[i];
		if (speed == 0.0f)
			continue;

		if (fabsf(speed) > unk160) {
			unk13C[i] += speed;

			if (!*isRolling) {
				if (unk138[i] > 0.0f)
					unk138[i] -= unk15C;
				else
					unk138[i] += unk15C;
			}

			if (unk13C[i] >= 360.0f)
				unk13C[i] -= 360.0f;
			if (unk13C[i] < 0.0f)
				unk13C[i] += 360.0f;
		} else {
			unk13C[i] += speed;

			if (unk13C[i] >= 360.0f)
				unk13C[i] -= 360.0f;
			if (unk13C[i] < 0.0f)
				unk13C[i] += 360.0f;

			if (*isRolling)
				continue;

			if ((int)fabsf(unk13C[i]) % unk168 != 0)
				continue;

			unk13C[i] = unk168 * (int)(unk13C[i] / (f32)unk168);
			unk138[i] = 0.0f;

			if (gpMSound->gateCheck(0x292C)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x292C, &mPosition, 0, nullptr, 0, 4);
			}

			for (int j = 0; j < unk148; ++j) {
				u8* otherRolling = &unk198 + j;
				if (*otherRolling) {
					f32 min = 0.0f;
					f32 max = 1.0f;
					TBossTelesa* owner = getSlotOwner(this);
					TBossTelesaSaveLoadParams* params
					    = (TBossTelesaSaveLoadParams*)owner->unk15C;
					f32 rate = params->mSLSlotHitCollectRate.get();

					if (min
					        + (max - min)
					            * (rand() * 0.000030517578f)
					    <= rate) {
						*(&unk1A8 + j) = 1;
					} else {
						*otherRolling = 0;
					}
				}
			}

			u8 allStopped = 1;
			if (unk138[0] != 0.0f)
				allStopped = 0;
			if (unk138[1] != 0.0f)
				allStopped = 0;
			if (unk138[2] != 0.0f)
				allStopped = 0;

			if (allStopped) {
				TBossTelesa* owner = getSlotOwner(this);
				TTelesaSlot* slot = (TTelesaSlot*)owner->unk184;
				int result        = slot->getSlotResult();

				if (result == 2 || result == 0) {
					owner->unk374.x = 0.0f;
					owner->unk374.y = 0.0f;
					owner->unk374.z = 0.0f;
					gpMarioParticleManager->emit(0xE1, &owner->unk374,
					                             0, nullptr);

					u32 sound = result == 2 ? 0x293F : 0x2940;
					if (gpMSound->gateCheck(sound)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    sound, &owner->mPosition, 0, nullptr, 0, 4);
					}
				} else {
					if (gpMSound->gateCheck(0x294D)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x294D, &owner->mPosition, 0, nullptr, 0, 4);
					}
				}
			}
		}
	}
}

void TTelesaSlot::calcRootMatrix()
{
	u8 rolling = 0;

	if (unk138[0] != 0.0f)
		rolling = 1;
	if (unk138[1] != 0.0f)
		rolling = 1;
	if (unk138[2] != 0.0f)
		rolling = 1;

	if (rolling) {
		if (unk1E0) {
			if (gpMSound->gateCheck(0x308D)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x308D, &mPosition, 0, nullptr, 0, 4);
			}
		}

		unk1E0 = 1 - unk1E0;
	}

	TSlotDrum::calcRootMatrix();
}

void TTelesaSlot::randomReset()
{
	int min = 0;
	int max = 8;

	for (int i = 0; i < 3; ++i) {
		unk13C[i] = (f32)(unk168
		                   * (min
		                      + (int)(rand() * 0.000030517578f
		                              * (max - min))));
		*(&unk198 + i) = 0;
	}
}

void TTelesaSlot::initMapObj()
{
	TSlotDrum::initMapObj();
	onLiveFlag(LIVE_FLAG_UNK10);
	unk14C = 160.0f;
	unk150 = mPosition.y;
	unk154 = 2.0f;
	unk158 = 2.0f;
	unk15C = 0.01f;
	unk160 = 0.5f;
	unk164 = 0;
	unk168 = 45;
	unk140 = mDamageRadius / 3.0f;
	unk144 = mDamageHeight;
	unk1DC = new TMapCollisionMove();
	unk1DC->init(2, 0, 0, nullptr);
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
	mActorType = 0x10000020;
	unk150     = 0x11;
	unk194     = (TBubbleSaveLoadParams*)getSaveParam();
	mSpine->initWith(&TNerveBubbleLive::theNerve());
	mMActor->setLightType(3);

	TScreenTexture* tex
	    = JDrama::TNameRefGen::search<TScreenTexture>("スクリーンテクスチャ");
	SMS_ChangeTextureAll(mMActor->getModel()->getModelData(), "H_ma_rak_dummy",
	                     *tex->getTexture()->getTexInfo());
}

void TBubble::calcRootMatrix()
{
	if (!isEaten()) {
		mPosition.y = mGroundHeight + unk1CC + 150.0f;

		MtxPtr mtx = mMActor->getModel()->getBaseTRMtx();
		MsMtxSetXYZRPH(mtx, mPosition.x, mPosition.y, mPosition.z,
		               (s16)(mRotation.x * 182.04445f),
		               (s16)(mRotation.y * 182.04445f),
		               (s16)(mRotation.z * 182.04445f));
		mMActor->getModel()->setBaseScale(mScaling);
	}
}

void TBubble::kill()
{
	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return;

	if (unk198) {
		if (unk1D2)
			unk198->receiveMessage(this, HIT_MESSAGE_UNK7);
		else
			unk198->kill();
		unk198 = nullptr;
	}

	mHitPoints = 1;
	if (mSpine->getCurrentNerve() != &TNerveSmallEnemyDie::theNerve()) {
		mSpine->reset();
		mSpine->setNext(&TNerveSmallEnemyDie::theNerve());
		mSpine->pushAfterCurrent(mSpine->getDefault());
	}

	onLiveFlag(LIVE_FLAG_UNK20000);
	onLiveFlag(LIVE_FLAG_UNK40);
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
	onLiveFlag(LIVE_FLAG_UNK8);

	f32 min = 50.0f;
	f32 max = 150.0f;
	f32 range = max - min;
	f32 factor = rand() * 0.000030517578f;
	factor *= range;
	factor += min;
	unk1CC = factor;
	unk1D0 = 0;
	unk1D1 = 1;
	unk1D2 = 0;
	unk198 = 0;
	mSpine->initWith(&TNerveBubbleLive::theNerve());
}

void TBubble::behaveToWater(THitActor*)
{
	if (mSpine->getCurrentNerve() == &TNerveBubbleLive::theNerve()
	    && mMActor->checkCurBckFromIndex(10)) {
		kill();
		TMapObjBase* item = gpItemManager->makeObjAppear(
		    mPosition.x, mPosition.y + 50.0f, mPosition.z, 0x20000002, true);
		if (item)
			((TItem*)item)->killByTimer(0x4B0);
	}
}

void TBubble::setDeadAnm() { setBckAnm(9); }

void TBubble::attackToMario()
{
	sendAttackMsgToMario();
	kill();
}

void TBubble::appendEnemy()
{
	unk198 = nullptr;

	f32 min = 0.0f;
	f32 max = 100.0f;
	f32 range = max - min;
	f32 randValue = rand() * 0.000030517578f;
	randValue *= range;
	randValue += min;

	TSmallEnemy* enemy;
	if (randValue < 50.0f) {
		enemy = (TSmallEnemy*)gpConductor->makeOneEnemyAppear(
		    mPosition, "ポポマネージャー", 1);
		enemy->unk154 = 0.6f;
		enemy->reset();
	} else if (randValue < 100.0f) {
		enemy = (TSmallEnemy*)gpConductor->makeOneEnemyAppear(
		    mPosition, "ボム兵マネージャー", 1);
		enemy->unk154 = 0.3f;
		enemy->reset();
	} else if (randValue < 150.0f) {
		enemy = (TSmallEnemy*)gpConductor->makeOneEnemyAppear(
		    mPosition, "テレサマネージャー", 1);
		if (!enemy)
			return;
		enemy->unk154 = 0.6f;
		enemy->reset();
		((TTelesa*)enemy)->setAttacker();
	} else {
		enemy = (TSmallEnemy*)gpConductor->makeOneEnemyAppear(
		    mPosition, "パックンマネージャー", 1);
		enemy->unk154 = 0.6f;
		enemy->reset();
	}

	if (enemy && enemy->receiveMessage(this, HIT_MESSAGE_TAKE)) {
		enemy->onHitFlag(HIT_FLAG_UNK8000000);
		mHeldObject = enemy;
		enemy->mVelocity.set(0.0f, 1.0f, -1.0f);
		enemy->onLiveFlag(LIVE_FLAG_AIRBORNE);
		unk198 = enemy;
	}
}

void TBubble::split()
{
	s32 numDivision = unk194->mSLNumDivision.get();
	for (int i = 0; i < numDivision; ++i) {
		TBubble* bubble = (TBubble*)gpConductor->makeOneEnemyAppear(
		    mPosition, "バブルマネージャー", 1);
		if (!bubble)
			break;

		bubble->reset();
		bubble->mPosition = mPosition;
		bubble->mPosition.y += unk1CC;
		bubble->unk1CC = 0.0f;
		bubble->unk1D0 = 1;

		f32 min = -2.0f;
		f32 max = 2.0f;
		JGeometry::TVec3<f32> velocity;

		f32 range = max - min;
		f32 factor = rand() * 0.000030517578f;
		factor *= range;
		factor += min;
		velocity.x = factor;

		range  = max - min;
		factor = rand() * 0.000030517578f;
		factor *= range;
		factor += min;
		velocity.y = factor;

		range  = max - min;
		factor = rand() * 0.000030517578f;
		factor *= range;
		factor += min;
		velocity.z = factor;

		bubble->mVelocity = velocity;
	}
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
	if (spine->getTime() == 0) {
		bubble->onHitFlag(HIT_FLAG_NO_COLLISION);
		bubble->split();
	}

	if (spine->getTime() == 10)
		bubble->setBckAnm(9);

	if (bubble->checkCurAnmEnd(0) && bubble->mMActor->checkCurBckFromIndex(9)) {
		bubble->unk1D2 = 0;
		bubble->kill();
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBubbleLive, TLiveActor)
{
	TBubble* bubble = getBubble(spine);
	if (spine->getTime() == 0) {
		bubble->offHitFlag(HIT_FLAG_NO_COLLISION);

		if (bubble->unk1D0) {
			bubble->setBckAnm(10);
			bubble->setGoalPathMario();

			f32 min = 0.0f;
			f32 max = 20.0f;
			f32 range = max - min;
			f32 randValue = rand() * 0.000030517578f;
			randValue *= range;
			randValue += min;
			bubble->mMActor->getFrameCtrl(0)->setFrame(randValue);
			bubble->onLiveFlag(LIVE_FLAG_UNK8);
		} else {
			bubble->setBckAnm(8);
		}
	} else if (bubble->checkCurAnmEnd(0)) {
		bubble->offHitFlag(HIT_FLAG_NO_COLLISION);
		bubble->setBckAnm(10);
	}

	if (!bubble->unk1D0) {
		if (bubble->unk1CC < bubble->unk194->mSLAddPosBase.get())
			bubble->unk1CC += 1.0f;
	} else {
		if (spine->getTime() > 40 && bubble->unk1D1) {
			JGeometry::TVec3<f32> velocity = bubble->mVelocity;
			velocity.scale(0.98f);
			bubble->mVelocity = velocity;
		} else {
			bubble->walkBehavior(0, 0.8f);
		}

		if (spine->getTime() == 80) {
			bubble->unk1D1 = 0;
			bubble->mVelocity.set(0.0f, 0.0f, 0.0f);
		}
	}

	bubble->unk1CC += 0.001f;
	if (bubble->unk1CC
	    > bubble->mPosition.y + bubble->unk194->mSLDeadHeight.get()) {
		bubble->unk1D2 = 0;
		bubble->kill();
	}

	if (bubble->mScaling.x < bubble->unk194->mSLMaxScale.get()) {
		f32 scale = bubble->mScaling.z * bubble->unk194->mSLRateExpand.get();
		bubble->mScaling.z = scale;
		bubble->mScaling.y = scale;
		bubble->mScaling.x = scale;
	}

	if (spine->getTime() > bubble->unk194->mSLLiveTime.get()) {
		spine->pushAfterCurrent(&TNerveBubbleSplit::theNerve());
		return TRUE;
	}

	return FALSE;
}
