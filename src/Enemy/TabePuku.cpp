#define JGEOMETRY_ROTATION3_SETQUAT_OUT_OF_LINE
#include <Enemy/TabePuku.hpp>
#undef JGEOMETRY_ROTATION3_SETQUAT_OUT_OF_LINE
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
#include <MarioUtil/MathUtil.hpp>
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

namespace {
f32 cAngleMax = JGeometry::TUtil<f32>::PI() / 8.0f;
}

static const char* tabepuku_bastable[] = {
	"/scene/tabepuku/bas/pukupuku_chase.bas",
	"/scene/tabepuku/bas/pukupuku_search.bas",
	"/scene/tabepuku/bas/pukupuku_swim.bas",
};

static inline bool
isTabePukuDiveOrDragNerve(const TNerveBase<TLiveActor>* nerve)
{
	return nerve == &TNerveTabePukuDive::theNerve()
	       || nerve == &TNerveTabePukuDrag::theNerve();
}

static inline bool isTabePukuHoldingNerve(const TNerveBase<TLiveActor>* nerve)
{
	return nerve == &TNerveTabePukuBite::theNerve()
	       || nerve == &TNerveTabePukuDive::theNerve()
	       || nerve == &TNerveTabePukuDrag::theNerve();
}

static inline bool isTabePukuHoldingNerve(TTabePuku* self)
{
	return isTabePukuHoldingNerve(self->mSpine->getLatestNerve());
}

static inline bool isTabePukuAttackNerve(TTabePuku* self)
{
	return self->mSpine->getLatestNerve()
	           == &TNerveTabePukuAttack::theNerve()
	       || isTabePukuHoldingNerve(self);
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

void TTabePukuManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "tabepuku.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};

	createModelDataArray(entry);
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

void TTabePuku::swimTo(const JGeometry::TVec3<f32>& target)
{
	JGeometry::TVec3<f32> dir(target);
	f32 epsilon = JGeometry::TUtil<f32>::epsilon();
	f32 lengthDiff = target.squared() - JGeometry::TUtil<f32>::zero();
	bool isZero = false;
	if (-epsilon <= lengthDiff && lengthDiff <= epsilon)
		isZero = true;
	if (isZero) {
		JGeometry::TVec3<f32> forward;
		mQuat.getZDir(forward);
		forward.scale(mMarchSpeed);
		JGeometry::TVec3<f32> velocity(mVelocity);
		velocity.scale(getSaveParam2()->mWaterFric.get());
		velocity.add(forward);
		mVelocity = velocity;
		mRotation.y = MsGetRotFromZaxisY(velocity);
		return;
	}

	{
		dir.normalize();

		JGeometry::TQuat4<f32> targetQuat;
		JGeometry::TVec3<f32> front(0.0f, 0.0f, 1.0f);
		f32 dotDiff = dir.dot(front) - -1.0f;
		bool opposite = false;
		if (-epsilon <= dotDiff && dotDiff <= epsilon)
			opposite = true;
		if (opposite)
			targetQuat.setEulerY(JGeometry::TUtil<f32>::PI());
		else
			targetQuat.setRotate(front, dir, 1.0f);
		mQuat.slerp(targetQuat, getSaveParam2()->mTurnSlerpRate.get());
		mQuat.normalize();
	}

	JGeometry::TVec3<f32> forward;
	mQuat.getZDir(forward);
	forward.scale(mMarchSpeed);

	JGeometry::TVec3<f32> velocity(mVelocity);
	velocity.scale(getSaveParam2()->mWaterFric.get());
	velocity.add(forward);
	mVelocity = velocity;

	mRotation.y = MsGetRotFromZaxisY(velocity);
}

bool TTabePuku::doKeepDistance()
{
	return !isTabePukuAttackNerve(this);
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
	if (!isTabePukuHoldingNerve(nerve)
	    && !isTabePukuGraphNerve(mSpine->getLatestNerve())) {
		if (SMS_SendMessageToMario(this, HIT_MESSAGE_TAKE)) {
			mHeldObject = (TTakeActor*)SMS_GetMarioHitActor();
			mSpine->reset();
			mSpine->setNext(&TNerveTabePukuBite::theNerve());
		}
	}
}

const char** TTabePuku::getBasNameTable() const { return tabepuku_bastable; }

MtxPtr TTabePuku::getTakingMtx()
{
	f32 x = mQuat.x;
	f32 y = mQuat.y;
	f32 z = mQuat.z;
	f32 w = mQuat.w;
	f32* row1 = mTakingMtx[1];
	f32* row2 = mTakingMtx[2];

	f32 ty = 2.0f * y;
	f32 tz = 2.0f * z;
	f32 tx = 2.0f * x;
	f32 tw = 2.0f * w;

	mTakingMtx[0][0] = 1.0f - ty * y - tz * z;
	mTakingMtx[0][1] = tx * y - tw * z;
	mTakingMtx[0][2] = tx * z + tw * y;
	row1[0] = tx * y + tw * z;
	row1[1] = 1.0f - tx * x - tz * z;
	row1[2] = ty * z - tw * x;
	row2[0] = tx * z - tw * y;
	row2[1] = ty * z + tw * x;
	row2[2] = 1.0f - tx * x - ty * y;

	f32 zAxisZ = row2[2];
	f32 zAxisY = row1[2];
	f32 zAxisX = mTakingMtx[0][2];
	f32 yAxisZ = row2[1];
	f32 yAxisY = row1[1];
	f32 yAxisX = mTakingMtx[0][1];

	f32 correctZ = getSaveParam2()->mCorrectZ.get();
	f32 transX   = mPosition.x + zAxisX * correctZ;
	f32 transY   = mPosition.y + zAxisY * correctZ;
	f32 transZ   = mPosition.z + zAxisZ * correctZ;

	f32 correctY = getSaveParam2()->mCorrectY.get();
	mTakingMtx[0][3] = yAxisX * correctY + transX;
	row1[3] = yAxisY * correctY + transY;
	row2[3] = yAxisZ * correctY + transZ;

	return mTakingMtx;
}

BOOL TTabePuku::receiveMessage(THitActor* sender, u32 message)
{
	switch ((s32)message) {
	case 0:
	case 1:
		return FALSE;
	default:
		return TSmallEnemy::receiveMessage(sender, message);
	}
}

inline bool TTabePuku::isAttacking() const
{
	return mSpine->getLatestNerve() == &TNerveTabePukuAttack::theNerve();
}

inline void TTabePuku::emitEffects()
{
	JPABaseEmitter* emitter = SMS_EasyEmitParticle(
	    (E_SMS_EFFECT_LOOP_NORMAL)0x178, getModel()->getAnmMtx(mMouthJointIndex),
	    this, JGeometry::TVec3<f32>(1.0f, 1.0f, 1.0f));
	if (emitter) {
		f32 lifeScale = -mPosition.y / 100.0f;
		if (lifeScale <= 0.0f)
			lifeScale = 0.0f;

		s32 life = (s32)lifeScale * 20 + 2;
		if (life > 200)
			life = 200;
		emitter->mBaseLifetime = life;

		if (isAttacking())
			emitter->mChildSpawnRate = 0.1f;
	}
}

void TTabePuku::calcRootMatrix()
{
	if (isTaken()) {
		TSpineEnemy::calcRootMatrix();
		return;
	}

	Mtx mtx;
	((JGeometry::TRotation3<
	     JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >*)&mtx)
	    ->setQuat(mQuat);
	mtx[0][3] = mPosition.x;
	mtx[1][3] = mPosition.y;
	mtx[2][3] = mPosition.z;

	getModel()->setBaseScale(mScaling);
	MtxPtr transformMtx = mtx;
	MtxPtr baseMtx      = getModel()->getBaseTRMtx();
	PSMTXCopy(transformMtx, baseMtx);

	emitEffects();
}

void TTabePuku::bind()
{
	TTPHitActor* hitActor = mHitActor;
	f32 damageHeight
	    = (f32)hitActor->mOwner->getSaveParam2()->mSLDamageHeight.get();
	f32 damageRadius
	    = (f32)hitActor->mOwner->getSaveParam2()->mSLDamageRadius.get();
	f32 attackHeight
	    = (f32)hitActor->mOwner->getSaveParam2()->mSLAttackHeight.get();
	hitActor->mAttackRadius
	    = (f32)hitActor->mOwner->getSaveParam2()->mSLAttackRadius.get();
	hitActor->mAttackHeight = attackHeight;
	hitActor->mDamageRadius = damageRadius;
	hitActor->mDamageHeight = damageHeight;
	hitActor->calcEntryRadius();

	mHitActor->updateTerrainCollsion();
	mHitActor->bind();

	mLinearVelocity = mHitActor->mMove;
	mTouchedWall    = mHitActor->mTouchedWall;
	s32 isAirborne = mHitActor->mIsAirborne;
	if (isAirborne != 0)
		onLiveFlag(LIVE_FLAG_AIRBORNE);
	else
		offLiveFlag(LIVE_FLAG_AIRBORNE);
	mGroundPlane  = mHitActor->mGroundPlane;
	mGroundHeight = mHitActor->mGroundHeight;
}

inline void TTPHitActor::checkHitActors()
{
	THitActor** end = mCollisions + mColCount;
	THitActor** it = mCollisions;
	for (; it != end; ++it) {
		THitActor* hit = *it;
		switch (hit->mActorType) {
		case 0x80000001:
			mOwner->attackToMario();
			break;
		}
	}
}

inline bool TTabePuku::isBiting() const
{
	const TNerveBase<TLiveActor>* nerve = mSpine->getLatestNerve();
	return nerve == &TNerveTabePukuBite::theNerve()
	       || nerve == &TNerveTabePukuDive::theNerve()
	       || nerve == &TNerveTabePukuDrag::theNerve();
}

inline void TTabePuku::updateSound()
{
	if (isBiting()) {
		if (gpMSound->gateCheck(0x2123))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x2123, &mPosition, 0, nullptr, 0, 4);
	}
}

void TTabePuku::control()
{
	TLiveActor::control();
	mHitActor->checkHitActors();
	updateSound();
}

void TTabePuku::perform(u32 flags, JDrama::TGraphics* graphics)
{
	mHitActor->perform(flags, graphics);
	TSmallEnemy::perform(flags, graphics);
}

void TTabePuku::reset() { mScaledBodyRadius = 130.0f; }

void TTPHitActor::bind()
{
	JGeometry::TVec3<f32> next(mPosition);
	next.add(mMove);
	JGeometry::TVec3<f32> velocity(mOwner->mVelocity);
	next.add(velocity);
	next.add(mOwner->mLinearVelocity);

	mGroundHeight = gpMap->checkGroundIgnoreWaterSurface(
	    next.x, next.y + mCheckHeight, next.z, &mGroundPlane);
	mGroundHeight += 1.0f;

	if (next.y <= mGroundHeight + 0.05f) {
		mIsAirborne = false;
		JGeometry::TVec3<f32> normal;
		normal.set(mGroundPlane->getNormal());
		f32 correction = 1.0f
		                 - (normal.dot(next)
		                    - normal.dot(JGeometry::TVec3<f32>(
		                        next.x, mGroundHeight, next.z)));
		if (correction > 0.0f)
			next.scaleAdd(correction, next, normal);
		next.y = mGroundHeight;
	} else {
		mIsAirborne = true;
	}

	if (0.0f <= next.y + mCheckHeight)
		next.y = -mCheckHeight;

	TBGWallCheckRecord record(next.x, next.y, next.z, mCheckRadius, 1, 0);
	bool touchedWall = gpMap->isTouchedWallsAndMoveXZ(&record);
	next.x       = record.mCenter.x;
	next.z       = record.mCenter.z;

	JGeometry::TVec3<f32> move(next);
	move -= mPosition;
	mMove = move;
	mTouchedWall = touchedWall;
	mPosition = next;
}

void TTPHitActor::updateTerrainCollsion()
{
	f32 yOffset = 0.6666667f * mAttackHeight;
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

	JGeometry::TVec3<f32> offset(0.0f, -1.0f, 0.0f);
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

	TIdxGroupObj* group
	    = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
	group->getChildren().push_back(this);
}

void TTabePuku::init(TLiveManager* manager)
{
	mManager = manager;
	mManager->manageActor(this);
	setMActorAndKeeper();
	mSpine->initWith(&TNerveTabePukuGraphWander::theNerve());

	initHitActor(0x10000035, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	onHitFlag(HIT_FLAG_NO_COLLISION);

	mHitActor = new TTPHitActor(this, "タベプク用当たり");
	mHitActor->init();
	mHitActor->mPosition = mPosition;

	mQuat.set(SMS_Eular2Quat(mRotation));
	mMouthJointIndex
	    = (u16)getModel()->getModelData()->getJointName()->getIndex("jnt_mouth_up");
	initAnmSound();
}

void TTabePukuManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TTabePukuSaveLoadParams("/enemy/tabepuku.prm");
	TSmallEnemyManager::load(stream);
}

inline void TTabePuku::swimToCurPathNode(const JGeometry::TVec3<f32>& offset)
{
	JGeometry::TVec3<f32> goal = unk104.getPoint();
	goal.sub(mPosition);
	goal.add(offset);
	swimTo(goal);
}

DEFINE_NERVE(TNerveTabePukuGraphWander, TLiveActor)
{
	TTabePuku* self = (TTabePuku*)spine->getBody();

	if (spine->getTime() == 0) {
		self->getTracer()->mPrevIdx = -1;
		self->goToShortestNextGraphNode();
		self->setBckAnm(2);
		self->mMarchSpeed = self->getSaveParam2()->mMarchSpeed.get();
	}

	if (self->isReachedToGoal())
		self->goToRandomNextGraphNode();

	if (self->isFindMario(1.0f)) {
		spine->pushAfterCurrent(&TNerveTabePukuFound::theNerve());
		return TRUE;
	}

	self->swimToCurPathNode(JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f));
	return FALSE;
}

DEFINE_NERVE(TNerveTabePukuFound, TLiveActor)
{
	TTabePuku* self = (TTabePuku*)spine->getBody();

	if (spine->getTime() == 0) {
		self->setBckAnm(1);
		self->mMarchSpeed = 0.0f;
	}

	JGeometry::TVec3<f32> forward;
	self->mQuat.getZDir(forward);
	forward.scale(self->mMarchSpeed);

	JGeometry::TVec3<f32> velocity(self->mVelocity);
	velocity.scale(self->getSaveParam2()->mWaterFric.get());
	velocity.add(forward);
	self->mVelocity = velocity;

	self->mRotation.y = MsGetRotFromZaxisY(velocity);

	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveTabePukuAttack::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveTabePukuRecoverGraph, TLiveActor)
{
	TTabePuku* self = (TTabePuku*)spine->getBody();

	if (spine->getTime() == 0) {
		self->getTracer()->mPrevIdx = -1;
		self->getTracer()->mCurrIdx = -1;
		self->goToShortestNextGraphNode();
		self->mMarchSpeed = self->getSaveParam2()->mMarchSpeed.get();
	}

	if (self->isReachedToGoal()) {
		spine->pushAfterCurrent(&TNerveTabePukuGraphWander::theNerve());
		return TRUE;
	}

	JGeometry::TVec3<f32> offset;
	bool useRecoveryOffset = true;
	if (self->isAirborne() && !self->mTouchedWall)
		useRecoveryOffset = false;
	if (useRecoveryOffset)
		offset.set(0.0f, 10000.0f, 0.0f);
	else
		offset.set(0.0f, 0.0f, 0.0f);

	JGeometry::TVec3<f32> goal = getTabePukuGoalRef(self);
	goal.sub(self->mPosition);
	goal.add(offset);
	self->swimTo(goal);
	return FALSE;
}

inline bool TTabePuku::isMissMario() const
{
	if (fabsf(gpMarioPos->y - mPosition.y)
	    > getSaveParam2()->getSLGiveUpHeight())
		return true;
	f32 giveUpLength = getSaveParam2()->getSLGiveUpLength();
	if (vecdist(unk104.getPoint(), mPosition) > giveUpLength)
		return true;
	JGeometry::TVec3<f32> graphPos
	    = unk124->getGraph()->getNearestPosOnGraphLink(mPosition);
	graphPos.sub(mPosition);
	f32 territory = getSaveParam2()->mTerritoryRange.get();
	if (territory * territory <= graphPos.dot(graphPos))
		return true;
	return false;
}

DEFINE_NERVE(TNerveTabePukuAttack, TLiveActor)
{
	TTabePuku* self = (TTabePuku*)spine->getBody();

	if (spine->getTime() == 0) {
		self->setBckAnm(0);
		self->setGoalPath(TPathNode((THitActor*)gpMarioAddress));
		self->mMarchSpeed = self->getSaveParam2()->mAttackSpeed.get();
	}


	if (self->isMissMario() || self->mTouchedWall) {
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

inline void TTabePuku::prepareDive()
{
	mDiveStartY = mPosition.y;
	setBckAnm(2);
	getMActor()->getFrameCtrl(0)->setRate(2.0f * SMSGetAnmFrameRate());
	mMarchSpeed = getSaveParam2()->mDiveSpeed.get();
}

inline bool TTabePuku::doDive()
{
	swimTo(JGeometry::TVec3<f32>(0.0f, mGroundHeight - mPosition.y, 0.0f));
	if (mPosition.y - mDiveStartY < -getSaveParam2()->mApartHeight.get()
	    || mPosition.y - mGroundHeight < 200.0f || !isAirborne())
		return true;
	return false;
}

DEFINE_NERVE(TNerveTabePukuDive, TLiveActor)
{
	TTabePuku* self = (TTabePuku*)spine->getBody();

	if (spine->getTime() == 0)
		self->prepareDive();

	if (self->doDive()) {
		spine->pushAfterCurrent(&TNerveTabePukuDrag::theNerve());
		return TRUE;
	}

	return FALSE;
}

static inline bool checkDragRelease(TTabePuku* self)
{
	if (!self->mTouchedWall && self->isAirborne()) {
		JGeometry::TVec3<f32> base = getTabePukuGoalRef(self);
		base.sub(self->mPosition);
		f32 distance = base.length();
		if (!(self->getSaveParam2()->mDragLength.get() < distance))
			return false;
	}
	SMS_SendMessageToMario(self, HIT_MESSAGE_UNK8);
	self->mHeldObject = nullptr;
	return true;
}

DEFINE_NERVE(TNerveTabePukuDrag, TLiveActor)
{
	TTabePuku* self = (TTabePuku*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mDragDirection.set(0.0f, 0.0f, 1.0f);
		JGeometry::TQuat4<f32> rot;
		rot.setEulerY((rand() * (1.0f / 32768.0f)) * 6.2831855f);
		rot.rotate(self->mDragDirection);
		self->setGoalPath(TPathNode(self->mPosition));
		self->mMarchSpeed = self->getSaveParam2()->mDiveSpeed.get();
	}

	self->swimTo(self->mDragDirection);

	if (checkDragRelease(self)) {
		spine->pushAfterCurrent(&TNerveTabePukuRecoverGraph::theNerve());
		return TRUE;
	}

	return FALSE;
}
