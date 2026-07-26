#include <Animal/Bird.hpp>
#include <Animal/AnimalSave.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <Player/MarioAccess.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <Enemy/WireBinder.hpp>
#include <Enemy/PathNode.hpp>
#include <Enemy/Graph.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <M3DUtil/MActor.hpp>
#include <Map/Map.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/Item.hpp>
#include <System/Particles.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <stdlib.h>
#include <math.h>

extern JGeometry::TVec3<f32>* gpMarioPos;

JGeometry::TQuat4<f32> SMS_Eular2Quat(const JGeometry::TVec3<f32>&);
f32 SMSGetAnmFrameRate();

static inline f32 callMsWrap(f32 t, f32 l, f32 r)
{
	return MsWrap<f32>(t, l, r);
}

namespace {
const int cRandomAnims[5] = { 7, 4, 0, 2, 8 };

const GXColorS10 cColorTable[4] = {
	{ 0, 0x64, (s16)0xFF, 0 },
	{ 0, (s16)0xC8, 0, 0 },
	{ (s16)0xFF, (s16)0xC8, 0, 0 },
	{ (s16)0xFF, 0, 0, 0 },
};

const char* cMatName = "_mat_body1";
}

// ---- Nerves ----

inline const TNerveAnimalBirdLanding& TNerveAnimalBirdLanding::theNerve()
{
	static TNerveAnimalBirdLanding instance;
	return instance;
}

inline const TNerveAnimalBirdGraphWander& TNerveAnimalBirdGraphWander::theNerve()
{
	static TNerveAnimalBirdGraphWander instance;
	return instance;
}

inline const TNerveAnimalBirdTakeoff& TNerveAnimalBirdTakeoff::theNerve()
{
	static TNerveAnimalBirdTakeoff instance;
	return instance;
}

BOOL TNerveAnimalBirdLanding::execute(TSpineBase<TLiveActor>* spine) const
{
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();
	J3DFrameCtrl* fc  = bird->mMActor->getFrameCtrl(0);

	if (spine->getTime() == 0) {
		JGeometry::TVec3<f32> zero(0.0f, 0.0f, 0.0f);
		bird->mVelocity = zero;
		bird->mMActor->setBckFromIndex(5);
		bird->setCurAnmSound();
		fc->setAttribute(1);
		fc->setFrame((f32)fc->getEnd());
		fc->setRate(-1.0f * fc->getRate());
	}

	BOOL inSight;
	f32 diffY = __fabsf(gpMarioPos->y - bird->mPosition.y);
	if (((TAnimalBirdParams*)bird->getSaveParam())->mSearchHeight.value
	    < diffY) {
		inSight = 0;
	} else {
		f32* aware  = &((TAnimalBirdParams*)bird->getSaveParam())
		                   ->mSearchAware.value;
		f32* angle  = &((TAnimalBirdParams*)bird->getSaveParam())
		                   ->mSearchAngle.value;
		f32* length = &((TAnimalBirdParams*)bird->getSaveParam())
		                   ->mSearchLength.value;
		f32 scale   = bird->unk174;
		inSight     = bird->isInSight(*gpMarioPos, scale * *length,
		                               scale * *angle, scale * *aware)
		          != 0;
	}

	if (inSight) {
		spine->pushAfterCurrent(&TNerveAnimalBirdGraphWander::theNerve());
		return TRUE;
	}

	if (fc->checkState(1)) {
		spine->pushAfterCurrent(&TNerveAnimalBirdWaitOnGround::theNerve());
		return TRUE;
	}
	return FALSE;
}

BOOL TNerveAnimalBirdPreLanding::execute(TSpineBase<TLiveActor>* spine) const
{
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();

	if (spine->getTime() == 0) {
		bird->mMActor->setBckFromIndex(1);
		bird->setCurAnmSound();
		J3DFrameCtrl* fc = bird->mMActor->getFrameCtrl(0);
		fc->setRate(fc->getRate() * 1.5f);
		bird->doLanding(true);
	}

	BOOL inSight;
	f32 diffY = __fabsf(gpMarioPos->y - bird->mPosition.y);
	if (((TAnimalBirdParams*)bird->getSaveParam())->mSearchHeight.value
	    < diffY) {
		inSight = FALSE;
	} else {
		f32* aware  = &((TAnimalBirdParams*)bird->getSaveParam())
		                   ->mSearchAware.value;
		f32* angle  = &((TAnimalBirdParams*)bird->getSaveParam())
		                   ->mSearchAngle.value;
		f32* length = &((TAnimalBirdParams*)bird->getSaveParam())
		                   ->mSearchLength.value;
		f32 scale   = bird->unk174;
		inSight     = bird->isInSight(*gpMarioPos, scale * *length,
		                               scale * *angle, scale * *aware)
		          != 0;
	}
	if (inSight) {
		spine->pushAfterCurrent(&TNerveAnimalBirdGraphWander::theNerve());
		return TRUE;
	}

	if (bird->doLanding(false)) {
		spine->pushAfterCurrent(&TNerveAnimalBirdLanding::theNerve());
		return TRUE;
	}
	return FALSE;
}

const TNerveAnimalBirdPreLanding& TNerveAnimalBirdPreLanding::theNerve()
{
	static TNerveAnimalBirdPreLanding instance;
	return instance;
}

BOOL TNerveAnimalBirdComeback::execute(TSpineBase<TLiveActor>* spine) const
{
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();

	if (spine->getTime() == 0) {
		TPathNode node(bird->unk158);
		bird->unkF4  = node;
		bird->unk104 = node;
		bird->unk114.clear();
		bird->mMActor->setBckFromIndex(3);
		bird->setCurAnmSound();
	}

	bird->doFlyToCurPathNode();

	BOOL inSight;
	f32 diffY = __fabsf(gpMarioPos->y - bird->mPosition.y);
	if (((TAnimalBirdParams*)bird->getSaveParam())->mSearchHeight.value
	    < diffY) {
		inSight = FALSE;
	} else {
		f32* aware  = &((TAnimalBirdParams*)bird->getSaveParam())
		                   ->mSearchAware.value;
		f32* angle  = &((TAnimalBirdParams*)bird->getSaveParam())
		                   ->mSearchAngle.value;
		f32* length = &((TAnimalBirdParams*)bird->getSaveParam())
		                   ->mSearchLength.value;
		f32 scale   = bird->unk174;
		inSight     = bird->isInSight(*gpMarioPos, scale * *length,
		                               scale * *angle, scale * *aware)
		          != 0;
	}
	if (inSight) {
		spine->pushAfterCurrent(&TNerveAnimalBirdGraphWander::theNerve());
		return TRUE;
	}

	if (bird->isReachedToGoal()) {
		spine->pushAfterCurrent(&TNerveAnimalBirdPreLanding::theNerve());
		return TRUE;
	}
	return FALSE;
}

const TNerveAnimalBirdComeback& TNerveAnimalBirdComeback::theNerve()
{
	static TNerveAnimalBirdComeback instance;
	return instance;
}

BOOL TNerveAnimalBirdChangeToCoin::execute(TSpineBase<TLiveActor>* spine) const
{
	TMapObjBase* spawned;
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();
	if (spine->getTime() == 0) {
		bird->mLiveFlag |= 1;

		u32 actorType = bird->unk150->mActorType;

		bool isShine;
		if (actorType == 0x20000013)
			isShine = true;
		else
			isShine = false;
		if (isShine) {
			((void (*)(TMapObjBase*, const Vec*))(*(u32**)bird->unk150)[35])(
			    bird->unk150, (const Vec*)&bird->mPosition);
			TMapObjBase* item = bird->unk150;
			((TShine*)item)->appearWithDemo("鳥シャインカメラ");
		} else {
			bool isCoin;
			if (actorType == 0x2000000E)
				isCoin = true;
			else
				isCoin = false;
			if (isCoin) {
				spawned = gpItemManager->makeObjAppear(0x2000000E);
			} else {
				spawned = bird->unk150;
			}

			if (spawned != NULL) {
				((void (*)(TMapObjBase*))(*(u32**)spawned)[63])(
				    spawned);
				((void (*)(TMapObjBase*, const Vec*))(*(u32**)spawned)[35])(
				    spawned, (const Vec*)&bird->mPosition);

				((TLiveActor*)spawned)->mVelocity.x = 0.0f;
				((TLiveActor*)spawned)->mVelocity.y = -10.0f;
				((TLiveActor*)spawned)->mVelocity.z = 0.0f;

				((TLiveActor*)spawned)->mLiveFlag &= ~0x10;
				((TLiveActor*)spawned)->mLiveFlag |= 0x80;
			}
		}
	}
	return TRUE;
}

const TNerveAnimalBirdChangeToCoin& TNerveAnimalBirdChangeToCoin::theNerve()
{
	static TNerveAnimalBirdChangeToCoin instance;
	return instance;
}

BOOL TNerveAnimalBirdGraphWander::execute(TSpineBase<TLiveActor>* spine) const
{
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();

	if (spine->getTime() == 0) {
		JGeometry::TVec3<f32> zero(0.0f, 0.0f, 0.0f);
		bird->mVelocity = zero;
		bird->getTracer()->reset();
		bird->goToShortestNextGraphNode();
	}

	if (spine->getTime() == 0 || bird->isReachedToGoal()) {
		bird->goToRandomNextGraphNode();

		JGeometry::TVec3<f32> pt = bird->unk104.getPoint();
		pt.x += 200.0f * (MsRandF() - 0.5f);
		pt.y += 200.0f * (MsRandF() - 0.5f);
		pt.z += 200.0f * (MsRandF() - 0.5f);

		TPathNode node(pt);
		bird->unkF4  = node;
		bird->unk104 = node;
		bird->unk114.clear();

		if (bird->mPosition.y <= bird->unkF4.getPoint().y) {
			bird->mMActor->setBckFromIndex(1);
			bird->setCurAnmSound();
		} else {
			bird->mMActor->setBckFromIndex(3);
			bird->setCurAnmSound();
		}
	}

	bird->checkCurAnmEnd(0);
	if (((TAnimalBirdParams*)bird->getSaveParam())->mReturnTimer.value
	    < (s32)spine->getTime()) {
		spine->pushAfterCurrent(&TNerveAnimalBirdComeback::theNerve());
		return TRUE;
	}

	bird->doFlyToCurPathNode();
	return FALSE;
}

BOOL TNerveAnimalBirdTakeoff::execute(TSpineBase<TLiveActor>* spine) const
{
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();

	if (spine->getTime() == 0) {
		bird->mMActor->setBckFromIndex(5);
		bird->setCurAnmSound();
		bird->mLiveFlag |= 0x80;
		bird->mGravity   = 0.0f;
		bird->unk17C     = 0;
		J3DFrameCtrl* fc = bird->mMActor->getFrameCtrl(0);
		fc->setRate(fc->getRate() * 3.0f);
		if (gpMSound->gateCheck(0x386b)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x386b, (const Vec*)&bird->mPosition, 0,
			    (JAISound**)NULL, 0, 4);
		}
	}

	if (bird->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveAnimalBirdGraphWander::theNerve());
		bird->mLiveFlag |= 0x80;
		bird->mGravity  = 0.0f;
		bird->unk17C    = 0;
		return TRUE;
	}
	return FALSE;
}

BOOL TNerveAnimalBirdWalkOnGround::execute(TSpineBase<TLiveActor>* spine) const
{
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();

	if (spine->getTime() == 0) {
		bird->unk170 = bird->unk170 * -1.0f;
		bird->mMActor->setBckFromIndex(8);
		bird->setCurAnmSound();
	}

	bool doTakeoff = false;
	bool wantTakeoff = true;
	if (bird->unk178 <= 0) {
		if (!(u8)bird->isFindMario())
			wantTakeoff = false;
	}
	if (wantTakeoff) {
		if (MsRandF() < 0.5f)
			doTakeoff = true;
	}
	if (doTakeoff) {
		spine->pushAfterCurrent(&TNerveAnimalBirdTakeoff::theNerve());
		return TRUE;
	}

	bird->mGravity = 0.15f;
	TAnimalBirdParams* p = (TAnimalBirdParams*)bird->getSaveParam();
	f32 turn              = p->mWalkingTorqueY.value;
	turn *= SMSGetAnmFrameRate();
	bird->mRotation.y = callMsWrap(
	    bird->mRotation.y + bird->unk170 * turn,
	    0.0f, 360.0f);

	JGeometry::TQuat4<f32> q = SMS_Eular2Quat(bird->mRotation);

	f32 speed = ((TAnimalBirdParams*)bird->getSaveParam())
	                ->mWalkingSpeed.value;

	JGeometry::TVec3<f32> forward(0.0f, 0.0f, speed);
	q.rotate(forward, forward);
	bird->mLinearVelocity = forward;

	if (((TAnimalBirdParams*)bird->getSaveParam())->mWalkTimer.value
	    < (s32)spine->getTime()) {
		spine->pushAfterCurrent(&TNerveAnimalBirdWaitOnGround::theNerve());
		return TRUE;
	}
	return FALSE;
}

const TNerveAnimalBirdWalkOnGround& TNerveAnimalBirdWalkOnGround::theNerve()
{
	static TNerveAnimalBirdWalkOnGround instance;
	return instance;
}

BOOL TNerveAnimalBirdActionOnGround::execute(TSpineBase<TLiveActor>* spine) const
{
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();

	if (spine->getTime() == 0) {
		int randIdx = (int)(MsRandF() * 5.0f);
		int animIdx = cRandomAnims[randIdx];
		if (animIdx == 8) {
			spine->pushAfterCurrent(
			    &TNerveAnimalBirdWalkOnGround::theNerve());
			return TRUE;
		}
		MActor* actor = bird->mMActor;
		if (!actor->checkCurBckFromIndex(animIdx)) {
			actor->setBckFromIndex(animIdx);
		}
	}

	bool doTakeoff = false;
	bool wantTakeoff = true;
	if (bird->unk178 <= 0) {
		if (!(u8)bird->isFindMario())
			wantTakeoff = false;
	}
	if (wantTakeoff) {
		if (MsRandF() < 0.5f)
			doTakeoff = true;
	}
	if (doTakeoff) {
		spine->pushAfterCurrent(&TNerveAnimalBirdTakeoff::theNerve());
		return TRUE;
	}

	if (bird->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveAnimalBirdWaitOnGround::theNerve());
		return TRUE;
	}
	return FALSE;
}

const TNerveAnimalBirdActionOnGround& TNerveAnimalBirdActionOnGround::theNerve()
{
	static TNerveAnimalBirdActionOnGround instance;
	return instance;
}

BOOL TNerveAnimalBirdWaitOnGround::execute(TSpineBase<TLiveActor>* spine) const
{
	bool doTakeoff;
	bool wantTakeoff;
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();

	if (spine->getTime() == 0) {
		bird->mMActor->setBckFromIndex(7);
		bird->setCurAnmSound();
	}

	doTakeoff   = false;
	wantTakeoff = true;
	if (bird->unk178 <= 0) {
		if (!(u8)bird->isFindMario())
			wantTakeoff = false;
	}
	if (wantTakeoff) {
		if (MsRandF() < 0.5f)
			doTakeoff = true;
	}
	if (doTakeoff) {
		spine->pushAfterCurrent(&TNerveAnimalBirdTakeoff::theNerve());
		return TRUE;
	}

	if (bird->checkCurAnmEnd(0)) {
		TAnimalBirdParams* p = (TAnimalBirdParams*)bird->getSaveParam();
		s32 diff = bird->mSpine->getTime() - p->mActionTimer.value;
		bool doAction;
		if (diff < 0) {
			doAction = false;
		} else {
			p         = (TAnimalBirdParams*)bird->getSaveParam();
			f32 ratio = (f32)diff / (f32)p->mActionTimerAdd.value;
			doAction  = MsRandF() < ratio;
		}
		if (doAction) {
			spine->pushAfterCurrent(
			    &TNerveAnimalBirdActionOnGround::theNerve());
			return TRUE;
		}
	}
	return FALSE;
}

const TNerveAnimalBirdWaitOnGround& TNerveAnimalBirdWaitOnGround::theNerve()
{
	static TNerveAnimalBirdWaitOnGround instance;
	return instance;
}

// ---- TAnimalBirdManager ----

void TAnimalBirdManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "bird_man.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TAnimalBirdManager::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	MSoundSESystem::MSRandPlay::createRandPlayVec(0x3869, (u16)mObjNum);
	MSoundSESystem::MSRandPlay::createRandPlayVec(0x3870, (u16)mObjNum);
}

void TAnimalBirdManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TAnimalBirdParams("/Animal/bird.prm");
	TEnemyManager::load(stream);
}

TAnimalBirdManager::TAnimalBirdManager(const char* name)
    : TEnemyManager(name)
{
}

// ---- TAnimalBirdParams ----

TAnimalBirdParams::TAnimalBirdParams(const char* prm)
    : TSpineEnemyParams(prm)
    , PARAM_INIT(mMarchSpeed, 5.0f)
    , PARAM_INIT(mTurnSpeed, 0.1f)
    , PARAM_INIT(mReturnTimer, 1800)
    , PARAM_INIT(mSearchLength, 800.0f)
    , PARAM_INIT(mSearchHeight, 600.0f)
    , PARAM_INIT(mSearchAware, 400.0f)
    , PARAM_INIT(mSearchAngle, 90.0f)
    , PARAM_INIT(mActionTimer, 100)
    , PARAM_INIT(mWaterproofTimerMax, 45)
    , PARAM_INIT(mFloatingTimerMax, 30)
    , PARAM_INIT(mLandingGravityY, 1.0f)
    , PARAM_INIT(mLandingTorqueY, 2.0f)
    , PARAM_INIT(mWalkingTorqueY, 2.0f)
    , PARAM_INIT(mWalkingSpeed, 0.5f)
    , PARAM_INIT(mWalkTimer, 100)
    , PARAM_INIT(mLandingFric, 0.95f)
    , PARAM_INIT(mActionTimerAdd, 300)
    , PARAM_INIT(mWaterPowerY, 15.0f)
{
	load(mPrmPath);
}

// ---- TAnimalBird ----

bool TAnimalBird::doLanding(bool initFrame)
{
	if (initFrame) {
		TAnimalBirdParams* p = (TAnimalBirdParams*)getSaveParam();
		f32 speed             = unk174 * p->mMarchSpeed.value;
		speed *= SMSGetAnmFrameRate();

		JGeometry::TQuat4<f32> q = SMS_Eular2Quat(mRotation);

		JGeometry::TVec3<f32> forward(0.0f, 0.0f, speed);
		JGeometry::TVec3<f32> velocity;
		q.rotate(forward, velocity);
		velocity.y = 0.0f;
		mVelocity  = velocity;
	}

	BOOL grounded = FALSE;
	JGeometry::TVec3<f32> deltaV(0.0f, 0.0f, 0.0f);
	if (mBinder2 != NULL) {
		((TWireBinder*)mBinder2)->getPoint(&deltaV, unk158);
	} else {
		gpMap->checkGround(mPosition, &mGroundPlane);
	}

	if (mLiveFlag & 0x80) {
		deltaV.y = -((TAnimalBirdParams*)getSaveParam())->mLandingGravityY.value;
	} else {
		grounded = TRUE;
	}

	mRotation.x = unk164.x;
	mRotation.z = unk164.z;

	TAnimalBirdParams* p = (TAnimalBirdParams*)getSaveParam();
	f32 torque            = p->mLandingTorqueY.value;
	torque *= SMSGetAnmFrameRate();

	f32 savedY  = unk164.y;
	f32 wrappedY
	    = callMsWrap(mRotation.y, savedY - 180.0f, savedY + 180.0f);
	f32 delta = savedY - wrappedY;
	f32 clamped;
	if (delta < -torque)
		clamped = -torque;
	else if (delta > torque)
		clamped = torque;
	else
		clamped = delta;

	mRotation.y     = MsWrap<f32>(wrappedY + clamped, 0.0f, 360.0f);
	mLinearVelocity = deltaV;

	JGeometry::TVec3<f32> velCopy = mVelocity;
	f32 mag = JGeometry::TUtil<f32>::sqrt(velCopy.x * velCopy.x
	                                      + velCopy.y * velCopy.y
	                                      + velCopy.z * velCopy.z);

	JGeometry::TVec3<f32> forward(0.0f, 0.0f, mag);
	f32 fric = ((TAnimalBirdParams*)getSaveParam())->mLandingFric.value;
	forward.scale(fric);
	JGeometry::TQuat4<f32> q = SMS_Eular2Quat(mRotation);
	q.rotate(forward, forward);
	mVelocity = forward;

	bool ret = false;
	if (grounded) {
		if (__fabsf(clamped) < 0.01f)
			ret = true;
	}
	return ret;
}

void TAnimalBird::doFlyToCurPathNode()
{
	JGeometry::TVec3<f32> toTarget = unkF4.getPoint();
	toTarget.x -= mPosition.x;
	toTarget.y -= mPosition.y;
	toTarget.z -= mPosition.z;

	f32 dist2 = toTarget.x * toTarget.x + toTarget.y * toTarget.y
	          + toTarget.z * toTarget.z;
	f32 dist = JGeometry::TUtil<f32>::sqrt(dist2);

	if (dist < 100.0f)
		return;

	TAnimalBirdParams* p = (TAnimalBirdParams*)getSaveParam();
	f32 speed             = unk174 * p->mMarchSpeed.value;
	speed *= SMSGetAnmFrameRate();

	p        = (TAnimalBirdParams*)getSaveParam();
	f32 turn = p->mTurnSpeed.value;
	turn *= SMSGetAnmFrameRate();

	f32 turnRadius = calcMinimumTurnRadius(speed, turn);
	if (dist <= 2.0f * turnRadius) {
		turn = calcTurnSpeedToReach(speed, 0.5f * dist);
	}

	TAnimalBase::getRotationFlyToDir(&mRotation, toTarget, speed, turn);

	JGeometry::TQuat4<f32> q = SMS_Eular2Quat(mRotation);

	JGeometry::TVec3<f32> forward(0.0f, 0.0f, speed);
	q.rotate(forward, forward);

	f32 wetRatio
	    = 1.0f
	    - (f32)unk178
	          / (f32)((TAnimalBirdParams*)getSaveParam())
	                ->mWaterproofTimerMax.value;
	forward.x *= wetRatio;
	forward.y *= wetRatio;
	forward.z *= wetRatio;

	f32 fallRatio
	    = (f32)unk178
	    / (f32)((TAnimalBirdParams*)getSaveParam())->mWaterproofTimerMax.value;
	forward.y -= ((TAnimalBirdParams*)getSaveParam())->mWaterPowerY.value
	           * fallRatio;
	mLinearVelocity = forward;
}

BOOL TAnimalBird::isFindMario() const
{
	f32 diffY = __fabsf(gpMarioPos->y - mPosition.y);
	if (((TAnimalBirdParams*)getSaveParam())->mSearchHeight.value < diffY)
		return FALSE;

	f32* aware  = &((TAnimalBirdParams*)getSaveParam())->mSearchAware.value;
	f32* angle  = &((TAnimalBirdParams*)getSaveParam())->mSearchAngle.value;
	f32* length = &((TAnimalBirdParams*)getSaveParam())->mSearchLength.value;
	f32 scale   = unk174;
	return isInSight(*gpMarioPos, scale * *length, scale * *angle,
	                 scale * *aware)
	    != 0;
}

const char** TAnimalBird::getBasNameTable() const
{
	static const char* bastable[] = {
		"/scene/bird/bas/bird_fly.bas",
		"/scene/bird/bas/bird_open.bas",
		"/scene/bird/bas/bird_start.bas",
		"/scene/bird/bas/bird_stop.bas",
		nullptr,
	};
	return bastable;
}

void TAnimalBird::bind()
{
	BOOL useWire = FALSE;
	if (mBinder2 != NULL) {
		BOOL cond1                 = TRUE;
		BOOL cond2                 = TRUE;
		BOOL cond3                 = TRUE;
		TSpineBase<TLiveActor>* sp = mSpine;
		if (&TNerveAnimalBirdWaitOnGround::theNerve()
		    != sp->getLatestNerve()) {
			if (&TNerveAnimalBirdActionOnGround::theNerve()
			    != sp->getLatestNerve())
				cond3 = FALSE;
		}
		if (!(u8)cond3) {
			if (&TNerveAnimalBirdWalkOnGround::theNerve()
			    != sp->getLatestNerve())
				cond2 = FALSE;
		}
		if (!(u8)cond2) {
			if (&TNerveAnimalBirdPreLanding::theNerve()
			    != mSpine->getLatestNerve())
				cond1 = FALSE;
		}
		if ((u8)cond1)
			useWire = TRUE;
	}

	if (!(u8)useWire) {
		TLiveActor::bind();
	} else {
		mBinder2->bind(this);
	}
}

void TAnimalBird::moveObject()
{
	if (unk178 > 0)
		unk178--;

	const TNerveBase<TLiveActor>* cur = mSpine->getLatestNerve();

	bool inGroundState = (cur == &TNerveAnimalBirdWaitOnGround::theNerve()
	                      || cur == &TNerveAnimalBirdActionOnGround::theNerve()
	                      || cur == &TNerveAnimalBirdWalkOnGround::theNerve());

	if (inGroundState && (mLiveFlag & 0x80)) {
		TAnimalBirdParams* p = (TAnimalBirdParams*)getSaveParam();
		unk17C++;
		if (p->mFloatingTimerMax.value < unk17C) {
			mSpine->reset();
			mSpine->setNext(&TNerveAnimalBirdTakeoff::theNerve());
		}
	} else if (inGroundState) {
		unk17C = 0;
	}

	if (mSpine->getLatestNerve()
	        != &TNerveAnimalBirdChangeToCoin::theNerve()
	    && *(u8*)((char*)this + 0x13C) == 0) {
		mSpine->reset();
		mSpine->setNext(&TNerveAnimalBirdChangeToCoin::theNerve());
	}

	const TNerveBase<TLiveActor>* cur2 = mSpine->getLatestNerve();
	if (cur2 == &TNerveAnimalBirdGraphWander::theNerve()
	    || cur2 == &TNerveAnimalBirdComeback::theNerve()) {
		if (gpMSound->gateCheck(0x3869)) {
			MSoundSESystem::MSRandPlay::startSeRandPlay(
			    0x3869, (u32)(s16)mInstanceIndex);
		}
	}

	const TNerveBase<TLiveActor>* cur3 = mSpine->getLatestNerve();
	if (cur3 == &TNerveAnimalBirdWaitOnGround::theNerve()
	    || cur3 == &TNerveAnimalBirdActionOnGround::theNerve()
	    || cur3 == &TNerveAnimalBirdWalkOnGround::theNerve()) {
		if (gpMSound->gateCheck(0x3870)) {
			MSoundSESystem::MSRandPlay::startSeRandPlay(
			    0x3870, (u32)(s16)mInstanceIndex);
		}
	}

	TLiveActor::moveObject();
}

class TBirdMount {
public:
	virtual void dummy0()  = 0;
	virtual void dummy1()  = 0;
	virtual void dummy2()  = 0;
	virtual void dummy3()  = 0;
	virtual void dummy4()  = 0;
	virtual void dummy5()  = 0;
	virtual void dummy6()  = 0;
	virtual void dummy7()  = 0;
	virtual void dummy8()  = 0;
	virtual void dummy9()  = 0;
	virtual void dummy10() = 0;
	virtual void dummy11() = 0;
	virtual void dummy12() = 0;
	virtual void dummy13() = 0;
	virtual void dummy14() = 0;
	virtual void dummy15() = 0;
	virtual void dummy16() = 0;
	virtual void dummy17() = 0;
	virtual void dummy18() = 0;
	virtual void dummy19() = 0;
	virtual void dummy20() = 0;
	virtual void dummy21() = 0;
	virtual void dummy22() = 0;
	virtual void dummy23() = 0;
	virtual void dummy24() = 0;
	virtual void dummy25() = 0;
	virtual void dummy26() = 0;
	virtual void dummy27() = 0;
	virtual void dummy28() = 0;
	virtual void dummy29() = 0;
	virtual void dummy30() = 0;
	virtual void dummy31() = 0;
	virtual void dummy32() = 0;
	virtual void dummy33() = 0;
	virtual void dummy34() = 0;
	virtual void dummy35() = 0;
	virtual void dummy36() = 0;
	virtual void dummy37() = 0;
	virtual void dummy38() = 0;
	virtual MtxPtr getRiderMtx() = 0;
};

void TAnimalBird::calcRootMatrix()
{
	TBirdMount* mount;
	if ((mount = (TBirdMount*)mHolder) != NULL) {
		MtxPtr m = mount->getRiderMtx();
		PSMTXCopy(m, getModel()->unk20);
	} else {
		TSpineEnemy::calcRootMatrix();
	}
	getModel()->unk20[1][3] += 35.0f;
}

BOOL TAnimalBird::receiveMessage(THitActor* sender, u32 msg)
{
	if (mLiveFlag & 1)
		return FALSE;

	if (msg == 0xF) {
		JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
		SMS_EasyEmitParticle((E_SMS_EFFECT_ONETIME_NORMAL)0xE7,
		                     &sender->mPosition, (const void*)NULL, scale);
		gpMSound->startSoundSet(0x6802, &sender->mPosition, 0, 0.0f, 0, 0, 4);

		if (unk178 <= 0) {
			TAnimalBirdParams* p = (TAnimalBirdParams*)getSaveParam();
			unk178               = p->mWaterproofTimerMax.value;
			if (mLiveFlag & 0x80 && mHitPoints > 0) {
				mHitPoints--;
			}
		}
		return TRUE;
	}

	if (msg == 4 && mHolder == NULL) {
		unk64 |= 1;
		mHolder = (TTakeActor*)sender;
		JGeometry::TVec3<f32> scale2(1.0f, 1.0f, 1.0f);
		SMS_EasyEmitParticle((E_SMS_EFFECT_ONETIME_NORMAL)0xE7,
		                     &sender->mPosition, (const void*)NULL, scale2);
		return TRUE;
	}

	if ((msg == 6 || msg == 7) && mHolder == (TTakeActor*)sender) {
		mHolder = (TTakeActor*)NULL;
		unk64 &= ~1;
		return TRUE;
	}

	if (msg == 0xB) {
		mHolder = (TTakeActor*)NULL;
		if (mSpine->getLatestNerve()
		    != &TNerveAnimalBirdChangeToCoin::theNerve()) {
			mSpine->reset();
			mSpine->setNext(&TNerveAnimalBirdChangeToCoin::theNerve());
		} else {
			kill();
		}
		return TRUE;
	}

	if (msg == 0) {
		u32 t = sender->mActorType - 0x10000000;
		bool isMario;
		if (t == 0xD)
			isMario = true;
		else
			isMario = false;
		if (isMario) {
			if (mSpine->getLatestNerve()
			    != &TNerveAnimalBirdChangeToCoin::theNerve()) {
				mSpine->reset();
				mSpine->setNext(&TNerveAnimalBirdChangeToCoin::theNerve());
			} else {
				receiveMessage(this, 0xF);
			}
			return TRUE;
		}
	}

	return TSpineEnemy::receiveMessage(sender, msg);
}

void TAnimalBird::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	MSoundSESystem::MSRandPlay::registerTrans(0x3869, &mPosition);
	MSoundSESystem::MSRandPlay::registerTrans(0x3870, &mPosition);
}

void TAnimalBird::load(JSUMemoryInputStream& stream)
{
	TSpineEnemy::load(stream);

	s32 itemId;
	stream.read(&itemId, 4);

	if (itemId >= 0) {
		unk150
		    = TMapObjBaseManager::newAndRegisterObjByEventID((u32)itemId, "鳥用");
	} else {
		unk150 = TMapObjBaseManager::newAndRegisterObjByEventID(0x64, "");
	}

	TMapObjBase* item = unk150;
	s32 actorType = item->mActorType;
	if (actorType != 0x20000010) {
		if (actorType >= 0x20000010) {
			if (actorType == 0x20000013) {
				*(int*)((char*)this + 0x180) = 2;
			} else {
				*(int*)((char*)this + 0x180) = 1;
			}
		} else if (actorType >= 0x2000000F) {
			*(int*)((char*)this + 0x180) = 3;
		} else {
			*(int*)((char*)this + 0x180) = 1;
		}
	} else {
		*(int*)((char*)this + 0x180) = 0;
		bool flag = TFlagManager::smInstance->getBlueCoinFlag(
		    gpMarDirector->mMap, (u8)itemId);
		if (flag) {
			mLiveFlag |= 1;
		}
	}

	const GXColorS10* color
	    = &cColorTable[*(int*)((char*)this + 0x180)];
	J3DModel* model = getModel();
	u16 matIdx
	    = (u16)model->getModelData()->getMaterialName()->getIndex(cMatName);
	SMS_InitPacket_OneTevColor(model, matIdx, GX_TEVREG1, color);
}

void TAnimalBird::init(TLiveManager* mgr)
{
	mManager = mgr;
	mManager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(mManager, (u16)1);
	mMActor       = mMActorKeeper->createMActor("bird_man.bmd", 0);

	mSpine->initWith(&TNerveAnimalBirdWaitOnGround::theNerve());

	initParams();
	initHitActor(0x10000032, 0, 0, 50.0f, 50.0f, 70.0f, 80.0f);

	onHitFlag(2);
	offHitFlag(1);

	mScaledBodyRadius = 35.0f;

	initAnmSound();
}

void TAnimalBird::initParams()
{
	unk158.set(mPosition);
	unk158.y += 90.0f;
	unk164.set(mRotation);

	TSpineEnemyParams* p = getSaveParam();
	u8 v;
	if (p != NULL) {
		v = (u8)getSaveParam()->mSLHitPointMax.get();
	} else {
		v = 1;
	}
	mHitPoints = v;

	unk178 = 0;
	unk17C = 0;
	unk170 = 1.0f;

	mLiveFlag &= ~0x80;

	unk174 = 1.0f - 0.1f * (MsRandF() - 0.5f);

	if (TWireBinder::isOnWire(mPosition)) {
		TWireBinder* wb = new TWireBinder();
		mBinder2        = wb;
		((TWireBinder*)mBinder2)->init(mPosition);
	}
}

TAnimalBird::TAnimalBird(const char* name)
    : TSpineEnemy(name)
{
	unk150   = (TMapObjBase*)NULL;
	mBinder2 = (TBinder*)NULL;
}
