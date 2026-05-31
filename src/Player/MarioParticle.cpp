#include <Player/MarioMain.hpp>
#include <Player/MarioEffect.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <Map/MapData.hpp>
#include <MarioUtil/EffectUtil.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>

// rogue includes for matching __sinit (15 JALList<T> templates)
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

static const u32 cParticleIDs[] = { 0x50, 0x126, 0x12B };

const char* cParticleFileNames[] = {
	"/scene/map/pollution/ms_m_ashios.jpa",
	"/scene/map/pollution/ms_m_spinos.jpa",
	"/scene/map/pollution/ms_m_tokeos.jpa",
};

bool TMario::askJumpIntoWaterEffectExist() const
{
	TMarioEffect* eff = (TMarioEffect*)mMarioEffect;
	if (eff->unk6C[0] == 1)
		return true;
	if (eff->unk6C[1] == 1)
		return true;
	return false;
}

void TMario::sinkInSandEffect()
{
	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emit(0x53, &mPosition, 0, nullptr);
	if (emitter) {
		emitter->unk154.x = 0.0f;
		emitter->unk154.y = 0.0f;
		emitter->unk154.z = 0.0f;
		emitter->unk174.x = 0.0f;
		emitter->unk174.y = 0.0f;
		emitter->unk174.z = 0.0f;
	}
}

void TMario::toroccoEffect()
{
	JGeometry::TVec3<f32> dist(mPosition);
	dist.sub(mToroccoPos);
	f32 len = dist.length();

	JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x11F, mTorocco->getModel()->getAnmMtx(0), 1, this);
	if (emitter)
		emitter->mChildSpawnRate = len * mParticleParams.mToroccoWind.value;

	emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x120, mTorocco->getModel()->getAnmMtx(0), 1, this);
	if (emitter)
		emitter->mChildSpawnRate = len * mParticleParams.mToroccoSpark.value;
}

void TMario::sleepingEffect()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0x124, &mSleepPos, 1, this);
}

void TMario::sleepingEffectKill()
{
	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emitAndBindToPosPtr(0x124, &mSleepPos, 1, this);
	if (emitter)
		emitter->deleteAllParticle();
}

void TMario::kickRoofEffect()
{
	if (getMotionFrameCtrl().checkPass(8.0f)) {
		MtxPtr mtx = mModel->getModel()->getAnmMtx(mBoneIDs[6]);
		unk1A8.x   = mtx[0][3];
		unk1A8.y   = mtx[1][3];
		unk1A8.z   = mtx[2][3];
		gpMarioParticleManager->emit(0x39, &unk1A8, 0, nullptr);
		rumbleStart(0x15, mMotorParams.mMotorWall.value);
	}
}

void TMario::emitSandEffect() { emitFootPrintWithEffect(0x3b, 0x3a); }
void TMario::emitDirtyFootPrint() { emitFootPrintWithEffect(0x50, -1); }

void TMario::emitFootPrintWithEffect(int footprintID, int effectID)
{
	MtxPtr mtx = nullptr;
	int foot   = 2;

	if (mAction == ACTION_RUNNING) {
		if (onYoshi()) {
			if (mYoshi->getFrameCtrl()->checkPass(47.0f)) {
				mtx  = mYoshi->getMtxPtrFootL();
				foot = 0;
			}
			if (mYoshi->getFrameCtrl()->checkPass(16.0f)) {
				mtx  = mYoshi->getMtxPtrFootR();
				foot = 1;
			}
		} else {
			if (getMotionFrameCtrl().checkPass(38.0f)) {
				mtx  = mModel->getModel()->getAnmMtx(mBoneIDs[9]);
				foot = 0;
			}
			if (getMotionFrameCtrl().checkPass(8.0f)) {
				mtx  = mModel->getModel()->getAnmMtx(mBoneIDs[7]);
				foot = 1;
			}
		}
	}

	if (mAction == ACTION_IDLE && onYoshi()) {
		if (mYoshi->getFrameCtrl()->checkPass(20.0f)
		    || mYoshi->getFrameCtrl()->checkPass(71.0f)
		    || mYoshi->getFrameCtrl()->checkPass(134.0f)) {
			mtx  = mYoshi->getMtxPtrFootL();
			foot = 0;
		}
		if (mYoshi->getFrameCtrl()->checkPass(45.0f)
		    || mYoshi->getFrameCtrl()->checkPass(102.0f)
		    || mYoshi->getFrameCtrl()->checkPass(134.0f)) {
			mtx  = mYoshi->getMtxPtrFootR();
			foot = 1;
		}
	}

	if (mtx != nullptr && foot != 2) {
		unk1A8.x = mtx[0][3];
		unk1A8.y = mtx[1][3];
		unk1A8.z = mtx[2][3];

		if (mAction == ACTION_RUNNING && mForwardVel > 20.0f && effectID > 0)
			gpMarioParticleManager->emit(effectID, &unk1A8, 0, nullptr);

		if (footprintID > 0) {
			calcGroundMtx(unk1A8);
			gpMarioParticleManager->emitAndBindToMtx(
			    footprintID, mGroundMtx, 0, nullptr);
		}
	}
}

void TMario::emitBlurSpinJump()
{
	gpMarioParticleManager->emitAndBindToMtxPtr(0x105, getCenterAnmMtx(), 1, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(0x106, getCenterAnmMtx(), 1, this);
	if (unk134 > 0.0f) {
		JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x126, getCenterAnmMtx(), 1, this);
		if (emitter)
			emitter->mChildSpawnRate = 1.75f * unk134 * (1.0f / 255.0f);
	}
}

void TMario::emitBlurHipDropSuper()
{
	gpMarioParticleManager->emitAndBindToMtxPtr(0x11A, getCenterAnmMtx(), 1, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(0x11B, getCenterAnmMtx(), 1, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(0x11C, getCenterAnmMtx(), 1, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(0x11D, getCenterAnmMtx(), 1, this);
}

void TMario::emitBlurHipDrop()
{
	gpMarioParticleManager->emitAndBindToMtxPtr(0x104, getCenterAnmMtx(), 1, this);
}

void TMario::blurEffect()
{
	gpMarioParticleManager->emitAndBindToMtxPtr(0x10E, getCenterAnmMtx(), 1, this);
}

void TMario::warpInLight()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0x51, &unk160[2], 0, this);
}

void TMario::elecEndEffect()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0x8B, &unk160[2], 0, this);
}

void TMario::elecEffect()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0x116, &unk160[2], 1, this);
	gpMarioParticleManager->emitAndBindToPosPtr(0x118, &unk160[2], 1, this);
	gpMarioParticleManager->emitAndBindToPosPtr(0x117, &unk160[2], 1, this);
}

void TMario::emitRotateShootEffect()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0x114, &unk160[2], 1, this);
	gpMarioParticleManager->emitAndBindToPosPtr(0x115, &unk160[2], 1, this);
}

void TMario::rocketEffectStart()
{
	gpMarioParticleManager->emit(0x5, &mPosition, 0, nullptr);
	gpMarioParticleManager->emit(0x4, &mPosition, 0, nullptr);
	gpMarioParticleManager->emit(0x12, &mPosition, 0, nullptr);

	if (mWaterGun) {
		gpMarioParticleManager->emitAndBindToPosPtr(
		    0x1, (JGeometry::TVec3<f32>*)((u8*)mWaterGun + 0x1C90), 0,
		    this);
		gpMarioParticleManager->emitAndBindToPosPtr(
		    0x2, (JGeometry::TVec3<f32>*)((u8*)mWaterGun + 0x1C90), 0,
		    this);
		gpMarioParticleManager->emitAndBindToPosPtr(
		    0x3, (JGeometry::TVec3<f32>*)((u8*)mWaterGun + 0x1C90), 0,
		    this);
	}
}

void TMario::meltInWaterEffect()
{
	if (unk134 > 0.0f) {
		JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x12B, mJointMtx2, 1, this);
		if (emitter) {
			emitter->mChildSpawnRate = unk134
			    * mParticleParams.mMeltInWaterMax.value * (1.0f / 255.0f);
			if (checkFlag(MARIO_FLAG_IN_SHALLOW_WATER)) {
				emitter->unk154.setAll(0.6f);
				emitter->unk174.setAll(0.6f);
			}
		}
	}
}

void TMario::wallSlipEffect()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0x102, &mPosition, 1, this);
}

void TMario::runningRippleEffect()
{
	JGeometry::TVec3<f32>* ripplePos = (JGeometry::TVec3<f32>*)&unk190;
	if (mForwardVel > 30.0f)
		gpMarioParticleManager->emit(0x34, ripplePos, 0, nullptr);
	SMS_EmitRippleTiny(ripplePos);
}

void TMario::rippleEffect()
{
	if (checkFlag(MARIO_FLAG_IN_SHALLOW_WATER)) {
		SMS_EmitRipplePool(mJointMtx2, this);
	} else {
		SMS_EmitRippleSea(mJointMtx2, this);
		if (checkActionFlag(0x2000)
		    && mForwardVel > *(f32*)((u8*)this + 0x2828))
			mWaterWakeAlpha = 0xFF;
	}
}

void TMario::smallTouchDownEffect()
{
	static JGeometry::TVec3<f32> scale(0.7f, 0.7f, 0.7f);

	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emit(0x11, &mPosition, 0, nullptr);
	if (emitter) {
		emitter->unk154.set(scale);
		emitter->unk174.set(scale);
	}
}

void TMario::treeSlipEffect()
{
	JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x102, getCenterAnmMtx(), 1, this);
	static JGeometry::TVec3<f32> scale(0.8f, 0.8f, 0.8f);

	if (emitter) {
		emitter->unk154.set(scale);
		emitter->unk174.set(scale);
	}
}

void TMario::surfingEffect()
{
	f32 scale         = 1.0f;
	f32 scaleMin      = getSurfingParamsWater().mScaleMin.get();
	f32 scaleMax      = getSurfingParamsWater().mScaleMax.get();
	f32 scaleMinSpeed = getSurfingParamsWater().mScaleMinSpeed.get();
	f32 scaleMaxSpeed = getSurfingParamsWater().mScaleMaxSpeed.get();

	if (mForwardVel < scaleMinSpeed)
		scale = scaleMin;

	if (scaleMinSpeed <= mForwardVel && mForwardVel <= scaleMaxSpeed)
		scale = scaleMin
		    + (scaleMax - scaleMin) * (mForwardVel - scaleMinSpeed)
		        / (scaleMaxSpeed - scaleMinSpeed);

	if (scaleMaxSpeed < mForwardVel)
		scale = scaleMax;

	JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x1E7, getRootAnmMtx()[0], 3, this);
	if (emitter) {
		emitter->unk154.setAll(scale);
		emitter->unk174.setAll(scale);
	}

	emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x121, mJointMtx1, 1, this);
	if (emitter) {
		emitter->unk154.setAll(scale);
		emitter->unk174.setAll(scale);
	}

	emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x123, mJointMtx1, 1, this);
	if (emitter) {
		emitter->unk154.setAll(scale);
		emitter->unk174.setAll(scale);
	}

	emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x122, mJointMtx1, 1, this);
	if (emitter) {
		emitter->unk154.setAll(scale);
		emitter->unk174.setAll(scale);
	}
}

void TMario::strongTouchDownEffect()
{
	gpMarioParticleManager->emitWithRotate(
	    0x10, &mPosition, 0, mFaceAngle.y, 0, 0, nullptr);
	gpMarioParticleManager->emitWithRotate(
	    0x11, &mPosition, 0, mFaceAngle.y, 0, 0, nullptr);
}

void TMario::emitGetCoinEffect(JGeometry::TVec3<f32>* pos)
{
	gpMarioParticleManager->emit(0x37, pos, 0, nullptr);
	gpMarioParticleManager->emit(0x38, pos, 0, nullptr);
}

void TMario::emitGetWaterEffect()
{
	JGeometry::TVec3<f32>* p = &unk160[0];
	gpMarioParticleManager->emitAndBindToPosPtr(0xF, p, 0, nullptr);
}

void TMario::emitGetEffect()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0xE, &unk160[0], 0, nullptr);
	startSoundActor(0x1989);
}

void TMario::emitSweatSometimes()
{
	s16 yaw = mFaceAngle.y;
	if ((gpMarDirector->unk58 & 0xF) == 0)
		emitSweat(yaw);
}

#pragma dont_inline on
void TMario::emitSweat(short rotY)
{
	if (checkFlag(MARIO_FLAG_HELMET_FLW_CAMERA))
		return;
	if (checkFlag(MARIO_FLAG_IN_SHALLOW_WATER | MARIO_FLAG_IN_WATER))
		return;
	if (isUnderWater())
		return;

	MtxPtr mtx = mModel->getModel()->getAnmMtx(mBoneIDs[10]);
	JGeometry::TVec3<f32> pos(mtx[0][3], mtx[1][3], mtx[2][3]);
	gpMarioParticleManager->emitWithRotate(0xD, &pos, 0, rotY, 0, 0, nullptr);
}
#pragma dont_inline off

void TMario::emitSmoke(short rotY)
{
	if (!mGroundPlane->isPool())
		gpMarioParticleManager->emitWithRotate(
		    0x15, &mPosition, 0, rotY, 0, 0, nullptr);
}

bool TMario::emitParticle(int id, short rotY)
{
	JPABaseEmitter* emitter = gpMarioParticleManager->emitWithRotate(
	    id, &mPosition, 0, rotY, 0, 0, nullptr);
	if (emitter == nullptr)
		return false;
	return true;
}

bool TMario::emitParticle(int id, const JGeometry::TVec3<f32>* pos)
{
	JPABaseEmitter* emitter = gpMarioParticleManager->emit(id, pos, 0, nullptr);
	if (emitter == nullptr)
		return false;
	return true;
}

bool TMario::emitParticle(int id)
{
	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emit(id, &mPosition, 0, nullptr);
	if (emitter == nullptr)
		return false;
	return true;
}

void TMario::moveParticle()
{
	if (mWaterWakeAlpha > 0) {
		JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x109, mJointMtx2, 1, this);
		if (emitter) {
			emitter->unk180.a = mWaterWakeAlpha;
			mWaterWakeAlpha -= *(s16*)((u8*)this + 0x283C);
		}
	}
}

void TMario::initParticle()
{
	for (int i = 0; i < 3; ++i) {
		const char* fileName = cParticleFileNames[i];
		if (JKRFileLoader::getGlbResource(fileName) != nullptr) {
			u16 id = cParticleIDs[i];
			if (!gParticleFlagLoaded[id]) {
				gpResourceManager->load(fileName, id);
				gParticleFlagLoaded[id] = true;
			}
		}
	}
}

void TMario::kickFruitEffect()
{
	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emit(0x39, &mPosition, 0, nullptr);
	if (emitter) {
		emitter->unk154.x = 1.2f;
		emitter->unk154.y = 1.2f;
		emitter->unk154.z = 1.2f;
		emitter->unk174.x = 1.2f;
		emitter->unk174.y = 1.2f;
		emitter->unk174.z = 1.2f;
		JGeometry::TVec3<f32> pos = mPosition;
		pos.y += 30.0f;
		emitter->unk160.x = pos.x;
		emitter->unk160.y = pos.y;
		emitter->unk160.z = pos.z;
	}
}
