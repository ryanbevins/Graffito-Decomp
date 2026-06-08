#include <Enemy/TabePuku.hpp>
#include <Enemy/Graph.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <M3DUtil/MActor.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionData.hpp>
#include <Map/MapData.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <Player/MarioAccess.hpp>
#include <System/Application.hpp>
#include <System/Particles.hpp>
#include <math.h>
#include <stdlib.h>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

JGeometry::TQuat4<f32> SMS_Eular2Quat(const JGeometry::TVec3<f32>&);
s16 matan(f32, f32);

static const char* tabepuku_bastable[] = {
	"/scene/tabepuku/bas/pukupuku_chase.bas",
	"/scene/tabepuku/bas/pukupuku_search.bas",
	"/scene/tabepuku/bas/pukupuku_swim.bas",
};

static inline bool isTabePukuAttackNerve(const TNerveBase<TLiveActor>* nerve)
{
	return nerve == &TNerveTabePukuAttack::theNerve()
	       || nerve == &TNerveTabePukuBite::theNerve()
	       || nerve == &TNerveTabePukuDive::theNerve()
	       || nerve == &TNerveTabePukuDrag::theNerve();
}

static inline bool isTabePukuGraphNerve(const TNerveBase<TLiveActor>* nerve)
{
	return nerve == &TNerveTabePukuGraphWander::theNerve()
	       || nerve == &TNerveTabePukuRecoverGraph::theNerve();
}

static inline JGeometry::TVec3<f32> getTabePukuGoal(TTabePuku* self)
{
	if (self->unk104.unk0)
		return self->unk104.unk0->getPosition();
	return self->unk104.unk4;
}

static inline const JGeometry::TVec3<f32>& getTabePukuGoalRef(TTabePuku* self)
{
	if (self->unk104.unk0)
		return self->unk104.unk0->mPosition;
	return self->unk104.unk4;
}

DEFINE_NERVE(TNerveTabePukuDrag, TLiveActor)
{
	TTabePuku* self = (TTabePuku*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mDragDirection.set(0.0f, 0.0f, 1.0f);
		JGeometry::TQuat4<f32> rot;
		rot.setRotate(self->mDragDirection,
		              (rand() * (1.0f / 32768.0f)) * 6.2831855f);
		rot.rotate(self->mDragDirection);
		self->setGoalPath(TPathNode(self->mPosition));
		self->mMarchSpeed = self->getSaveParam2()->mDiveSpeed.get();
	}

	self->swimTo(self->mDragDirection);

	bool shouldRelease = self->mTouchedWall || !self->isAirborne();
	if (!shouldRelease) {
		JGeometry::TVec3<f32> base = getTabePukuGoal(self);
		base.sub(self->mPosition);
		shouldRelease = JGeometry::TUtil<f32>::sqrt(base.dot(base))
		                > self->getSaveParam2()->mDragLength.get();
	}

	if (shouldRelease) {
		SMS_SendMessageToMario(self, HIT_MESSAGE_UNK8);
		self->mHeldObject = nullptr;
		spine->pushAfterCurrent(&TNerveTabePukuRecoverGraph::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveTabePukuDive, TLiveActor)
{
	TTabePuku* self = (TTabePuku*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mDiveStartY = self->mPosition.y;
		self->setBckAnm(2);
		self->getMActor()->getFrameCtrl(0)->setRate(2.0f * SMSGetAnmFrameRate());
		self->mMarchSpeed = self->getSaveParam2()->mDiveSpeed.get();
	}

	JGeometry::TVec3<f32> towardGround(0.0f, self->mGroundHeight - self->mPosition.y,
	                                   0.0f);
	self->swimTo(towardGround);

	bool keepDiving = false;
	if (self->mPosition.y - self->mDiveStartY
	    < -self->getSaveParam2()->mCorrectY.get()) {
		keepDiving = true;
	}
	if (self->mPosition.y - self->mGroundHeight < 200.0f && self->isAirborne())
		keepDiving = true;

	if (keepDiving) {
		spine->pushAfterCurrent(&TNerveTabePukuDrag::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveTabePukuBite, TLiveActor)
{
	TTabePuku* self = (TTabePuku*)spine->getBody();

	self->setBckAnm(2);
	if (gpMSound->gateCheck(0x2922))
		MSoundSESystem::MSoundSE::startSoundActor(0x2922, &self->mPosition, 0,
		                                          nullptr, 0, 4);

	spine->pushAfterCurrent(&TNerveTabePukuDive::theNerve());
	return TRUE;
}

BOOL TNerveTabePukuAttack::execute(TSpineBase<TLiveActor>* spine) const
{
	TTabePuku* self = (TTabePuku*)spine->getBody();

	if (spine->getTime() == 0) {
		self->setBckAnm(0);
		self->setGoalPath(TPathNode((THitActor*)gpMarioAddress));
		self->mMarchSpeed = self->getSaveParam2()->mAttackSpeed.get();
	}

	bool giveUp = false;
	if (fabsf(gpMarioPos->y - self->mPosition.y)
	    > self->getSaveParam2()->getSLGiveUpHeight()) {
		giveUp = true;
	} else {
		f32 giveUpLength = self->getSaveParam2()->getSLGiveUpLength();
		JGeometry::TVec3<f32> goal = getTabePukuGoalRef(self);
		goal.sub(self->mPosition);
		if (JGeometry::TUtil<f32>::sqrt(goal.dot(goal)) > giveUpLength) {
			giveUp = true;
		} else {
			JGeometry::TVec3<f32> graphPos
			    = self->getTracer()->getGraph()->getNearestPosOnGraphLink(
			        self->mPosition);
			graphPos.sub(self->mPosition);
			f32 territory = self->getSaveParam2()->mTerritoryRange.get();
			if (territory * territory <= graphPos.dot(graphPos))
				giveUp = true;
		}
	}

	if (giveUp || self->mTouchedWall) {
		spine->pushAfterCurrent(&TNerveTabePukuRecoverGraph::theNerve());
		return TRUE;
	}

	JGeometry::TVec3<f32> towardMario = getTabePukuGoalRef(self);
	towardMario.sub(self->mPosition);
	{
		JGeometry::TVec3<f32> offset(0.0f, 150.0f, 0.0f);
		towardMario.add(offset);
	}
	self->swimTo(towardMario);
	return FALSE;
}
DEFINE_NERVE(TNerveTabePukuRecoverGraph, TLiveActor)
{
	TTabePuku* self = (TTabePuku*)spine->getBody();

	if (spine->getTime() == 0) {
		if (self->getTracer()) {
			self->getTracer()->mPrevIdx = -1;
			self->getTracer()->mCurrIdx = -1;
		}
		self->goToShortestNextGraphNode();
		self->mMarchSpeed = self->getSaveParam2()->mMarchSpeed.get();
	}

	if (self->isReachedToGoal()) {
		spine->pushAfterCurrent(&TNerveTabePukuGraphWander::theNerve());
		return TRUE;
	}

	JGeometry::TVec3<f32> goal = getTabePukuGoal(self);
	goal.sub(self->mPosition);
	if (self->isAirborne() || self->mTouchedWall)
		goal.y += 10000.0f;
	self->swimTo(goal);
	return FALSE;
}

BOOL TNerveTabePukuFound::execute(TSpineBase<TLiveActor>* spine) const
{
	TTabePuku* self = (TTabePuku*)spine->getBody();

	if (spine->getTime() == 0) {
		self->setBckAnm(1);
		self->mMarchSpeed = 0.0f;
	}

	JGeometry::TVec3<f32> forward;
	self->mQuat.getZDir(forward);

	JGeometry::TVec3<f32> velocity(self->mVelocity);
	velocity.scale(self->getSaveParam2()->mWaterFric.get());
	velocity.x += forward.x * self->mMarchSpeed;
	velocity.y += forward.y * self->mMarchSpeed;
	velocity.z += forward.z * self->mMarchSpeed;
	self->mVelocity = velocity;

	f32 rotation;
	if (velocity.z == 0.0f) {
		if (velocity.x >= 0.0f)
			rotation = 90.0f;
		else
			rotation = -90.0f;
	} else if (velocity.z >= 0.0f) {
		rotation = matan(velocity.z, velocity.x) * (360.0f / 65536.0f);
	} else {
		rotation
		    = 180.0f
		      - matan(-velocity.z, velocity.x) * (360.0f / 65536.0f);
	}
	self->mRotation.y = rotation;

	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveTabePukuAttack::theNerve());
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTabePukuGraphWander::execute(TSpineBase<TLiveActor>* spine) const
{
	TTabePuku* self = (TTabePuku*)spine->getBody();

	if (spine->getTime() == 0) {
		self->goToShortestNextGraphNode();
		self->mMarchSpeed = self->getSaveParam2()->mMarchSpeed.get();
		self->setBckAnm(2);
	}

	if (self->isFindMario(self->getSaveParam2()->mTerritoryRange.get())) {
		spine->pushAfterCurrent(&TNerveTabePukuFound::theNerve());
		return TRUE;
	}

	if (self->isReachedToGoal())
		self->goToShortestNextGraphNode();

	JGeometry::TVec3<f32> goal = getTabePukuGoal(self);
	goal.sub(self->mPosition);
	self->swimTo(goal);
	return FALSE;
}

void TTabePukuManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "tabepuku.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};

	createModelDataArray(entry);
}

void TTabePukuManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TTabePukuSaveLoadParams("/enemy/tabepuku.prm");
	TSmallEnemyManager::load(stream);
}

TTabePukuManager::TTabePukuManager(const char* name)
    : TSmallEnemyManager(name)
{
}

TTabePuku::TTabePuku(const char* name)
    : TSmallEnemy(name)
{
	onLiveFlag(LIVE_FLAG_UNK1000);
}

#pragma dont_inline on
void TTabePuku::swimTo(const JGeometry::TVec3<f32>& target)
{
	JGeometry::TVec3<f32> dir(target);
	if (!dir.isZero())
		dir.normalize();

	JGeometry::TVec3<f32> forward;
	mQuat.getZDir(forward);

	JGeometry::TQuat4<f32> steer;
	steer.setRotate(forward, dir, getSaveParam2()->mTurnSlerpRate.get());
	mQuat.mul(steer);
	mQuat.normalize();

	JGeometry::TVec3<f32> desired;
	desired.scale(mMarchSpeed, dir);
	mVelocity.scale(getSaveParam2()->mWaterFric.get());
	mVelocity.add(desired);
}
#pragma dont_inline off

bool TTabePuku::doKeepDistance()
{
	if (isTabePukuAttackNerve(mSpine->getLatestNerve()))
		return false;
	return true;
}

bool TTabePuku::isFindMario(float range)
{
	return isFindMarioFromParam(range);
}

void TTabePuku::forceKill() { }

void TTabePuku::behaveToWater(THitActor*) { }

void TTabePuku::attackToMario()
{
	const TNerveBase<TLiveActor>* nerve = mSpine->getLatestNerve();
	if (isTabePukuAttackNerve(nerve) || isTabePukuGraphNerve(nerve))
		return;

	if (SMS_SendMessageToMario(this, HIT_MESSAGE_TAKE)) {
		mHeldObject = (TTakeActor*)SMS_GetMarioHitActor();
		mSpine->reset();
		mSpine->setNext(&TNerveTabePukuBite::theNerve());
	}
}

const char** TTabePuku::getBasNameTable() const { return tabepuku_bastable; }

MtxPtr TTabePuku::getTakingMtx()
{
	f32 x = mQuat.x;
	f32 y = mQuat.y;
	f32 z = mQuat.z;
	f32 w = mQuat.w;

	mTakingMtx[0][0] = 1.0f - 2.0f * y * y - 2.0f * z * z;
	mTakingMtx[0][1] = 2.0f * x * y - 2.0f * w * z;
	mTakingMtx[0][2] = 2.0f * x * z + 2.0f * w * y;
	mTakingMtx[1][0] = 2.0f * x * y + 2.0f * w * z;
	mTakingMtx[1][1] = 1.0f - 2.0f * x * x - 2.0f * z * z;
	mTakingMtx[1][2] = 2.0f * y * z - 2.0f * w * x;
	mTakingMtx[2][0] = 2.0f * x * z - 2.0f * w * y;
	mTakingMtx[2][1] = 2.0f * y * z + 2.0f * w * x;
	mTakingMtx[2][2] = 1.0f - 2.0f * x * x - 2.0f * y * y;

	f32 correctZ = getSaveParam2()->mCorrectZ.get();
	f32 correctY = getSaveParam2()->mCorrectY.get();
	mTakingMtx[0][3] = mPosition.x + mTakingMtx[0][2] * correctZ
	                   + mTakingMtx[0][1] * correctY;
	mTakingMtx[1][3] = mPosition.y + mTakingMtx[1][2] * correctZ
	                   + mTakingMtx[1][1] * correctY;
	mTakingMtx[2][3] = mPosition.z + mTakingMtx[2][2] * correctZ
	                   + mTakingMtx[2][1] * correctY;

	return mTakingMtx;
}

BOOL TTabePuku::receiveMessage(THitActor* sender, u32 message)
{
	if ((s32)message < 2 && (s32)message >= 0)
		return FALSE;
	return TSmallEnemy::receiveMessage(sender, message);
}

void TTabePuku::calcRootMatrix()
{
	if (isTaken()) {
		TSpineEnemy::calcRootMatrix();
		return;
	}

	MtxPtr mtx = getTakingMtx();
	J3DModel* model = getModel();
	model->setBaseScale(mScaling);
	model->setBaseTRMtx(mtx);

	JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
	SMS_EasyEmitParticle((E_SMS_EFFECT_LOOP_NORMAL)0x178,
	                     model->getAnmMtx(mMouthJointIndex), this, scale);
}

void TTabePuku::bind()
{
	TTabePukuSaveLoadParams* params = getSaveParam2();
	mHitActor->mAttackRadius = (f32)params->mSLAttackRadius.get();
	mHitActor->mAttackHeight = (f32)params->mSLAttackHeight.get();
	mHitActor->mDamageRadius = (f32)params->mSLDamageRadius.get();
	mHitActor->mDamageHeight = (f32)params->mSLDamageHeight.get();
	mHitActor->calcEntryRadius();

	mHitActor->updateTerrainCollsion();
	mHitActor->bind();

	mLinearVelocity = mHitActor->mMove;
	mTouchedWall    = mHitActor->mTouchedWall;
	if (mHitActor->mIsAirborne)
		onLiveFlag(LIVE_FLAG_AIRBORNE);
	else
		offLiveFlag(LIVE_FLAG_AIRBORNE);
	mGroundPlane  = mHitActor->mGroundPlane;
	mGroundHeight = mHitActor->mGroundHeight;
}

void TTabePuku::control()
{
	TLiveActor::control();

	for (int i = 0; i < mHitActor->getColNum(); ++i) {
		THitActor* hit = mHitActor->getCollision(i);
		if (hit->mActorType == 0x80000001)
			mHitActor->mOwner->forceKill();
	}

	const TNerveBase<TLiveActor>* nerve = mSpine->getLatestNerve();
	if (nerve == &TNerveTabePukuBite::theNerve()
	    || nerve == &TNerveTabePukuDive::theNerve()
	    || nerve == &TNerveTabePukuDrag::theNerve()) {
		if (gpMSound->gateCheck(0x2123))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x2123, &mPosition, 0, nullptr, 0, 4);
	}
}

void TTabePuku::perform(u32 flags, JDrama::TGraphics* graphics)
{
	mHitActor->perform(flags, graphics);
	TSmallEnemy::perform(flags, graphics);
}

void TTabePuku::reset() { mScaledBodyRadius = 130.0f; }

void TTabePuku::init(TLiveManager* manager)
{
	mManager = manager;
	mManager->manageActor(this);
	setMActorAndKeeper();
	mSpine->initWith(&TNerveTabePukuGraphWander::theNerve());

	initHitActor(0x10000035, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	onHitFlag(HIT_FLAG_NO_COLLISION);

	mHitActor = new TTPHitActor(this, "タベプク補助当たり");
	mHitActor->init();
	mHitActor->mPosition = mPosition;

	mQuat = SMS_Eular2Quat(mRotation);
	mMouthJointIndex
	    = getModel()->getModelData()->getJointName()->getIndex("jnt_mouth_up");
	initAnmSound();
}

void TTPHitActor::bind()
{
	JGeometry::TVec3<f32> next(mPosition);
	next.add(mMove);
	next.add(mOwner->mVelocity);
	next.add(mOwner->mLinearVelocity);

	f32 ground = gpMap->checkGroundIgnoreWaterSurface(
	    next.x, next.y + mCheckHeight, next.z, &mGroundPlane);
	mGroundHeight = ground + 1.0f;

	if (next.y <= mGroundHeight + 0.05f) {
		mIsAirborne = false;
		next.y      = mGroundHeight;
	} else {
		mIsAirborne = true;
	}

	if (0.0f >= next.y + mCheckHeight)
		next.y = -mCheckHeight;

	TBGWallCheckRecord record(next.x, next.y, next.z, mCheckRadius, 1, 0);
	mTouchedWall = gpMap->isTouchedWallsAndMoveXZ(&record);
	next.x       = record.mCenter.x;
	next.z       = record.mCenter.z;

	mMove.set(next);
	mMove.sub(mPosition);
	mPosition = next;
}

void TTPHitActor::updateTerrainCollsion()
{
	f32 yOffset = -0.6666667f * mAttackHeight;
	JGeometry::TVec3<f32> up;
	JGeometry::TQuat4<f32> quat(mOwner->mQuat);
	quat.getYDir(up);

	mCheckHeight = mAttackHeight;
	mCheckRadius = mAttackRadius;

	TTakeActor* heldObject = mOwner->mHeldObject;
	int hasHeldObject      = heldObject != nullptr ? 1 : 0;
	if (hasHeldObject) {
		yOffset += heldObject->mDamageHeight;
		mCheckHeight += heldObject->mDamageHeight;
		mCheckRadius += mOwner->mHeldObject->mDamageRadius;
	}

	JGeometry::TVec3<f32> next;
	next.scaleAdd(0.5f * mAttackHeight, mOwner->mPosition, up);

	JGeometry::TVec3<f32> offset(0.0f, 1.0f, 0.0f);
	next.scaleAdd(yOffset, next, offset);

	JGeometry::TVec3<f32> move(next);
	move.sub(mPosition);
	mMove = move;
	mPosition = next;
}

BOOL TTPHitActor::receiveMessage(THitActor* sender, u32 message)
{
	return mOwner->receiveMessage(sender, message);
}

void TTPHitActor::init()
{
	initHitActor(0x10000035, 1, 0x80000000, 10.0f, 10.0f, 10.0f, 10.0f);
	offHitFlag(HIT_FLAG_NO_COLLISION);
	onHitFlag(HIT_FLAG_UNK4);

	JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ")->add(this);
}
