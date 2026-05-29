#include <Enemy/Kazekun.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Player/MarioAccess.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/MtxUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/Particles.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/ObjModel.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/JGeometry.hpp>
#include <JSystem/JGeometry/JGQuat4.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <dolphin/mtx.h>

// rogue includes needed for matching sinit & rodata
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

const char* Kazekun_bastable[] = {
	"/scene/Kazekun/bas/kazekun_appear.bas",
	"/scene/Kazekun/bas/kazekun_attack.bas",
	nullptr,
	"/scene/Kazekun/bas/kazekun_vanish.bas",
	"/scene/Kazekun/bas/kazekun_wait.bas",
};

// ============= nerves =============

DEFINE_NERVE(TNerveKazekunHitWater, TLiveActor)
{
	TKazekun* self = (TKazekun*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mMActor->setBck("kazekun_hit");
		self->setCurAnmSound();
		if (gpMSound->gateCheck(0x291d)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x291d, &self->mPosition, 0, nullptr, 0, 4);
		}
	}

	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveKazekunDisappear::theNerve());
		self->mWaitLimit = self->getKazekunParam()->mResetTimeHitting.get();
		return TRUE;
	}

	self->mVelocity.set(0.0f, 0.0f, 0.0f);
	return FALSE;
}

DEFINE_NERVE(TNerveKazekunWait, TLiveActor)
{
	TKazekun* self = (TKazekun*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mLiveFlag |= 0xa;
		self->setAnmSound(nullptr);
	}

	if (self->mWaitLimit < spine->getTime()) {
		spine->pushAfterCurrent(&TNerveKazekunSearch::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveKazekunDisappear, TLiveActor)
{
	TKazekun* self = (TKazekun*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mMActor->setBck("kazekun_vanish");
		self->setCurAnmSound();
		gpMarioParticleManager->emit(0xcf, &self->mPosition, 0, nullptr);
		self->mVelocity.set(0.0f, 0.0f, 0.0f);
		self->unk64 |= 0x1;
	}

	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveKazekunWait::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveKazekunSearch, TLiveActor)
{
	TKazekun* self = (TKazekun*)spine->getBody();

	if (spine->getTime() == 0)
		self->reset();

	self->updateSquareToMario();

	f32 appear = self->getKazekunParam()->mAppearDist.get();
	if (self->mDistToMarioSquared <= appear * appear) {
		spine->pushAfterCurrent(&TNerveKazekunAppear::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveKazekunAppear, TLiveActor)
{
	TKazekun* self = (TKazekun*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mLiveFlag &= ~0xa;
		gpMarioParticleManager->emit(0xcf, &self->mPosition, 0, nullptr);
		self->mMActor->setBck("kazekun_appear");
		self->setCurAnmSound();
	}

	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveKazekunTurn::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveKazekunTurn, TLiveActor)
{
	TKazekun* self = (TKazekun*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mMActor->setBck("kazekun_wait");
		self->setCurAnmSound();
		self->unk64 &= ~0x1;
	}

	self->flyAroundMario();

	f32 dy            = gpMarioPos->y - self->mHomePos.y;
	TKazekunParams* p = self->getKazekunParam();
	bool lost         = true;
	if (!(dy < -p->mLostOffsetYDown.get())) {
		if (!(p->mLostOffsetYUp.get() < dy))
			lost = false;
	}

	if (lost) {
		spine->pushAfterCurrent(&TNerveKazekunDisappear::theNerve());
		return TRUE;
	}

	if (p->mAroundTime.get() < spine->getTime()) {
		spine->pushAfterCurrent(&TNerveKazekunPreAttack::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveKazekunPreAttack, TLiveActor)
{
	TKazekun* self = (TKazekun*)spine->getBody();

	if (spine->getTime() == 0) {
		self->doAttackPose(true);
		if (gpMSound->gateCheck(0x28b6)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x28b6, &self->mPosition, 0, nullptr, 0, 4);
		}
		self->setAnmSound(nullptr);
	}

	TKazekunParams* p = self->getKazekunParam();
	if (p->mPoseTime.get() * p->mDicideTiming.get() < spine->getTime()) {
		// TODO(INVESTIGATION): capture Mario position into target path nodes
		// (self->unkF4 / self->unk104). See notes/Kazekun.md.
	}

	self->doAttackPose(false);

	if (p->mPoseTime.get() < spine->getTime()) {
		spine->pushAfterCurrent(&TNerveKazekunAttack::theNerve());
		return TRUE;
	}
	return FALSE;
}

// TODO(INVESTIGATION): math-heavy quaternion aim/move. Honest partial impl --
// sets the attack anim. See notes/Kazekun.md for the full asm decode plan.
DEFINE_NERVE(TNerveKazekunAttack, TLiveActor)
{
	TKazekun* self = (TKazekun*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mMActor->setBck("kazekun_attack");
		self->setCurAnmSound();
	}
	return FALSE;
}

// ============= TKazekun =============

TKazekun::TKazekun(const char* name)
    : TSmallEnemy(name)
{
	mWaitLimit = 0;
	mLiveFlag |= 0x10;
}

void TKazekun::init(TLiveManager* manager)
{
	mManager = manager;
	mManager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor = mMActorKeeper->createMActor(
	    "\x83\x4A\x83\x5A\x83\x4E\x83\x93", 0); // "Kazekun"

	mSpine->initWith(&TNerveKazekunSearch::theNerve());

	mHeadHeight       = 40.0f;
	mBodyRadius       = 50.0f;
	mScaledBodyRadius = 50.0f;
	initHitActor(0x10000029, 0x8000, 0, mBodyScale * mHeadHeight,
	             mBodyScale * mBodyRadius, 0.0f, 0.0f);
	unk64 |= 0x1;

	if (!gParticleFlagLoaded[0xcf]) {
		gpResourceManager->load("/scene/kazekun/jpa/ms_kaze_appear.jpa", 0xcf);
		gParticleFlagLoaded[0xcf] = 1;
	}
	if (!gParticleFlagLoaded[0x189]) {
		gpResourceManager->load("/scene/kazekun/jpa/ms_kaze_wind.jpa", 0x189);
		gParticleFlagLoaded[0x189] = 1;
	}
	if (!gParticleFlagLoaded[0x18a]) {
		gpResourceManager->load("/scene/kazekun/jpa/ms_kaze_blur.jpa", 0x18a);
		gParticleFlagLoaded[0x18a] = 1;
	}

	initAnmSound();

	mHomePos.set(mPosition.x, mPosition.y, mPosition.z);
	reset();
}

void TKazekun::reset()
{
	mQuat.set(0.0f, 0.0f, 0.0f, 1.0f);
	JGeometry::TVec3<f32> home(mHomePos);
	mPosition.set(home);
	mLiveFlag |= 0xa;
	setAnmSound(nullptr);
}

void TKazekun::bind()
{
	mLinearVelocity.x += mVelocity.x;
	mLinearVelocity.y += mVelocity.y;
	mLinearVelocity.z += mVelocity.z;
}

void TKazekun::calcRootMatrix()
{
	if (mHolder != nullptr) {
		TSpineEnemy::calcRootMatrix();
		return;
	}

	Mtx m;
	f32 x  = mQuat.x;
	f32 y  = mQuat.y;
	f32 z  = mQuat.z;
	f32 w  = mQuat.w;
	f32 x2 = 2.0f * x;
	f32 y2 = 2.0f * y;
	f32 z2 = 2.0f * z;
	f32 w2 = 2.0f * w;
	m[0][0] = 1.0f - (y2 * y) - (z2 * z);
	m[0][1] = (x2 * y) - (w2 * z);
	m[0][2] = (x2 * z) + (w2 * y);
	m[1][0] = (x2 * y) + (w2 * z);
	m[1][1] = 1.0f - (x2 * x) - (z2 * z);
	m[1][2] = (y2 * z) - (w2 * x);
	m[2][0] = (x2 * z) - (w2 * y);
	m[2][1] = (y2 * z) + (w2 * x);
	m[2][2] = 1.0f - (x2 * x) - (y2 * y);
	m[0][3] = mPosition.x;
	m[1][3] = mPosition.y;
	m[2][3] = mPosition.z;
	PSMTXCopy(m, getModel()->getBaseTRMtx());

	TNerveBase<TLiveActor>* latest = mSpine->getLatestNerve();
	bool active = (latest == &TNerveKazekunPreAttack::theNerve())
	              || (latest == &TNerveKazekunAttack::theNerve())
	              || (latest == &TNerveKazekunHitWater::theNerve());

	if (active) {
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x189, getModel()->getBaseTRMtx(), 1, this);
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x18a, getModel()->getBaseTRMtx(), 1, this);
	}
}

const char** TKazekun::getBasNameTable() const { return Kazekun_bastable; }

void TKazekun::setDeadAnm()
{
	mMActor->getFrameCtrl(1)->init(1);
	mMActor->getFrameCtrl(0)->setFrame(0.0f);
}

bool TKazekun::isCollidMove(THitActor*) { return false; }

void TKazekun::attackToMario()
{
	TNerveBase<TLiveActor>* latest = mSpine->getLatestNerve();
	if (latest == &TNerveKazekunTurn::theNerve()
	    || latest == &TNerveKazekunPreAttack::theNerve()
	    || latest == &TNerveKazekunAttack::theNerve()) {
		SMS_SendMessageToMario(this, 0xe);
	}
}

void TKazekun::behaveToWater(THitActor*)
{
	TNerveBase<TLiveActor>* latest = mSpine->getLatestNerve();
	if (latest == &TNerveKazekunTurn::theNerve()
	    || latest == &TNerveKazekunPreAttack::theNerve()
	    || latest == &TNerveKazekunAttack::theNerve()) {
		mSpine->reset();
		mSpine->setNext(&TNerveKazekunHitWater::theNerve());
	}
}

// TODO(INVESTIGATION): math-heavy. Honest stub -- see notes/Kazekun.md.
void TKazekun::doAttackPose(bool) { }

// TODO(INVESTIGATION): math-heavy orbit. Honest stub -- see notes/Kazekun.md.
void TKazekun::flyAroundMario() { }

// ============= TKazekunManager =============

TKazekunManager::TKazekunManager(const char* name)
    : TSmallEnemyManager(name)
{
}

void TKazekunManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "kazekun.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TKazekunManager::load(JSUMemoryInputStream& in)
{
	TKazekunParams* params = new TKazekunParams("/enemy/kazekun.prm");
	unk38                  = params;
	params->mSLAttackRadius.set(50);
	params->mSLAttackHeight.set(40);
	params->mSLDamageRadius.set(50);
	params->mSLDamageHeight.set(40);
	TSmallEnemyManager::load(in);
	unk5C = 0;
}

// ============= TKazekunParams =============

TKazekunParams::TKazekunParams(const char* prm)
    : TSmallEnemyParams(prm)
    , PARAM_INIT(mAppearDist, 1000.0f)
    , PARAM_INIT(mAroundDist, 400.0f)
    , PARAM_INIT(mAroundSpeed, 30.0f)
    , PARAM_INIT(mAroundTime, 600)
    , PARAM_INIT(mAttackSpeed, 30.0f)
    , PARAM_INIT(mAirFric, 0.97f)
    , PARAM_INIT(mResetTime, 300)
    , PARAM_INIT(mResetTimeHitting, 1500)
    , PARAM_INIT(mPoseTime, 120)
    , PARAM_INIT(mDicideTiming, 0.1f)
    , PARAM_INIT(mTurnOffsetY, 200.0f)
    , PARAM_INIT(mLostOffsetYUp, 500.0f)
    , PARAM_INIT(mLostOffsetYDown, 500.0f)
    , PARAM_INIT(mPoseSpeed, 7.6f)
    , PARAM_INIT(mPoseOmegaRate, 0.04f)
{
	TParams::load(mPrmPath);
}
