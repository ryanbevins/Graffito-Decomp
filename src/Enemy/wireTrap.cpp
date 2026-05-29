#include <Enemy/WireTrap.hpp>
#include <Enemy/Conductor.hpp>
#include <Player/MarioAccess.hpp>
#include <Enemy/Launcher.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <M3DUtil/MActor.hpp>
#include <System/Application.hpp>
#include <System/Particles.hpp>
#include <System/EmitterViewObj.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/ObjModel.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JMath.hpp>
#include <dolphin/mtx.h>

#include <M3DUtil/InfectiousStrings.hpp>

extern "C" bool SMS_IsMarioOnWire();

namespace {
const char cMatName[]      = "_mat_1";
const GXColorS10 cRedColor  = { 0xD2, 0x14, 0x0F, 0x00 };
const GXColorS10 cBlueColor = { 0x0F, 0x14, 0xD2, 0x00 };
} // namespace

DEFINE_NERVE(TNerveWireTrapGoWait, TLiveActor)
{
	TWireTrap* self = (TWireTrap*)spine->getBody();
	return self->getSaveParam2()->mGoTimerMax.get() < spine->getTime();
}

DEFINE_NERVE(TNerveWireTrapWait, TLiveActor)
{
	TWireTrap* self = (TWireTrap*)spine->getBody();
	return self->mWaitTime < spine->getTime();
}

DEFINE_NERVE(TNerveWireTrapSearch, TLiveActor)
{
	TWireTrap* self = (TWireTrap*)spine->getBody();

	bool transit = false;
	if (!SMS_IsMarioOnWire()) {
		transit = false;
	} else {
		if (self->mBiriTimer > 0)
			self->mBiriTimer -= 1;

		JGeometry::TVec3<f32> delta = *gpMarioPos;
		delta.x -= self->mPosition.x;
		delta.y -= self->mPosition.y;
		delta.z -= self->mPosition.z;

		JGeometry::TVec3<f32> wdir = self->getWireBinder()->getDir();
		f32 dot = delta.x * wdir.x + delta.y * wdir.y + delta.z * wdir.z;
		int sign;
		if (dot > 0.0f)
			sign = 1;
		else if (dot < 0.0f)
			sign = -1;
		else
			sign = 0;
		self->mWireDir = (f32)sign;

		JGeometry::TVec3<f32> vel = self->getWireDir();
		f32 s;
		if (self->mShakeTimer > 0)
			s = 1.0f + self->mShakeWidth * (f32)self->mShakeTimer / 30.0f;
		else
			s = 1.0f;
		vel.scale(self->mWireDir * s);
		vel.scale(self->mScaleSpeed);
		self->mLinearVelocity = vel;
		transit = false;
	}

	if (transit) {
		spine->pushAfterCurrent(this);
		spine->pushAfterCurrent(&TNerveWireTrapWait::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveWireTrapOnewayMoveEnd, TLiveActor)
{
	TWireTrap* self = (TWireTrap*)spine->getBody();
	self->unk64 |= 1;

	self->mScaleRate -= 1.0f / self->getSaveParam2()->mScaleTimerMax.get();

	bool done;
	if (self->mScaleRate <= 0.0f) {
		self->mScaleRate = 0.0f;
		done             = true;
		self->unk64 &= ~1;
	} else {
		done = false;
	}

	if (done) {
		f32 sign = self->mWireDir < 0.0f ? -1.0f : 1.0f;
		self->getWireBinder()->getPoint(&self->mPosition,
		                                sign * 0.01f + self->mWireDir);
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveWireTrapOnewayMove, TLiveActor)
{
	TWireTrap* self = (TWireTrap*)spine->getBody();

	if (spine->getTime() == 0 || self->mScaleRate < 1.0f) {
		spine->pushAfterCurrent(this);
		spine->pushAfterCurrent(&TNerveWireTrapGoWait::theNerve());
		spine->pushAfterCurrent(&TNerveWireTrapOnewayMoveStart::theNerve());
		return TRUE;
	}

	if (self->mBiriTimer > 0)
		self->mBiriTimer -= 1;

	bool transit;
	if (self->getWireBinder()->isEndWire(self->mPosition, self->mWireDir)) {
		transit = true;
	} else {
		JGeometry::TVec3<f32> vel = self->getWireDir();
		f32 s;
		if (self->mShakeTimer > 0)
			s = 1.0f + self->mShakeWidth * (f32)self->mShakeTimer / 30.0f;
		else
			s = 1.0f;
		vel.scale(self->mWireDir * s);
		vel.scale(self->mScaleSpeed);
		self->mLinearVelocity = vel;
		transit = false;
	}

	if (transit) {
		spine->pushAfterCurrent(this);
		spine->pushAfterCurrent(&TNerveWireTrapWait::theNerve());
		spine->pushAfterCurrent(&TNerveWireTrapOnewayMoveEnd::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveWireTrapOnewayMoveStart, TLiveActor)
{
	TWireTrap* self = (TWireTrap*)spine->getBody();
	self->unk64 |= 1;

	self->mScaleRate += 1.0f / self->getSaveParam2()->mScaleTimerMax.get();

	bool done;
	if (self->mScaleRate >= 1.0f) {
		self->mScaleRate = 1.0f;
		done             = true;
		self->unk64 &= ~1;
	} else {
		done = false;
	}

	return done;
}

DEFINE_NERVE(TNerveWireTrapReturnMove, TLiveActor)
{
	TWireTrap* self = (TWireTrap*)spine->getBody();

	if (self->mBiriTimer > 0)
		self->mBiriTimer -= 1;

	bool transit;
	if (self->getWireBinder()->isEndWire(self->mPosition, self->mWireDir)) {
		self->mWireDir *= -1.0f;
		transit = true;
	} else {
		JGeometry::TVec3<f32> vel = self->getWireDir();
		f32 s;
		if (self->mShakeTimer > 0)
			s = 1.0f + self->mShakeWidth * (f32)self->mShakeTimer / 30.0f;
		else
			s = 1.0f;
		vel.scale(self->mWireDir * s);
		vel.scale(self->mScaleSpeed);
		self->mLinearVelocity = vel;
		transit = false;
	}

	if (transit) {
		spine->pushAfterCurrent(this);
		spine->pushAfterCurrent(&TNerveWireTrapWait::theNerve());
		return TRUE;
	}
	return FALSE;
}

void TWireTrapManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "wire_trap.bmd", 0x10210000, 0 },
	};
	createModelDataArray(entry);
}

void TWireTrapManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TWireTrapParams("/enemy/wiretrap.prm");
	TEnemyManager::load(stream);
}

TWireTrapManager::TWireTrapManager(const char* name)
    : TEnemyManager(name)
{
}

const JGeometry::TVec3<f32>& TWireTrap::getWireDir() const
{
	return getWireBinder()->getDir();
}

const TNerveBase<TLiveActor>* TWireTrap::getNerveFromMode(int mode)
{
	switch (mode) {
	case 0:
		return &TNerveWireTrapReturnMove::theNerve();
	case 1:
		return &TNerveWireTrapOnewayMove::theNerve();
	case 2:
		return &TNerveWireTrapSearch::theNerve();
	default:
		return nullptr;
	}
}

void TWireTrap::checkHitActors()
{
	int max            = mColCount;
	THitActor** actors = mCollisions;

	for (int i = 0; i < max; i++) {
		THitActor* other = actors[i];
		if (other->mActorType == 0x10000026) {
			if (other == this)
				continue;

			JGeometry::TVec3<f32> myDir = getWireBinder()->getDir();
			f32 ms;
			if (mShakeTimer > 0)
				ms = 1.0f + mShakeWidth * (f32)mShakeTimer / 30.0f;
			else
				ms = 1.0f;
			myDir.scale(mWireDir * ms);

			TWireTrap* o = (TWireTrap*)other;
			JGeometry::TVec3<f32> otDir = o->getWireBinder()->getDir();
			f32 os;
			if (o->mShakeTimer > 0)
				os = 1.0f + o->mShakeWidth * (f32)o->mShakeTimer / 30.0f;
			else
				os = 1.0f;
			otDir.scale(o->mWireDir * os);

			if (mBiriTimer <= 0 && mColorType == 0) {
				mBiriTimer = 0x1e;
				JGeometry::TVec3<f32> a = getWireDir();
				a.scale(mWireDir * ms);
				JGeometry::TVec3<f32> b = o->getWireDir();
				b.scale(o->mWireDir * os);
				if (a.x * b.x + a.y * b.y + a.z * b.z < 0.0f)
					mWireDir *= -1.0f;
				mSpine->initWith(&TNerveWireTrapWait::theNerve());
			}
		} else if (other->mActorType == 0x80000001) {
			SMS_SendMessageToMario(this, 9);
		}
	}
}

void TWireTrap::moveObject()
{
	if (mShakeTimer > 0) {
		mShakeTimer -= 1;
		if (mShakeTimer <= 0)
			mShakeWidth = 0.0f;
	}

	mAttackRadius = 20.0f * mScaling.x;
	mAttackHeight = 30.0f * mScaling.y;
	mDamageRadius = 40.0f * mScaling.x;
	mDamageHeight = 40.0f * mScaling.y;

	calcEntryRadius();
	checkHitActors();
	TLiveActor::moveObject();

	if (gpMSound->gateCheck(0x20bf))
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x20bf, (Vec*)&mPosition, 0, nullptr, 0, 4);
}

void TWireTrap::calcRootMatrix()
{
	if (mHolder != nullptr) {
		TSpineEnemy::calcRootMatrix();
		return;
	}

	JPABaseEmitter* e1 = gpMarioParticleManager->emitAndBindToPosPtr(
	    0x190, &mPosition, 1, this);
	if (e1) {
		e1->unk154.set(mScaling.x, mScaling.y, mScaling.z);
		e1->unk174.set(mScaling.x, mScaling.y, mScaling.z);
	}
	JPABaseEmitter* e2 = gpMarioParticleManager->emitAndBindToPosPtr(
	    0x191, &mPosition, 1, this);
	if (e2) {
		e2->unk154.set(mScaling.x, mScaling.y, mScaling.z);
		e2->unk174.set(mScaling.x, mScaling.y, mScaling.z);
	}
}

void TWireTrap::kill()
{
	if (mLiveFlag & 1)
		return;

	mLiveFlag |= 1;
	unk64 |= 1;
	mSpine->reset();
	mSpine->setNext(&TNerveWaitForever<TLiveActor>::theNerve());

	JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
	SMS_EasyEmitParticle<E_SMS_EFFECT_ONETIME_NORMAL>(
	    (E_SMS_EFFECT_ONETIME_NORMAL)0xe4, &mPosition, nullptr, scale);
	JGeometry::TVec3<f32> scale2(1.0f, 1.0f, 1.0f);
	SMS_EasyEmitParticle<E_SMS_EFFECT_ONETIME_NORMAL>(
	    (E_SMS_EFFECT_ONETIME_NORMAL)0xe6, &mPosition, nullptr, scale2);
}

BOOL TWireTrap::receiveMessage(THitActor* sender, u32 message)
{
	switch (message) {
	case 0xF: {
		JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
		SMS_EasyEmitParticle<E_SMS_EFFECT_ONETIME_NORMAL>(
		    PARTICLE_MS_ENM_WATHIT, &mPosition, nullptr, scale);
		gpMSound->startSoundSet(0x6802, (Vec*)&mPosition, 0, 0.0f, 0, 0, 4);
		mShakeTimer = 0x1e;

		JGeometry::TVec3<f32> delta = mPosition;
		delta.x -= gpMarioPos->x;
		delta.y -= gpMarioPos->y;
		delta.z -= gpMarioPos->z;

		JGeometry::TVec3<f32> dir = getWireBinder()->getDir();
		dir.scale(mWireDir);
		if (0.0f <= delta.x * dir.x + delta.y * dir.y + delta.z * dir.z)
			mShakeWidth = getSaveParam2()->mInWaterPowerRate.get();
		else
			mShakeWidth = -getSaveParam2()->mInWaterPowerRate.get();
		return TRUE;
	}
	case 4:
		if (mHolder == nullptr) {
			unk64 |= 1;
			mHolder = (TTakeActor*)sender;
			return TRUE;
		}
		break;
	case 7:
	case 8:
		if (mHolder != nullptr) {
			mHolder = nullptr;
			return TRUE;
		}
		break;
	case 0xB:
		kill();
		return TRUE;
	default:
		break;
	}
	return TSpineEnemy::receiveMessage(sender, message);
}

void TWireTrap::load(JSUMemoryInputStream& stream)
{
	TSpineEnemy::load(stream);

	s32 shakeBase;
	s32 colorType;
	s32 mode;
	stream.read(&shakeBase, 4);
	stream.read(&colorType, 4);
	stream.read(&mWaitTime, 4);
	stream.read(&mode, 4);

	mScaleSpeed = (f32)shakeBase / 10.0f;

	if (colorType == -1)
		colorType = 0;
	mColorType = colorType;

	if (mode == -1) {
		u16 idx = getModel()->getModelData()->getMaterialName()->getIndex(
		    cMatName);
		SMS_InitPacket_OneTevColor(getModel(), idx, GX_TEVREG2, &cRedColor);
	} else {
		u16 idx = getModel()->getModelData()->getMaterialName()->getIndex(
		    cMatName);
		SMS_InitPacket_OneTevColor(getModel(), idx, GX_TEVREG2, &cBlueColor);
	}

	getWireBinder()->init(mPosition);

	mSpine->reset();
	mSpine->setNext(getNerveFromMode(mColorType));

	JGeometry::TVec3<f32> v;
	s16 ang = (s16)(182.04445f * mRotation.y);
	v.set(1.0f * jmaSinTable[(ang >> jmaSinShift)],
	      0.0f,
	      1.0f * jmaCosTable[(ang >> jmaSinShift)]);
	JGeometry::TVec3<f32> wdir = getWireBinder()->getDir();
	if (0.0f <= v.x * wdir.x + v.y * wdir.y + v.z * wdir.z)
		mWireDir = 1.0f;
	else
		mWireDir = -1.0f;
	mScaleRate = 1.0f;
}

void TWireTrap::init(TLiveManager* manager)
{
	mManager = manager;
	mManager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor       = mMActorKeeper->createMActor("wire_trap.bmd", 0);

	mBinder = new TWireBinder();

	mSpine->initWith(&TNerveWaitForever<TLiveActor>::theNerve());

	initHitActor(0x10000026, 2, 0x90000000, 20.0f, 30.0f, 40.0f, 40.0f);
	unk64 &= ~1;

	if (!gParticleFlagLoaded[0]) {
		gpResourceManager->load("/scene/wireTrap/jpa/ms_wrt_biri_a.jpa", 0x190);
		gParticleFlagLoaded[0] = 1;
	}
	if (!gParticleFlagLoaded[1]) {
		gpResourceManager->load("/scene/wireTrap/jpa/ms_wrt_biri_b.jpa", 0x191);
		gParticleFlagLoaded[1] = 1;
	}
}

TWireTrap::TWireTrap(const char* name)
    : TSpineEnemy(name)
{
	unk160      = 0;
	mShakeTimer = 0;
}

TWireTrapParams::TWireTrapParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mInWaterPowerRate, 0.9f)
    , PARAM_INIT(mScaleTimerMax, 60)
    , PARAM_INIT(mGoTimerMax, 90)
{
	TParams::load(mPrmPath);
}
