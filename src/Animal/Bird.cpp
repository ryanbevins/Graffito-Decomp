#include <Animal/Bird.hpp>
#include <Animal/AnimalSave.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSound.hpp>
#include <Player/MarioAccess.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <Enemy/WireBinder.hpp>
#include <Enemy/PathNode.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <M3DUtil/MActor.hpp>
#include <Map/Map.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/Item.hpp>
#include <System/Particles.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <stdlib.h>

extern JGeometry::TVec3<f32>* gpMarioPos;

void SMS_Eular2Quat(const JGeometry::TVec3<f32>&, JGeometry::TQuat4<f32>*);
f32 SMSGetAnmFrameRate();

namespace {
const int cRandomAnims[5] = { 7, 4, 0, 2, 8 };

struct ColorEntry {
	GXColorS10 c1;
	GXColorS10 c2;
};

const ColorEntry cColorTable[4] = {
	{ { 0, 0, 0, 0x64 }, { 0xFF, 0, 0, 0 } },
	{ { 0, 0, 0, 0xC8 }, { 0, 0, 0, 0 } },
	{ { 0, 0xFF, 0, 0xC8 }, { 0, 0, 0, 0 } },
	{ { 0, 0xFF, 0, 0 }, { 0, 0, 0, 0 } },
};

const char* cMatName = "_mat_body1";
}

// ---- TAnimalBirdParams ----

TAnimalBirdParams::TAnimalBirdParams(const char* prm)
    : TSpineEnemyParams(prm)
    , PARAM_INIT(mMarchSpeed, 5.0f)
    , PARAM_INIT(mTurnSpeed, 0.1f)
    , PARAM_INIT(mReturnTimer, 1800)
    , PARAM_INIT(mSearchLength, 800.0f)
    , PARAM_INIT(mSearchHeight, 600.0f)
    , PARAM_INIT(mSearchAware, 400.0f)
    , PARAM_INIT(mSearchAngle, 90.0f)
    , PARAM_INIT(mActionTimer, 100)
    , PARAM_INIT(mWaterproofTimerMax, 45)
    , PARAM_INIT(mFloatingTimerMax, 30)
    , PARAM_INIT(mLandingGravityY, 1.0f)
    , PARAM_INIT(mLandingTorqueY, 2.0f)
    , PARAM_INIT(mWalkingTorqueY, 2.0f)
    , PARAM_INIT(mWalkingSpeed, 0.5f)
    , PARAM_INIT(mWalkTimer, 100)
    , PARAM_INIT(mLandingFric, 0.95f)
    , PARAM_INIT(mActionTimerAdd, 300)
    , PARAM_INIT(mWaterPowerY, 15.0f)
{
	load(mPrmPath);
}

// ---- TAnimalBirdManager ----

TAnimalBirdManager::TAnimalBirdManager(const char* name)
    : TEnemyManager(name)
{
}

void TAnimalBirdManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "bird_man.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TAnimalBirdManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TAnimalBirdParams("/Animal/bird.prm");
	TEnemyManager::load(stream);
}

void TAnimalBirdManager::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	MSoundSESystem::MSRandPlay::createRandPlayVec(0x3869, (u16)mObjNum);
	MSoundSESystem::MSRandPlay::createRandPlayVec(0x3870, (u16)mObjNum);
}

// ---- TAnimalBird ----

TAnimalBird::TAnimalBird(const char* name)
    : TSpineEnemy(name)
{
	unk150   = (TMapObjBase*)NULL;
	mBinder2 = (TBinder*)NULL;
}

void TAnimalBird::load(JSUMemoryInputStream& stream)
{
	TSpineEnemy::load(stream);

	s32 itemId;
	stream.read(&itemId, 4);

	TMapObjBase* item;
	if (itemId >= 0) {
		item = TMapObjBaseManager::newAndRegisterObjByEventID((u32)itemId, "鳥");
	} else {
		item = TMapObjBaseManager::newAndRegisterObjByEventID(0x64, "");
	}
	unk150 = item;

	u32 actorType = item->mActorType;
	int variant;
	if (actorType == 0x20000010) {
		variant   = 0;
		bool flag = TFlagManager::smInstance->getBlueCoinFlag(
		    gpMarDirector->mMap, (u8)itemId);
		if (flag) {
			mLiveFlag |= 1;
		}
	} else if (actorType == 0x20000013) {
		variant = 2;
	} else if (actorType >= 0x2000000F) {
		variant = 3;
	} else {
		variant = 1;
	}
	*(int*)((char*)this + 0x180) = variant;

	J3DModel* model = getModel();
	u16 matIdx
	    = (u16)model->getModelData()->getMaterialName()->getIndex(cMatName);
	SMS_InitPacket_OneTevColor(model, matIdx, GX_TEVREG2,
	                           (const GXColorS10*)&cColorTable[variant]);
}

void TAnimalBird::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	MSoundSESystem::MSRandPlay::registerTrans(0x3869, &mPosition);
	MSoundSESystem::MSRandPlay::registerTrans(0x3870, &mPosition);
}

BOOL TAnimalBird::receiveMessage(THitActor* sender, u32 msg)
{
	if (mLiveFlag & 1)
		return FALSE;

	if (msg == 0xF) {
		JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
		SMS_EasyEmitParticle((E_SMS_EFFECT_ONETIME_NORMAL)0xE7,
		                     &sender->mPosition, (const void*)NULL, scale);
		gpMSound->startSoundSet(0x6802, &sender->mPosition, 0, 0.0f, 0, 0, 4);

		if (unk178 <= 0) {
			TAnimalBirdParams* p = (TAnimalBirdParams*)getSaveParam();
			unk178               = p->mWaterproofTimerMax.value;
			if (mLiveFlag & 0x80 && mHitPoints > 0) {
				mHitPoints--;
			}
		}
		return TRUE;
	}

	if (msg == 4) {
		if (*(THitActor**)((char*)this + 0x68) == NULL) {
			unk64 |= 1;
			*(THitActor**)((char*)this + 0x68) = sender;
			JGeometry::TVec3<f32> scale2(1.0f, 1.0f, 1.0f);
			SMS_EasyEmitParticle((E_SMS_EFFECT_ONETIME_NORMAL)0xE7,
			                     &sender->mPosition, (const void*)NULL, scale2);
		}
		return TRUE;
	}

	if (msg == 6 || msg == 7) {
		if (*(THitActor**)((char*)this + 0x68) == sender) {
			*(THitActor**)((char*)this + 0x68) = (THitActor*)NULL;
			unk64 &= ~1;
		}
		return TRUE;
	}

	if (msg == 0xB) {
		*(THitActor**)((char*)this + 0x68) = (THitActor*)NULL;
		if (mSpine->getLatestNerve()
		    != &TNerveAnimalBirdChangeToCoin::theNerve()) {
			mSpine->setNext(&TNerveAnimalBirdChangeToCoin::theNerve());
		}
		return TRUE;
	}

	if (msg == 0) {
		u32 t = sender->mActorType - 0x10000000;
		if (t == 0xD) {
			if (mSpine->getLatestNerve()
			    != &TNerveAnimalBirdChangeToCoin::theNerve()) {
				mSpine->setNext(&TNerveAnimalBirdChangeToCoin::theNerve());
			} else {
				receiveMessage(this, 0xF);
			}
			return TRUE;
		}
	}

	return TSpineEnemy::receiveMessage(sender, msg);
}

void TAnimalBird::init(TLiveManager* mgr)
{
	mManager = mgr;
	mgr->manageActor(this);

	mMActorKeeper = new TMActorKeeper(mgr, (u16)1);
	mMActor       = mMActorKeeper->createMActor("bird_man.bmd", 0);

	mSpine->initWith(&TNerveAnimalBirdWaitOnGround::theNerve());

	initParams();
	initHitActor(0x10000032, 0, 0, 50.0f, 50.0f, 70.0f, 80.0f);

	onHitFlag(2);
	offHitFlag(1);

	mScaledBodyRadius = 35.0f;

	initAnmSound();
}

class TBirdMount {
public:
	virtual MtxPtr getRiderMtx() = 0;
};

void TAnimalBird::calcRootMatrix()
{
	TBirdMount* mount = *(TBirdMount**)((char*)this + 0x68);
	if (mount != NULL) {
		typedef MtxPtr (*MtxFn)(TBirdMount*);
		MtxFn fn  = *(MtxFn*)(*(char**)mount + 0xA4);
		MtxPtr m  = fn(mount);
		PSMTXCopy(m, getModel()->unk20);
	} else {
		TSpineEnemy::calcRootMatrix();
	}
	getModel()->unk20[1][3] += 35.0f;
}

void TAnimalBird::bind()
{
	BOOL useWire = FALSE;
	if (mBinder2 != NULL) {
		BOOL cond1                 = TRUE;
		BOOL cond2                 = TRUE;
		BOOL cond3                 = TRUE;
		TSpineBase<TLiveActor>* sp = mSpine;
		if (sp->getLatestNerve() != &TNerveAnimalBirdWaitOnGround::theNerve()) {
			if (sp->getLatestNerve()
			    != &TNerveAnimalBirdActionOnGround::theNerve())
				cond3 = FALSE;
		}
		if (!cond3) {
			if (sp->getLatestNerve()
			    != &TNerveAnimalBirdWalkOnGround::theNerve())
				cond2 = FALSE;
		}
		if (!cond2) {
			if (mSpine->getLatestNerve()
			    != &TNerveAnimalBirdPreLanding::theNerve())
				cond1 = FALSE;
		}
		if (cond1)
			useWire = TRUE;
	}

	if (!useWire) {
		TLiveActor::bind();
	} else {
		((TWireBinder*)mBinder2)->bind(this);
	}
}

void TAnimalBird::moveObject()
{
	if (unk178 > 0)
		unk178--;

	const TNerveBase<TLiveActor>* cur = mSpine->getLatestNerve();

	bool inGroundState = (cur == &TNerveAnimalBirdWaitOnGround::theNerve()
	                      || cur == &TNerveAnimalBirdActionOnGround::theNerve()
	                      || cur == &TNerveAnimalBirdWalkOnGround::theNerve());

	if (inGroundState && (unk64 & 1)) {
		TAnimalBirdParams* p = (TAnimalBirdParams*)getSaveParam();
		unk17C++;
		if (p->mFloatingTimerMax.value < unk17C + 1) {
			mSpine->setNext(&TNerveAnimalBirdTakeoff::theNerve());
		}
	} else if (inGroundState) {
		unk17C = 0;
	}

	const TNerveBase<TLiveActor>* cur2 = mSpine->getLatestNerve();
	if (cur2 == &TNerveAnimalBirdGraphWander::theNerve()
	    || cur2 == &TNerveAnimalBirdComeback::theNerve()) {
		if (gpMSound->gateCheck(0x3869)) {
			MSoundSESystem::MSRandPlay::startSeRandPlay(
			    0x3869, (u32)(s16)mInstanceIndex);
		}
	}

	const TNerveBase<TLiveActor>* cur3 = mSpine->getLatestNerve();
	if (cur3 == &TNerveAnimalBirdWaitOnGround::theNerve()
	    || cur3 == &TNerveAnimalBirdActionOnGround::theNerve()
	    || cur3 == &TNerveAnimalBirdWalkOnGround::theNerve()) {
		if (gpMSound->gateCheck(0x3870)) {
			MSoundSESystem::MSRandPlay::startSeRandPlay(
			    0x3870, (u32)(s16)mInstanceIndex);
		}
	}

	TLiveActor::moveObject();
}

const char** TAnimalBird::getBasNameTable() const
{
	static const char* bastable[] = {
		"/scene/bird/bas/bird_fly.bas",
		"/scene/bird/bas/bird_open.bas",
		"/scene/bird/bas/bird_start.bas",
		"/scene/bird/bas/bird_stop.bas",
		nullptr,
	};
	return bastable;
}

void TAnimalBird::initParams()
{
	unk158 = mPosition;
	unk158.y += 90.0f;
	unk164 = mRotation;

	TSpineEnemyParams* p = getSaveParam();
	u8 v;
	if (p != NULL) {
		v = (u8)getSaveParam()->mSLHitPointMax.value;
	} else {
		v = 1;
	}
	mHitPoints = v;

	unk178 = 0;
	unk17C = 0;
	unk170 = 1.0f;

	mLiveFlag &= ~0x180;

	unk174 = 1.0f - 0.1f * (MsRandF() - 0.5f);

	if (TWireBinder::isOnWire(mPosition)) {
		TWireBinder* wb = new TWireBinder();
		mBinder2        = wb;
		wb->init(mPosition);
	}
}

BOOL TAnimalBird::isFindMario() const
{
	f32 diffY = mPosition.y - gpMarioPos->y;
	if (diffY < 0)
		diffY = -diffY;
	TAnimalBirdParams* p = (TAnimalBirdParams*)getSaveParam();
	if (p->mSearchHeight.value < diffY)
		return FALSE;

	f32 scale = unk174;
	return isInSight(*gpMarioPos, scale * p->mSearchLength.value,
	                 scale * p->mSearchAngle.value,
	                 scale * p->mSearchAware.value)
	    ? 1
	    : 0;
}

void TAnimalBird::doFlyToCurPathNode()
{
	TAnimalBirdParams* p         = (TAnimalBirdParams*)getSaveParam();
	JGeometry::TVec3<f32> target = unkF4.getPoint();
	JGeometry::TVec3<f32> toTarget;
	toTarget.x = target.x - mPosition.x;
	toTarget.y = target.y - mPosition.y;
	toTarget.z = target.z - mPosition.z;

	f32 dist2 = toTarget.x * toTarget.x + toTarget.y * toTarget.y
	          + toTarget.z * toTarget.z;
	f32 dist = JGeometry::TUtil<f32>::sqrt(dist2);

	if (dist > 0.0f) {
		f32 speed  = p->mMarchSpeed.value * unk174;
		toTarget.x = toTarget.x / dist * speed;
		toTarget.y = toTarget.y / dist * speed;
		toTarget.z = toTarget.z / dist * speed;
	}

	mLinearVelocity = toTarget;
}

void TAnimalBird::doLanding(bool initFrame)
{
	TAnimalBirdParams* p = (TAnimalBirdParams*)getSaveParam();
	if (initFrame) {
		JGeometry::TQuat4<f32> q;
		SMS_Eular2Quat(mRotation, &q);
		mVelocity.x = q.x;
		mVelocity.y = q.y;
		mVelocity.z = q.z;
	}

	mRotation.x = unk164.x;
	mRotation.z = unk164.z;

	f32 torque  = p->mLandingTorqueY.value * unk174 * SMSGetAnmFrameRate();
	f32 deltaY  = unk164.y - mRotation.y;
	f32 wrapped = MsWrap<f32>(deltaY, -180.0f, 180.0f);
	if (wrapped < -torque)
		wrapped = -torque;
	else if (wrapped > torque)
		wrapped = torque;

	mRotation.y = MsWrap<f32>(mRotation.y + wrapped, 0.0f, 360.0f);
}

// ---- Nerves ----

DEFINE_NERVE(TNerveAnimalBirdWaitOnGround, TLiveActor)
{
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();

	if (spine->getTime() == 0) {
		bird->mMActor->setBckFromIndex(7);
		bird->setCurAnmSound();
	}

	bool wantTakeoff = (bird->unk178 > 0) || bird->isFindMario();
	if (wantTakeoff && MsRandF() < 0.5f) {
		spine->pushAfterCurrent(&TNerveAnimalBirdTakeoff::theNerve());
		return TRUE;
	}

	if (!bird->checkCurAnmEnd(0))
		return FALSE;

	TAnimalBirdParams* p = (TAnimalBirdParams*)bird->getSaveParam();
	s32 diff             = spine->getTime() - p->mActionTimer.value;
	if (diff < 0)
		return FALSE;

	f32 ratio = (f32)diff / (f32)p->mActionTimerAdd.value;
	if (MsRandF() < ratio) {
		spine->pushAfterCurrent(&TNerveAnimalBirdActionOnGround::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveAnimalBirdActionOnGround, TLiveActor)
{
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();

	if (spine->getTime() == 0) {
		int randIdx = (int)(MsRandF() * 5.0f);
		if (randIdx < 0)
			randIdx = 0;
		if (randIdx > 4)
			randIdx = 4;
		int animIdx = cRandomAnims[randIdx];
		if (animIdx == 8) {
			spine->pushAfterCurrent(
			    &TNerveAnimalBirdWalkOnGround::theNerve());
			return TRUE;
		}
		if (!bird->mMActor->checkCurBckFromIndex(animIdx)) {
			bird->mMActor->setBckFromIndex(animIdx);
		}
	}

	bool wantTakeoff = (bird->unk178 > 0) || bird->isFindMario();
	if (wantTakeoff && MsRandF() < 0.5f) {
		spine->pushAfterCurrent(&TNerveAnimalBirdTakeoff::theNerve());
		return TRUE;
	}

	if (bird->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveAnimalBirdWaitOnGround::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveAnimalBirdWalkOnGround, TLiveActor)
{
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();

	if (spine->getTime() == 0) {
		bird->unk170 = -bird->unk170;
		bird->mMActor->setBckFromIndex(8);
		bird->setCurAnmSound();
	}

	bool wantTakeoff = (bird->unk178 > 0) || bird->isFindMario();
	if (wantTakeoff && MsRandF() < 0.5f) {
		spine->pushAfterCurrent(&TNerveAnimalBirdTakeoff::theNerve());
		return TRUE;
	}

	TAnimalBirdParams* p    = (TAnimalBirdParams*)bird->getSaveParam();
	bird->mLinearVelocity.y = 0.15f;
	f32 turnRate
	    = bird->unk170 * p->mWalkingTorqueY.value * SMSGetAnmFrameRate();
	bird->mRotation.y = MsWrap<f32>(bird->mRotation.y + turnRate, 0.0f, 360.0f);

	if (spine->getTime() >= p->mWalkTimer.value) {
		spine->pushAfterCurrent(&TNerveAnimalBirdWaitOnGround::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveAnimalBirdTakeoff, TLiveActor)
{
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();

	if (spine->getTime() == 0) {
		bird->mMActor->setBckFromIndex(5);
		bird->setCurAnmSound();
		bird->mLiveFlag |= 0x80;
		bird->unk17C     = 0;
		J3DFrameCtrl* fc = bird->mMActor->getFrameCtrl(0);
		fc->setRate(fc->getRate() * 3.0f);
		if (gpMSound->gateCheck(0x386b)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x386b, (const Vec*)&bird->mPosition, 0,
			    (JAISound**)NULL, 0, 4);
		}
	}

	if (bird->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveAnimalBirdGraphWander::theNerve());
		bird->mLiveFlag |= 0x80;
		bird->unk17C = 0;
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveAnimalBirdGraphWander, TLiveActor)
{
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();

	if (spine->getTime() == 0) {
		bird->mLinearVelocity.x = 0.0f;
		bird->mLinearVelocity.y = 0.0f;
		bird->mLinearVelocity.z = 0.0f;
		bird->unkF4.unk0        = (THitActor*)NULL;
		bird->goToShortestNextGraphNode();
	}

	bird->doFlyToCurPathNode();
	return FALSE;
}

DEFINE_NERVE(TNerveAnimalBirdComeback, TLiveActor)
{
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();

	if (spine->getTime() == 0) {
		bird->unkF4.unk0  = (THitActor*)NULL;
		bird->unkF4.unk4  = bird->unk158;
		bird->unk104.unk0 = (THitActor*)NULL;
		bird->unk104.unk4 = bird->unk158;
		bird->unk114.clear();
		bird->mMActor->setBckFromIndex(3);
		bird->setCurAnmSound();
	}

	bird->doFlyToCurPathNode();

	if (bird->isFindMario()) {
		spine->pushAfterCurrent(&TNerveAnimalBirdGraphWander::theNerve());
		return TRUE;
	}

	JGeometry::TVec3<f32> diff;
	diff.x = bird->unk158.x - bird->mPosition.x;
	diff.y = bird->unk158.y - bird->mPosition.y;
	diff.z = bird->unk158.z - bird->mPosition.z;
	f32 d2 = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
	TAnimalBirdParams* p = (TAnimalBirdParams*)bird->getSaveParam();
	if (d2 < p->mMarchSpeed.value * p->mMarchSpeed.value) {
		spine->pushAfterCurrent(&TNerveAnimalBirdPreLanding::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveAnimalBirdPreLanding, TLiveActor)
{
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();

	if (spine->getTime() == 0) {
		bird->mMActor->setBckFromIndex(1);
		bird->setCurAnmSound();
		J3DFrameCtrl* fc = bird->mMActor->getFrameCtrl(0);
		fc->setRate(fc->getRate() * 1.5f);
		bird->doLanding(true);
	}

	if (bird->isFindMario()) {
		spine->pushAfterCurrent(&TNerveAnimalBirdGraphWander::theNerve());
		return TRUE;
	}

	bird->doLanding(false);
	if (bird->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveAnimalBirdLanding::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveAnimalBirdLanding, TLiveActor)
{
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();
	J3DFrameCtrl* fc  = bird->mMActor->getFrameCtrl(0);

	if (spine->getTime() == 0) {
		bird->mLinearVelocity.x = 0.0f;
		bird->mLinearVelocity.y = 0.0f;
		bird->mLinearVelocity.z = 0.0f;
		bird->mMActor->setBckFromIndex(5);
		bird->setCurAnmSound();
		fc->setAttribute(1);
		fc->setFrame((f32)fc->getEnd());
		fc->setRate(-fc->getRate());
	}

	if (bird->isFindMario()) {
		spine->pushAfterCurrent(&TNerveAnimalBirdGraphWander::theNerve());
		return TRUE;
	}

	if (fc->getAttribute() & 1) {
		spine->pushAfterCurrent(&TNerveAnimalBirdWaitOnGround::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveAnimalBirdChangeToCoin, TLiveActor)
{
	TAnimalBird* bird = (TAnimalBird*)spine->getBody();
	if (spine->getTime() != 0)
		return TRUE;

	bird->mLiveFlag |= 1;

	TMapObjBase* item = bird->unk150;
	if (item == NULL)
		return TRUE;

	u32 actorType = item->mActorType;

	if (actorType == 0x20000013) {
		((void (*)(TMapObjBase*, const Vec*))(*(u32**)item)[35])(
		    item, (const Vec*)&bird->mPosition);
		((TShine*)item)->appearWithDemo("");
		return TRUE;
	}

	TMapObjBase* spawned;
	if (actorType == 0x2000000E) {
		spawned = gpItemManager->makeObjAppear(0x2000000E);
	} else {
		spawned = item;
	}

	if (spawned == NULL)
		return TRUE;

	(*(void (**)(TMapObjBase*))((*(u32**)spawned)[63]))(spawned);
	((void (*)(TMapObjBase*, const Vec*))(*(u32**)spawned)[35])(
	    spawned, (const Vec*)&bird->mPosition);

	((TLiveActor*)spawned)->mLinearVelocity.x = 0.0f;
	((TLiveActor*)spawned)->mLinearVelocity.y = -10.0f;
	((TLiveActor*)spawned)->mLinearVelocity.z = 0.0f;

	((TLiveActor*)spawned)->mLiveFlag &= ~1;
	((TLiveActor*)spawned)->mLiveFlag |= 0x80;
	return TRUE;
}
