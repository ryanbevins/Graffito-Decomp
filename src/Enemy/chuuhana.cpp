#include <Enemy/ChuuHana.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/Graph.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DNode.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
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

	TMirrorActor* mirror = new TMirrorActor("チュウハナin鏡");
	mirror->init(getModel(), 0);
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
	JGeometry::TVec3<f32> goal = mPosition;
	Vec dir;
	dir.x = 0.0f;
	dir.y = 0.0f;
	dir.z = 1.0f;

	Mtx mtx;
	f32 randYaw = -30.0f + rand() * (1.0f / (RAND_MAX + 1)) * 60.0f;
	MsMtxSetRotRPH(mtx, mRotation.x, mRotation.y + randYaw, mRotation.z);
	PSMTXMultVec(mtx, &dir, &dir);

	goal.x += 1000.0f * dir.x;
	goal.z += 1000.0f * dir.z;

	TPathNode node(goal);
	unkF4  = node;
	unk104 = node;
	unk114.clear();
	unk1A4 = mCheckOnPanelTime;
	unk1B2 = 0;
}

BOOL TChuuHana::willFall(s32 time)
{
	f32 radius = mSmallMirrorR;
	if (mInstanceIndex > 0)
		radius = mMediumMirrorR;
	if (mInstanceIndex > 2)
		radius = mLargeMirrorR;

	if (time == mCheckOnPanelTimeRoll)
		radius += 250.0f;

	if (unk218 != nullptr) {
		JGeometry::TVec3<f32>* mirrorPos
		    = (JGeometry::TVec3<f32>*)unk218;
		f32 dx = mPosition.x - mirrorPos->x;
		f32 dy = mPosition.y - mirrorPos->y;
		f32 dz = mPosition.z - mirrorPos->z;
		f32 dist = JGeometry::TUtil<f32>::sqrt(dx * dx + dy * dy + dz * dz);

		if (dist > radius) {
			unk1A4 = mCheckOnPanelTime;

			TGraphWeb* graph = unk124->unk0;
			int index
			    = (int)(rand() * (1.0f / (RAND_MAX + 1)) * graph->getNodeNum());
			JGeometry::TVec3<f32> point;
			graph->getGraphNode(index).getPoint((Vec*)&point);

			TPathNode node(point);
			unkF4  = node;
			unk104 = node;
			unk114.clear();
			unk1B2 = 1;
			return TRUE;
		}
	}

	unk1B2 = 0;
	return FALSE;
}

void TChuuHana::checkStretchType()
{
	unk1A0 = 0;

	TChuuHanaSaveLoadParams* params = getChuuHanaParams();
	f32 height                     = unk1A8;

	if (mSpine->getCurrentNerve()
	    == &TNerveChuuHanaKeepBalance::theNerve()) {
		f32 reverseHeight = params->mSLReverseHeightS.get();
		if (mInstanceIndex > 0)
			reverseHeight = params->mSLReverseHeightM.get();
		if (mInstanceIndex > 2)
			reverseHeight = params->mSLReverseHeightL.get();

		if (height > reverseHeight) {
			unk1B1 = 1;
			unk214 = 1;
			mSpine->pushNerve(&TNerveChuuHanaFall2::theNerve());
			mSpine->pushNerve(&TNerveChuuHanaJumpPrepare::theNerve());
			return;
		}
	}

	f32 stretchHeight = params->mSLStretchHeightS.get();
	if (mInstanceIndex > 0)
		stretchHeight = params->mSLStretchHeightM.get();
	if (mInstanceIndex > 2)
		stretchHeight = params->mSLStretchHeightL.get();

	if (height > stretchHeight) {
		unk1B1 = 0;
		unk214 = 0;
		setBckAnm(8);
		mSpine->pushNerve(&TNerveChuuHanaForceJumped::theNerve());
		return;
	}

	stretchHeight = params->mSLMediumStretchHeightS.get();
	if (mInstanceIndex > 0)
		stretchHeight = params->mSLMediumStretchHeightM.get();
	if (mInstanceIndex > 2)
		stretchHeight = params->mSLMediumStretchHeightL.get();

	if (height > stretchHeight) {
		unk1B1 = 0;
		unk214 = 0;
		setBckAnm(9);
		mSpine->pushNerve(&TNerveChuuHanaForceJumped::theNerve());
		return;
	}

	stretchHeight = params->mSLSmallStretchHeightS.get();
	if (mInstanceIndex > 0)
		stretchHeight = params->mSLSmallStretchHeightM.get();
	if (mInstanceIndex > 2)
		stretchHeight = params->mSLSmallStretchHeightL.get();

	if (height > stretchHeight) {
		unk1B1 = 0;
		unk214 = 0;
		setBckAnm(10);
		mSpine->pushNerve(&TNerveChuuHanaForceJumped::theNerve());
	}
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

void TChuuHanaAseParCallback::execute(JPABaseEmitter* emitter, JPABaseParticle*)
{
	TChuuHana* owner = mOwner;
	if (!owner->checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
		emitter->setGlobalRTMatrix(
		    owner->mMActor->getModel()->getAnmMtx(TChuuHana::mEyeJntIndex));
		emitter->unk154.x = 0.0f;
		emitter->unk154.y = 0.0f;
		emitter->unk154.z = 0.0f;
		emitter->unk174.x = 0.0f;
		emitter->unk174.y = 0.0f;
		emitter->unk174.z = 0.0f;
	}
}

void TChuuHanaAseParCallback::draw(JPABaseEmitter*, JPABaseParticle*) { }

static int ChuuHanaBodyCallback(J3DNode* node, int timing)
{
	if (timing == 0) {
		TChuuHana* owner = gpCurChuuHana;
		if (owner != nullptr
		    && owner->mSpine->getCurrentNerve()
		        == &TNerveChuuHanaRoll::theNerve()) {

			u16 jointIndex = ((J3DJoint*)node)->getJntNo();
			MtxPtr jointMtx = owner->getModel()->mNodeMatrices[jointIndex];

			Mtx identity;
			identity[0][0] = 1.0f;
			identity[0][1] = 0.0f;
			identity[0][2] = 0.0f;
			identity[0][3] = 0.0f;
			identity[1][0] = 0.0f;
			identity[1][1] = 1.0f;
			identity[1][2] = 0.0f;
			identity[1][3] = 0.0f;
			identity[2][0] = 0.0f;
			identity[2][1] = 0.0f;
			identity[2][2] = 1.0f;
			identity[2][3] = 0.0f;

			Vec dir;
			dir.x = owner->unk204.x;
			dir.y = 0.0f;
			dir.z = owner->unk204.z;
			if (dir.x == 0.0f && dir.z == 0.0f)
				dir.x = 0.001f;

			Vec up;
			up.x = 0.0f;
			up.y = 1.0f;
			up.z = 0.0f;
			Vec axis;
			PSVECCrossProduct(&up, &dir, &axis);

			f32 len2 = jointMtx[0][2] * jointMtx[0][2]
			           + jointMtx[1][2] * jointMtx[1][2]
			           + jointMtx[2][2] * jointMtx[2][2];
			f32 z = 0.0f;
			if (len2 != 0.0f)
				z = (axis.x * jointMtx[0][2] + axis.y * jointMtx[1][2]
				     + axis.z * jointMtx[2][2])
				    / len2;

			len2 = jointMtx[0][1] * jointMtx[0][1]
			       + jointMtx[1][1] * jointMtx[1][1]
			       + jointMtx[2][1] * jointMtx[2][1];
			f32 y = 0.0f;
			if (len2 != 0.0f)
				y = (axis.x * jointMtx[0][1] + axis.y * jointMtx[1][1]
				     + axis.z * jointMtx[2][1])
				    / len2;

			len2 = jointMtx[0][0] * jointMtx[0][0]
			       + jointMtx[1][0] * jointMtx[1][0]
			       + jointMtx[2][0] * jointMtx[2][0];
			f32 x = 0.0f;
			if (len2 != 0.0f)
				x = (axis.x * jointMtx[0][0] + axis.y * jointMtx[1][0]
				     + axis.z * jointMtx[2][0])
				    / len2;

			Vec localAxis;
			localAxis.x = x;
			localAxis.y = y;
			localAxis.z = z;
			Mtx rot;
			PSMTXRotAxisRad(rot, &localAxis,
			                0.017453292f * owner->unk210);
			PSMTXConcat(jointMtx, rot, jointMtx);
			PSMTXConcat(jointMtx, identity, jointMtx);
			PSMTXConcat(J3DSys::mCurrentMtx, rot, J3DSys::mCurrentMtx);
			PSMTXConcat(J3DSys::mCurrentMtx, identity,
			            J3DSys::mCurrentMtx);
		}
	}

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
