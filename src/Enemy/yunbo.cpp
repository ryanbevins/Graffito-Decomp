#include <Enemy/Yumbo.hpp>
#include <Enemy/Conductor.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorData.hpp>
#include <MoveBG/MapObjWave.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/ObjModel.hpp>
#include <System/Application.hpp>
#include <System/EmitterViewObj.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JGadget/std-list.hpp>
#include <dolphin/mtx.h>
#include <stdlib.h>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

extern "C" {
s16 matan(f32, f32);
}

static char* sambohead_bastable[] = {
	(char*)"/scene/sambohead/bas/flower_shoot.bas",
	(char*)"/scene/sambohead/bas/samboHead_crash.bas",
	(char*)"/scene/sambohead/bas/samboHead_dance.bas",
	(char*)"/scene/sambohead/bas/samboHead_down.bas",
	(char*)"/scene/sambohead/bas/samboHead_Fhide.bas",
	(char*)"/scene/sambohead/bas/samboHead_hit.bas",
	(char*)"/scene/sambohead/bas/samboHead_hit_end.bas",
	(char*)"/scene/sambohead/bas/samboHead_jump_end.bas",
	(char*)"/scene/sambohead/bas/samboHead_jump_start.bas",
	(char*)0,
	(char*)"/scene/sambohead/bas/samboHead_set.bas",
	(char*)"/scene/sambohead/bas/samboHead_turn.bas",
	(char*)0,
};

DEFINE_NERVE(TNerveYumboFreeze, TLiveActor)
{
	TYumbo* self = (TYumbo*)spine->getBody();
	if (spine->getTime() == 0) {
		self->setBckAnm(11);
		if (gpMSound->gateCheck(0x28a6))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x28a6, (Vec*)&self->mPosition, 0, nullptr, 0, 4);
	}
	if (self->getSaveParam2()->mRecoverTimer.get() < spine->getTime()) {
		spine->pushAfterCurrent(&TNerveYumboDancing::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveYumboAttack, TLiveActor)
{
	TYumbo* self = (TYumbo*)spine->getBody();
	if (spine->getTime() == 0) {
		self->mMActor = self->mMActorKeeper->getMActor("flower.bmd");
		self->shotSeeds();
	}

	bool giveup;
	f32 giveupHeight = self->getSaveParam2()->mSLGiveUpHeight.get();
	if (giveupHeight <= fabsf(gpMarioPos->y - self->mPosition.y)) {
		giveup = true;
	} else {
		JGeometry::TVec3<f32> delta = *gpMarioPos;
		delta.x -= self->mPosition.x;
		delta.y -= self->mPosition.y;
		delta.z -= self->mPosition.z;
		delta.y = 0.0f;
		f32 len = self->getSaveParam2()->mSLGiveUpLength.get();
		giveup  = (len * len < delta.squared());
	}

	if (giveup) {
		spine->pushAfterCurrent(&TNerveYumboAppearing::theNerve());
		return TRUE;
	}

	if (self->getSaveParam2()->mSeedLife.get() / 16 < spine->getTime()) {
		spine->pushAfterCurrent(this);
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveYumboAppearing, TLiveActor)
{
	TYumbo* self = (TYumbo*)spine->getBody();
	if (spine->getTime() == 0) {
		self->mMActor = self->mMActorKeeper->getMActor("yumbo.bmd");
		self->setBckAnm(10);
		gpMarioParticleManager->emit(0xb6, &self->mPosition, 0, nullptr);
		gpMarioParticleManager->emit(0xb7, &self->mPosition, 0, nullptr);
	}

	JGeometry::TVec3<f32> delta = *gpMarioPos;
	delta.x -= self->mPosition.x;
	delta.y -= self->mPosition.y;
	delta.z -= self->mPosition.z;
	self->mRotation.y = MsGetRotFromZaxisY(delta);

	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveYumboDancing::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveYumboHiding, TLiveActor)
{
	TYumbo* self = (TYumbo*)spine->getBody();
	if (spine->getTime() == 0) {
		self->setBckAnm(4);
		f32 rate = 3.0f * SMSGetAnmFrameRate();
		self->getMActor()->getFrameCtrl(0)->setRate(rate);
		self->mSpawnedHideParticles = 0;
		if (gpMSound->gateCheck(0x296c))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x296c, (Vec*)&self->mPosition, 0, nullptr, 0, 4);
	}

	if (self->mSpawnedHideParticles == 0) {
		if (self->getMActor()->getFrameCtrl(0)->checkPass(34.0f)) {
			gpMarioParticleManager->emit(0xb6, &self->mPosition, 0, nullptr);
			gpMarioParticleManager->emit(0xb7, &self->mPosition, 0, nullptr);
			self->mSpawnedHideParticles = 1;
		}
	}

	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveYumboAttack::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveYumboDancing, TLiveActor)
{
	TYumbo* self = (TYumbo*)spine->getBody();
	if (spine->getTime() == 0)
		self->setBckAnm(2);

	JGeometry::TVec3<f32> delta = *gpMarioPos;
	delta.x -= self->mPosition.x;
	delta.y -= self->mPosition.y;
	delta.z -= self->mPosition.z;
	self->mRotation.y = MsGetRotFromZaxisY(delta);

	if (fabsf(gpMarioPos->y - self->mPosition.y)
	    < self->getSaveParam2()->mSLSearchHeight.get()) {
		JGeometry::TVec3<f32> mp = *gpMarioPos;
		mp.y = self->mPosition.y;
		f32 sl = self->getSaveParam2()->mSLSearchLength.get();
		f32 sa = self->getSaveParam2()->mSLSearchAngle.get();
		f32 sw = self->getSaveParam2()->mSLSearchAware.get();
		if (self->isInSight(mp, sl, sa, sw)) {
			spine->pushAfterCurrent(&TNerveYumboHiding::theNerve());
			return TRUE;
		}
	}
	return FALSE;
}

TYumboParams::TYumboParams(const char* path)
    : TSmallEnemyParams(path)
    , PARAM_INIT(mRecoverTimer, 600)
    , PARAM_INIT(mShootSpeed, 55.0f)
    , PARAM_INIT(mShootAngleX, 0.2f)
    , PARAM_INIT(mSeedLife, 80)
    , PARAM_INIT(mSeedAirFric, 0.94f)
    , PARAM_INIT(mSeedGravityY, 0.9f)
{
	TParams::load(mPrmPath);
}

TYumboManager::TYumboManager(const char* name)
    : TSmallEnemyManager(name)
    , mFlowerMatTable(nullptr)
{
}

void TYumboManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "yumbo.bmd", 0x10210000, 0 },
		{ "flower.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TYumboManager::load(JSUMemoryInputStream& stream)
{
	TYumboParams* params = new TYumboParams("/enemy/Yumbo.prm");
	unk38                = params;
	TSmallEnemyManager::load(stream);

	params->mSLAttackRadius.set(97);
	params->mSLAttackHeight.set(225);
	params->mSLDamageRadius.set(90);
	params->mSLDamageHeight.set(225);

	void* res       = JKRGetResource("/scene/samboHead/flower_blue.bmt");
	mFlowerMatTable = J3DModelLoaderDataBase::loadMaterialTable(res);
}

const char** TYumbo::getBasNameTable() const
{
	return (const char**)sambohead_bastable;
}

TYumbo::TYumbo(const char* name)
    : TSmallEnemy(name)
{
	onLiveFlag(LIVE_FLAG_UNK10);
}

void TYumbo::init(TLiveManager* manager)
{
	mManager = manager;
	mManager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(mManager, 0x12);

	MActor* yumboBody  = mMActorKeeper->createMActor("yumbo.bmd", 0);
	MActor* flowerBody = mMActorKeeper->createMActor("flower.bmd", 0);
	flowerBody->getModel()->getModelData()->setMaterialTable(
	    (J3DMaterialTable*)((TYumboManager*)mManager)->mFlowerMatTable,
	    J3DMatCopyFlag_All);
	flowerBody->initDL();
	flowerBody->getModel()->lock();

	mMActor = yumboBody;

	mSpine->initWith(&TNerveYumboDancing::theNerve());

	for (TYumboSeed** seed = mSeeds; seed != mSeeds + 16; ++seed) {
		*seed = new TYumboSeed(
		    this, mMActorKeeper->createMActor("samboSeed.bmd", 3));
		(*seed)->init();
	}

	initHitActor(0x1000002A, 1, 0x80000000, 97.5f, 225.0f, 90.0f, 225.0f);
	offHitFlag(HIT_FLAG_NO_COLLISION);

	mGroundHeight = gpMap->checkGround(
	    mPosition.x, mPosition.y + mHeadHeight, mPosition.z, &mGroundPlane);
	mScaledBodyRadius = 75.0f;
	mScaling.set(1.5f, 1.5f, 1.5f);

	initAnmSound();

	mDanceJointIndex = getModel()->getModelData()->getJointName()->getIndex("center");
}

void TYumbo::reset() { }

BOOL TYumbo::receiveMessage(THitActor* sender, u32 message)
{
	if (mLiveFlag & LIVE_FLAG_DEAD)
		return FALSE;
	switch (message) {
	case 0:
	case 1:
		if (mSpine->getLatestNerve() != &TNerveYumboFreeze::theNerve())
			return FALSE;
		mSpine->reset();
		mSpine->setNext(&TNerveSmallEnemyDie::theNerve());
		return TRUE;
	default:
		return TSmallEnemy::receiveMessage(sender, message);
	}
}

void TYumbo::moveObject()
{
	if (mSpine->getLatestNerve() == &TNerveYumboFreeze::theNerve()) {
		JPABaseEmitter* p = gpMarioParticleManager->emitAndBindToPosPtr(
		    0x12f, &mPosition, 1, this);
		if (p) {
			*(f32*)((u8*)p + 0x154) = mScaling.x;
			*(f32*)((u8*)p + 0x158) = mScaling.y;
			*(f32*)((u8*)p + 0x15c) = mScaling.z;
			*(f32*)((u8*)p + 0x174) = mScaling.x;
			*(f32*)((u8*)p + 0x178) = mScaling.y;
			*(f32*)((u8*)p + 0x17c) = mScaling.z;
		}
	}

	if (mMActor->checkCurAnm("sambohead_dance", 0)) {
		int idx     = mDanceJointIndex;
		J3DModel* m = getModel();
		MtxPtr mtx  = m->getAnmMtx(idx);
		gpMarioParticleManager->emitAndBindToMtxPtr(0x18b, mtx, 1, this);
	}

	const TNerveBase<TLiveActor>* latest = mSpine->getLatestNerve();
	if (latest == &TNerveYumboFreeze::theNerve()
	    || latest == &TNerveSmallEnemyDie::theNerve())
		onHitFlag(HIT_FLAG_UNK2);
	else
		offHitFlag(HIT_FLAG_UNK2);

	updateSquareToMario();
	TSmallEnemy::moveObject();
}

void TYumbo::perform(u32 action, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(action, graphics);
	TYumboSeed** end = mSeeds + 16;
	for (TYumboSeed** seed = mSeeds; seed != end; ++seed)
		(*seed)->perform(action, graphics);
}

void TYumbo::behaveToWater(THitActor* sender)
{
	const TNerveBase<TLiveActor>* latest = mSpine->getLatestNerve();
	bool b3                              = true;
	bool b2                              = true;
	bool b1                              = true;
	if (latest != &TNerveYumboFreeze::theNerve()
	    && latest != &TNerveSmallEnemyDie::theNerve())
		b1 = false;
	if (!b1 && latest != &TNerveYumboHiding::theNerve())
		b2 = false;
	if (!b2 && latest != &TNerveYumboAttack::theNerve())
		b3 = false;
	if (!b3) {
		mSpine->reset();
		mSpine->setNext(&TNerveYumboFreeze::theNerve());
	}
}

void TYumbo::shotSeeds()
{
	TYumboSeed* seed = nullptr;
	for (int i = 0; i < 16; ++i) {
		if (!(mSeeds[i]->mState & 1)) {
			seed = mSeeds[i];
			break;
		}
	}
	if (!seed)
		return;

	MActor* m = mMActor;
	if (!m->checkCurBckFromIndex(0))
		m->setBckFromIndex(0);
	setCurAnmSound();

	JGeometry::TVec3<f32> dir = *gpMarioPos;
	dir.x -= mPosition.x;
	dir.y -= mPosition.y;
	dir.z -= mPosition.z;
	dir.y += 200.0f * (0.5f + (f32)rand() * (1.0f / 32768.0f));

	f32 speed = getSaveParam2()->mShootSpeed.get();
	f32 lenSq = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
	if (lenSq <= 0.0000038146973f) {
		dir.set(0.0f, 0.0f, 0.0f);
	} else {
		f32 inv = speed * JGeometry::TUtil<f32>::inv_sqrt(lenSq);
		dir.x  *= inv;
		dir.y  *= inv;
		dir.z  *= inv;
	}

	f32 yawDeg = MsGetRotFromZaxisY(dir);
	f32 yawRad = -(0.017453294f * 0.5f * yawDeg);
	f32 sinY   = sinf(yawRad);
	f32 cosY   = cosf(yawRad);

	f32 qx = 0.0f;
	f32 qy = sinY;
	f32 qz = 0.0f;
	f32 qw = cosY;
	f32 x  = dir.x;
	f32 y  = dir.y;
	f32 z  = dir.z;

	dir.x = (1.0f - 2.0f * (qy * qy + qz * qz)) * x
	        + 2.0f * (qx * qy - qz * qw) * y
	        + 2.0f * (qx * qz + qy * qw) * z;
	dir.y = 2.0f * (qx * qy + qz * qw) * x
	        + (1.0f - 2.0f * (qx * qx + qz * qz)) * y
	        + 2.0f * (qy * qz - qx * qw) * z;
	dir.z = 2.0f * (qx * qz - qy * qw) * x
	        + 2.0f * (qy * qz + qx * qw) * y
	        + (1.0f - 2.0f * (qx * qx + qy * qy)) * z;

	f32 randomHalf = 0.5f * (6.2831855f * ((f32)rand() * (1.0f / 32768.0f)));
	f32 randSin    = sinf(randomHalf);
	f32 randCos    = cosf(randomHalf);
	f32 angleX     = getSaveParam2()->mShootAngleX.get();
	f32 spreadHalf = 0.5f * (-3.1415927f * angleX);
	f32 spreadSin  = sinf(spreadHalf);
	f32 spreadCos  = cosf(spreadHalf);

	qx = randCos * spreadSin;
	qy = randSin * spreadCos;
	qz = -randSin * spreadSin;
	qw = randCos * spreadCos;
	x  = dir.x;
	y  = dir.y;
	z  = dir.z;

	JGeometry::TVec3<f32> dir4;
	dir4.x = (1.0f - 2.0f * (qy * qy + qz * qz)) * x
	         + 2.0f * (qx * qy - qz * qw) * y
	         + 2.0f * (qx * qz + qy * qw) * z;
	dir4.y = 2.0f * (qx * qy + qz * qw) * x
	         + (1.0f - 2.0f * (qx * qx + qz * qz)) * y
	         + 2.0f * (qy * qz - qx * qw) * z;
	dir4.z = 2.0f * (qx * qz - qy * qw) * x
	         + 2.0f * (qy * qz + qx * qw) * y
	         + (1.0f - 2.0f * (qx * qx + qy * qy)) * z;

	s32 seedLife = getSaveParam2()->mSeedLife.get();
	seed->mState &= ~1;
	seed->mPosition = mPosition;
	seed->mVelocity.set(dir4.x, dir4.y, dir4.z);
	seed->mLife = seedLife;
	seed->mScaling.set(2.0f, 2.0f, 2.0f);
	seed->unk64 &= ~1;
	seed->unk64 |= 4;
}

void TYumbo::setDeadAnm() { setBckAnm(3); }

bool TYumbo::doKeepDistance()
{
	return mSpine->getLatestNerve() == &TNerveYumboFreeze::theNerve();
}

void TYumbo::attackToMario()
{
	if (mSpine->getLatestNerve() != &TNerveYumboFreeze::theNerve())
		sendAttackMsgToMario();
}

void TYumboSeed::perform(u32 action, JDrama::TGraphics* graphics)
{
	if (mState & 1)
		return;

	if (action & 2) {
		Mtx mtx;
		mtx[0][0] = 1.0f;
		mtx[1][0] = 0.0f;
		mtx[2][0] = 0.0f;
		mtx[0][1] = 0.0f;
		mtx[1][1] = 1.0f;
		mtx[2][1] = 0.0f;
		mtx[0][2] = 0.0f;
		mtx[1][2] = 0.0f;
		mtx[2][2] = 1.0f;
		mtx[0][3] = mPosition.x;
		mtx[1][3] = mPosition.y;
		mtx[2][3] = mPosition.z;

		J3DModel* mdl = mMActor->getModel();
		// stomp model scale in-place via integer copy
		*(u32*)((u8*)mdl + 0x14) = *(u32*)&mScaling.x;
		*(u32*)((u8*)mdl + 0x18) = *(u32*)&mScaling.y;
		*(u32*)((u8*)mdl + 0x1c) = *(u32*)&mScaling.z;

		PSMTXCopy(mtx, (MtxPtr)((u8*)mdl + 0x20));

		typedef void (*ModelCalcFn)(J3DModel*);
		((ModelCalcFn)((u32*)(*(u32**)mdl))[4])(mdl);
	}

	if (action & 1) {
		mVelocity.y -= mOwner->getSaveParam2()->mSeedGravityY.get();
		mPosition.x += mVelocity.x;
		mPosition.y += mVelocity.y;
		mPosition.z += mVelocity.z;

		f32 fric = mOwner->getSaveParam2()->mSeedAirFric.get();
		mVelocity.x *= fric;
		mVelocity.y *= fric;
		mVelocity.z *= fric;

		for (int i = 0; i < mColCount; ++i) {
			if (mCollisions[i]->mActorType == 0x80000001) {
				SMS_SendMessageToMario(this, HIT_MESSAGE_ATTACK);
				mState |= 1;
			}
		}

		mLife--;
		if (mLife == 0) {
			mState |= 1;
			unk64  |= HIT_FLAG_NO_COLLISION;
		}
	}

	if (!(mState & 4))
		mMActor->perform(action, graphics);
}

void TYumboSeed::init()
{
	initHitActor(0x1000002A, 1, 0x80000000, 30.0f, 30.0f, 0.0f, 0.0f);

	// Insert ourselves into the parent name-ref node's child list.
	// The asm looks up that NameRef by SJIS name (matching @4066), then
	// inserts (void*)this into its JGadget list field at +0x10.
	JDrama::TNameRef* root = JDrama::TNameRefGen::instance->mRootNameRef;
	const char* kName      = "\x93\x47\x83\x4F\x83\x8B\x81\x5B\x83\x76";
	JDrama::TNameRef* parent
	    = root->searchF(JDrama::TNameRef::calcKeyCode(kName), kName);
	JGadget::TList_pointer_void* list
	    = (JGadget::TList_pointer_void*)((u8*)parent + 0x10);
	void* self = this;
	list->insert(list->end(), self);
}
