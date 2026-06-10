#include "MoveBG/MapObjCorona.hpp"
#include "MoveBG/MapObjBase.hpp"

#include <Camera/CameraShake.hpp>
#include <Enemy/Koopa.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorData.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <Player/MarioAccess.hpp>
#include <System/Particles.hpp>
#include <math.h>
#include <stdio.h>

// rogue includes for matching __sinit (15 JALList<T> templates)
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

static inline s32 getParamS32(const TBathtubParams* params, u32 offset)
{
	return *(const s32*)((const u8*)params + offset);
}

static inline bool isGripDead(TBathtubGrip* grip)
{
	return *((u8*)grip + 0x249) == 0;
}

static inline TKoopa* getKoopa()
{
	return JDrama::TNameRefGen::search<TKoopa>("クッパ");
}

static inline bool canUseKiller(const TBathtub* bathtub)
{
	if (bathtub->unk29A != 0)
		return false;

	TKoopa* koopa = getKoopa();
	if (!koopa->allowsLaunch())
		return false;

	return bathtub->unk248 == 0;
}

static inline MtxPtr getJointMtx(TBathtub* bathtub, s32 index)
{
	return bathtub->mMActor->unk4->getAnmMtx(index);
}

// ============= TBathtubParams =============

TBathtubParams::TBathtubParams()
    : TParams("/MapObj/bathtub.prm")
    , PARAM_INIT(resetGrip, 0)
    , PARAM_INIT(trampleRelease, 10)
    , PARAM_INIT(trampleRecover, 10)
    , PARAM_INIT(quakeRelease, 500)
    , PARAM_INIT(quakeRecover, 500)
    , PARAM_INIT(hipdropRelease, 35)
    , PARAM_INIT(hipdropRecover, 35)
    , PARAM_INIT(breakCount0, 750)
    , PARAM_INIT(breakCount1, 710)
    , PARAM_INIT(breakCount2, 685)
    , PARAM_INIT(breakCount3, 655)
    , PARAM_INIT(launchStopCount, 1000)
    , PARAM_INIT(animSpeed0, 0.15f)
    , PARAM_INIT(animSpeed1, 0.15f)
    , PARAM_INIT(animSpeed2, 0.15f)
    , PARAM_INIT(animSpeed3, 0.15f)
    , PARAM_INIT(animSpeed4, 0.22f)
    , PARAM_INIT(shake, 0.0f)
    , PARAM_INIT(watermark, 0.3f)
    , PARAM_INIT(maxAngle, 35.0f)
    , PARAM_INIT(angleVelDamp, 0.93f)
    , PARAM_INIT(rebound, 0.0005f)
    , PARAM_INIT(shakeDamp, 0.93f)
    , PARAM_INIT(marioWeight, 0.01f)
    , PARAM_INIT(marioDropWeight, 5.0f)
    , PARAM_INIT(outerHeight, 20.0f)
{
	TParams::load(mPrmPath);
}

Mtx* TBathtubGripParts::getRootJointMtx() const
{
	return (Mtx*)unkF4->getModel()->getAnmMtx(unkF4->unk200[unkF8]);
}

BOOL TBathtubGripPartsFragile::receiveMessage(THitActor* sender, u32 message)
{
	return unkF4->receiveMessage(sender, message);
}

BOOL TBathtubGripPartsHard::receiveMessage(THitActor* sender, u32 message)
{
	if (message == HIT_MESSAGE_UNK3)
		message = HIT_MESSAGE_HIP_DROP;
	return unkF4->receiveMessage(sender, message);
}

void TBathtubGrip::kill()
{
	unk24A = 1;
	calcRootMatrix();

	for (s32 i = 0; i < 17; ++i)
		unk164[i]->remove();
	for (s32 i = 0; i < 5; ++i)
		unk150[i]->remove();
}

TBathtubGrip::TBathtubGrip(TBathtub* bathtub, f32 angle, MActorAnmData* data,
                           const char* name)
    : TMapObjBase(name)
    , unk244(bathtub)
    , unk248(0)
    , unk249(1)
    , unk24A(0)
    , unk24B(0)
    , unk24C(angle)
    , unk250(1.0f)
    , unk254(0)
    , unk258(100)
    , unk260(0)
{
	unk25C = new MActor(data);

	void* res = JKRFileLoader::getGlbResource(
	    "/scene/map/map/stand_effect/stand_effect.bmd");
	J3DModelData* modelData = J3DModelLoaderDataBase::load(res, 0x50050000);
	J3DModel* model         = new J3DModel(modelData, 0, 1);
	unk25C->setModel(model, 0x50050000);

	initAndRegister("stand_break");
	calcRootMatrix();
	getModel()->calc();

	JUTNameTab* names = getModel()->getModelData()->getJointName();
	char jointName[256];
	char collisionPath[256];

	for (s32 i = 0; i < 17; ++i) {
		sprintf(jointName, "c%d", i + 1);
		sprintf(collisionPath, "/scene/mapObj/stand_break_%s.col", jointName);

		unk200[i] = names->getIndex(jointName);
		unk164[i] = new TMapCollisionMove;
		unk1BC[i] = new TBathtubGripPartsHard(
		    this, i, "バタブの足場の一部（壊れない）");
		unk164[i]->init(collisionPath, 0, unk1BC[i]);

		if (i < 5) {
			sprintf(jointName, "b%d", i + 1);
			sprintf(collisionPath, "/scene/mapObj/stand_break_%s.col",
			        jointName);

			unk150[i] = new TMapCollisionMove;
			unk1A8[i] = new TBathtubGripPartsFragile(
			    this, i, "バタブの足場の一部（弱点）");
			unk150[i]->init(collisionPath, 0, unk1A8[i]);
		}
	}

	offLiveFlag(LIVE_FLAG_DEAD);
	unk248 = 0;
	unk24A = 0;
	unk249 = 1;
	unk24B = 0;
	startAnim(0);

	J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
	if (ctrl != nullptr) {
		ctrl->setFrame(0.0f);
		ctrl->setRate(0.0f);
	}

	unk250 = 1.0f;
	unk258 = 100;
	unk260 = 0;
}

void TBathtubGrip::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TMapObjBase::perform(flags, graphics);
	if (unk260 == 0)
		unk25C->perform(flags, graphics);
}

BOOL TBathtubGrip::receiveMessage(THitActor*, u32) { return false; }

Mtx* TBathtubGrip::getRootJointMtx() const
{
	return (Mtx*)getModel()->getBaseTRMtx();
}

void TBathtubGrip::calcRootMatrix() { TMapObjBase::calcRootMatrix(); }

void TBathtubGrip::control() { TMapObjBase::control(); }

void TBathtub::loadAfter()
{
	SMS_LoadParticle("/scene/map/map/ms_lkp_yuge1.jpa", 0x1BE);
	SMS_LoadParticle("/scene/map/map/ms_kp_funsui.jpa", 0x1BF);
	SMS_LoadParticle("/scene/map/map/ms_kp_break_a.jpa", 0xF6);
	SMS_LoadParticle("/scene/map/map/ms_kp_break_b.jpa", 0xF7);
}

void TBathtub::hipdrop(const JGeometry::TVec3<f32>& pos)
{
	if (unk29A != 0)
		return;

	if (unk250 > getParamS32(unk16C, 0x7C))
		return;

	JGeometry::TVec3<f32> diff;
	diff.x = pos.x - mInitialPosition.x;
	diff.y = 0.0f;
	diff.z = pos.z - mInitialPosition.z;
	diff.normalize(diff);

	unk250 = getParamS32(unk16C, 0x7C);
	unk258 = getParamS32(unk16C, 0x90);
	unk25C = getParamS32(unk16C, 0x90);
	unk254 = getParamS32(unk16C, 0x7C);

	getKoopa()->stagger(false);
}

void TBathtub::quake(const JGeometry::TVec3<f32>& pos)
{
	if (unk29A != 0)
		return;

	JGeometry::TVec3<f32> diff;
	diff.x = pos.x - mInitialPosition.x;
	diff.y = 0.0f;
	diff.z = pos.z - mInitialPosition.z;
	diff.normalize(diff);

	unk24C = 300;
	unk250 = getParamS32(unk16C, 0x54);
	unk258 = getParamS32(unk16C, 0x68);
	unk25C = getParamS32(unk16C, 0x68);
	unk254 = getParamS32(unk16C, 0x7C);
	unk248 = getParamS32(unk16C, 0xF4);

	TKoopa* koopa = getKoopa();
	gpCameraShake->startShake((EnumCamShakeMode)0x25, 1.0f);
	gpCameraShake->startShake((EnumCamShakeMode)0x26, 1.0f);
	SMSRumbleMgr->start(4, (f32*)nullptr);

	JGeometry::TVec3<f32> throwDir;
	throwDir.x = 0.0f;
	throwDir.y = 1.0f;
	throwDir.z = 0.0f;
	SMS_ThrowMario(throwDir, 60.0f);
	koopa->getDown();
}

s32 TBathtub::getNumGripsDead() const
{
	s32 count = 0;
	if (isGripDead(unk168[0]))
		++count;
	if (isGripDead(unk168[1]))
		++count;
	if (isGripDead(unk168[2]))
		++count;
	if (isGripDead(unk168[3]))
		++count;
	if (isGripDead(unk168[4]))
		++count;
	return count;
}

void TBathtub::tumble(f32 angle, f32 power)
{
	if (unk29A != 0)
		return;

	s16 shortAngle = angle * (65536.0f / 360.0f);
	f32 scaled     = power * 0.1f;
	unk1E8.x += scaled * JMASCos(shortAngle);
	unk1E8.y += 0.0f;
	unk1E8.z += scaled * -JMASSin(shortAngle);
}

MtxPtr TBathtub::getTakingMtx() { return getJointMtx(this, unk260); }

MtxPtr TBathtub::getSubmarineMtxInDemo() { return getJointMtx(this, unk26C); }

MtxPtr TBathtub::getPeachMtxInDemo() { return getJointMtx(this, unk270); }

MtxPtr TBathtub::getKoopaJrMtxInDemo() { return getJointMtx(this, unk274); }

BOOL TBathtub::receiveMessage(THitActor* sender, u32 message)
{
	switch (message) {
	case HIT_MESSAGE_TRAMPLE:
		if (unk29A == 0 && unk250 <= getParamS32(unk16C, 0x2C)) {
			unk250 = getParamS32(unk16C, 0x2C);
			unk258 = getParamS32(unk16C, 0x40);
			unk25C = getParamS32(unk16C, 0x40);
			unk254 = getParamS32(unk16C, 0x7C);
		}
		return true;
	case HIT_MESSAGE_HIP_DROP:
	case HIT_MESSAGE_UNK3:
		hipdrop(*gpMarioPos);
		return true;
	default:
		return false;
	}
}

Mtx* TBathtub::getRootJointMtx() const
{
	J3DModel* model = mMActor->unk4;
	if (unk29A != 0)
		return (Mtx*)model->getAnmMtx(0);
	return (Mtx*)model->getBaseTRMtx();
}

void TBathtub::perform(u32, JDrama::TGraphics*) { }

void TBathtub::control() { }

void TBathtub::calcBathtubData() { }

void TBathtub::setupCollisions_() { }

void TBathtub::removeCollisions_() { } // Unused

void TBathtub::startDemo() { }

bool TBathtub::allowsTumble() const { return false; }

void TBathtub::calcRootMatrix() { }

bool TBathtub::getNearGrip(const JGeometry::TVec3<f32>&, f32, f32*) const
{
	return false;
}

u8 TBathtub::getNextJuncture(const JGeometry::TVec3<f32>&,
                             const JGeometry::TVec3<f32>&) const
{
	return 0;
}

u8 TBathtub::getNextGrip(const JGeometry::TVec3<f32>&,
                         const JGeometry::TVec3<f32>&, f32, f32*) const
{
	return 0;
}

void TBathtub::updatePosture_() { }

TBathtub::TBathtub(const char* name)
    : TMapObjBase(name)
    , unk164(nullptr)
    , unk290(nullptr)
{
	unk16C = new TBathtubParams;

	unk1D8.x = 0.0f;
	unk1D8.y = 0.0f;
	unk1D8.z = 0.0f;
	unk1E4   = 1.0f;

	mPosition.z = 0.0f;
	mPosition.y = 0.0f;
	mPosition.x = 0.0f;

	unk1E8.z = 0.0f;
	unk1E8.y = 0.0f;
	unk1E8.x = 0.0f;

	unk250 = 0;
	unk254 = 1;
	unk258 = 0;
	unk25C = 1;
	unk248 = 0;
	unk298 = 0;

	unk23C.z = 0.0f;
	unk23C.y = 0.0f;
	unk23C.x = 0.0f;

	unk299 = 0;
	unk29A = 0;
	unk2A0 = nullptr;
	unk294 = nullptr;
}

void TBathtub::load(JSUMemoryInputStream& stream)
{
	unk24C = 0;
	TMapObjBase::load(stream);

	mPosition.x = mInitialPosition.x;
	mPosition.y = mInitialPosition.y;
	mPosition.z = mInitialPosition.z;

	unk164 = new TMapCollisionMove*[30];
	for (s32 i = 0; i < 30; ++i) {
		unk164[i] = new TMapCollisionMove;
		const char* path = nullptr;
		switch (i % 6) {
		case 0:
			path = "/scene/mapObj/bath_col_inside3.col";
			break;
		case 1:
			path = "/scene/mapObj/bath_col_inside2.col";
			break;
		case 2:
			path = "/scene/mapObj/bath_col_inside1.col";
			break;
		case 3:
			path = "/scene/mapObj/bath_col_inside6.col";
			break;
		case 4:
			path = "/scene/mapObj/bath_col_inside5.col";
			break;
		case 5:
			path = "/scene/mapObj/bath_col_inside4.col";
			break;
		}
		unk164[i]->init(path, 0, this);
		((TMapCollisionBase*)unk164[i])->remove();
	}

	unk170.x = mInitialPosition.x;
	unk170.y = mInitialPosition.y;
	unk170.z = mInitialPosition.z;

	unk188[7] = 0.0f;
	unk188[6] = 0.0f;
	unk188[5] = 0.0f;
	unk188[3] = 0.0f;
	unk188[2] = 0.0f;
	unk188[1] = 0.0f;
	unk188[8] = 1.0f;
	unk188[4] = 1.0f;
	unk188[0] = 1.0f;

	unk1AC = 3000.0f;
	unk1B0 = 3600.0f;
	unk1B4 = unk1AC * sinf(0.27925268f);

	unk1BC.z = 0.0f;
	unk1BC.y = 0.0f;
	unk1BC.x = 0.0f;
	unk1C8.z = 0.0f;
	unk1C8.y = 0.0f;
	unk1C8.x = 0.0f;
	unk1B8   = 100.0f;
	unk1D4   = 0;

	unk17C.x = 0.0f;
	unk17C.y = 1.0f;
	unk17C.z = 0.0f;

	unk168 = new TBathtubGrip*[5];
	unk138 = new MActorAnmData;
	unk138->init("scene/map/map/stand_effect", nullptr);
}

s32 TBathtub::getNumKillerLaunchable() const
{
	if (!canUseKiller(this))
		return 0;

	s32 count = getNumGripsDead() + 1;
	if (count < 2)
		count = 2;
	if (count > 4)
		count = 4;
	return count;
}

bool TBathtub::isKillerAttackable() const { return unk248 == 0; }

s32 TBathtub::getNumKillerBurstable() const
{
	if (!canUseKiller(this))
		return 0;

	s32 count = getNumGripsDead();
	if (count >= 4)
		return 8;

	if (allowsTumble())
		return 0;

	if (unk250 != 0 || unk258 != 0)
		return 0;

	switch (count) {
	case 1:
		return 4;
	case 2:
		return 6;
	case 3:
		return 8;
	default:
		return 0;
	}
}

// Unused
bool TBathtub::isBreaking() const { return false; }

// Unused
bool TBathtub::isKillerLaunchable() const { return false; }

// Unused
void TBathtub::showMessage(u32) { }

// Unused
u8 TBathtub::getNearJuncture(const JGeometry::TVec3<f32>&) const { return 0; }

// Unused
MtxPtr TBathtub::getKoopaMtxInDemo() { return nullptr; }

// Unused
MtxPtr TBathtub::getWaterMtx(s32) { return nullptr; }

// Unused
MtxPtr TBathtub::getShineEffectMtx() { return nullptr; }

// Unused
MtxPtr TBathtub::getShineMtx() { return nullptr; }

// Unused
void TBathtub::liftMario(const JGeometry::TVec3<f32>&) { }

// Unused
void TBathtub::trample(const JGeometry::TVec3<f32>&) { }
