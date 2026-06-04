#include <MoveBG/MapObjMamma.hpp>
#include <Camera/Camera.hpp>
#include <Camera/CameraShake.hpp>
#include <Enemy/Beam.hpp>
#include <Enemy/SleepBossHanachan.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Map/MapCollisionManager.hpp>
#include <Map/MapData.hpp>
#include <Map/MapMirror.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/MapObjBall.hpp>
#include <MoveBG/MapObjFlag.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <MoveBG/MapObjWave.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorUtil.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/MirrorActor.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <System/TargetArrow.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>
#include <dolphin/mtx.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

extern TBeamManager* gpBeamManager;

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

static inline TLiveActor* findLiveActor(const char* name)
{
	JDrama::TNameRefGen* gen = JDrama::TNameRefGen::instance;
	JDrama::TNameRef* root   = gen->mRootNameRef;
	u16 key                  = JDrama::TNameRef::calcKeyCode(name);
	return (TLiveActor*)root->searchF(key, name);
}

static inline f32 vecLength(const JGeometry::TVec3<f32>& vec)
{
	return vec.length();
}

static inline void addLeanMirrorImpulse(TLeanMirror* mirror, THitActor* actor,
                                        f32 rate)
{
	MtxPtr mtx = mirror->getModel()->getAnmMtx(0);
	f32 divX   = fabsf(mirror->unk138 * mtx[0][0]);
	f32 divZ   = fabsf(mirror->unk138 * mtx[2][2]);
	f32 localX = (actor->mPosition.x - mirror->mPosition.x) / divX;
	f32 localZ = (actor->mPosition.z - mirror->mPosition.z) / divZ;

	mirror->unk14C.x += rate * (localX - mtx[0][1]);
	mirror->unk14C.z += rate * (localZ - mtx[2][1]);
}

static s32 startCameraShakeSE(u32, u32);

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
	unk140.z = 0.0f;
	unk140.y = 0.0f;
	unk140.x = 0.0f;
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
    , unk70(0)
    , unk71(0)
    , unk72(0)
    , unk73(0)
    , unk74(0)
    , unk78(0)
    , unk7C(0.0f)
{
}

void TShiningStone::load(JSUMemoryInputStream& stream)
{
	static const char* modelNames[] = {
		"/scene/mapObj/ShiningStoneGreen.bmd",
		"/scene/mapObj/ShiningStoneBlue.bmd",
		"/scene/mapObj/ShiningStoneRed.bmd",
		"/scene/mapObj/ShiningStoneWhite.bmd",
	};

	JDrama::TActor::load(stream);

	Mtx mtx;
	MsMtxSetXYZRPH(mtx, mPosition.x, mPosition.y, mPosition.z, mRotation.x,
	               mRotation.y, mRotation.z);

	unk68 = new MActor*[4];
	for (int i = 0; i < 4; ++i) {
		unk68[i] = SMS_MakeMActorWithAnmData(
		    modelNames[i], gpMapObjManager->getMActorAnmData(), 3, 0x10020000);
		PSMTXCopy(mtx, unk68[i]->getModel()->getBaseTRMtx());

		TMirrorActor* mirrorActor = new TMirrorActor("鏡用石in鏡");
		mirrorActor->init(unk68[i]->getModel(), 0x1A);
	}

	unk6C = SMS_MakeMActorWithAnmData("/scene/mapObj/ShiningStone.bmd",
	                                  gpMapObjManager->getMActorAnmData(), 3,
	                                  0x10020000);
	unk6C->setBpk("shiningstone");
	unk6C->setBtk("shiningstone");
	PSMTXCopy(mtx, unk6C->getModel()->getBaseTRMtx());

	SMS_LoadParticle("/scene/mapObj/ShiningStone1.jpa", 0x143);
	SMS_LoadParticle("/scene/mapObj/ShiningStone2.jpa", 0x144);
	SMS_LoadParticle("/scene/mapObj/ShiningStone3.jpa", 0x145);
	SMS_LoadParticle("/scene/mapObj/ShiningStoneF.jpa", 0x56);
}

void TShiningStone::perform(u32 flags, JDrama::TGraphics* graphics)
{
	for (int i = 0; i < 4; ++i) {
		unk68[i]->perform(flags, graphics);
		if (unk74 > 0)
			gpMarioParticleManager->emit(0x143, &mPosition, 1, this);
		if (unk74 > 1)
			gpMarioParticleManager->emit(0x144, &mPosition, 1, this);
		if (unk74 > 2)
			gpMarioParticleManager->emit(0x145, &mPosition, 1, this);
	}

	unk6C->perform(flags, graphics);
}

void TShiningStone::putOnLight(TLiveActor* actor)
{
	if (strcmp(actor->getName(), "mirrorS") == 0) {
		unk68[0]->setBck("shiningstonegreen");
		unk68[0]->setBrk("shiningstonegreen");
		unk70 = 1;
	} else if (strcmp(actor->getName(), "mirrorM") == 0) {
		unk68[1]->setBck("shiningstoneblue");
		unk68[1]->setBrk("shiningstoneblue");
		unk71 = 1;
	} else if (strcmp(actor->getName(), "mirrorL") == 0) {
		unk68[2]->setBck("shiningstonered");
		unk68[2]->setBrk("shiningstonered");
		unk72 = 1;
	}

	switch (unk74) {
	case 0:
		unk78 = gpMarioParticleManager->emit(0x143, &mPosition, 1, this);
		unk78->mChildSpawnRate = 3.0f;
		unk7C                 = 1.5f;
		if (gpMSound->gateCheck(0x2893)) {
			MSoundSESystem::MSoundSE::startSoundActor(0x2893, &mPosition, 0,
			                                          nullptr, 0, 4);
		}
		break;
	case 1:
		unk78 = gpMarioParticleManager->emit(0x144, &mPosition, 1, this);
		unk78->mChildSpawnRate = 0.4f;
		unk7C                 = 0.2f;
		if (gpMSound->gateCheck(0x2894)) {
			MSoundSESystem::MSoundSE::startSoundActor(0x2894, &mPosition, 0,
			                                          nullptr, 0, 4);
		}
		break;
	case 2:
		unk78 = gpMarioParticleManager->emit(0x145, &mPosition, 1, this);
		unk7C = 0.0f;
		if (gpMSound->gateCheck(0x2895)) {
			MSoundSESystem::MSoundSE::startSoundActor(0x2895, &mPosition, 0,
			                                          nullptr, 0, 4);
		}
		break;
	default:
		break;
	}

	gpMarioParticleManager->emit(0x56, &mPosition, 0, nullptr);

	unk74 += 1;
	if (unk74 == 3) {
		unk68[3]->setBck("shiningstonewhite");
		unk68[3]->setBrk("shiningstonewhite");
		unk73 = 1;
	}
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

void TLeanMirror::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);

	f32 radius;
	stream.read(&radius, 4);
	unk138 = radius * 100.0f * 0.5f;
	unk13C = unk138;

	if (gpMarDirector->unk7D == 1) {
		char buf[64];
		stream.readString(buf, 64);
		stream.read(&unk1A0.x, 4);
		stream.read(&unk1A0.y, 4);
		stream.read(&unk1A0.z, 4);
	}

	TMirrorModelObj* mirrorModel = new TMirrorModelObj;
	char modelName[64];
	snprintf(modelName, 64, "/scene/mapObj/%sTop.bmd", unkF4);
	mirrorModel->init(modelName);
	mirrorModel->unk28 = getModel();

	if (gpMarDirector->unk7D != 1)
		mState = 4;
}

void TLeanMirror::initMapObj()
{
	TMapObjBase::initMapObj();
	unk158.x = 0.03f;
	unk158.y = 0.999f;
	unk158.z = 0.0001f;
	unk164.y = 1.0f;
	unk164.z = 0.0002f;
	unk170   = 0.0001f;
	unk174   = 0.865f;
	unk178   = 0.5f;

	if (strcmp(unkF4, "mirrorS") == 0) {
		unk164.x = 0.002f;
		unk164.y = 1.0f;
		unk174   = 0.87f;
		unk19C   = 1;
	} else if (strcmp(unkF4, "mirrorM") == 0) {
		unk164.x = 0.004f;
		unk19C   = 2;
	} else {
		unk164.x = 0.006f;
		unk19C   = 3;
	}
}

void TLeanMirror::loadAfter()
{
	TMapObjBase::loadAfter();
	unk17C = (TShiningStone*)findLiveActor("ShiningStone");

	unk180 = unk17C->mPosition - mPosition;

	f32 lenSq = unk180.squared();
	if (lenSq <= JGeometry::TUtil<f32>::epsilon()) {
		unk180.zero();
	} else {
		f32 rate = JGeometry::TUtil<f32>::one()
		           * JGeometry::TUtil<f32>::inv_sqrt(lenSq);
		unk180.scale(rate);
	}
}

void TLeanMirror::control()
{
	TMapObjBase::control();
	switch (mState) {
	case 1:
		controlShake();
		if (gpMSound->gateCheck(0x3048)) {
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(
			    0x3048, &mPosition, nullptr, vecLength(unk14C), 0, 0, nullptr,
			    0, 4);
		}
		break;
	case 2:
		controlGoTarget();
		if (gpMSound->gateCheck(0x304A)) {
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(
			    0x304A, &mPosition, nullptr, vecLength(unk14C), 0, 0, nullptr,
			    0, 4);
		}
		break;
	case 3:
		if (!(mLifeTimer > 0 ? true : false)) {
			if (unk17C->unk74 < 3)
				MSBgm::setTrackVolume(0, 1.0f, 10, 0);

			if (unk17C->unk7C > 0.0f)
				unk17C->unk78->mChildSpawnRate = unk17C->unk7C;

			mState = 4;
		}
		break;
	default:
		break;
	}
}

void TLeanMirror::controlShake()
{
	if ((unk19C != 0 ? true : false) && unk1AC
	    && SMS_IsMarioTouchGround4cm()
	    && SMS_GetMarioGrPlane()->getActor() != this) {
		MSBgm::stopTrackBGM(1, 10);
		MSBgm::setTrackVolume(0, 1.0f, 10, 0);
		unk1AC = 0;
	}

	if (unk14C.squared() <= 0.0000038146973f)
		return;

	unk14C.scale(unk158.y);

	J3DModel* model = getModel();
	MtxPtr mtx      = model->getAnmMtx(0);

	JGeometry::TVec3<f32> axis(unk14C.x, 0.0f, unk14C.z);
	rotateVecByAxisY(&axis, 1.5707963f);

	f32 speedXZ
	    = JGeometry::TUtil<f32>::sqrt(unk14C.x * unk14C.x + unk14C.z * unk14C.z);
	f32 angle   = speedXZ * unk158.x;
	Mtx rotMtx;
	MTXIdentity(rotMtx);
	makeMtxRotByAxis(axis, angle, rotMtx);
	concatOnlyRotFromLeft(rotMtx, mtx, mtx);

	bool hitLimit = false;
	if (mtx[1][1] < unk174) {
		f32 dot = unk14C.x * mtx[0][1] + unk14C.z * mtx[2][1];
		if (dot > 0.0f)
			hitLimit = true;
	}

	if (hitLimit) {
		if (gpMSound->gateCheck(0x3849)) {
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(
			    0x3849, &mPosition, nullptr, vecLength(unk14C), 0, 0, nullptr,
			    0, 4);
		}
		unk14C.scale(-unk178);
		PSMTXCopy(model->getBaseTRMtx(), mtx);
	} else {
		PSMTXCopy(mtx, model->getBaseTRMtx());
	}
}

void TLeanMirror::controlGoTarget()
{
	MtxPtr mtx = getModel()->getAnmMtx(0);
	Mtx rotMtx;
	rotMtx[1][0] = rotMtx[2][0] = rotMtx[0][1] = rotMtx[2][1]
	    = rotMtx[0][2] = rotMtx[1][2] = rotMtx[0][3] = rotMtx[1][3]
	    = rotMtx[2][3] = 0.0f;
	rotMtx[0][0] = rotMtx[1][1] = rotMtx[2][2] = 1.0f;
	makeMtxRotByAxis(unk18C, unk198, rotMtx);
	concatOnlyRotFromLeft(rotMtx, mtx, mtx);

	if (mLifeTimer > 0 ? true : false)
		return;

	unk17C->putOnLight(this);
	if (unk17C->unk73) {
		TSleepBossHanachan* boss
		    = (TSleepBossHanachan*)findLiveActor("居眠りボスハナチャン");
		if (boss) {
			boss->startFall(unk17C->mPosition.x, unk17C->mPosition.y + 1100.0f,
			                unk17C->mPosition.z);
		}

		JDrama::TFlagT<u16> flag(0);
		gpMarDirector->fireStartDemoCamera(
		    "demohanatyan_cam01", nullptr, -1, 0.0f, true,
		    startCameraShakeSE, (u32)&mPosition, nullptr, flag);
	} else {
		JDrama::TFlagT<u16> flag(0);
		gpMarDirector->fireStartDemoCamera("太陽石点灯カメラ",
		                                   &unk17C->mPosition, mDemoLightTime,
		                                   0.0f, true, nullptr, 0, nullptr,
		                                   flag);
	}

	mLifeTimer = mDemoLightTime;
	mState     = 3;
}

static s32 startCameraShakeSE(u32 pos, u32 time)
{
	if (time == 0 && gpMSound->gateCheck(0x3008)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x3008, (const Vec*)pos, 0,
		                                          nullptr, 0, 4);
	}

	return 0;
}

void TLeanMirror::release()
{
	MtxPtr mtx = getModel()->getAnmMtx(0);
	JGeometry::TVec3<f32> up(mtx[0][1], mtx[1][1], mtx[2][1]);

	unk18C.x = unk180.y * up.z - unk180.z * up.y;
	unk18C.y = unk180.z * up.x - unk180.x * up.z;
	unk18C.z = unk180.x * up.y - unk180.y * up.x;

	JGeometry::TVec3<f32> axis(up.y * unk180.z - up.z * unk180.y,
	                           up.z * unk180.x - up.x * unk180.z,
	                           up.x * unk180.y - up.y * unk180.x);
	f32 crossLen = JGeometry::TUtil<f32>::sqrt(axis.squared());
	f32 dot      = up.dot(unk180);
	unk198       = fabsf(atan2f(crossLen, dot)) / (f32)mGoTargetTime;

	mLifeTimer = mGoTargetTime;
	mState     = 2;
	unkF8 &= ~2;
	SMS_MarioMoveRequest(unk1A0);

	JDrama::TFlagT<u16> flag(0);
	if (strcmp(unkF4, "mirrorS") == 0) {
		gpMarDirector->fireStartDemoCamera(
		    "ぐらぐら鏡Ｓカメラ", &unk17C->mPosition,
		    mGoTargetTime + mDemoWaitTime, 0.0f, true, nullptr, 0, nullptr,
		    flag);
	} else if (strcmp(unkF4, "mirrorM") == 0) {
		gpMarDirector->fireStartDemoCamera(
		    "ぐらぐら鏡Ｍカメラ", &unk17C->mPosition,
		    mGoTargetTime + mDemoWaitTime, 0.0f, true, nullptr, 0, nullptr,
		    flag);
	} else if (strcmp(unkF4, "mirrorL") == 0) {
		gpMarDirector->fireStartDemoCamera(
		    "ぐらぐら鏡Ｌカメラ", &unk17C->mPosition,
		    mGoTargetTime + mDemoWaitTime, 0.0f, true, nullptr, 0, nullptr,
		    flag);
	}

	MSBgm::stopTrackBGM(1, 10);
}

void TLeanMirror::touchEnemy(THitActor* actor)
{
	bool isBossPart = actor->mActorType == 0x10000016 ? true : false;
	if (isBossPart && *((u8*)actor + 0x1B0))
		addLeanMirrorImpulse(this, actor, unk164.z);
}

void TLeanMirror::touchPlayer(THitActor* actor)
{
	bool canTouch = mState == 1 ? true : false;
	if (!canTouch)
		return;
	if (!marioIsOn())
		return;

	addLeanMirrorImpulse(this, actor, unk158.z);
	if (!unk1AC) {
		MSBgm::startBGM(0x80010011);
		MSBgm::setTrackVolume(0, 0.0f, 10, 0);
		unk1AC = 1;
	}
}

BOOL TLeanMirror::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 0) {
		sendMsg(0x10000016, message);
		addLeanMirrorImpulse(this, sender, unk164.x);
		return TRUE;
	}

	if (message == 1) {
		sendMsg(0x10000016, message);
		addLeanMirrorImpulse(this, sender, unk164.y);
		return TRUE;
	}

	if (message == 3) {
		addLeanMirrorImpulse(this, sender, unk170);
		return TRUE;
	}

	if (message == 8) {
		unk19C -= 1;
		if (unk19C == 0)
			release();
		return TRUE;
	}

	return FALSE;
}

void TLeanMirror::draw() const
{
	MtxPtr mtx = getModel()->getAnmMtx(0);
	JGeometry::TVec3<f32> dir(mtx[0][1], mtx[1][1], mtx[2][1]);
	JGeometry::TVec3<f32> start = dir;
	start.scale(350.0f * 0.001f * mBodyRadius);
	start.add(mPosition);

	JGeometry::TVec3<f32> end = dir;
	end.scale(10000.0f);
	end.add(mPosition);

	gpBeamManager->requestCone(start, end, 1.7f * mBodyRadius, true, true,
	                           false);
}

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
	((TSandLeaf*)unk144)->unk138 = (u32)this;
	unk144->appear();

	unk158 = findMapObj("ステージ切替（砂の城）");
	unk158->makeObjDead();
}

TMapObjBase* TSandCastle::findTriggerActor()
{
	return findMapObj("砂の城爆発の芽");
}

void TSandCastle::calcRootMatrix()
{
	bool inState2 = mState == 2 ? true : false;
	if (!inState2)
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

void TSandCastle::waitBeforeExplode()
{
	mState = 6;
	mLifeTimer = unk148;

	JDrama::TFlagT<u16> flag(0);
	gpMarDirector->fireStartDemoCamera("mamma1_sandcastle", nullptr, -1,
	                                   0.0f, true, SandCastleCallBack, 0,
	                                   nullptr, flag);
	unk15C = 1;
}

void TSandCastle::explode()
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
}

void TSandCastle::expanded()
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
	((TSandLeaf*)unk144)->unk138 = (u32)this;
	unk144->appear();
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

void TSandBombBase::grow()
{
	mState = 5;
}

void TSandBombBase::waitBeforeExplode()
{
	mState = 6;
	mLifeTimer = unk148;
}

void TSandBombBase::explode()
{
	startControlAnim(1);
	mScaling.y = 1.0f;
	if (mMapCollisionManager)
		mMapCollisionManager->changeCollision(1);
	gpMarioParticleManager->emit(0x55, &mPosition, 0, 0);
	mState = 7;
}

void TSandBombBase::exploding()
{
	if (animIsFinished())
		mState = 8;
}

void TSandBombBase::expanded()
{
	if (unk144 && unk144->animIsFinished())
		mState = 2;
}

void TSandBombBase::withered()
{
	mLifeTimer = unk140;
	mState     = 3;
	unk144->sleep();
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

void TSandLeafBase::grow()
{
	if (mScaling.y < 1.0f) {
		mScaling.y += unk138;
		if (mScaling.y > 1.0f)
			mScaling.y = 1.0f;
	}
}

TSandBase::TSandBase(const char* name)
    : TMapObjBase(name)
    , unk138(0.0f)
    , unk13C(0.0f)
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
