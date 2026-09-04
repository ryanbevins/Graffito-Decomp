#define JGEOMETRY_TVEC3_SUB_OUT_OF_LINE
#include <Enemy/ChuuHana.hpp>
#undef JGEOMETRY_TVEC3_SUB_OUT_OF_LINE
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
#include <Map/MapData.hpp>
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
	static const char* graphlist[] = {
		"kohana0", "kohana1", "kohana1", "kohana2", "kohana2", "kohana2",
	};

	for (int i = 0; i < mCapacity; ++i) {
		TGraphWeb* graph = gpConductor->getGraphByName(graphlist[i]);
		TChuuHana* enemy = (TChuuHana*)unk18[i];

		if (i == 0)
			enemy->unk21C = &unk60;
		else if (i < 3)
			enemy->unk21C = &unk61;
		else
			enemy->unk21C = &unk62;

		TMsRange<s32> nodeRange(0, graph->getNodeNum());
		int index = nodeRange.rand();
		JGeometry::TVec3<f32> pos;
		graph->getGraphNode(index).getPoint((Vec*)&pos);
		enemy->mPosition = pos;
		enemy->mPosition.y += 50.0f;
		enemy->onLiveFlag(LIVE_FLAG_AIRBORNE);
		enemy->getTracer()->init(graph);
		enemy->reset();
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
	unk1A4 = 30;
	mHeadHeight = 200.0f;
	unk1F8 = mPosition;
	unk19C = 0.0f;
	unk198 = 0.0f;
	unk1A0 = 0;
	unk224 = 0;
	unk1A4 = mCheckOnPanelTime;

	TMsRange<s32> nodeRange(0, unk124->unk0->getNodeNum());
	int index = nodeRange.rand();
	JGeometry::TVec3<f32> point;
	unk124->unk0->getGraphNode(index).getPoint((Vec*)&point);

	TPathNode node(point);
	unkF4  = node;
	unk104 = node;
	unk114.clear();
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
	if (!checkLiveFlag(LIVE_FLAG_DEAD)) {
		onLiveFlag(LIVE_FLAG_HIDDEN);
		if (unk218 != nullptr) {
			unk218->receiveMessage(this, 8);
			unk218 = nullptr;
		}
		TSmallEnemy::kill();
	}
}

void TChuuHana::forceKill()
{
	if ((!mGroundPlane->checkFlag(BG_CHECK_FLAG_ILLEGAL)
	        && (mGroundPlane->isDeathPlane() || mGroundPlane->isPool()
	            || mGroundPlane->isWaterSurface()))
	    || !gpMap->isInArea(mPosition.x, mPosition.z)
	    || mGroundPlane->checkFlag(BG_CHECK_FLAG_ILLEGAL))
		kill();
}

bool TChuuHana::isFindMario(f32) { return false; }

const char** TChuuHana::getBasNameTable() const { return tyuhana_bastable; }

f32 TChuuHana::getGravityY() const
{
	f32 gravity = TLiveActor::getGravityY();

	if (mSpine->getCurrentNerve() == &TNerveChuuHanaWalkOnPanel::theNerve()
	    || mSpine->getCurrentNerve()
	           == &TNerveChuuHanaKeepBalance::theNerve()
	    || mSpine->getCurrentNerve()
	           == &TNerveChuuHanaForceJumped::theNerve())
		gravity = getChuuHanaParams()->mSLWalkGravity.get();
	else if (mSpine->getCurrentNerve() == &TNerveChuuHanaStick::theNerve())
		gravity = getChuuHanaParams()->mSLWaterHitGravity.get();
	else if (mSpine->getCurrentNerve() == &TNerveChuuHanaFall2::theNerve()
	         || mSpine->getCurrentNerve()
	                == &TNerveChuuHanaJumpPrepare::theNerve())
		gravity = getChuuHanaParams()->mSLJumpGravity.get();
	return gravity;
}

void TChuuHana::setGoal()
{
	JGeometry::TVec3<f32> goal;
	goal.x = mPosition.x;
	goal.y = mPosition.y;
	goal.z = mPosition.z;

	Mtx mtx;
	TMsRange<f32> yawRange(-30.0f, 30.0f);
	f32 randYaw = yawRange.rand();
	Vec dir;
	dir.x = 0.0f;
	dir.y = 0.0f;
	dir.z = 1.0f;
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

bool TChuuHana::willFall(s32 time)
{
	f32 radius = mSmallMirrorR;
	s32 instanceIndex = mInstanceIndex;
	if (instanceIndex > 0)
		radius = mMediumMirrorR;
	if (instanceIndex > 2)
		radius = mLargeMirrorR;

	if (time == mCheckOnPanelTimeRoll)
		radius += 250.0f;

	if (unk218 != nullptr) {
		f32 dx = mPosition.x - unk218->mPosition.x;
		f32 dy = mPosition.y - unk218->mPosition.y;
		f32 dz = mPosition.z - unk218->mPosition.z;
		f32 dist = JGeometry::TUtil<f32>::sqrt(dx * dx + dy * dy + dz * dz);

		if (dist > radius) {
			unk1A4 = mCheckOnPanelTime;

			TMsRange<s32> nodeRange(0, unk124->unk0->getNodeNum());
			int index = nodeRange.rand();
			JGeometry::TVec3<f32> point;
			unk124->unk0->getGraphNode(index).getPoint((Vec*)&point);

			TPathNode node(point);
			unkF4  = node;
			unk104 = node;
			unk114.clear();
			unk1B2 = 1;
			return true;
		}
	}

	unk1B2 = 0;
	return false;
}

void TChuuHana::checkStretchType()
{
	unk1A0 = 0;

	f32 height = unk1A8;

	if (mSpine->getCurrentNerve()
	    == &TNerveChuuHanaKeepBalance::theNerve()) {
		s32 instanceIndex = mInstanceIndex;
		f32 reverseHeight = getChuuHanaParams()->mSLReverseHeightS.get();
		if (instanceIndex > 0)
			reverseHeight = getChuuHanaParams()->mSLReverseHeightM.get();
		if (instanceIndex > 2)
			reverseHeight = getChuuHanaParams()->mSLReverseHeightL.get();

		if (height > reverseHeight) {
			unk1B1 = 1;
			unk214 = 1;
			mSpine->pushNerve(&TNerveChuuHanaFall2::theNerve());
			mSpine->pushNerve(&TNerveChuuHanaJumpPrepare::theNerve());
			return;
		}
	}

	s32 instanceIndex = mInstanceIndex;
	f32 stretchHeight = getChuuHanaParams()->mSLStretchHeightS.get();
	if (instanceIndex > 0)
		stretchHeight = getChuuHanaParams()->mSLStretchHeightM.get();
	if (instanceIndex > 2)
		stretchHeight = getChuuHanaParams()->mSLStretchHeightL.get();

	if (height > stretchHeight) {
		unk1B1 = 0;
		unk214 = 0;
		setBckAnm(8);
		mSpine->pushNerve(&TNerveChuuHanaForceJumped::theNerve());
		return;
	}

	stretchHeight = getChuuHanaParams()->mSLMediumStretchHeightS.get();
	if (instanceIndex > 0)
		stretchHeight = getChuuHanaParams()->mSLMediumStretchHeightM.get();
	if (instanceIndex > 2)
		stretchHeight = getChuuHanaParams()->mSLMediumStretchHeightL.get();

	if (height > stretchHeight) {
		unk1B1 = 0;
		unk214 = 0;
		setBckAnm(9);
		mSpine->pushNerve(&TNerveChuuHanaForceJumped::theNerve());
		return;
	}

	stretchHeight = getChuuHanaParams()->mSLSmallStretchHeightS.get();
	if (instanceIndex > 0)
		stretchHeight = getChuuHanaParams()->mSLSmallStretchHeightM.get();
	if (instanceIndex > 2)
		stretchHeight = getChuuHanaParams()->mSLSmallStretchHeightL.get();

	if (height > stretchHeight) {
		unk1B1 = 0;
		unk214 = 0;
		setBckAnm(10);
		mSpine->pushNerve(&TNerveChuuHanaForceJumped::theNerve());
	}
}

void TChuuHana::bind()
{
	if (mSpine->getCurrentNerve() != &TNerveChuuHanaRoll::theNerve()
	    && mSpine->getCurrentNerve() != &TNerveChuuHanaFall2::theNerve()
	    && mSpine->getCurrentNerve()
	           != &TNerveChuuHanaJumpPrepare::theNerve()) {
		TLiveActor::bind();
		return;
	}

	JGeometry::TVec3<f32> next = mPosition;
	next.add(mLinearVelocity);
	next.add(mVelocity);

	mVelocity.y -= getGravityY();
	if (mVelocity.y < TLiveActor::mVelocityMinY)
		mVelocity.y = TLiveActor::mVelocityMinY;

	mGroundHeight = gpMap->checkGround(
	    next.x, next.y + mHeadHeight, next.z, &mGroundPlane);
	mGroundHeight += 1.0f;

	if (next.y <= mGroundHeight + 0.05f && mGroundPlane->getActor() == nullptr
	    && mPosition.y < unk1F8.y - 2.5f) {
		offLiveFlag(LIVE_FLAG_AIRBORNE);
		next.y = mGroundHeight;
	} else {
		onLiveFlag(LIVE_FLAG_AIRBORNE);
	}

	gpMap->isTouchedOneWallAndMoveXZ(
	    &next.x, next.y + mHeadHeight, &next.z, mBodyRadius);
	JGeometry::TVec3<f32> linearVelocity = next;
	linearVelocity.sub(mPosition);
	mLinearVelocity = linearVelocity;
}

void TChuuHana::moveObject()
{
	TWalkerEnemy::moveObject();

	if (unk1A0 == 0) {
		unk19C = mPosition.y;
		unk198 = mPosition.y;
		unk1A8 = 0.0f;
	} else {
		++unk1A0;
		if (unk198 > mPosition.y)
			unk198 = mPosition.y;
		if (unk19C < mPosition.y)
			unk19C = mPosition.y;

		if (unk1A0 > getChuuHanaParams()->mSLCheckFrame.get()) {
			f32 stretch = unk19C - unk198;
			if (unk1A8 < stretch)
				unk1A8 = stretch;

			if (mPosition.y < unk19C) {
				unk1A8 /= (f32)unk1A0;
				checkStretchType();
			} else {
				if (mSpine->getCurrentNerve()
				    == &TNerveChuuHanaKeepBalance::theNerve())
					setBckAnm(7);
				unk1A0 = 1;
				unk19C = mPosition.y;
				unk198 = mPosition.y;
			}
		}
	}

	if (mSpine->getCurrentNerve() == &TNerveChuuHanaRoll::theNerve()
	    && !isAirborne()) {
		f32 power = getChuuHanaParams()->mSLGetGroundPow.get();
		JGeometry::TVec3<f32> vel(mGroundPlane->mNormal.x * power, 0.0f,
		                           mGroundPlane->mNormal.z * power);
		JGeometry::TVec3<f32> nextVel = mVelocity;
		PSVECAdd((Vec*)&nextVel, (Vec*)&vel, (Vec*)&nextVel);
		nextVel.y = 0.0f;
		mVelocity = nextVel;
		mPosition.y += 5.0f;
		onLiveFlag(LIVE_FLAG_AIRBORNE);
	}

	unk194 = MsClamp(unk194 - 0.1f, 0.0f, 1.0f);

	if (mMActor->unkC != nullptr)
		mMActor->unkC->setMotionBlendRatio(unk194);

	unk1EC = mLinearVelocity;

	if (!isAirborne()
	    && (mGroundPlane->getActor() == nullptr
	        || mGroundPlane->getActor() != unk218))
		kill();
}

bool TChuuHana::isCollidMove(THitActor* actor)
{
	if (actor->isActorType(0x10000016)) {
		TChuuHana* other = (TChuuHana*)actor;

		if ((other->mSpine->getCurrentNerve()
		     == &TNerveChuuHanaRoll::theNerve())
		    ? true
		    : false) {
			if (mSpine->getCurrentNerve()
			    == &TNerveChuuHanaWalkOnPanel::theNerve())
				mSpine->pushNerve(&TNerveChuuHanaRoll::theNerve());
		} else if (unk1B2 == 0
		           && mSpine->getCurrentNerve()
		               != &TNerveChuuHanaAttack::theNerve()
		           && other->mInstanceIndex > mInstanceIndex) {
			TMsRange<s32> rollRange(0, 100);
			int roll = rollRange.rand();
			if (roll % 4 == 0) {
				unk1A4 = mCheckOnPanelTime;

				TGraphWeb* graph = unk124->unk0;
				TMsRange<s32> nodeRange(0, graph->getNodeNum());
				int index = nodeRange.rand();
				JGeometry::TVec3<f32> point;
				graph->getGraphNode(index).getPoint((Vec*)&point);

				TPathNode node(point);
				unkF4  = node;
				unk104 = node;
				unk114.clear();
				unk1B2 = 1;
			}
		}
	}

	return mSpine->getCurrentNerve() == &TNerveChuuHanaObject::theNerve()
	    ? false
	    : true;
}

void TChuuHana::calcRootMatrix()
{
	gpCurChuuHana = this;

	if (mSpine->getCurrentNerve() == &TNerveChuuHanaJumpPrepare::theNerve()
	    || mSpine->getCurrentNerve() == &TNerveChuuHanaFall2::theNerve()) {
		J3DModel* model = mMActor->getModel();
		MsMtxSetXYZRPH(model->getBaseTRMtx(), mPosition.x,
		               mPosition.y + unk220, mPosition.z, mRotation.x,
		               mRotation.y, mRotation.z);
		model->setBaseScale(mScaling);
	} else {
		TSpineEnemy::calcRootMatrix();
	}

	if (mSpine->getCurrentNerve() == &TNerveChuuHanaKeepBalance::theNerve()) {
		gpMarioParticleManager->emitParticleCallBack(
		    0x130, &mPosition, 1, &mAseCallback, this);
	}

	if (mCurrentBckAnm == 6
	    && mMActor->getFrameCtrl(0)->checkPass(2.0f)) {
		JPABaseEmitter* emitter
		    = gpMarioParticleManager->emitAndBindToMtxPtr(
		        0x54, mMActor->getModel()->getAnmMtx(mBodyJntIndex), 0,
		        nullptr);
		if (emitter != nullptr) {
			emitter->unk154 = mScaling;
			emitter->unk174 = mScaling;
		}
	}
}

void TChuuHana::attackToMario()
{
	if (mSpine->getCurrentNerve() != &TNerveChuuHanaObject::theNerve()) {
		if (mDamageSw != 0) {
			SMS_SendMessageToMario(this, 0xe);
			return;
		}

		if (mSpine->getCurrentNerve() == &TNerveChuuHanaAttack::theNerve()) {
			if (!SMS_IsMarioTouchGround4cm())
				return;

			SMS_SendMessageToMario(this, 7);

			JGeometry::TVec3<f32> diff = mPosition;
			diff.sub(*gpMarioPos);
			f32 yaw = MsGetRotFromZaxisY(diff);

			Mtx mtx;
			MsMtxSetRotRPH(mtx, 0.0f, yaw, 0.0f);

			JGeometry::TVec3<f32> dir;
			dir.x = 0.0f;
			dir.y = 1.0f;
			dir.z = -1.0f;
			PSMTXMultVec(mtx, (Vec*)&dir, (Vec*)&dir);

			SMS_ThrowMario(dir, getChuuHanaParams()->mSLTacklePow.get());
			*unk21C = 0;
			return;
		}

		SMS_SendMessageToMario(this, 0xe);
	} else {
		unk215 = 1;
	}
}

BOOL TChuuHana::receiveMessage(THitActor* sender, u32 message)
{
	if (unk1A0 == 0
	    && (mSpine->getCurrentNerve()
	            == &TNerveChuuHanaWalkOnPanel::theNerve()
	        || mSpine->getCurrentNerve()
	            == &TNerveChuuHanaKeepBalance::theNerve())
	    && message == HIT_MESSAGE_HIP_DROP) {
		unk1A0 = 1;
	}

	if (message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		if (mSprayedByWaterCooldown == 0) {
			mSprayedByWaterCooldown = 1;
			behaveToWater(sender);
		}
		unk165 = true;
		gpMarioParticleManager->emit(0xe7, &sender->mPosition, 0, nullptr);
		gpMSound->startSoundSet(0x6802, &mPosition, 0, 0.0f, 0, 0, 4);
		return TRUE;
	}

	return FALSE;
}

void TChuuHana::behaveToWater(THitActor* actor)
{
	unk165 = true;
	unk224 = 0;

	if (mSpine->getCurrentNerve() == &TNerveChuuHanaRoll::theNerve()) {
		JGeometry::TVec3<f32> dir;
		dir.set(mPosition.x - gpMarioPos->x, 0.0f,
		        mPosition.z - gpMarioPos->z);
		MsVECNormalize((Vec*)&dir, (Vec*)&dir);
		dir.scale(getChuuHanaParams()->mSLGetWaterPow.get());

		JGeometry::TVec3<f32> vel = mVelocity;
		PSVECAdd((Vec*)&vel, (Vec*)&dir, (Vec*)&vel);
		vel.y = 0.0f;
		mVelocity = vel;
		onLiveFlag(LIVE_FLAG_AIRBORNE);
		mPosition.y += 10.0f;
		return;
	}

	if (mSpine->getCurrentNerve() == &TNerveChuuHanaWalkOnPanel::theNerve()
	    || mSpine->getCurrentNerve() == &TNerveChuuHanaAttack::theNerve()
	    || mSpine->getCurrentNerve() == &TNerveChuuHanaWait::theNerve()
	    || (mNewSw != 0
	        && mSpine->getCurrentNerve()
	            == &TNerveChuuHanaStick::theNerve())) {
		unk165 = true;
		if (mAttackVersion != 0)
			*unk21C = 1;

		JGeometry::TVec3<f32> dir;
		dir.set(mPosition.x - gpMarioPos->x, 0.0f,
		        mPosition.z - gpMarioPos->z);
		MsVECNormalize((Vec*)&dir, (Vec*)&dir);
		dir.scale(getChuuHanaParams()->mSLGetWaterPow2.get());

		if (!isAirborne()) {
			if (mCompareHeight != 0)
				mPosition.y += 2.0f;
			else
				mPosition.y += 1.0f;
		}

		mVelocity = dir;
		onLiveFlag(LIVE_FLAG_AIRBORNE);

		if (mSpine->getCurrentNerve()
		    != &TNerveChuuHanaStick::theNerve())
			mSpine->pushNerve(&TNerveChuuHanaStick::theNerve());

		mSprayedByWaterCooldown = 0;
		return;
	}

	if (mNewSw == 0
	    && mSpine->getCurrentNerve()
	        == &TNerveChuuHanaKeepBalance::theNerve()) {
		JGeometry::TVec3<f32> dir;
		dir.set(mPosition.x - gpMarioPos->x, 10.0f,
		        mPosition.z - gpMarioPos->z);
		MsVECNormalize((Vec*)&dir, (Vec*)&dir);
		dir.scale(2.0f * getChuuHanaParams()->mSLGetWaterPow.get());

		mVelocity = dir;
		onLiveFlag(LIVE_FLAG_AIRBORNE);
		mPosition.y += 20.0f;
	}
}

void TChuuHanaAseParCallback::execute(JPABaseEmitter* emitter, JPABaseParticle*)
{
	TChuuHana* owner = mOwner;
	if (!owner->checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
			emitter->setGlobalRTMatrix(
			    owner->mMActor->getModel()->getAnmMtx(TChuuHana::mEyeJntIndex));
			emitter->unk154.x = 2.5f;
			emitter->unk154.y = 2.5f;
			emitter->unk154.z = 2.5f;
			emitter->unk174.x = 2.5f;
			emitter->unk174.y = 2.5f;
			emitter->unk174.z = 2.5f;
		}
	}

void TChuuHanaAseParCallback::draw(JPABaseEmitter*, JPABaseParticle*) { }

static int ChuuHanaBodyCallback(J3DNode* node, int timing)
{
	if (timing == 0) {
		TChuuHana* owner = gpCurChuuHana;
		if (owner == nullptr)
			return 1;
		bool isRoll = owner->mSpine->getCurrentNerve()
		              == &TNerveChuuHanaRoll::theNerve();
		if (!isRoll)
			return 1;

		u16 jointIndex = ((J3DJoint*)node)->getJntNo();
		MtxPtr jointMtx
		    = gpCurChuuHana->getModel()->mNodeMatrices[jointIndex];

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

	return 1;
}

DEFINE_NERVE(TNerveChuuHanaWalkOnPanel, TLiveActor)
{
	TChuuHana* self = chuuHana(spine);
	if (spine->getTime() == 0) {
		self->setWalkAnm();
		*self->unk21C = 0;
	}

	if (self->unk218 == nullptr) {
		if (self->mGroundPlane->getActor() != nullptr) {
			self->unk1F8 = self->mGroundPlane->getActor()->mPosition;
			self->unk218 = (TLiveActor*)self->mGroundPlane->getActor();
		}
	} else {
		self->walkBehavior(2, 1.0f);
	}

	++self->unk1A4;
	if (self->unk1A4 > TChuuHana::mCheckOnPanelTimeRoll) {
		self->unk1A4 = 0;
		if (self->willFall(TChuuHana::mCheckOnPanelTime))
			self->unk1A4 = -100;
	}

	if (!self->isAirborne() && self->mGroundPlane->getActor() == nullptr
	    && self->mPosition.y + 2.5f < self->unk1F8.y)
		spine->pushNerve(&TNerveChuuHanaFall2::theNerve());

	if (self->isReachedToGoalXZ())
		self->setGoal();

	if (*self->unk21C != 0) {
		spine->pushNerve(&TNerveChuuHanaAttack::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaForceJumped, TLiveActor)
{
	TChuuHana* self = chuuHana(spine);
	if (spine->getTime() == 0) {
		self->unk1A4 = TChuuHana::mCheckOnPanelTime;
		TGraphWeb* graph = self->unk124->unk0;
		TMsRange<s32> nodeRange(0, graph->getNodeNum());
		int index = nodeRange.rand();
		JGeometry::TVec3<f32> point;
		graph->getGraphNode(index).getPoint((Vec*)&point);

		TPathNode node(point);
		self->unkF4  = node;
		self->unk104 = node;
		self->unk114.clear();
		self->unk1B2 = 1;
	}

	if (self->unk214 != 0 && self->getCurAnmFrameNo(0) > 80.0f) {
		if (self->mGroundPlane->getActor() != nullptr)
			((TLiveActor*)self->mGroundPlane->getActor())
			    ->receiveMessage(self, 3);
		self->unk214 = 0;
	}

	if (self->checkCurAnmEnd(0)) {
		if (self->unk1B1 != 0) {
			spine->pushAfterCurrent(&TNerveChuuHanaRoll::theNerve());
		} else {
			spine->reset();
			spine->setDefaultNext();
			spine->pushAfterCurrent(spine->getDefault());
		}
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaKeepBalance, TLiveActor)
{
	TChuuHana* self = (TChuuHana*)spine->getBody();
	if (spine->getTime() == 0) {
		self->setBckAnm(2);
		self->mPosition.x -= 10.0f * self->unk1EC.x;
		self->mPosition.z -= 10.0f * self->unk1EC.z;
	} else if (self->checkCurAnmEnd(0)) {
		if (self->isBckAnm(2)) {
			self->setBckAnm(1);
		} else if (self->isBckAnm(1)) {
			if (spine->getTime()
			    > self->getChuuHanaParams()->mSLKeepBalanceTime.get())
				self->setBckAnm(0);
			else
				self->setBckAnm(1);
		} else if (self->isBckAnm(0) || self->isBckAnm(7)) {
			spine->reset();
			spine->setNext(&TNerveChuuHanaKeepBalance::theNerve());
			spine->pushAfterCurrent(spine->getDefault());

			self->unk1A4 = TChuuHana::mCheckOnPanelTime;
			TMsRange<s32> nodeRange(0, self->unk124->unk0->getNodeNum());
			int index = nodeRange.rand();
			JGeometry::TVec3<f32> point;
			self->unk124->unk0->getGraphNode(index).getPoint((Vec*)&point);

			TPathNode node(point);
			self->unkF4  = node;
			self->unk104 = node;
			self->unk114.clear();
			self->unk1B2 = 1;
			return TRUE;
		}
	}

	if (TChuuHana::mAttackVersion != 0)
		*self->unk21C = 1;

	if (TChuuHana::mNewSw == 0 && self->mGroundPlane->getActor() == nullptr) {
		spine->pushAfterCurrent(&TNerveChuuHanaFall::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaStick, TLiveActor)
{
	TChuuHana* self = chuuHana(spine);
	if (spine->getTime() == 0 || !self->isBckAnm(4)) {
		self->setBckAnm(4);
		self->setGoalPath(TPathNode(*gpMarioPos));
		if (TChuuHana::mAttackVersion != 0)
			*self->unk21C = 1;
	} else {
		++self->unk224;
		if (self->unk224
		    > self->getChuuHanaParams()->mSLHitWaterTimer.get()) {
			if (self->checkCurAnmEnd(0)) {
				*self->unk21C = 0;
				spine->pushAfterCurrent(&TNerveChuuHanaWait::theNerve());
				return TRUE;
			}
		} else {
			if (self->checkCurAnmEnd(0))
				self->setBckAnm(4);
		}
	}

	if (TChuuHana::mNewSw != 0
	    && self->willFall(TChuuHana::mCheckOnPanelTimeRoll)) {
		spine->pushAfterCurrent(&TNerveChuuHanaKeepBalance::theNerve());
		return TRUE;
	}

	self->walkBehavior(3, 0.2f);
	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaRoll, TLiveActor)
{
	TChuuHana* self = chuuHana(spine);
	if (spine->getTime() == 0) {
		self->unk210 = 0.0f;
		self->unk204.x = 0.0f;
		self->unk204.y = 0.0f;
		self->unk204.z = 0.0f;
	}

	if (self->unk1B0 != 0) {
		if (self->willFall(TChuuHana::mCheckOnPanelTimeRoll)) {
			spine->pushAfterCurrent(&TNerveChuuHanaKeepBalance::theNerve());
			return TRUE;
		}
	} else if (spine->getTime() > 5000) {
		return TRUE;
	}

	JGeometry::TVec3<f32> velocity = self->mVelocity;
	JGeometry::TVec3<f32> target   = velocity;
	f32 deltaX = target.x - self->unk204.x;
	deltaX *= 0.2f;
	self->unk204.x += deltaX;
	f32 deltaZ = target.z - self->unk204.z;
	deltaZ *= 0.2f;
	self->unk204.z += deltaZ;

	f32 speed = JGeometry::TUtil<f32>::sqrt(
	    self->unk204.x * self->unk204.x + self->unk204.z * self->unk204.z);
	self->unk1B8 = speed / self->mBodyRadius * 2.0f;
	self->unk210 += self->unk1B8;
	if (self->unk210 > 360.0f)
		self->unk210 -= 360.0f;
	if (self->unk210 < 0.0f)
		self->unk210 += 360.0f;

	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaFall, TLiveActor)
{
	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaFall2, TLiveActor)
{
	TChuuHana* self = chuuHana(spine);
	if (spine->getTime() == 0)
		self->setBckAnm(6);

	self->unk220 *= 0.98f;

	if (!self->isAirborne() || spine->getTime() > 800) {
		spine->pushAfterCurrent(&TNerveChuuHanaObject::theNerve());
		self->kill();
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
	if (spine->getTime() == 0) {
		self->setBckAnm(12);
		self->getMActor()->setFrameRate(2.0f * SMSGetAnmFrameRate(), 0);
		self->setGoalPathMario();
	}

	if ((*gpMarioGroundPlane)->getActor() != self->unk218)
		*self->unk21C = 0;

	if (spine->getTime() > self->getChuuHanaParams()->mSLAttackTimer.get()) {
		spine->pushAfterCurrent(&TNerveChuuHanaWalkOnPanel::theNerve());
		spine->pushAfterCurrent(&TNerveChuuHanaWait::theNerve());
		return TRUE;
	}

	++self->unk1A4;
	if (self->unk1A4 > TChuuHana::mCheckOnPanelTimeRoll) {
		self->unk1A4 = 0;
		if (self->willFall(TChuuHana::mCheckOnPanelTime))
			self->unk1A4 = -100;

		if (!self->isAirborne() && self->mGroundPlane->getActor() == nullptr
		    && self->mPosition.y + 200.0f < self->unk1F8.y)
			self->mSpine->pushNerve(&TNerveChuuHanaFall2::theNerve());
	}

	if (self->isReachedToGoalXZ())
		self->setGoalPathMario();

	self->walkBehavior(2, self->getChuuHanaParams()->mSLDashRate.get());
	return FALSE;
}

DEFINE_NERVE(TNerveChuuHanaJumpPrepare, TLiveActor)
{
	TChuuHana* self = chuuHana(spine);
	if (spine->getTime() == 0)
		self->setBckAnm(3);

	MtxPtr footMtx
	    = self->mMActor->getModel()->getAnmMtx(TChuuHana::mFootJntIndex);
	self->unk220   = self->mPosition.y - footMtx[1][3];
	if (self->getMActor()->getFrameCtrl(0)->checkPass(10.0f)) {
		JGeometry::TVec3<f32> target;
		target.x = 2.0f * self->unk1F8.x - self->mPosition.x;
		target.y = 2.0f * self->unk1F8.y - self->mPosition.y;
		target.z = 2.0f * self->unk1F8.z - self->mPosition.z;

		*self->unk21C = 0;
		target.y += self->getChuuHanaParams()->mSLJumpHeight.get();
		f32 jumpSpeed = self->getChuuHanaParams()->mSLJumpSp.get();
		JGeometry::TVec3<f32> velocity
		    = self->calcVelocityToJumpToY(target, jumpSpeed, self->getGravityY());
		velocity.y += 10.0f;
		self->mPosition.y += 10.0f;
		self->mVelocity = velocity;
		self->onLiveFlag(LIVE_FLAG_AIRBORNE);
		self->unk130 = 0;
	}

	return self->checkCurAnmEnd(0) ? TRUE : FALSE;
}

DEFINE_NERVE(TNerveChuuHanaWait, TLiveActor)
{
	TChuuHana* self = chuuHana(spine);
	if (spine->getTime() == 0)
		self->setBckAnm(11);
	return self->checkCurAnmEnd(0) ? TRUE : FALSE;
}
