#include <Enemy/BossWanwan.hpp>
#include <Camera/CameraShake.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/MtxUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/Spine.hpp>
#include <System/Particles.hpp>

static const char* bwanwan_bastable[] = {
	"/scene/bwanwan/bas/bwanwan_bark.bas",
	nullptr,
	"/scene/bwanwan/bas/bwanwan_shake.bas",
	nullptr,
	"/scene/bwanwan/bas/bwanwan_wait.bas",
	"/scene/bwanwan/bas/bwanwan_wait2.bas",
	nullptr,
};

static const TModelDataLoadEntry sModelDataEntries[] = {
	{ "bwanwan_body.bmd", 0x10220000, 0 },
	{ "bwanwan_chain.bmd", 0x10220000, 0 },
	{ "bwanwan_picket.bmd", 0x10220000, 0 },
	{ nullptr, 0, 0 },
};

TBWParams::TBWParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mSLMarchSpeed, 6.0f)
    , PARAM_INIT(mSLTurnSpeed, 1.0f)
    , PARAM_INIT(mSLLeashNodeLen, 120.0f)
    , PARAM_INIT(mSLPicketHeight, 100.0f)
    , PARAM_INIT(mSLPicketRadius, 100.0f)
    , PARAM_INIT(mSLChainHitHeight, 100.0f)
    , PARAM_INIT(mSLChainHitRadius, 100.0f)
    , PARAM_INIT(mSLChainGroundRadius, 60.0f)
    , PARAM_INIT(mSLPullLimit, 1.0f)
    , PARAM_INIT(mSLAttackSpeed, 10.0f)
    , PARAM_INIT(mSLStunTimer, 4000)
    , PARAM_INIT(mSLSearchLength, 10000.0f)
    , PARAM_INIT(mSLSearchAngle, 60.0f)
    , PARAM_INIT(mSLBWHitPointMax, (u8)0xff)
    , PARAM_INIT(mSLHeadGap, 150.0f)
    , PARAM_INIT(mSLShakeLengthMax, 3000.0f)
    , PARAM_INIT(mSLShakeLengthMaxHP0, 2000.0f)
{
	TParams::load(mPrmPath);
}

void TBWLeashNode::calcTemperature() { }

void TBWLeashNode::calcMatrix() { }

void TBWLeashNode::perform(u32 flags, JDrama::TGraphics* graphics)
{
	calcTemperature();
	calcMatrix();
	THitActor::perform(flags, graphics);
}

TBWLeash::TBWLeash(TBossWanwan* owner, int node_count, const char* name)
    : JDrama::TViewObj(name)
    , mOwner(owner)
    , mRope(nullptr)
    , mNodes(nullptr)
{
	mRope = new TRope(node_count, mOwner->mPosition,
	                  ((TBWParams*)mOwner->getSaveParam())
	                      ->mSLLeashNodeLen.get(),
	                  mOwner->mTurnSpeed, 0.0f, 0.0f);
	mNodes = new TBWLeashNode*[node_count];
	for (int i = 0; i < node_count; ++i)
		mNodes[i] = new TBWLeashNode(this, i, "鎖部");
}

void TBWLeash::perform(u32 flags, JDrama::TGraphics* graphics)
{
	for (int i = 0; i < mRope->mNumPoints; ++i)
		mNodes[i]->perform(flags, graphics);
}

BOOL TBWPicket::receiveMessage(THitActor* sender, u32 message)
{
	if (sender->getActorType() != 0x80000001)
		return FALSE;

	if (message == HIT_MESSAGE_HIP_DROP) {
		mOwner->unk17C = 1;
		mOwner->unk184 = 0;
		if (gpMSound->gateCheck(0x28C0))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x28C0, &mPosition, 0, nullptr, 0, 4);
		return TRUE;
	}

	if (message == HIT_MESSAGE_TAKE) {
		if (mOwner->unk17C != 0) {
			JPABaseEmitter* emitter = gpMarioParticleManager->emit(
			    0xAE, &mOwner->mPicket->mPosition, 0, nullptr);
			if (emitter != nullptr) {
				JGeometry::TVec3<f32> scale(0.3f, 0.5f, 0.3f);
				emitter->setScale(scale);
			}
		}
		mOwner->unk194 = 0;
		mOwner->unk17C = 0;
		mHolder        = (TTakeActor*)sender;
		return TRUE;
	}

	if (message == HIT_MESSAGE_UNK7 || message == HIT_MESSAGE_UNK8) {
		mHolder = nullptr;
		return TRUE;
	}

	return FALSE;
}

bool TBWPicket::moveRequest(const JGeometry::TVec3<f32>& position)
{
	if (mOwner->mSpine->getLatestNerve()
	        == &TNerveBWJumpToBath::theNerve()
	    || mOwner->mSpine->getLatestNerve() == &TNerveBWDie::theNerve())
		return false;

	if (mOwner->mHitPoints != 0)
		return false;

	TRope* rope = mOwner->mLeash->mRope;
	JGeometry::TVec3<f32> tailBefore = rope->mPoints[0].mPosition;
	rope->constraintTail(position);
	mOwner->unk15C = rope->mPoints[0].mPosition;
	mOwner->unk15C.sub(tailBefore);
	return true;
}

MtxPtr TBWPicket::getTakingMtx() { return unk74; }

void TBWPicket::perform(u32 flags, JDrama::TGraphics* graphics)
{
	THitActor::perform(flags, graphics);
}

BOOL TBWHit::receiveMessage(THitActor* sender, u32 message)
{
	return mOwner->receiveMessage(sender, message);
}

void TBWHit::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		if (mJointIndex >= 0)
			mOwner->getJointTransByIndex(mJointIndex, &mPosition);

		for (int i = 0; i < mColCount; ++i) {
			THitActor* actor = mCollisions[i];
			if (mOwner->mHitPoints != 0
			    && actor->getActorType() == 0x80000001)
				actor->receiveMessage(mOwner, HIT_MESSAGE_UNKA);
		}
	}

	THitActor::perform(flags, graphics);
}

void TBWBinder::bind(TLiveActor* actor)
{
	actor->mPosition.add(actor->mLinearVelocity);
	actor->mPosition.add(actor->mVelocity);
}

void TBossWanwanMtxCalc::calc(u16 joint_no)
{
	M3UMtxCalcSIAnmBlendQuat::calc(joint_no);
}

TBossWanwan::TBossWanwan(const char* name)
    : TSpineEnemy(name)
    , mMtxCalc(nullptr)
    , mLeash(nullptr)
    , mPicket(nullptr)
    , unk168(0.0f)
    , unk16C(0)
    , mHeadHit(nullptr)
    , mBodyHit(nullptr)
    , unk17C(0)
    , unk180(0)
    , unk184(0)
    , unk188(0)
    , unk18C(0)
    , unk18D(0)
    , unk190(0)
    , unk194(1)
    , unk195(0)
    , unk198(0)
    , unk19C(0)
    , unk1A0(0)
    , unk1A4(0.0f)
    , unk1A8(0.0f)
    , unk1AC(0.0f)
    , unk1B0(0)
    , unk1B4(0)
{
	mBinder = new TBWBinder;
}

void TBossWanwan::init(TLiveManager* manager)
{
	mManager = manager;
	manager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(manager, 3);
	mMActor       = mMActorKeeper->createMActor("bwanwan_body.bmd", 0);
	mMtxCalc      = new TBossWanwanMtxCalc(this);
	mMActor->setCalcForBck(mMtxCalc);
	mMActor->calc();

	mLeash   = new TBWLeash(this, 15, "ボスワンワン鎖");
	mPicket  = new TBWPicket(this, "ボスワンワンつかみ");
	mHeadHit = new TBWHit(this, 3, "ボスワンワンヒット");
	mBodyHit = new TBWHit(this, -1, "ボスワンワンヒット");

	mSpine->initWith(&TNerveBWGraphWander::theNerve());
}

void TBossWanwan::shakeCamera(int mode)
{
	if (!SMS_IsMarioTouchGround4cm())
		return;

	f32 marioDist = JGeometry::TUtil<f32>::sqrt(mDistToMarioSquared);
	TBWParams* params = (TBWParams*)getSaveParam();
	f32 lengthMax     = params->mSLShakeLengthMax.get();
	f32 lengthMaxHP0  = params->mSLShakeLengthMaxHP0.get();
	f32 ratio;

	if (mMActor->checkCurBckFromIndex(0)) {
		ratio = 1.0f;
	} else {
		ratio = (f32)mHitPoints / (f32)params->mSLBWHitPointMax.get();
	}

	f32 length = lengthMax * ratio + lengthMaxHP0 * (1.0f - ratio);
	f32 power  = length - marioDist;
	if (power < 0.0f)
		return;

	power /= length;
	if (power > 1.0f)
		power = 1.0f;

	power *= ratio;
	gpCameraShake->startShake((EnumCamShakeMode)mode, power);
	SMSRumbleMgr->start(8, &mPosition);
}

BOOL TBossWanwan::receiveMessage(THitActor* sender, u32 message)
{
	u32 actorType = sender->getActorType();
	if (actorType == 0x80000001)
		return FALSE;

	if (actorType == 0x1000001) {
		if (unk18C)
			return TRUE;

		gpMarioParticleManager->emit(0xE7, &sender->mPosition, 0, nullptr);

		if (mHitPoints == 0) {
			if (gpMSound->gateCheck(0x28D1))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x28D1, &mPosition, 0, nullptr, 0, 4);
		} else if (mHitPoints == 1) {
			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0xB0, getModel()->mNodeMatrices[1], 0, nullptr);
			if (gpMSound->gateCheck(0x28C5))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x28C5, &mPosition, 0, nullptr, 0, 4);
		} else {
			if (gpMSound->gateCheck(0x28BE))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x28BE, &mPosition, 0, nullptr, 0, 4);
		}

		if (mHitPoints != 0)
			--mHitPoints;

		++unk190;
		return TRUE;
	}

	if (actorType == 0x4000005A) {
		sender->receiveMessage(this, HIT_MESSAGE_HIP_DROP);
		mHitPoints = 0;
		++unk190;

		if (!unk1A0)
			++unk1A0;

		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0xB0, getModel()->mNodeMatrices[1], 0, nullptr);
		if (gpMSound->gateCheck(0x28C5))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x28C5, &mPosition, 0, nullptr, 0, 4);
	}

	return TSpineEnemy::receiveMessage(sender, message);
}

void TBossWanwan::calcRootMatrix()
{
	J3DModel* model = getModel();
	model->unk14.x  = mScaling.x;
	model->unk14.y  = mScaling.y;
	model->unk14.z  = mScaling.z;

	MsMtxSetXYZRPH(getModel()->getBaseTRMtx(), mPosition.x,
	               mPosition.y + 500.0f, mPosition.z, mRotation.x,
	               mRotation.y, mRotation.z);
}

void TBossWanwan::slideToCurPathNode(f32 march_speed, f32 turn_speed)
{
	walkToCurPathNode(march_speed, turn_speed, 0.0f);
}

void TBossWanwan::control() { TSpineEnemy::control(); }

void TBossWanwan::emitEffects() { }

void TBossWanwan::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (mLeash != nullptr)
		mLeash->perform(flags, graphics);

	TSpineEnemy::perform(flags, graphics);
}

void TBossWanwan::kill() { }

TBossWanwanManager::TBossWanwanManager(const char* name)
    : TEnemyManager(name)
{
}

TSpineEnemy* TBossWanwanManager::createEnemyInstance()
{
	return new TBossWanwan("ボスワンワン");
}

void TBossWanwanManager::createModelData() { createModelDataArray(sModelDataEntries); }

void TBossWanwanManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TBWParams("/enemy/bosswanwan.prm");

	TEnemyManager::load(stream);

	SMS_LoadParticle("/scene/bwanwan/jpa/ms_bwan_jump_rock.jpa", 0xad);
	SMS_LoadParticle("/scene/bwanwan/jpa/ms_bwan_jump_smoke.jpa", 0xae);
	SMS_LoadParticle("/scene/bwanwan/jpa/ms_bwan_downyuge.jpa", 0xb0);
	SMS_LoadParticle("/scene/bwanwan/jpa/ms_bwan_hibana.jpa", 0xaf);
	SMS_LoadParticle("/scene/bwanwan/jpa/ms_bwan_deadyuge.jpa", 0xb1);
	SMS_LoadParticle("/scene/bwanwan/jpa/ms_bwan_yugami.jpa", 0x1ee);
	SMS_LoadParticle("/scene/bwanwan/jpa/ms_bwan_hityuge.jpa", 0x167);
	SMS_LoadParticle("/scene/bwanwan/jpa/ms_bwan_kira.jpa", 0x168);
}

DEFINE_NERVE(TNerveBWGraphWander, TLiveActor)
{
	TBossWanwan* self = (TBossWanwan*)spine->getBody();
	if (spine->getTime() == 0)
		self->mMActor->setBck(bwanwan_bastable[4]);

	self->slideToCurPathNode(((TBWParams*)self->getSaveParam())->mSLMarchSpeed.get(),
	                         ((TBWParams*)self->getSaveParam())->mSLTurnSpeed.get());
	return false;
}

DEFINE_NERVE(TNerveBWRoll, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveBWBark, TLiveActor)
{
	TBossWanwan* self = (TBossWanwan*)spine->getBody();
	if (spine->getTime() == 0)
		self->mMActor->setBck(bwanwan_bastable[0]);
	return false;
}

DEFINE_NERVE(TNerveBWJump, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveBWStun, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveBWWakeup, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveBWJumpToBath, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveBWDie, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveBWJumpAway, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveBWShake, TLiveActor)
{
	TBossWanwan* self = (TBossWanwan*)spine->getBody();
	if (spine->getTime() == 0)
		self->mMActor->setBck(bwanwan_bastable[2]);
	return false;
}

DEFINE_NERVE(TNerveBWFall, TLiveActor)
{
	return false;
}
