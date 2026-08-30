#define JGEOMETRY_ROTATION3_IDENTITY33_OUT_OF_LINE
#include <Enemy/Popo.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/Graph.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DNode.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JGadget/std-list.hpp>
#include <JSystem/JMath.hpp>
#include <M3DUtil/MActor.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionData.hpp>
#include <Map/MapData.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/ModelWaterManager.hpp>
#include <Player/Watergun.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <System/Particles.hpp>
#include <dolphin/mtx.h>
#include <math.h>

#undef JGEOMETRY_ROTATION3_IDENTITY33_OUT_OF_LINE

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

static int PopoNonScaleCallback(J3DNode*, int);
static int PopoPossessedCallback(J3DNode*, int);
static int PopoRollCallback(J3DNode*, int);

static TPopo* gpCurPopo;

u8 TPopo::mRollSw          = 1;
u8 TPopo::mTriggerSw       = 1;
f32 TPopo::mTestAng_x      = 90.0f;
f32 TPopo::mTestAng_y      = 90.0f;
f32 TPopo::mNozzleOffsetZ  = -15.0f;
u8 TPopo::mCenterJntIndex  = 1;
u8 TPopo::mMouthJntIndex   = 2;
u8 TPopo::mRLegJntIndex    = 5;
u8 TPopo::mLLegJntIndex    = 11;
u8 TPopo::mRHandJntIndex   = 7;
u8 TPopo::mLHandJntIndex   = 9;
f32 TPopo::mTestBodyScale  = 35.0f;
u8 TPopo::mBrkFlag         = 1;
f32 TPopo::mColOffsetY     = 20.0f;
f32 TPopo::mColMinVal      = 0.6f;
u8 TPopo::mLevelShootSw    = 1;
f32 TPopo::mTestAng_z;
u8 TPopo::mExplosionSw;

static const char* popo_bastable[] = {
	"/scene/popo/bas/popo_chase.bas",
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	"/scene/popo/bas/popo_jump.bas",
	"/scene/popo/bas/popo_wait.bas",
};

static inline f32 callMsWrap(f32 t, f32 l, f32 r)
{
	return MsWrap<f32>(t, l, r);
}

static inline TPopo* popo(TSpineBase<TLiveActor>* spine)
{
	return (TPopo*)spine->getBody();
}

static inline void startPopoSound(u32 sound_id,
                                  const JGeometry::TVec3<f32>& pos)
{
	if (gpMSound->gateCheck(sound_id))
		MSoundSESystem::MSoundSE::startSoundActor(sound_id, &pos, 0, nullptr,
		                                          0, 4);
}

static inline void copyMtxTrans(JGeometry::TVec3<f32>& dst, MtxPtr mtx)
{
	dst.x = mtx[0][3];
	dst.y = mtx[1][3];
	dst.z = mtx[2][3];
}

DEFINE_NERVE(TNervePopoPossessedNozzle, TLiveActor)
{
	TPopo* self = popo(spine);

	if (spine->getTime() == 0) {
		TPopoManager* manager = (TPopoManager*)self->getManager();
		if (!manager->unk60) {
			spine->pushAfterCurrent(&TNerveWalkerGraphWander::theNerve());
			return TRUE;
		}

		manager->unk60 = 0;
		self->possessedIn();
	}

	if (self->checkCurAnmEnd(0)) {
		if (self->unsetUnk165()) {
			self->setBckAnm(3);
			MActor* actor = self->mMActor;
			actor->setFrameRate(SMSGetAnmFrameRate(), 3);
		} else {
			self->setBckAnm(4);
			self->mMActor->getFrameCtrl(3)->setFrame(0.0f);
			self->mMActor->setFrameRate(0.0f, 3);
		}
	}

	if (self->checkTrigger()) {
		spine->pushAfterCurrent(&TNervePopoFly::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNervePopoAttack, TLiveActor)
{
	TPopo* self = popo(spine);
	if (spine->getTime() == 0)
		self->setGoalPathMario();

	if (!self->isAirborne()) {
		if (!((TPopoManager*)self->mManager)->unk60)
			return TRUE;
		bool onYoshi;
		if (gpMarioOriginal->mState & 2)
			onYoshi = true;
		else
			onYoshi = false;
		if (onYoshi)
			return TRUE;
		if (fabsf(gpMarioPos->y - self->mPosition.y)
		    > self->getSaveParam2()->getSLGiveUpHeight())
			return TRUE;
		if (self->isResignationAttack())
			return TRUE;
	}

	self->walkBehavior(0, 1.0f);
	return FALSE;
}

DEFINE_NERVE(TNervePopoFly, TLiveActor)
{
	TPopo* self = popo(spine);
	if (spine->getTime() == 0) {
		self->setBckAnm(2);

		TWaterGun* gun = (TWaterGun*)SMS_GetMarioWaterGun();
		MtxPtr emit    = gun->getEmitMtx(0);
		TPopoSaveLoadParams* params = self->mPopoParams;
		f32 speed      = params->mSLReleaseSpeed.get()
		            * (self->unk198
		               / params->mSLWaterScaleMax.get());

		JGeometry::TVec3<f32> velocity;
		velocity.x      = speed * emit[0][0];
		velocity.y      = speed * emit[1][0];
		velocity.z      = speed * emit[2][0];
		self->mVelocity = velocity;
		self->onLiveFlag(LIVE_FLAG_AIRBORNE);

		if (self->unk1B4) {
			((TPopoManager*)self->mManager)->unk60 = 1;
			self->unk1B4                         = 0;
		}

		f32 rotation;
		if (velocity.z == 0.0f) {
			if (velocity.x >= 0.0f)
				rotation = 90.0f;
			else
				rotation = -90.0f;
		} else if (velocity.z >= 0.0f) {
			rotation = (360.0f / 65536.0f) * matan(velocity.z, velocity.x);
		} else {
			f32 angle = (360.0f / 65536.0f)
			            * matan(-velocity.z, velocity.x);
			rotation = 180.0f - angle;
		}

		self->mRotation.set(0.0f, callMsWrap(rotation, 0.0f, 360.0f),
		                    0.0f);

		if (TPopo::mExplosionSw)
			self->offHitFlag(HIT_FLAG_NO_COLLISION);
	} else if (!self->isAirborne()) {
		spine->pushAfterCurrent(&TNervePopoExplosion::theNerve());
		return TRUE;
	}

	if (spine->getTime() > 5) {
		self->offHitFlag(HIT_FLAG_NO_COLLISION);
		self->mCollision->offHitFlag(HIT_FLAG_NO_COLLISION);
	}

	self->flyBehavior();
	return FALSE;
}

DEFINE_NERVE(TNervePopoExplosion, TLiveActor)
{
	TPopo* self = popo(spine);
	if (spine->getTime() == 0) {
		JGeometry::TVec3<f32> zero(0.0f, 0.0f, 0.0f);
		self->mVelocity = zero;
		self->mMActor->setFrameRate(0.0f, 0);

		if (self->unk1B4) {
			((TPopoManager*)self->mManager)->unk60 = 1;
			self->unk1B4                         = 0;
		}

		self->onHitFlag(HIT_FLAG_NO_COLLISION);
		self->onLiveFlag(LIVE_FLAG_UNK8);

		MtxPtr centerMtx
		    = self->mMActor->getModel()->getAnmMtx(TPopo::mCenterJntIndex);
		self->mCallbackPos.set(centerMtx[0][3], centerMtx[1][3],
		                       centerMtx[2][3]);
		gpMarioParticleManager->emit(0xa1, &self->mCallbackPos, 0, nullptr);
		gpMarioParticleManager->emit(0xa2, &self->mCallbackPos, 0, nullptr);
	}

	if (spine->getTime() > self->mPopoParams->mSLExplosionEmitTime.get()) {
		self->onLiveFlag(LIVE_FLAG_DEAD);
		self->onLiveFlag(LIVE_FLAG_UNK8);
		self->offLiveFlag(LIVE_FLAG_HIDDEN);
		self->offLiveFlag(LIVE_FLAG_UNK10000);
		self->mHolder = nullptr;
		self->stopAnmSound();
		spine->reset();
		spine->setNext(&TNerveSmallEnemyDie::theNerve());
		spine->pushAfterCurrent(spine->getDefault());
		return TRUE;
	}

	self->explosion();
	return FALSE;
}

DEFINE_NERVE(TNervePopoWait, TLiveActor)
{
	TPopo* self = popo(spine);
	if (spine->getTime() == 0) {
		self->onLiveFlag(LIVE_FLAG_UNK10);
		self->setBckAnm(6);
		self->setGoalPathMario();
	}

	self->walkToCurPathNode(0.0f, self->mTurnSpeed, 0.0f);
	return FALSE;
}

DEFINE_NERVE(TNervePopoThrown, TLiveActor)
{
	TPopo* self = popo(spine);
	if (spine->getTime() > 30 && !self->isAirborne()) {
		spine->pushAfterCurrent(&TNerveWalkerGraphWander::theNerve());
		return TRUE;
	}

	return FALSE;
}

const char** TPopo::getBasNameTable() const { return popo_bastable; }

void TPopo::thrownByChorobei()
{
	mSpine->initWith(&TNervePopoThrown::theNerve());
}

void TPopo::possessedIn()
{
	mMActor = getActorKeeper()->getMActor("popoL.bmd");
	setBckAnm(3);
	mMActor->setBtpFromIndex(0);
	mMActor->setFrameRate(0.0f, 3);
	if (!mExplosionSw)
		onHitFlag(HIT_FLAG_NO_COLLISION);
	mMActor->setBrkFromIndex(0);
	mMActor->getFrameCtrl(5)->setFrame(0.0f);
	unk1A0 = 30.0f;
	mMActor->setFrameRate(0.0f, 5);
	offLiveFlag(LIVE_FLAG_UNK10);
	unk1B8 = 90.0f;
	unk1B4 = 1;
	startPopoSound(0x2861, mPosition);
	unk1CC = 0;
	unk1CD = 0;
}

void TPopo::explosion()
{
	if (unk198 > 1.0f)
		unk198 *= 0.9f;

	TPopoManager* manager = (TPopoManager*)mManager;

	JGeometry::TVec3<f32> pos = mPosition;
	pos.y += 100.0f;

	if (mSpine->getTime() % 2 == 0) {
		JGeometry::TVec3<f32>* dirPtr
		    = &manager->mExplosionWaterEmitInfo->mDir.value;
		JGeometry::TVec3<f32> dir = *dirPtr;
		dir.y *= -1.0f;
		*dirPtr = dir;
	}

	s32* numPtr = &manager->mExplosionWaterEmitInfo->mNum.value;
	f32 num = (f32)*numPtr
	          * (unk198 / mPopoParams->mSLWaterScaleMax.get());
	if (num < 2.0f)
		num = 2.0f;
	*numPtr = (s32)num;
	manager->mExplosionWaterEmitInfo->mPos.value = pos;
	gpModelWaterManager->emitRequest(*manager->mExplosionWaterEmitInfo);
}

void TPopo::flyBehavior()
{
	unk19C++;
	if (unk19C > mPopoParams->mSLFlyLimitTime.get()) {
		unk19C = 0;
		mSpine->pushNerve(&TNervePopoExplosion::theNerve());
	}

	if (unk198 > 1.0f)
		unk198 *= 0.999f;

	JGeometry::TVec3<f32> pos;
	if (mLiveFlag & LIVE_FLAG_CLIPPED_OUT) {
		pos = mPosition;
	} else {
		MtxPtr mouthMtx = getModel()->getAnmMtx(mMouthJntIndex);
		copyMtxTrans(pos, mouthMtx);
	}

	TPopoManager* manager = (TPopoManager*)mManager;
	manager->mWaterEmitInfo->mPos.value = pos;
	gpModelWaterManager->emitRequest(*manager->mWaterEmitInfo);
	startPopoSound(0x20ce, mPosition);
}

bool TPopo::isCollidMove(THitActor* other)
{
	if (mSpine->getCurrentNerve() == &TNervePopoFly::theNerve()
	    && other->receiveMessage(this, HIT_MESSAGE_TRAMPLE))
		mSpine->pushNerve(&TNervePopoExplosion::theNerve());

	return false;
}

bool TPopo::isFindMario(float length)
{
	if (mSpine->getTime() > 100) {
		bool onYoshi;
		if (gpMarioOriginal->mState & 2)
			onYoshi = true;
		else
			onYoshi = false;
		if (!onYoshi) {
			TSmallEnemyParams* params = (TSmallEnemyParams*)getSaveParam();
			JGeometry::TVec3<f32> marioPos;
			marioPos.set(gpMarioPos->x, gpMarioPos->y, gpMarioPos->z);
			f32 searchLength = params->getSLSearchLength() * length;
			f32 searchAngle  = params->getSLSearchAngle() * length;
			f32 searchAware  = params->getSLSearchAware() * length;
			if (isInSight(marioPos, searchLength, searchAngle, searchAware))
				return true;
		}
	}

	return false;
}

bool TPopo::isHitValid(u32 message)
{
	if (message == HIT_MESSAGE_UNKB)
		return true;

	if (message <= HIT_MESSAGE_HIP_DROP)
		mSpine->pushNerve(&TNervePopoExplosion::theNerve());

	return false;
}

void TPopo::bind()
{
	for (int i = 0; i < mCollision->getColNum(); ++i) {
		THitActor* other = mCollision->getCollision(i);
		if (other->isActorTypeOf(ACTOR_TYPE_PLAYER))
			attackToMario();
		else
			behaveToHitOthers(other);
	}

	if (checkLiveFlag(LIVE_FLAG_UNK10))
		return;

	BOOL shouldCheckExplosion = FALSE;
	if (mSpine->getCurrentNerve() == &TNervePopoPossessedNozzle::theNerve()
	    && unk198 > 1.2f && mExplosionSw)
		shouldCheckExplosion = TRUE;
	else if (mSpine->getCurrentNerve() == &TNervePopoFly::theNerve())
		shouldCheckExplosion = TRUE;
	else {
		TLiveActor::bind();
		return;
	}

	mGroundHeight = gpMap->checkGround(mPosition.x, mPosition.y + mHeadHeight,
	                                   mPosition.z, &mGroundPlane);
	if (mPosition.y <= mGroundHeight + 30.0f
	    || (fabsf(mVelocity.x) < 1.0f && fabsf(mVelocity.z) < 1.0f))
		mSpine->pushNerve(&TNervePopoExplosion::theNerve());

	TBGWallCheckRecord record(mPosition.x, mPosition.y, mPosition.z,
	                          unk198 * (mBodyScale * mWallRadius), 1, 0);
	if (gpMap->isTouchedWallsAndMoveXZ(&record))
		mSpine->pushNerve(&TNervePopoExplosion::theNerve());
	else if (mSpine->getCurrentNerve() == &TNervePopoFly::theNerve())
		TLiveActor::bind();
}

void TPopo::forceKill()
{
	if (mGroundPlane->isIllegalData()
	    || (!mGroundPlane->isDeathPlane() && !mGroundPlane->isPool()
	        && !mGroundPlane->isWaterSurface())
	    || isAirborne() || checkLiveFlag(LIVE_FLAG_UNK10)) {
		if (gpMap->isInArea(mPosition.x, mPosition.z))
			return;
	}

	if (mSpine->getCurrentNerve() != &TNervePopoExplosion::theNerve()) {
		mSpine->reset();
		mSpine->setNext(&TNervePopoExplosion::theNerve());
		mSpine->pushAfterCurrent(mSpine->getDefault());
		mLiveFlag |= LIVE_FLAG_UNK20000;
		mHitPoints = 1;
	}
}

void TPopo::kill()
{
	if (unk1B4) {
		((TPopoManager*)mManager)->unk60 = 1;
		unk1B4                         = 0;
	}
	TSmallEnemy::kill();
}

void TPopo::calcRootMatrix()
{
	gpCurPopo = this;

	MtxPtr centerMtx = getModel()->getAnmMtx(mCenterJntIndex);
	copyMtxTrans(mCollision->mPosition, centerMtx);

	if (unk1B4) {
		unk190 = 0.8f * unk198 / mPopoParams->mSLWaterScaleMax.get();
		if (unk190 < mColMinVal)
			unk190 = mColMinVal;
		expandCollision();
		getModel()->setBaseScale(mScaling);

		TPosition3f rootMtx;
		if (mSpine->getCurrentNerve() == &TNervePopoFly::theNerve()) {
			rootMtx.identity33();
			rootMtx.setTrans(mPosition);
		} else {
			TWaterGun* gun = (TWaterGun*)SMS_GetMarioWaterGun();
			MtxPtr emit    = gun->getEmitMtx(0);
			PSMTXCopy(emit, rootMtx);

			JGeometry::TVec3<f32> axis;
			axis.set(rootMtx.at(0, 0), rootMtx.at(1, 0),
			         rootMtx.at(2, 0));
			f32 xLength = axis.length();
			axis.set(rootMtx.at(0, 1), rootMtx.at(1, 1),
			         rootMtx.at(2, 1));
			f32 yLength = axis.length();
			axis.set(rootMtx.at(0, 2), rootMtx.at(1, 2),
			         rootMtx.at(2, 2));
			f32 zLength = axis.length();

			if (zLength != 0.0f) {
				rootMtx.ref(0, 0) /= xLength;
				rootMtx.ref(1, 0) /= xLength;
				rootMtx.ref(2, 0) /= xLength;
			}
			if (xLength != 0.0f) {
				rootMtx.ref(0, 1) /= yLength;
				rootMtx.ref(1, 1) /= yLength;
				rootMtx.ref(2, 1) /= yLength;
			}
			if (yLength != 0.0f) {
				rootMtx.ref(0, 2) /= zLength;
				rootMtx.ref(1, 2) /= zLength;
				rootMtx.ref(2, 2) /= zLength;
			}
		}

		TPosition3f nozzleOffset;
		nozzleOffset.identity33();
		nozzleOffset.ref(0, 3) = 7.0f * unk198 + mNozzleOffsetZ;
		nozzleOffset.ref(1, 3) = 0.0f;
		nozzleOffset.ref(2, 3) = 0.0f;
		PSMTXConcat(rootMtx, nozzleOffset, rootMtx);

		TPosition3f bodyOffset;
		bodyOffset.identity33();
		bodyOffset.ref(0, 3) = mTestBodyScale * unk198;
		bodyOffset.ref(1, 3) = 0.0f;
		bodyOffset.ref(2, 3) = 0.0f;
		PSMTXConcat(rootMtx, bodyOffset, bodyOffset);

		mPosition.x = bodyOffset.at(0, 3);
		mPosition.y = bodyOffset.at(1, 3) - mColOffsetY * unk198;
		mPosition.z = bodyOffset.at(2, 3);

		if (unk1BC[0]) {
			PSMTXCopy(centerMtx, unk200);
			unk200[0][3] = bodyOffset.at(0, 3);
			unk200[1][3] = bodyOffset.at(1, 3);
			unk200[2][3] = bodyOffset.at(2, 3);

			JPABaseEmitter* emitter = gpMarioParticleManager
			                              ->emitAndBindToMtxPtr(0x13D, unk200,
			                                                    1, this);
			if (emitter) {
				emitter->unk154.set(unk230);
				emitter->unk174.set(unk230);
			}
		}

		Mtx rot;
		MsMtxSetRotRPH(rot, mTestAng_x, mTestAng_y, mTestAng_z);
		PSMTXConcat(rootMtx, rot, rootMtx);
		PSMTXCopy(rootMtx, getModel()->getBaseTRMtx());
	} else {
		TSpineEnemy::calcRootMatrix();
	}

}

void TPopo::attackToMario()
{
	TPopoManager* manager = (TPopoManager*)mManager;

	if ((mSpine->getCurrentNerve() == &TNervePopoAttack::theNerve()
	        || mSpine->getCurrentNerve() == &TNervePopoWait::theNerve())
	    && manager->unk60) {
		mSpine->pushNerve(&TNervePopoPossessedNozzle::theNerve());
		return;
	}

	if (mSpine->getCurrentNerve() != &TNerveWalkerEscape::theNerve()
	    && mSpine->getCurrentNerve() != &TNerveWalkerGraphWander::theNerve())
		return;

	sendAttackMsgToMario();

	JGeometry::TVec3<f32> away;
	JGeometry::TVec3<f32> push;
	push.set(0.0f, 0.0f, 0.0f);
	away.set(mPosition.x - gpMarioPos->x, mPosition.y - gpMarioPos->y,
	         mPosition.z - gpMarioPos->z);
	MsVECNormalize(&away, &away);

	mVelocity.x = away.x;
	mVelocity.z = away.z;

	f32 pushSpeed = mBodyScale * mBodyRadius;
	away.scale(pushSpeed);
	push.add(away);
	mLinearVelocity = push;
}

void TPopo::walkBehavior(int graph_direction, float multiplier)
{
	if (!isAirborne()) {
		JGeometry::TVec3<f32> target = unk104.getPoint();
		JGeometry::TVec3<f32> dir(unk104.getPoint().x - mPosition.x, 0.0f,
		                              unk104.getPoint().z - mPosition.z);
		if (dir.x == 0.0f && dir.y == 0.0f && dir.z == 0.0f)
			dir.x += 1.0f;
		MsVECNormalize(&dir, &dir);

		f32 jumpSpeed     = mPopoParams->mSLMoveJumpSp.get();
		f32 targetDist    = mPopoParams->mSLMoveDist.get();
		f32 randomScale   = 1.0f;
		TMsRange<f32> randomOffset(-20.0f, 20.0f);
		if (mSpine->getCurrentNerve() == &TNervePopoAttack::theNerve()) {
			setBckAnm(0);
			jumpSpeed   = mPopoParams->mSLAttackJumpSp.get();
			targetDist  = mPopoParams->mSLAttackDist.get();
			randomScale = 10.0f;
		}

		target.x        = mPosition.x + dir.x * targetDist
		           + randomScale * randomOffset.rand();
		target.z = mPosition.z + dir.z * targetDist
		           + randomScale * randomOffset.rand();
		target.y = mPosition.y;

		f32 jumpRate = 1.0f;
		if (mSpine->getCurrentNerve() == &TNerveWalkerEscape::theNerve()) {
			jumpRate = 1.2f;
			setBckAnm(5);
		}

		mVelocity = calcVelocityToJumpToY(target, jumpSpeed * jumpRate,
		                                  getGravityY());
		mPosition.y += 2.0f;
		onLiveFlag(LIVE_FLAG_AIRBORNE);

		if (mSpine->getCurrentNerve()
		    == &TNerveWalkerGraphWander::theNerve()) {
			TPathNode node(target);
			unkF4  = node;
			unk104 = node;
			unk114.clear();
			setBckAnm(5);
		}
	} else {
		if (mVelocity.y > 1.5f)
			mPosition.y += 0.5f * mVelocity.y;
		if (mVelocity.y < -1.0f)
			mPosition.y += 0.2f * mVelocity.y;
	}

	unk1B8 += 1.0f;
	if (mSpine->getCurrentNerve() == &TNervePopoAttack::theNerve())
		unk1B8 += 2.0f;
	if (unk1B8 > 360.0f)
		unk1B8 -= 360.0f;
	if (!mRollSw)
		unk1B8 = 0.0f;

	if (mVelocity.y > 0.0f)
		walkToCurPathNode(0.0f, mTurnSpeed, 0.0f);
}

void TPopo::behaveToFindMario()
{
	TPopoManager* manager = (TPopoManager*)mManager;

	if (SMS_CheckMarioFlag(0x8000) && manager->unk60) {
		TWaterGun* gun = (TWaterGun*)SMS_GetMarioWaterGun();
		s32 currentNozzle = gun->mCurrentNozzle;
		if (currentNozzle == 0 && !gpMarioOriginal->onYoshi()) {
			setGoalPathMario();
			mSpine->pushAfterCurrent(&TNerveWalkerGraphWander::theNerve());
			mSpine->pushAfterCurrent(&TNervePopoAttack::theNerve());
			return;
		}
	}

	mSpine->pushAfterCurrent(&TNerveWalkerGraphWander::theNerve());
}

f32 TPopo::getGravityY() const
{
	f32 gravity = mGravity;

	if (mSpine->getCurrentNerve() == &TNerveWalkerGraphWander::theNerve()
	    || mSpine->getCurrentNerve() == &TNerveWalkerEscape::theNerve()
	    || mSpine->getCurrentNerve() == &TNerveWalkerAttack::theNerve())
		return mPopoParams->mSLMoveGravity.get();

	if (mSpine->getCurrentNerve() == &TNervePopoAttack::theNerve())
		gravity = mPopoParams->mSLAttackGravity.get();
	else if (mSpine->getCurrentNerve() == &TNervePopoFly::theNerve())
		gravity = mPopoParams->mSLFlyGravity.get();
	else if (mSpine->getCurrentNerve() == &TNervePopoThrown::theNerve())
		gravity = mPopoParams->mSLThrownGravity.get();

	return gravity;
}

void TPopo::behaveToWater(THitActor* water)
{
	if (mSpine->getCurrentNerve() == &TNervePopoExplosion::theNerve()
	    || mSpine->getCurrentNerve() == &TNervePopoFly::theNerve()
	    || mSpine->getCurrentNerve() == &TNerveSmallEnemyDie::theNerve())
		return;

	if (mSpine->getCurrentNerve() == &TNervePopoPossessedNozzle::theNerve()) {
		mSprayedByWaterCooldown = 0;
		return;
	}

	if (isAirborne()) {
		JGeometry::TVec3<f32> oldVelocity = mVelocity;
		JGeometry::TVec3<f32> away;
		away.set(mPosition.x - SMS_GetMarioPos().x, 0.0f,
		         mPosition.z - SMS_GetMarioPos().z);
		MsVECNormalize(&away, &away);
		away.scale(12.0f);
		away.y = -1.0f;
		away.add(oldVelocity);
		mVelocity = away;
		return;
	}

	if (mSpine->getCurrentNerve() != &TNerveSmallEnemyFreeze::theNerve())
		mSpine->pushNerve(&TNerveSmallEnemyFreeze::theNerve());
}

bool TPopo::checkTrigger()
{
	unk1BC[0] = 0;
	if (gpMarioOriginal->onYoshi()
	    || (s32)((TWaterGun*)SMS_GetMarioWaterGun())->mCurrentNozzle
	           != TWaterGun::Spray) {
		kill();
		return false;
	}

	SMS_SendMessageToMario(this, HIT_MESSAGE_UNK5);

	f32 maxScale = mPopoParams->mSLWaterScaleMax.get();
	s32 pressure = (s32)gpMarioOriginal->mGamePad->mCompSPos[3];
	if ((u8)pressure > 20) {
		unk1BC[0] = 1;
		MSound* sound  = gpMSound;
		f32 soundScale = unk198;
		if (sound->gateCheck(0x20C2)) {
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(
			    0x20C2, &mPosition, nullptr, soundScale, 0, 0, nullptr, 0, 4);
		}

		mSprayedByWaterCooldown = 0;
		unk165                  = true;
		f32 scale               = unk198;
		f32 pump                = (f32)(u8)pressure * mPopoParams->mSLPumpRate.get();
		unk198                  = scale + pump;
		if (unk198 > maxScale) {
			unk198 = maxScale;
			if (!mBrkFlag) {
				MActor* actor = mMActor;
				actor->setFrameRate(SMSGetAnmFrameRate(), 5);
			}
		}

		f32 brkMaxScale = mPopoParams->mSLWaterScaleMax.get();
		if (mBrkFlag)
			mMActor->getFrameCtrl(5)->setFrame(unk1A0 * unk198 / brkMaxScale);
	}

	if ((gpMarioOriginal->mGamePad->mEnabledFrameMeaning & 0x400)
	    || !mTriggerSw) {
		if (mLevelShootSw) {
			unk1CC = 1;
		} else if (unk198 >= maxScale) {
			unk1CC = 1;
		}
	}

	if (mLevelShootSw && unk198 < maxScale - 0.1f && unk198 > 1.0f)
		unk198 *= mPopoParams->mSLScaleRate.get();

	f32 levelLimit = mPopoParams->mSLLevelLimit.get();
	if ((u8)pressure < 20 && (unk1CC || unk198 > levelLimit)) {
		startPopoSound(0x28CD, mPosition);
		onHitFlag(HIT_FLAG_NO_COLLISION);
		mCollision->offHitFlag(HIT_FLAG_NO_COLLISION);
		return true;
	}

	mScaledBodyRadius = (8.0f * unk198 + 8.0f) * (mBodyScale * mBodyRadius);
	if (unk198 >= maxScale)
		mMActor->getFrameCtrl(3)->setFrame(5.0f);

	return false;
}

void TPopo::reset()
{
	gpCurPopo = this;
	TWalkerEnemy::reset();

	unk165 = false;
	unk1B4 = 0;
	unk198 = 1.0f;
	unk1B8 = 0.0f;
	unk19C = 0;

	mScaledBodyRadius = mBodyScale * mBodyRadius * 15.0f;
	unk190            = 0.2f;
	expandCollision();

	MActor* low = mMActorKeeper->getMActor("popoL.bmd");
	mMActor     = low;

	if (unk1A4) {
		onLiveFlag(LIVE_FLAG_UNK10);
		mSpine->initWith(&TNervePopoWait::theNerve());
		mPosition = mInitialPosition;
		offLiveFlag(LIVE_FLAG_UNK800);
	}

	mCollision->onHitFlag(HIT_FLAG_NO_COLLISION);

	unk18C = 0;
}

void TPopo::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 2);
	mMActor       = mMActorKeeper->createMActor("popoH.bmd", 3);
	mMActorKeeper->createMActor("popoL.bmd", 3);
}

void TPopo::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(flags, graphics);
	mCollision->THitActor::perform(flags, graphics);
}

void TPopo::init(TLiveManager* manager)
{
	TWalkerEnemy::init(manager);
	mActorType = 0x1000000d;

	if (mInstanceIndex == 0) {
		for (u8 i = 0; i < getModel()->getModelData()->getJointNum(); ++i) {
		}
	}

	unk150      = 0x11;
	mPopoParams = (TPopoSaveLoadParams*)getSaveParam();
	mSpine->initWith(&TNerveWalkerGraphWander::theNerve());
	onHitFlag(HIT_FLAG_UNK8000000);

	getMActor()->setJointCallback(mCenterJntIndex, PopoRollCallback);
	MActor* low = mMActorKeeper->getMActor("popoL.bmd");
	low->setJointCallback(mCenterJntIndex, PopoRollCallback);

	getMActor()->setJointCallback(mMouthJntIndex, PopoPossessedCallback);
	getMActor()->setJointCallback(mRLegJntIndex, PopoNonScaleCallback);
	getMActor()->setJointCallback(mLLegJntIndex, PopoNonScaleCallback);
	getMActor()->setJointCallback(mRHandJntIndex, PopoNonScaleCallback);
	getMActor()->setJointCallback(mLHandJntIndex, PopoNonScaleCallback);

	unk188     = 0.0f;
	mCollision = new TPopoCollision("ポポコリジョン");
	TIdxGroupObj* group
	    = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
	group->getChildren().push_back(mCollision);
	mCollision->initHitActor(0x1000000d, 2, -0x68000000, 80.0f, 80.0f,
	                         80.0f, 80.0f);
	mCollision->onHitFlag(HIT_FLAG_NO_COLLISION);
	mCollision->mOwner = this;
}

void TPopo::load(JSUMemoryInputStream& stream)
{
	TSmallEnemy::load(stream);
	mInitialPosition = mPosition;
	unk1A4           = 1;
	reset();
}

TPopo::TPopo(const char* name)
    : TWalkerEnemy(name)
{
	mPopoParams = nullptr;
	unk198      = 1.0f;
	unk19C      = 0;
	unk1A0      = 30.0f;
	unk1A4      = 0;
	unk1B4      = 0;
	unk1B8      = 0.0f;
	unk1CC      = 0;
	unk1CD      = 0;
	mCollision  = nullptr;
}

static int PopoNonScaleCallback(J3DNode* node, int timing)
{
	if (timing == 0) {
		TPopo* popo = gpCurPopo;
		if (popo
		    && (popo->mSpine->getCurrentNerve() == &TNervePopoFly::theNerve()
		        || popo->mSpine->getCurrentNerve()
		               == &TNervePopoExplosion::theNerve()
		        || popo->unk1B4)) {
			u16 jointIndex = ((J3DJoint*)node)->getJntNo();
			MtxPtr mtx
			    = gpCurPopo->getModel()->mNodeMatrices[jointIndex];
			Mtx scaleMtx;
			scaleMtx[0][3] = 0.0f;
			scaleMtx[1][3] = 0.0f;
			scaleMtx[2][3] = 0.0f;
			f32 scale = 0.9f * gpCurPopo->mBodyScale;
			scaleMtx[0][0] = scale;
			scaleMtx[0][1] = 0.0f;
			scaleMtx[0][2] = 0.0f;
			scaleMtx[1][0] = 0.0f;
			scaleMtx[1][1] = scale;
			scaleMtx[1][2] = 0.0f;
			scaleMtx[2][0] = 0.0f;
			scaleMtx[2][1] = 0.0f;
			scaleMtx[2][2] = scale;
			PSMTXConcat(mtx, scaleMtx, mtx);
			PSMTXConcat(J3DSys::mCurrentMtx, scaleMtx, J3DSys::mCurrentMtx);
		}
	}
	return 1;
}

static int PopoPossessedCallback(J3DNode* node, int timing)
{
	if (timing == 0) {
		TPopo* popo = gpCurPopo;
		if (!popo)
			return 1;

		bool shouldScale = false;
		if (popo->mSpine->getCurrentNerve()
		    == &TNervePopoFly::theNerve())
			shouldScale = true;
		else if (popo->mSpine->getCurrentNerve()
		         == &TNervePopoExplosion::theNerve())
			shouldScale = true;
		else if (popo->unk1B4)
			shouldScale = true;

		if (!shouldScale)
			return 1;

		f32 scale = popo->unk198;
		if (scale < 1.1f)
			return 1;

		J3DJoint* joint = (J3DJoint*)node;
		MtxPtr mtx = popo->getModel()->mNodeMatrices[joint->getJntNo()];
		Mtx scaleMtx;
		scaleMtx[0][0] = scale;
		scaleMtx[0][1] = 0.0f;
		scaleMtx[0][2] = 0.0f;
		scaleMtx[0][3] = 0.0f;
		scaleMtx[1][0] = 0.0f;
		scaleMtx[1][1] = scale;
		scaleMtx[1][2] = 0.0f;
		scaleMtx[1][3] = 0.0f;
		scaleMtx[2][0] = 0.0f;
		scaleMtx[2][1] = 0.0f;
		scaleMtx[2][2] = scale;
		scaleMtx[2][3] = 0.0f;
		PSMTXConcat(mtx, scaleMtx, mtx);
		PSMTXConcat(J3DSys::mCurrentMtx, scaleMtx,
		            J3DSys::mCurrentMtx);

		if (popo->unk1BC[0]) {
			PSMTXCopy(mtx, popo->unk1D0);
			Mtx rot;
			MsMtxSetRotRPH(rot, 0.0f, 270.0f, 0.0f);
			PSMTXConcat(popo->unk1D0, rot, popo->unk1D0);

			JGeometry::TVec3<f32> axis;
			axis.set(mtx[0][0], mtx[1][0], mtx[2][0]);
			popo->unk230.y = axis.length();
			axis.set(mtx[0][1], mtx[1][1], mtx[2][1]);
			popo->unk230.z = axis.length();
			axis.set(mtx[0][2], mtx[1][2], mtx[2][2]);
			popo->unk230.x = axis.length();

			JPABaseEmitter* emitter
			    = gpMarioParticleManager->emitAndBindToMtxPtr(
			        0x13C, popo->unk1D0, 1, popo);
			if (emitter) {
				emitter->unk154.set(popo->unk230);
				emitter->unk174.set(popo->unk230);
			}
		}
	}

	return 1;
}

static int PopoRollCallback(J3DNode* node, int timing)
{
	if (timing == 0) {

		TPopo* popo = gpCurPopo;
		if (!popo)
			return 1;

		J3DJoint* joint = (J3DJoint*)node;
		MtxPtr jointMtx = popo->getModel()->mNodeMatrices[joint->getJntNo()];

		Mtx scaleMtx;
		scaleMtx[0][0] = popo->mBodyScale;
		scaleMtx[0][1] = 0.0f;
		scaleMtx[0][2] = 0.0f;
		scaleMtx[0][3] = 0.0f;
		scaleMtx[1][0] = 0.0f;
		scaleMtx[1][1] = popo->mBodyScale;
		scaleMtx[1][2] = 0.0f;
		scaleMtx[1][3] = 0.0f;
		scaleMtx[2][0] = 0.0f;
		scaleMtx[2][1] = 0.0f;
		scaleMtx[2][2] = popo->mBodyScale;
		scaleMtx[2][3] = 0.0f;

		Mtx roll;
		if (popo->mSpine->getCurrentNerve() != &TNervePopoFly::theNerve()) {
			s32 phase = (s32)(popo->unk1B8 * 182.04445f);
			u16 idx   = (u16)phase >> jmaSinShift;
			f32 sin   = jmaSinTable[idx];
			f32 cos   = jmaCosTable[idx];

			roll[0][0] = 1.0f;
			roll[0][1] = 0.0f;
			roll[0][2] = 0.0f;
			roll[0][3] = 0.0f;
			roll[1][0] = 0.0f;
			roll[1][1] = cos;
			roll[1][2] = -sin;
			roll[1][3] = 0.0f;
			roll[2][0] = 0.0f;
			roll[2][1] = sin;
			roll[2][2] = cos;
			roll[2][3] = 0.0f;
		} else {
			u16 idx = (u16)0x8000 >> jmaSinShift;
			f32 sin = jmaSinTable[idx];
			f32 cos = jmaCosTable[idx];

			roll[0][0] = cos;
			roll[0][1] = 0.0f;
			roll[0][2] = sin;
			roll[0][3] = 0.0f;
			roll[1][0] = 0.0f;
			roll[1][1] = 1.0f;
			roll[1][2] = 0.0f;
			roll[1][3] = 0.0f;
			roll[2][0] = -sin;
			roll[2][1] = 0.0f;
			roll[2][2] = cos;
			roll[2][3] = 0.0f;
		}

		PSMTXConcat(jointMtx, roll, jointMtx);
		PSMTXConcat(jointMtx, scaleMtx, jointMtx);
		PSMTXConcat(J3DSys::mCurrentMtx, roll, J3DSys::mCurrentMtx);
		PSMTXConcat(J3DSys::mCurrentMtx, scaleMtx, J3DSys::mCurrentMtx);
	}
	return 1;
}

BOOL TPopoCollision::receiveMessage(THitActor* sender, u32 message)
{
	TPopo* owner = mOwner;

	bool canReceive;
	if (owner->mSpine->getCurrentNerve() == &TNervePopoFly::theNerve())
		canReceive = false;
	else
		canReceive = true;

	if (canReceive)
		return mOwner->receiveMessage(sender, message);

	return FALSE;
}

void TPopoManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		for (int i = 0; i < getActiveObjNum(); ++i) {
			TPopo* popo = (TPopo*)unk18[i];
			if (popo->unk1A4 && (popo->mLiveFlag & LIVE_FLAG_DEAD))
				popo->reset();
		}
	}

	TEnemyManager::perform(flags, graphics);
}

void TPopoManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "popoH.bmd", 0x10020000, 0 },
		{ "popoL.bmd", 0x10020000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TPopoManager::initSetEnemies()
{
	TPopo* popo = (TPopo*)unk18[0];
	TGraphWeb* graph = popo->getTracer()->getGraph();
	if (graph && graph->isDummy())
		return;
}

TSmallEnemy* TPopoManager::createEnemyInstance()
{
	return new TPopo("ポポ");
}

void TPopoManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38                = new TPopoSaveLoadParams("/enemy/popo.prm");
	mWaterEmitInfo       = new TWaterEmitInfo("/enemy/popowater.prm");
	mExplosionWaterEmitInfo = new TWaterEmitInfo("/enemy/popoexpwater.prm");
}

TPopoManager::TPopoManager(const char* name)
    : TSmallEnemyManager(name)
{
	unk60                   = 1;
	mWaterEmitInfo          = nullptr;
	mExplosionWaterEmitInfo = nullptr;
	gpCurPopo               = nullptr;
	unk5C                   = 0;
}

TPopoSaveLoadParams::TPopoSaveLoadParams(const char* path)
    : TWalkerEnemyParams(path)
    , PARAM_INIT(mSLMoveDist, 100.0f)
    , PARAM_INIT(mSLMoveGravity, 0.1f)
    , PARAM_INIT(mSLMoveJumpSp, 10.0f)
    , PARAM_INIT(mSLAttackDist, 100.0f)
    , PARAM_INIT(mSLAttackGravity, 0.1f)
    , PARAM_INIT(mSLAttackJumpSp, 10.0f)
    , PARAM_INIT(mSLReleaseSpeed, 10.0f)
    , PARAM_INIT(mSLFlyGravity, 0.0f)
    , PARAM_INIT(mSLFlyLimitTime, 300)
    , PARAM_INIT(mSLExplosionEmitTime, 60)
    , PARAM_INIT(mSLWaterScaleMax, 2.0f)
    , PARAM_INIT(mSLThrownGravity, 0.5f)
    , PARAM_INIT(mSLPumpRate, 0.0001f)
    , PARAM_INIT(mSLLevelLimit, 1.2f)
    , PARAM_INIT(mSLScaleRate, 0.99f)
{
	load(mPrmPath);
}
