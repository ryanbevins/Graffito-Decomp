#include <Enemy/TobiPuku.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/EffectObj.hpp>
#include <Enemy/Graph.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DNode.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionData.hpp>
#include <Map/MapData.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MoveBG/MapObjBlock.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/Spine.hpp>
#include <System/Application.hpp>
#include <Strategic/Strategy.hpp>
#include <System/Particles.hpp>
#include <math.h>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

static const char* pukupuku_bastable[] = {
	nullptr,
	"/scene/pukupuku/bas/pukupuku_death.bas",
	"/scene/pukupuku/bas/pukupuku_down_air.bas",
	"/scene/pukupuku/bas/pukupuku_down_land.bas",
	nullptr,
	"/scene/pukupuku/bas/pukupuku_fall_end_land.bas",
	nullptr,
	nullptr,
	"/scene/pukupuku/bas/pukupuku_pitipiti.bas",
	"/scene/pukupuku/bas/pukupuku_swim.bas",
	nullptr,
};

static const char* moepuku_bastable[] = {
	nullptr,
	nullptr,
	"/scene/moepuku/bas/moepuku_down_air.bas",
	"/scene/moepuku/bas/moepuku_down_land.bas",
	nullptr,
	"/scene/moepuku/bas/moepuku_fall_end_land.bas",
	nullptr,
	nullptr,
	"/scene/moepuku/bas/moepuku_pitipiti.bas",
	nullptr,
	nullptr,
};

static TTobiPuku* gpCurTobiPuku;

f32 TTobiPuku::mLandAngle      = 90.0f;
u8 TTobiPuku::mBoundSw         = 1;
f32 TTobiPuku::mBoundVelocityY = 0.8f;
u8 TTobiPuku::mReturnLaunchSw  = 1;

static inline TTobiPuku* tobiPuku(TSpineBase<TLiveActor>* spine)
{
	return (TTobiPuku*)spine->getBody();
}

static inline void startTobiPukuSound(u32 sound_id,
                                      const JGeometry::TVec3<f32>& pos)
{
	if (gpMSound->gateCheck(sound_id))
		MSoundSESystem::MSoundSE::startSoundActor(sound_id, &pos, 0, nullptr,
		                                          0, 4);
}

static inline void copyMtxTrans(JGeometry::TVec3<f32>& dst, MtxPtr mtx)
{
	f32 z = mtx[2][3];
	f32 y = mtx[1][3];
	f32 x = mtx[0][3];
	dst.x = x;
	dst.y = y;
	dst.z = z;
}

static inline void emitPichiEffect(TTobiPuku* self)
{
	MtxPtr mtx = self->mMActor->unk4->mNodeMatrices[1];
	copyMtxTrans(self->unk1A0, mtx);
	gpMarioParticleManager->emitAndBindToPosPtr(0x177, &self->unk1A0, 1,
	                                            self);
}

static inline void calcTobiPukuRootMatrix(TTobiPuku* self)
{
	gpCurTobiPuku = self;
	self->TSpineEnemy::calcRootMatrix();

	if (self->mRotation.x != 0.0f && !self->isEaten()) {
		f32 rotX = self->mRotation.x;
		f32 y    = self->mPosition.y + 70.0f * rotX / TTobiPuku::mLandAngle;
		MsMtxSetXYZRPH(self->getModel()->getBaseTRMtx(), self->mPosition.x, y,
		                self->mPosition.z, rotX, self->mRotation.y,
		                self->mRotation.z);
	}

	if (self->isPichiEffect())
		emitPichiEffect(self);
}

BOOL TNerveTobiPukuSwimWander::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);

	if (spine->getTime() == 0) {
		self->unk1E0 = self->mPosition.y;
		self->setSwimAnm();
		self->initialGraphNode();
		self->onLiveFlag(LIVE_FLAG_UNK10);
	}

	if (self->isReachedToGoalXZ()) {
		self->goToRandomNextGraphNode();
		self->walkBehavior(0, 1.5f);
	}

	return FALSE;
}

BOOL TNerveTobiPukuReturnLaunch::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);

	if (spine->getTime() == 0) {
		self->unkF4  = TPathNode(self->mLaunchPad->mPosition);
		self->unk104 = TPathNode(self->mLaunchPad->mPosition);
		self->unk114.clear();
		self->setSwimAnm();
		self->unk1E0 = self->mPosition.y;
	}

	self->swimEffect();
	if (self->isReachedToGoalXZ()) {
		spine->pushAfterCurrent(&TNerveTobiPukuPrepareFly::theNerve());
		return TRUE;
	}

	JGeometry::TVec3<f32> toPad = self->mLaunchPad->mPosition;
	toPad.sub(self->mPosition);
	toPad.y = 0.0f;
	MsVECNormalize((Vec*)&toPad, (Vec*)&toPad);

	self->mLaunchVelocity.x *= 0.99f;
	self->mLaunchVelocity.z *= 0.99f;
	self->mPosition.x += toPad.x * self->mMarchSpeed - self->mLaunchVelocity.x;
	self->mPosition.z += toPad.z * self->mMarchSpeed - self->mLaunchVelocity.z;

	self->unk1EC += 1.0f;
	if (self->unk1EC > 180.0f)
		self->unk1EC = 180.0f;
	else if (self->unk1EC < 0.0f)
		self->unk1EC = 0.0f;

	return FALSE;
}

BOOL TNerveTobiPukuPrepareFly::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0) {
		f32 rot = self->unk1B4;
		while (rot >= 360.0f)
			rot -= 360.0f;
		while (rot < 0.0f)
			rot += 360.0f;
		self->unk1F0 = (rot - self->mRotation.x) / 60.0f;
	}

	self->mPosition.x += (self->mLaunchPad->mPosition.x - self->mPosition.x)
	                     * (1.0f / 60.0f);
	self->mPosition.y += (self->mLaunchPad->mPosition.y - self->mPosition.y)
	                     * (1.0f / 60.0f);
	self->mPosition.z += (self->mLaunchPad->mPosition.z - self->mPosition.z)
	                     * (1.0f / 60.0f);

	self->unk1EC -= 3.0f;
	if (self->unk1EC > 180.0f)
		self->unk1EC = 180.0f;
	else if (self->unk1EC < 0.0f)
		self->unk1EC = 0.0f;

	self->mRotation.x += self->unk1F0;

	if (spine->getTime() == 50.0f)
		self->setJumpStartAnm();
	if (spine->getTime() > 60.0f) {
		self->mLaunchPad->forceLaunch(self);
		self->reset();
	}

	return FALSE;
}

BOOL TNerveTobiPukuBound::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0) {
		self->unk1AE = 1;
		if (self->unk198 < self->mTobiPukuParams->mSLBoundNum.get()) {
			self->unk198++;
			f32 scale = self->mTobiPukuParams->mSLBoundVal.get();
			JGeometry::TVec3<f32> velocity = self->mLaunchVelocity;
			velocity.x *= scale;
			velocity.z *= scale;
			velocity.y = TTobiPuku::mBoundVelocityY * scale
			             * (self->unk1B0 - self->mGroundHeight) / 30.0f;
			self->mLaunchVelocity = velocity;
			self->mVelocity       = velocity;
			self->onLiveFlag(LIVE_FLAG_AIRBORNE);
		}
	}

	if (self->mVelocity.y > 0.0f)
		self->unk1B0 = self->mPosition.y;

	if (!self->isAirborne()) {
		spine->pushAfterCurrent(&TNerveTobiPukuLand::theNerve());
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTobiPukuLand::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() < 2) {
		if (self->mGroundPlane->isWaterSurface()) {
			self->mPosition.y -= 10.0f;
			self->onLiveFlag(LIVE_FLAG_UNK10);
			self->generateEffectColumWater();
			if (TTobiPuku::mReturnLaunchSw) {
				self->unk1E4 *= 0.8f;
				f32 ticks = fabsf(600.0f / self->unk1E4);
				self->unk1E8 = (180.0f - self->mRotation.x) / ticks;
			}
			return FALSE;
		}

		if (TTobiPuku::mBoundSw) {
			BOOL canBound
			    = self->unk198 < self->mTobiPukuParams->mSLBoundNum.get();
			if (!canBound)
				self->unk1AE = 0;
			if (canBound) {
				spine->pushAfterCurrent(&TNerveTobiPukuBound::theNerve());
				return TRUE;
			}
		}

		self->unk1B8[0] = self->mPosition;
		self->setFallEndLandAnm();
		self->mRotation.x = 0.0f;
		return FALSE;
	}

	if (self->isFallEndLandBck()) {
		if (spine->getTime() == 1) {
			self->unk1B8[1] = self->mPosition;
			self->unk1B8[1].sub(self->unk1B8[0]);
		}

		if (spine->getTime() < 20) {
			f32 scale = 0.05f * (f32)spine->getTime();
			self->mPosition = self->unk1B8[0];
			self->mPosition.x += self->unk1B8[1].x * scale;
			self->mPosition.y += self->unk1B8[1].y * scale;
			self->mPosition.z += self->unk1B8[1].z * scale;
		}

		if (self->checkCurAnmEnd(0)) {
			spine->pushAfterCurrent(&TNerveTobiPukuPitiPiti::theNerve());
			return TRUE;
		}
	} else if (TTobiPuku::mReturnLaunchSw) {
		f32 distY = self->unk1E0 - self->mPosition.y;
		JGeometry::TVec3<f32> velocity = self->mLaunchVelocity;
		self->mLaunchVelocity.y *= 0.5f;
		self->mLaunchVelocity.z *= 0.5f;
		velocity.x = self->mLaunchVelocity.y;
		velocity.z = self->mLaunchVelocity.z;
		velocity.y = self->unk1E4 * (600.0f - distY) / 600.0f;

		self->mRotation.x += self->unk1E8;
		if (self->mRotation.x > 180.0f)
			self->mRotation.x = 180.0f;
		else if (self->mRotation.x < 0.0f)
			self->mRotation.x = 0.0f;

		f32 absDistY = fabsf(distY);
		f32 rot      = self->mRotation.x * 182.04445f;
		s32 idx      = (s32)rot;
		f32 cos      = jmaCosTable[(u16)idx >> jmaSinShift];
		velocity.x *= cos;
		velocity.z *= cos;
		velocity.y = self->unk1E4;
		self->mPosition.add(velocity);

		if (absDistY > 120.0f) {
			self->unk1EC += 3.0f;
			if (self->unk1EC > 180.0f)
				self->unk1EC = 180.0f;
			else if (self->unk1EC < 0.0f)
				self->unk1EC = 0.0f;
		}

		if (fabsf(distY) > 600.0f) {
			spine->pushAfterCurrent(&TNerveTobiPukuReturnLaunch::theNerve());
			return TRUE;
		}
	} else {
		self->mPosition.y -= 12.0f;
		if (self->isJumpBck() && self->mRotation.x < TTobiPuku::mLandAngle)
			self->mRotation.x += 1.2f;
		if (spine->getTime() > 100) {
			self->onLiveFlag(LIVE_FLAG_DEAD);
			return TRUE;
		}
	}

	return FALSE;
}

BOOL TNerveTobiPukuDie::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0) {
		if (self->isAirborne()) {
			self->onHitFlag(HIT_FLAG_NO_COLLISION);
			JGeometry::TVec3<f32> velocity(0.0f, self->mVelocity.y, 0.0f);
			self->mVelocity = velocity;
			self->setDownAirAnm();
		} else if (self->unk1AD) {
			self->onHitFlag(HIT_FLAG_NO_COLLISION);
			self->setDownLandAnm();
		} else {
			self->onLiveFlag(LIVE_FLAG_UNK20000);
			self->setDeadAnm();
		}
	}

	if (self->checkCurAnmEnd(0)) {
		self->onLiveFlag(LIVE_FLAG_DEAD);
		self->onLiveFlag(LIVE_FLAG_UNK8);
		self->offLiveFlag(LIVE_FLAG_HIDDEN);
		self->offLiveFlag(LIVE_FLAG_UNK10000);
		self->mHolder = nullptr;
		self->stopAnmSound();
		spine->reset();
		spine->setNext(&TNerveSmallEnemyDie::theNerve());
		spine->pushAfterCurrent(spine->getDefault());
		self->onHitFlag(HIT_FLAG_NO_COLLISION);
		self->genRandomItem();
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTobiPukuPitiPiti::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0)
		self->setPichiAnm();

	if (self->checkCurAnmEnd(0)) {
		if (spine->getTime() > self->getTobiPukuParams()->mSLLifeTimer.get()) {
			self->unk1AD = 0;
			spine->pushAfterCurrent(&TNerveTobiPukuDie::theNerve());
			return TRUE;
		}
	}

	return FALSE;
}

BOOL TNerveTobiPukuFall::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0) {
		self->mRotation.x = 0.0f;
		self->setFallAnm();
	}

	if (!self->isAirborne()) {
		if (self->mGroundPlane->isWaterSurface()) {
			spine->pushAfterCurrent(&TNerveTobiPukuDie::theNerve());
			self->generateEffectColumWater();
			self->onLiveFlag(LIVE_FLAG_UNK20000);
		} else {
			spine->pushAfterCurrent(&TNerveTobiPukuDie::theNerve());
			self->onLiveFlag(LIVE_FLAG_UNK20000);
		}
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTobiPukuHitWater::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0) {
		if (self->isAirborne()) {
			if (self->mPosition.y - self->mGroundHeight > 50.0f) {
				self->setAttackAnm();
				self->hitWater();
			}
		} else if (self->unk1AE) {
			self->setPichiAnm();
		}
	}

	if (!self->isAirborne()) {
		Vec dir;
		dir.x = self->mPosition.x - gpMarioPos->x;
		dir.y = 0.0f;
		dir.z = self->mPosition.z - gpMarioPos->z;
		if (dir.x == 0.0f && dir.y == 0.0f && dir.z == 0.0f)
			dir.x += 1.0f;
		MsVECNormalize(&dir, &dir);
		dir.y = 5.0f;
		dir.x *= 5.0f;
		dir.z *= 5.0f;
		self->mVelocity = JGeometry::TVec3<f32>(dir);
		self->onLiveFlag(LIVE_FLAG_AIRBORNE);
		self->mPosition.y += 5.0f;
	}

	if (self->checkCurAnmEnd(0) && self->isAttackBck()) {
		spine->pushAfterCurrent(&TNerveTobiPukuFall::theNerve());
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTobiPukuAttack::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0)
		self->setAttackAnm();

	if (!self->isAirborne())
		return TRUE;

	if (self->getCurAnmFrameNo(0) >= 6.0f) {
		self->unk194 = 0;
		JGeometry::TVec3<f32> velocity(0.0f, self->mVelocity.y, 0.0f);
		self->mVelocity = velocity;
		self->mPosition.y += 2.0f;
		self->onLiveFlag(LIVE_FLAG_AIRBORNE);
	}

	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveTobiPukuFall::theNerve());
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTobiPukuFly::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0) {
		self->setJumpStartAnm();
		self->offLiveFlag(LIVE_FLAG_UNK10);
	}

	if (self->checkCurAnmEnd(0) && self->isJumpStartBck())
		self->setJumpAnm();

	if (!self->isAirborne()) {
		spine->pushAfterCurrent(&TNerveTobiPukuLand::theNerve());
		return TRUE;
	}

	JGeometry::TVec3<f32> velocity = self->mVelocity;
	self->unk1E4                   = velocity.y;
	self->mRotation.x              = MsGetRotFromZaxis(velocity).x;
	return FALSE;
}

BOOL TNerveTobiPukuGenerate::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0) {
		self->onLiveFlag(LIVE_FLAG_UNK10);
		self->mPosition.y -= 300.0f;
		JGeometry::TVec3<f32> velocity = self->mVelocity;
		self->mRotation.x              = MsGetRotFromZaxis(velocity).x;
		self->setJumpAnm();
	}

	self->mPosition.y += self->mLaunchVelocity.y;
	if (self->mPosition.y > self->unk1B0) {
		self->unk198 = 0;
		self->unk194 = 1;
		self->mVelocity = self->mLaunchVelocity;
		self->unk1B4    = MsGetRotFromZaxis(self->mLaunchVelocity).x;
		self->generateEffectColumWater();
		self->onLiveFlag(LIVE_FLAG_AIRBORNE);
		self->offLiveFlag(LIVE_FLAG_UNK10);
		spine->pushAfterCurrent(&TNerveTobiPukuFly::theNerve());
		return TRUE;
	}

	return FALSE;
}

const char** TMoePuku::getBasNameTable() const { return moepuku_bastable; }

void TMoePuku::generateEffectColumWater()
{
	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT))
		return;

	TEffectColumWater* enemy
	    = (TEffectColumWater*)gpConductor->makeOneEnemyAppear(
	        mPosition, "エフェクト水柱マネージャー", 0);
	if (enemy)
		enemy->generate(mPosition, mScaling);

	if (mSpine->getCurrentNerve() != &TNerveTobiPukuGenerate::theNerve())
		startTobiPukuSound(0x296A, mPosition);
	else
		startTobiPukuSound(0x2809, mPosition);

	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emit(0x1D4, &mPosition, 2, nullptr);
	if (emitter) {
		emitter->unk154.set(mScaling);
		emitter->unk174.set(mScaling);
	}
}

void TMoePuku::setJumpStartAnm()
{
	if (isBckAnm(7))
		setBckAnm(7);
}
void TMoePuku::setFallEndLandAnm() { setBckAnm(5); }
void TMoePuku::setDeadAnm() { setBckAnm(1); }
void TMoePuku::setDownLandAnm() { setBckAnm(3); }
void TMoePuku::setDownAirAnm() { setBckAnm(2); }
void TMoePuku::setFallAnm() { setBckAnm(4); }
void TMoePuku::setPichiAnm() { setBckAnm(8); }
void TMoePuku::setAttackAnm() { setBckAnm(0); }
void TMoePuku::setSwimAnm() { setBckAnm(9); }
void TMoePuku::setJumpAnm() { setBckAnm(6); }
bool TMoePuku::isJumpStartBck()
{
	bool result = isBckAnm(7);
	if (result)
		return true;
	return false;
}

bool TMoePuku::isFallEndLandBck()
{
	bool result = isBckAnm(5);
	if (result)
		return true;
	return false;
}

bool TMoePuku::isAttackBck()
{
	bool result = isBckAnm(0);
	if (result)
		return true;
	return false;
}

bool TMoePuku::isDeadBck()
{
	bool result = isBckAnm(1);
	if (result)
		return true;
	return false;
}

bool TMoePuku::isJumpBck()
{
	bool result = isBckAnm(6);
	if (result)
		return true;
	return false;
}

bool TMoePuku::isPichiEffect()
{
	bool result = isBckAnm(8);
	if (result)
		return true;
	return false;
}

void TMoePuku::hitWater()
{
	TTobiPuku::hitWater();

	MtxPtr mtx = mMActor->unk4->mNodeMatrices[1];
	copyMtxTrans(unk1A0, mtx);
	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emitAndBindToPosPtr(0x8B, &mPosition, 0,
	                                                  nullptr);
	if (emitter) {
		emitter->unk154.set(2.0f, 2.0f, 2.0f);
		emitter->unk174.set(2.0f, 2.0f, 2.0f);
	}

	startTobiPukuSound(0x28C5, mPosition);
}

void TMoePuku::calcRootMatrix()
{
	calcTobiPukuRootMatrix(this);

	if (mSpine->getCurrentNerve() == &TNerveTobiPukuFly::theNerve()) {
		startTobiPukuSound(0x20C3, mPosition);
		MtxPtr mtx = mMActor->unk4->mNodeMatrices[1];
		gpMarioParticleManager->emitAndBindToMtxPtr(0x1D1, mtx, 1, this);
		gpMarioParticleManager->emitAndBindToMtxPtr(0x1D2, mtx, 1, this);
		gpMarioParticleManager->emitAndBindToMtxPtr(0x1F8, mtx, 3, this);
	}
}

TPukuPuku::TPukuPuku(const char* name)
    : TTobiPuku(name)
{
}

void TPukuPuku::reset()
{
	TTobiPuku::reset();
	mSpine->initWith(&TNerveTobiPukuSwimWander::theNerve());
}

void TPukuPuku::init(TLiveManager* manager)
{
	TTobiPuku::init(manager);
	mSpine->initWith(&TNerveTobiPukuSwimWander::theNerve());
	gpCurTobiPuku = nullptr;
}

void TPukuPuku::load(JSUMemoryInputStream& stream)
{
	TSmallEnemy::load(stream);
	reset();
	unk1AC = 0;
}

const char** TTobiPuku::getBasNameTable() const { return pukupuku_bastable; }

void TTobiPuku::scalingChangeActor()
{
	f32 xzScale = mJuiceBlock->unk140.x + 0.02f;
	if (xzScale > TSmallEnemyManager::mBlockXZScale)
		xzScale = TSmallEnemyManager::mBlockXZScale;
	else if (xzScale < 0.0f)
		xzScale = 0.0f;

	mJuiceBlock->unk140.z  = xzScale;
	mJuiceBlock->unk140.x  = xzScale;
	mJuiceBlock->mScaling.z = xzScale;
	mJuiceBlock->mScaling.x = xzScale;

	f32 yScale = mJuiceBlock->unk140.y + 0.01f;
	if (yScale > TSmallEnemyManager::mBlockYScale)
		yScale = TSmallEnemyManager::mBlockYScale;
	else if (yScale < 0.0f)
		yScale = 0.0f;

	mJuiceBlock->unk140.y   = yScale;
	mJuiceBlock->mScaling.y = yScale;
}

void TTobiPuku::initAttacker(THitActor* actor)
{
	mRotation = actor->mRotation;
	mSpine->pushNerve(&TNerveTobiPukuFly::theNerve());
	unk184 = 1;
}

void TTobiPuku::changeOut()
{
	offLiveFlag(LIVE_FLAG_HIDDEN);
	mPosition = mJuiceBlock->mPosition;
	gpMarioParticleManager->emitAndBindToPosPtr(0xCD, &mPosition, 0, nullptr);
	getMActor()->setFrameRate(SMSGetAnmFrameRate(), 0);
	mJuiceBlock->kill();
	mJuiceBlock = nullptr;
}

void TTobiPuku::genEventCoin()
{
	isDeadBck();
}

void TTobiPuku::forceKill()
{
	if (mSpine->getCurrentNerve() == &TNerveTobiPukuDie::theNerve())
		return;
	if (mSpine->getCurrentNerve() == &TNerveTobiPukuPrepareFly::theNerve())
		return;
	if (mSpine->getCurrentNerve() == &TNerveTobiPukuFly::theNerve())
		return;
	if (!checkLiveFlag(LIVE_FLAG_UNK10))
		isJumpBck();
}

void TTobiPuku::kill()
{
	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return;
	if (mGroundPlane->checkFlag(0x10))
		return;

	mHitPoints = 1;
	if (mSpine->getCurrentNerve() == &TNerveTobiPukuDie::theNerve() && unk1AD)
		return;

	unk1AD = 1;
	mSpine->reset();
	mSpine->setNext(&TNerveTobiPukuDie::theNerve());
	mSpine->pushAfterCurrent(mSpine->getDefault());
}

void TTobiPuku::hitWater()
{
	Vec velocity = mVelocity;
	Vec dir;
	dir.x = mPosition.x - gpMarioPos->x;
	dir.y = mPosition.y - gpMarioPos->y;
	dir.z = mPosition.z - gpMarioPos->z;

	if (dir.x == 0.0f && dir.y == 0.0f && dir.z == 0.0f)
		dir.x += 1.0f;

	MsVECNormalize(&dir, &dir);
	f32 power = mTobiPukuParams->mSLPowerFromWater.get();
	velocity.x = dir.x * power;
	velocity.y = 2.0f * (dir.y * power);
	velocity.z = dir.z * power;

	*(Vec*)&mVelocity       = velocity;
	*(Vec*)&mLaunchVelocity = velocity;
	unk1B0 = mPosition.y;
	mRotation.y = 180.0f - 0.005493164f * (f32)*gpMarioAngleY;
}

f32 TTobiPuku::getGravityY() const
{
	if (unk194)
		return mTobiPukuParams->mSLFlyGravityY.get();
	return mGravity;
}

void TTobiPuku::attackToMario()
{
	SMS_SendMessageToMario(this, HIT_MESSAGE_ATTACK);

	if (mSpine->getCurrentNerve() == &TNerveTobiPukuAttack::theNerve())
		return;
	if (unk1AE)
		return;
	bool marioFlag = (*gpMarioFlag & 0x20000) != 0 ? true : false;
	if (marioFlag)
		return;

	Vec velocity;
	velocity.x = 0.0f;
	velocity.y = 0.0f;
	velocity.z = 0.0f;
	*(Vec*)&mLaunchVelocity = velocity;
	mSpine->pushNerve(&TNerveTobiPukuAttack::theNerve());
}

void TTobiPuku::generateEffectColumWater()
{
	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT))
		return;

	TEffectColumWater* enemy
	    = (TEffectColumWater*)gpConductor->makeOneEnemyAppear(
	        mPosition, "エフェクト水柱マネージャー", 0);
	if (enemy)
		enemy->generate(mPosition, mScaling);

	if (mSpine->getCurrentNerve() != &TNerveTobiPukuGenerate::theNerve())
		startTobiPukuSound(0x286D, mPosition);
	else
		startTobiPukuSound(0x286C, mPosition);
}

bool TTobiPuku::isReachedToGoalXZ()
{
	JGeometry::TVec3<f32> toGoal = unk104.getPoint();
	toGoal.x -= mPosition.x;
	toGoal.y -= mPosition.y;
	toGoal.z -= mPosition.z;
	toGoal.y = 0.0f;

	if (toGoal.x == 0.0f && toGoal.z == 0.0f)
		return true;
	return MsVECMag2((Vec*)&toGoal) < 200.0f ? true : false;
}

void TTobiPuku::swimEffect()
{
	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT))
		return;

	JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x178, mMActor->unk4->mNodeMatrices[6], 1, this);
	if (emitter) {
		s16 life = 20 + ((s16)(mGroundHeight - mPosition.y) * 16) / 100;
		if (life > 200)
			life = 200;
		emitter->mBaseLifetime = life;
	}
}

void TTobiPuku::walkBehavior(int walk_state, float speed)
{
	TWalkerEnemy::walkBehavior(walk_state, speed);

	s32 time       = mSpine->getTime();
	s32 phase      = (s32)(2.0f * (f32)time * 182.04445f);
	f32 oldY       = mPosition.y;
	u16 idx        = (u16)phase >> jmaSinShift;
	mPosition.y    = unk1E0 + 10.0f * jmaSinTable[idx];
	Vec travel     = mLinearVelocity;
	travel.y       = oldY - mPosition.y;
	mRotation.x    = MsGetRotFromZaxis((JGeometry::TVec3<f32>&)travel).x;
}

void TTobiPuku::behaveToWater(THitActor* actor)
{
	if (mSpine->getCurrentNerve() == &TNerveTobiPukuHitWater::theNerve())
		return;

	startTobiPukuSound(0x282B, mPosition);
	mSpine->pushNerve(&TNerveTobiPukuHitWater::theNerve());
}

void TTobiPuku::setJumpStartAnm()
{
	if (isBckAnm(7))
		setBckAnm(7);
}

void TTobiPuku::setFallEndLandAnm() { setBckAnm(5); }
void TTobiPuku::setDeadAnm() { setBckAnm(1); }
void TTobiPuku::setDownLandAnm() { setBckAnm(3); }
void TTobiPuku::setDownAirAnm() { setBckAnm(2); }
void TTobiPuku::setFallAnm() { setBckAnm(4); }
void TTobiPuku::setPichiAnm() { setBckAnm(8); }
void TTobiPuku::setAttackAnm() { setBckAnm(0); }
void TTobiPuku::setSwimAnm() { setBckAnm(9); }
void TTobiPuku::setJumpAnm() { setBckAnm(6); }
bool TTobiPuku::isJumpStartBck()
{
	bool result = isBckAnm(7);
	if (result)
		return true;
	return false;
}

bool TTobiPuku::isFallEndLandBck()
{
	bool result = isBckAnm(5);
	if (result)
		return true;
	return false;
}

bool TTobiPuku::isAttackBck()
{
	bool result = isBckAnm(0);
	if (result)
		return true;
	return false;
}

bool TTobiPuku::isDeadBck()
{
	bool result = isBckAnm(1);
	if (result)
		return true;
	return false;
}

bool TTobiPuku::isJumpBck()
{
	bool result = isBckAnm(6);
	if (result)
		return true;
	return false;
}

bool TTobiPuku::isPichiEffect()
{
	bool result = isBckAnm(8);
	if (result)
		return true;
	return false;
}

void TTobiPuku::calcRootMatrix()
{
	calcTobiPukuRootMatrix(this);
}

void TTobiPuku::hitWall()
{
	TBGWallCheckRecord record(mPosition.x, mPosition.y + mHeadHeight,
	                          mPosition.z, mBodyScale * mWallRadius * 1.1f, 1,
	                          0);

	if (gpMap->isTouchedWallsAndMoveXZ(&record)) {
		const TBGCheckData* wall = record.mResultWalls[0];
		f32 dot = mVelocity.x * wall->mNormal.x
		          + mVelocity.y * wall->mNormal.y
		          + mVelocity.z * wall->mNormal.z;
		f32 bounce = -(2.0f * dot);
		mVelocity.x += bounce * wall->mNormal.x;
		mVelocity.y *= 0.5f;
		mVelocity.z += bounce * wall->mNormal.z;
		mLaunchVelocity = mVelocity;
		unk1B0          = mPosition.y;
		return;
	}

	const TBGCheckData* roof;
	gpMap->checkRoof(mPosition.x, mPosition.y + mHeadHeight, mPosition.z,
	                 &roof);
	if (roof && roof->mActor && mVelocity.y > 0.0f)
		mVelocity.y = 0.0f;
}

void TTobiPuku::moveObject()
{
	mTurnSpeed = mTobiPukuParams->mSLTurnSpeedLow.get();
	if (mBoundSw) {
		BOOL airborne = checkLiveFlag(LIVE_FLAG_AIRBORNE) ? TRUE : FALSE;
		if (airborne)
			hitWall();
	}
	TWalkerEnemy::moveObject();
}

void TTobiPuku::reset()
{
	gpCurTobiPuku = this;
	TWalkerEnemy::reset();
	mSpine->initWith(&TNerveTobiPukuGenerate::theNerve());

	unk1AD  = 1;
	unk194  = 0;
	unk1B8[1] = mPosition;
	unk1B8[0] = unk1B8[1];
	unk1E0  = mPosition.y;
}

void TTobiPuku::init(TLiveManager* manager)
{
	TWalkerEnemy::init(manager);
	mActorType      = 0x10000012;
	unk150          = 0x31;
	mTobiPukuParams = getTobiPukuParams();
	getMActor()->setJointCallback(1, TobiPukuRollCallback);
}

TTobiPuku::TTobiPuku(const char* name)
    : TWalkerEnemy(name)
    , unk194(0)
    , unk198(0)
    , mTobiPukuParams(nullptr)
    , unk1AC(1)
    , unk1AD(1)
    , unk1AE(0)
    , unk1B0(0.0f)
    , unk1B4(0.0f)
    , unk1E0(0.0f)
    , unk1E4(0.0f)
    , unk1E8(0.0f)
    , unk1EC(0.0f)
{
	gpCurTobiPuku = nullptr;
}

void TMoePukuLaunchPad::launch()
{
	TTobiPuku* puku = (TTobiPuku*)gpConductor->makeOneEnemyAppear(
	    mPosition, "モエプクマネージャー", 1);
	if (puku) {
		forceLaunch(puku);
		mLaunchedPuku = puku;
	}
}

void TTobiPukuLaunchPad::forceLaunch(TTobiPuku* puku)
{
	JGeometry::TVec3<f32> target = mPosition;
	s32 yaw                      = mRotation.y * 16384.0f / 90.0f;
	f32 sinY = jmaSinTable[(u16)yaw >> jmaSinShift];
	f32 cosY = jmaCosTable[(u16)yaw >> jmaSinShift];

	JGeometry::TVec3<f32> velocity;
	if (((TTobiPukuLaunchPadManager*)mManager)->mForceJumpToPad) {
		f32 dist = mLaunchParams->mSLFlyDist.get();
		target.x += sinY * dist;
		target.z += cosY * dist;
		f32 launchVelocityY = mLaunchParams->mSLLaunchVelocityY.get();
		f32 flyGravityY     = puku->mTobiPukuParams->mSLFlyGravityY.get();
		velocity = calcVelocityToJumpToY(target, launchVelocityY, flyGravityY);
	} else {
		s32 pitch = mRotation.x * 16384.0f / 90.0f;
		f32 cosP = jmaCosTable[(u16)pitch >> jmaSinShift];
		f32 sinP = jmaSinTable[(u16)pitch >> jmaSinShift];
		velocity.x = sinY * mLaunchSpeed * cosP;
		velocity.y = 1.0f * mLaunchSpeed * sinP;
		velocity.z = cosY * mLaunchSpeed * cosP;
	}

	puku->reset();
	puku->mPosition       = mPosition;
	puku->mRotation       = mRotation;
	puku->mLaunchVelocity = velocity;
	puku->unk1B0          = mPosition.y;
	puku->mLaunchPad      = this;

	JGeometry::TVec3<f32> currentVelocity = mVelocity;
	JGeometry::TVec3<f32> rotation        = MsGetRotFromZaxis(currentVelocity);
	puku->unk1B4                          = rotation.x;
}

void TTobiPukuLaunchPad::launch()
{
	TTobiPuku* puku = (TTobiPuku*)gpConductor->makeOneEnemyAppear(
	    mPosition, "とびプクマネージャー", 1);
	if (puku) {
		forceLaunch(puku);
		mLaunchedPuku = puku;
	}
}

void TTobiPukuLaunchPad::reset()
{
	TSmallEnemy::reset();
	mTimer        = 0;
	mLaunchedPuku = nullptr;
}

void TTobiPukuLaunchPad::load(JSUMemoryInputStream& stream)
{
	TSmallEnemy::load(stream);
	s32 speed;
	stream.read(speed);
	mLaunchSpeed = speed;
	reset();
}

void TTobiPukuLaunchPad::init(TLiveManager* manager)
{
	TSmallEnemy::init(manager);
	mActorType     = 0x10000012;
	mLaunchParams  = (TTobiPukuLaunchPadParams*)getSaveParam();
}

void TTobiPukuLaunchPad::perform(u32 flags, JDrama::TGraphics* graphics)
{
	u32 liveFlag = mLiveFlag;
	if (liveFlag & LIVE_FLAG_UNK200)
		return;
	if (liveFlag & LIVE_FLAG_DEAD)
		return;
	if (!(flags & 1))
		return;

	if (TTobiPuku::mReturnLaunchSw) {
		if (!mLaunchedPuku) {
			launch();
			return;
		}
		if (mLaunchedPuku->checkLiveFlag(LIVE_FLAG_DEAD))
			launch();
		return;
	}

	mTimer++;
	if (mTimer > mLaunchParams->mSLLaunchInterval.get()) {
		mTimer = 0;
		launch();
	}
}

TTobiPukuLaunchPad::TTobiPukuLaunchPad(const char* name)
    : TSmallEnemy(name)
    , mTimer(0)
    , mLaunchSpeed(30.0f)
    , mLaunchedPuku(nullptr)
{
}

TSpineEnemy* TMoePukuManager::createEnemyInstance()
{
	return new TMoePuku("モエプク");
}

TSpineEnemy* TTobiPukuManager::createEnemyInstance()
{
	return new TTobiPuku("とびプク");
}

void TTobiPukuManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TTobiPukuParams("/enemy/tobipuku.prm");
	TSmallEnemyManager::load(stream);
}

TTobiPukuManager::TTobiPukuManager(const char* name)
    : TSmallEnemyManager(name)
{
}

TSpineEnemy* TMoePukuLaunchPadManager::createEnemyInstance()
{
	return new TMoePukuLaunchPad("モエプク発射台");
}

void TTobiPukuLaunchPadManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	for (int i = 0; i < getActiveObjNum(); ++i)
		getObj(i)->perform(flags, graphics);
}

TSpineEnemy* TTobiPukuLaunchPadManager::createEnemyInstance()
{
	return new TTobiPukuLaunchPad("とびプク発射台");
}

void TTobiPukuLaunchPadManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TTobiPukuLaunchPadParams("/enemy/tobipukulaunch.prm");
	TSmallEnemyManager::load(stream);
}

TTobiPukuLaunchPadManager::TTobiPukuLaunchPadManager(const char* name)
    : TSmallEnemyManager(name)
    , mForceJumpToPad(0)
{
}

int TobiPukuRollCallback(J3DNode* node, int timing)
{
	if (timing != 0)
		return 1;

	TTobiPuku* puku = gpCurTobiPuku;
	if (!puku)
		return 1;

	const TNerveBase<TLiveActor>* nerve = puku->mSpine->getCurrentNerve();
	if (nerve != &TNerveTobiPukuLand::theNerve()
	    && nerve != &TNerveTobiPukuPrepareFly::theNerve()
	    && nerve != &TNerveTobiPukuReturnLaunch::theNerve())
		return 1;

	s32 phase = (s32)(puku->unk1EC * 182.04445f);
	u16 idx   = (u16)phase >> jmaSinShift;
	f32 sin   = jmaSinTable[idx];
	f32 cos   = jmaCosTable[idx];
	Mtx roll;
	roll[0][0] = cos;
	roll[0][1] = -sin;
	roll[0][2] = 0.0f;
	roll[0][3] = 0.0f;
	roll[1][0] = sin;
	roll[1][1] = cos;
	roll[1][2] = 0.0f;
	roll[1][3] = 0.0f;
	roll[2][0] = 0.0f;
	roll[2][1] = 0.0f;
	roll[2][2] = 1.0f;
	roll[2][3] = 0.0f;

	J3DJoint* joint = (J3DJoint*)node;
	MtxPtr jointMtx = puku->mMActor->unk4->mNodeMatrices[joint->getJntNo()];
	PSMTXConcat(jointMtx, roll, roll);
	PSMTXConcat(J3DSys::mCurrentMtx, roll, J3DSys::mCurrentMtx);
	return 1;
}

void TMoePuku::swimEffect()
{
}

BOOL TTobiPuku::isInhibitedForceMove()
{
	return checkLiveFlag(LIVE_FLAG_AIRBORNE) ? TRUE : FALSE;
}
