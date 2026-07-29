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

namespace {
f32 cAngleMax = JGeometry::TUtil<f32>::PI() / 8.0f;
}

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
		JGeometry::TVec3<f32> base = getTabePukuGoalRef(self);
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
	    < -self->getSaveParam2()->mApartHeight.get()) {
		keepDiving = true;
	}
	if (self->mPosition.y - self->mGroundHeight < 200.0f || !self->isAirborne())
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
	if (!self->isAirborne() || self->mTouchedWall)
		offset.set(0.0f, 10000.0f, 0.0f);
	else
		offset.set(0.0f, 0.0f, 0.0f);

	JGeometry::TVec3<f32> goal = getTabePukuGoalRef(self);
	goal.sub(self->mPosition);
	goal.add(offset);
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

	JGeometry::TVec3<f32> goal = getTabePukuGoalRef(self);
	goal.sub(self->mPosition);
	self->swimTo(goal);
	return FALSE;
}

void TTabePukuManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
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

void TTabePuku::swimTo(const JGeometry::TVec3<f32>& target)
{
	JGeometry::TVec3<f32> dir(target);
	if (!dir.isZero()) {
		dir.normalize();

		JGeometry::TQuat4<f32> targetQuat;
		targetQuat.setRotate(JGeometry::TVec3<f32>(0.0f, 0.0f, 1.0f), dir);
		mQuat.slerp(targetQuat, getSaveParam2()->mTurnSlerpRate.get());
		mQuat.normalize();
	}

	JGeometry::TVec3<f32> forward;
	mQuat.getZDir(forward);

	JGeometry::TVec3<f32> velocity(mVelocity);
	velocity.scale(getSaveParam2()->mWaterFric.get());
	velocity.x += forward.x * mMarchSpeed;
	velocity.y += forward.y * mMarchSpeed;
	velocity.z += forward.z * mMarchSpeed;
	mVelocity = velocity;

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
	mRotation.y = rotation;
}

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
	f32* row1 = mTakingMtx[1];
	f32* row2 = mTakingMtx[2];

	mTakingMtx[0][0] = 1.0f - 2.0f * y * y - 2.0f * z * z;
	mTakingMtx[0][1] = 2.0f * x * y - 2.0f * w * z;
	mTakingMtx[0][2] = 2.0f * x * z + 2.0f * w * y;
	row1[0] = 2.0f * x * y + 2.0f * w * z;
	row1[1] = 1.0f - 2.0f * x * x - 2.0f * z * z;
	row1[2] = 2.0f * y * z - 2.0f * w * x;
	row2[0] = 2.0f * x * z - 2.0f * w * y;
	row2[1] = 2.0f * y * z + 2.0f * w * x;
	row2[2] = 1.0f - 2.0f * x * x - 2.0f * y * y;

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
	PSMTXCopy(mtx, getModel()->getBaseTRMtx());

	JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
	JPABaseEmitter* emitter = SMS_EasyEmitParticle(
	    (E_SMS_EFFECT_LOOP_NORMAL)0x178, getModel()->getAnmMtx(mMouthJointIndex),
	    this, scale);
	if (emitter) {
		f32 lifeScale = -mPosition.y / 100.0f;
		if (lifeScale <= 0.0f)
			lifeScale = 0.0f;

		s32 life = (s32)lifeScale * 20 + 2;
		if (life > 200)
			life = 200;
		emitter->mBaseLifetime = life;

		if (mSpine->getLatestNerve() == &TNerveTabePukuAttack::theNerve())
			emitter->mChildSpawnRate = 0.1f;
	}
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

	hitActor->updateTerrainCollsion();
	hitActor->bind();

	mLinearVelocity = hitActor->mMove;
	mTouchedWall    = hitActor->mTouchedWall;
	if (hitActor->mIsAirborne)
		onLiveFlag(LIVE_FLAG_AIRBORNE);
	else
		offLiveFlag(LIVE_FLAG_AIRBORNE);
	mGroundPlane  = hitActor->mGroundPlane;
	mGroundHeight = hitActor->mGroundHeight;
}

void TTabePuku::control()
{
	TLiveActor::control();

	TTPHitActor* hitActor = mHitActor;
	THitActor** it        = hitActor->mCollisions;
	THitActor** end       = it + hitActor->mColCount;
	s32 targetType        = 0x80000000;
	targetType += 1;
	for (; it != end; ++it) {
		THitActor* hit = *it;
		if ((s32)hit->mActorType != targetType)
			continue;
		hitActor->mOwner->attackToMario();
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
	    = (u16)getModel()->getModelData()->getJointName()->getIndex("jnt_mouth_up");
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
		const JGeometry::TVec3<f32>& normal = mGroundPlane->getNormal();
		f32 nextDot
		    = normal.x * next.x + normal.y * next.y + normal.z * next.z;
		f32 groundDot = normal.x * next.x + normal.y * mGroundHeight
		                + normal.z * next.z;
		f32 correction = 1.0f - (nextDot - groundDot);
		if (correction > 0.0f)
			next.scaleAdd(correction, next, normal);
		next.y = mGroundHeight;
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
	f32 yOffset = 0.6666667f * mAttackHeight;
	JGeometry::TVec3<f32> up;
	JGeometry::TQuat4<f32> quat(mOwner->mQuat);
	quat.getYDir(up);
	up.x = -up.x;

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
