#include <Enemy/ChuuHana.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/Graph.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DNode.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <M3DUtil/MActor.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionData.hpp>
#include <Map/MapMirror.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/MirrorActor.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <System/Particles.hpp>
#include <stdlib.h>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

static int ChuuHanaBodyCallback(J3DNode*, int);

static const char* tyuhana_bastable[] = {
	"/scene/tyuhana/bas/tyuhana_chance_end.bas",
	nullptr,
	"/scene/tyuhana/bas/tyuhana_chance_start.bas",
	"/scene/tyuhana/bas/tyuhana_jump.bas",
	"/scene/tyuhana/bas/tyuhana_push.bas",
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	"/scene/tyuhana/bas/tyuhana_walk.bas",
};

static const char* graphlist[] = {
	"kohana0", "kohana1", "kohana1", "kohana2", "kohana2", "kohana2",
};

static TChuuHana* gpCurChuuHana;

s32 TChuuHana::mCheckOnPanelTimeRoll = 20;
s32 TChuuHana::mCheckOnPanelTime     = 400;
u8 TChuuHana::mBodyJntIndex          = 1;
u8 TChuuHana::mEyeJntIndex           = 12;
u8 TChuuHana::mFootJntIndex          = 5;
u8 TChuuHana::mNewSw                 = 1;
u8 TChuuHana::mCompareHeight         = 1;
f32 TChuuHana::mSmallMirrorR         = 650.0f;
f32 TChuuHana::mMediumMirrorR        = 900.0f;
f32 TChuuHana::mLargeMirrorR         = 1100.0f;
u8 TChuuHana::mAttackVersion         = 1;
u8 TChuuHana::mDamageSw              = 1;

static inline TChuuHana* chuuHana(TSpineBase<TLiveActor>* spine)
{
	return (TChuuHana*)spine->getBody();
}

TChuuHanaSaveLoadParams::TChuuHanaSaveLoadParams(const char* path)
    : TWalkerEnemyParams(path)
    , PARAM_INIT(mSLGetWaterPow, 1.0f)
    , PARAM_INIT(mSLGetGroundPow, 1.0f)
    , PARAM_INIT(mSLKeepBalanceTime, 200)
    , PARAM_INIT(mSLCheckFrame, 5)
    , PARAM_INIT(mSLReverseHeightS, 15.0f)
    , PARAM_INIT(mSLStretchHeightS, 10.0f)
    , PARAM_INIT(mSLMediumStretchHeightS, 7.0f)
    , PARAM_INIT(mSLSmallStretchHeightS, 3.0f)
    , PARAM_INIT(mSLReverseHeightM, 15.0f)
    , PARAM_INIT(mSLStretchHeightM, 10.0f)
    , PARAM_INIT(mSLMediumStretchHeightM, 7.0f)
    , PARAM_INIT(mSLSmallStretchHeightM, 3.0f)
    , PARAM_INIT(mSLReverseHeightL, 15.0f)
    , PARAM_INIT(mSLStretchHeightL, 10.0f)
    , PARAM_INIT(mSLMediumStretchHeightL, 7.0f)
    , PARAM_INIT(mSLSmallStretchHeightL, 3.0f)
    , PARAM_INIT(mSLWalkGravity, 4.0f)
    , PARAM_INIT(mSLWaterHitGravity, 0.2f)
    , PARAM_INIT(mSLJumpGravity, 0.2f)
    , PARAM_INIT(mSLJumpSp, 12.0f)
    , PARAM_INIT(mSLJumpHeight, 300.0f)
    , PARAM_INIT(mSLGetWaterPow2, 1.0f)
    , PARAM_INIT(mSLTacklePow, 100.0f)
    , PARAM_INIT(mSLDashRate, 2.0f)
    , PARAM_INIT(mSLAttackTimer, 300)
    , PARAM_INIT(mSLHitWaterTimer, 60)
{
	TParams::load(mPrmPath);
}

TChuuHanaManager::TChuuHanaManager(const char* name)
    : TSmallEnemyManager(name)
{
	gpCurChuuHana = nullptr;
	unk60         = 0;
	unk61         = 0;
	unk62         = 0;
}

void TChuuHanaManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38 = new TChuuHanaSaveLoadParams("/enemy/chuuhana.prm");
}

void TChuuHanaManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TEnemyManager::perform(flags, graphics);
}

TSmallEnemy* TChuuHanaManager::createEnemyInstance()
{
	return new TChuuHana("チュウハナ");
}

void TChuuHanaManager::initSetEnemies()
{
	for (int i = 0; i < mObjNum; ++i) {
		TChuuHana* enemy = (TChuuHana*)unk18[i];
		TGraphWeb* graph = gpConductor->getGraphByName(graphlist[i]);

		if (i == 0)
			enemy->unk21C = &unk60;
		else if (i < 3)
			enemy->unk21C = &unk61;
		else
			enemy->unk21C = &unk62;

		if (graph != nullptr && graph->getNodeNum() > 0) {
			int index = (int)(rand() * (1.0f / (RAND_MAX + 1))
			                  * graph->getNodeNum());
			JGeometry::TVec3<f32> pos;
			graph->getGraphNode(index).getPoint((Vec*)&pos);
			enemy->mPosition = pos;
			enemy->mPosition.y += 50.0f;
			enemy->onLiveFlag(LIVE_FLAG_AIRBORNE);
			enemy->getTracer()->init(graph);
			enemy->reset();
		}
	}
}

TChuuHana::TChuuHana(const char* name)
    : TWalkerEnemy(name)
    , unk194(0.0f)
    , unk198(0.0f)
    , unk19C(0.0f)
    , unk1A0(0)
    , unk1A4(0)
    , unk1A8(0.0f)
    , unk1AC(0)
    , unk1B0(1)
    , unk1B1(0)
    , unk1B2(0)
    , unk1B8(0.0f)
    , unk210(0.0f)
    , unk214(0)
    , unk215(0)
    , unk218(nullptr)
    , unk21C(nullptr)
    , unk220(0.0f)
    , unk224(0)
    , mAseCallback(this)
{
}

void TChuuHana::init(TLiveManager* manager)
{
	TWalkerEnemy::init(manager);
	mActorType = 0x10000016;
	unk150     = 17;
	offHitFlag(HIT_FLAG_UNK40000000);
	mSpine->initWith(&TNerveChuuHanaWalkOnPanel::theNerve());
	getMActor()->setJointCallback(mBodyJntIndex, ChuuHanaBodyCallback);
	unk130 = 1;

	if (mMActor->unkC != nullptr)
		mMActor->unkC->initNormalMotionBlend();

	unk1B4 = (TChuuHanaSaveLoadParams*)getSaveParam();
	if (mMActor->getModel() != nullptr)
		mMActor->getModel()->calc();

	unk218 = new TMirrorActor("チュウハナin鏡");
	unk218->init(getModel(), 0);
}

void TChuuHana::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor       = mMActorKeeper->createMActor("default.bmd", 3);
}

void TChuuHana::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(flags, graphics);

	JGeometry::TVec3<f32> marioPos = *gpMarioPos;
	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)
	    && (gpMirrorModelManager->isInMirror(mPosition)
	        || gpMirrorModelManager->isInMirror(marioPos))) {
		if (flags & 2) {
			calcRootMatrix();
			getMActor()->calc();
		}
		if (flags & 4)
			getMActor()->viewCalc();
	}
}

void TChuuHana::reset()
{
	gpCurChuuHana = this;
	TWalkerEnemy::reset();
	unk215 = 0;
	unk1A4 = mCheckOnPanelTime;
	unk194 = 0.0f;
	unk198 = 0.0f;
	unk19C = 0.0f;
	unk1A0 = 0;
	unk224 = 0;
	unk1F8 = mPosition;
	unk1B2 = 1;
}

void TChuuHana::setBckAnm(int index)
{
	unk194 = 1.0f;
	f32 blend = unk194;
	if (mMActor->unkC != nullptr)
		mMActor->unkC->setMotionBlendRatio(blend);

	MActor* actor = mMActor;
	J3DAnmTransform* oldAnm;
	if (actor->unkC == nullptr)
		oldAnm = nullptr;
	else
		oldAnm = actor->unkC->unk24;

	if (actor->unkC != nullptr)
		actor->unkC->setOldMotionBlendAnmPtr(oldAnm);

	TSmallEnemy::setBckAnm(index);
}

void TChuuHana::setWalkAnm()
{
	bool wasInvalid = false;
	if (mCurrentBckAnm < 0)
		wasInvalid = true;

	setBckAnm(12);

	if (wasInvalid) {
		f32 frame = 10.0f * mInstanceIndex;
		getMActor()->getFrameCtrl(0)->setFrame(frame);
	}
}

void TChuuHana::kill()
{
	TSmallEnemy::kill();
	if (unk21C != nullptr)
		*unk21C = 0;
}

void TChuuHana::forceKill()
{
	kill();
	onLiveFlag(LIVE_FLAG_DEAD);
}

bool TChuuHana::isFindMario(f32) { return false; }

const char** TChuuHana::getBasNameTable() const { return tyuhana_bastable; }

f32 TChuuHana::getGravityY() const
{
	const TChuuHanaSaveLoadParams* params = getChuuHanaParams();
	if (mSpine->getCurrentNerve() == &TNerveChuuHanaWalkOnPanel::theNerve())
		return params->mSLWalkGravity.get();
	if (mSpine->getCurrentNerve() == &TNerveChuuHanaFall::theNerve()
	    || mSpine->getCurrentNerve() == &TNerveChuuHanaFall2::theNerve())
		return params->mSLJumpGravity.get();
	if (mSpine->getCurrentNerve() == &TNerveSmallEnemyHitWaterJump::theNerve())
		return params->mSLWaterHitGravity.get();
	return TSmallEnemy::getGravityY();
}

void TChuuHana::setGoal()
{
	if (unk124->getGraph() == nullptr)
		return;

	goToRandomNextGraphNode();
	unk1F8 = mPosition;
}

BOOL TChuuHana::willFall(s32 time)
{
	if (time < 0)
		return FALSE;
	if (unk21C != nullptr && *unk21C != 0)
		return FALSE;
	if (unk1A4 > time)
		return FALSE;
	return TRUE;
}

bool TChuuHana::checkStretchType()
{
	f32 height = mPosition.y - unk1F8.y;
	f32 mirrorR;
	if (height < 300.0f)
		mirrorR = mSmallMirrorR;
	else if (height < 600.0f)
		mirrorR = mMediumMirrorR;
	else
		mirrorR = mLargeMirrorR;

	return mDistToMarioSquared < mirrorR * mirrorR;
}

void TChuuHana::bind()
{
	TWalkerEnemy::bind();
	if (!isAirborne())
		unk215 = 0;
}

void TChuuHana::moveObject()
{
	if (unk1A4 > 0)
		--unk1A4;

	TWalkerEnemy::moveObject();
	if (unk21C != nullptr && *unk21C != 0)
		mSpine->pushNerve(&TNerveChuuHanaKeepBalance::theNerve());
}

bool TChuuHana::isCollidMove(THitActor* actor)
{
	if (actor->isActorType(0x80000001)) {
		attackToMario();
		return true;
	}
	return TSmallEnemy::isCollidMove(actor);
}

void TChuuHana::calcRootMatrix()
{
	gpCurChuuHana = this;
	TSpineEnemy::calcRootMatrix();
}

void TChuuHana::attackToMario()
{
	TWalkerEnemy::attackToMario();
	if (mAttackVersion != 0)
		mSpine->pushNerve(&TNerveChuuHanaAttack::theNerve());
}

BOOL TChuuHana::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 0) {
		attackToMario();
		return TRUE;
	}
	if (message == 1) {
		behaveToWater(sender);
		return TRUE;
	}
	return TSmallEnemy::receiveMessage(sender, message);
}

void TChuuHana::behaveToWater(THitActor* actor)
{
	if (mSpine->getCurrentNerve() == &TNerveSmallEnemyHitWaterJump::theNerve())
		return;

	TChuuHanaSaveLoadParams* params = getChuuHanaParams();
	mVelocity.y = params->mSLGetWaterPow.get();
	if (actor != nullptr) {
		TLiveActor* liveActor = (TLiveActor*)actor;
		mVelocity.x += liveActor->mVelocity.x * params->mSLGetWaterPow2.get();
		mVelocity.z += liveActor->mVelocity.z * params->mSLGetWaterPow2.get();
	}
	mSpine->pushNerve(&TNerveSmallEnemyHitWaterJump::theNerve());
}

void TChuuHanaAseParCallback::execute(JPABaseEmitter*, JPABaseParticle*) { }

void TChuuHanaAseParCallback::draw(JPABaseEmitter*, JPABaseParticle*) { }

static int ChuuHanaBodyCallback(J3DNode*, int timing)
{
	if (timing == 0)
		return 1;

	if (gpCurChuuHana != nullptr)
		gpCurChuuHana->mMActor->getModel()->getBaseTRMtx();

	return 1;
}

DEFINE_NERVE(TNerveChuuHanaWalkOnPanel, TLiveActor)
{
	TChuuHana* self = chuuHana(spine);
	if (spine->getTime() == 0)
		self->setWalkAnm();

	self->walkBehavior(0, 1.0f);
	if (self->willFall(TChuuHana::mCheckOnPanelTime)) {
		spine->pushAfterCurrent(&TNerveChuuHanaFall::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaForceJumped, TLiveActor)
{
	TChuuHana* self = chuuHana(spine);
	if (spine->getTime() == 0) {
		self->setBckAnm(3);
		self->mVelocity.y = self->getChuuHanaParams()->mSLJumpSp.get();
	}
	if (!self->isAirborne()) {
		spine->pushAfterCurrent(&TNerveChuuHanaWalkOnPanel::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaKeepBalance, TLiveActor)
{
	TChuuHana* self = chuuHana(spine);
	if (spine->getTime() == 0)
		self->setBckAnm(4);

	if (spine->getTime() > self->getChuuHanaParams()->mSLKeepBalanceTime.get()) {
		spine->pushAfterCurrent(&TNerveChuuHanaWalkOnPanel::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaStick, TLiveActor)
{
	TChuuHana* self = chuuHana(spine);
	if (spine->getTime() == 0)
		self->setBckAnm(4);
	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveChuuHanaKeepBalance::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaRoll, TLiveActor)
{
	TChuuHana* self = chuuHana(spine);
	if (spine->getTime() == 0)
		self->setBckAnm(4);
	if (spine->getTime() > TChuuHana::mCheckOnPanelTimeRoll) {
		spine->pushAfterCurrent(&TNerveChuuHanaFall2::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaFall, TLiveActor)
{
	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaFall2, TLiveActor)
{
	TChuuHana* self = chuuHana(spine);
	if (!self->isAirborne()) {
		spine->pushAfterCurrent(&TNerveChuuHanaWalkOnPanel::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaObject, TLiveActor)
{
	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaAttack, TLiveActor)
{
	TChuuHana* self = chuuHana(spine);
	if (spine->getTime() == 0)
		self->setBckAnm(4);
	if (spine->getTime() > self->getChuuHanaParams()->mSLAttackTimer.get()) {
		spine->pushAfterCurrent(&TNerveChuuHanaWalkOnPanel::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaJumpPrepare, TLiveActor)
{
	TChuuHana* self = chuuHana(spine);
	if (spine->getTime() == 0)
		self->setBckAnm(3);
	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveChuuHanaForceJumped::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaWait, TLiveActor)
{
	TChuuHana* self = chuuHana(spine);
	if (spine->getTime() == 0)
		self->setBckAnm(11);
	return self->checkCurAnmEnd(0) ? TRUE : FALSE;
}
