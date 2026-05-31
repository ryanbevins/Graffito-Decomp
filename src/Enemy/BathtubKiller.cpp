#include <Enemy/BathtubKiller.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/EffectObj.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <new>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

static const char* bathtubkiller_bastable[] = {
	"/scene/bathtubkiller/bas/bathtubdownkiller_down1.bas",
	nullptr,
	nullptr,
};

TBathtubKillerPersonality::TBathtubKillerPersonality() { }

void TBathtubKillerPersonality::makeFast(const TBathtubKillerParams*) { }

void TBathtubKillerPersonality::makeShine(const TBathtubKillerParams*) { }

void TBathtubKillerPersonality::makeNormal(const TBathtubKillerParams*) { }

TBathtubKillerParams::TBathtubKillerParams(const char* prm)
    : TSmallEnemyParams(prm)
    , PARAM_INIT(fastAccelerationQuatRate, 0.0f)
    , PARAM_INIT(fastChaseAcceleration, 5.0f)
    , PARAM_INIT(fastChaseSpeed, 40.0f)
    , PARAM_INIT(fastInitialSpeed, 15.0f)
    , PARAM_INIT(fastDeadPeriod, 1800)
    , PARAM_INIT(shineAccelerationQuatRate, 0.0f)
    , PARAM_INIT(shineChaseAcceleration, 5.0f)
    , PARAM_INIT(shineChaseSpeed, 40.0f)
    , PARAM_INIT(shineInitialSpeed, 15.0f)
    , PARAM_INIT(shineDeadPeriod, 1800)
    , PARAM_INIT(mushroomProbability, 0.0f)
    , PARAM_INIT(mSLColorChangeRateDelta, 0.0f)
    , PARAM_INIT(mSLAccelerationQuatRate, 0.0f)
    , PARAM_INIT(mSLChaseAcceleration, 5.0f)
    , PARAM_INIT(mSLChaseSpeed, 40.0f)
    , PARAM_INIT(mSLInitialSpeed, 15.0f)
    , PARAM_INIT(mSLDeadPeriod, 1800)
    , PARAM_INIT(mSLStraightDistance, 0.0f)
    , PARAM_INIT(mSLChaseMinY, 0.0f)
    , PARAM_INIT(mSLChaseMaxY, 0.0f)
    , PARAM_INIT(mSLAboidDistanceY, 0.0f)
    , PARAM_INIT(mSLAboidDistance, 0.0f)
    , PARAM_INIT(mSLChaseDistanceY, 5.0f)
    , PARAM_INIT(mSLChaseDistance, 5.0f)
    , PARAM_INIT(mSLTrampleVelocity, 40.0f)
    , PARAM_INIT(mSLFlyingSpeedMax, 0.0f)
    , PARAM_INIT(mSLFlyingGravityY, 5.0f)
    , PARAM_INIT(mSLBombRange, 1.0f)
    , PARAM_INIT(aboidAngle, 10.0f)
    , PARAM_INIT(mSLChaseStraightPeriod, 360)
    , PARAM_INIT(mSLSmokeInterval, 3)
    , PARAM_INIT(mSLLaunchingPeriod, 360)
    , PARAM_INIT(noCollisionAmongKillers, 360)
{
	TParams::load(mPrmPath);

	fastAccelerationQuatRate.set(0.03f);
	fastChaseAcceleration.set(0.1f);
	fastChaseSpeed.set(10.0f);
	fastInitialSpeed.set(14.0f);
	fastDeadPeriod.set(720);
	shineAccelerationQuatRate.set(0.03f);
	shineChaseAcceleration.set(0.03f);
	shineChaseSpeed.set(7.0f);
	shineInitialSpeed.set(12.0f);
	shineDeadPeriod.set(2400);
	mSLAccelerationQuatRate.set(0.03f);
	mSLChaseAcceleration.set(0.1f);
	mSLChaseSpeed.set(8.0f);
	mSLInitialSpeed.set(12.0f);
	mSLDeadPeriod.set(1440);
	mSLColorChangeRateDelta.set(0.16f);
	mSLStraightDistance.set(400.0f);
	mSLChaseMinY.set(50.0f);
	mSLChaseMaxY.set(100.0f);
	mSLAboidDistanceY.set(100.0f);
	mSLAboidDistance.set(100.0f);
	mSLChaseDistanceY.set(400.0f);
	mSLDamageRadius.set(120);
	mSLDamageHeight.set(100);
	mSLAttackRadius.set(100);
	mSLAttackHeight.set(90);
	mSLTrampleVelocity.set(500.0f);
	mSLFlyingSpeedMax.set(200.0f);
	mSLFlyingGravityY.set(0.06f);
	mSLBombRange.set(500.0f);
	aboidAngle.set(5.0f);
	mSLChaseStraightPeriod.set(420);
	mSLSmokeInterval.set(4);
	mSLLaunchingPeriod.set(240);
	noCollisionAmongKillers.set(480);
	mushroomProbability.set(0.3f);
}

TBathtubKiller::TBathtubKiller(const char* name)
    : TSmallEnemy(name)
{
	unk1CC = (TBathtub*)NULL;
}

void TBathtubKiller::init(TLiveManager* manager)
{
	TSmallEnemy::init(manager);
	mActorType = 0x08000024;
	unk150     = 0x11;
	onLiveFlag(LIVE_FLAG_UNK10);
	onLiveFlag(LIVE_FLAG_DEAD);
	onLiveFlag(LIVE_FLAG_UNK8);
	onHitFlag(HIT_FLAG_NO_COLLISION);
	unk194 = 0;
	resetBathtubKiller();
}

void TBathtubKiller::setMActorAndKeeper() { }

void TBathtubKiller::reset()
{
	TSmallEnemy::reset();
	offLiveFlag(LIVE_FLAG_DEAD);
	offLiveFlag(LIVE_FLAG_UNK8);
	offHitFlag(HIT_FLAG_NO_COLLISION);
	offHitFlag(HIT_FLAG_UNK2);
	offHitFlag(HIT_FLAG_UNK4);
	resetBathtubKiller();
}

#pragma dont_inline on
void TBathtubKiller::resetBathtubKiller()
{
	mSpine->initWith(&TNerveBathtubKillerWander::theNerve());
	onLiveFlag(LIVE_FLAG_AIRBORNE);

	unk208 = 0;
	unk20C = 0;
	unk210 = 0;
	unk214 = 0;
	unk218 = 0;

	mQuat.x = 0.0f;
	mQuat.y = 0.0f;
	mQuat.z = 0.0f;
	mQuat.w = 1.0f;
	mVelocity.set(0.0f, 0.0f, 0.0f);
	unk1BC.set(0.0f, 0.0f, 0.0f);
	unk21C = 0;
	unk1D4 = 0;

	if (unk194 == 1) {
		unk1D8.r = 50;
		unk1D8.g = 70;
		unk1D8.b = 160;
		unk1D8.a = 0;
		unk1E0   = unk1D8;
		unk1E8   = unk1D8;
		unk1F0   = unk1D8;

		TBathtubKillerParams* p = getSaveParam2();
		unk198                  = p->shineAccelerationQuatRate.value;
		unk19C                  = p->shineChaseAcceleration.value;
		unk1A0                  = p->shineChaseSpeed.value;
		unk1A4                  = p->shineInitialSpeed.value;
		unk1A8                  = p->shineDeadPeriod.value;
	} else {
		unk1D8.r = 0;
		unk1D8.g = 0;
		unk1D8.b = 0;
		unk1D8.a = 0;
		unk1E0   = unk1D8;
		unk1E8   = unk1D8;
		unk1F0   = unk1D8;

		if (unk194 == 2) {
			TBathtubKillerParams* p = getSaveParam2();
			unk198                  = p->fastAccelerationQuatRate.value;
			unk19C                  = p->fastChaseAcceleration.value;
			unk1A0                  = p->fastChaseSpeed.value;
			unk1A4                  = p->fastInitialSpeed.value;
			unk1A8                  = p->fastDeadPeriod.value;
		} else {
			TBathtubKillerParams* p = getSaveParam2();
			unk198                  = p->mSLAccelerationQuatRate.value;
			unk19C                  = p->mSLChaseAcceleration.value;
			unk1A0                  = p->mSLChaseSpeed.value;
			unk1A4                  = p->mSLInitialSpeed.value;
			unk1A8                  = p->mSLDeadPeriod.value;
		}
	}

	unk1FC = 0.0f;
	unk1F8 = getSaveParam2()->mSLColorChangeRateDelta.value;
	unk208 = unk1A8;
	unk20C = getSaveParam2()->mSLLaunchingPeriod.value;
	unk214 = getSaveParam2()->noCollisionAmongKillers.value;
	unk200 = getSaveParam2()->mSLChaseMinY.value;
	unk204 = getSaveParam2()->mSLChaseMaxY.value;

	if (unk194 == 2) {
		int sel = (int)(MsRandF() * 4.0f);
		f32 off = 0.0f;
		if (sel == 0)
			off = 120.0f;
		else if (sel == 1)
			off = 240.0f;
		unk200 += off;
		unk204 += off;
	}
}
#pragma dont_inline off

void TBathtubKiller::generateItemBathtubKiller() { }

void TBathtubKiller::killBathtubKiller()
{
	mMActor = mMActorKeeper->getMActor("bathtubdownkiller_model1.bmd");
	setBckAnm(0);

	mQuat.x = 0.0f;
	mQuat.y = 0.0f;
	mQuat.z = 0.0f;
	mQuat.w = 1.0f;
	unk1BC.zero();

	JGeometry::TVec3<f32> vel;
	vel.set(0, 0, 0);
	setVelocity(vel);

	onLiveFlag(LIVE_FLAG_UNK8);
	unk1E0 = unk1D8;
}

void TBathtubKiller::breakBathtubKiller()
{
	killBathtubKiller();
	generateItemBathtubKiller();
	onHitFlag(HIT_FLAG_NO_COLLISION);
}

void TBathtubKiller::explodeBathtubKiller()
{
	killBathtubKiller();

	TEffectExplosion* effect = (TEffectExplosion*)gpConductor->makeOneEnemyAppear(
	    mPosition, "エフェクト爆発マネージャー", 1);
	if (effect)
		effect->generate(mPosition, mScaling);

	onHitFlag(HIT_FLAG_NO_COLLISION);
}

void TBathtubKiller::bind() { }

void TBathtubKiller::perform(u32, JDrama::TGraphics*) { }

void TBathtubKiller::makeNoseColor() { }

f32 TBathtubKiller::getBathtubY() { return 0.0f; }

void TBathtubKiller::makeInitialVelocity(JGeometry::TVec3<f32>) { }

void TBathtubKiller::moveParabolic() { }

void TBathtubKiller::moveChasing() { }

void TBathtubKiller::moveStraight() { }

void TBathtubKiller::makeVelocityQuat() { }

void TBathtubKiller::makeAccelerationQuat() { }

void TBathtubKiller::makeQuat(JGeometry::TVec3<f32> axis, f32 moveAmountY,
                              f32 moveAmountX)
{
	JGeometry::TVec3<f32> normAxis = axis;
	normAxis.normalize();

	JGeometry::TVec3<f32> forward;
	mQuat.getZDir(forward);

	JGeometry::TVec3<f32> up;
	mQuat.getYDir(up);

	JGeometry::TQuat4<f32> steer;
	steer.setRotate(forward, normAxis, moveAmountY);
	mQuat.mul(steer);

	// Y-axis rotation
	JGeometry::TVec3<f32> right;
	right.cross(forward, JGeometry::TVec3<f32>(0.0f, 1.0f, 0.0f));
	if (right.length() > 0.0f) {
		right.normalize();

		JGeometry::TQuat4<f32> tiltQuat;
		tiltQuat.setRotate(right, M_PI / 2.0f);

		JGeometry::TVec3<f32> curUp;
		tiltQuat.rotate(forward, curUp);

		steer.setRotate(up, curUp, moveAmountX);
		mQuat.mul(steer);
	}

	mQuat.normalize();
}

void TBathtubKiller::makeScrewQuat(JGeometry::TVec3<f32>, f32, f32) { }

f32 TBathtubKiller::getGravityY() const
{
	return getSaveParam2()->mSLFlyingGravityY.value;
}

void TBathtubKiller::calcRootMatrix()
{
	Mtx m;
	((JGeometry::TRotation3<
	    JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >*)&m)
	    ->setQuat(mQuat);
	m[0][3] = mPosition.x;
	m[1][3] = mPosition.y;
	m[2][3] = mPosition.z;
	getModel()->setBaseScale(mScaling);
	PSMTXCopy(m, getModel()->getBaseTRMtx());
}

BOOL TBathtubKiller::receiveMessage(THitActor*, u32) { return false; }

void TBathtubKiller::attackToMario()
{
	if (mSpine->getCurrentNerve()
	        != &TNerveBathtubKillerExplosion::theNerve()
	    && mSpine->getCurrentNerve() != &TNerveBathtubKillerBreak::theNerve()
	    && gpMarioPos->y < mPosition.y) {
		mSpine->pushNerve(&TNerveBathtubKillerExplosion::theNerve());
		SMS_SendMessageToMario(this, 0xe);
		SMS_ThrowMario(JGeometry::TVec3<f32>(0.0f, 1.0f, 0.0f), 60.0f);
		unk21C = 1;
	}
}

bool TBathtubKiller::isCollidMove(THitActor*) { return false; }

void TBathtubKiller::behaveToWater(THitActor*)
{
	if (mSpine->getCurrentNerve()
	        != &TNerveBathtubKillerExplosion::theNerve()
	    && mSpine->getCurrentNerve()
	           != &TNerveBathtubKillerBreak::theNerve())
		mSpine->pushNerve(&TNerveBathtubKillerBreak::theNerve());
}

const char** TBathtubKiller::getBasNameTable() const
{
	return bathtubkiller_bastable;
}

void TBathtubKiller::setNormalBathtubKillerAnm() { }

void TBathtubKiller::setChaseBathtubKillerAnm() { }

void TBathtubKiller::setStraightBathtubKillerAnm() { }

void TBathtubKiller::setDeadBathtubKillerAnm() { }

void TBathtubKiller::updateTimers() { }

bool TBathtubKiller::isAttackable() { return false; }

bool TBathtubKiller::isAboided() { return false; }

bool TBathtubKiller::canChase() { return false; }

void TBathtubKiller::generateExplosion() { }

DEFINE_NERVE(TNerveBathtubKillerWander, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveBathtubKillerChase, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveBathtubKillerChaseStraight, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveBathtubKillerStraight, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveBathtubKillerBreak, TLiveActor)
{
	TBathtubKiller* self = (TBathtubKiller*)spine->getBody();
	if (spine->getTime() == 0)
		self->breakBathtubKiller();

	if (self->checkCurAnmEnd(0)) {
		self->unk21C = 0;
		self->onLiveFlag(LIVE_FLAG_DEAD);
		self->stopAnmSound();
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBathtubKillerExplosion, TLiveActor)
{
	TBathtubKiller* self = (TBathtubKiller*)spine->getBody();
	if (spine->getTime() == 0)
		self->explodeBathtubKiller();

	if (self->checkCurAnmEnd(0)) {
		self->unk21C = 0;
		self->onLiveFlag(LIVE_FLAG_DEAD);
		self->stopAnmSound();
		return TRUE;
	}
	return FALSE;
}

TBathtubKillerManager::TBathtubKillerManager(const char* name)
    : TSmallEnemyManager(name)
{
}

void TBathtubKillerManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38 = new TBathtubKillerParams("/enemy/bathtubkiller.prm");
}

void TBathtubKillerManager::loadAfter() { }

void TBathtubKillerManager::generateMushroom(JGeometry::TVec3<f32>) { }

int TBathtubKillerManager::countActiveKillers()
{
	int count = 0;
	for (int i = 0;
	     i < (unk38 == NULL ? mObjNum
	                        : (unk38->mSLActiveEnemyNum.value > mObjNum
	                               ? mObjNum
	                               : unk38->mSLActiveEnemyNum.value));
	     i++) {
		if (!(((TLiveActor*)unk18[i])->mLiveFlag & 1))
			count++;
	}
	return count;
}

int TBathtubKillerManager::countActiveShineKillers() { return 0; }

void TBathtubKillerManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "bathtubkiller_model1.bmd", 0x50230000, 0 },
		{ "bathtubdownkiller_model1.bmd", 0x50210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TSpineEnemy* TBathtubKillerManager::createEnemyInstance()
{
	TBathtubKiller* k = new TBathtubKiller;
	new (&k->unk220) TMtx34f();
	return k;
}
