#include "MoveBG/MapObjCorona.hpp"
#include "MoveBG/MapObjBase.hpp"

#include <Camera/CameraShake.hpp>
#include <Enemy/Koopa.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JMath.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <Player/MarioAccess.hpp>
#include <System/Particles.hpp>

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
{
}

void TBathtub::load(JSUMemoryInputStream&) { }

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
