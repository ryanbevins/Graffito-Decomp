#include <Enemy/Kukku.hpp>
#include <Enemy/Graph.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Strategy.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/TexUtil.hpp>
#include <Animal/AnimalBase.hpp>
#include <MSound/MSound.hpp>
#include <System/Application.hpp>
#include <System/MarDirector.hpp>
#include <Map/Map.hpp>
#include <Map/PollutionManager.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/ItemManager.hpp>
#include <System/Particles.hpp>
#include <System/EmitterViewObj.hpp>
#include <JSystem/JMath.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <Map/MapData.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRGraphics.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JKernel/JKRArchive.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

JGeometry::TQuat4<f32> SMS_Eular2Quat(const JGeometry::TVec3<f32>&);

static char* tori_bastable[] = {
	(char*)"/scene/tori/bas/tori_back.bas",
	(char*)"/scene/tori/bas/tori_down.bas",
	(char*)0,
	(char*)"/scene/tori/bas/tori_fall_end.bas",
	(char*)"/scene/tori/bas/tori_hit.bas",
	(char*)"/scene/tori/bas/tori_wait.bas",
};

namespace {
const int cDropCoinNumTable[] = { 3, 3, 1, 2 };
}

static inline JGeometry::TVec3<f32> makeForwardVec(f32 speed)
{
	return JGeometry::TVec3<f32>(0.0f, 0.0f, speed);
}

DEFINE_NERVE(TNerveKukkuFall, TLiveActor)
{
	TKukku* self = (TKukku*)spine->getBody();

	if (spine->getTime() == 0) {
		self->getMActor()->setBck("tori_wait");
		self->setCurAnmSound();
		J3DFrameCtrl* fc = self->getMActor()->getFrameCtrl(0);
		fc->setRate(2.0f * SMSGetAnmFrameRate());

		JGeometry::TVec3<f32> vel(0.0f, -self->getSaveParam2()->mWaterPowerY.get(),
		                          0.0f);
		self->mVelocity = vel;
		self->dropCoins();
	}

	if (!self->checkLiveFlag(LIVE_FLAG_AIRBORNE)) {
		JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
		SMS_EasyEmitParticle<E_SMS_EFFECT_ONETIME_NORMAL>(
		    (E_SMS_EFFECT_ONETIME_NORMAL)0xA1, &self->mPosition, nullptr, scale);
		JGeometry::TVec3<f32> scale2(1.0f, 1.0f, 1.0f);
		SMS_EasyEmitParticle<E_SMS_EFFECT_ONETIME_NORMAL>(
		    (E_SMS_EFFECT_ONETIME_NORMAL)0xA2, &self->mPosition, nullptr, scale2);
		spine->pushAfterCurrent(&TNerveSmallEnemyDie::theNerve());
		return TRUE;
	}

	JGeometry::TVec3<f32> vel = self->mVelocity;
	f32 fric                  = self->getSaveParam2()->mAirFric.get();
	vel.x *= fric;
	vel.y *= fric;
	vel.z *= fric;
	vel.y += self->getSaveParam2()->mUpperVelocityY.get();

	bool landed = false;
	if (-0.1f < vel.y) {
		vel.y  = 0.0f;
		landed = true;
	}
	self->mVelocity = vel;

	if (landed) {
		spine->pushAfterCurrent(&TNerveKukkuRecoverGraph::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveKukkuPostFall, TLiveActor)
{
	TKukku* self = (TKukku*)spine->getBody();

	if (spine->getTime() == 0) {
		self->getMActor()->setBck("tori_back");
		self->setCurAnmSound();
		JGeometry::TVec3<f32> vel(0.0f, 0.0f, 0.0f);
		self->mVelocity = vel;
	}

	if (self->checkCurAnmEnd(0)) {
		if (self->getSaveParam2()->mHabatakiTimer.get() < spine->getTime()) {
			spine->pushAfterCurrent(&TNerveKukkuGraphWander::theNerve());
			return TRUE;
		}
	}
	return FALSE;
}

DEFINE_NERVE(TNerveKukkuRecoverGraph, TLiveActor)
{
	TKukku* self = (TKukku*)spine->getBody();

	if (spine->getTime() == 0) {
		self->getMActor()->setBck("tori_back");
		self->setCurAnmSound();
		JGeometry::TVec3<f32> vel(0.0f, 0.0f, 0.0f);
		self->mVelocity = vel;
	}

	if (self->getSaveParam2()->mHabatakiTimer.get() < spine->getTime()) {
		spine->pushAfterCurrent(&TNerveKukkuGraphWander::theNerve());
		return TRUE;
	}

	self->updateRotation();
	self->mLinearVelocity
	    = self->calcMomentum(self->getSaveParam2()->mMarchSpeed.get());
	return FALSE;
}

DEFINE_NERVE(TNerveKukkuGraphWander, TLiveActor)
{
	TKukku* self = (TKukku*)spine->getBody();

	if (spine->getTime() == 0) {
		JGeometry::TVec3<f32> zero(0.0f, 0.0f, 0.0f);
		self->mVelocity = zero;
		self->getTracer()->reset();
		self->goToShortestNextGraphNode();
		if (self->getMActor()->checkCurAnm("tori_back", 0)) {
			self->getMActor()->setBck("tori_back");
			self->setCurAnmSound();
		} else {
			self->getMActor()->setBck("tori_wait");
			self->setCurAnmSound();
		}
	}

	if (self->isReachedToGoal()) {
		self->goToRandomNextGraphNode();
		if (!self->getMActor()->checkCurAnm("tori_wait", 0)) {
			self->getMActor()->setBck("tori_wait");
			self->setCurAnmSound();
		}
	}

	JGeometry::TVec3<f32> toMario = *gpMarioPos;
	toMario.x -= self->mPosition.x;
	toMario.y -= self->mPosition.y;
	toMario.z -= self->mPosition.z;
	toMario.y = 0.0f;

	f32 range = self->getSaveParam2()->mSearchRange.get();
	if (toMario.x * toMario.x + toMario.y * toMario.y + toMario.z * toMario.z
	    < range * range) {
		if (self->unk1AC >= 0) {
			self->unk1AC -= 1;
		} else {
			TKukkuBall* found = nullptr;
			for (TKukkuBall** p = self->mKukkuBalls;
			     p != (TKukkuBall**)&self->unk1A0; ++p) {
				TKukkuBall* ball = *p;
				bool isFree = false;
				if ((ball->mFlags & 1) && ball->mState == 0)
					isFree = true;
				if (isFree) {
					found = ball;
					break;
				}
			}
			if (found) {
				JGeometry::TVec3<f32> dir;
				dir.set(0.0f, 1.0f, 0.0f);
				f32 speed = self->getSaveParam2()->mShootSpeed.get();
				f32 d     = dir.dot(dir);
				if (d <= 0.0000038146973f) {
					dir.set(0.0f, 0.0f, 0.0f);
				} else {
					dir.scale(speed * JGeometry::TUtil<f32>::inv_sqrt(d), dir);
				}
				self->getModel()->getModelData()->getJointName()->getIndex(
				    "null_osen");
				found->offHitFlag(1);
				found->mFlags &= ~1;
				found->mPosition = self->mPosition;
				found->mVelocity = dir;
				self->unk1AC = self->getSaveParam2()->mShootInterval.get();
				if (gpMSound->gateCheck(0x28f3)) {
					MSoundSESystem::MSoundSE::startSoundActor(
					    0x28f3, &self->mPosition, 0, nullptr, 0, 4);
				}
			}
		}
	}

	self->updateRotation();
	JGeometry::TQuat4<f32> q = SMS_Eular2Quat(self->mRotation);
	JGeometry::TVec3<f32> fwd
	    = makeForwardVec(self->getSaveParam2()->mMarchSpeed.get());
	q.rotate(fwd, fwd);
	self->mLinearVelocity = fwd;
	return FALSE;
}

void TKukkuManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
	    { "tori.bmd", 0x10210000, 0 },
	    { nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TKukkuParams::TKukkuParams(const char* path)
    : TSmallEnemyParams(path)
    , PARAM_INIT(mMarchSpeed, 3.0f)
    , PARAM_INIT(mTurnSpeed, 0.2f)
    , PARAM_INIT(mWaterPowerY, 12.0f)
    , PARAM_INIT(mShootSpeed, 1.0f)
    , PARAM_INIT(mShootInterval, 60)
    , PARAM_INIT(mSearchRange, 800.0f)
    , PARAM_INIT(mHabatakiTimer, 45)
    , PARAM_INIT(mAirFric, 0.97f)
    , PARAM_INIT(mUpperVelocityY, 0.0f)
    , PARAM_INIT(mDropSpeed, 5.0f)
    , PARAM_INIT(mDropAngleX, -0.25f)
{
	TParams::load(mPrmPath);
}

void TKukkuManager::load(JSUMemoryInputStream& stream)
{
	TKukkuParams* params = new TKukkuParams("/enemy/kukku.prm");
	unk38                = params;
	params->mSLAttackRadius.set(30);
	params->mSLAttackHeight.set(30);
	params->mSLDamageRadius.set(100);
	params->mSLDamageHeight.set(100);
	params->mSLBodyRadius.set(5.0f);
	TSmallEnemyManager::load(stream);
}

TKukkuManager::TKukkuManager(const char* name)
    : TSmallEnemyManager(name)
{
}

const char** TKukku::getBasNameTable() const
{
	return (const char**)tori_bastable;
}

void TKukku::setDeadAnm()
{
	getMActor()->setBck("tori_down");
	setCurAnmSound();
}

void TKukku::setAfterDeadEffect()
{
	TSmallEnemy::setAfterDeadEffect();
	gpPollution->stamp(((TSmallEnemyManager*)mManager)->getUnk58(), mPosition.x,
	                   mPosition.y, mPosition.z, 1000.0f);
}

void TKukku::bind() { TLiveActor::bind(); }

void TKukku::control()
{
	if (getUnk1A4() > 0)
		unk1A4 -= 1;
	TLiveActor::control();
}

void TKukku::reset()
{
	unk1A4 = 0;
	unk1AC = 0;
	mGravity = 0.0f;
	unk1B0 = 0;
	onLiveFlag(LIVE_FLAG_AIRBORNE);
	mScaledBodyRadius = 75.0f;
}

BOOL TKukku::receiveMessage(THitActor* sender, u32 message)
{
	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return FALSE;

	switch (message) {
	case 0:
	case 1:
		mSpine->reset();
		mSpine->setNext(&TNerveSmallEnemyDie::theNerve());
		return TRUE;
	}
	return TSmallEnemy::receiveMessage(sender, message);
}

void TKukku::perform(u32 action, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(action, graphics);

	for (TKukkuBall** ball = mKukkuBalls; ball != (TKukkuBall**)&unk1A0;
	     ++ball) {
		(*ball)->perform(action, graphics);
	}
}

TKukku::TKukku(const char* name)
    : TSmallEnemy(name)
{
	unk1A0 = nullptr;
}

void TKukku::init(TLiveManager* manager)
{
	mManager = manager;
	mManager->manageActor(this);
	mMActorKeeper = new TMActorKeeper(mManager, 4);
	mMActor       = mMActorKeeper->createMActor("tori.bmd", 0);
	mSpine->initWith(&TNerveKukkuGraphWander::theNerve());

	unk1A0 = TMapObjBaseManager::newAndRegisterObj("mushroom1upR");

	for (TKukkuBall** ball = mKukkuBalls; ball != (TKukkuBall**)&unk1A0;
	     ++ball) {
		*ball = new TKukkuBall(mMActorKeeper->createMActor("torifun.bmd", 3));
		(*ball)->init();
	}

	initHitActor(0x1000002e, 1, 0x80000000, 30.0f, 30.0f, 100.0f, 100.0f);
	offHitFlag(0x1);
	initAnmSound();

	SMS_LoadParticle("/scene/tori/jpa/ms_cooc_ase.jpa", 0x18c);
	SMS_LoadParticle("/scene/tori/jpa/ms_cooc_hane.jpa", 0x18d);

	unk1A8 = getModel()->getModelData()->getJointName()->getIndex("center");
	reset();
}

TKukkuBall::TKukkuBall(MActor* mactor, const char* name)
    : THitActor(name)
{
	mMActor = mactor;
	mFlags  = 1;
	mState  = 0;
}

void TKukkuBall::init()
{
	initHitActor(0x1000002e, 1, 0x80000000, 30.0f, 30.0f, 0.0f, 0.0f);
	onHitFlag(1);
	onHitFlag(4);

	JDrama::TNameRefGen::search<TIdxGroupObj>(
	    "\x93\x47\x83\x4f\x83\x8b\x81\x5b\x83\x76")
	    ->getChildren()
	    .push_back(this);

	ResTIMG* res = (ResTIMG*)JKRFileLoader::getGlbResource(
	    "/scene/map/pollution/H_ma_rak.bti");
	if (res)
		SMS_ChangeTextureAll(mMActor->getModel()->getModelData(),
		                     "K_name_dummy", *res);
}

void TKukkuBall::perform(u32 action, JDrama::TGraphics* graphics)
{
	if (mFlags & 1)
		return;

	if (action & 2) {
		Mtx m;
		m[0][0] = 1.0f;
		m[1][0] = 0.0f;
		m[2][0] = 0.0f;
		m[0][1] = 0.0f;
		m[1][1] = 1.0f;
		m[2][1] = 0.0f;
		m[0][2] = 0.0f;
		m[1][2] = 0.0f;
		m[2][2] = 1.0f;
		m[0][3] = mPosition.x;
		m[1][3] = mPosition.y;
		m[2][3] = mPosition.z;
		mMActor->getModel()->setBaseScale(mScaling);
		mMActor->getModel()->setBaseTRMtx(m);
		mMActor->getModel()->calc();
	}

	if (action & 1) {
		for (THitActor** c = mCollisions; c != mCollisions + mColCount; ++c) {
			if ((*c)->getActorType() == 0x80000001) {
				SMS_SendMessageToMario(this, 0xe);
				onHitFlag(1);
				mFlags |= 1;
				gpPollution->stamp(1, mPosition.x, mPosition.y, mPosition.z,
				                   500.0f);
			}
		}

		mVelocity.y -= 0.9f;
		mVelocity.x *= 0.94f;
		mVelocity.y *= 0.94f;
		mVelocity.z *= 0.94f;

		JGeometry::TVec3<f32> next = mPosition;
		next.add(mVelocity);

		const TBGCheckData* ground;
		f32 groundY = gpMap->checkGround(next.x, next.y + mAttackHeight, next.z,
		                                 &ground);
		groundY += 1.0f;
		if (next.y <= groundY + 0.05f) {
			onHitFlag(1);
			mFlags |= 1;
			gpPollution->stamp(1, mPosition.x, mPosition.y, mPosition.z, 500.0f);
		}

		gpMap->isTouchedOneWallAndMoveXZ(&next.x, next.y + mAttackHeight,
		                                 &next.z, mAttackRadius);
		mPosition = next;
	}

	if (!(mFlags & 4))
		mMActor->perform(action, graphics);
}

#pragma dont_inline on
JGeometry::TVec3<f32> TKukku::calcMomentum(f32 speed)
{
	JGeometry::TQuat4<f32> q = SMS_Eular2Quat(mRotation);
	JGeometry::TVec3<f32> v(0.0f, 0.0f, speed);
	q.rotate(v, v);
	return v;
}
#pragma dont_inline off

bool TKukku::isFalling() const
{
	const TNerveBase<TLiveActor>* nerve = mSpine->getLatestNerve();
	return nerve == &TNerveKukkuFall::theNerve()
	    || nerve == &TNerveKukkuPostFall::theNerve();
}

void TKukku::updateRotation()
{
	JGeometry::TVec3<f32> toTarget = getUnkF4().getPoint();
	toTarget.x -= mPosition.x;
	toTarget.y -= mPosition.y;
	toTarget.z -= mPosition.z;

	f32 dist = JGeometry::TUtil<f32>::sqrt(toTarget.x * toTarget.x
	                                       + toTarget.y * toTarget.y
	                                       + toTarget.z * toTarget.z);
	if (dist < 100.0f)
		return;

	f32 speed = getSaveParam2()->mMarchSpeed.get();
	f32 turn  = getSaveParam2()->mTurnSpeed.get();

	f32 turnRadius = calcMinimumTurnRadius(speed, turn);
	if (dist <= 2.0f * turnRadius)
		turn = calcTurnSpeedToReach(speed, 0.5f * dist);

	TAnimalBase::getRotationFlyToDir(&mRotation, toTarget, speed, turn);

	mRotation.x *= isFalling() ? 0.0f : 1.0f;
	mRotation.z *= isFalling() ? 0.0f : 1.0f;
}

void TKukku::behaveToWater(THitActor* sender)
{
	const TNerveBase<TLiveActor>* cur = mSpine->getLatestNerve();
	bool b1 = (cur == &TNerveKukkuFall::theNerve()
	           || cur == &TNerveKukkuPostFall::theNerve());
	if (b1)
		return;

	bool recovering;
	if (mSpine->getLatestNerve() != &TNerveKukkuRecoverGraph::theNerve())
		recovering = false;
	else
		recovering = true;
	if (recovering)
		return;

	mSpine->reset();
	mSpine->setNext(&TNerveKukkuFall::theNerve());
}

void TKukku::dropCoins()
{
	if (unk1B0 > 10)
		return;

	if (unk1B0 == 0 && unk1A0 != nullptr) {
		unk1B0              = 1;
		TMapObjBase* mapObj = (TMapObjBase*)unk1A0;
		mapObj->appear();
		mapObj->JSGSetTranslation(mPosition);
		mapObj->mVelocity.set(0.0f, 0.0f, 0.0f);
		mapObj->offLiveFlag(LIVE_FLAG_UNK10);
		return;
	}

	s32 idx = (s32)(4.0f * MsRandF());
	if (idx < 1)
		idx = 1;
	else if (idx > 3)
		idx = 3;
	s32 numCoins = cDropCoinNumTable[idx];

	JGeometry::TQuat4<f32> qSpin;
	qSpin.setRotate(JGeometry::TVec3<f32>(0.0f, 1.0f, 0.0f),
	                6.2831855f / numCoins);

	JGeometry::TQuat4<f32> qTilt;
	qTilt.setRotate(JGeometry::TVec3<f32>(1.0f, 0.0f, 0.0f),
	                3.1415927f * getSaveParam2()->mDropAngleX.get());

	f32 speed = getSaveParam2()->mDropSpeed.get();
	JGeometry::TVec3<f32> dir;
	dir.set(speed * JMASin(mRotation.y), 0.0f, speed * JMACos(mRotation.y));

	qTilt.rotate(dir);
	qSpin.rotate(dir);

	for (s32 i = 0; i < numCoins; ++i) {
		TMapObjBase* coin = gpItemManager->makeObjAppear(0x2000000e);
		if (!coin)
			return;
		coin->appear();
		coin->JSGSetTranslation(mPosition);
		coin->mVelocity.set(dir);
		coin->offLiveFlag(LIVE_FLAG_UNK10);
		if (++unk1B0 == 10)
			return;
		qSpin.rotate(dir);
	}
}

void TKukku::calcRootMatrix()
{
	if (mSpine->getLatestNerve() == &TNerveSmallEnemyDie::theNerve()) {
		JGeometry::TVec3<f32> normal;
		if (getGroundPlane()) {
			normal.normalize(getGroundPlane()->getNormal());
		} else {
			normal.set(0.0f, 1.0f, 0.0f);
		}

		JGeometry::TQuat4<f32> qHeading;
		qHeading.setRotate(JGeometry::TVec3<f32>(0.0f, 1.0f, 0.0f),
		                   0.017453294f * mRotation.y);

		JGeometry::TQuat4<f32> q;
		q.setRotate(JGeometry::TVec3<f32>(0.0f, 1.0f, 0.0f), normal);
		q.mul(q, qHeading);

		TPosition3f mtx;
		mtx.setQT(q, mPosition);
		getModel()->setBaseTRMtx(mtx);
		getModel()->setBaseScale(mScaling);
	} else {
		TSpineEnemy::calcRootMatrix();
	}

	const TNerveBase<TLiveActor>* n = mSpine->getLatestNerve();
	bool isFallOrPost = (n == &TNerveKukkuFall::theNerve()
	                     || n == &TNerveKukkuPostFall::theNerve());
	if (isFallOrPost) {
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x18d, getModel()->getAnmMtx(unk1A8), 1, this);
	}

	if (getMActor()->checkCurAnm("tori_back", 0)) {
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x18c, getModel()->getAnmMtx(unk1A8), 1, this);
	}
}
