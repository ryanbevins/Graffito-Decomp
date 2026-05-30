#include <MoveBG/MapObjMamma.hpp>
#include <Camera/Camera.hpp>
#include <Camera/CameraShake.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Map/MapCollisionManager.hpp>
#include <Map/MapData.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/MapObjBall.hpp>
#include <MoveBG/MapObjFlag.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <MoveBG/MapObjWave.hpp>
#include <M3DUtil/MActor.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <Player/MarioAccess.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <System/TargetArrow.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>
#include <dolphin/mtx.h>
#include <string.h>

u32 TSandBase::mWitherTime = 800;
f32 TSandBase::mScaleMin   = 0.00001f;

f32 TSandBombBase::mFiringFrameSpeed     = 3.0f;
f32 TSandBombBase::mFiringFrameDownSpeed = 0.2f;
f32 TSandBombBase::mExplodeFrameSpeed    = 1.0f;
f32 TSandBombBase::mMarioJumpRate        = 0.12f;
u32 TSandBombBase::mExlodingRumbleTime   = 20;

f32 TSandCastle::mCollisionRate = 1.7f;

u32 TLeanMirror::mGoTargetTime  = 600;
u32 TLeanMirror::mDemoWaitTime  = 0xFFFFFFFF;
u32 TLeanMirror::mDemoLightTime = 360;

f32 TMammaBlockRotate::mRotSpeed       = 0.1f;
f32 TMammaBlockRotate::mRotReturnSpeed = 0.01f;
f32 TMammaBlockRotate::mRotEnd         = 130.0f;
f32 TMammaBlockRotate::mMapGoSpeed     = 1.0f;
f32 TMammaBlockRotate::mMapBackSpeed   = 0.1f;
u32 TMammaBlockRotate::mWaitTime       = 600;

static void zeroVec(JGeometry::TVec3<f32>& v)
{
	v.x = 0.0f;
	v.y = 0.0f;
	v.z = 0.0f;
}

static void oneVec(JGeometry::TVec3<f32>& v)
{
	v.x = 1.0f;
	v.y = 1.0f;
	v.z = 1.0f;
}

static inline TMapObjBase* findMapObj(const char* name)
{
	JDrama::TNameRefGen* gen = JDrama::TNameRefGen::instance;
	JDrama::TNameRef* root   = gen->mRootNameRef;
	u16 key                  = JDrama::TNameRef::calcKeyCode(name);
	return (TMapObjBase*)root->searchF(key, name);
}

u32 TSandEgg::getSDLModelFlag() const { return 0; }

TMammaMirrorMapOperator::TMammaMirrorMapOperator(const char* name)
    : JDrama::TViewObj(name)
{
	for (int i = 0; i < 8; ++i) {
		unk10[i] = 0;
		zeroVec(unk30[i]);
		unk90[i] = 0.0f;
		unkB0[i] = 0;
	}

	for (int i = 0; i < 3; ++i)
		zeroVec(unkB8[i]);
}

void TMammaMirrorMapOperator::loadAfter()
{
	static const char* names[] = { "mirrorS", "mirrorM", "mirrorL" };

	for (int i = 0; i < 3; ++i) {
		TMapObjBase* obj = findMapObj(names[i]);
		if (obj)
			unkB8[i].set(obj->mPosition);
	}
}

void TMammaMirrorMapOperator::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (!(flags & 1))
		return;

	for (int i = 0; i < 8; ++i) {
		if (unk10[i] && unkB0[i])
			unk10[i]->mPosition.set(unk30[i]);
	}
}

TGoalWatermelon::TGoalWatermelon(const char* name)
    : TMapObjBase(name)
    , unk138(0)
    , unk13C(0)
{
	zeroVec(unk140);
}

void TGoalWatermelon::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	char buf[32];
	stream.readString(buf, 32);
	stream.read(&unk140.x, 4);
	stream.read(&unk140.y, 4);
	stream.read(&unk140.z, 4);
}

void TGoalWatermelon::loadAfter()
{
	TMapObjBase::loadAfter();
	unk64 |= 4;
	unk138 = findMapObj("シャイン（お化けスイカ用）");
	if (unk138) {
		unk138->mPosition.set(unk140);
		unk138->appear();
	}
}

void TGoalWatermelon::control()
{
	TMapObjBase::control();
	if (mState == 2 && unk13C && ((TMapObjBase*)unk13C)->animIsFinished()) {
		gpItemManager->makeShineAppearWithDemoOffset("シャイン（お化けスイカ用）",
		                                             "スイカゴールカメラ",
		                                             0.0f, 0.0f, 0.0f);
		mState = 3;
	}
}

void TGoalWatermelon::touchActor(THitActor* actor)
{
	if (mState != 1 || !actor)
		return;

	if ((actor->mActorType - 0x40000000) != 0xD0)
		return;

	unk13C = actor;
	((TMapObjBase*)unk13C)->unkF8 &= ~0x100;
	((TMapObjBase*)unk13C)->startBck("watermelon_shrink");
	((TMapObjBase*)unk13C)->mVelocity.zero();
	JDrama::TFlagT<u16> flag(0);
	gpMarDirector->fireStartDemoCamera("スイカゴールカメラ", &actor->mPosition,
	                                   -1, 0.0f, true, 0, 0, actor, flag);
	mState = 2;
}

TSandBird::TSandBird(const char* name)
    : TJointCoin(name)
{
	unk150 = 0;
	unk151 = 0;
}

void TSandBird::initMapObj()
{
	TJointCoin::initMapObj();
	if (!gParticleFlagLoaded[0x159]) {
		gpResourceManager->load("/scene/map/map/ms_sunadori_a.jpa", 0x159);
		gParticleFlagLoaded[0x159] = true;
	}
	if (!gParticleFlagLoaded[0x15A]) {
		gpResourceManager->load("/scene/map/map/ms_sunadori_b.jpa", 0x15A);
		gParticleFlagLoaded[0x15A] = true;
	}
}

bool TSandBird::nameIsObj(const char* name) { return strstr(name, "none") == 0; }

TMapObjBase* TSandBird::makeObjFromJointName(const char* name, u16 joint_id)
{
	TMapObjBase* obj = TJointCoin::makeObjFromJointName(name, joint_id);
	if (obj)
		return obj;

	if (strstr(name, "none"))
		return 0;

	return makeObj("SandBirdBlock", joint_id);
}

void TSandBird::control() { TJointCoin::control(); }

void TMammaYacht::initMapObj()
{
	TMapObjBase::initMapObj();
	unk138 = new TMapObjFlag("旗");
	unk138->mPosition.set(mPosition.x + 2.0f, mPosition.y + 1125.0f,
	                      mPosition.z - 15.0f);
	unk138->mRotation.set(0.0f, 180.0f, 0.0f);
	unk138->mScaling.set(1.0f, 2.5f, 3.8f);
	unk138->init("MammaYacht00");
}

void TMammaYacht::control()
{
	TMapObjBase::control();
	u16 attr = mGroundPlane ? mGroundPlane->mBGType : 0;
	bool onWater = attr == 0x100 || attr == 0x101
	               || (u16)(attr - 0x102) <= 3 || attr == 0x4104;
	if (onWater) {
		mPosition.y = mInitialPosition.y
		              + gpMapObjWave->getWaveHeight(mPosition.x, mPosition.z);
		if (unk138)
			unk138->mPosition.y = mPosition.y - 50.0f;
	}
}

TMammaBlockRotate::TMammaBlockRotate(const char* name)
    : TMapObjBase(name)
    , unk138(0)
    , unk13C(0)
    , unk140(0.0f)
    , unk144(0)
    , unk148(0)
{
}

void TMammaBlockRotate::load(JSUMemoryInputStream& stream)
{
	unk144 = new TMapCollisionMove;
	unk144->init("/scene/mapObj/MammaBlockDown.col", 0, this);
	unk148 = new TMapCollisionMove;
	unk148->init("/scene/mapObj/MammaBlockUp.col", 0, this);
	TMapObjBase::load(stream);
}

void TMammaBlockRotate::initMapObj()
{
	TMapObjBase::initMapObj();
	unk140 = 0.0f;
}

void TMammaBlockRotate::control()
{
	TMapObjBase::control();
	if (mState == 1) {
		unk140 += mRotSpeed;
		if (unk140 > mRotEnd) {
			unk140 = mRotEnd;
			mState = 2;
			mLifeTimer = mWaitTime;
		}
	} else if (mState == 3) {
		unk140 -= mRotReturnSpeed;
		if (unk140 < 0.0f) {
			unk140 = 0.0f;
			mState = 0;
		}
	}
}

u32 TMammaBlockRotate::touchWater(THitActor*)
{
	if (mState == 0)
		mState = 1;
	return 1;
}

TShiningStone::TShiningStone(const char* name)
    : THitActor(name)
    , unk68(0)
    , unk74(0)
    , unk78(0)
    , unk7C(0.0f)
    , unk70(0)
    , unk71(0)
    , unk72(0)
    , unk73(0)
{
}

TLeanMirror::TLeanMirror(const char* name)
    : TMapObjBase(name)
    , unk138(0.0f)
    , unk13C(0.0f)
    , unk170(0.0f)
    , unk17C(0)
    , unk198(0.0f)
    , unk19C(0)
    , unk1AC(0)
    , unk1AE(0)
{
	zeroVec(unk140);
	zeroVec(unk14C);
	zeroVec(unk158);
	zeroVec(unk164);
	zeroVec(unk180);
	zeroVec(unk18C);
	zeroVec(unk1A0);
}

u32 TLeanMirror::getSDLModelFlag() const { return 0; }

TSandCastle::TSandCastle(const char* name)
    : TSandBombBase(name)
    , unk158(0)
    , unk15C(0)
{
}

void TSandCastle::initMapObj()
{
	TSandBombBase::initMapObj();
	unk13C = 0.2f;
	unk148 = 120;
	sleep();
}

void TSandCastle::loadAfter()
{
	unk144 = findTriggerActor();
	if (unk144) {
		((TSandLeaf*)unk144)->unk138 = (u32)this;
		unk144->appear();
	}

	unk158 = findMapObj("ステージ切替（砂の城）");
	if (unk158)
		unk158->makeObjAppeared();
}

TMapObjBase* TSandCastle::findTriggerActor()
{
	return findMapObj("砂の城爆発の芽");
}

void TSandCastle::calcRootMatrix()
{
	if (mState != 2)
		TMapObjBase::calcRootMatrix();
}

static s32 SandCastleCallBack(u32, u32 param_2)
{
	if (param_2 == 1) {
		gpTargetArrow->unk14 = 1;
		JGeometry::TVec3<f32> pos(8400.0f, 300.0f, 8150.0f);
		gpTargetArrow->setPos(pos);

		JGeometry::TVec3<f32>* at
		    = (JGeometry::TVec3<f32>*)((u8*)gpCamera + 0x148);
		s16 angle = matan(gpCamera->unk124.z - at->z,
		                   gpCamera->unk124.x - at->x);
		gpCamera->warpPosAndAt(gpCamera->unkA8, angle);
	}

	return 1;
}

BOOL TSandCastle::waitBeforeExplode()
{
	mState = 6;
	mLifeTimer = unk148;

	JDrama::TFlagT<u16> flag(0);
	gpMarDirector->fireStartDemoCamera("mamma1_sandcastle", nullptr, -1,
	                                   0.0f, true, SandCastleCallBack, 0,
	                                   nullptr, flag);
	unk15C = 1;
	return true;
}

BOOL TSandCastle::explode()
{
	startControlAnim(1);
	mScaling.y = 1.0f;

	if (mMapCollisionManager) {
		mMapCollisionManager->changeCollision(1);
		TMapCollisionBase* collision = mMapCollisionManager->unk8;
		if (collision) {
			collision->setUp();
			collision->moveSRT(mPosition, mRotation, mScaling);
		}
	}

	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emit(0x55, &mPosition, 0, nullptr);
	if (emitter) {
		JGeometry::TVec3<f32> scale(unk14C, unk14C, unk14C);
		emitter->setScale(scale);
	}

	if (!gpMarDirector->checkUnk124Thing2())
		gpCameraShake->startShake((EnumCamShakeMode)0xD, 1.0f);

	if (gpMSound->gateCheck(0x28A4)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x28A4, &mPosition, 0,
		                                          nullptr, 0, 4);
	}

	SMSRumbleMgr->start(0x15, mExlodingRumbleTime, (Vec*)&mPosition);
	mState = 7;
	awake();

	if (unk158)
		unk158->appear();

	startControlAnim(3);
	return true;
}

BOOL TSandCastle::expanded()
{
	if (unk144) {
		MActor* actor  = unk144->getMActor();
		J3DFrameCtrl* frame = actor->getFrameCtrl(0);
		frame->setFrame(frame->getFrame() + unk150);

		if (gpMSound->gateCheck(0x20C6)) {
			MSoundSESystem::MSoundSE::startSoundActor(0x20C6,
			                                          &unk144->mPosition, 0,
			                                          nullptr, 0, 4);
		}

		if (unk144->animIsFinished())
			mState = 2;

		if (unk144->animIsFinished()) {
			mState = 2;
			startControlAnim(2);
			startControlAnim(3);
		}
	}

	return true;
}

BOOL TSandCastle::withering()
{
	J3DFrameCtrl* frame = getMActor()->getFrameCtrl(0);
	frame->setFrame(frame->getFrame() + unk13C);

	J3DFrameCtrl* frame2 = getMActor()->getFrameCtrl(5);
	frame2->setFrame(frame2->getFrame() + unk13C);

	frame = getMActor()->getFrameCtrl(0);
	f32 current = frame->getFrame();
	f32 end     = (f32)frame->getEnd();
	mScaling.y  = mCollisionRate * ((end - current) / end);

	if (current > 240.0f && unk158 && !(unk158->unkF8 & 1)) {
		unk158->kill();
		gpTargetArrow->unk14 = 0;
	}

	if (animIsFinished()) {
		sleep();
		return true;
	}

	return false;
}

TSandBombBase::TSandBombBase(const char* name)
    : TSandBase(name)
    , unk148(0)
    , unk14C(1.0f)
    , unk150(0.0f)
    , unk154(0.0f)
{
}

void TSandBombBase::initMapObj()
{
	unk138 = 0.006f;
	unk13C = 0.0000038146973f;
	unk140 = 0;
	unk148 = 60;
	unk154 = 2000.0f;
	mScaling.y = mScaleMin;
	TMapObjBase::initMapObj();
	unk150 = 0.5f;
}

void TSandBombBase::loadAfter()
{
	unk144 = findTriggerActor();
	if (unk144) {
		((TSandLeaf*)unk144)->unk138 = (u32)this;
		unk144->appear();
	}
}

TMapObjBase* TSandBombBase::findTriggerActor()
{
	JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
	return TMapObjBaseManager::newAndRegisterObj("SandBomb", mPosition,
	                                            mRotation, scale);
}

void TSandBombBase::control()
{
	TMapObjBase::control();
	switch (mState) {
	case 5:
		waitBeforeExplode();
		break;
	case 6:
		explode();
		break;
	case 7:
		exploding();
		break;
	case 8:
		expanded();
		break;
	default:
		break;
	}
}

BOOL TSandBombBase::grow()
{
	mState = 5;
	return true;
}

BOOL TSandBombBase::waitBeforeExplode()
{
	mState = 6;
	mLifeTimer = unk148;
	return true;
}

BOOL TSandBombBase::explode()
{
	startControlAnim(1);
	mScaling.y = 1.0f;
	if (mMapCollisionManager)
		mMapCollisionManager->changeCollision(1);
	gpMarioParticleManager->emit(0x55, &mPosition, 0, 0);
	mState = 7;
	return true;
}

BOOL TSandBombBase::exploding()
{
	if (animIsFinished())
		mState = 8;
	return true;
}

BOOL TSandBombBase::expanded()
{
	if (unk144 && unk144->animIsFinished())
		mState = 2;
	return true;
}

BOOL TSandBombBase::withered()
{
	sleep();
	return true;
}

void TSandBomb::initMapObj() { TMapObjBase::initMapObj(); }

u32 TSandBomb::getSDLModelFlag() const { return 0; }

u32 TSandBomb::touchWater(THitActor*)
{
	if (unk138)
		((TMapObjBase*)unk138)->makeObjAppeared();
	startControlAnim(3);
	startControlAnim(4);
	startControlAnim(5);
	unk64 |= 1;
	return 1;
}

void TSandBomb::makeObjAppeared()
{
	TMapObjBase::makeObjAppeared();
	startControlAnim(1);
	startControlAnim(2);
}

void TSandLeafBase::initMapObj()
{
	unk138 = 0.03f;
	unk13C = 0.0001f;
	unk140 = 0;
	mScaling.y = mScaleMin;
	TMapObjBase::initMapObj();
	JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
	unk144 = TMapObjBaseManager::newAndRegisterObj("SandLeaf", mPosition,
	                                              mRotation, scale);
	if (unk144) {
		((TSandLeaf*)unk144)->unk138 = (u32)this;
		unk144->appear();
	}
}

void TSandLeafBase::control()
{
	TMapObjBase::control();
	if (mState == 2)
		grow();
	else if (mState == 5)
		withering();
}

BOOL TSandLeafBase::grow()
{
	if (mScaling.y < 1.0f) {
		mScaling.y += unk138;
		if (mScaling.y > 1.0f)
			mScaling.y = 1.0f;
		return false;
	}
	return true;
}

TSandBase::TSandBase(const char* name)
    : TMapObjBase(name)
    , unk138(0.0f)
    , unk13C(0.0f)
    , unk140(0)
    , unk144(0)
{
}

BOOL TSandBase::withering()
{
	mScaling.y -= unk13C;
	if (mScaling.y < mScaleMin)
		mScaling.y = mScaleMin;
	return mScaling.y <= mScaleMin;
}

void TSandLeaf::control()
{
	TMapObjBase::control();
	mPosition.y = gpMap->checkGround(mPosition.x, mPosition.y + 50.0f,
	                                 mPosition.z, &mGroundPlane);
}

u32 TSandLeaf::touchWater(THitActor*)
{
	if (unk138)
		((TMapObjBase*)unk138)->makeObjAppeared();
	return 1;
}
