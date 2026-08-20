#define JGEOMETRY_TVEC3_SUB_OUT_OF_LINE
#include <MoveBG/MapObjGeneral.hpp>
#undef JGEOMETRY_TVEC3_SUB_OUT_OF_LINE
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <Player/MarioAccess.hpp>
#include <Map/MapCollisionManager.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Map/PollutionManager.hpp>
#include <Map/MapCollisionData.hpp>
#include <Map/MapData.hpp>
#include <Map/Map.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <Strategic/Binder.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JGeometry/JGUtil.hpp>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

u32 TMapObjGeneral::mNormalLivingTime       = 960;
u32 TMapObjGeneral::mNormalFlushTime        = 360;
int TMapObjGeneral::mNormalFlushInterval    = 10;
u32 TMapObjGeneral::mNormalWaitToAppearTime = 360;
f32 TMapObjGeneral::mNormalAppearingScaleUp = 0.01f;
f32 TMapObjGeneral::mNormalThrowSpeedRate   = 0.5f;

bool TMapObjGeneral::isPollutedGround(const JGeometry::TVec3<f32>& v) const
{
	// WTF? Did they forget to refactor this?!
	const JGeometry::TVec3<f32>& p = mInitialPosition;
	if (gpPollution->isPolluted(v.x, p.y, p.z)
	    || gpPollution->isPolluted(v.x - 32.0f, v.y, v.z - 32.0f)
	    || gpPollution->isPolluted(v.x + 32.0f, v.y, v.z - 32.0f)
	    || gpPollution->isPolluted(v.x - 32.0f, v.y, v.z + 32.0f)
	    || gpPollution->isPolluted(v.x + 32.0f, v.y, v.z + 32.0f))
		return true;

	return false;
}

inline f32 distToMario(const JGeometry::TVec3<f32>& v)
{
	const JGeometry::TVec3<f32>& mario = *gpMarioPos;
	f32 dx = v.x - mario.x;
	f32 dy = v.y - mario.y;
	f32 dz = v.z - mario.z;
	f32 x2 = dx * dx;
	f32 y2 = dy * dy;
	f32 z2 = dz * dz;
	return JGeometry::TUtil<f32>::sqrt(x2 + y2 + z2);
}

void TMapObjGeneral::waitingToAppear()
{
	if (mLifeTimer > 0 ? true : false)
		return;

	if (isActorType(0x4000005a)) {
		f32 r    = mDamageRadius;
		f32 dist = distToMario(mInitialPosition);
		if (dist > SMS_GetMarioDamageRadius() + r + 100.0f)
			appear();
	} else {
		f32 r    = mDamageRadius;
		f32 dist = distToMario(mInitialPosition);
		if (dist > SMS_GetMarioDamageRadius() + r)
			appear();
	}
}

void TMapObjGeneral::waitingToRecover()
{
	if (!isPollutedGround(mInitialPosition))
		recover();
}

void TMapObjGeneral::waitToAppear(s32 param_1)
{
	if (param_1 == 0)
		mLifeTimer = mNormalWaitToAppearTime;
	else
		mLifeTimer = param_1;
	mState = 10;
}

void TMapObjGeneral::sink()
{
	mVelocity.x = mVelocity.y = mVelocity.z = 0.0f;
	onLiveFlag(LIVE_FLAG_UNK10);
	mState = 7;
	mSavedY = mPosition.y;
	setUpMapCollision(1);
	startSound(6);
}

void TMapObjGeneral::put()
{
	mHolder    = nullptr;
	mHolder    = nullptr;
	int saved  = mLifeTimer;
	makeObjAppeared();
	mLifeTimer       = saved;

	f32 r = mDamageRadius;
	const f32& marioX = gpMarioPos->x;
	mPosition.x = JMASSin(*gpMarioAngleY)
	                   * (SMS_GetMarioDamageRadius() + r + 10.0f)
	               + marioX;
	mPosition.y = gpMarioPos->y;
	r           = mDamageRadius;
	const JGeometry::TVec3<f32>& marioPos = *gpMarioPos;
	mPosition.z = JMASCos(*gpMarioAngleY)
	                   * (SMS_GetMarioDamageRadius() + r + 10.0f)
	               + marioPos.z;

	offLiveFlag(LIVE_FLAG_UNK10);
	mGroundHeight = gpMap->checkGround(mPosition, &mGroundPlane);
}

void TMapObjGeneral::thrown()
{
	mPosition.set<f32>(gpMarioPos->x, gpMarioPos->y, gpMarioPos->z);

	mRotation.set<f32>((f32)*gpMarioAngleX, (f32)*gpMarioAngleY,
	                   (f32)*gpMarioAngleZ);

	mGroundHeight = gpMap->checkGround(mPosition, &mGroundPlane);

	mWallPlane  = nullptr;
	mHolder = nullptr;

	const TMapObjPhysicalData* phys = mMapObjData->mPhysical->unk4;
	s16 angleY                      = *gpMarioAngleY;
	f32 vx = JMASSin(angleY) * phys->unk2C * (*gpMarioThrowPower)
	         + mNormalThrowSpeedRate * (*gpMarioSpeedX);
	f32 vz = JMASCos(angleY) * phys->unk2C * (*gpMarioThrowPower)
	         + mNormalThrowSpeedRate * (*gpMarioSpeedZ);
	mVelocity.x = vx;
	mVelocity.y = phys->unk30;
	mVelocity.z = vz;

	offLiveFlag(LIVE_FLAG_UNK10);

	JGeometry::TVec3<f32> v = mVelocity;
	mPosition.x += v.x;
	mPosition.y += v.y;
	mPosition.z += v.z;

	onLiveFlag(LIVE_FLAG_AIRBORNE);
	removeMapCollision();
	unk64 &= ~1;
	startAnim(5);
	startSound(5);
	mState = 1;
}

void TMapObjGeneral::touchingWater()
{
	if (animIsFinished() && hasModelOrAnimData(4))
		startAnim(0);
}

void TMapObjGeneral::touchingPlayer()
{
	if (animIsFinished() && hasModelOrAnimData(4))
		startAnim(0);
}

void TMapObjGeneral::holding()
{
	mPosition     = mHolder->mPosition;
	mGroundHeight = gpMap->checkGround(mPosition, &mGroundPlane);
}

void TMapObjGeneral::recovering()
{
	startSound(9);
	if (hasModelOrAnimData(6)) {
		J3DModel* model = getModel();
		MtxPtr mat      = model->getAnmMtx(0);
		f32 fVar1       = mat[1][3] - mSavedY;
		mDamageHeight += fVar1;
		calcEntryRadius();
		if (mHeldObject)
			mHeldObject->mPosition.y += fVar1;
		mSavedY = mat[1][3];
		if (!animIsFinished())
			return;
	} else if (mPosition.y < mSavedY) {
		mPosition.y += mMapObjData->mSink->unk4;
		if (mHeldObject)
			mHeldObject->mPosition.y += mMapObjData->mSink->unk4;
		return;
	}

	makeObjRecovered();
}

void TMapObjGeneral::sinking()
{
	mPosition.y -= mMapObjData->mSink->unk0;

	for (int i = 0; i < getColNum(); ++i) {
		if (getCollision(i)->checkActorType(0x1000000)) {
			recover();
			return;
		}
	}

	if (mPosition.y + mMapObjData->mHit->unkC[2].unk4 < mSavedY) {
		if (mPosition.x != mInitialPosition.x
		    || mPosition.z != mInitialPosition.z) {
			makeObjDefault();
			makeObjAppeared();
		} else {
			makeObjBuried();
		}
	}
}

void TMapObjGeneral::breaking()
{
	if (animIsFinished()) {
		makeObjDead();
		if (checkMapObjFlag(0x80000)) {
			makeObjDefault();
			waitToAppear(0);
		}
	}
}

void TMapObjGeneral::appearing()
{
	// TODO: uuuuuuuh...
	if (hasAnim(1)) {
		if (animIsFinished())
			goto uuuh;
		return;
	}

	{
		mScaling.x += mNormalAppearingScaleUp;
		mScaling.y += mNormalAppearingScaleUp;
		mScaling.z += mNormalAppearingScaleUp;
		if (mScaling.x < mInitialScaling.x)
			return;

		const JGeometry::TVec3<f32>& initialScaling = getInitialScaling();
		mScaling.set(initialScaling);
	}

uuuh:
	if (!checkLiveFlag(LIVE_FLAG_UNK10))
		return;

	makeObjAppeared();
}

void TMapObjGeneral::appeared()
{
	if (checkMapObjFlag(0x40000) && !(mLifeTimer > 0 ? true : false))
		makeObjDead();
}

void TMapObjGeneral::makeObjRecovered()
{
	makeObjDefault();
	makeObjAppeared();
}

void TMapObjGeneral::makeObjBuried()
{
	mSavedY = mPosition.y;
	mPosition.y -= mMapObjData->mHit->unkC[2].unkC;
	unk64 |= 1;
	removeMapCollision();
	mMActor = nullptr;
	mState  = 8;
}

void TMapObjGeneral::receiveMessageFromPlayer() { startAnim(4); }

u32 TMapObjGeneral::touchWater(THitActor* water)
{
	if (checkMapObjFlag(0x400000)) {
		kill();
		return 1;
	} else {
		if (hasModelOrAnimData(3)) {
			startAnim(3);
			mState = 5;
		}
		return 1;
	}
}

void TMapObjGeneral::touchPlayer(THitActor* player)
{
	TMapObjBase::touchPlayer(player);
	if (hasModelOrAnimData(4)) {
		startAnim(4);
		mState = 4;
	}
}

void TMapObjGeneral::recover()
{
	const TMapObjHitDataTable& hitData = mMapObjData->mHit->unkC[2];
	gpPollution->clean(mPosition.x, mSavedY, mPosition.z,
	                   (u16)(hitData.unk0 / 6.0f));

	setUpMapCollision(1);
	startAnim(6);
	mState = 9;
	setObjHitData(0);
	startSound(8);
	mDamageHeight = 0.0f;
	calcEntryRadius();
	unk64 &= ~0x1;
	if (hasModelOrAnimData(6)) {
		f32 tmp     = mPosition.y;
		mPosition.y = mSavedY;
		mSavedY      = tmp;
		getModel();
	}
}

void TMapObjGeneral::hold(TTakeActor* actor)
{
	if (mMapCollisionManager && mMapCollisionManager->unk8)
		mMapCollisionManager->unk8->remove();
	unk64 |= 1;
	mHolder = actor;
	mState  = 6;
}

void TMapObjGeneral::ensureTakeSituation()
{
	TMapObjBase::ensureTakeSituation();
	if (isState(6) && mHolder == nullptr) {
		mState = 1;
		offLiveFlag(LIVE_FLAG_UNK10);
	}
}

void TMapObjGeneral::kill()
{
	unk64 |= 1;
	removeMapCollision();
	onLiveFlag(LIVE_FLAG_UNK10 | LIVE_FLAG_UNK8);
	mLifeTimer = 0xffffffff;
	startAnim(2);
	mState = 3;
	startSound(2);
	breaking();
}

void TMapObjGeneral::appear()
{
	makeObjAppeared();
	startAnim(1);
	if (checkMapObjFlag(0x800000)) {
		mScaling.x = mNormalAppearingScaleUp;
		mScaling.y = mNormalAppearingScaleUp;
		mScaling.z = mNormalAppearingScaleUp;
	}

	if (!isActorType(0x20000010)
	    || !TFlagManager::smInstance->getBlueCoinFlag(
	        gpMarDirector->getCurrentMap(), unk134))
		startSound(1);

	appearing();
	if (checkMapObjFlag(0x40000))
		mLifeTimer = getLivingTime();

	mState = 2;
}

void TMapObjGeneral::work()
{
	switch (mState) {
	case 1:
		appeared();
		break;
	case 2:
		appearing();
		break;
	case 3:
		breaking();
		break;
	case 7:
		sinking();
		break;
	case 9:
		recovering();
		break;
	case 4:
		touchingPlayer();
		break;
	case 5:
		touchingWater();
		break;
	case 6:
		holding();
		break;
	case 8:
		waitingToRecover();
		break;
	}
}

void TMapObjGeneral::touchWall(JGeometry::TVec3<f32>* param_1,
                               TBGWallCheckRecord* param_2)
{
	param_1->x = param_2->mCenter.x;
	param_1->z = param_2->mCenter.z;
	calcReflectingVelocity(param_2->mResultWalls[0],
	                       mMapObjData->mPhysical->unk4->unk8, &mVelocity);
}

void TMapObjGeneral::checkWallCollision(JGeometry::TVec3<f32>* param_1)
{
	param_1->y += mMapObjData->mPhysical->unk4->unk1C;

	TBGWallCheckRecord check(*param_1, mBodyRadius, 4,
	                         mMapObjData->mPhysical->mWallCheckFlags);

	bool touched = gpMap->isTouchedWallsAndMoveXZ(&check);

	param_1->y -= mMapObjData->mPhysical->unk4->unk1C;

	if (touched) {
		mWallPlane = check.mResultWalls[0];
		touchWall(param_1, &check);
	} else {
		mWallPlane = 0;
	}
}

void TMapObjGeneral::touchRoof(JGeometry::TVec3<f32>* param_1)
{
	param_1->y = mRoofHeight;
}

void TMapObjGeneral::checkRoofCollision(JGeometry::TVec3<f32>* param_1)
{
	mRoofHeight = gpMap->checkRoof(param_1->x, param_1->y + mHeadHeight, param_1->z,
	                          &mRoofPlane);
	if (param_1->y + mHeadHeight >= mRoofHeight)
		touchRoof(param_1);
}

void TMapObjGeneral::touchGround(JGeometry::TVec3<f32>* param_1)
{
	if (mMapObjData->mPhysical ? true : false) {
		mVelocity.x *= mMapObjData->mPhysical->unk4->unk10;
		mVelocity.z *= mMapObjData->mPhysical->unk4->unk10;
	}

	if ((mMapObjData->mPhysical ? true : false)
	    && std::fabsf(JGeometry::TVec3<f32>(mVelocity).y)
	           > mMapObjData->mPhysical->unk4->unkC) {
		param_1->y -= JGeometry::TVec3<f32>(mVelocity).y;
		mVelocity.y *= -mMapObjData->mPhysical->unk4->unk4;
		if (isCoin(this)) {
			// TODO: this is an inline 100%
			f32 a = __fabsf(JGeometry::TVec3<f32>(mVelocity).y);
			if (gpMSound->gateCheck(0x4842)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x4842, mPosition, nullptr, a, 0, 0, nullptr, 0, 4);
			}
		} else {
			startSound(4);
		}
	} else {
		offLiveFlag(LIVE_FLAG_AIRBORNE);
		mVelocity.x = mVelocity.y = mVelocity.z = 0.0f;
		onLiveFlag(LIVE_FLAG_UNK10);
		param_1->y = mGroundHeight;
	}
}

void TMapObjGeneral::checkGroundCollision(JGeometry::TVec3<f32>* param_1)
{
	mGroundHeight = gpMap->checkGround(param_1->x, param_1->y + mHeadHeight,
	                                   param_1->z, &mGroundPlane);
	mGroundHeight += 1.0f;
	if (param_1->y <= mGroundHeight) {
		touchGround(param_1);
	} else if (!mGroundActor)
		onLiveFlag(LIVE_FLAG_AIRBORNE);
}

void TMapObjGeneral::calcVelocity()
{
	if (isAirborne()) {
		mVelocity.y -= getGravityY();
		mVelocity.y = MsClamp(mVelocity.y, -mBodyRadius, mBodyRadius);
	}

	if (mMapObjData->mPhysical ? true : false) {
		mVelocity.x *= mMapObjData->mPhysical->unk4->unk18;
		mVelocity.z *= mMapObjData->mPhysical->unk4->unk18;
		mVelocity.x = MsClamp(mVelocity.x, -mBodyRadius, mBodyRadius);
		mVelocity.z = MsClamp(mVelocity.z, -mBodyRadius, mBodyRadius);

		if (1.0f == mGroundPlane->mNormal.y) {
			if (std::fabsf(mVelocity.x) < mMapObjData->mPhysical->unk4->unkC)
				mVelocity.x = 0.0f;
			if (std::fabsf(mVelocity.z) < mMapObjData->mPhysical->unk4->unkC)
				mVelocity.z = 0.0f;
		}
	}
}

void TMapObjGeneral::bind()
{
	if (checkLiveFlag(LIVE_FLAG_UNK10))
		return;

	if (mBinder) {
		mBinder->bind(this);
		return;
	}

	calcVelocity();

	JGeometry::TVec3<f32> result = mPosition;
	result.add(mLinearVelocity);
	result.add(mVelocity);

	checkGroundCollision(&result);

	if (checkMapObjFlag(0x10000))
		checkWallCollision(&result);

	if (checkMapObjFlag(0x20000) && JGeometry::TVec3<f32>(mVelocity).y > 0.0f)
		checkRoofCollision(&result);

	if (mGroundPlane->isIllegalData()) {
		kill();
	} else {
		if (!isAirborne()) {
			JGeometry::TVec3<f32> v(mVelocity);
			if (JGeometry::TVec3<f32>(v).x == 0.0f
			    && JGeometry::TVec3<f32>(v).y == 0.0f
			    && JGeometry::TVec3<f32>(v).z == 0.0f)
				onLiveFlag(LIVE_FLAG_UNK10);
		}
		JGeometry::TVec3<f32> diff = result;
		diff.sub(mPosition);
		mLinearVelocity = diff;
	}
}

void TMapObjGeneral::control()
{
	TMapObjBase::control();
	if (checkMapObjFlag(0x1000000) && isState(1) && !isAirborne()
	    && isPollutedGround(mPosition))
		sink();

	work();
}

void TMapObjGeneral::calcRootMatrix()
{
	J3DModel* model = getModel();

	if (isState(6) && mHolder) {
		if (mMapObjData->mHold) {
			TMapObjHoldData* hold = mMapObjData->mHold;

			MtxPtr src = mHolder->getTakingMtx();
			MTXCopy(src, hold->unkC->getBaseTRMtx());
			hold->unkC->calc();

			MtxPtr src2 = hold->unk10;
			MTXCopy(src2, model->getBaseTRMtx());
			mPosition.set(src2[0][3], src2[1][3], src2[2][3]);
		} else {
			MtxPtr src = mHolder->getTakingMtx();
			MTXCopy(src, checkMapObjFlag(0x100) ? model->getAnmMtx(0)
			                                    : model->getBaseTRMtx());
			mPosition.set(src[0][3], src[1][3], src[2][3]);
		}
	} else {
		JGeometry::TVec3<f32> pos(mPosition.x, mPosition.y - mYOffset,
		                          mPosition.z);
		MsMtxSetXYZRPH(model->getBaseTRMtx(), pos.x, pos.y, pos.z, mRotation.x,
		               mRotation.y, mRotation.z);
	}
	model->setBaseScale(mScaling);
}

void TMapObjGeneral::perform(u32 param_1, JDrama::TGraphics* param_2)
{
	if (param_1 & 1) {
		if (isState(10)) {
			waitingToAppear();
		}
	} else {
		if (checkMapObjFlag(0x40000) && isLifeTimerActive()
		    && getLifeTimer() < getFlushTime()
		    && ((getLifeTimer() / mNormalFlushInterval) & 1) != 0) {
			return;
		}
	}

	TMapObjBase::perform(param_1, param_2);
}

BOOL TMapObjGeneral::receiveMessage(THitActor* sender, u32 message)
{
	int ret = TMapObjBase::receiveMessage(sender, message);
	if (ret)
		return true;

	// TODO: concerning. Is unkAC actually a Vec?
	if (message == HIT_MESSAGE_TAKE && checkMapObjFlag(0x100000)
	    && JGeometry::TVec3<f32>(mVelocity).squared() <= 3.814697e-06f
	    && (isState(2) || isState(1) || isState(4) || isState(5))) {
		hold((TTakeActor*)sender);
		return true;
	}

	if (message == HIT_MESSAGE_TAKE && sender->isActorType(0x10000025)
	    && (isState(2) || isState(1))) {
		hold((TTakeActor*)sender);
		return 1;
	}

	if (message == HIT_MESSAGE_UNK6 && isState(6)) {
		put();
		return true;
	}

	if (message == HIT_MESSAGE_UNK7 && isState(6)
	    && mMapObjData->mPhysical != nullptr) {
		thrown();
		return true;
	}

	if (message == HIT_MESSAGE_HIP_DROP && checkMapObjFlag(0x200000)) {
		kill();
		return true;
	}

	if (sender->isActorType(0x80000001)
	    && (message == HIT_MESSAGE_TRAMPLE
	        || message == HIT_MESSAGE_HIP_DROP)) {
		receiveMessageFromPlayer();
		return true;
	}

	if (message == HIT_MESSAGE_UNKB && checkMapObjFlag(0x200000)) {
		kill();
	}

	return false;
}

void TMapObjGeneral::loadAfter()
{
	TMapObjBase::loadAfter();
	if (checkMapObjFlag(0x1000000) && isPollutedGround(mPosition))
		makeObjBuried();
}

TMapObjGeneral::TMapObjGeneral(const char* name)
    : TMapObjBase(name)
    , mWallPlane(0)
    , mRoofPlane(0)
    , mRoofHeight(0.0f)
    , mSavedY(0.0f)
{
}
