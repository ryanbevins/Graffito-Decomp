#include <Enemy/BossWanwan.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
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
    , unk14(node_count)
    , unk18(nullptr)
    , unk1C(0.0f)
    , unk20(0.0f)
{
	unk18 = new TBWLeashNode*[unk14];
	for (int i = 0; i < unk14; ++i)
		unk18[i] = new TBWLeashNode("鎖部");
}

void TBWLeash::perform(u32 flags, JDrama::TGraphics* graphics)
{
	for (int i = 0; i < unk14; ++i)
		unk18[i]->perform(flags, graphics);
}

BOOL TBWPicket::receiveMessage(THitActor*, u32) { return FALSE; }

bool TBWPicket::moveRequest(const JGeometry::TVec3<f32>& position)
{
	mPosition = position;
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
    , unk154(nullptr)
    , unk158(nullptr)
    , unk168(0.0f)
    , unk16C(0)
    , unk170(nullptr)
    , unk17C(nullptr)
    , unk180(nullptr)
    , unk184(nullptr)
    , unk188(nullptr)
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

	unk154 = new TBWHit(this, 0, "ボスワンワンヒット");
	unk158 = new TBWPicket(this, "ボスワンワンつかみ");
	unk170 = new TBWLeash(this, 10, "ボスワンワン鎖");

	mSpine->initWith(&TNerveBWGraphWander::theNerve());
}

void TBossWanwan::shakeCamera(int) { }

BOOL TBossWanwan::receiveMessage(THitActor*, u32) { return FALSE; }

void TBossWanwan::calcRootMatrix() { calcEnemyRootMatrix(); }

void TBossWanwan::slideToCurPathNode(f32 march_speed, f32 turn_speed)
{
	walkToCurPathNode(march_speed, turn_speed, 0.0f);
}

void TBossWanwan::control() { TSpineEnemy::control(); }

void TBossWanwan::emitEffects() { }

void TBossWanwan::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (unk170 != nullptr)
		unk170->perform(flags, graphics);

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
