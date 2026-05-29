#include <Enemy/Killer.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/HitActor.hpp>
#include <Strategic/ObjModel.hpp>
#include <System/Particles.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <M3DUtil/MActor.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DNode.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DCluster.hpp>

// rogue includes needed for matching sinit & rodata
#include <M3DUtil/InfectiousStrings.hpp>

static TKiller* gpCurKiller;

bool TKiller::mTrampleDie;
bool TKiller::mRollSw;
bool TKiller::mSerialBomb;

f32 TFlyEnemy::mTestSp         = 2.5f;
s32 TFlyEnemy::mInvalidTime    = 200;
f32 TFlyEnemy::mTestMarioSpMax = 12.0f;

static const char* killer_bastable[] = {
	"/scene/killer/bas/downkiller_down1.bas",
	nullptr,
	nullptr,
	"/scene/killer/bas/killer_search1.bas",
	nullptr,
};

static int KillerBodyCallback(J3DNode* node, int when)
{
	if (when != 0)
		return 1;
	if (gpCurKiller == nullptr || !TKiller::mRollSw)
		return 1;
	if (!gpCurKiller->isRollFly())
		return 1;
	return 1;
}

// ---------------------------------------------------------------------------
// TKillerManager
// ---------------------------------------------------------------------------

TKillerManager::TKillerManager(const char* name)
    : TSmallEnemyManager(name)
{
	gpCurKiller = nullptr;
}

void TKillerManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "killer_model1.bmd", 0x14240000, 0 },
		{ "downkiller_model1.bmd", 0x14240000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TKillerManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38 = new TKillerParams("/enemy/killer.prm");
}

TSpineEnemy* TKillerManager::createEnemyInstance()
{
	return new TKiller("\x83\x4C\x83\x89\x81\x5B");
}

// ---------------------------------------------------------------------------
// TFlyEnemyParams / TKillerParams
// ---------------------------------------------------------------------------

TFlyEnemyParams::TFlyEnemyParams(const char* path)
    : TWalkerEnemyParams(path)
    , PARAM_INIT(mSLNormalFlyGravityY, 0.2f)
    , PARAM_INIT(mSLNormalFlySpeed, 10.0f)
    , PARAM_INIT(mSLChaseFlyGravityY, 0.1f)
    , PARAM_INIT(mSLChaseDist, 2000.0f)
    , PARAM_INIT(mSLForceGravityY, 0.1f)
{
}

TKillerParams::TKillerParams(const char* path)
    : TFlyEnemyParams(path)
    , PARAM_INIT(mSLWaterAddGravityY, 1.0f)
    , PARAM_INIT(mSLChaseTimer, 1000)
    , PARAM_INIT(mSLBombRange, 300.0f)
{
}

// ---------------------------------------------------------------------------
// TFlyEnemy
// ---------------------------------------------------------------------------

TFlyEnemy::TFlyEnemy(const char* name)
    : TWalkerEnemy(name)
{
	mFlyState     = 0;
	mFlyParams    = nullptr;
	mFlyTimer     = 0;
	mIsChaseMode  = 1;
	mColorVariant = 0;
}

void TFlyEnemy::init(TLiveManager* manager)
{
	TWalkerEnemy::init(manager);
	mFlyParams = getSaveParam2();
}

f32 TFlyEnemy::getGravityY() const
{
	if (mSpine->getCurrentNerve() == &TNerveFlyEnemyChaseFly::theNerve())
		return mCurGravityY;
	return mFlyParams->mSLNormalFlyGravityY.get();
}

void TFlyEnemy::reset()
{
	TWalkerEnemy::reset();
	mFlyTimer      = 0;
	mFlyState      = 1;
	mChaseFinished = 0;
	mCurGravityY   = mFlyParams->mSLNormalFlyGravityY.get();
	mIsChaseMode   = 0;
}

void TFlyEnemy::bind()
{
	if (mSpine->getCurrentNerve() == &TNerveFlyEnemyChaseFly::theNerve()
	    || mFlyTimer < mInvalidTime)
		fly();
	else
		TLiveActor::bind();
}

void TFlyEnemy::calcChaseParam()
{
	JGeometry::TVec3<f32> diff;
	diff.x = (gpMarioPos->x - mPosition.x) * 1.1f;
	diff.y = gpMarioPos->y - mPosition.y;
	diff.z = (gpMarioPos->z - mPosition.z) * 1.1f;

	if (diff.y > 100.0f) {
		if (mIsChaseMode) {
			mCurGravityY = mFlyParams->mSLChaseFlyGravityY.get();
			mFlyState    = 2;
		} else {
			mPosition.y -= 1.0f;
		}
	} else {
		mCurGravityY = mFlyParams->mSLNormalFlyGravityY.get();
		JGeometry::TVec3<f32> vel(0.0f, 0.0f, 0.0f);
		if (mFlyState == 2 && diff.y > 150.0f) {
			mFlyState = 0;
			MsVECNormalize(&diff, &diff);
			vel.x        = diff.x * mFlyParams->mSLNormalFlySpeed.get();
			vel.z        = diff.z * mFlyParams->mSLNormalFlySpeed.get();
			mCurGravityY = mFlyParams->mSLForceGravityY.get();
		} else {
			mPosition.y -= 3.0f;
		}
		vel.z = 0.0f;
		vel.x = 0.0f;
		mVelocity.set(vel);
	}
}

void TFlyEnemy::fly()
{
	JGeometry::TVec3<f32> pos = mPosition;
	pos.add(mLinearVelocity);

	JGeometry::TVec3<f32> vel = mVelocity;
	vel.x += gpMarioPos->x / mTestMarioSpMax;
	vel.z += gpMarioPos->z / mTestMarioSpMax;

	pos.add(vel);
	pos.y += mCurGravityY;

	if (pos.y <= mPosition.y) {
		if (mFlyTimer > mInvalidTime) {
			offLiveFlag(0x80);
			mVelocity.set(0.0f, 0.0f, 0.0f);
		}
	} else {
		onLiveFlag(0x80);
	}

	JGeometry::TVec3<f32> delta = pos;
	delta.sub(mPosition);
	mLinearVelocity.set(delta);
}

void TFlyEnemy::flyBehavior() { }
void TFlyEnemy::setChaseFlyAnm() { }
void TFlyEnemy::setNormalFlyAnm() { }
void TFlyEnemy::setAfterDeadEffect() { }

// ---------------------------------------------------------------------------
// TKiller
// ---------------------------------------------------------------------------

TKiller::TKiller(const char* name)
    : TFlyEnemy(name)
{
}

void TKiller::init(TLiveManager* manager)
{
	TWalkerEnemy::init(manager);

	TKillerParams* p = (TKillerParams*)getSaveParam();
	mFlyParams       = p;
	mActorType       = 0x1000001F;
	unk150           = 0x11;
	mKillerParams    = p;

	mSpine->initWith(&TNerveFlyEnemyNormalFly::theNerve());
	onLiveFlag(0x400);
	offLiveFlag(0x800);
	onHitFlag(0x40000000);

	J3DSkinDeform* deform = new J3DSkinDeform;
	mMActor->getModel()->setSkinDeform(deform, J3D_DEFORM_ATTACH_FLAG_UNK_1);
	mMActor->resetDL();
	mMActor->setJointCallback(1, &KillerBodyCallback);
}

void TKiller::reset()
{
	gpCurKiller = this;
	TWalkerEnemy::reset();
	mFlyTimer      = 0;
	mFlyState      = 1;
	mChaseFinished = 0;
	mCurGravityY   = mFlyParams->mSLNormalFlyGravityY.get();
	mIsChaseMode   = 0;

	mColor3.b     = 0;
	mColor3.g     = 0;
	mColor3.r     = 0;
	mColor2.b     = 0;
	mColor2.g     = 0;
	mColor2.r     = 0;
	mColorVariant = 0;

	if (MsRandF() < 0.05f) {
		mColorVariant = 1;
		mColor2.r     = 200;
		mColor2.g     = 185;
		mColor2.b     = 0;
		mColor3.r     = 255;
		mColor3.g     = 225;
		mColor3.b     = 70;
	}
}

void TKiller::calcRootMatrix()
{
	gpCurKiller = this;
	if (mRotation.x > 90.0f)
		mRotation.x = 90.0f;
	else if (mRotation.x < -25.0f)
		mRotation.x = -25.0f;
	TSpineEnemy::calcRootMatrix();
}

void TKiller::bind()
{
	if (mSpine->getCurrentNerve() == &TNerveFlyEnemyChaseFly::theNerve()
	    || mFlyTimer < mInvalidTime)
		fly();
	else
		TLiveActor::bind();

	mFlyTimer++;

	if (mSpine->getCurrentNerve() != &TNerveKillerExplosion::theNerve()) {
		if (!checkLiveFlag(0x80) && mFlyTimer > mInvalidTime)
			mSpine->pushNerve(&TNerveKillerExplosion::theNerve());
	}
}

const char** TKiller::getBasNameTable() const { return killer_bastable; }

void TKiller::genEventCoin()
{
	int count = mColorVariant ? 8 : 2;
	for (int i = 0; i < count; i++) {
		// TODO(INVESTIGATION): ring direction via JMA sin/cos matrix (see asm 0x13F0)
		if (TMapObjBase* obj = gpItemManager->makeObjAppear(
		        mPosition.x, mPosition.y, mPosition.z, 0x2000000e, true)) {
			obj->mPosition.y = mPosition.y;
		}
	}
}

void TKiller::behaveToWater(THitActor* sender)
{
	if (mSpine->getCurrentNerve() == &TNerveFlyEnemyNormalFly::theNerve()
	    || mSpine->getCurrentNerve() == &TNerveFlyEnemyChaseFly::theNerve())
		genEventCoin();

	if (mSpine->getCurrentNerve() != &TNerveKillerExplosion::theNerve()) {
		mSpine->pushNerve(&TNerveKillerExplosion::theNerve());
		onHitFlag(0x1);
		mVelocity.set(0.0f, 0.0f, 0.0f);
	}
}

void TKiller::changeOut()
{
	onLiveFlag(0x1);
	genEventCoin();
	onHitFlag(0x1);
}

void TKiller::setDeadAnm()
{
	mMActor = getActorKeeper()->getMActor("downkiller_model1.bmd");
	setBckAnm(0);
}

void TKiller::attackToMario()
{
	if (gpMarioPos->y < mPosition.y) {
		if (mSpine->getCurrentNerve() != &TNerveKillerExplosion::theNerve()) {
			mSpine->pushNerve(&TNerveKillerExplosion::theNerve());
			sendAttackMsgToMario();
		}
	}
}

void TKiller::forceKill() { }

void TKiller::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 2);
	mMActor       = mMActorKeeper->createMActor("killer_model1.bmd", 3);
	mMActorKeeper->createMActor("downkiller_model1.bmd", 3);

	s32 noseMatIdx = getActorKeeper()
	                     ->getMActor("killer_model1.bmd")
	                     ->getModel()
	                     ->getModelData()
	                     ->getMaterialName()
	                     ->getIndex("_nosemat1");
	s32 eyesMatIdx = getActorKeeper()
	                     ->getMActor("killer_model1.bmd")
	                     ->getModel()
	                     ->getModelData()
	                     ->getMaterialName()
	                     ->getIndex("_eyesmat1");
	s32 bodyMatIdx = getActorKeeper()
	                     ->getMActor("killer_model1.bmd")
	                     ->getModel()
	                     ->getModelData()
	                     ->getMaterialName()
	                     ->getIndex("_body1");

	SMS_InitPacket_OneTevColor(getMActor()->getModel(), noseMatIdx, GX_TEVREG0,
	                           &mColor0);
	SMS_InitPacket_OneTevColor(getMActor()->getModel(), eyesMatIdx, GX_TEVREG0,
	                           &mColor1);
	SMS_InitPacket_OneTevColor(getMActor()->getModel(), bodyMatIdx, GX_TEVREG0,
	                           &mColor2);
	SMS_InitPacket_OneTevColor(
	    getActorKeeper()->getMActor("downkiller_model1.bmd")->getModel(),
	    bodyMatIdx, GX_TEVREG0, &mColor3);
}

bool TKiller::isHitValid(u32 message)
{
	if (message == 0xb) {
		onLiveFlag(0x1);
		onHitFlag(0x1);
		genEventCoin();
		return false;
	}
	if (mTrampleDie)
		mCurGravityY -= 12.0f;
	return false;
}

bool TKiller::isCollidMove(THitActor* actor)
{
	if (mSerialBomb
	    && mSpine->getCurrentNerve() == &TNerveFlyEnemyChaseFly::theNerve())
		mSpine->pushNerve(&TNerveKillerExplosion::theNerve());
	return true;
}

bool TKiller::isFindMario(float param_1) { return isFindMarioFromParam(param_1); }

void TKiller::flyBehavior()
{
	mTurnSpeed = mKillerParams->getSLTurnSpeedLow();
	if (mSpine->getTime() > mKillerParams->mSLChaseTimer.get())
		mCurGravityY -= mKillerParams->mSLWaterAddGravityY.get();
	if (checkCurAnmEnd(0) && isBckAnm(3))
		setBckAnm(1);
	mRollAnim += 2.5f;
}

void TKiller::setChaseFlyAnm() { setBckAnm(3); }

void TKiller::setNormalFlyAnm()
{
	mMActor = getActorKeeper()->getMActor("killer_model1.bmd");
	setBckAnm(2);
	mRollAnim = 0.0f;
	mFlyTimer = 0;
}

void TKiller::setColorType()
{
	if (mColorVariant != 0)
		mIsChaseMode = 0;

	if (mIsChaseMode == 0)
		return;

	mColor2.r = 0x46;
	mColor2.g = 0x14;
	mColor2.b = 0x46;
	mColor3.r = 0x46;
	mColor3.g = 0x14;
	mColor3.b = 0x46;
}

bool TKiller::isRollFly()
{
	return mSpine->getCurrentNerve() == &TNerveFlyEnemyChaseFly::theNerve()
	    && (mCurrentBckAnm == 1 ? true : false);
}

// ---------------------------------------------------------------------------
// Nerves
// ---------------------------------------------------------------------------

DEFINE_NERVE(TNerveFlyEnemyChaseFly, TLiveActor)
{
	TFlyEnemy* self = (TFlyEnemy*)spine->getBody();
	self->calcChaseParam();
	if (spine->getTime() == 0)
		self->setChaseFlyAnm();
	self->fly();
	return FALSE;
}

DEFINE_NERVE(TNerveFlyEnemyNormalFly, TLiveActor)
{
	TFlyEnemy* self = (TFlyEnemy*)spine->getBody();
	if (spine->getTime() == 0)
		self->setNormalFlyAnm();
	self->flyBehavior();
	self->fly();
	return FALSE;
}

DEFINE_NERVE(TNerveKillerExplosion, TLiveActor)
{
	TKiller* self = (TKiller*)spine->getBody();
	if (spine->getTime() == 0)
		self->setAfterDeadEffect();
	return FALSE;
}
