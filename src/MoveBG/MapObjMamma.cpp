#define LIVEACTOR_GETMACTOR_OUT_OF_LINE
#include <MoveBG/MapObjMamma.hpp>
#include <Camera/Camera.hpp>
#include <Camera/CameraShake.hpp>
#include <Enemy/Beam.hpp>
#include <Enemy/SleepBossHanachan.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <Map/Map.hpp>
#include <Map/MapModel.hpp>
#include <Map/MapStaticObject.hpp>
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
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/MirrorActor.hpp>
#include <GC2D/GCConsole2.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <System/TargetArrow.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JGeometry/JGMatrix34.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>
#include <dolphin/mtx.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

extern TBeamManager* gpBeamManager;

static inline MActor* getMActorInline(const TLiveActor* actor)
{
	return actor->mMActor;
}

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
	v.z = 0.0f;
	v.y = 0.0f;
	v.x = 0.0f;
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
	f32 localX = (actor->mPosition.x - mirror->mPosition.x)
	             / fabsf(mirror->unk138 * mtx[0][0]);
	f32 localZ = (actor->mPosition.z - mirror->mPosition.z)
	             / fabsf(mirror->unk138 * mtx[2][2]);

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
	unkB8[0].set(findMapObj("mirrorS")->mPosition);
	unkB8[1].set(findMapObj("mirrorM")->mPosition);
	unkB8[2].set(findMapObj("mirrorL")->mPosition);

	J3DJoint* joint = ((TMapStaticObj*)findMapObj("mirrorL"))
	                      ->getModelData()
	                      ->getJointNodePointer(2);

	for (int i = 0; i < 8; ++i) {
		unk10[i] = (TMapObjBase*)joint;

		const Vec& min = joint->getMin();
		const Vec& max = joint->getMax();

		unk30[i].x = 0.5f * (max.x + min.x);
		unk30[i].y = 0.5f * (max.y + min.y);
		unk30[i].z = 0.5f * (max.z + min.z);

		f32 radX = 0.5f * (max.x - min.x);
		f32 radZ = 0.5f * (max.z - min.z);
		if (radX > radZ)
			unk90[i] = radX;
		else
			unk90[i] = radZ;

		unk90[i] += 2000.0f;
		if (unk90[i] > 3000.0f)
			unk90[i] = 3000.0f;

		joint = (J3DJoint*)joint->getYounger();
	}
}

void TMammaMirrorMapOperator::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (!(flags & 2))
		return;

	TMirrorModelManager* mirrorManager = gpMirrorModelManager;
	const int& mirrorIndex = mirrorManager->unk18;
	if (mirrorIndex != -1 ? true : false) {
		const JGeometry::TVec3<f32>& camPos = mirrorManager->unk24->unk98;

		const JGeometry::TVec3<f32>& mirrorPos = unkB8[mirrorIndex];
		f32 dx = camPos.x - mirrorPos.x;
		f32 dy = camPos.y - mirrorPos.y;
		f32 dz = camPos.z - mirrorPos.z;
		f32 dxSq = dx * dx;
		f32 dySq = dy * dy;
		f32 dzSq = dz * dz;
		f32 inMirrorDist
		    = JGeometry::TUtil<f32>::sqrt(dxSq + dySq + dzSq);

		for (int i = 0; i < 8; ++i) {
			f32 dx = camPos.x - unk30[i].x;
			f32 dy = camPos.y - unk30[i].y;
			f32 dz = camPos.z - unk30[i].z;
			f32 dxSq = dx * dx;
			f32 dySq = dy * dy;
			f32 dzSq = dz * dz;
			f32 dist
			    = JGeometry::TUtil<f32>::sqrt(dxSq + dySq + dzSq);

			if (dist > unk90[i] || dist > inMirrorDist) {
				if (unkB0[i]) {
					SMS_ShowJoint(((J3DJoint*)unk10[i])->getMesh(), true);
					unkB0[i] = 0;
				}
			} else {
				if (!unkB0[i]) {
					SMS_ShowJoint(((J3DJoint*)unk10[i])->getMesh(), false);
					unkB0[i] = 1;
				}
			}
		}
	} else {
		for (int i = 0; i < 8; ++i) {
			if (!unkB0[i]) {
				SMS_ShowJoint(((J3DJoint*)unk10[i])->getMesh(), false);
				unkB0[i] = 1;
			}
		}
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
	unk138->mPosition.set(unk140);
	unk138->appear();
}

void TGoalWatermelon::control()
{
	TMapObjBase::control();
	switch (mState) {
	case 0:
	case 1:
		break;
	case 2:
		if (unk13C->animIsFinished()) {
			gpItemManager->makeShineAppearWithDemoOffset(
			    "シャイン（お化けスイカ用）", "スイカゴールカメラ", 0.0f, 0.0f,
			    0.0f);
			mState = 3;
		}
		break;
	case 3:
		break;
	default:
		break;
	}
}

void TGoalWatermelon::touchActor(THitActor* actor)
{
	bool inState1;
	if (mState == 1)
		inState1 = true;
	else
		inState1 = false;
	if (!inState1)
		return;

	bool isWatermelon;
	if ((actor->mActorType - 0x40000000) == 0xD0)
		isWatermelon = true;
	else
		isWatermelon = false;
	if (!isWatermelon)
		return;

	unk13C = (TMapObjBall*)actor;
	getMActorInline(unk13C)->setBck("watermelon_shrink");
	unk13C->unkF8 &= ~0x100;
	unk13C->mVelocity = JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f);
	TMarDirector* director = gpMarDirector;
	JDrama::TFlagT<u16> flag(0);
	director->fireStartDemoCamera("スイカゴールカメラ", &unk13C->mPosition, -1,
	                              0.0f, true, 0, 0, nullptr, flag);
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

bool TSandBird::nameIsObj(const char* name)
{
	if (strstr(name, "none") == 0)
		return true;
	else
		return false;
}

TMapObjBase* TSandBird::makeObjFromJointName(const char* name, u16 joint_id)
{
	TMapObjBase* obj = TJointCoin::makeObjFromJointName(name, joint_id);
	if (obj)
		return obj;

	if (!strstr(name, "none"))
		return makeObj("SandBirdBlock", joint_id);

	return 0;
}

void TSandBird::control()
{
	TJointCoin::control();

	if (gpMSound->gateCheck(0x217C)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x217C, &mPosition, 0, nullptr,
		                                          0, 4);
	}
	if (gpMSound->gateCheck(0x217D)) {
		MSoundSESystem::MSoundSE::startSoundSystemSE(0x217D, 0, nullptr, 0);
	}

	for (int i = 0; i < unk13C; ++i) {
		TMapObjBase* block = unk140[i];
		u32 type           = block->mActorType;
		bool isTarget;
		if ((type - 0x20000000) == 0xE)
			isTarget = true;
		else if ((type - 0x40000000) == 0x23)
			isTarget = true;
		else
			isTarget = false;

		if (isTarget) {
			gpMarioParticleManager->emitAndBindToPosPtr(0x159, &block->mPosition,
			                                            1, nullptr);
			gpMarioParticleManager->emitAndBindToPosPtr(
			    0x15A, &unk140[i]->mPosition, 1, nullptr);
		}
	}

	bool camActive;
	if (gpCamera->isSimpleDemoCamera())
		camActive = true;
	else
		camActive = gpCamera->mMode == 0x49 ? true : false;

	if (!camActive && !unk150) {
		const TBGCheckData* plane = *gpMarioGroundPlane;
		const TLiveActor* actor   = plane->getActor();
		if (actor) {
			bool isType;
			if ((actor->mActorType - 0x40000000) == 0x2C9)
				isType = true;
			else
				isType = false;
			if (isType) {
				gpMarDirector->mConsole->startAppearBalloon(0xE002F, false);
				mLifeTimer = 0x960;
				unk150     = 1;
			}
		}
	}

	if (!unk151 && unk150) {
		if (!isLifeTimerActive()) {
			gpMarDirector->mConsole->startDisappearBalloon(0xE002F, false);
			unk151 = 1;
		}
	}
}

void TMammaYacht::initMapObj()
{
	TMapObjBase::initMapObj();
	unk138 = new TMapObjFlag("旗");
	unk138->mPosition.set(mPosition.x + 2.0f,
	                      mPosition.y + 1315.0f - 190.0f,
	                      mPosition.z - 15.0f);
	unk138->mRotation.set(0.0f, 180.0f, 0.0f);
	unk138->mScaling.set(1.0f, 2.5f, 3.8f);
	unk138->init("MammaYacht00");
}

void TMammaYacht::control()
{
	TMapObjBase::control();
	u16 attr = mGroundPlane->getBGType();
	bool onWater;
	if (attr == 0x100 || attr == 0x101 || (u16)(attr - 0x102) <= 3
	    || attr == 0x4104)
		onWater = true;
	else
		onWater = false;
	if (onWater) {
		mPosition.y = mInitialPosition.y
		              + gpMapObjWave->getWaveHeight(mPosition.x, mPosition.z);
		unk138->mPosition.y = mPosition.y - 50.0f;
	}
}

TMammaBlockRotate::TMammaBlockRotate(const char* name)
    : TMapObjBase(name)
    , unk13C(0)
    , unk140(0)
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

	unk138 = gpMap->getModelManager()->getJointModel(0);
	unk13C = unk138->getChild(0)->getChild(0)->getChild(0)->getChild(1);

	JGeometry::TVec3<f32> vec;

	J3DJoint* joint13C = unk13C->getJoint();
	f32 height13C      = joint13C->getMax().y - joint13C->getMin().y;
	moveJoint(joint13C, 0.0f, height13C, 0.0f);

	vec.set(0.0f, height13C, 0.0f);
	unk144->setUp();
	unk144->moveTrans(vec);

	unk140 = unk138->getChild(0)->getChild(0)->getChild(0)->getChild(2);

	J3DJoint* joint140 = unk140->getJoint();
	f32 height140      = joint140->getMax().y - joint140->getMin().y;
	moveJoint(joint140, 0.0f, joint140->getMax().y - joint140->getMin().y,
	          0.0f);

	unk148->setUp();
	vec.set(0.0f, height140, 0.0f);
	unk148->moveTrans(vec);

	unk138->getModel()->calc();
}

void TMammaBlockRotate::control()
{
	TMapObjBase::control();
	JGeometry::TVec3<f32> vec;
	switch (mState) {
	case 1: {
		if (mRotation.y > 0.0f)
			mRotation.y -= mRotReturnSpeed;
		else
			mRotation.y = 0.0f;
		break;
	}
	case 2: {
		moveJoint(unk140->getJoint(), 0.0f, -mMapGoSpeed, 0.0f);
		moveJoint(unk13C->getJoint(), 0.0f, -mMapGoSpeed, 0.0f);

		J3DTransformInfo& info = unk140->getJoint()->getTransformInfo();
		unk138->getModel()->calc();

		vec.set(0.0f, info.mTranslate.y, 0.0f);
		unk144->moveTrans(vec);
		vec.set(0.0f, info.mTranslate.y, 0.0f);
		unk148->moveTrans(vec);

		if (info.mTranslate.y < 0.0f) {
			mLifeTimer = mWaitTime;
			mState     = 3;
		}
		break;
	}
	case 3: {
		if (!isLifeTimerActive())
			mState = 4;
		break;
	}
	case 4: {
		moveJoint(unk140->getJoint(), 0.0f, mMapBackSpeed, 0.0f);
		moveJoint(unk13C->getJoint(), 0.0f, mMapBackSpeed, 0.0f);

		J3DTransformInfo& info = unk140->getJoint()->getTransformInfo();
		vec.set(0.0f, info.mTranslate.y, 0.0f);
		unk144->moveTrans(vec);
		vec.set(0.0f, info.mTranslate.y, 0.0f);
		unk148->moveTrans(vec);

		unk138->getModel()->calc();

		if (info.mTranslate.y
		    > unk140->getJoint()->getMax().y - unk140->getJoint()->getMin().y)
			mState = 1;
		break;
	}
	default:
		break;
	}
}

u32 TMammaBlockRotate::touchWater(THitActor*)
{
	bool inState1 = mState == 1 ? true : false;
	if (inState1) {
		mRotation.y += mRotSpeed;
		if (mRotation.y > mRotEnd)
			mState = 2;
	}
	return 1;
}

TShiningStone::TShiningStone(const char* name)
    : THitActor(name)
    , unk74(0)
    , unk78(0)
    , unk7C(0.0f)
{
	unk70 = 0;
	unk71 = 0;
	unk72 = 0;
	unk73 = 0;
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
{
	unk158.x = 0.0f;
	unk158.y = 0.0f;
	unk158.z = 0.0f;
	unk164.x = 0.0f;
	unk164.y = 0.0f;
	unk164.z = 0.0f;
	unk170 = 0.0f;
	unk17C = 0;
	unk198 = 0.0f;
	unk19C = 0;
	unk1AC = 0;
	unk1AE = 0;
	zeroVec(unk140);
	zeroVec(unk14C);
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
		f32 volume1 = __fabsf(vecLength(unk14C));
		if (gpMSound->gateCheck(0x3048)) {
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(
			    0x3048, &mPosition, nullptr, volume1, 0, 0, nullptr, 0, 4);
		}
		break;
	case 2:
		controlGoTarget();
		f32 volume2 = __fabsf(vecLength(unk14C));
		if (gpMSound->gateCheck(0x304A)) {
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(
			    0x304A, &mPosition, nullptr, volume2, 0, 0, nullptr, 0, 4);
		}
		break;
	case 3:
		if (!(mLifeTimer > 0 ? true : false)) {
			TShiningStone* stone = unk17C;
			if (stone->unk74 < 3)
				MSBgm::setTrackVolume(0, 1.0f, 10, 0);

			if (stone->unk7C > 0.0f)
				stone->unk78->mChildSpawnRate = stone->unk7C;

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
	JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > rotMtx;
	rotMtx.identity();
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

		TMarDirector* director = gpMarDirector;
		JDrama::TFlagT<u16> flag(0);
		director->fireStartDemoCamera(
		    "demohanatyan_cam01", nullptr, -1, 0.0f, true,
		    startCameraShakeSE, (u32)&mPosition, nullptr, flag);
	} else {
		TMarDirector* director = gpMarDirector;
		JDrama::TFlagT<u16> flag(0);
		director->fireStartDemoCamera("太陽石点灯カメラ",
		                              &unk17C->mPosition, mDemoLightTime, 0.0f,
		                              true, nullptr, 0, nullptr, flag);
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
	((TSandLeaf*)unk144)->unk138 = this;
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

		JGeometry::TVec3<f32>* cameraPos
		    = (JGeometry::TVec3<f32>*)((u8*)gpCamera + 0x124);
		JGeometry::TVec3<f32>* at
		    = (JGeometry::TVec3<f32>*)((u8*)gpCamera + 0x148);
		s16 angle = matan(cameraPos->z - at->z, cameraPos->x - at->x);
		gpCamera->warpPosAndAt(gpCamera->mCurrentTarget.unk28, angle);
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

	mMapCollisionManager->changeCollision(1);
	mMapCollisionManager->unk8->setUp();
	if (mMapCollisionManager->unk8)
		mMapCollisionManager->unk8->moveSRT(mPosition, mRotation, mScaling);

	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emit(0x55, &mPosition, 0, nullptr);
	JGeometry::TVec3<f32> scale(unk14C, unk14C, unk14C);
	emitter->setScale(scale);

	if (!gpMarDirector->checkUnk124Thing2())
		gpCameraShake->startShake((EnumCamShakeMode)0xD, 1.0f);

	if (gpMSound->gateCheck(0x28A4)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x28A4, &mPosition, 0,
		                                          nullptr, 0, 4);
	}

	SMSRumbleMgr->start(0x15, mExlodingRumbleTime, (Vec*)&mPosition);
	mState = 7;
	awake();

	unk158->appear();

	startControlAnim(3);
}

void TSandCastle::expanded()
{
	TMapObjBase* trigger = unk144;
	f32 speed           = unk150;
	f32 frame           = trigger->getMActor()->getFrameCtrl(0)->getFrame();
	trigger->getMActor()->getFrameCtrl(0)->setFrame(frame + speed);

	const JGeometry::TVec3<f32>* pos = &unk144->mPosition;
	if (gpMSound->gateCheck(0x20C6)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x20C6, pos, 0, nullptr, 0,
		                                          4);
	}

	if (unk144->animIsFinished())
		mState = 2;

	if (unk144->animIsFinished()) {
		mState = 2;
		startControlAnim(2);
		startControlAnim(3);
	}
}

BOOL TSandCastle::withering()
{
	f32 speed = unk13C;
	f32 frame0 = getMActorInline(this)->getFrameCtrl(0)->getFrame();
	getMActorInline(this)->getFrameCtrl(0)->setFrame(frame0 + speed);
	speed = unk13C;
	f32 frame5 = getMActorInline(this)->getFrameCtrl(5)->getFrame();
	getMActorInline(this)->getFrameCtrl(5)->setFrame(frame5 + speed);

	f32 current = getMActorInline(this)->getFrameCtrl(0)->getFrame();
	f32 end     = (f32)getMActorInline(this)->getFrameCtrl(0)->getEnd();
	mScaling.y  = mCollisionRate * ((end - current) / end);

	if (current > 240.0f && !unk158->checkLiveFlag(1)) {
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
	unk13C = 0.005f;
	unk140 = 0;
	unk148 = 60;
	unk154 = 1000.0f;
	mScaling.y = mScaleMin;
	TMapObjBase::initMapObj();
	unk150 = 0.5f;

	if (strcmp(unkF4, "SandBombBasePyramid") == 0) {
		unk14C = 1.3f;
		unk154 = 1200.0f;
	} else if (strcmp(unkF4, "SandBombBaseShit") == 0) {
		unk14C = 1.3f;
		unk154 = 1500.0f;
	} else if (strcmp(unkF4, "SandBombBaseStar") == 0) {
		unk14C = 1.2f;
	} else if (strcmp(unkF4, "SandBombBaseTurtle") == 0) {
		unk14C = 1.2f;
	}

	if (!gParticleFlagLoaded[0x55]) {
		gpResourceManager->load("/scene/mapObj/SandBomb.jpa", 0x55);
		gParticleFlagLoaded[0x55] = true;
	}
}

void TSandBombBase::loadAfter()
{
	unk144 = findTriggerActor();
	((TSandLeaf*)unk144)->unk138 = this;
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
	TMapObjBase* trigger = unk144;
	switch (mState) {
	case 1: {
		f32 newFrame
		    = getMActorInline(trigger)->getFrameCtrl(0)->getFrame()
		      - mFiringFrameDownSpeed;
		if (newFrame >= 0.0f) {
			getMActorInline(unk144)->getFrameCtrl(0)->setFrame(newFrame);
			getMActorInline(unk144)->getFrameCtrl(5)->setFrame(newFrame);
		}
		break;
	}
	case 5: {
		f32 explodeFrameSpeed = mExplodeFrameSpeed;
		f32 frame0
		    = getMActorInline(trigger)->getFrameCtrl(0)->getFrame();
		getMActorInline(trigger)->getFrameCtrl(0)->setFrame(
		    frame0 + explodeFrameSpeed);
		explodeFrameSpeed = mExplodeFrameSpeed;
		TMapObjBase* trigger5 = unk144;
		f32 frame5
		    = getMActorInline(trigger5)->getFrameCtrl(5)->getFrame();
		getMActorInline(trigger5)->getFrameCtrl(5)->setFrame(
		    frame5 + explodeFrameSpeed);
		explodeFrameSpeed = mExplodeFrameSpeed;
		TMapObjBase* trigger3 = unk144;
		f32 frame3
		    = getMActorInline(trigger3)->getFrameCtrl(3)->getFrame();
		getMActorInline(trigger3)->getFrameCtrl(3)->setFrame(
		    frame3 + explodeFrameSpeed);
		if (unk144->animIsFinished())
			waitBeforeExplode();
		break;
	}
	case 6: {
		if (!isLifeTimerActive())
			explode();
		break;
	}
	case 7:
		exploding();
		break;
	case 8:
		expanded();
		break;
	case 2: {
		SMSRumbleMgr->start(0x13, -1, (Vec*)&mPosition);
		if ((u8)withering()) {
			withered();
			SMSRumbleMgr->stop(0x13);
		}
		break;
	}
	case 3: {
		if (!isLifeTimerActive()) {
			mState = 1;
			unk144->awake();
			unk144->startControlAnim(1);
			unk144->startControlAnim(2);
		}
		break;
	}
	default:
		break;
	}

	if (unk144->mColCount == 0)
		((TSandBomb*)trigger)->unk140 = 0;
}

BOOL TSandBombBase::grow()
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

	mMapCollisionManager->changeCollision(1);
	mMapCollisionManager->unk8->setUp();
	if (mMapCollisionManager->unk8)
		mMapCollisionManager->unk8->moveSRT(mPosition, mRotation, mScaling);

	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emit(0x55, &mPosition, 0, nullptr);
	JGeometry::TVec3<f32> scale(unk14C, unk14C, unk14C);
	emitter->setScale(scale);

	if (!gpMarDirector->checkUnk124Thing2())
		gpCameraShake->startShake((EnumCamShakeMode)0xD, 1.0f);

	if (gpMSound->gateCheck(0x28A4)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x28A4, &mPosition, 0,
		                                          nullptr, 0, 4);
	}

	SMSRumbleMgr->start(0x15, mExlodingRumbleTime, (Vec*)&mPosition);
	mState = 7;
}

void TSandBombBase::exploding()
{
	f32 explodeFrameSpeed = mExplodeFrameSpeed;
	f32 frame0 = getMActorInline(this)->getFrameCtrl(0)->getFrame();
	getMActorInline(this)->getFrameCtrl(0)->setFrame(
	    frame0 + explodeFrameSpeed);
	explodeFrameSpeed = mExplodeFrameSpeed;
	TMapObjBase* trigger = unk144;
	frame0 = getMActorInline(trigger)->getFrameCtrl(0)->getFrame();
	getMActorInline(trigger)->getFrameCtrl(0)->setFrame(
	    frame0 + explodeFrameSpeed);

	f32 dist = getDistanceXZ(*gpMarioPos);

	bool isType;
	if ((mActorType - 0x40000000) == 0xCE)
		isType = true;
	else
		isType = false;

	bool skipThrow;
	if (isType)
		skipThrow = true;
	else
		skipThrow = false;

	if (!skipThrow) {
		if (getMActorInline(this)->getFrameCtrl(0)->getFrame() < 80.0f) {
			f32 grLevel = SMS_GetMarioGrLevel();
			if (grLevel > gpMarioPos->y - 30.0f) {
				if (dist < unk154) {
					SMS_SendMessageToMario(this, 7);
					JGeometry::TVec3<f32> vec(0.0f, 1.0f, 0.0f);
					SMS_ThrowMario(vec, mMarioJumpRate * (unk154 - dist));
				}
			}
		}
	}

	if (animIsFinished()) {
		unk144->startControlAnim(6);
		mState = 8;
	}
}

void TSandBombBase::expanded()
{
	TMapObjBase* trigger = unk144;
	f32 speed           = unk150;
	f32 frame           = getMActorInline(trigger)->getFrameCtrl(0)->getFrame();
	getMActorInline(trigger)->getFrameCtrl(0)->setFrame(
	    frame + speed);

	const JGeometry::TVec3<f32>* pos = &unk144->mPosition;
	if (gpMSound->gateCheck(0x20C6)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x20C6, pos, 0, nullptr, 0,
		                                          4);
	}

	if (unk144->animIsFinished())
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
	f32 firingFrameSpeed = TSandBombBase::mFiringFrameSpeed;
	f32 frame0 = getMActorInline(this)->getFrameCtrl(0)->getFrame();
	getMActorInline(this)->getFrameCtrl(0)->setFrame(frame0 + firingFrameSpeed);
	firingFrameSpeed = TSandBombBase::mFiringFrameSpeed;
	f32 frame5 = getMActorInline(this)->getFrameCtrl(5)->getFrame();
	getMActorInline(this)->getFrameCtrl(5)->setFrame(frame5 + firingFrameSpeed);
	getMActorInline(this)->getFrameCtrl(0);

	soundBas(0x289A, 7.0f, TSandBombBase::mFiringFrameSpeed);
	soundBas(0x289B, 50.0f, TSandBombBase::mFiringFrameSpeed);
	soundBas(0x289C, 100.0f, TSandBombBase::mFiringFrameSpeed);
	soundBas(0x289D, 150.0f, TSandBombBase::mFiringFrameSpeed);

	if (getMActorInline(this)->curAnmEndsNext(0, 0)) {
		unk138->grow();
		startControlAnim(3);
		startControlAnim(4);
		startControlAnim(5);
		unk64 |= 1;
	}
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
	unk144 = TMapObjBaseManager::newAndRegisterObj("SandLeaf", mPosition,
	                                              mRotation,
	                                              JGeometry::TVec3<f32>(
	                                                  1.0f, 1.0f, 1.0f));
	((TSandLeaf*)unk144)->unk138 = this;
	unk144->appear();
}

void TSandLeafBase::control()
{
	TMapObjBase::control();
	switch (mState) {
	case 1:
		break;
	case 2: {
		SMSRumbleMgr->start(0x13, -1, (Vec*)&mPosition);
		if ((u8)withering()) {
			SMSRumbleMgr->stop(0x13);
			mMapCollisionManager->changeCollision(0);
			TMapCollisionManager* mgr = mMapCollisionManager;
			Mtx mtx;
			MsMtxSetTRS(mtx, mPosition.x, mPosition.y, mPosition.z,
			            mRotation.x, mRotation.y, mRotation.z, mScaling.x,
			            mScaling.y, mScaling.z);
			TMapCollisionBase* col = mgr->unk8;
			PSMTXCopy(mtx, col->unk20);
			col->setUp();
			mLifeTimer = unk140;
			mState     = 3;
		}
		break;
	}
	case 3: {
		if (!isLifeTimerActive() && unk144->animIsFinished()) {
			unk144->awake();
			unk144->startAnim(1);
			const Vec* soundPos = (Vec*)&unk144->mPosition;
			if (gpMSound->gateCheck(0x3802)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x3802, soundPos, 0, nullptr, 0, 4);
			}
			mState = 5;
		}
		break;
	}
	case 5: {
		if (unk144->animIsFinished()) {
			unk144->startAnim(0);
			mState = 1;
		}
		break;
	}
	default:
		break;
	}
}

BOOL TSandLeafBase::grow()
{
	if (mState != 1 && mState != 4)
		return;

	if (mScaling.y < 1.0f) {
		mScaling.y += unk138;
		if (mScaling.y > 1.0f)
			mScaling.y = 1.0f;
	}

	if (mState == 1) {
		mMapCollisionManager->changeCollision(1);
		TMapCollisionManager* mgr = mMapCollisionManager;
		Mtx mtx;
		MsMtxSetTRS(mtx, mPosition.x, mPosition.y, mPosition.z, mRotation.x,
		            mRotation.y, mRotation.z, mScaling.x, mScaling.y,
		            mScaling.z);
		TMapCollisionBase* col = mgr->unk8;
		PSMTXCopy(mtx, col->unk20);
		col->setUp();
		unk144->startControlAnim(2);
		mState = 4;
	}

	f32 rate = SMSGetAnmFrameRate();
	TMapObjBase* leaf = unk144;
	getMActorInline(leaf)->getFrameCtrl(0)->setFrame(
	    rate + getMActorInline(leaf)->getFrameCtrl(0)->getFrame());
	SMSRumbleMgr->start(0x15, 5, (Vec*)&mPosition);
	const Vec* soundPos = (Vec*)&unk144->mPosition;
	if (gpMSound->gateCheck(0x2099)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x2099, soundPos, 0, nullptr,
		                                          0, 4);
	}
	mLifeTimer = mWitherTime;
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

	const JGeometry::TVec3<f32>* pos = &unk144->getPosition();
	if (gpMSound->gateCheck(0x2099)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x2099, pos, 0, nullptr, 0, 4);
	}

	BOOL result;
	if (mScaling.y <= mScaleMin)
		result = 1;
	else
		result = 0;
	return result;
}

void TSandLeaf::control()
{
	TMapObjBase::control();
	mGroundHeight = gpMap->checkGround(mPosition.x, mPosition.y + 50.0f,
	                                   mPosition.z, &mGroundPlane);
	mPosition.y = mGroundHeight;
}

u32 TSandLeaf::touchWater(THitActor*)
{
	unk138->grow();
	return 1;
}
