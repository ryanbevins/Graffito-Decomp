#include <Enemy/Igaiga.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/Graph.hpp>
#include <Enemy/Launcher.hpp>
#include <Enemy/AreaCylinder.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <Map/MapEventSink.hpp>
#include <Map/MapMirror.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <JSystem/JMath.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MoveBG/ItemManager.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/ModelWaterManager.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/SharedParts.hpp>
#include <Strategic/Spine.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/Particles.hpp>
#include <dolphin/mtx.h>
#include <stdlib.h>

class JAISound;
static TRollEnemy* gpCurRollEnemy;

f32 TRollEnemy::mBoundVal     = 80.0f;
f32 TRollEnemy::mTransYOffset = 0.0f;
f32 TIgaiga::mReachNodeDist   = 300.0f;

static const char* igaiga_bastable[] = {
	"/scene/igaiga/bas/igaiga_down1.bas",
	"/scene/igaiga/bas/igaiga_down2.bas",
	nullptr,
	nullptr,
	"/scene/igaiga/bas/igaiga_shoot1.bas",
	"/scene/igaiga/bas/igaiga_waterdown1.bas",
	"/scene/igaiga/bas/igaiga_waterhit1.bas",
	nullptr,
};

static const char* gorogoro_bastable[] = {
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

static const char* anmlist[] = { "bosspaku_head_move", nullptr };
static const char* graphlist[] = { "gorogoro0", "gorogoro1" };

DEFINE_NERVE(TNerveGorogoroDie, TLiveActor)
{
	TGorogoro* self = (TGorogoro*)spine->getBody();

	if (spine->getTime() < 2) {
		self->onLiveFlag(LIVE_FLAG_CLIPPED_OUT);
		if (self->isRolling())
			self->bound();
	}

	if (self->checkCurAnmEnd(0) || spine->getTime() > 360) {
		self->onLiveFlag(LIVE_FLAG_CLIPPED_OUT);
		self->onLiveFlag(0x10000);
		self->offLiveFlag(0x4);
		self->stopAnmSound();
		spine->setNext(&TNerveSmallEnemyDie::theNerve());
		self->setAfterDeadEffect();
		return TRUE;
	}

	if (self->checkLiveFlag(0x10000))
		self->walkBehavior(2, 0.5f);

	return FALSE;
}

DEFINE_NERVE(TNerveGorogoroRollOnGraph, TLiveActor)
{
	TGorogoro* self = (TGorogoro*)spine->getBody();

	if (spine->getTime() == 0) {
		self->goToShortestNextGraphNode();
		self->setBckAnm(2);
	}

	self->walkBehavior(0, 0.0f);
	return FALSE;
}

DEFINE_NERVE(TNerveIgaigaShootFromCannon, TLiveActor)
{
	TIgaiga* self = (TIgaiga*)spine->getBody();

	if (spine->getTime() == 0) {
		self->offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
		self->setBckAnm(4);
	}

	self->moveObject();
	if (self->isReachedToGoalXZ()) {
		spine->pushAfterCurrent(&TNerveIgaigaRollOnGraph::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveIgaigaWaterHit, TLiveActor)
{
	TIgaiga* self = (TIgaiga*)spine->getBody();

	if (spine->getTime() == 0)
		self->setBckAnm(6);

	self->walkBehavior(0, 0.0f);
	if (self->checkCurAnmEnd(0) || spine->getTime() > 120) {
		spine->pushAfterCurrent(&TNerveIgaigaRollOnGraph::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveIgaigaRollOnGraph, TLiveActor)
{
	TIgaiga* self = (TIgaiga*)spine->getBody();

	if (spine->getTime() == 0) {
		self->goToShortestNextGraphNode();
		self->setBckAnm(0);
	}

	self->walkBehavior(0, 0.0f);
	return FALSE;
}

TRollEnemy::TRollEnemy(const char* name)
    : TWalkerEnemy(name)
    , unk194(0.0f)
    , unk198(0.0f)
    , unk19C(0.0f)
    , unk1A0(0.0f)
    , unk1A4(0)
    , unk1A8(false)
    , unk1AC(0.0f)
    , unk1B0(1.0f)
{
}

void TRollEnemy::setBehavior()
{
	if (isReachedToGoalXZ())
		goToShortestNextGraphNode();

	walkBehavior(0, 0.0f);
}

bool TRollEnemy::isReachedToGoalXZ()
{
	JGeometry::TVec3<f32> diff = unk104.getPoint();
	diff.sub(mPosition);
	if (!unk1A8)
		diff.y = 0.0f;

	if (MsVECMag2((Vec*)&diff) < 90000.0f)
		return true;
	return false;
}

bool TRollEnemy::isCollidMove(THitActor* actor)
{
	bool isMapObj;
	if (actor->mActorType == 0x4000022B)
		isMapObj = true;
	else
		isMapObj = false;

	if (isMapObj) {
		kill();
		return true;
	}

	actor->receiveMessage(this, 0xE);
	return false;
}

void TRollEnemy::attackToMario()
{
	SMS_SendMessageToMario(this, HIT_MESSAGE_ATTACK);
}

void TRollEnemy::behaveToWater(THitActor*)
{
	mSprayedByWaterCooldown = 0;

	TRollEnemySaveLoadParams* params = (TRollEnemySaveLoadParams*)unk1A4;
	if (unk158 < params->mSLExpandMax.get()) {
		f32 expandRate = params->mSLExpandRate.get();
		mBodyScale *= expandRate;
		unk158 *= expandRate;
		mScaledBodyRadius *= expandRate;

		f32 scale = mScaling.z * expandRate;
		mScaling.z = scale;
		mScaling.y = scale;
		mScaling.x = scale;

		f32 attackRadius = getSaveParam2()->getSLAttackRadius();
		f32 attackHeight = getSaveParam2()->getSLAttackHeight();
		f32 damageRadius = getSaveParam2()->getSLDamageRadius();
		f32 ratio        = mBodyScale / unk154;
		mAttackRadius    = attackRadius * ratio;
		mAttackHeight    = attackHeight * ratio;
		mDamageRadius    = damageRadius * ratio;
		mDamageHeight    = getSaveParam2()->getSLDamageHeight() * ratio;
		calcEntryRadius();
	}
}

void TRollEnemy::flagJump()
{
	JGeometry::TVec3<f32> target;
	unk124->getGraph()
	    ->getGraphNode(unk124->mCurrIdx)
	    .getPoint((Vec*)&target);
	mPosition.y += 30.0f;
	mVelocity = calcVelocityToJumpToY(target, unk124->unkC, getGravityY());
	unk1A8    = true;
	onLiveFlag(LIVE_FLAG_AIRBORNE);
}

void TRollEnemy::walkBehavior(int mode, f32 speed)
{
	if (!unk1A8)
		TWalkerEnemy::walkBehavior(mode, speed);

	if (checkLiveFlag(LIVE_FLAG_AIRBORNE)) {
		if (mPosition.y > mGroundHeight + 20.0f) {
			f32 bounce = (mPosition.y - mGroundHeight) / mBoundVal;
			f32 wrapped
			    = MsWrap(bounce, 0.0f, getRollParams()->mSLBoundVYMax.get());
			if (unk1A0 < wrapped)
				unk1A0 = wrapped;
		}
	} else {
		if (!mGroundPlane->isWaterSurface()) {
			unk1A8 = false;

			if (unk1A0 > unk1B0) {
				bound();
				mVelocity.set(0.0f, unk1A0, 0.0f);
				onLiveFlag(LIVE_FLAG_AIRBORNE);
				mPosition.y += 5.0f;
				unk1A0 = 0.0f;
				boundSE();
			}
		}
	}

	if (mPosition.y < mGroundHeight + 5.0f)
		flagJump();

	if (unk128 > 300) {
		onLiveFlag(LIVE_FLAG_UNK10000);
		kill();
	}
}

void TRollEnemy::reset()
{
	gpCurRollEnemy = this;
	TWalkerEnemy::reset();

	FakeRandInterval randAngle(0.0f, 360.0f);
	unk194 = randAngle.get();
	unk158 = 1.0f;

	TGraphWeb* graph = unk124->getGraph();
	JGeometry::TVec3<f32> point;
	graph->getFirstGraphNode().getPoint((Vec*)&point);
	mPosition = point;
	mPosition.y += 10.0f;

	JGeometry::TVec3<f32> next;
	graph->getGraphNode(1).getPoint((Vec*)&next);
	JGeometry::TVec3<f32> dir = next;
	dir.sub(mPosition);
	mRotation.y = MsAngleWrap(MsGetRotFromZaxisY(dir));

	unk198 = mMarchSpeed * 1.5f;
	unk19C = mMarchSpeed;
	unk1A0 = 0.0f;
	unk124->mCurrIdx = 0;
}

#pragma dont_inline on
TIgaiga::TIgaiga(const char* name)
    : TRollEnemy(name)
    , unk1B4(0)
    , unk1B8(0)
    , unk1BC(true)
    , unk1CC(1.0f)
    , unk1D0(nullptr)
    , unk1E4(1.0f)
    , unk1E8(0)
{
}
#pragma dont_inline off

void TIgaiga::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(flags, graphics);
}

void TIgaiga::calcRootMatrix()
{
	gpCurRollEnemy = this;
	TSpineEnemy::calcRootMatrix();
}

void TIgaiga::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor       = mMActorKeeper->createMActor("igaiga_model1.bmd", 0);
}

void TIgaiga::init(TLiveManager* manager)
{
	TWalkerEnemy::init(manager);
	mActorType = 0x10000017;
	unk150      = 0x11;
	unk64 &= ~0x18000000;
	unk64 |= 0x40000000;
	mSpine->initWith(&TNerveIgaigaRollOnGraph::theNerve());
	unk1A4 = getSaveParam();

	mMActor->setJointCallback(1, RollEnemyBodyCallback);
	mMActor->setBtkFromIndex(0);
	unk124->setGraph(gpConductor->getGraphByName("igaiga"));
}

void TIgaiga::moveObject()
{
	TWalkerEnemy::moveObject();

	f32 t = unk1CC - 0.0002f;
	if (t > 1.0f)
		t = 1.0f;
	else if (t < 0.5f)
		t = 0.5f;
	unk1CC = t;

	f32 attackRadius = getSaveParam2()->getSLAttackRadius();
	f32 attackHeight = getSaveParam2()->getSLAttackHeight();
	TSmallEnemyParams* params = getSaveParam2();
	f32 damageRadius = params->getSLDamageRadius();
	f32 damageHeight = params->getSLDamageHeight();

	f32 lo = unk154 * unk1CC;
	f32 hi = 3.0f * mBodyScale;
	f32 bodyScale = unk1E4 * lo;
	if (bodyScale > hi)
		bodyScale = hi;
	else if (bodyScale < lo)
		bodyScale = lo;
	mBodyScale = bodyScale;

	f32 ratio = mBodyScale / unk154;

	f32 scaleFactor = unk1CC * unk1E4;
	if (scaleFactor > 1.2f)
		scaleFactor = 1.2f;
	else if (scaleFactor < 1.0f)
		scaleFactor = 1.0f;

	mScaledBodyRadius = 8.0f * (mBodyScale * mBodyRadius) * scaleFactor;
	mScaling.z = mBodyScale;
	mScaling.y = mBodyScale;
	mScaling.x = mBodyScale;
	mAttackRadius = attackRadius * ratio;
	mAttackHeight = attackHeight * ratio;
	mDamageRadius = damageRadius * ratio;
	mDamageHeight = damageHeight * ratio;
	calcEntryRadius();
}

void TIgaiga::kill()
{
	unk194 = 0.0f;
	TSmallEnemy::kill();
}

const char** TIgaiga::getBasNameTable() const
{
	return igaiga_bastable;
}

void TIgaiga::reset()
{
	TRollEnemy::reset();
	setWalkAnm();
	offLiveFlag(LIVE_FLAG_UNK10);
	unk1B4 = 0;

	int min = 50;
	int max = 100;
	unk1B8 = (min + (int)((max - min) * (rand() * 0.000030517578f))) * 120;
	unk1BC  = true;

	mPosition.y += 20.0f;
	gpMap->checkGround(mPosition.x, mPosition.y + mHeadHeight, mPosition.z,
	                   &mGroundPlane);
	onLiveFlag(LIVE_FLAG_AIRBORNE);

	unk1E4 = 1.0f;
	unk1CC = 1.0f;
	unk1AC = 50.0f;
	unk1B0 = 500.0f;
	unk1E8 = 0;
}

void TIgaiga::behaveToWater(THitActor* actor)
{
	mSprayedByWaterCooldown = 0;

	TRollEnemySaveLoadParams* params = (TRollEnemySaveLoadParams*)unk1A4;
	if (unk1E4 < params->mSLExpandMax.get())
		unk1E4 *= params->mSLExpandRate.get();

	unk165 = true;
	if (mSpine->getCurrentNerve() != &TNerveIgaigaWaterHit::theNerve())
		mSpine->pushNerve(&TNerveIgaigaWaterHit::theNerve());
}

void TIgaiga::setWalkAnm()
{
	setBckAnm(3);
}

void TIgaiga::setDeadAnm()
{
	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
		unk1C0 = mPosition;
	} else {
		MtxPtr mtx = getModel()->getBaseTRMtx();
		unk1C0.x   = mtx[0][3];
		unk1C0.y   = mtx[1][3];
		unk1C0.z   = mtx[2][3];
	}

	gpMarioParticleManager->emit(0xcb, &unk1C0, 0, nullptr);

	if (unk1BC)
		setBckAnm(0);
	else
		setBckAnm(1);

	JGeometry::TVec3<f32> scale = mScaling;
	scale *= unk1CC * unk1E4;
	mPosition.y = mGroundHeight;
	if (scale.x > 1.5f)
		scale.x = 1.5f;
	else if (scale.x < 0.8f)
		scale.x = 0.8f;
	scale.z = scale.x;
	scale.y = scale.x;

	((TIgaigaManager*)mManager)
	    ->mPolluteModelManager->generatePolluteModel(mPosition, scale);
}

void TIgaiga::setMeltAnm()
{
	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
		unk1C0 = mPosition;
	} else {
		MtxPtr mtx = getModel()->getBaseTRMtx();
		unk1C0.x   = mtx[0][3];
		unk1C0.y   = mtx[1][3];
		unk1C0.z   = mtx[2][3];
	}

	if (!checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)
	    && SMS_IsMarioTouchGround4cm()) {
		if (mScaling.x > mBodyScale)
			SMSRumbleMgr->start(0x15, 0xa, &mPosition);
		else
			SMSRumbleMgr->start(0x14, 0xa, &mPosition);
	}

	JGeometry::TVec3<f32> scale = mScaling;
	scale *= 0.5f;
	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emit(0xa1, &unk1C0, 0, nullptr);
	if (emitter)
		emitter->setScale(scale);

	emitter = gpMarioParticleManager->emit(0xa2, &unk1C0, 0, nullptr);
	if (emitter)
		emitter->setScale(scale);

	setBckAnm(5);

	if (rand() * 0.000030517578f < 0.2f) {
		gpItemManager->makeObjAppear(mPosition.x, mPosition.y + 20.0f,
		                             mPosition.z, 0x20000002, true);
	}
}

bool TIgaiga::isHitValid(u32 message)
{
	unk1BC = true;
	if (message == 1)
		unk1BC = false;
	return true;
}

bool TIgaiga::isReachedToGoalXZ()
{
	JGeometry::TVec3<f32> diff = unk104.getPoint();
	diff.sub(mPosition);
	if (!unk1A8)
		diff.y = 0.0f;

	if (MsVECMag2((Vec*)&diff) < mReachNodeDist * mReachNodeDist)
		return true;
	return false;
}

void TIgaiga::walkBehavior(int mode, f32 speed)
{
	TRollEnemy::walkBehavior(mode, speed);

	f32 x = mLinearVelocity.x;
	f32 z = mLinearVelocity.z;
	if (unk1A8) {
		JGeometry::TVec3<f32> velocity = mVelocity;
		x                               = velocity.x;
		z                               = velocity.z;
	}

	f32 rollSpeed = JGeometry::TUtil<f32>::sqrt(x * x + z * z);
	unk194 += 100.0f * (rollSpeed / (mBodyRadius * unk1CC * unk1E4));

	if (unk1B4 != 0) {
		unk1B4++;
		if (unk1B4 > unk1B8)
			unk1B4 = 0;
	}

	if (!checkLiveFlag(LIVE_FLAG_AIRBORNE) && mGroundPlane) {
		const TLiveActor* actor = mGroundPlane->getActor();
		if (actor)
			((THitActor*)actor)->receiveMessage(this, HIT_MESSAGE_ATTACK);
	}

	if (unk138) {
		const TLiveActor* actor = unk138->getActor();
		if (actor)
			((THitActor*)actor)->receiveMessage(this, HIT_MESSAGE_ATTACK);
	}
}

void TIgaiga::bound()
{
	if (unk1A0 > 5.0f) {
		setBckAnm(2);
		if (!checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)
		    && SMS_IsMarioTouchGround4cm()) {
			if (mScaling.x > mBodyScale)
				SMSRumbleMgr->start(0x15, 0xa, &mPosition);
			else
				SMSRumbleMgr->start(0x14, 0xa, &mPosition);
		}
	}
}

bool TIgaiga::isRolling()
{
	const TNerveBase<TLiveActor>* current = mSpine->getCurrentNerve();
	return current == &TNerveIgaigaRollOnGraph::theNerve()
	    || current == &TNerveIgaigaShootFromCannon::theNerve()
	    || current == &TNerveIgaigaWaterHit::theNerve();
}

void TIgaiga::rollSE()
{
	gpMSound->startSoundActorSpecial(0x212f, &mPosition, mScaling.x,
	                                 mMarchSpeed, 0, nullptr, 0, 4);
}

void TIgaiga::boundSE()
{
	f32 volume = __fabsf(mGroundPlane->getNormal().y);
	if (gpMSound->gateCheck(0x28ad))
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x28ad, &mPosition, nullptr, volume, 0, 0, nullptr, 0, 4);
}

void TIgaiga::shoot(JGeometry::TVec3<f32>& velocity)
{
	mSpine->setNext(&TNerveIgaigaShootFromCannon::theNerve());
	unk1D8 = velocity;
	offLiveFlag(LIVE_FLAG_UNK10);
	unk1A8 = true;
}

void TGorogoro::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(flags, graphics);

	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)
	    && gpMirrorModelManager->isInMirror(mPosition)) {
		if (flags & 2) {
			calcRootMatrix();
			mMActor->calc();
		}

		if (flags & 4)
			mMActor->viewCalc();
	}
}

void TGorogoro::init(TLiveManager* manager)
{
	TRollEnemy::init(manager);
	mActorType = 0x10000019;
	unk150     = 0x31;
	unk64 &= 0xA7FFFFFF;
	mSpine->initWith(&TNerveGorogoroRollOnGraph::theNerve());
	unk1A4 = getSaveParam();
	mMActor->setJointCallback(1, RollEnemyBodyCallback);
	unk130    = 1;
	unk1ED[3] = 0xff;
}

void TGorogoro::calcRootMatrix()
{
	gpCurRollEnemy = this;

	if (mSpine->getCurrentNerve() == &TNerveGorogoroDie::theNerve()) {
		TSpineEnemy::calcRootMatrix();
		if (checkLiveFlag(LIVE_FLAG_UNK10000)) {
			unk1B4.ref(0, 3) = mPosition.x;
			unk1B4.ref(2, 3) = mPosition.z;
		}
		return;
	}

	if (isEaten())
		return;

	TRollEnemySaveLoadParams* params = (TRollEnemySaveLoadParams*)unk1A4;
	f32 groundOffsetY                = params->mSLGroundOffsetY.get();
	if (mPosition.y < 30.0f + mGroundHeight) {
		JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x175, getMActor()->getModel()->getAnmMtx(0), 1, this);
		if (emitter)
			emitter->setScale(mScaling);
	}

	f32 y = groundOffsetY * unk158 + mPosition.y;
	MsMtxSetXYZRPH(getModel()->getBaseTRMtx(), mPosition.x, y, mPosition.z,
	               mRotation.x, mRotation.y, mRotation.z);
	getModel()->setBaseScale(mScaling);
}

void TGorogoro::kill()
{
	unk194 = 0.0f;
	if (mSpine->getCurrentNerve() != &TNerveSmallEnemyDie::theNerve()
	    && mSpine->getCurrentNerve() != &TNerveGorogoroDie::theNerve()) {
		mSpine->setNext(&TNerveGorogoroDie::theNerve());
		onLiveFlag(LIVE_FLAG_UNK8);
	}
}

const char** TGorogoro::getBasNameTable() const
{
	return gorogoro_bastable;
}

void TGorogoro::reset()
{
	unk130 = 1;
	TRollEnemy::reset();
	offLiveFlag(0x1000);
	unk1ED[3] = 0xff;
	unk1AC    = -10.0f;
	unk1B0    = 1.0f;
	unk1E4 = false;
	unk1E8 = 0;
	unk1EC = false;
}

void TGorogoro::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor       = mMActorKeeper->createMActor("bosspaku_head.bmd", 3);
}

void TGorogoro::behaveToWater(THitActor* actor)
{
	TRollEnemy::behaveToWater(actor);

	u8 maxHp = getMaxHitPoints();
	if (maxHp == 0)
		maxHp = 1;

	unk1ED[3] = mHitPoints * 0xff / maxHp;
	if (mHitPoints < 2)
		mHitPoints = 1;

	JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x176, getModel()->getAnmMtx(1), 1, this);
	if (emitter)
		emitter->setScale(mScaling);
}

void TGorogoro::setDeadAnm()
{
	JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0xbf, getModel()->getAnmMtx(1), 0, nullptr);
	if (emitter)
		emitter->setScale(mScaling);

	setBckAnm(0);

	if (gpMSound->gateCheck(0x2856))
		MSoundSESystem::MSoundSE::startSoundActor(0x2856, &mPosition, 0,
		                                          nullptr, 0, 4);
}

void TGorogoro::setMeltAnm()
{
	setBckAnm(1);
	unk130 = 0;
	onLiveFlag(0x1000);
	PSMTXCopy(getModel()->getBaseTRMtx(), unk1B4.mMtx);
	unk1B4.ref(1, 3) = mGroundHeight;

	JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0xbe, unk1B4.mMtx, 0, nullptr);
	if (emitter)
		emitter->setScale(mScaling);

	if (gpMSound->gateCheck(0x2913))
		MSoundSESystem::MSoundSE::startSoundActor(0x2913, &mPosition, 0,
		                                          nullptr, 0, 4);
}

void TGorogoro::forceKill()
{
	if (!mGroundPlane->checkFlag(BG_CHECK_FLAG_ILLEGAL)
	    && mGroundPlane->isWaterSurface()
	    && !checkLiveFlag(LIVE_FLAG_AIRBORNE)) {
		if (mSpine->getCurrentNerve() != &TNerveGorogoroDie::theNerve()) {
			mSpine->setNext(&TNerveGorogoroDie::theNerve());
			onLiveFlag(LIVE_FLAG_UNK20000);
			onLiveFlag(LIVE_FLAG_UNK10000);
		}
	}
}

void TGorogoro::walkBehavior(int mode, f32 speed)
{
	if (mPosition.y > mGroundHeight)
		onLiveFlag(LIVE_FLAG_AIRBORNE);

	mTurnSpeed = getSaveParam2()->mSLTurnSpeedLow.get();
	TRollEnemy::walkBehavior(mode, speed);
	unk194 += 0.4f * mMarchSpeed;

	if (mSpine->getCurrentNerve() != &TNerveGorogoroDie::theNerve()
	    && mPosition.y < mGroundHeight + 10.0f && mGroundPlane->isWaterSurface()) {
		onLiveFlag(LIVE_FLAG_UNK10000);
		onLiveFlag(LIVE_FLAG_UNK20000);
		kill();
	}
}

void TGorogoro::flagJump()
{
	JGeometry::TVec3<f32> target;
	unk124->getGraph()
	    ->getGraphNode(unk124->mCurrIdx)
	    .getPoint((Vec*)&target);
	mPosition.y += 30.0f;
	mVelocity = calcVelocityToJumpToY(target, unk124->unkC, getGravityY());
	unk1A8    = true;
	onLiveFlag(LIVE_FLAG_AIRBORNE);
}

void TGorogoro::bound()
{
	((TGorogoroManager*)mManager)
	    ->mPolluteModelManager->generatePolluteModel(mPosition, mScaling);

	if (!checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)
	    && SMS_IsMarioTouchGround4cm())
		SMSRumbleMgr->start(0x15, 0xa, &mPosition);
}

bool TGorogoro::isRolling()
{
	return mSpine->getCurrentNerve() == &TNerveGorogoroRollOnGraph::theNerve()
	    || mCurrentBckAnm == 1;
}

void TGorogoro::rollSE()
{
	f32 volume = __fabsf(mGroundPlane->getNormal().y);
	if (gpMSound->gateCheck(0x2054))
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x2054, &mPosition, nullptr, volume, 0, 0, nullptr, 0, 4);
}

void TGorogoro::boundSE()
{
	f32 volume = __fabsf(mGroundPlane->getNormal().y);
	if (gpMSound->gateCheck(0x2844))
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x2844, &mPosition, nullptr, volume, 0, 0, nullptr, 0, 4);
}

void TGorogoro::generateByGateKeeper(const JGeometry::TVec3<f32>& position,
                                     const JGeometry::TVec3<f32>& velocity)
{
	reset();

	TGraphWeb* graph = unk124->getGraph();
	int nodeIndex    = graph->findNearestNodeIndex(position, -1);
	unk124->mCurrIdx = nodeIndex;
	unk124->mPrevIdx = nodeIndex - 1;

	TGraphNode& node = graph->getGraphNode(nodeIndex);
	BOOL aimAtMario  = MsIsInSight(position, velocity.y, *gpMarioPos, 2000.0f,
	                               360.0f, -1.0f);

	JGeometry::TVec3<f32> target;
	if (aimAtMario)
		target = *gpMarioPos;
	else
		node.getPoint((Vec*)&target);

	target.sub(position);
	mPosition = position;

	if (target.squared() > 0.0000038146973f) {
		PSVECNormalize((Vec*)&target, (Vec*)&target);

		f32 randFactor = rand() * 0.000030517578f;
		s16 angle      = (15.0f - 30.0f * randFactor) * (65536.0f / 360.0f);
		f32 sin        = JMASSin(angle);
		f32 cos        = JMASCos(angle);
		Mtx rotMtx     = {
			    { cos, 0.0f, sin, 0.0f },
			    { 0.0f, 1.0f, 0.0f, 0.0f },
			    { -sin, 0.0f, cos, 0.0f },
		};
		PSMTXMultVec(rotMtx, (Vec*)&target, (Vec*)&target);

		mRotation.y = MsAngleWrap(MsGetRotFromZaxisY(target));

		target.scale(1500.0f * (rand() * 0.000030517578f));
		target.add(position);
		mVelocity = calcVelocityToJumpToY(target, 15.0f, getGravityY());
	} else {
		mVelocity = calcVelocityToJumpToY(position, 15.0f, getGravityY());
		mRotation.y = MsAngleWrap(MsGetRotFromZaxisY(target));
	}

	if (aimAtMario) {
		unk114.push(unkF4);
		unkF4 = TPathNode(*gpMarioPos);
	}

	unk1A8 = true;
}

void TIgaigaPolluteModel::setAnm()
{
	unk10->unk18->setBckFromIndex(7);
	unk10->unk18->getFrameCtrl(0)->setFrame(0.0f);
}

void TGorogoroPolluteModel::setAnm()
{
	unk10->unk18->setBckFromIndex(3);
	unk10->unk18->getFrameCtrl(0)->setFrame(0.0f);
}

void TIgaigaPolluteModelManager::init(TLiveActor* owner)
{
	TEnemyPolluteModelManager::init(owner);

	void* res
	    = JKRFileLoader::getGlbResource("/scene/igaiga/stamp_igaiga_model1.bmd");
	SDLModelData* modelData
	    = new SDLModelData(J3DModelLoaderDataBase::load(res, 0x10210000));

	for (int i = 0; i < unk14; ++i)
		unk18[i] = new TIgaigaPolluteModel(owner, 0, modelData,
		                                    "イガイガ汚染モデル");
}

void TGorogoroPolluteModelManager::init(TLiveActor* owner)
{
	TEnemyPolluteModelManager::init(owner);

	void* res = JKRFileLoader::getGlbResource(
	    "/scene/gorogoro/bosspaku_head_stamp.bmd");
	SDLModelData* modelData
	    = new SDLModelData(J3DModelLoaderDataBase::load(res, 0x10210000));

	for (int i = 0; i < unk14; ++i)
		unk18[i] = new TGorogoroPolluteModel(owner, 0, modelData,
		                                      "汚染モデル");
}

TIgaigaManager::TIgaigaManager(const char* name)
    : TSmallEnemyManager(name)
    , unk64(nullptr)
    , mWaterEmitInfo(nullptr)
{
	gpCurRollEnemy = nullptr;
}

void TIgaigaManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38          = new TRollEnemySaveLoadParams("/enemy/igaiga.prm");
	mWaterEmitInfo = new TWaterEmitInfo("/enemy/igaigawater.prm");
}

void TIgaigaManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSmallEnemyManager::perform(flags, graphics);
	mPolluteModelManager->perform(flags, graphics);
}

void TIgaigaManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "igaiga_model1.bmd", 0x11240000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TSpineEnemy* TIgaigaManager::createEnemyInstance()
{
	return new TIgaiga("イガイガ");
}

void TIgaigaManager::initSetEnemies()
{
	mPolluteModelManager = new TIgaigaPolluteModelManager("イガイガモデル汚染");
	mPolluteModelManager->init(getObj(0));
}

TGorogoroManager::TGorogoroManager(const char* name)
    : TSmallEnemyManager(name)
    , unk60(0)
    , unk64(nullptr)
    , unk68(true)
    , mPolluteModelManager(nullptr)
    , unk70(nullptr)
{
}

void TGorogoroManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38 = new TRollEnemySaveLoadParams("/enemy/gorogoro.prm");
	createSharedMActorSet(anmlist);
}

void TGorogoroManager::loadAfter()
{
	unk64 = (TMapEventSink*)JDrama::TNameRefGen::getInstance()
	           ->getRootNameRef()
	           ->search("イベント（寝込むビアンコ）");
	unk70 = (TAreaCylinderManager*)gpConductor->search("ゴロゴロ発生マネージャー");
}

void TGorogoroManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		unk60 += 1;

		TRollEnemySaveLoadParams* params
		    = (TRollEnemySaveLoadParams*)getSaveParam();
		int interval = params->mSLGenerateInterval.get();
		if (unk60 > interval) {
			unk60 = 0;

			bool shouldGenerate = true;
			if (unk70)
				shouldGenerate = unk70->contain(*gpMarioPos);

			if (shouldGenerate) {
				for (int i = 0; i < getActiveObjNum(); ++i) {
					TGorogoro* enemy = (TGorogoro*)getObj(i);
					if (!enemy->checkLiveFlag(LIVE_FLAG_DEAD))
						continue;

					if (unk64) {
						if (unk64->isBuried(1)) {
							unk60 = interval;
							break;
						}

						if (unk68) {
							unk68 = false;

							enemy->reset();
							JGeometry::TVec3<f32> position;
							enemy->unk124->getGraph()
							    ->getGraphNode(10)
							    .getPoint((Vec*)&position);
							enemy->mPosition = position;
							enemy->unk124->mCurrIdx = 10;
							enemy->unk124->mPrevIdx = 9;

							JGeometry::TVec3<f32> target;
							enemy->unk124->getGraph()
							    ->getGraphNode(11)
							    .getPoint((Vec*)&target);
							JGeometry::TVec3<f32> dir = target;
							dir.sub(enemy->mPosition);
							enemy->mRotation.y
							    = MsAngleWrap(MsGetRotFromZaxisY(dir));
							enemy->setGoalPath(TPathNode(target));

							TGorogoro* other = (TGorogoro*)getObj(1);
							other->reset();
							other->unk124->getGraph()
							    ->getGraphNode(16)
							    .getPoint((Vec*)&position);
							other->mPosition = position;
							other->unk124->mCurrIdx = 16;
							other->unk124->mPrevIdx = 15;

							other->unk124->getGraph()
							    ->getGraphNode(17)
							    .getPoint((Vec*)&target);
							dir = target;
							dir.sub(other->mPosition);
							other->mRotation.y
							    = MsAngleWrap(MsGetRotFromZaxisY(dir));
							other->setGoalPath(TPathNode(target));
							break;
						}
					}

					enemy->reset();
					break;
				}
			}
		}
	}

	TEnemyManager::perform(flags, graphics);
	mPolluteModelManager->perform(flags, graphics);
}

void TGorogoroManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "bosspaku_head.bmd", 0x10300000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TSpineEnemy* TGorogoroManager::createEnemyInstance()
{
	return new TGorogoro("ゴロゴロ");
}

void TGorogoroManager::initSetEnemies()
{
	mPolluteModelManager = new TGorogoroPolluteModelManager("ゴロゴロモデル汚染");
	mPolluteModelManager->init(getObj(0));

	for (int i = 0; i < getObjNum(); ++i) {
		TGorogoro* enemy = (TGorogoro*)getObj(i);
		TGraphWeb* graph = gpConductor->getGraphByName(graphlist[i & 1]);
		if (graph->isDummy())
			graph = gpConductor->getGraphByName(graphlist[0]);

		if (!graph->isDummy()) {
			enemy->unk124->setGraph(graph);
			enemy->mPosition = graph->getFirstGraphNode().getPoint();
			enemy->unk1E8    = graph->getNodeNum() - 1;
		}
	}
}

int RollEnemyBodyCallback(J3DNode* node, int timing)
{
	if (timing != 0)
		return 1;

	if (!gpCurRollEnemy || !gpCurRollEnemy->isRolling())
		return 1;

	u16 jointNo = ((J3DJoint*)node)->getJntNo();
	MtxPtr jointMtx
	    = gpCurRollEnemy->getModel()->getAnmMtx(jointNo);

	Mtx rollMtx;
	s16 angle = gpCurRollEnemy->unk194 * (65536.0f / 360.0f);
	f32 s     = JMASSin(angle);
	f32 c     = JMASCos(angle);

	rollMtx[0][0] = 1.0f;
	rollMtx[0][1] = 0.0f;
	rollMtx[0][2] = 0.0f;
	rollMtx[0][3] = 0.0f;
	rollMtx[1][0] = 0.0f;
	rollMtx[1][1] = c;
	rollMtx[1][2] = -s;
	rollMtx[1][3] = 0.0f;
	rollMtx[2][0] = 0.0f;
	rollMtx[2][1] = s;
	rollMtx[2][2] = c;
	rollMtx[2][3] = 0.0f;

	jointMtx[1][3] += TRollEnemy::mTransYOffset;
	PSMTXConcat(jointMtx, rollMtx, jointMtx);
	PSMTXConcat(J3DSys::mCurrentMtx, rollMtx, J3DSys::mCurrentMtx);
	return 1;
}
