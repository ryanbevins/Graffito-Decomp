#include <Enemy/Killer.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/EffectObj.hpp>
#include <Camera/CameraShake.hpp>
#include <System/MarDirector.hpp>
#include <JSystem/JMath.hpp>
#include <MarioUtil/RumbleMgr.hpp>
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
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <Map/MapCollisionData.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <System/Application.hpp>
#include <MoveBG/MapObjBlock.hpp>
#include <dolphin/mtx.h>

// rogue includes needed for matching sinit & rodata
#include <M3DUtil/InfectiousStrings.hpp>

static TKiller* gpCurKiller;

bool TKiller::mSerialBomb = true;
bool TKiller::mTrampleDie = true;
bool TKiller::mRollSw;

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

	u16 idx         = ((J3DJoint*)node)->getJntNo();
	MtxPtr jointMtx = gpCurKiller->getModel()->getAnmMtx(idx);

	f32 scale = gpCurKiller->getBodyScale();

	Mtx scaleMtx;
	scaleMtx[0][3] = 0.0f;
	scaleMtx[1][3] = 0.0f;
	scaleMtx[2][3] = 0.0f;
	scaleMtx[0][0] = scale;
	scaleMtx[0][1] = 0.0f;
	scaleMtx[0][2] = 0.0f;
	scaleMtx[1][0] = 0.0f;
	scaleMtx[1][1] = scale;
	scaleMtx[1][2] = 0.0f;
	scaleMtx[2][0] = 0.0f;
	scaleMtx[2][1] = 0.0f;
	scaleMtx[2][2] = scale;

	f32 s = JMASin(gpCurKiller->mRollAnim);
	f32 c = JMACos(gpCurKiller->mRollAnim);

	Mtx rollMtx;
	rollMtx[0][0] = c;
	rollMtx[0][1] = -s;
	rollMtx[0][2] = 0.0f;
	rollMtx[0][3] = 0.0f;
	rollMtx[1][0] = s;
	rollMtx[1][1] = c;
	rollMtx[1][2] = 0.0f;
	rollMtx[1][3] = 0.0f;
	rollMtx[2][0] = 0.0f;
	rollMtx[2][1] = 0.0f;
	rollMtx[2][2] = 1.0f;
	rollMtx[2][3] = 0.0f;

	PSMTXConcat(jointMtx, rollMtx, jointMtx);
	PSMTXConcat(jointMtx, scaleMtx, jointMtx);
	PSMTXConcat(J3DSys::mCurrentMtx, rollMtx, J3DSys::mCurrentMtx);
	PSMTXConcat(J3DSys::mCurrentMtx, scaleMtx, J3DSys::mCurrentMtx);
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
	TParams::load(mPrmPath);
}

TKillerParams::TKillerParams(const char* path)
    : TFlyEnemyParams(path)
    , PARAM_INIT(mSLWaterAddGravityY, 1.0f)
    , PARAM_INIT(mSLChaseTimer, 1000)
    , PARAM_INIT(mSLBombRange, 300.0f)
{
	TParams::load(mPrmPath);
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
	diff.x = gpMarioPos->x - mPosition.x;
	diff.y = gpMarioPos->y - mPosition.y;
	diff.z = gpMarioPos->z - mPosition.z;
	diff.x *= 1.1f;
	diff.z *= 1.1f;

	JGeometry::TVec3<f32> target;
	target.x = mPosition.x + diff.x;
	target.y = mPosition.y + diff.y;
	target.z = mPosition.z + diff.z;

	if (mIsChaseMode) {
		setGoalPath(TPathNode(target));
	}

	if (diff.y > 100.0f || fabs(diff.y) < 100.0f) {
		if (mIsChaseMode) {
			if (mFlyState != 2 && mSprayedByWaterCooldown == 0) {
				mCurGravityY = mFlyParams->mSLChaseFlyGravityY.get();
			}
			mFlyState = 2;
		} else {
			mPosition.y -= 1.0f;
		}
	} else {
		mCurGravityY = mFlyParams->mSLNormalFlyGravityY.get();
		JGeometry::TVec3<f32> vel(0.0f, 0.0f, 0.0f);
		if (mFlyState != 2 || diff.y > 150.0f) {
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
		mVelocity = vel;
	}
}

void TFlyEnemy::fly()
{
	JGeometry::TVec3<f32> pos = mPosition;
	pos.add(mLinearVelocity);

	JGeometry::TVec3<f32> vel = mVelocity;
	vel.x += *gpMarioSpeedX / mTestMarioSpMax;
	vel.z += *gpMarioSpeedZ / mTestMarioSpMax;

	pos.add(vel);
	pos.y += mCurGravityY;

	mGroundHeight = gpMap->checkGround(pos.x, pos.y + mHeadHeight, pos.z,
	                                   &mGroundPlane);
	mGroundHeight += 1.0f;

	if (pos.y <= mGroundHeight) {
		if (mFlyTimer > mInvalidTime) {
			offLiveFlag(0x80);
			mVelocity.set(0.0f, 0.0f, 0.0f);
			pos.y = mGroundHeight;
		}

		const TLiveActor* groundActor = mGroundPlane->getActor();
		if (groundActor && groundActor->isActorType(0x4000000a))
			((TLiveActor*)groundActor)->kill();

		if (mGroundPlane->isIllegalData())
			kill();
	} else {
		onLiveFlag(0x80);
	}

	JGeometry::TVec3<f32> delta = pos;
	delta.sub(mPosition);
	mLinearVelocity = delta;
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
	mKillerParams    = (TKillerParams*)getSaveParam();

	mSpine->initWith(&TNerveFlyEnemyNormalFly::theNerve());
	onLiveFlag(0x400);
	offLiveFlag(0x800);
	onHitFlag(0x40000000);

	J3DModel* model = mMActor->getModel();
	if (model->getSkinDeform() == nullptr) {
		J3DSkinDeform* deform = new J3DSkinDeform;
		model->setSkinDeform(deform, J3D_DEFORM_ATTACH_FLAG_UNK_1);
	}
	mMActor->resetDL();
	if (mInstanceIndex == 0) {
		u8 i = 0;
		while (i < getModel()->getModelData()->getJointNum())
			i++;
	}
	mMActor->setJointCallback(1, &KillerBodyCallback);
	unk188 = 0.0f;
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

	TMsRange<f32> colorRollRange(0.0f, 1.0f);

	mColor3.b     = 0;
	mColor3.g     = 0;
	mColor3.r     = 0;
	mColor2.b     = 0;
	mColor2.g     = 0;
	mColor2.r     = 0;
	mColorVariant = 0;

	if (colorRollRange.rand() < 0.05f) {
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
	if (gpMarDirector->checkUnk4CFlag(0xf)) {
		onLiveFlag(1);
		onHitFlag(1);
	}

	if (isBckAnm(2)) {
		if (mColorVariant != 0) {
			mColor1.r = 170;
			mColor0.r = 170;
			mColor1.g = 140;
			mColor0.g = 140;
			mColor1.b = 0;
			mColor0.b = 0;
		} else {
			mColor1.b = 0;
			mColor1.g = 0;
			mColor1.r = 0;
			mColor0.b = 0;
			mColor0.g = 0;
			mColor0.r = 0;
		}
	}

	if (isBckAnm(3)) {
		mColor0.b = 0;
		mColor0.g = 0;
		mColor1.r = 0;
		if (mSpine->getTime() % 10 < 5) {
			mColor0.r = 0;
			mColor1.b = 0;
			mColor1.g = 0;
		}
		if (mColorVariant != 0) {
			if (mSpine->getTime() % 10 < 5) {
				mColor0.r = 170;
				mColor0.g = 140;
				mColor0.b = 0;
			} else {
				mColor0.r = 180;
				mColor0.g = 140;
				mColor0.b = 150;
			}
		}
		if (mIsChaseMode != 0) {
			if (mSpine->getTime() % 10 < 5) {
				mColor1.r = 200;
				mColor0.r = 200;
				mColor0.g = 0;
				mColor0.b = 0;
			} else {
				mColor0.r = 70;
				mColor0.g = 20;
				mColor0.b = 70;
			}
		}
	}

	if (isBckAnm(1)) {
		mColor1.r = 0;
		mColor1.b = 0;
		mColor1.g = 0;
		mColor0.b = 0;
		mColor0.g = 0;

		f32 pulse = fabsf(JMASin(360.0f * (f32)mSpine->getTime() / 120.0f));

		if (mColorVariant != 0) {
			mColor2.r = 170;
			mColor2.g = 140;
			mColor2.b = 0;
			mColor0.r = (u8)(int)(10.0f * pulse + 160.0f);
			mColor0.g = (u8)(int)(30.0f * pulse + 140.0f);
			mColor0.b = (u8)(int)(150.0f * pulse);
		}
		if (mIsChaseMode != 0) {
			mColor2.r = 70;
			mColor2.g = 20;
			mColor2.b = 70;
			mColor3.r = 70;
			mColor3.g = 20;
			mColor3.b = 70;
			mColor0.r = (u8)(int)(130.0f * pulse + 70.0f);
			mColor0.g = (u8)(int)(20.0f - 20.0f * pulse);
			mColor0.b = (u8)(int)(70.0f - 70.0f * pulse);
		}
	}

	gpCurKiller = this;

	f32 rotX = mRotation.x;
	if (rotX > 90.0f)
		rotX = 90.0f;
	else if (rotX < -25.0f)
		rotX = -25.0f;
	mRotation.x = rotX;

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
		if (!checkLiveFlag(0x80)) {
			if (mFlyTimer > mInvalidTime)
				mSpine->pushNerve(&TNerveKillerExplosion::theNerve());
		} else {
			if (mFlyTimer > mInvalidTime) {
				TBGWallCheckRecord rec(mPosition.x, mPosition.y + mHeadHeight,
				                       mPosition.z, 2.0f * mBodyRadius, 1, 0);
				if (gpMap->isTouchedWallsAndMoveXZ(&rec)) {
					const TLiveActor* wallActor = rec.mResultWalls[0]->mActor;
					if (wallActor != nullptr
					    && wallActor->isActorType(0x4000000a))
						((TLiveActor*)wallActor)->kill();
					mSpine->pushNerve(&TNerveKillerExplosion::theNerve());
				}
			}
		}
	}

	if (checkLiveFlag(0x80)) {
		if (gpMSound->gateCheck(0x20a9))
			MSoundSESystem::MSoundSE::startSoundActor(0x20a9, &mPosition, 0,
			                                          nullptr, 0, 4);

		if (mRotation.x > 90.0f)
			mRotation.x = 90.0f;
		else if (mRotation.x < -25.0f)
			mRotation.x = -25.0f;

		MsMtxSetXYZRPH(mRollMtx.mMtx, mPosition.x, mPosition.y, mPosition.z,
		               mRotation.x, mRotation.y, mRotation.z);
		gpMarioParticleManager->emitAndBindToMtxPtr(0x174, mRollMtx.mMtx, 1,
		                                            this);
	}
}

const char** TKiller::getBasNameTable() const { return killer_bastable; }

void TKiller::genEventCoin()
{
	Mtx rotMtx;
	JGeometry::TVec3<f32> dir;

	int count = mColorVariant ? 8 : 2;
	for (int i = 0; i < count; i++) {
		f32 angle = 360.0f * (1.0f / count) * (i + 1);
		f32 sin   = JMASin(angle);
		f32 cos   = JMACos(angle);

		dir.set(0.0f, 0.0f, 30.0f);

		rotMtx[0][0] = cos;
		rotMtx[0][1] = 0.0f;
		rotMtx[0][2] = sin;
		rotMtx[0][3] = 0.0f;
		rotMtx[1][0] = 0.0f;
		rotMtx[1][1] = 1.0f;
		rotMtx[1][2] = 0.0f;
		rotMtx[1][3] = 0.0f;
		rotMtx[2][0] = -sin;
		rotMtx[2][1] = 0.0f;
		rotMtx[2][2] = cos;
		rotMtx[2][3] = 0.0f;

		PSMTXMultVec(rotMtx, &dir, &dir);

		TMapObjBase* obj = gpItemManager->makeObjAppear(
		    mPosition.x + dir.x, mPosition.y, mPosition.z + dir.z, 0x2000000e,
		    true);
		if (obj) {
			obj->mPosition.y = mPosition.y;
			MsVECNormalize(&dir, &dir);
			obj->mVelocity.set(3.0f * dir.x, 20.0f, 3.0f * dir.z);
			obj->offLiveFlag(0x10);
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
	if (gpMSound->gateCheck(0x293d))
		MSoundSESystem::MSoundSE::startSoundActor(0x293d, &mPosition, 0, nullptr,
		                                          0, 4);

	onLiveFlag(0x1);
	genEventCoin();
	onHitFlag(0x1);

	mPosition = mJuiceBlock->mPosition;
	gpMarioParticleManager->emitAndBindToPosPtr(0xcd, &mPosition, 0, nullptr);
	mMActor->setFrameRate(SMSGetAnmFrameRate(), 0);
	mJuiceBlock->kill();
	mJuiceBlock = nullptr;
}

void TKiller::setDeadAnm()
{
	mMActor = getActorKeeper()->getMActor("downkiller_model1.bmd");
	setBckAnm(0);
	TSpineEnemy* effectBase = gpConductor->makeOneEnemyAppear(
	    mPosition, "エフェクト爆発マネージャー", 1);
	if (effectBase != nullptr) {
		TEffectExplosion* effect = (TEffectExplosion*)effectBase;
		effect->generate(mPosition, mScaling);
	}
	gpCameraShake->startShake(CAM_SHAKE_MODE_UNK6, 1.0f);
	SMSRumbleMgr->start(0x15, 5, (f32*)nullptr);
}

void TKiller::attackToMario()
{
	if (gpMarioPos->y < mPosition.y) {
		if (mSpine->getCurrentNerve() != &TNerveKillerExplosion::theNerve()) {
			mSpine->pushNerve(&TNerveKillerExplosion::theNerve());
			sendAttackMsgToMario();
		} else {
			SMS_SendMessageToMario(this, HIT_MESSAGE_UNKA);
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
	if (actor->isActorType(0x4000000a))
		((TLiveActor*)actor)->kill();

	if (mSerialBomb
	    && mSpine->getCurrentNerve() == &TNerveFlyEnemyChaseFly::theNerve())
		mSpine->pushNerve(&TNerveKillerExplosion::theNerve());
	return true;
}

bool TKiller::isFindMario(float param_1)
{
	TSmallEnemyParams* prms = (TSmallEnemyParams*)getSaveParam();

	if (fabs(gpMarioPos->y - mPosition.y) < prms->mSLSearchHeight.get()) {
		JGeometry::TVec3<f32> marioPos(gpMarioPos->x, gpMarioPos->y,
		                              gpMarioPos->z);

		f32 searchLength = prms->mSLSearchLength.get();
		f32 searchAngle  = prms->mSLSearchAngle.get();
		f32 searchAware  = prms->mSLSearchAware.get();

		if (isInSight(marioPos, searchLength * param_1, searchAngle * param_1,
		              searchAware * param_1))
			return true;
		else
			return false;
	}

	return false;
}

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
	if (mSpine->getCurrentNerve() == &TNerveFlyEnemyChaseFly::theNerve()
	    && (mCurrentBckAnm == 1 ? true : false))
		return true;
	return false;
}

// ---------------------------------------------------------------------------
// Nerves
// ---------------------------------------------------------------------------

#pragma dont_inline on
f32 MsSin(f32 v) { return JMASin(v); }
f32 MsCos(f32 v) { return JMACos(v); }
#pragma dont_inline off

static inline JGeometry::TVec3<f32> makeKillerMove(f32 x, f32 y, f32 z)
{
	return JGeometry::TVec3<f32>(x, y, z);
}

DEFINE_NERVE(TNerveFlyEnemyChaseFly, TLiveActor)
{
	TKiller* self = (TKiller*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mChaseTarget = self->getVelocity();
		self->calcChaseParam();
		self->setChaseFlyAnm();
	}

	f32 speed = 1.0f;
	if (self->mIsChaseMode == 0) {
		f32 sp = self->mMarchSpeed;
		self->mChaseTarget.y = 0.1f;
		speed = JGeometry::TUtil<f32>::sqrt(self->mChaseTarget.squared())
		      / (sp * sp) * TFlyEnemy::mTestSp;
	} else if (self->mFlyState == 0) {
		speed = 2.0f;
	}
	self->walkBehavior(2, speed);

	if ((self->mIsChaseMode != 0 && self->isReachedToGoalXZ())
	    || (self->mFlyState == 0
	        && self->mPosition.y < 100.0f + self->mGroundHeight))
		self->calcChaseParam();

	switch (self->mFlyState) {
	case 2: {
		JGeometry::TVec3<f32> goal = self->getVelocity();
		goal.scale(0.9f);
		self->mVelocity = goal;

		JGeometry::TVec3<f32> diff = self->unkF4.getPoint();
		diff.sub(self->mPosition);
		PSVECMag((Vec*)&diff);

		f32 targetYaw = MsAngleWrap(MsGetRotFromZaxisY(diff));

		f32 delta = targetYaw
		          - MsWrap(self->mRotation.y, targetYaw - 180.0f,
		                   targetYaw + 180.0f);
		if (delta > 0.0f) {
			if (delta > self->mTurnSpeed)
				delta = self->mTurnSpeed;
		} else {
			if (delta < -self->mTurnSpeed)
				delta = -self->mTurnSpeed;
		}
		self->mRotation.y = MsAngleWrap(self->mRotation.y + delta);

		JGeometry::TVec3<f32> vel = self->mLinearVelocity;
		f32 yaw = self->mRotation.y;
		f32 sp  = self->mMarchSpeed;
		f32 cz  = sp * MsCos(yaw);
		JGeometry::TVec3<f32> move
		    = makeKillerMove(sp * MsSin(yaw), 0.0f, cz);
		vel.add(move);
		self->mLinearVelocity = vel;
		break;
	}
	case 0:
	case 1:
		self->walkBehavior(3, 1.0f);
		break;
	default:
		break;
	}

	if (self->mFlyState != 2
	    && self->mPosition.y > 200.0f + self->mGroundHeight) {
		f32 sp = self->mMarchSpeed;
		f32 g  = self->getGravityY();
		JGeometry::TVec3<f32> a;
		a.x = self->mMarchSpeed;
		a.y = -40.0f * g;
		a.z = sp;
		self->mRotation.x = MsGetRotFromZaxis(a).x;

		JGeometry::TVec3<f32> b = self->mLinearVelocity;
		b.y = self->getGravityY();
		self->mRotation.x = MsGetRotFromZaxis(b).x;
	} else {
		self->mRotation.x *= 0.99f;
	}

	self->flyBehavior();

	f32 sc = MsClamp(1.1f * self->mScaling.x, 0.0f, self->mBodyScale);
	self->mScaling.z = sc;
	self->mScaling.y = sc;
	self->mScaling.x = sc;

	return FALSE;
}

DEFINE_NERVE(TNerveFlyEnemyNormalFly, TLiveActor)
{
	TFlyEnemy* self = (TFlyEnemy*)spine->getBody();

	if (spine->getTime() == 0)
		self->setNormalFlyAnm();

	if (self->mChaseFinished != 0 && self->mFlyTimer > 500) {
		self->updateSquareToMario();
		f32 chaseDist = self->mFlyParams->mSLChaseDist.get();
		if (self->mDistToMarioSquared < chaseDist * chaseDist) {
			spine->pushAfterCurrent(&TNerveFlyEnemyChaseFly::theNerve());
			return TRUE;
		}
	} else if (self->mColorVariant == 0 && self->isFindMario(1.0f)
	           && self->mFlyTimer > 100) {
		self->updateSquareToMario();
		f32 chaseDist = self->mFlyParams->mSLChaseDist.get();
		if (self->mDistToMarioSquared < chaseDist * chaseDist) {
			spine->pushAfterCurrent(&TNerveFlyEnemyChaseFly::theNerve());
			return TRUE;
		}
	}

	JGeometry::TVec3<f32> vel = self->mVelocity;
	self->mRotation.x = MsGetRotFromZaxis(vel).x;

	f32 sc = MsClamp(1.05f * self->mScaling.x, 0.0f, self->mBodyScale);
	self->mScaling.z = sc;
	self->mScaling.y = sc;
	self->mScaling.x = sc;

	return FALSE;
}

DEFINE_NERVE(TNerveKillerExplosion, TLiveActor)
{
	TKiller* self = (TKiller*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mExplosionTimer = ((TKillerParams*)self->getSaveParam())
		                            ->mSLBombRange.get()
		                        * self->getBodyScale() / self->getAttackRadius();
		self->mRotation.x = 0.0f;
		self->setDeadAnm();

		if (!self->isAirborne()) {
			if (self->getGroundPlane()->isWaterSurface()) {
				if (TEffectBombColumWater* water
				    = (TEffectBombColumWater*)gpConductor->makeOneEnemyAppear(
				        self->mPosition, "エフェクト爆発水柱マネージャー", 1)) {
					JGeometry::TVec3<f32> scale(2.0f, 2.0f, 2.0f);
					water->generate(self->mPosition, scale);
				}
			}

			if (self->getGroundPlane()->isSand()) {
				if (TEffectColumSand* sand
				    = (TEffectColumSand*)gpConductor->makeOneEnemyAppear(
				        self->mPosition, "エフェクト砂柱マネージャー", 1)) {
					JGeometry::TVec3<f32> scale(0.6f, 0.9f, 0.6f);
					sand->generate(self->mPosition, scale);
				}
			}
		}

		SMSRumbleMgr->start(0x13, &self->mPosition);
	}

	if (self->unk190 < self->mExplosionTimer) {
		self->unk190 *= 1.3f;
	} else {
		self->onHitFlag(HIT_FLAG_NO_COLLISION);
		if (self->checkCurAnmEnd(0)) {
			self->onLiveFlag(LIVE_FLAG_DEAD);
			self->onLiveFlag(LIVE_FLAG_UNK8);
			self->offLiveFlag(LIVE_FLAG_UNK10000);
			self->mHolder = nullptr;
			self->stopAnmSound();
			spine->reset();
			spine->setNext(&TNerveSmallEnemyDie::theNerve());
			spine->pushAfterCurrent(spine->getDefault());

			self->mPosition.y -= 200.0f;
			return true;
		}
	}

	self->expandCollision();
	return false;
}
