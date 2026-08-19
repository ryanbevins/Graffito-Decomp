#include <Enemy/LimitKoopa.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/HitActor.hpp>
#include <Strategic/Strategy.hpp>
#include <System/Particles.hpp>
#include <System/EmitterViewObj.hpp>
#include <M3DUtil/MActor.hpp>
#include <Player/MarioAccess.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DNode.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JGadget/std-list.hpp>

// rogue includes needed for matching sinit & rodata
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

namespace {
int KoopaNeckCallBack(J3DNode*, int)
{
	return 1;
}
} // namespace

TLimitKoopaManager::TLimitKoopaManager(const char* name)
    : TEnemyManager(name)
{
}

TSpineEnemy* TLimitKoopaManager::createEnemyInstance() { return nullptr; }

void TLimitKoopaManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "koopa_model.bmd", 0x14240000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

inline TLimitKoopaParams::TLimitKoopaParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(rotationSpeed, 1.0f)
    , PARAM_INIT(bodyScale, 1.0f)
    , PARAM_INIT(hipDropInitialSpeedY, 1.0f)
    , PARAM_INIT(hipDropGravityY, 1.0f)
    , PARAM_INIT(turnSpeed, 1.6f)
    , PARAM_INIT(turnAnim, 3.7f)
    , PARAM_INIT(waitStep, 600.0f)
    , PARAM_INIT(attackRadius, 800.0f)
    , PARAM_INIT(attackHeight, 1000.0f)
    , PARAM_INIT(focusRange, 2.0f)
    , PARAM_INIT(waitRange, 12.0f)
    , PARAM_INIT(fireSpeed, 4.0f)
    , PARAM_INIT(tumbleSpeed, 2.0f)
    , PARAM_INIT(waitSpeed, 2.0f)
    , PARAM_INIT(staggerSpeed, 2.0f)
    , PARAM_INIT(downSpeed, 1.8f)
    , PARAM_INIT(tumbleWeight, 4.2f)
    , PARAM_INIT(flameScale, 1.0f)
    , PARAM_INIT(flameCount, 300)
    , PARAM_INIT(flameFocusStartStep, 100)
    , PARAM_INIT(flameFocusEndStep, 300)
    , PARAM_INIT(flameRadius, 200.0f)
    , PARAM_INIT(flameHeight, 600.0f)
    , PARAM_INIT(headRadius, 400.0f)
    , PARAM_INIT(waterhitSpeed, 2.0f)
    , PARAM_INIT(flameOverStart, 0.9f)
    , PARAM_INIT(flameNeckRange, 16.0f)
    , PARAM_INIT(flameNeckDownRate, 0.3f)
    , PARAM_INIT(marioEstimationFire, 20.0f)
    , PARAM_INIT(marioEstimationWait, 10.0f)
{
	TParams::load(mPrmPath);

	rotationSpeed.set(0.25f);
	bodyScale.set(0.7f);
	hipDropInitialSpeedY.set(40.0f);
	hipDropGravityY.set(0.4f);
}

void TLimitKoopaManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk38 = new TLimitKoopaParams("/enemy/limitkoopa.prm");
}

void TLimitKoopaManager::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	SMS_LoadParticle("/scene/koopa/jpa/ms_kp_fire_a.jpa", 0x1c0);
	SMS_LoadParticle("/scene/koopa/jpa/ms_kp_fire_b.jpa", 0x1c1);
	SMS_LoadParticle("/scene/koopa/jpa/ms_kp_fire_c.jpa", 0x1c2);
	SMS_LoadParticle("/scene/koopa/jpa/ms_kp_fire_d.jpa", 0x1c3);
	SMS_LoadParticle("/scene/koopa/jpa/ms_kp_fire_e.jpa", 0x1f3);
}

// ---------------------------------------------------------------------------
// TLimitKoopaParts hierarchy
// ---------------------------------------------------------------------------

inline TLimitKoopaParts::TLimitKoopaParts(const char* name)
    : TLiveActor(name)
{
}

inline TLimitKoopaBody::TLimitKoopaBody(const char* name)
    : TLimitKoopaParts(name)
{
}

inline TLimitKoopaHead::TLimitKoopaHead(const char* name)
    : TLimitKoopaParts(name)
{
}

inline TLimitKoopaHand::TLimitKoopaHand(const char* name)
    : TLimitKoopaParts(name)
{
}

inline TLimitKoopaFlame::TLimitKoopaFlame(const char* name)
    : TLimitKoopaParts(name)
{
}

void TLimitKoopaParts::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TLiveActor::perform(flags, graphics);

	if (flags & 1) {
		for (int i = 0; i < mColCount; i++)
			attack_(mCollisions[i]);
	}
}

void TLimitKoopaBody::attack_(THitActor* actor) { actor->receiveMessage(this, 0xE); }

BOOL TLimitKoopaBody::receiveMessage(THitActor* sender, u32 message) { return TRUE; }

void TLimitKoopaHead::attack_(THitActor* actor) { actor->receiveMessage(this, 0xE); }

BOOL TLimitKoopaHead::receiveMessage(THitActor* sender, u32 message)
{
	switch ((s32)message) {
	case 0xF: {
		TLimitKoopa* owner = (TLimitKoopa*)mOwner;
		if (owner->mSpine->getCurrentNerve()
		    == &TNerveLimitKoopaTumble::theNerve())
			break;

		if (owner->mSpine->getCurrentNerve()
		    == &TNerveLimitKoopaGetDown::theNerve())
			break;

		if (owner->mSpine->getCurrentNerve()
		    == &TNerveLimitKoopaStagger::theNerve())
			owner->mSpine->setNext(&TNerveLimitKoopaGetShowered::theNerve());

		owner->mSpine->pushNerve(&TNerveLimitKoopaGetShowered::theNerve());
	}
	}

	return TRUE;
}

void TLimitKoopaHand::attack_(THitActor* actor) { actor->receiveMessage(this, 0xE); }

BOOL TLimitKoopaHand::receiveMessage(THitActor* sender, u32 message) { return TRUE; }

void TLimitKoopaFlame::attack_(THitActor* actor)
{
	if (actor->receiveMessage(this, 0xA)) {
		TLimitKoopa* owner = (TLimitKoopa*)mOwner;
		TLimitKoopaParams* params
		    = (TLimitKoopaParams*)((TEnemyManager*)owner->getManager())
		          ->getSaveParam();
		f32 fireSpeed = params->fireSpeed.get();

		MActor* mactor = owner->getMActor();
		if (!mactor->checkCurBckFromIndex(3))
			mactor->setBckFromIndex(3);

		owner->getMActor()->getFrameCtrl(0)->setRate(fireSpeed);
	}
}

BOOL TLimitKoopaFlame::receiveMessage(THitActor* sender, u32 message)
{
	switch ((s32)message) {
	case 0xF:
		return FALSE;
	}

	return TRUE;
}

// ---------------------------------------------------------------------------
// TLimitKoopa
// ---------------------------------------------------------------------------

TLimitKoopa::TLimitKoopa(const char* name)
    : TSpineEnemy(name)
{
	mLiveFlag |= 0x80;
	mLiveFlag &= ~0x100;
	mLiveFlag |= 0x10;
	mScaledBodyRadius = 1600.0f;
}

void TLimitKoopa::load(JSUMemoryInputStream& stream) { TSpineEnemy::load(stream); }

f32 TLimitKoopa::getGravityY() const
{
	return getSaveParam2()->hipDropGravityY.get();
}

BOOL TLimitKoopa::receiveMessage(THitActor* sender, u32 message)
{
	return TSpineEnemy::receiveMessage(sender, message);
}

void TLimitKoopa::calcRootMatrix()
{
	f32 scale = getSaveParam2()->bodyScale.get();
	mScaling.x = scale;
	mScaling.y = scale;
	mScaling.z = scale;

	mRotation.y = TDirectionCalc::r2d(mDirection.mDirection);

	TSpineEnemy::calcRootMatrix();
}

void TLimitKoopa::bind()
{
	JGeometry::TVec3<f32> nextPos;
	nextPos = mPosition;
	nextPos.add(mLinearVelocity);
	nextPos.add(mVelocity);

	mVelocity.x += mFallVelocity.x;
	mVelocity.y += mFallVelocity.y;
	mVelocity.z += mFallVelocity.z;

	mGroundHeight = 3500.0f;
	mGroundHeight += 1.0f;

	if (nextPos.y <= 0.05f + mGroundHeight) {
		nextPos.y = mGroundHeight;
		unk168    = 1;
		mFallVelocity.x = 0.0f;
		mFallVelocity.y = 0.0f;
		mFallVelocity.z = 0.0f;
		mVelocity.x = 0.0f;
		mVelocity.y = 0.0f;
		mVelocity.z = 0.0f;
	}

	JGeometry::TVec3<f32> delta = nextPos - mPosition;
	mLinearVelocity = delta;
}

void TLimitKoopa::reset()
{
	TSpineEnemy::reset();

	f32 waitSpeed = getSaveParam2()->waitSpeed.get();
	MActor* actor = mMActor;
	if (!actor->checkCurBckFromIndex(0xc))
		actor->setBckFromIndex(0xc);
	mMActor->getFrameCtrl(0)->setRate(waitSpeed);

	mSpine->reset();
	unk150 = 0;
	unk154 = 0;
	unk158 = 0;
	unk168 = 1;
	mDirection.mDirection = 0.0f;
}

void TLimitKoopa::init(TLiveManager* manager)
{
	mBodyRadius = 800.0f;
	mHeadHeight = 2000.0f;

	TSpineEnemy::init(manager);

	onHitFlag(0x1);
	onHitFlag(0x4);
	offHitFlag(0x2);

	mSpine->initWith(&TNerveLimitKoopaWait::theNerve());

	if (mMActor->getAnmBck())
		mMActor->getAnmBck()->initSimpleMotionBlend(0x10);

	unk170 = 0.0f;

	reset();

	J3DModelData* modelData = getModel()->getModelData();
	JUTNameTab* nameTab     = modelData->getJointName();
	mHeadJointIndex         = nameTab->getIndex("ago");
	mNeckJointIndex         = nameTab->getIndex("head");
	mJointIndex2            = nameTab->getIndex("neck");

	J3DNode* node = (J3DNode*)getModel()->getModelData()->getJointNodePointer(
	    mNeckJointIndex);
	node->setCallBack(&KoopaNeckCallBack);
	node->setCallBackUserData(this);
}

void TLimitKoopa::loadAfter()
{
	JDrama::TNameRef::loadAfter();

	for (int i = 0; i < 10; i++) {
		TLimitKoopaFlame* p = new TLimitKoopaFlame(
		    "\x83\x4E\x83\x62\x83\x70\x82\xCC\x93\x66\x82\xAD\x89\x8A");
		p->mOwner = this;
		registerToGroup(p);
		p->initHitActor(0x08000030, 5, 0x80000000, 100.0f, 100.0f, 100.0f,
		                100.0f);
		p->onHitFlag(0x2);
		p->onHitFlag(0x4);
		p->onHitFlag(0x1);
		mFlameHitActors[i] = p;
	}

	for (int i = 0; i < 2; i++) {
		TLimitKoopaHand* p
		    = new TLimitKoopaHand("\x83\x4E\x83\x62\x83\x70\x8E\xE8");
		p->mOwner = this;
		registerToGroup(p);
		p->initHitActor(0x08000032, 5, 0x80000000, 100.0f, 100.0f, 100.0f,
		                100.0f);
		p->onHitFlag(0x2);
		p->onHitFlag(0x4);
		p->onHitFlag(0x1);
		(&unk1A0)[i] = p;
	}

	{
		TLimitKoopaHead* p
		    = new TLimitKoopaHead("\x83\x4E\x83\x62\x83\x70\x93\xAA");
		p->mOwner = this;
		registerToGroup(p);
		p->initHitActor(0x08000031, 5, 0x80000000, 100.0f, 100.0f, 100.0f,
		                100.0f);
		p->onHitFlag(0x2);
		p->onHitFlag(0x4);
		p->onHitFlag(0x1);
		mHeadHitActor = p;
	}

	{
		TLimitKoopaBody* p
		    = new TLimitKoopaBody("\x83\x4E\x83\x62\x83\x70\x91\xCC");
		p->mOwner = this;
		registerToGroup(p);
		p->initHitActor(0x08000033, 5, 0x80000000, 100.0f, 100.0f, 100.0f,
		                100.0f);
		p->onHitFlag(0x2);
		p->onHitFlag(0x4);
		p->onHitFlag(0x1);
		unk1AC = p;
	}
}

void TLimitKoopa::setUpHitActors()
{
	MtxPtr neckMtx = getMActor()->getModel()->getAnmMtx(mNeckJointIndex);

	bool emitting;
	if (getMActor()->getCurAnmIdx(0) == 4) {
		emitting = true;
	} else if (getMActor()->getCurAnmIdx(0) == 5
	           && getMActor()->getFrameCtrl(0)->getFrame() >= 127.0f) {
		emitting = true;
	} else {
		emitting = false;
	}

	if (emitting) {
		f32 ratio = 1.0f;
		if (getMActor()->getCurAnmIdx(0) == 5) {
			J3DFrameCtrl* fc = getMActor()->getFrameCtrl(0);
			ratio = (fc->getFrame() - 125.0f) / ((f32)fc->getEnd() - 125.0f);
		}

		for (int i = 0; i < 10; i++) {
			TLimitKoopaParams* prm = getSaveParam2();
			f32 flameRadius = prm->flameRadius.get();
			f32 flameHeight = prm->flameHeight.get();

			f32 dist = 0.8f * (2.0f + (f32)(2 * i)) * flameRadius * ratio;

			THitActor* hit = mFlameHitActors[i];
			JGeometry::TVec3<f32> flameOffset(dist, 0.0f, 0.0f);
			hit->mPosition.set<f32>(
			    neckMtx[0][3]
			        + (neckMtx[0][0] * flameOffset.x
			           + neckMtx[0][1] * flameOffset.y
			           + neckMtx[0][2] * flameOffset.z),
			    mPosition.y,
			    neckMtx[2][3]
			        + (neckMtx[2][0] * flameOffset.x
			           + neckMtx[2][1] * flameOffset.y
			           + neckMtx[2][2] * flameOffset.z));

			hit->offHitFlag(0x2);
			hit->offHitFlag(0x4);
			hit->offHitFlag(0x1);

			f32 height = flameHeight;
			if (height <= 0.0f)
				height = 2.0f * flameRadius;
			hit->mAttackRadius = flameRadius;
			hit->mAttackHeight = height;
			hit->mDamageRadius = flameRadius;
			hit->mDamageHeight = height;
			hit->calcEntryRadius();
		}
	} else {
		for (int i = 0; i < 10; i++) {
			THitActor* hit = mFlameHitActors[i];
			hit->onHitFlag(0x2);
			hit->onHitFlag(0x4);
			hit->onHitFlag(0x1);
		}
	}

	MtxPtr headMtx = getMActor()->getModel()->getAnmMtx(mHeadJointIndex);
	f32 headRadius = getSaveParam2()->headRadius.get();
	THitActor* head = mHeadHitActor;

	head->mPosition.set<f32>(headMtx[0][3], headMtx[1][3] - 200.0f,
	                         headMtx[2][3]);

	head->offHitFlag(0x2);
	head->offHitFlag(0x4);
	head->offHitFlag(0x1);

	head->mAttackRadius = headRadius;
	head->mAttackHeight = 2.0f * headRadius;
	head->mDamageRadius = headRadius;
	head->mDamageHeight = 2.0f * headRadius;
	head->calcEntryRadius();
}

#pragma dont_inline on
void TLimitKoopa::startHipDrop()
{
	JGeometry::TVec3<f32> vel(0.0f, 1.0f, 0.0f);
	vel.scale(getSaveParam2()->hipDropInitialSpeedY.get());

	JGeometry::TVec3<f32> marioVec = *gpMarioPos;
	marioVec.y                     = mGroundHeight;

	JGeometry::TVec3<f32> target = marioVec;
	target.sub(mPosition);
	target.add(mPosition);

	vel = calcVelocityToJumpToY(target, vel.y,
	                            getSaveParam2()->hipDropGravityY.get());

	if (vel.length() > 200.0f) {
		vel.normalize();
		vel.scale(200.0f);
	}

	mVelocity.set(vel);
}
#pragma dont_inline off

void TLimitKoopa::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSpineEnemy::perform(flags, graphics);

	for (int i = 0; i < 10; i++)
		mFlameHitActors[i]->perform(flags, graphics);
	mHeadHitActor->perform(flags, graphics);
	unk1A0->perform(flags, graphics);
	unk1A4->perform(flags, graphics);
	unk1AC->perform(flags, graphics);

	if (flags & 1) {
		setUpHitActors();
		if (unk150 > 0)
			unk150--;
		if (unk154 > 0)
			unk154--;
		if (unk158 > 0)
			unk158--;
	}

	if (flags & 2) {
		bool emitting;
		if (getMActor()->getCurAnmIdx(0) == 4) {
			emitting = true;
		} else if (getMActor()->getCurAnmIdx(0) == 5
		           && getMActor()->getFrameCtrl(0)->getFrame() >= 127.0f) {
			emitting = true;
		} else {
			emitting = false;
		}

		if (emitting) {
			f32 scale = getSaveParam2()->flameScale.get();
			JGeometry::TVec3<f32> scaleVec(scale, scale, scale);
			JPABaseEmitter* e;

			e = gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x1f3, getMActor()->getModel()->getAnmMtx(mNeckJointIndex), 3,
			    this);
			if (e) {
				e->unk154.set(scaleVec);
				e->unk174.set(scaleVec);
			}
			e = gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x1c3, getMActor()->getModel()->getAnmMtx(mNeckJointIndex), 1,
			    this);
			if (e) {
				e->unk154.set(scaleVec);
				e->unk174.set(scaleVec);
			}
			e = gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x1c2, getMActor()->getModel()->getAnmMtx(mNeckJointIndex), 1,
			    this);
			if (e) {
				e->unk154.set(scaleVec);
				e->unk174.set(scaleVec);
			}
			e = gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x1c1, getMActor()->getModel()->getAnmMtx(mNeckJointIndex), 1,
			    this);
			if (e) {
				e->unk154.set(scaleVec);
				e->unk174.set(scaleVec);
			}
			e = gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x1c0, getMActor()->getModel()->getAnmMtx(mNeckJointIndex), 1,
			    this);
			if (e) {
				e->unk154.set(scaleVec);
				e->unk174.set(scaleVec);
			}
		}
	}
}

inline void TLimitKoopa::registerToGroup(THitActor* part)
{
	JDrama::TNameRef* root
	    = JDrama::TNameRefGen::getInstance()->getRootNameRef();
	const char* kName = "\x93\x47\x83\x4F\x83\x8B\x81\x5B\x83\x76";
	TIdxGroupObj* group = (TIdxGroupObj*)root->searchF(
	    JDrama::TNameRef::calcKeyCode(kName), kName);
	group->getChildren().push_back(part);
}

// ---------------------------------------------------------------------------
// Nerves
// ---------------------------------------------------------------------------

DEFINE_NERVE(TNerveLimitKoopaWait, TLiveActor)
{
	TLimitKoopa* self = (TLimitKoopa*)spine->getBody();

	if (spine->getTime() == 0) {
		MActor* m = self->mMActor;
		if (!m->checkCurBckFromIndex(10))
			m->setBckFromIndex(10);
		self->mMActor->getFrameCtrl(0)->setRate(2.0f);
		self->unk150 = 240;
	}

	{
		TDirectionCalc tdc;
		JGeometry::TVec3<f32> diff;
		diff.x = gpMarioPos->x - self->mPosition.x;
		diff.y = gpMarioPos->y - self->mPosition.y;
		diff.z = gpMarioPos->z - self->mPosition.z;
		diff.y = 0.0f;
		tdc.makeDirection(diff);

		f32 turn = TDirectionCalc::d2r(
		    self->getSaveParam2()->rotationSpeed.get());
		self->mDirection.mDirection
		    = self->mDirection.calcTurnDirection(tdc.mDirection, turn);
	}

	f32 absDir;
	{
		TDirectionCalc tdc;
		JGeometry::TVec3<f32> diff;
		diff.x = gpMarioPos->x - self->mPosition.x;
		diff.y = gpMarioPos->y - self->mPosition.y;
		diff.z = gpMarioPos->z - self->mPosition.z;
		diff.y = 0.0f;
		tdc.makeDirection(diff);
		absDir = self->mDirection.absDirection(tdc.mDirection);
	}

	bool turned;
	if (absDir > 0.31415927f)
		turned = false;
	else
		turned = true;

	if (turned && self->mMActor->isCurAnmAlreadyEnd(0)) {
		spine->pushAfterCurrent(&TNerveLimitKoopaHipDropStart::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveLimitKoopaStagger, TLiveActor)
{
	TLimitKoopa* self = (TLimitKoopa*)spine->getBody();

	f32 rate  = self->getSaveParam2()->staggerSpeed.get();
	MActor* m = self->mMActor;
	if (!m->checkCurBckFromIndex(9))
		m->setBckFromIndex(9);
	self->mMActor->getFrameCtrl(0)->setRate(rate);

	if (self->mMActor->curAnmEndsNext(0, nullptr))
		return TRUE;
	return FALSE;
}

DEFINE_NERVE(TNerveLimitKoopaGetShowered, TLiveActor)
{
	TLimitKoopa* self = (TLimitKoopa*)spine->getBody();

	f32 rate  = self->getSaveParam2()->waterhitSpeed.get();
	MActor* m = self->mMActor;
	if (!m->checkCurBckFromIndex(14))
		m->setBckFromIndex(14);
	self->mMActor->getFrameCtrl(0)->setRate(rate);

	if (self->mMActor->curAnmEndsNext(0, nullptr))
		return TRUE;
	return FALSE;
}

DEFINE_NERVE(TNerveLimitKoopaGetDown, TLiveActor)
{
	TLimitKoopa* self = (TLimitKoopa*)spine->getBody();

	switch (self->mMActor->getCurAnmIdx(0)) {
	case 0:
		if (self->mMActor->curAnmEndsNext(0, nullptr)) {
			f32 rate  = self->getSaveParam2()->downSpeed.get();
			MActor* m = self->mMActor;
			if (!m->checkCurBckFromIndex(1))
				m->setBckFromIndex(1);
			self->mMActor->getFrameCtrl(0)->setRate(rate);
		}
		break;
	case 1:
		if (self->mMActor->curAnmEndsNext(0, nullptr)) {
			f32 rate  = self->getSaveParam2()->downSpeed.get();
			MActor* m = self->mMActor;
			if (!m->checkCurBckFromIndex(7))
				m->setBckFromIndex(7);
			self->mMActor->getFrameCtrl(0)->setRate(rate);
		}
		break;
	case 7:
		if (self->mMActor->curAnmEndsNext(0, nullptr))
			return TRUE;
		break;
	default: {
		f32 rate  = self->getSaveParam2()->downSpeed.get();
		MActor* m = self->mMActor;
		if (!m->checkCurBckFromIndex(0))
			m->setBckFromIndex(0);
		self->mMActor->getFrameCtrl(0)->setRate(rate);
		break;
	}
	}
	return FALSE;
}

DEFINE_NERVE(TNerveLimitKoopaTumble, TLiveActor)
{
	TLimitKoopa* self = (TLimitKoopa*)spine->getBody();

	f32 speed = self->getSaveParam2()->tumbleSpeed.get();
	MActor* m = self->mMActor;
	if (!m->checkCurBckFromIndex(8))
		m->setBckFromIndex(8);
	self->mMActor->getFrameCtrl(0)->setRate(speed);

	if (self->mMActor->curAnmEndsNext(0, nullptr))
		return TRUE;
	return FALSE;
}

DEFINE_NERVE(TNerveLimitKoopaHipDropStart, TLiveActor)
{
	TLimitKoopa* self = (TLimitKoopa*)spine->getBody();

	if (spine->getTime() == 0) {
		MActor* m = self->mMActor;
		if (!m->checkCurBckFromIndex(5))
			m->setBckFromIndex(5);
		self->mMActor->getFrameCtrl(0)->setRate(2.0f);
		self->unk154 = 30;
	}

	if (self->unk154 <= 0) {
		self->startHipDrop();
		self->unk168 = 0;
		spine->pushAfterCurrent(&TNerveLimitKoopaHipDropJump::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveLimitKoopaHipDropJump, TLiveActor)
{
	TLimitKoopa* self = (TLimitKoopa*)spine->getBody();

	f32 g                = self->getGravityY();
	self->mFallVelocity.x = 0.0f;
	self->mFallVelocity.y = -g;
	self->mFallVelocity.z = 0.0f;

	BOOL result;
	if (self->unk168 != 0) {
		spine->pushAfterCurrent(&TNerveLimitKoopaWait::theNerve());
		result = TRUE;
	} else {
		result = FALSE;
	}
	return result;
}
