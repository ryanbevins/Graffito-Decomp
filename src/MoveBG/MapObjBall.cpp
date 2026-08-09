#define JG_TUTIL_SQRT_OUT_OF_LINE
#include <MoveBG/MapObjBall.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <Strategic/HitActor.hpp>
#include <MarioUtil/MapUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <Enemy/PoiHana.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <Player/ModelWaterManager.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <stdio.h>
#include <string.h>
#include <Map/Map.hpp>
#include <Map/MapCollisionData.hpp>
#include <Map/MapData.hpp>
#include <Map/PollutionManager.hpp>
#include <Player/MarioAccess.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRActor.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
// Rogue includes for static init (JALList<*> instantiations)
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSSetSound.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

s32 TResetFruit::mFruitLivingTime = 14400;
f32 TResetFruit::mScaleUpSpeed    = 1.05f;
f32 TResetFruit::mRottingScaleSpeed;
f32 TResetFruit::mBreakingScaleSpeed    = 0.96f;
s32 TResetFruit::mFruitWaitTimeToAppear = 360;
u32 TResetFruit::mRottenColor;

TMapObjBall::TMapObjBall(const char* name)
    : TMapObjGeneral(name)
{
	unk148 = 0.0f;
	unk14C = 0.0f;
	unk150 = 0.0f;
	unk154 = 0.0f;
	unk158 = 0.0f;
	unk15C = 0.0f;
	unk160 = 0.0f;
	unk164 = 0.0f;
	unk168 = 0.0f;
	unk16C = 0.0f;
	unk170 = 0.0f;
	unk174 = 0.0f;
	unk178 = 0.0f;
	unk17C = 0.0f;
	unk180 = 0.0f;
	unk184 = 0.0f;
	unk188 = 0.0f;
	unk18C = 0.0f;
	unk190 = 0.0f;
	unk194 = 0;
	mInitialScaling.z = 0.0f;
	mInitialScaling.y = 0.0f;
	mInitialScaling.x = 0.0f;
}

TResetFruit::TResetFruit(const char* name)
    : TMapObjBall(name)
{
	unk198 = 0.0f;
	unk1A4 = 0;
	unk19C = 0xFF;
	unk19E = 0xFF;
	unk1A0 = 0xFF;
	unk1A2 = 0xFF;
}

TRandomFruit::TRandomFruit(const char* name)
    : TResetFruit(name)
{
	memset(mFruitName, 0, sizeof(mFruitName));
}

void TRandomFruit::initMapObj()
{
	int r = (int)(MsRandF() * 5.0f);
	switch (r) {
	case 0: snprintf(mFruitName, 32, "FruitCoconut"); break;
	case 1: snprintf(mFruitName, 32, "FruitDurian"); break;
	case 2: snprintf(mFruitName, 32, "FruitPapaya"); break;
	case 3: snprintf(mFruitName, 32, "FruitPine"); break;
	case 4:
	case 5:
	default: snprintf(mFruitName, 32, "FruitPine"); break;
	}
	unkF4 = mFruitName;
	TMapObjBall::initMapObj();
	SMS_InitPacket_OneTevColor(getModel(), 0, GX_TEVREG0,
	                           (GXColorS10*)&unk19C);
}

BOOL TCoverFruit::receiveMessage(THitActor* sender, u32 message)
{
	if (sender->isActorType(0x08000083) && message == 4) {
		unk64 |= 0x1;
		mHolder = (TTakeActor*)sender;
		return 1;
	}
	if (message == 0xB) {
		kill();
		TFlagManager::smInstance->setBool(1, 0x1038B);
		return 1;
	}
	return 0;
}

void TCoverFruit::calcRootMatrix()
{
	if (mHolder != nullptr) {
		MtxPtr src = mHolder->getTakingMtx();
		PSMTXCopy(src, getModel()->unk20);
		mPosition.set(src[0][3], src[1][3], src[2][3]);
	} else {
		MsMtxSetXYZRPH(getModel()->unk20, mPosition.x,
		               mPosition.y - mYOffset, mPosition.z, mRotation.x,
		               mRotation.y, mRotation.z);
	}
	getModel()->unk14 = mScaling;
}

void TCoverFruit::loadAfter()
{
	TMapObjBase::loadAfter();
	if (TFlagManager::smInstance->getBool(0x1038B)) {
		makeObjDead();
	}
}

BOOL TResetFruit::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 0xB) {
		if (isState(1) || isState(6) || isState(0xB)) {
			mState = 0xB;
			makeObjDefault();
			makeObjDead();
			calcRootMatrix();
			getModel()->calc();
			mLifeTimer = mFruitWaitTimeToAppear;
			unkF8 &= ~0x40000;
			mState = 0xA;
			if (gpMarDirector->mMap == 3 && unk1A4 != 0) {
				makeObjDead();
			}
			return 1;
		}
		return 0;
	}
	if (message == 0xD) {
		kill();
		return 1;
	}
	if (isState(1) || isState(6) || isState(0xB)) {
		if (!isState(2) && !isState(3) && !isState(0xC) && !isState(0xA)) {
			TMapObjBall::touchActor(sender);
			if (!(unkF8 & 0x04000000) && isState(1)
			    && !(mLiveFlag & 0x10)) {
				if (!isLifeTimerActive()) {
					unkF8 |= 0x40000;
					mLifeTimer = getLivingTime();
				}
				mLiveFlag &= ~0x10;
				mState = 0xB;
			}
		}
		BOOL result;
		if (TMapObjGeneral::receiveMessage(sender, message)) {
			result = 1;
		} else if (message == 4 && (unkF8 & 0x100000)) {
			hold((TTakeActor*)sender);
			result = 1;
		} else if (sender->isActorType(0x80000001)
		           && !isActorType(0x400000D0) && message != 4) {
			kicked();
			result = 1;
		} else {
			result = 0;
		}
		if (message == 6 && isState(1)) {
			mState = 0xB;
		}
		return result;
	}
	return 0;
}

void TResetFruit::touchActor(THitActor* actor)
{
	if (isState(2))
		return;
	if (isState(3))
		return;
	if (isState(0xC))
		return;
	if (isState(0xA))
		return;
	TMapObjBall::touchActor(actor);
	if (unkF8 & 0x04000000)
		return;
	if (!isState(1))
		return;
	if (mLiveFlag & 0x10)
		return;
	if (!isLifeTimerActive()) {
		unkF8 |= 0x40000;
		mLifeTimer = getLivingTime();
	}
	mLiveFlag &= ~0x10;
	mState = 0xB;
}

void TResetFruit::hold(TTakeActor* taker)
{
	JGeometry::TVec3<f32> v = mVelocity;
	if (!(v.length() > 10.0f)) {
		TMapObjGeneral::hold(taker);
		mVelocity.z = 0.0f;
		mVelocity.y = 0.0f;
		mVelocity.x = 0.0f;
	}
	mVelocity.z = 0.0f;
	mVelocity.y = 0.0f;
	mVelocity.x = 0.0f;
	mLiveFlag |= 0x10;
	if (unkF8 & 0x04000000)
		return;
	if (!isLifeTimerActive()) {
		unkF8 |= 0x40000;
		mLifeTimer = getLivingTime();
	}
}

void TResetFruit::waitingToAppear()
{
	if (gpMarDirector->mMap == 3 && unk1A4 != 0) {
		makeObjDead();
	}
	if (unkF8 & 0x04000000)
		return;
	if (isLifeTimerActive())
		return;
	if (mColCount != 0)
		return;
	unkF8 |= 0x40000;
	makeObjAppeared();
	Mtx scaleMtx;
	PSMTXScale(scaleMtx, 0.2f, 0.2f, 0.2f);
	MtxPtr modelMtx = getModel()->mNodeMatrices[0];
	concatOnlyRotFromLeft(scaleMtx, getModel()->mNodeMatrices[0],
	                      modelMtx);
	mScaling.y = 0.2f;
	unk64 |= 0x1;
	mState = 2;
	if (gpMSound->gateCheck(0x3802)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x3802, (Vec*)&mPosition, 0,
		                                          nullptr, 0, 4);
	}
}

void TResetFruit::appearing()
{
	Mtx scaleMtx;
	PSMTXScale(scaleMtx, mScaleUpSpeed, mScaleUpSpeed, mScaleUpSpeed);
	MtxPtr modelMtx = getModel()->getAnmMtx(0);
	concatOnlyRotFromLeft(scaleMtx, modelMtx, modelMtx);
	mScaling.y        = mScaling.y * mScaleUpSpeed;
	mScaledBodyRadius = mBodyRadius * mScaling.y;
	modelMtx[1][3]    = mBodyRadius * mScaling.y + mPosition.y;
	if (mScaling.y >= mInitialScaling.y) {
		mScaling.x = mInitialScaling.x;
		mScaling.y = mInitialScaling.y;
		mScaling.z = mInitialScaling.z;
		getModel()->calc();
		unk64 &= ~0x1;
		makeObjAppeared();
		mState = 1;
	}
}

void TResetFruit::kicked()
{
	if (unkF8 & 0x02000000)
		return;
	if (isState(6))
		return;
	f32 marioY = *gpMarioSpeedY;
	if (marioY < 0.0f)
		return;
	JGeometry::TVec3<f32> v = mVelocity;
	JGeometry::TVec3<f32> w = v;
	if (w.y > 0.0f)
		return;
	f32 dot = w.z * (gpMarioPos->z - mPosition.z)
	          + w.x * (gpMarioPos->x - mPosition.x);
	if ((mLiveFlag & 0x80) && dot > 0.0f)
		return;
	if (v.y == 0.0f) {
		mVelocity.y = unk178;
	} else {
		mVelocity.y = unk174 * marioY - unk160 * v.y;
	}
	mVelocity.x = unk170 * (*gpMarioSpeedX) + mVelocity.x;
	mVelocity.z = unk170 * (*gpMarioSpeedZ) + mVelocity.z;
	f32 thresh = mMapObjData->mPhysical->unk4->unkC;
	if (fabsf(mVelocity.x) < thresh && fabsf(mVelocity.z) < thresh) {
		mVelocity.x = (MsRandF() - 0.5f) * 2.0f;
		mVelocity.z = (MsRandF() - 0.5f) * 2.0f;
	}
	unk194 = 10;
	mLiveFlag &= ~0x10;
	SMS_GetMarioHitActor()->receiveMessage(this, 0xE);
	if (gpMSound->gateCheck(0x194F)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x194F, (Vec*)&mPosition, 0,
		                                          nullptr, 0, 4);
	}
}

void TResetFruit::breaking()
{
	Mtx scaleMtx;
	PSMTXScale(scaleMtx, 1.0f, mBreakingScaleSpeed, 1.0f);
	MtxPtr modelMtx = getModel()->getAnmMtx(0);
	concatOnlyRotFromLeft(scaleMtx, modelMtx, modelMtx);
	mScaling.y     = mScaling.y * mBreakingScaleSpeed;
	modelMtx[1][3] = mBodyRadius * mScaling.y + mPosition.y;
	if (mScaling.y < 0.2f) {
		mPosition.y += mBodyRadius * 3.0f;
		mScaling.x  = mInitialScaling.x;
		mScaling.y  = mInitialScaling.y;
		mScaling.z  = mInitialScaling.z;
		emitAndScale(0xE5, 0, &mPosition);
		if (gpMSound->gateCheck(0x387D)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x387D, (Vec*)&mPosition, 0, nullptr, 0, 4);
		}
		mLifeTimer = 0xF0;
		sleep();
		mState = 0xD;
	}
}

void TResetFruit::checkGroundCollision(JGeometry::TVec3<f32>* pos)
{
	u8 stage = gpMarDirector->mMap;
	if (stage != 7 && stage != 4) {
		TMapObjGeneral::checkGroundCollision(pos);
		return;
	}
	if (stage == 4) {
		mGroundHeight = gpMap->checkGround(pos->x, pos->y + 200.0f, pos->z,
		                                   &mGroundPlane);
		mGroundHeight = mGroundHeight + 1.0f;
		if (pos->y <= mGroundHeight) {
			touchGround(pos);
		} else {
			mLiveFlag |= 0x80;
		}
	} else {
		mGroundHeight = gpMap->checkGround(pos->x, pos->y + mHeadHeight,
		                                   pos->z, &mGroundPlane);
		u16 type = mGroundPlane->mBGType;
		if ((type == 0x801 || type == 0x203) ? true : false) {
			mGroundHeight = gpMap->checkGroundExactY(
			    pos->x, mGroundHeight - 200.0f, pos->z, &mGroundPlane);
		}
		mGroundHeight = mGroundHeight + 1.0f;
		if (pos->y <= mGroundHeight) {
			touchGround(pos);
		} else {
			mLiveFlag |= 0x80;
		}
	}
}

void TResetFruit::touchGround(JGeometry::TVec3<f32>* pos)
{
	if (mGroundPlane->isDeathPlane()) {
		mState = 0xB;
		makeObjDefault();
		makeObjDead();
		calcRootMatrix();
		getModel()->calc();
		mLifeTimer = mFruitWaitTimeToAppear;
		unkF8 &= ~0x40000;
		mState = 0xA;
		if (gpMarDirector->mMap == 3 && unk1A4 != 0) {
			makeObjDead();
		}
		pos->x = mPosition.x;
		pos->y = mPosition.y;
		pos->z = mPosition.z;
	} else {
		TMapObjBall::touchGround(pos);
	}
}

void TResetFruit::makeObjWaitingToAppear()
{
	mState = 0xB;
	makeObjDefault();
	makeObjDead();
	calcRootMatrix();
	getModel()->calc();
	mLifeTimer = mFruitWaitTimeToAppear;
	unkF8 &= ~0x40000;
	mState = 0xA;
	if (gpMarDirector->mMap == 3 && unk1A4 != 0) {
		makeObjDead();
	}
}

u32 TResetFruit::touchWater(THitActor* actor)
{
	if (!isState(6) && !isState(2)) {
		JGeometry::TVec3<f32> vel = mVelocity;
		JGeometry::TVec3<f32> result(vel.x, vel.y, vel.z);
		JGeometry::TVec3<f32>* speed
		    = ((TMapObjBase*)actor)->getWaterSpeed(actor);
		f32 factor = unk17C;
		result.x   = speed->x * factor + result.x;
		result.y   = speed->y * factor + result.y;
		result.z   = speed->z * factor + result.z;
		mVelocity = result;
		mLiveFlag &= ~0x10;
	}
	if (!isLifeTimerActive()) {
		unkF8 |= 0x40000;
		mLifeTimer = getLivingTime();
	}
	mLiveFlag &= ~0x10;
	mState = 0xB;
	return 1;
}

void TResetFruit::touchPollution()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0x8B, &mPosition, 0, nullptr);
	if (gpMSound->gateCheck(0x3881)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x3881, (Vec*)&mPosition, 0,
		                                          nullptr, 0, 4);
	}
	makeObjDefault();
	mState = 0xB;
	makeObjDefault();
	makeObjDead();
	calcRootMatrix();
	getModel()->calc();
	mLifeTimer = mFruitWaitTimeToAppear;
	unkF8 &= ~0x40000;
	mState = 0xA;
	if (gpMarDirector->mMap == 3 && unk1A4 != 0) {
		makeObjDead();
	}
}

void TResetFruit::touchWaterSurface()
{
	emitColumnWater();
	if (gpMSound->gateCheck(0x3875)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x3875, (Vec*)&mPosition, 0,
		                                          nullptr, 0, 4);
	}
	mState = 0xB;
	makeObjDefault();
	makeObjDead();
	calcRootMatrix();
	getModel()->calc();
	mLifeTimer = mFruitWaitTimeToAppear;
	unkF8 &= ~0x40000;
	mState = 0xA;
	if (gpMarDirector->mMap == 3 && unk1A4 != 0) {
		makeObjDead();
	}
}

void TResetFruit::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (gpMarDirector->mMap == 7) {
		if (isState(6)) {
			if (mLiveFlag & 0x200) {
				mLiveFlag &= ~0x200;
			}
		} else {
			JGeometry::TVec3<f32> velocity = mVelocity;
			if (!velocity.isZero()) {
				if (mLiveFlag & 0x200) {
					mLiveFlag &= ~0x200;
				}
			} else if (!gpCubeArea->isInAreaCube(mPosition)
			           && isState(0xB)
			           && (mPosition.x != mInitialPosition.x
			               || mPosition.z != mInitialPosition.z)) {
				mState = 0xB;
				makeObjDefault();
				makeObjDead();
				calcRootMatrix();
				getModel()->calc();
				mLifeTimer = mFruitWaitTimeToAppear;
				unkF8 &= ~0x40000;
				mState = 0xA;
				if (gpMarDirector->mMap == 3 && unk1A4 != 0) {
					makeObjDead();
				}
				return;
			}
		}
	}
	TMapObjGeneral::perform(flags, graphics);
}

void TResetFruit::makeObjAppeared()
{
	if (unkF8 & 0x04000000) {
		makeObjDefault();
	}
	TMapObjBase::makeObjAppeared();
	calcCurrentMtx();
	Mtx* m       = getModel()->mNodeMatrices;
	(*m)[0][3]   = mPosition.x;
	(*m)[1][3]   = mPosition.y + mBodyRadius;
	(*m)[2][3]   = mPosition.z;
	if (isActorType(0x40000394)) {
		if ((*m)[1][1] > 0.0f) {
			(*m)[1][3] = (*m)[1][3] - 50.0f * (*m)[1][1];
		}
	}
	if (isActorType(0x40000392)) {
		(*m)[1][3] = (*m)[1][3] - 10.0f * (1.0f - (*m)[1][1]);
	}
	unkE8 = 0;
	if (unkF8 & 0x04000000) {
		mState = 0xB;
	}
}

void TResetFruit::control()
{
	switch (mState) {
	case 1: {
		unk64 &= ~0x1;
		for (int i = 0; i < mColCount; i++) {
			THitActor* col = mCollisions[i];
			u16 st         = mState;
			if (st == 2 || st == 3 || st == 0xC || st == 0xA)
				continue;
			TMapObjBall::touchActor(col);
			if (unkF8 & 0x04000000)
				continue;
			if (!isState(1))
				continue;
			if (mLiveFlag & 0x10)
				continue;
			if (!isLifeTimerActive()) {
				unkF8 |= 0x40000;
				mLifeTimer = getLivingTime();
			}
			mLiveFlag &= ~0x10;
			mState = 0xB;
		}
		if (mGroundPlane->mActor != nullptr) {
			calcCurrentMtx();
		}
	} break;
	case 2:
	case 3: {
		TMapObjGeneral::control();
		if (unk194 != 0) {
			unk194 = unk194 - 1;
		}
		if (isState(6)) {
			Mtx tmp;
			PSMTXCopy(mHolder->getTakingMtx(), tmp);
			tmp[1][3] = tmp[1][3] + unk190;
			PSMTXCopy(tmp, getModel()->mNodeMatrices[0]);
		} else {
			JGeometry::TVec3<f32> v = mVelocity;
			f32 sq = v.x * v.x + v.y * v.y + v.z * v.z;
			if (sq > 0.0000038146973f
			    || mGroundPlane->mActor != nullptr) {
				calcCurrentMtx();
			}
		}
	} break;
	case 6: {
		TMapObjBall::control();
		if (unkF8 & 0x04000000)
			break;
		if (isLifeTimerActive())
			break;
		if (mHolder != nullptr) {
			mHolder->receiveMessage(this, 8);
			mHolder->mHeldObject = nullptr;
			mHolder              = nullptr;
		}
		mVelocity.x = 0.0f;
		mVelocity.y = 0.0f;
		mVelocity.z = 0.0f;
		mState      = 0xC;
	} break;
	case 0xB: {
		unk64 &= ~0x1;
		if (gpMarDirector->mMap == 4) {
			if (mLiveFlag & 0x10) {
				mLiveFlag &= ~0x10;
			}
		}
		if (mGroundPlane->mActor == nullptr) {
			unk198 = 0.0f;
		} else {
			if (mLiveFlag & 0x10) {
				mLiveFlag &= ~0x10;
			}
			if (mPosition.y < 200.0f + mGroundHeight) {
				TLiveActor* act = (TLiveActor*)mGroundPlane->mActor;
				if (act->isActorType(0x400000CD)
				    || act->isActorType(0x400000CD)) {
					f32 prev = unk198;
					unk198   = SMS_GetSandRiseUpRatio(this);
					if (unk198 > 0.05f) {
						if (unk198 > prev) {
							mVelocity.y = mVelocity.y + 20.0f;
						}
					}
				}
			}
		}
		TMapObjBall::control();
		if (unkF8 & 0x04000000)
			break;
		if (isLifeTimerActive())
			break;
		if (mHolder != nullptr) {
			mHolder->receiveMessage(this, 8);
			mHolder->mHeldObject = nullptr;
			mHolder              = nullptr;
		}
		mVelocity.x = 0.0f;
		mVelocity.y = 0.0f;
		mVelocity.z = 0.0f;
		mState      = 0xC;
	} break;
	case 0xC: {
		mPosition.y = mBodyRadius * 0.5f + mPosition.y;
		mScaling.x  = mInitialScaling.x;
		mScaling.y  = mInitialScaling.y;
		mScaling.z  = mInitialScaling.z;
		emitAndScale(0xE5, 0, &mPosition);
		if (gpMSound->gateCheck(0x387D)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x387D, (Vec*)&mPosition, 0, nullptr, 0, 4);
		}
		mLifeTimer = 0xF0;
		sleep();
		mState = 0xD;
	} break;
	case 0xD: {
		if (isLifeTimerActive())
			break;
		unk19C = 0xFF;
		unk19E = 0xFF;
		unk1A0 = 0xFF;
		awake();
		mState = 0xB;
		makeObjDefault();
		makeObjDead();
		calcRootMatrix();
		getModel()->calc();
		mLifeTimer = mFruitWaitTimeToAppear;
		unkF8 &= ~0x40000;
		mState = 0xA;
		if (gpMarDirector->mMap == 3 && unk1A4 != 0) {
			makeObjDead();
		}
	} break;
	default:
		break;
	}
}

void TResetFruit::makeObjLiving()
{
	u8 hasTimer;
	if (mLifeTimer > 0)
		hasTimer = 1;
	else
		hasTimer = 0;
	if (!hasTimer) {
		unkF8 |= 0x40000;
		mLifeTimer = getLivingTime();
	}
	mLiveFlag &= ~0x10;
	mState = 0xB;
}

void TResetFruit::thrown()
{
	TMapObjGeneral::thrown();
	mState = 0xB;
}

void TResetFruit::initMapObj()
{
	TMapObjBall::initMapObj();
	SMS_InitPacket_OneTevColor(getModel(), 0, GX_TEVREG0,
	                           (GXColorS10*)&unk19C);
}

void TMapObjBall::makeObjDefault()
{
	TMapObjBase::makeObjDefault();
	Mtx* m = getModel()->mNodeMatrices;
	(*m)[0][3] = mPosition.x;
	(*m)[1][3] = mPosition.y + mBodyRadius;
	(*m)[2][3] = mPosition.z;
}

void TMapObjBall::put()
{
	TMapObjGeneral::put();
	calcCurrentMtx();
}

u32 TMapObjBall::touchWater(THitActor* actor)
{
	if (isState(6) || isState(2))
		return 1;
	JGeometry::TVec3<f32> v;
	JGeometry::TVec3<f32> velocity = mVelocity;
	v.set(velocity);
	JGeometry::TVec3<f32>* speed = ((TMapObjBase*)actor)->getWaterSpeed(actor);
	f32 speedX                    = speed->x;
	f32 factor                   = unk17C;
	v.x                          = speedX * factor + v.x;
	v.y                          = speed->y * factor + v.y;
	v.z                          = speed->z * factor + v.z;
	mVelocity                    = v;
	mLiveFlag &= ~0x10;
	return 1;
}

void TMapObjBall::hold(TTakeActor* taker)
{
	JGeometry::TVec3<f32> v = mVelocity;
	if (!(v.length() > 10.0f)) {
		TMapObjGeneral::hold(taker);
		mVelocity.z = 0.0f;
		mVelocity.y = 0.0f;
		mVelocity.x = 0.0f;
	}
}

void TMapObjBall::makeObjAppeared()
{
	TMapObjBase::makeObjAppeared();
	calcCurrentMtx();
	Mtx* m       = getModel()->mNodeMatrices;
	(*m)[0][3]   = mPosition.x;
	(*m)[1][3]   = mPosition.y + mBodyRadius;
	(*m)[2][3]   = mPosition.z;
	if (isActorType(0x40000394)) {
		if ((*m)[1][1] > 0.0f) {
			(*m)[1][3] = (*m)[1][3] - 50.0f * (*m)[1][1];
		}
	}
	if (isActorType(0x40000392)) {
		(*m)[1][3] = (*m)[1][3] - 10.0f * (1.0f - (*m)[1][1]);
	}
	unkE8 = 0;
}

void TMapObjBall::control()
{
	TMapObjGeneral::control();
	if (unk194 != 0) {
		unk194 = unk194 - 1;
	}
	if (isState(6)) {
		Mtx tmp;
		PSMTXCopy(mHolder->getTakingMtx(), tmp);
		tmp[1][3] = tmp[1][3] + unk190;
		PSMTXCopy(tmp, getModel()->mNodeMatrices[0]);
	} else {
		JGeometry::TVec3<f32> v = mVelocity;
		f32 sq                  = v.x * v.x + v.y * v.y + v.z * v.z;
		if (sq <= 0.0000038146973f && mGroundPlane->mActor == nullptr)
			return;
		calcCurrentMtx();
	}
}

void TMapObjBall::initMapObj()
{
	TMapObjGeneral::initMapObj();
	mInitialScaling.x = mScaling.x;
	mInitialScaling.y = mScaling.y;
	mInitialScaling.z = mScaling.z;
	switch (mActorType) {
	case 0x40000064:
		unk148      = 0.6f;
		unk14C      = 2.0f;
		unk150      = 0.02f;
		unk154      = 0.0f;
		unk158      = 0.055f;
		unk15C      = 0.02f;
		unk160      = 0.83f;
		unk170      = 0.9f;
		unk174      = 0.13f;
		unk178      = 20.0f;
		unk164      = 0.5f;
		unk168      = 0.02f;
		unk16C      = 0.5f;
		unk17C      = 1.2f;
		unk180      = 0.8f;
		unk184      = 1.0f;
		unk188      = 1.5f;
		mBodyRadius = 50.0f * mScaling.y;
		unk18C      = mBodyRadius / 3.0f;
		break;
	case 0x400000D0:
		unk14C      = 4.0f;
		unk150      = 0.0f;
		unk154      = 0.0f;
		unk158      = 0.15f;
		unk15C      = 0.0f;
		unk160      = 0.9f;
		unk164      = 0.06f;
		unk168      = 1.5f;
		unk16C      = 0.5f;
		unk170      = 0.5f;
		unk174      = 0.2f;
		unk178      = 2.5f;
		unk17C      = 0.001f;
		unk180      = 0.3f;
		unk184      = 0.0f;
		unk188      = 0.0f;
		mBodyRadius = 50.0f * mScaling.y;
		unk18C      = mBodyRadius / 3.0f;
		break;
	case 0x40000390:
	case 0x40000391:
	case 0x40000392:
	case 0x40000395:
		unk148      = 0.4f;
		unk14C      = 0.2f;
		unk150      = 1.3f;
		unk154      = 0.0f;
		unk158      = 1.2f;
		unk15C      = 0.8f;
		unk160      = 0.5f;
		unk170      = 0.9f;
		unk174      = 0.13f;
		unk178      = 20.0f;
		unk164      = 2.0f;
		unk168      = 0.02f;
		unk16C      = 0.3f;
		unk17C      = 0.05f;
		unk180      = 0.5f;
		unk184      = 1.0f;
		unk188      = 1.5f;
		mBodyRadius = 50.0f * mScaling.y;
		unk18C      = 50.0f;
		break;
	case 0x40000393:
		unk148      = 0.6f;
		unk14C      = 0.2f;
		unk150      = 1.3f;
		unk154      = 15.0f;
		unk158      = 0.5f;
		unk15C      = 1.3f;
		unk160      = 1.0f;
		unk170      = 0.9f;
		unk174      = 0.13f;
		unk178      = 20.0f;
		unk164      = 0.2f;
		unk168      = 0.02f;
		unk16C      = 0.3f;
		unk17C      = 0.05f;
		unk180      = 0.5f;
		unk184      = 1.0f;
		unk188      = 1.5f;
		mBodyRadius = 50.0f * mScaling.y;
		unk18C      = 50.0f;
		break;
	case 0x40000394:
		unk148      = 0.2f;
		unk14C      = 0.0f;
		unk150      = 0.0f;
		unk154      = 0.0f;
		unk158      = 0.0f;
		unk15C      = 0.0f;
		unk160      = 0.0f;
		unk170      = 0.0f;
		unk174      = 0.0f;
		unk178      = 0.0f;
		unk164      = 0.0f;
		unk168      = 0.0f;
		unk16C      = 0.0f;
		unk17C      = 0.05f;
		unk180      = 0.5f;
		unk184      = 1.0f;
		unk188      = 1.5f;
		mBodyRadius = 50.0f * mScaling.y;
		unk18C      = 50.0f;
		break;
	default:
		break;
	}
	if (isActorType(0x40000393)) {
		mBodyRadius = 45.0f * mScaling.y;
		unk190      = mBodyRadius;
	}
	if (isActorType(0x40000390)) {
		mBodyRadius = 40.0f * mScaling.y;
		unk190      = 20.0f;
	}
	if (isActorType(0x40000391)) {
		mBodyRadius = 40.0f * mScaling.y;
		unk190      = 20.0f;
	}
	if (isActorType(0x40000392)) {
		unk190 = 10.0f;
	}
}

BOOL TMapObjBall::receiveMessage(THitActor* sender, u32 message)
{
	if (TMapObjGeneral::receiveMessage(sender, message))
		return 1;
	if (message == 4 && (unkF8 & 0x100000)) {
		hold((TTakeActor*)sender);
		return 1;
	}
	if (sender->isActorType(0x80000001) && !isActorType(0x400000D0)
	    && message != 4) {
		kicked();
		return 1;
	}
	return 0;
}

void TMapObjBall::kicked()
{
	JGeometry::TVec3<f32> v = mVelocity;
	JGeometry::TVec3<f32> w = v;
	if (w.y > 0.0f) {
		// fall through to common tail
	} else {
		JGeometry::TVec3<f32> equalityVelocity = v;
		if (equalityVelocity.y == 0.0f) {
			mVelocity.y = unk178;
		} else {
			JGeometry::TVec3<f32> bounceVelocity = v;
			mVelocity.y
			    = unk174 * (*gpMarioSpeedY) - unk160 * bounceVelocity.y;
		}
		mVelocity.x = unk170 * (*gpMarioSpeedX) + mVelocity.x;
		mVelocity.z = unk170 * (*gpMarioSpeedZ) + mVelocity.z;
		f32 thresh = mMapObjData->mPhysical->unk4->unkC;
		if (fabsf(mVelocity.x) < thresh && fabsf(mVelocity.z) < thresh) {
			mVelocity.x = 0.02f * MsRandF() - 0.2f;
			mVelocity.z = 0.02f * MsRandF() - 0.2f;
		}
		unk194 = 10;
		mLiveFlag &= ~0x10;
		mLiveFlag |= 0x80;
		SMS_GetMarioHitActor()->receiveMessage(this, 0xE);
		if (!isActorType(0x400000D0)) {
			if (gpMSound->gateCheck(0x194F)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x194F, (Vec*)&mPosition, 0, nullptr, 0, 4);
			}
		}
	}
}

void TMapObjBall::calcCurrentMtx()
{
	Mtx rotMtx;
	rotMtx[2][3] = 0.0f;
	rotMtx[1][3] = 0.0f;
	rotMtx[0][3] = 0.0f;
	rotMtx[1][2] = 0.0f;
	rotMtx[0][2] = 0.0f;
	rotMtx[2][1] = 0.0f;
	rotMtx[0][1] = 0.0f;
	rotMtx[2][0] = 0.0f;
	rotMtx[1][0] = 0.0f;
	rotMtx[2][2] = 1.0f;
	rotMtx[1][1] = 1.0f;
	rotMtx[0][0] = 1.0f;

	{
		JGeometry::TVec3<f32> v = mVelocity;
		f32 thresh              = mMapObjData->mPhysical->unk4->unkC;
		if (fabsf(v.x) < thresh) {
			JGeometry::TVec3<f32> v2 = mVelocity;
			if (fabsf(v2.z) < thresh && mGroundPlane->mNormal.y == 1.0f) {
				mVelocity.x = 0.0f;
				mVelocity.z = 0.0f;
			}
		}
	}

	{
		JGeometry::TVec3<f32> v3 = mVelocity;
		f32 thresh               = mMapObjData->mPhysical->unk4->unkC;
		bool computeRot          = false;
		if (fabsf(v3.x) > thresh) {
			computeRot = true;
		} else {
			JGeometry::TVec3<f32> v4 = mVelocity;
			if (fabsf(v4.z) > thresh)
				computeRot = true;
		}

		if (computeRot) {
			JGeometry::TVec3<f32> v5 = mVelocity;
			JGeometry::TVec3<f32> v6 = mVelocity;
			JGeometry::TVec3<f32> result;
			getVerticalVecToTargetXZ(mPosition.x + v6.x,
			                         mPosition.z + v6.z, &result);

			JGeometry::TVec3<f32> v7 = mVelocity;
			JGeometry::TVec3<f32> v8(v7.x, v7.y, v7.z);
			f32 sq = v8.x * v8.x + v8.z * v8.z;
			f32 mag;
			if (sq > 0.0f)
				mag = JGeometry::TUtil<f32>::sqrt(sq);
			else
				mag = sq;

			f32 angle = 2.0f * (mag / mBodyRadius);

			JGeometry::TVec3<f32> axis;
			f32 dot = result.dot(result);
			if (dot <= 0.0000038146973f) {
				axis.x = 0.0f;
				axis.y = 0.0f;
				axis.z = 0.0f;
			} else {
				axis.scale(1.0f * JGeometry::TUtil<f32>::inv_sqrt(dot),
				           result);
			}

			f32 sn = sinf(angle);
			f32 cs = cosf(angle);
			f32 om = 1.0f - cs;

			rotMtx[0][0] = om * (axis.x * axis.x) + cs;
			rotMtx[0][1] = (om * axis.x) * axis.y - sn * axis.z;
			rotMtx[0][2] = (om * axis.x) * axis.z + sn * axis.y;
			rotMtx[1][0] = (om * axis.x) * axis.y + sn * axis.z;
			rotMtx[1][1] = om * (axis.y * axis.y) + cs;
			rotMtx[1][2] = (om * axis.y) * axis.z - sn * axis.x;
			rotMtx[2][0] = (om * axis.x) * axis.z - sn * axis.y;
			rotMtx[2][1] = (om * axis.y) * axis.z + sn * axis.x;
			rotMtx[2][2] = om * (axis.z * axis.z) + cs;
		}
	}

	Mtx animMtx;
	JGeometry::gekko_ps_copy12(animMtx, getModel()->mNodeMatrices);
	animMtx[0][3] = 0.0f;
	animMtx[1][3] = 0.0f;
	animMtx[2][3] = 0.0f;
	PSMTXConcat(rotMtx, animMtx, rotMtx);

	rotMtx[0][3] = mPosition.x;
	rotMtx[1][3] = mPosition.y + mBodyRadius;
	rotMtx[2][3] = mPosition.z;

	if (isActorType(0x40000394)) {
		if (rotMtx[1][1] > 0.0f) {
			rotMtx[1][3] = rotMtx[1][3] - 50.0f * rotMtx[1][1];
		}
	}
	if (isActorType(0x40000392)) {
		rotMtx[1][3] = rotMtx[1][3] - 10.0f * (1.0f - rotMtx[1][1]);
	}

	PSMTXCopy(rotMtx, getModel()->mNodeMatrices[0]);
}

void TMapObjBall::boundByActor(THitActor* actor)
{
	JGeometry::TVec3<f32> diff;
	diff.x = actor->mPosition.x - mPosition.x;
	diff.y = 0.0f;
	diff.z = actor->mPosition.z - mPosition.z;
	f32 r;
	if (isActorType(0x400000D0))
		r = mAttackRadius + actor->mDamageRadius;
	else
		r = mDamageRadius;
	if (r * r < diff.x * diff.x + diff.z * diff.z)
		return;
	if (diff.x != 0.0f && diff.z != 0.0f) {
		MsVECNormalize((Vec*)&diff, (Vec*)&diff);
	}
	if (actor->isActorType(0x80000001)) {
		if (!(unkF8 & 0x02000000)) {
			f32 thresh = mMapObjData->mPhysical->unk4->unkC;
			if (fabsf(*gpMarioSpeedX) > thresh
			    || fabsf(*gpMarioSpeedZ) > thresh) {
				mVelocity.y = mVelocity.y + unk150;
				if (!isActorType(0x400000D0)) {
					if (gpMSound->gateCheck(0x194F)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x194F, (Vec*)&mPosition, 0, nullptr, 0, 4);
					}
				}
			} else {
				mVelocity.y = mVelocity.y + unk154;
			}
			mVelocity.x += unk148 * (*gpMarioSpeedX) - diff.x * unk14C;
			mVelocity.z += unk148 * (*gpMarioSpeedZ) - diff.z * unk14C;
			actor->receiveMessage(this, 0xE);
		}
	} else {
		JGeometry::TVec3<f32> v  = mVelocity;
		JGeometry::TVec3<f32> v2 = v;
		f32 dot                  = v2.x * diff.x + v2.z * diff.z;
		bool bigBounce           = false;
		if (dot >= 0.0f) {
			JGeometry::TVec3<f32> v3 = v;
			f32 thresh = mMapObjData->mPhysical->unk4->unkC;
			if (fabsf(v3.x) > thresh && fabsf(v3.z) > thresh) {
				bigBounce = true;
			}
		}
		if (bigBounce) {
			f32 add     = 1.0f + unk16C;
			mVelocity.x = mVelocity.x - add * (diff.x * dot);
			mVelocity.y = mVelocity.y + unk168;
			mVelocity.z = mVelocity.z - add * (diff.z * dot);
			actor->receiveMessage(this, 0x10);
			if (isActorType(0x400000D0)) {
				if (gpMSound->gateCheck(0x3862)) {
					MSoundSESystem::MSoundSE::startSoundActor(
					    0x3862, (Vec*)&mPosition, 0, nullptr, 0, 4);
				}
			}
		} else {
			mVelocity.x = mVelocity.x - diff.x * unk164;
			mVelocity.y = mVelocity.y + unk168;
			mVelocity.z = mVelocity.z - diff.z * unk164;
		}
	}
	if (actor->isActorType(0x80000001) && !(unkF8 & 0x02000000)) {
		JGeometry::TVec3<f32> vc  = mVelocity;
		JGeometry::TVec3<f32> vc2 = vc;
		if (vc2.y < 0.0f) {
			if (mPosition.y + mBodyRadius > 130.0f + gpMarioPos->y) {
				mVelocity.y = unk160 * (-vc.y);
				mVelocity.x
				    = mVelocity.x + unk158 * (*gpMarioSpeedX);
				mVelocity.y
				    = mVelocity.y + unk15C * (*gpMarioSpeedY);
				mVelocity.z
				    = mVelocity.z + unk158 * (*gpMarioSpeedZ);
				if (!isActorType(0x400000D0)) {
					if (gpMSound->gateCheck(0x194F)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x194F, (Vec*)&mPosition, 0, nullptr, 0, 4);
					}
				}
			}
		}
	}
	unk194 = 10;
	mLiveFlag &= ~0x10;
	mLiveFlag |= 0x80;
}
#pragma dont_inline off

void TMapObjBall::touchActor(THitActor* actor)
{
	if (unk194 != 0)
		return;
	{
		u8 t;
		if (mState == 6)
			t = 1;
		else
			t = 0;
		if (t)
			return;
	}
	if (isHideObj(actor))
		return;
	if (actor->isActorType(0x08000083)
	    || actor->isActorType(0x400000CA)
	    || actor->isActorType(0x400000CC))
		return;

	if (actor->isActorType(0x80000001) && !isActorType(0x400000D0)
	    && *gpMarioSpeedY != 0.0f) {
		kicked();
	} else {
		boundByActor(actor);
	}
}

void TMapObjBall::checkWallCollision(JGeometry::TVec3<f32>* pos)
{
	JGeometry::TVec3<f32> tmp;
	tmp.x = pos->x;
	tmp.y = pos->y + mBodyRadius;
	tmp.z = pos->z;
	TBGWallCheckRecord record(tmp, mBodyRadius, 4,
	                          mMapObjData->mPhysical->mWallCheckFlags);
	if (gpMap->isTouchedWallsAndMoveXZ(&record)) {
		mWallPlane = record.mResultWalls[0];
		pos->x = record.mCenter.x;
		pos->z = record.mCenter.z;
		touchWall(pos, &record);
	} else {
		mWallPlane = nullptr;
	}
}

void TMapObjBall::touchGround(JGeometry::TVec3<f32>* pos)
{
	JGeometry::TVec3<f32> v = mVelocity;
	f32 mag = JGeometry::TUtil<f32>::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	f32 absMag              = fabsf(mag);
	if (absMag > 0.05f) {
		if (isActorType(0x400000D0)) {
			if (mScaling.y >= 5.0f) {
				if (gpMSound->gateCheck(0x308A)) {
					MSoundSESystem::MSoundSE::startSoundActorWithInfo(
					    0x308A, (Vec*)&mPosition, nullptr, absMag, 0, 0,
					    nullptr, 0, 4);
				}
			} else {
				if (gpMSound->gateCheck(0x308B)) {
					MSoundSESystem::MSoundSE::startSoundActorWithInfo(
					    0x308B, (Vec*)&mPosition, nullptr, absMag, 0, 0,
					    nullptr, 0, 4);
				}
			}
		}
	}
	u16 type = mGroundPlane->mBGType;
	bool isWater;
	if (type == 0x100)
		isWater = true;
	else if (type == 0x101)
		isWater = true;
	else if ((u16)(type - 0x102) <= 3)
		isWater = true;
	else if (type == 0x4104)
		isWater = true;
	else
		isWater = false;
	if (isWater) {
		touchWaterSurface();
		pos->x = mPosition.x;
		pos->y = mPosition.y;
		pos->z = mPosition.z;
	} else if (gpPollution->isPolluted(pos->x, pos->y, pos->z)) {
		touchPollution();
		pos->x = mPosition.x;
		pos->y = mPosition.y;
		pos->z = mPosition.z;
	} else {
		if (mVelocity.y > -unk188) {
			mLiveFlag &= ~0x80;
			mVelocity.y = 0.0f;
			pos->y      = mGroundHeight;
		} else {
			rebound(pos);
		}
	}
	if (!(mLiveFlag & 0x80)) {
		mVelocity.x = unk180 * mGroundPlane->mNormal.x + mVelocity.x;
		mVelocity.z = unk180 * mGroundPlane->mNormal.z + mVelocity.z;
	}
	f32 fric    = mMapObjData->mPhysical->unk4->unk10;
	mVelocity.x = mVelocity.x * fric;
	mVelocity.z = mVelocity.z * fric;
	if (isActorType(0x400000D0)) {
		f32 thresh = mMapObjData->mPhysical->unk4->unkC;
		if (fabsf(mVelocity.x) > thresh || fabsf(mVelocity.z) > thresh) {
			if (gpMSound->gateCheck(0x1009)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x1009, (Vec*)&mPosition, 0, nullptr, 0, 4);
			}
		}
	}
}

void TMapObjBall::touchRoof(JGeometry::TVec3<f32>* pos)
{
	if (pos->y > mRoofHeight) {
		pos->y = mRoofHeight;
	}
	calcReflectingVelocity(mRoofPlane, mMapObjData->mPhysical->unk4->unk4,
	                       &mVelocity);
}

void TMapObjBall::rebound(JGeometry::TVec3<f32>* pos)
{
	calcReflectingVelocity(mGroundPlane,
	                       mMapObjData->mPhysical->unk4->unk4, &mVelocity);
	pos->y                                = mGroundHeight;
	mLiveFlag |= 0x80;
	if (isActorType(0x400000D0)) {
		if (mScaling.y >= 5.0f) {
			f32 vol = __fabsf(mGroundPlane->mNormal.y);
			if (gpMSound->gateCheck(0x3889)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x3889, (Vec*)&mPosition, nullptr, vol, 0, 0, nullptr, 0, 4);
			}
		} else {
			f32 vol = __fabsf(mGroundPlane->mNormal.y);
			if (gpMSound->gateCheck(0x388C)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x388C, (Vec*)&mPosition, nullptr, vol, 0, 0, nullptr, 0, 4);
			}
		}
	} else {
		u32 soundId = mMapObjData->mSound->unk4->unk0[4];
		if (gpMSound->gateCheck(soundId)) {
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(
			    soundId, (Vec*)&mPosition, (Vec*)&mVelocity, 0.0f, 0, 0,
			    nullptr, 0, 4);
		}
	}
}

void TMapObjBall::touchWall(JGeometry::TVec3<f32>* pos,
                            TBGWallCheckRecord* record)
{
	if (!(mLiveFlag & 0x80) && !isActorType(0x400000D0)) {
		JGeometry::TVec3<f32> v = mVelocity;
		f32 mag
		    = JGeometry::TUtil<f32>::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
		mVelocity.y = unk184 * mag + mVelocity.y;
	}
	for (int i = 0; i < record->mResultWallsNum; i++) {
		TBGCheckData* wall      = record->mResultWalls[i];
		JGeometry::TVec3<f32> v = mVelocity;
		f32 dot = v.x * wall->mNormal.x + v.y * wall->mNormal.y
		          + v.z * wall->mNormal.z;
		if (dot >= 0.0f)
			continue;
		f32 sd = pos->x * wall->mNormal.x + pos->y * wall->mNormal.y
		         + pos->z * wall->mNormal.z + wall->mPlaneDistance;
		pos->x = (mBodyRadius - sd) * wall->mNormal.x + pos->x;
		pos->z = (mBodyRadius - sd) * wall->mNormal.z + pos->z;
		f32 bf   = -(1.0f + mMapObjData->mPhysical->unk4->unk8);
		f32 bd   = dot * bf;
		mVelocity.x = bd * wall->mNormal.x + mVelocity.x;
		mVelocity.z = bd * wall->mNormal.z + mVelocity.z;
		if (isActorType(0x400000D0)) {
			JGeometry::TVec3<f32> v2 = mVelocity;
			f32 mag2                 = JGeometry::TUtil<f32>::sqrt(
                v2.x * v2.x + v2.y * v2.y + v2.z * v2.z);
			f32 absMag = fabsf(mag2);
			if (mScaling.y >= 5.0f) {
				if (gpMSound->gateCheck(0x308A)) {
					MSoundSESystem::MSoundSE::startSoundActorWithInfo(
					    0x308A, (Vec*)&mPosition, nullptr, absMag, 0, 0,
					    nullptr, 0, 4);
				}
			} else {
				if (gpMSound->gateCheck(0x308B)) {
					MSoundSESystem::MSoundSE::startSoundActorWithInfo(
					    0x308B, (Vec*)&mPosition, nullptr, absMag, 0, 0,
					    nullptr, 0, 4);
				}
			}
		} else {
			u32 soundId = mMapObjData->mSound->unk4->unk0[4];
			if (gpMSound->gateCheck(soundId)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    soundId, (Vec*)&mPosition, (Vec*)&mVelocity, 0.0f, 0, 0,
				    nullptr, 0, 4);
			}
		}
	}
}

void TMapObjBall::touchPollution()
{
	kill();
}

void TMapObjBall::touchWaterSurface()
{
	kill();
}

TBigWatermelon::TBigWatermelon(const char* name)
    : TMapObjBall(name)
{
	mWaterEmitInfo = nullptr;
	mItemCount     = 0;
	unk1A0         = 0.0f;
}

u32 TResetFruit::getLivingTime() const
{
	return mFruitLivingTime;
}

void TResetFruit::killByTimer(int timer)
{
	mLifeTimer = timer;
	unkF8 |= 0x40000;
	mState = 0xB;
}

void TBigWatermelon::checkWallCollision(JGeometry::TVec3<f32>* pos)
{
	TMapObjGeneral::checkWallCollision(pos);
}

void TBigWatermelon::touchGround(JGeometry::TVec3<f32>* pos)
{
	TMapObjBall::touchGround(pos);
}

void TBigWatermelon::touchWall(JGeometry::TVec3<f32>* pos,
                               TBGWallCheckRecord* record)
{
	TMapObjGeneral::touchWall(pos, record);
}

void TBigWatermelon::loadAfter()
{
	TMapObjGeneral::loadAfter();
	JDrama::TActor* shine
	    = JDrama::TNameRefGen::search<JDrama::TActor>("シャイン（お化けスイカ用）");
	shine->mPosition.x = -4659.0f;
	shine->mPosition.y = 460.0f;
	shine->mPosition.z = 13620.0f;
}

void TBigWatermelon::initMapObj()
{
	TMapObjBall::initMapObj();
	if (!gParticleFlagLoaded[0x5d]) {
		gpResourceManager->load("/scene/mapObj/watermelon_bomb.jpa", 0x5d);
		gParticleFlagLoaded[0x5d] = 1;
	}
	if (!gParticleFlagLoaded[0x5e]) {
		gpResourceManager->load("/scene/mapObj/watermelon_bomb_a.jpa", 0x5e);
		gParticleFlagLoaded[0x5e] = 1;
	}
	if (!gParticleFlagLoaded[0x5f]) {
		gpResourceManager->load("/scene/mapObj/watermelon_bomb_b.jpa", 0x5f);
		gParticleFlagLoaded[0x5f] = 1;
	}
	if (!gParticleFlagLoaded[0x6b]) {
		gpResourceManager->load("/scene/mapObj/watermelon_shrink_a.jpa", 0x6b);
		gParticleFlagLoaded[0x6b] = 1;
	}
	if (!gParticleFlagLoaded[0x6c]) {
		gpResourceManager->load("/scene/mapObj/watermelon_shrink_b.jpa", 0x6c);
		gParticleFlagLoaded[0x6c] = 1;
	}
	mWaterEmitInfo = new TWaterEmitInfo("/watermelon.prm");
}

void TBigWatermelon::kill()
{
	emitAndScale(0x5D, 0, &mPosition);
	emitAndScale(0x5E, 0, &mPosition);
	emitAndScale(0x5F, 0, &mPosition);
	JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
	emitAndScale(0x6B, 0, &mPosition, scale);
	emitAndScale(0x6C, 0, &mPosition, scale);
	mWaterEmitInfo->mPos.value = mPosition;
	gpModelWaterManager->emitRequest(*mWaterEmitInfo);
	if (gpMSound->gateCheck(0x38A3)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x38A3, (Vec*)&mPosition, 0,
		                                          nullptr, 0, 4);
	}
	if (mItemCount < 10) {
		TMapObjBase* obj = gpItemManager->makeObjAppear(
		    mPosition.x, mPosition.y, mPosition.z, 0x2000000E, true);
		if (obj != nullptr) {
			obj->mVelocity.x = 0.0f;
			obj->mVelocity.y = 25.0f;
			obj->mVelocity.z = 0.0f;
			obj->mLiveFlag &= ~0x10;
			mItemCount = mItemCount + 1;
		}
	}
	TMapObjGeneral::kill();
}

BOOL TBigWatermelon::receiveMessage(THitActor* sender, u32 message)
{
	if (sender->isActorType(0x80000001)) {
		boundByActor(sender);
		return 1;
	}
	if (TMapObjGeneral::receiveMessage(sender, message))
		return 1;
	if (message == 4 && (unkF8 & 0x100000)) {
		hold((TTakeActor*)sender);
		return 1;
	}
	if (sender->isActorType(0x80000001) && !isActorType(0x400000D0)
	    && message != 4) {
		kicked();
		return 1;
	}
	return 0;
}

void TBigWatermelon::appearing()
{
	TMapObjGeneral::appearing();
	Mtx* m = getModel()->mNodeMatrices;
	calcRootMatrix();
	getModel()->calc();
	(*m)[1][3] = mBodyRadius * (mScaling.y / mInitialScaling.y) + mPosition.y;
	mScaledBodyRadius = 50.0f * mScaling.x;
	mDamageRadius     = 50.0f * mScaling.x;
	calcEntryRadius();
	if (isState(1)) {
		mActorType    = 0x400000D0;
		mAttackRadius = 50.0f * mScaling.x;
		calcEntryRadius();
	} else {
		mActorType    = 0x400000DB;
		mAttackRadius = 0.0f;
		calcEntryRadius();
	}
}

void TBigWatermelon::rebound(JGeometry::TVec3<f32>* pos)
{
	if (isState(0xC)) {
		kill();
		*pos = mPosition;
		return;
	}
	calcReflectingVelocity(mGroundPlane,
	                       mMapObjData->mPhysical->unk4->unk4, &mVelocity);
	pos->y = mGroundHeight;
	mLiveFlag |= 0x80;
	if (isActorType(0x400000D0)) {
		if (mScaling.y >= 5.0f) {
			f32 vol = __fabsf(mGroundPlane->mNormal.y);
			if (gpMSound->gateCheck(0x3889)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x3889, (Vec*)&mPosition, nullptr, vol, 0, 0, nullptr, 0, 4);
			}
		} else {
			f32 vol = __fabsf(mGroundPlane->mNormal.y);
			if (gpMSound->gateCheck(0x388C)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x388C, (Vec*)&mPosition, nullptr, vol, 0, 0, nullptr, 0, 4);
			}
		}
	} else {
		u32 soundId = mMapObjData->mSound->unk4->unk0[4];
		if (gpMSound->gateCheck(soundId)) {
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(
			    soundId, (Vec*)&mPosition, (Vec*)&mVelocity, 0.0f, 0, 0,
			    nullptr, 0, 4);
		}
	}
	if (isState(0xB)) {
		mState = 0xC;
	}
}

void TBigWatermelon::touchActor(THitActor* actor)
{
	if (isState(2))
		return;
	if (!isState(1)) {
		JGeometry::TVec3<f32> v = mVelocity;
		if (v.y < 0.0f) {
			kill();
			return;
		}
	}
	if (actor->isActorType(0x80000001)) {
		f32 dx   = mPosition.x - actor->mPosition.x;
		f32 dy   = mPosition.y - actor->mPosition.y;
		f32 dz   = mPosition.z - actor->mPosition.z;
		f32 dx2  = dx * dx;
		f32 dy2  = dy * dy;
		f32 dz2  = dz * dz;
		f32 sq   = dx2 + dy2;
		sq       = dz2 + sq;
		if (sq > 0.0f)
			sq = sq * JGeometry::TUtil<f32>::inv_sqrt(sq);
		if (sq < 0.6f * mBodyRadius) {
			kill();
			return;
		}
	}
	if (actor->isActorType(0x10000015)
	    && ((TPoiHana*)actor)->isMoving()) {
		f32 thresh = mMapObjData->mPhysical->unk4->unkC;
		if (fabsf(mVelocity.y) < thresh) {
			mVelocity.y = mVelocity.y + 30.0f;
			mState      = 0xB;
		}
		return;
	}
	if (unk194 != 0)
		return;
	if (isState(6))
		return;
	if (isHideObj(actor))
		return;
	if (actor->isActorType(0x08000083))
		return;
	if (actor->isActorType(0x400000CA))
		return;
	if (actor->isActorType(0x400000CC))
		return;
	if (actor->isActorType(0x80000001) && !isActorType(0x400000D0)
	    && *gpMarioSpeedY != 0.0f) {
		kicked();
	} else {
		boundByActor(actor);
	}
}

void TBigWatermelon::control()
{
	TMapObjGeneral::control();
	if (unk194 != 0) {
		unk194 = unk194 - 1;
	}
	if (isState(6)) {
		Mtx tmp;
		PSMTXCopy(mHolder->getTakingMtx(), tmp);
		tmp[1][3] = tmp[1][3] + unk190;
		PSMTXCopy(tmp, getModel()->mNodeMatrices[0]);
	} else {
		JGeometry::TVec3<f32> v = mVelocity;
		f32 sq = v.x * v.x + v.y * v.y + v.z * v.z;
		if (sq > 0.0000038146973f || mGroundPlane->mActor != nullptr) {
			calcCurrentMtx();
		}
	}
	switch (mState) {
	case 1: {
		if (mLiveFlag & 0x10) {
			mLiveFlag &= ~0x10;
		}
		f32 thresh           = mGroundHeight + 200.0f;
		TLiveActor* gpActor  = (TLiveActor*)mGroundPlane->mActor;
		if (mPosition.y >= thresh)
			break;
		if (gpActor == nullptr)
			break;
		if (!gpActor->isActorType(0x400000CD)
		    && !gpActor->isActorType(0x400000CD))
			break;
		f32 prev = unk1A0;
		unk1A0   = SMS_GetSandRiseUpRatio(this);
		if (unk1A0 <= 0.05f)
			break;
		if (unk1A0 <= prev)
			break;
		mVelocity.y = mVelocity.y + 20.0f;
	} break;
	case 2:
		break;
	case 0xA:
		break;
	case 0xD: {
		if (mLifeTimer <= 0) {
			JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
			emitAndScale(0x6B, 0, &mPosition, scale);
			emitAndScale(0x6C, 0, &mPosition, scale);
			mLifeTimer = 30;
		}
		if (animIsFinished()) {
			makeObjDead();
		}
	} break;
	default:
		break;
	}
}

void TBigWatermelon::startEvent()
{
	if (strcmp(getName(), "スイカ（大）") == 0) {
		mPosition.x = -4660.0f;
		mPosition.y = 1300.0f;
		mPosition.z = 13600.0f;
		unkF8 &= ~0x100;
		mLiveFlag |= 0x10;
		mVelocity.x = 0.0f;
		mVelocity.y = 0.0f;
		mVelocity.z = 0.0f;
		mLiveFlag |= 0x10;
		startAnim(7);
		TMarDirector* director = gpMarDirector;
		JDrama::TFlagT<u16> flagT;
		flagT.set(0);
		director->fireStartDemoCamera("スイカコールカメラ", &mPosition, -1,
		                              0.0f, true, nullptr, 0, nullptr, flagT);
		gpItemManager->makeShineAppearWithDemoOffset(
		    "シャイン（お化けスイカ用）", "スイカシャインカメラ", 0.0f,
		    0.0f, 0.0f);
		mLifeTimer = 0x17C;
		mState = 0xD;
	} else {
		for (int i = 0; i < 10; i++) {
			TMapObjBase* obj = gpItemManager->makeObjAppear(
			    gpMarioPos->x, gpMarioPos->y, gpMarioPos->z, 0x2000000E, true);
			if (obj != nullptr) {
				obj->mVelocity.set((MsRandF() - 0.5f) * 20.0f,
				                   MsRandF() * 20.0f + 20.0f,
				                   (MsRandF() - 0.5f) * 20.0f);
				obj->mLiveFlag &= ~0x10;
				*(u32*)((char*)obj + 0x14C) = 0x3C0;
			}
		}
		makeObjDead();
	}
}

void TBigWatermelon::touchWaterSurface()
{
	emitColumnWater();
	if (gpMSound->gateCheck(0x3875)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x3875, (Vec*)&mPosition, 0,
		                                          nullptr, 0, 4);
	}
	kill();
}
