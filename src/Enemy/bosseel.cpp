#include <Enemy/BossEel.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DCluster.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/MtxUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <MarioUtil/ScreenUtil.hpp>
#include <MarioUtil/TexUtil.hpp>
#include <Map/Map.hpp>
#include <MSound/MSound.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/SharedParts.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/Particles.hpp>
#include <math.h>
#include <stdlib.h>

// rogue includes needed for matching sinit
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

static f32 dummy1431[3] = { 1.0f, 1.0f, 1.0f };
static f32 dummy1411[3] = { 1.0f, 1.0f, 1.0f };
static u32 dummy1210[4] = { 0, 2, 1, 3 };

static const char dummyMactorStringValue1[] = "\0\0\0\0\0\0\0\0\0\0\0";
static const char SMS_NO_MEMORY_MESSAGE[]   = "メモリが足りません\n";
static const char MtxCalcTypeName0[]
    = "MActorMtxCalcType_Basic クラシックスケールＯＮ";
static const char MtxCalcTypeName1[]
    = "MActorMtxCalcType_Softimage クラシックスケールＯＦＦ";
static const char MtxCalcTypeName2[]
    = "MActorMtxCalcType_MotionBlend モーションブレンド";
static const char MtxCalcTypeName3[]
    = "MActorMtxCalcType_User ユーザー定義";
static const char* MtxCalcTypeName[] = {
	MtxCalcTypeName0,
	MtxCalcTypeName1,
	MtxCalcTypeName2,
	MtxCalcTypeName3,
};
static const f32 dummy2933[3] = { 0.0f, 0.0f, 0.0f };
static const f32 dummy2935[3] = { 1.0f, 1.0f, 1.0f };

static inline void setEffectMtxOnTex1(J3DMaterial* mat, MtxPtr mtx)
{
	mat->getTexGenBlock()->getTexMtx(1)->setEffectMtx(mtx);
}

f32 TBossEel::mOpenRollSpeed = 0.3f;
u8 TBossEel::mUseObjCollision = TRUE;
f32 TBossEel::mForcePow       = 10.0f;
u8 TBossEel::mUseMapCollision;

static const char* bosseel_bastable[] = {
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	"/scene/bosseel/bas/meoto_in_loop.bas",
	nullptr,
	"/scene/bosseel/bas/meoto_mogu.bas",
	nullptr,
	nullptr,
	nullptr,
	"/scene/bosseel/bas/meoto_out_loop.bas",
	"/scene/bosseel/bas/meoto_paku.bas",
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

static const char* bossEelTears_bastable[] = {
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

static const char cDummyTextureName[] = "M_dummy";
static const char cTearsRecoverCollisionName[] = "回復コリジョン";

DEFINE_NERVE(TNerveBEelTearsMoveUp, TLiveActor)
{
	TBEelTears* tears = (TBEelTears*)spine->getBody();

	if (spine->getTime() == 0) {
		tears->mMActor = tears->mMActorKeeper->getMActor("tears.bmd");
		tears->mMActor->setBckFromIndex(1);
	}

	tears->mPosition.y += tears->unk15C->mSLTearsUpSpeed.get();
	return FALSE;
}

DEFINE_NERVE(TNerveBEelTearsGenerate, TLiveActor)
{
	TBEelTears* tears = (TBEelTears*)spine->getBody();

	if (spine->getTime() == 0) {
		tears->mMActor = tears->mMActorKeeper->getMActor("tears.bmd");
		tears->mMActor->setBckFromIndex(2);
	}

	if (tears->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveBEelTearsMoveUp::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBEelTearsSplit, TLiveActor)
{
	TBEelTears* tears = (TBEelTears*)spine->getBody();

	if (spine->getTime() == 0) {
		if (gpMSound->gateCheck(0x8926)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x8926, &tears->mPosition, 0, nullptr, 0, 4);
		}

		tears->mMActor
		    = tears->mMActorKeeper->getMActor("tears_waterhit.bmd");
		tears->mMActor->setBckFromIndex(3);

		MActor* actor = tears->mMActor;
		actor->setFrameRate(
		    tears->unk15C->mSLHitAnmFrameRate.get() * SMSGetAnmFrameRate(),
		    0);
	}

	if (tears->checkCurAnmEnd(0)) {
		JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToPosPtr(
		    0xd5, &tears->mPosition, 0, nullptr);
		if (emitter) {
			emitter->unk154.x = tears->mScaling.x;
			emitter->unk154.y = tears->mScaling.y;
			emitter->unk154.z = tears->mScaling.z;
			emitter->unk174.x = tears->mScaling.x;
			emitter->unk174.y = tears->mScaling.y;
			emitter->unk174.z = tears->mScaling.z;
		}

		((u8*)tears->unk16C)[0x81] = FALSE;
		tears->unk16C->offHitFlag(HIT_FLAG_NO_COLLISION);
		((u8*)tears->unk16C)[0x80] = TRUE;
		tears->unk16C->mPosition = tears->mPosition;

		((TBEelTearsManager*)tears->mManager)->splitTears(tears->mPosition);

		if (gpMSound->gateCheck(0x8927)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x8927, &tears->mPosition, 0, nullptr, 0, 4);
		}

		tears->onLiveFlag(LIVE_FLAG_HIDDEN);
		spine->pushAfterCurrent(&TNerveBEelTearsMarioRecover::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBEelTearsWaterHit, TLiveActor)
{
	TBEelTears* tears = (TBEelTears*)spine->getBody();

	if (spine->getTime() == 0) {
		if (gpMSound->gateCheck(0x8926)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x8926, &tears->mPosition, 0, nullptr, 0, 4);
		}

		tears->mMActor
		    = tears->mMActorKeeper->getMActor("tears_waterhit.bmd");
		tears->mMActor->setBckFromIndex(3);
	}

	--tears->unk164;
	f32 frameRate = tears->unk15C->mSLHitAnmFrameRate.get();

	if (tears->unk164 < 0) {
		MActor* actor = tears->mMActor;
		actor->setFrameRate(-frameRate * SMSGetAnmFrameRate(), 0);
		if (tears->getCurAnmFrameNo(0) < 1.0f)
			return TRUE;
	} else {
		MActor* actor = tears->mMActor;
		actor->setFrameRate(frameRate * SMSGetAnmFrameRate(), 0);
	}

	if (tears->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveBEelTearsMarioRecover::theNerve());

		JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToPosPtr(
		    0xd5, &tears->mPosition, 0, nullptr);
		if (emitter) {
			emitter->unk154.x = tears->mScaling.x;
			emitter->unk154.y = tears->mScaling.y;
			emitter->unk154.z = tears->mScaling.z;
			emitter->unk174.x = tears->mScaling.x;
			emitter->unk174.y = tears->mScaling.y;
			emitter->unk174.z = tears->mScaling.z;
		}

		((u8*)tears->unk16C)[0x81] = FALSE;
		tears->unk16C->offHitFlag(HIT_FLAG_NO_COLLISION);
		((u8*)tears->unk16C)[0x80] = TRUE;
		tears->unk16C->mPosition = tears->mPosition;

		((TBEelTearsManager*)tears->mManager)->splitTears(tears->mPosition);

		if (gpMSound->gateCheck(0x8927)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x8927, &tears->mPosition, 0, nullptr, 0, 4);
		}

		tears->onLiveFlag(LIVE_FLAG_HIDDEN);
		return TRUE;
	}

	if (tears->unk160)
		tears->mPosition.y += tears->unk15C->mSLTearsDamageUpSpeed.get();

	return FALSE;
}

DEFINE_NERVE(TNerveBEelTearsMarioRecover, TLiveActor)
{
	TBEelTears* tears = (TBEelTears*)spine->getBody();

	if (((u8*)tears->unk16C)[0x80] == FALSE) {
		JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToPosPtr(
		    0xd6, gpMarioPos, 0, nullptr);
		if (emitter) {
			emitter->unk154.x = tears->mScaling.x;
			emitter->unk154.y = tears->mScaling.y;
			emitter->unk154.z = tears->mScaling.z;
			emitter->unk174.x = tears->mScaling.x;
			emitter->unk174.y = tears->mScaling.y;
			emitter->unk174.z = tears->mScaling.z;
		}

		tears->kill();
		return TRUE;
	}

	JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToPosPtr(
	    0x19d, &tears->mPosition, 1, tears);
	if (emitter) {
		emitter->unk154.x = tears->mScaling.x;
		emitter->unk154.y = tears->mScaling.y;
		emitter->unk154.z = tears->mScaling.z;
		emitter->unk174.x = tears->mScaling.x;
		emitter->unk174.y = tears->mScaling.y;
		emitter->unk174.z = tears->mScaling.z;
	}

	if (spine->getTime() > 1000) {
		tears->kill();
		return TRUE;
	}

	if (((u8*)tears->unk16C)[0x81] != FALSE) {
		tears->mPosition.y += tears->unk15C->mSLTearsUpSpeed.get();
		tears->unk16C->mPosition.y = tears->mPosition.y;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveOilBallStay, TLiveActor)
{
	TLiveActor* body = spine->getBody();
	MActor* actor    = body->mMActor;
	TOilBall* oilBall = (TOilBall*)body;

	if (actor->checkCurBckFromIndex(3)
	    || (oilBall->checkCurAnmEnd(0)
	        && !oilBall->mMActor->checkCurBckFromIndex(1))) {
		oilBall->mMActor = oilBall->mMActorKeeper->getMActor("tears.bmd");
		oilBall->mMActor->setBckFromIndex(1);
	}

	return FALSE;
}

TBossEel::TBossEel(const char* name)
    : TSpineEnemy(name)
    , unk1A8(nullptr)
    , unk1AC(nullptr)
    , unk1B0(nullptr)
    , unk1BC(1.0f)
    , unk1C0(nullptr)
    , unk1C4(nullptr)
    , unk1CC(500.0f)
    , unk1D0(TRUE)
    , unk1D4(2350.0f)
    , unk1D8(0.75f)
    , unk1E8(nullptr)
    , unk1F0(FALSE)
    , unk1F4(0.0f)
    , unk1FC(FALSE)
    , unk1FD(FALSE)
    , unk1FE(FALSE)
    , unk200(nullptr)
    , unk210(nullptr)
    , unk214(nullptr)
    , unk218(nullptr)
    , unk21C(FALSE)
    , unk21D(TRUE)
{
	TBossEelUnk1EC* unk = new TBossEelUnk1EC;
	if (unk != nullptr) {
		unk->unk0 = 0;
		unk->unk4 = 1;
	}
	unk1EC = unk;
}

BOOL TBossEel::receiveMessage(THitActor*, u32) { return FALSE; }

BOOL TBossEel::hasMapCollision() const { return TRUE; }

const char** TBossEel::getBasNameTable() const { return bosseel_bastable; }

TBossEelSaveParams::TBossEelSaveParams()
    : TParams("/enemy/bosseel.prm")
    , PARAM_INIT(mSLInitTransYOffset, 0.0f)
    , PARAM_INIT(mSLAppearMoveDistY, 6000.0f)
    , PARAM_INIT(mSLBodyScale, 0.75f)
    , PARAM_INIT(mSLViewClipFar, 25000.0f)
    , PARAM_INIT(mSLViewClipRadius, 12000.0f)
    , PARAM_INIT(mSLBodyToHeadDistance, 6000.0f)
    , PARAM_INIT(mSLBodyAttackRadius, 2400.0f)
    , PARAM_INIT(mSLBodyAttackHeight, 6300.0f)
    , PARAM_INIT(mSLBodyDamageRadius, 2200.0f)
    , PARAM_INIT(mSLBodyDamageHeight, 6100.0f)
    , PARAM_INIT(mSLHeadAttackRadius, 3600.0f)
    , PARAM_INIT(mSLHeadAttackHeight, 5600.0f)
    , PARAM_INIT(mSLHeadDamageRadius, 3400.0f)
    , PARAM_INIT(mSLHeadDamageHeight, 5400.0f)
    , PARAM_INIT(mSLToothAttackRadius, 500.0f)
    , PARAM_INIT(mSLToothAttackHeight, 300.0f)
    , PARAM_INIT(mSLToothDamageRadius, 600.0f)
    , PARAM_INIT(mSLToothDamageHeight, 400.0f)
    , PARAM_INIT(mSLSpinAccel, 0.01f)
    , PARAM_INIT(mSLSpinMaxSpeed, 10.0f)
    , PARAM_INIT(mSLToothUpSpeed, 5.0f)
    , PARAM_INIT(mSLToothLiveHeight, 2000.0f)
    , PARAM_INIT(mSLToothMaxHitPoint, 50)
    , PARAM_INIT(mSLGenTearsTime, 100)
    , PARAM_INIT(mSLVortexAttackRadius, 500.0f)
    , PARAM_INIT(mSLVortexAttackHeight, 300.0f)
    , PARAM_INIT(mSLVortexDamageRadius, 600.0f)
    , PARAM_INIT(mSLVortexDamageHeight, 400.0f)
    , PARAM_INIT(mSLVortexLiveTimer, 300)
    , PARAM_INIT(mSLVortexScaleXZ, 1.0f)
    , PARAM_INIT(mSLVortexScaleY, 1.0f)
    , PARAM_INIT(mSLMouthOpenFrame, 500)
    , PARAM_INIT(mSLMouthOpenInterval, 2000)
    , PARAM_INIT(mSLCanEatFrame, 200)
    , PARAM_INIT(mSLBreathInPower, 10.0f)
{
	TParams::load(mPrmPath);
}

void TBossEelManager::clipEnemies(JDrama::TGraphics* graphics)
{
	clipActorsAux(graphics, mSaveParams.mSLViewClipFar.get(),
	              mSaveParams.mSLViewClipRadius.get());
}

void TBossEelManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "meoto_model.bmd", 0x10020000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TBossEelCollision::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		calcEntryRadius();

		for (int i = 0; i < mColCount; ++i) {
			if (mCollisions[i]->isActorTypeOf(ACTOR_TYPE_PLAYER))
				behaveToMario();
		}
	}

	if (flags & 2) {
		mPosition.x = unk68[0][3];
		mPosition.y = unk68[1][3];
		mPosition.z = unk68[2][3];
	}

	THitActor::perform(flags, graphics);
}

void TBossEelCollision::behaveToMario()
{
	JGeometry::TVec3<f32> velocity(0.0f, TBossEel::mForcePow, 0.0f);
	velocity.add(*gpMarioPos);
	SMS_MarioMoveRequest(velocity);

	TBossEel* eel = unk7C;
	if (eel == nullptr)
		return;

	u8 canEat = *(u8*)((u8*)eel + 0x1C8);
	if (!canEat) {
		JGeometry::TVec3<f32> pos = *gpMarioPos;
		MtxPtr mtx = eel->mMActor->getModel()
		                 ->mNodeMatrices[*(u16*)((u8*)eel + 0x1A0)];
		pos.x -= mtx[0][3];
		pos.y -= mtx[1][3];
		pos.z -= mtx[2][3];

		f32 dist = MsVECMag2(&pos);
		if (dist < eel->unk1D4 * eel->unk1D8)
			canEat = TRUE;
	}

	if (canEat
	    && eel->mSpine->getCurrentNerve() != &TNerveBossEelEat::theNerve())
		eel->mSpine->pushNerve(&TNerveBossEelEat::theNerve());
}

void TBossEelCollision::initCollision()
{
	initHitActor(0x08000023, 5, 0x80000000, unk6C, unk70, unk74, unk78);
}

void TBossEelTearsRecoverCollision::perform(u32 flags,
                                            JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		calcEntryRadius();

		for (int i = 0; i < mColCount; ++i) {
			if (mCollisions[i]->isActorTypeOf(ACTOR_TYPE_PLAYER))
				behaveToMario();
		}
	}

	THitActor::perform(flags, graphics);
}

void TBossEelTearsRecoverCollision::behaveToMario()
{
	SMS_SendMessageToMario(this, HIT_MESSAGE_ATTACK);
	unk80 = FALSE;
	onHitFlag(HIT_FLAG_NO_COLLISION);
}

void TBossEelTearsRecoverCollision::initCollision()
{
	unk74 = 400.0f;
	unk6C = 400.0f;
	unk78 = 400.0f;
	unk70 = 400.0f;
	initHitActor(0x2000002C, 3, 0x80000000, unk6C, unk70, unk74, unk78);
}

void TBossEelBarrierCollision::behaveToMario()
{
	JGeometry::TVec3<f32> velocity(0.0f, TBossEel::mForcePow, 0.0f);
	velocity.add(*gpMarioPos);
	SMS_MarioMoveRequest(velocity);
}

void TBossEelBarrierCollision::initCollision()
{
	unk74 = 0.0f;
	unk6C = 0.0f;
	unk78 = 0.0f;
	unk70 = 0.0f;
	initHitActor(0x08000023, 5, 0x80000000, unk6C, unk70, unk74, unk78);
}

void TBossEelAwaCollision::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		calcEntryRadius();

		if (gpMarioPos->y < mPosition.y + 500.0f)
			offHitFlag(HIT_FLAG_NO_COLLISION);

		if (gpMarioPos->y > mPosition.y + mAttackHeight)
			onHitFlag(HIT_FLAG_NO_COLLISION);

		for (int i = 0; i < mColCount; ++i) {
			if (mCollisions[i]->isActorTypeOf(ACTOR_TYPE_PLAYER))
				behaveToMario();
		}
	}

	if (flags & 2) {
		mPosition.x = unk68[0][3];
		mPosition.y = unk68[1][3] + 11000.0f;
		mPosition.z = unk68[2][3];
	}

	THitActor::perform(flags, graphics);
}

void TBossEelAwaCollision::behaveToMario()
{
	JGeometry::TVec3<f32> velocity(0.0f, 10.0f, 0.0f);
	velocity.y = 15.0f;
	*gpMarioSpeedY = 0.0f;
	velocity.add(*gpMarioPos);
	SMS_MarioMoveRequest(velocity);
}

void TBossEelAwaCollision::initCollision()
{
	unk74 = 2000.0f;
	unk6C = 2000.0f;
	unk78 = 2000.0f;
	unk70 = 2000.0f;
	initHitActor(0x08000003, 2, 0x80000000, unk6C, unk70, unk74, unk78);
}

void TBossEelBodyCollision::initCollision()
{
	unk6C = 2000.0f;
	unk74 = 2000.0f;
	unk70 = 5800.0f;
	unk78 = 5000.0f;
	initHitActor(0x08000023, 5, 0x80000000, unk6C, unk70, unk74, unk78);
}

void TBossEelVortex::reset()
{
	offHitFlag(HIT_FLAG_NO_COLLISION);
	onHitFlag(0x80000000);
	unk70 = 0;
}

void TBossEelVortex::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (unk6C)
		return;

	if (flags & 1) {
		TBossEelSaveParams* params = unk68->unk1E8;
		f32 scale                  = unk68->mScaling.x;
		f32 attackRadius           = params->mSLVortexAttackRadius.get() * scale;
		f32 attackHeight           = params->mSLVortexAttackHeight.get() * scale;
		f32 damageHeight           = params->mSLVortexDamageHeight.get() * scale;
		f32 damageRadius           = params->mSLVortexDamageRadius.get() * scale;
		mAttackRadius              = attackRadius;
		mAttackHeight              = attackHeight;
		mDamageRadius              = damageRadius;
		mDamageHeight              = damageHeight;
		calcEntryRadius();

		++unk70;
		if (unk70 > 30) {
			if (unk68->mMActor->checkCurBckFromIndex(14)
			    || unk68->mMActor->checkCurBckFromIndex(17)) {
				for (int i = 0; i < mColCount; ++i) {
					if (mCollisions[i]->isActorTypeOf(ACTOR_TYPE_PLAYER)) {
						JGeometry::TVec3<f32> velocity;
						velocity.x = unk68->mPosition.x - gpMarioPos->x;
						velocity.y = unk68->mPosition.y - gpMarioPos->y;
						velocity.z = unk68->mPosition.z - gpMarioPos->z;
						MsVECNormalize(&velocity, &velocity);

						f32 power = params->mSLBreathInPower.get();
						f32 pulse = fabsf(JMASin(unk70 * 0.9f));
						if (pulse > 1.0f)
							pulse = 1.0f;
						else if (pulse < 0.1f)
							pulse = 0.1f;

						power *= pulse;
						SMSRumbleMgr->start(8, &mPosition);

						if (unk68->mMActor->checkCurBckFromIndex(17))
							power *= 0.5f;

						velocity.x *= power;
						velocity.y *= power;
						velocity.z *= power;
						velocity.add(*gpMarioPos);
						SMS_MarioMoveRequest(velocity);
					}
				}
			} else {
				onHitFlag(HIT_FLAG_NO_COLLISION);
			}
		}
	}

	if (flags & 2) {
		mPosition.x
		    = unk68->mMActor->getModel()->mNodeMatrices[unk68->unk1A4][0][3];
		mPosition.y = unk68->mMActor->getModel()
		                  ->mNodeMatrices[unk68->unk1A4][1][3]
		            + 1000.0f;
		mPosition.z
		    = unk68->mMActor->getModel()->mNodeMatrices[unk68->unk1A4][2][3];
	}

	THitActor::perform(flags, graphics);
}

#define LOAD_BOSSEEL_PARTICLE(id, path)                                        \
	do {                                                                       \
		if (!gParticleFlagLoaded[id]) {                                        \
			gpResourceManager->load(path, id);                                 \
			gParticleFlagLoaded[id] = true;                                    \
		}                                                                      \
	} while (0)

void TBossEelManager::loadAfter()
{
	LOAD_BOSSEEL_PARTICLE(0xd3, "/scene/bosseel/jpa/ms_meo_awa_tooth.jpa");
	LOAD_BOSSEEL_PARTICLE(0xd4, "/scene/bosseel/jpa/ms_meo_awa_mouth.jpa");
	LOAD_BOSSEEL_PARTICLE(0x192, "/scene/bosseel/jpa/ms_meo_eyeblur.jpa");
	LOAD_BOSSEEL_PARTICLE(0x193, "/scene/bosseel/jpa/ms_meo_spin_smoke.jpa");
	LOAD_BOSSEEL_PARTICLE(0x194, "/scene/bosseel/jpa/ms_meo_spin_smoke_l.jpa");
	LOAD_BOSSEEL_PARTICLE(0x195, "/scene/bosseel/jpa/ms_meo_spin_awa.jpa");
	LOAD_BOSSEEL_PARTICLE(0x196, "/scene/bosseel/jpa/ms_meo_spin_awa_l.jpa");
	LOAD_BOSSEEL_PARTICLE(0x197, "/scene/bosseel/jpa/ms_meo_awa_body.jpa");
	LOAD_BOSSEEL_PARTICLE(0x198, "/scene/bosseel/jpa/ms_meo_awa_dead.jpa");
	LOAD_BOSSEEL_PARTICLE(0x199, "/scene/bosseel/jpa/ms_meo_suikomi.jpa");
	LOAD_BOSSEEL_PARTICLE(0x19a, "/scene/bosseel/jpa/ms_meo_tooth_wash.jpa");
	LOAD_BOSSEEL_PARTICLE(0x19b, "/scene/bosseel/jpa/ms_meo_tooth_kira.jpa");
	LOAD_BOSSEEL_PARTICLE(0x19c, "/scene/bosseel/jpa/ms_meo_tooth_always.jpa");
}

TBEelTears::TBEelTears(const char* name)
    : TSpineEnemy(name)
    , unk15C(nullptr)
    , unk160(TRUE)
    , unk164(0)
    , unk168(nullptr)
    , unk16C(nullptr)
{
}

const char** TBEelTears::getBasNameTable() const
{
	return bossEelTears_bastable;
}

void TBEelTears::kill()
{
	onHitFlag(HIT_FLAG_NO_COLLISION);
	unk16C->onHitFlag(HIT_FLAG_NO_COLLISION);
	onLiveFlag(LIVE_FLAG_DEAD);
}

BOOL TBEelTears::receiveMessage(THitActor*, u32 message)
{
	if (message == 0xf) {
		unk164 = 60;

		if (mSpine->getCurrentNerve() != &TNerveBEelTearsMoveUp::theNerve()
		    && mSpine->getCurrentNerve() != &TNerveOilBallStay::theNerve())
			mSpine->pushNerve(&TNerveBEelTearsWaterHit::theNerve());

		if (mSpine->getCurrentNerve() == &TNerveBEelTearsWaterHit::theNerve()) {
			MActor* actor = mMActor;
			actor->setFrameRate(SMSGetAnmFrameRate(), 0);
		}

		return TRUE;
	}

	return FALSE;
}

void TBEelTears::reset()
{
	mActorType = 0x8000003;
	TSpineEnemy::reset();
	mSpine->initWith(&TNerveBEelTearsGenerate::theNerve());
	onLiveFlag(LIVE_FLAG_UNK10);
	offLiveFlag(LIVE_FLAG_DEAD);
	offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
	offLiveFlag(LIVE_FLAG_UNK200);
	offLiveFlag(LIVE_FLAG_HIDDEN);
	mGroundPlane = TMap::getIllegalCheckData();
	offHitFlag(HIT_FLAG_NO_COLLISION);
}

TBEelTearsManager::TBEelTearsManager(const char* name)
    : TEnemyManager(name)
{
}

void TBEelTearsManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TBEelTearsSaveLoadParams("/enemy/bossEelTears.prm");
	TEnemyManager::load(stream);
}

void TBEelTearsManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 2)
		clipEnemies(graphics);

	for (int i = 0; i < getActiveObjNum(); ++i)
		getObj(i)->perform(flags, graphics);

	for (int i = 0; i < 30; ++i) {
		if (mDrops[i]->unk6C)
			mDrops[i]->perform(flags, graphics);
	}
}

TSpineEnemy* TBEelTearsManager::createEnemyInstance()
{
	return new TBEelTears("めおとウナギ涙");
}

void TBEelTearsManager::splitTears(JGeometry::TVec3<f32>& position)
{
	position.y += 600.0f;

	int splitNum = ((TBEelTearsSaveLoadParams*)unk38)->mSLTearsSplitNum.get();
	f32 minPos   = -250.0f;
	f32 maxPos   = 250.0f;

	for (int i = 0; i < 30; ++i) {
		TBEelTearsDrop* drop = mDrops[i];
		if (drop->unk6C)
			continue;

		JGeometry::TVec3<f32> dropPos = position;
		f32 offset = rand() * 0.000030517578f;
		offset *= maxPos - minPos;
		offset += minPos;
		dropPos.x += offset;
		offset = rand() * 0.000030517578f;
		offset *= maxPos - minPos;
		offset += minPos;
		dropPos.y += offset;
		offset = rand() * 0.000030517578f;
		offset *= maxPos - minPos;
		offset += minPos;
		dropPos.z += offset;

		--splitNum;
		drop->offHitFlag(HIT_FLAG_NO_COLLISION);
		drop->unk6C = TRUE;

		f32 minSpeed = 4.0f;
		f32 maxSpeed = 6.0f;
		f32 speed = rand() * 0.000030517578f;
		speed *= maxSpeed - minSpeed;
		speed += minSpeed;
		drop->unk70 = speed;
		drop->mPosition = dropPos;

		f32 minScale = 1.0f;
		f32 maxScale = 1.5f;
		f32 scale    = rand() * 0.000030517578f;
		scale *= maxScale - minScale;
		scale += minScale;

		TBEelTearsSaveLoadParams* params = drop->unk74->unk15C;
		scale = rand() * 0.000030517578f;
		scale *= params->mTearsDropScaleHigh - params->mTearsDropScaleLow;
		scale += params->mTearsDropScaleLow;
		drop->mScaling.x = scale;
		drop->mScaling.y = scale;
		drop->mScaling.z = scale;

		if (splitNum < 0)
			break;
	}
}

void TBEelTearsManager::createEnemies(int count)
{
	TEnemyManager::createEnemies(count);

	void* resource
	    = JKRFileLoader::getGlbResource("/scene/bossEelTears/tears_drop.bmd");
	SDLModelData* modelData
	    = new SDLModelData(J3DModelLoaderDataBase::load(resource, 0x11240000));
	TBEelTears* tears = (TBEelTears*)unk18[0];

	for (int i = 0; i < 30; ++i)
		mDrops[i] = new TBEelTearsDrop(tears, 0, modelData, "涙粒");
}

void TBEelTearsManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "tears.bmd", 0x11240000, 0 },
		{ "tears_waterhit.bmd", 0x11240000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TBEelTearsManager::loadAfter()
{
	LOAD_BOSSEEL_PARTICLE(0xd5,
	                      "/scene/bossEelTears/ms_meo_tear_bomb.jpa");
	LOAD_BOSSEEL_PARTICLE(0xd6,
	                      "/scene/bossEelTears/ms_meo_tear_awaget.jpa");
	LOAD_BOSSEEL_PARTICLE(0x19d,
	                      "/scene/bossEelTears/ms_meo_tear_awa.jpa");
}

#undef LOAD_BOSSEEL_PARTICLE

TBEelTearsSaveLoadParams::TBEelTearsSaveLoadParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mSLTearsUpSpeed, 5.0f)
    , PARAM_INIT(mSLTearsDamageUpSpeed, 1.0f)
    , PARAM_INIT(mSLTearsLiveHeight, 2000.0f)
    , PARAM_INIT(mSLTearsSplitNum, 2)
    , PARAM_INIT(mSLTearsDamageRadius, 600)
    , PARAM_INIT(mSLTearsDamageHeight, 400)
    , PARAM_INIT(mSLTearsAttackRadius, 500)
    , PARAM_INIT(mSLTearsAttackHeight, 300)
    , PARAM_INIT(mSLTearsDropDamageRadius, 600)
    , PARAM_INIT(mSLTearsDropDamageHeight, 400)
    , PARAM_INIT(mSLTearsDropAttackRadius, 500)
    , PARAM_INIT(mSLTearsDropAttackHeight, 300)
    , PARAM_INIT(mSLHighPolyDistY, 5.0f)
    , PARAM_INIT(mSLHitAnmFrameRate, 1.0f)
    , PARAM_INIT(mSLBodyScaleLow, 1.0f)
    , PARAM_INIT(mSLBodyScaleHigh, 1.0f)
    , PARAM_INIT(mSLTearsDropScaleLow, 1.0f)
    , PARAM_INIT(mSLTearsDropScaleHigh, 1.0f)
    , mBodyScaleLow(0.0f)
    , mBodyScaleHigh(1.0f)
    , mTearsDropScaleLow(0.0f)
    , mTearsDropScaleHigh(1.0f)
{
	TParams::load(mPrmPath);
	mBodyScaleLow       = mSLBodyScaleLow.get();
	mBodyScaleHigh      = mSLBodyScaleHigh.get();
	mTearsDropScaleLow  = mSLTearsDropScaleLow.get();
	mTearsDropScaleHigh = mSLTearsDropScaleHigh.get();
}

void TOilBall::load(JSUMemoryInputStream& stream)
{
	unk160 = FALSE;
	TSpineEnemy::load(stream);
	unk150 = mPosition;
	reset();
}

void TOilBall::calcRootMatrix() { TSpineEnemy::calcRootMatrix(); }

void TOilBall::reset()
{
	mActorType = 0x08000003;
	TSpineEnemy::reset();
	mSpine->initWith(&TNerveBEelTearsGenerate::theNerve());
	onLiveFlag(LIVE_FLAG_UNK10);
	offLiveFlag(LIVE_FLAG_DEAD);
	offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
	offLiveFlag(LIVE_FLAG_UNK200);
	offLiveFlag(LIVE_FLAG_HIDDEN);
	mGroundPlane = TMap::getIllegalCheckData();
	offHitFlag(HIT_FLAG_NO_COLLISION);
	mPosition = unk150;
	mSpine->initWith(&TNerveOilBallStay::theNerve());
	mMActor = mMActorKeeper->getMActor("tears.bmd");
	mMActor->setBckFromIndex(2);
	unk160 = FALSE;
}

void TOilBall::moveObject()
{
	TBEelTearsSaveLoadParams* params = unk15C;
	f32 scale                       = mScaling.x;
	mAttackRadius  = params->mSLTearsAttackRadius.get() * scale;
	mAttackHeight  = params->mSLTearsAttackHeight.get() * scale;
	mDamageRadius  = params->mSLTearsDamageRadius.get() * scale;
	mDamageHeight  = params->mSLTearsDamageHeight.get() * scale;
	calcEntryRadius();

	for (int i = 0; i < mColCount; ++i) {
		THitActor* hitActor = mCollisions[i];

		if (hitActor->isActorTypeOf(ACTOR_TYPE_PLAYER)) {
			if (mSpine->getCurrentNerve() == &TNerveBEelTearsMoveUp::theNerve()
			    || mSpine->getCurrentNerve() == &TNerveOilBallStay::theNerve()) {
				SMS_SendMessageToMario(this, HIT_MESSAGE_ATTACK);
				mSpine->pushNerve(&TNerveBEelTearsSplit::theNerve());
			}
		} else {
			JGeometry::TVec3<f32> velocity;
			velocity.zero();

			JGeometry::TVec3<f32> direction;
			direction.x = mPosition.x - hitActor->mPosition.x;
			direction.y = mPosition.y - hitActor->mPosition.y;
			direction.z = mPosition.z - hitActor->mPosition.z;

			if (direction.x == 0.0f && direction.y == 0.0f
			    && direction.z == 0.0f)
				direction.x += 1.0f;

			MsVECNormalize(&direction, &direction);
			direction.x *= 5.0f;
			direction.y *= 5.0f;
			direction.z *= 5.0f;
			velocity.add(direction);
			mLinearVelocity = velocity;
		}
	}

	TLiveActor::moveObject();
}

void TBEelTears::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 2);
	mMActor       = mMActorKeeper->createMActor("tears.bmd", 0);
	mMActorKeeper->createMActor("tears_waterhit.bmd", 0);
}

void TBEelTears::init(TLiveManager* manager)
{
	TSpineEnemy::init(manager);
	mActorType = 0x08000003;
	unk64 |= 0x80000000;
	setMActorAndKeeper();
	unk15C = (TBEelTearsSaveLoadParams*)getSaveParam();
	mSpine->initWith(&TNerveBEelTearsGenerate::theNerve());

	TScreenTexture* texture
	    = JDrama::TNameRefGen::search<TScreenTexture>("スクリーンテクスチャ");
	const ResTIMG* texInfo = texture->getTexture()->getTexInfo();
	J3DSkinDeform* skinDeform = new J3DSkinDeform;

	MActor* actor = mMActorKeeper->getMActor("tears.bmd");
	actor->getModel()->setSkinDeform(skinDeform,
	                                 J3D_DEFORM_ATTACH_FLAG_UNK_1);
	actor->resetDL();
	SMS_ChangeTextureAll(actor->getModel()->getModelData(), cDummyTextureName,
	                     *texInfo);
	actor->setLightType(3);

	actor = mMActorKeeper->getMActor("tears_waterhit.bmd");
	SMS_ChangeTextureAll(actor->getModel()->getModelData(), cDummyTextureName,
	                     *texInfo);
	actor->setLightType(3);

	onLiveFlag(LIVE_FLAG_DEAD);

	f32 scaleMin = unk15C->mBodyScaleLow;
	f32 scaleMax = unk15C->mBodyScaleHigh;
	mBodyScale   = scaleMin
	             + (scaleMax - scaleMin) * (rand() * 0.000030517578f);

	TIdxGroupObj* group = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
	MtxPtr mtx          = mMActor->getModel()->mNodeMatrices[0];
	unk16C = new TBossEelTearsRecoverCollision(mtx, cTearsRecoverCollisionName);
	unk16C->initCollision();

	JGadget::TList_pointer_void* list
	    = (JGadget::TList_pointer_void*)((u8*)group + 0x10);
	list->insert(list->end(), unk16C);
	unk16C->onHitFlag(HIT_FLAG_NO_COLLISION);
}

void TBEelTears::perform(u32 flags, JDrama::TGraphics* graphics)
{
	unk16C->perform(flags, graphics);

	if (mLiveFlag & LIVE_FLAG_DEAD)
		return;

	if (flags & 1)
		moveObject();

	u32 calcFlags = flags & 2;
	if (calcFlags) {
		Mtx mtx;
		SMS_GetLightPerspectiveForEffectMtx(mtx);
		setEffectMtxOnTex1(
		    mMActor->getModel()->getModelData()->getMaterialNodePointer(0),
		    mtx);
		updateAnmSound();
	}

	if (calcFlags)
		mMActor->frameUpdate();

	if (mLiveFlag & (LIVE_FLAG_HIDDEN | LIVE_FLAG_CLIPPED_OUT))
		return;

	if (calcFlags) {
		calcRootMatrix();
		mMActor->calc();
	}

	if (flags & 4)
		mMActor->viewCalc();

	if (flags & 0x200)
		mMActor->entry();

	THitActor::perform(flags, graphics);
}

void TBEelTears::calcRootMatrix()
{
	if (unk168) {
		if (mPosition.y < unk168[1][3])
			mPosition.y = unk168[1][3];

		TSpineEnemy::calcRootMatrix();

		if (mSpine->getCurrentNerve() == &TNerveBEelTearsGenerate::theNerve()) {
			f32 z = mPosition.z;
			f32 y = mPosition.y;
			f32 x = mPosition.x;
			TPosition3f mtx;

			mtx.translation(x, y, z);

			mtx.ref(0, 3) += 0.1f * (unk168[0][3] - mtx.ref(0, 3));
			mtx.ref(2, 3) += 0.1f * (unk168[2][3] - mtx.ref(2, 3));

			f32 scale = unk15C->mSLBodyScaleLow.get();
			mScaling.x = scale;
			mScaling.y = scale;
			mScaling.z = scale;
			mMActor->getModel()->setBaseScale(mScaling);

			PSMTXCopy(mtx.mMtx, mMActor->getModel()->getBaseTRMtx());
		}
	} else {
		TSpineEnemy::calcRootMatrix();
	}
}

void TBEelTears::moveObject()
{
	mVelocity.x *= 0.9f;
	mVelocity.z *= 0.9f;
	f32 liveHeight = unk15C->mSLTearsLiveHeight.get();

	mPosition.x += mVelocity.x;
	mPosition.z += mVelocity.z;

	if (((u8*)unk16C)[0x81] == FALSE && unk168) {
		if (mPosition.y > gpMarioPos->y + 3000.0f
		    || mPosition.y > unk168[1][3] + liveHeight) {
			kill();
			return;
		}
	}

	f32 scale      = mScaling.x;
	mAttackRadius  = unk15C->mSLTearsAttackRadius.get() * scale;
	mAttackHeight  = unk15C->mSLTearsAttackHeight.get() * scale;
	mDamageRadius  = unk15C->mSLTearsDamageRadius.get() * scale;
	mDamageHeight  = unk15C->mSLTearsDamageHeight.get() * scale;
	calcEntryRadius();

	for (int i = 0; i < mColCount; ++i) {
		THitActor* hitActor = mCollisions[i];

		if (hitActor->isActorTypeOf(ACTOR_TYPE_PLAYER)) {
			if (mSpine->getCurrentNerve() == &TNerveBEelTearsMoveUp::theNerve()) {
				SMS_SendMessageToMario(this, HIT_MESSAGE_ATTACK);
				mSpine->pushNerve(&TNerveBEelTearsSplit::theNerve());
			}
		} else {
			JGeometry::TVec3<f32> velocity;
			velocity.zero();

			JGeometry::TVec3<f32> direction;
			direction.x = mPosition.x - hitActor->mPosition.x;
			direction.y = mPosition.y - hitActor->mPosition.y;
			direction.z = mPosition.z - hitActor->mPosition.z;

			if (direction.x == 0.0f && direction.y == 0.0f
			    && direction.z == 0.0f)
				direction.x += 1.0f;

			MsVECNormalize(&direction, &direction);
			direction.x *= 5.0f;
			direction.y *= 5.0f;
			direction.z *= 5.0f;
			velocity.add(direction);
			mLinearVelocity = velocity;
		}
	}

	TLiveActor::moveObject();
}

void TBEelTearsDrop::perform(u32 flags, JDrama::TGraphics* graphics)
{
	THitActor::perform(flags, graphics);

	if (flags & 1) {
		mPosition.y += unk70;
		if (mPosition.y > gpMarioPos->y + 2000.0f)
			unk6C = FALSE;
	}

	if (flags & 2) {
		Mtx mtx;
		MsMtxSetXYZRPH(mtx, mPosition.x, mPosition.y, mPosition.z,
		               mRotation.x, mRotation.y, mRotation.z);

		MActor* actor = unk68->getMActor();
		PSMTXCopy(mtx, actor->getModel()->getBaseTRMtx());

		f32 scale = unk74->unk15C->mSLTearsDropScaleLow.get();
		mScaling.x = scale;
		mScaling.y = scale;
		mScaling.z = scale;
		actor->getModel()->setBaseScale(mScaling);
	}

	unk68->getMActor()->perform(flags, graphics);
}

TBEelTearsDrop::TBEelTearsDrop(TBEelTears* tears, int index,
                               SDLModelData* model_data, const char* name)
    : THitActor(name)
    , unk68(nullptr)
    , unk74(tears)
{
	unk68 = new TSharedParts(tears, index, model_data, 0, "<TSharedParts>");

	TBEelTearsSaveLoadParams* params = tears->unk15C;
	initHitActor(0x2000002c, 3, 0x80000000,
	             params->mSLTearsDropAttackRadius.get(),
	             params->mSLTearsDropAttackHeight.get(),
	             params->mSLTearsDropDamageRadius.get(),
	             params->mSLTearsDropDamageHeight.get());

	TIdxGroupObj* group = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
	JGadget::TList_pointer_void* list
	    = (JGadget::TList_pointer_void*)((u8*)group + 0x10);
	list->insert(list->end(), this);
	unk6C = FALSE;

	TScreenTexture* texture
	    = JDrama::TNameRefGen::search<TScreenTexture>("スクリーンテクスチャ");
	new J3DSkinDeform;

	MActor* actor = unk68->getMActor();
	SMS_ChangeTextureAll(actor->getModel()->getModelData(), cDummyTextureName,
	                     *texture->getTexture()->getTexInfo());
	actor->setBckFromIndex(0);
	actor->setLightType(3);
}
