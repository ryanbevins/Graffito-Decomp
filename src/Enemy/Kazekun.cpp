#define JGEOMETRY_KAZEKUN_OWNER_HELPERS

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

static inline JGeometry::TVec3<f32> makeVec3(f32 x, f32 y, f32 z)
{
	return JGeometry::TVec3<f32>(x, y, z);
}

static inline JGeometry::TVec3<f32> getYDirVec(const TPosition3f& mtx)
{
	return JGeometry::TVec3<f32>(mtx.at(0, 1), mtx.at(1, 1), mtx.at(2, 1));
}

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

	JGeometry::TVec3<f32> v(0.0f, 0.0f, 0.0f);
	self->mVelocity = v;
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
		JGeometry::TVec3<f32> v(0.0f, 0.0f, 0.0f);
		self->mVelocity = v;
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
	bool lost;
	TKazekun* self = (TKazekun*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mMActor->setBck("kazekun_wait");
		self->setCurAnmSound();
		self->unk64 &= ~0x1;
	}

	self->flyAroundMario();

	f32 dy = gpMarioPos->y - self->mHomePos.y;
	lost   = true;
	if (!(dy < -self->getKazekunParam()->mLostOffsetYDown.get())) {
		if (!(self->getKazekunParam()->mLostOffsetYUp.get() < dy))
			lost = false;
	}

	if (lost) {
		spine->pushAfterCurrent(&TNerveKazekunDisappear::theNerve());
		return TRUE;
	}

	if ((f32)self->getKazekunParam()->mAroundTime.get()
	    < (f32)spine->getTime()) {
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

	if (self->getKazekunParam()->mPoseTime.get()
	        * self->getKazekunParam()->mDicideTiming.get()
	    < spine->getTime()) {
		self->setGoalPath(TPathNode(JGeometry::TVec3<f32>(*gpMarioPos)));
	}

	self->doAttackPose(false);

	if (self->getKazekunParam()->mPoseTime.get() < spine->getTime()) {
		spine->pushAfterCurrent(&TNerveKazekunAttack::theNerve());
		return TRUE;
	}
	return FALSE;
}

// Dive at the captured goal point: on entry, aim the velocity at the target and
// set its magnitude to mAttackSpeed; every frame slerp the facing quaternion
// toward the velocity direction, apply air friction, and drop to Disappear once
// the speed bleeds below 1.0. NOTE(INVESTIGATION): the velocity setLength + the
// friction/Disappear tail are byte-decoded; the quaternion aim/slerp/normalize
// middle hits the same frame-size/inline cascade as flyAroundMario/doAttackPose
// (lands low fuzzy). See notes/Kazekun.md.
DEFINE_NERVE(TNerveKazekunAttack, TLiveActor)
{
	TKazekun* self = (TKazekun*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mMActor->setBck("kazekun_attack");
		self->setCurAnmSound();

		JGeometry::TVec3<f32> dir(self->unk104.getPoint());
		dir.x -= self->mPosition.x;
		dir.y -= self->mPosition.y;
		dir.z -= self->mPosition.z;
		dir.setLength(self->getKazekunParam()->mAttackSpeed.get());
		self->mVelocity = dir;
	}

	TPosition3f mtx;
	JGeometry::TVec3<f32> up = makeVec3(0.0f, 1.0f, 0.0f);
	SMS_CalcToDirMatrix(mtx, self->mVelocity, up);

	JGeometry::TQuat4<f32> aim;
	mtx.getQuat(aim);
	JGeometry::TVec3<f32> axis = getYDirVec(mtx);
	JGeometry::TQuat4<f32> rot;
	rot.setRotate(axis, 0.0f);
	aim.mul(rot, aim);

	JGeometry::TQuat4<f32> cur;
	cur = self->mQuat;
	cur.slerp(aim, 0.1f);
	cur.normalize();
	self->mQuat = cur;

	self->mVelocity.scale(self->getKazekunParam()->mAirFric.get());
	if (self->mVelocity.dot(self->mVelocity) < 1.0f) {
		spine->pushAfterCurrent(&TNerveKazekunDisappear::theNerve());
		self->mWaitLimit = self->getKazekunParam()->mResetTime.get();
		return TRUE;
	}
	return FALSE;
}

template <>
void JGeometry::TRotation3<
    JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >::getQuat(
    JGeometry::TQuat4<f32>& quat) const
{
	if (this->at(0, 0) + this->at(1, 1) + this->at(2, 2) >= 0.0f) {
		f32 scale = TUtil<f32>::sqrt(this->at(0, 0) + this->at(1, 1)
		                             + this->at(2, 2) + 1.0f);
		quat.w    = 0.5f * scale;
		quat.x    = 0.5f / scale * (this->at(2, 1) - this->at(1, 2));
		quat.y    = 0.5f / scale * (this->at(0, 2) - this->at(2, 0));
		quat.z    = 0.5f / scale * (this->at(1, 0) - this->at(0, 1));
		return;
	}

	f32 maxDiag = max(max(this->at(0, 0), this->at(1, 1)), this->at(2, 2));

	if (maxDiag == this->at(0, 0)) {
		f32 scale = TUtil<f32>::sqrt(
		    this->at(0, 0) - (this->at(1, 1) + this->at(2, 2)) + 1.0f);
		quat.x = 0.5f * scale;
		quat.y = 0.5f / scale * (this->at(0, 1) + this->at(1, 0));
		quat.z = 0.5f / scale * (this->at(2, 0) + this->at(0, 2));
		quat.w = 0.5f / scale * (this->at(2, 1) - this->at(1, 2));
		return;
	}

	if (maxDiag == this->at(1, 1)) {
		f32 scale = TUtil<f32>::sqrt(
		    this->at(1, 1) - (this->at(2, 2) + this->at(0, 0)) + 1.0f);
		quat.y = 0.5f * scale;
		quat.z = 0.5f / scale * (this->at(1, 2) + this->at(2, 1));
		quat.x = 0.5f / scale * (this->at(0, 1) + this->at(1, 0));
		quat.w = 0.5f / scale * (this->at(0, 2) - this->at(2, 0));
		return;
	}

	f32 scale = TUtil<f32>::sqrt(
	    this->at(2, 2) - (this->at(0, 0) + this->at(1, 1)) + 1.0f);
	quat.z = 0.5f * scale;
	quat.x = 0.5f / scale * (this->at(2, 0) + this->at(0, 2));
	quat.y = 0.5f / scale * (this->at(1, 2) + this->at(2, 1));
	quat.w = 0.5f / scale * (this->at(1, 0) - this->at(0, 1));
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
	mMActor = mMActorKeeper->createMActor("kazekun.bmd", 0);

	mSpine->initWith(&TNerveKazekunSearch::theNerve());

	mHeadHeight       = 40.0f;
	mBodyRadius       = 50.0f;
	mScaledBodyRadius = 50.0f;
	initHitActor(0x10000029, 1, 0x80000000, mBodyScale * mBodyRadius,
	             mBodyScale * mHeadHeight, mBodyScale * mBodyRadius,
	             mBodyScale * mHeadHeight);
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

	mHomePos.x = mPosition.x;
	mHomePos.y = mPosition.y;
	mHomePos.z = mPosition.z;
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
	if (isTaken()) {
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
	f32 yy = y2 * y;
	f32 zz = z2 * z;
	f32 xy = x2 * y;
	f32 wz = w2 * z;
	f32 xx = x2 * x;
	f32 xz = x2 * z;
	f32 wy = w2 * y;
	f32 yz = y2 * z;
	f32 wx = w2 * x;
	m[0][0] = 1.0f - yy - zz;
	m[0][1] = xy - wz;
	m[0][2] = xz + wy;
	m[1][0] = xy + wz;
	m[1][1] = 1.0f - xx - zz;
	m[1][2] = yz - wx;
	m[2][0] = xz - wy;
	m[2][1] = yz + wx;
	m[2][2] = 1.0f - xx - yy;
	m[0][3] = mPosition.x;
	m[1][3] = mPosition.y;
	m[2][3] = mPosition.z;
	PSMTXCopy(m, getModel()->getBaseTRMtx());

	bool active = (mSpine->getLatestNerve() == &TNerveKazekunTurn::theNerve())
	              || (mSpine->getLatestNerve() == &TNerveKazekunPreAttack::theNerve())
	              || (mSpine->getLatestNerve() == &TNerveKazekunAttack::theNerve())
	              || (mSpine->getLatestNerve() == &TNerveKazekunHitWater::theNerve());

	if (active) {
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x189, getModel()->getBaseTRMtx(), 1, this);
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x18a, getModel()->getBaseTRMtx(), 1, this);
	}
}

// File-scope helper: build a matrix whose Z axis points along `dir`, using
// `up` to resolve the X/Y axes. The original game emits this as the single
// global definition (wireTrap/fireWanwan only call it). Each normalize is the
// `if (v.isZero()) v.set(default); else v.setLength(v, 1.0f);` idiom: squared()
// is computed once and compared twice, and setLength's internal zero() check
// becomes the (dead) middle branch.
void SMS_CalcToDirMatrix(TPosition3f& mtx,
                         const JGeometry::TVec3<f32>& dir,
                         const JGeometry::TVec3<f32>& up)
{
	JGeometry::TVec3<f32> z(dir);
	if (z.isZero())
		z.set(0.0f, 0.0f, 1.0f);
	else
		z.setLength(z, 1.0f);

	JGeometry::TVec3<f32> x;
	x.cross(up, z);
	if (x.isZero())
		x.set(1.0f, 0.0f, 0.0f);
	else
		x.setLength(x, 1.0f);

	JGeometry::TVec3<f32> y;
	y.cross(z, x);
	y.setLength(y, 1.0f);

	mtx.setXDir(x);
	mtx.setYDir(y);
	mtx.setZDir(z);
}

const char** TKazekun::getBasNameTable() const { return Kazekun_bastable; }

void TKazekun::setDeadAnm()
{
	mMActor->getFrameCtrl(0)->init(1);
	mMActor->getFrameCtrl(0)->setFrame(0.0f);
}

bool TKazekun::isCollidMove(THitActor*) { return false; }

void TKazekun::attackToMario()
{
	TNerveBase<TLiveActor>* latest = mSpine->getLatestNerve();
	bool b27 = (latest == &TNerveKazekunTurn::theNerve())
	           || (mSpine->getLatestNerve() == &TNerveKazekunPreAttack::theNerve());
	bool b28 = b27
	           || (mSpine->getLatestNerve() == &TNerveKazekunAttack::theNerve());
	if (b28) {
		SMS_SendMessageToMario(this, 0xe);
	}
}

void TKazekun::behaveToWater(THitActor*)
{
	TNerveBase<TLiveActor>* latest = mSpine->getLatestNerve();
	bool b27 = (latest == &TNerveKazekunTurn::theNerve())
	           || (mSpine->getLatestNerve() == &TNerveKazekunPreAttack::theNerve());
	bool b28 = b27
	           || (mSpine->getLatestNerve() == &TNerveKazekunAttack::theNerve());
	if (b28) {
		mSpine->reset();
		mSpine->setNext(&TNerveKazekunHitWater::theNerve());
	}
}

// Aim the kazekun's facing quaternion toward Mario (horizontal only) and spin
// it by mPoseOmegaRate, then re-point the velocity along the new forward axis.
// When `decide` is true the orientation is recomputed from Mario's position
// (the windup); the spin + velocity reorient run every frame regardless.
// NOTE(INVESTIGATION): block1 (the decide branch: aim + pose-speed velocity)
// is byte-decoded; the per-frame spin's rotation axis is best-effort (the asm
// inlines TQuat4::rotate of a basis the hand-decode couldn't pin -- see
// notes/Kazekun.md). Lands low fuzzy until the frame/inline cascade is cracked.
void TKazekun::doAttackPose(bool decide)
{
	JGeometry::TVec3<f32> dir(*gpMarioPos);
	dir.x -= mPosition.x;
	dir.y -= mPosition.y;
	dir.z -= mPosition.z;
	dir.y = 0.0f;

	if (decide) {
		TPosition3f mtx;
		JGeometry::TVec3<f32> up(0.0f, 1.0f, 0.0f);
		SMS_CalcToDirMatrix(mtx, dir, up);

		JGeometry::TQuat4<f32> quat;
		mtx.getQuat(quat);

		JGeometry::TVec3<f32> axis;
		mtx.getYDir(axis);
		JGeometry::TQuat4<f32> rot;
		rot.setRotate(axis, 1.5707964f);

		quat.mul(rot, quat);
		mQuat = quat;

		JGeometry::TVec3<f32> vel(0.0f, 0.0f,
		                          getKazekunParam()->mPoseSpeed.get());
		quat.rotate(vel);
		mVelocity = vel;
	}

	JGeometry::TVec3<f32> spinAxis(0.0f, 1.0f, 0.0f);
	mQuat.rotate(spinAxis);
	JGeometry::TQuat4<f32> spin;
	spin.setRotate(spinAxis,
	               3.1415927f * getKazekunParam()->mPoseOmegaRate.get());
	mQuat.mul(spin, mQuat);

	f32 speed = JGeometry::TUtil<f32>::sqrt(mVelocity.dot(mVelocity));
	JGeometry::TVec3<f32> newVel(0.0f, 0.0f, speed);
	mQuat.rotate(newVel);
	mVelocity = newVel;
}

void TKazekun::flyAroundMario()
{
	JGeometry::TVec3<f32> dir(*gpMarioPos);
	dir.y += getKazekunParam()->mTurnOffsetY.get();
	dir.x -= mPosition.x;
	dir.y -= mPosition.y;
	dir.z -= mPosition.z;

	f32 tilt = JGeometry::TUtil<f32>::clamp(dir.y, -400.0f, 400.0f) * 0.0025f;
	dir.y    = 0.0f;

	f32 dist  = JGeometry::TUtil<f32>::sqrt(dir.dot(dir));
	f32 ratio = JGeometry::TUtil<f32>::clamp(
	    dist / getKazekunParam()->mAroundDist.get(), 0.0f, 2.0f);

	TPosition3f mtx;
	JGeometry::TVec3<f32> up(0.0f, 1.0f, 0.0f);
	SMS_CalcToDirMatrix(mtx, dir, up);

	JGeometry::TQuat4<f32> quat;
	mtx.getQuat(quat);

	JGeometry::TVec3<f32> axis;
	mtx.getYDir(axis);
	JGeometry::TQuat4<f32> rot;
	rot.setRotate(axis, (2.0f - ratio) * 1.5707964f);

	quat.mul(rot, quat);
	mQuat = quat;

	JGeometry::TVec3<f32> vel(0.0f, 0.0f, 1.0f);
	quat.rotate(vel);
	vel.y = tilt;
	vel.scale(1.0f + __fabsf(tilt));
	vel.scale(getKazekunParam()->mAroundSpeed.get());
	mLinearVelocity = vel;
}

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
