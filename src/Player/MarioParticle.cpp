#include <Player/MarioMain.hpp>
#include <Player/MarioEffect.hpp>
#include <Player/Watergun.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <Map/MapData.hpp>
#include <MarioUtil/EffectUtil.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JParticle/JPAParticle.hpp>

// rogue includes for matching __sinit (15 JALList<T> templates)
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";
static const char* SMS_NO_MEMORY_MESSAGE   = "メモリが足りません\n";
static const char cDirtyFileName[]          = "/scene/map/pollution/H_ma_rak.bti";
static const char cDirtyTexName[]           = "H_ma_rak_dummy";
static const char* MtxCalcTypeName[] = {
	"MActorMtxCalcType_Basic クラシックスケールＯＮ",
	"MActorMtxCalcType_Softimage クラシックスケールＯＦＦ",
	"MActorMtxCalcType_MotionBlend モーションブレンド",
	"MActorMtxCalcType_User ユーザー定義",
};

static const u32 cParticleIDs[] = { 0x50, 0x126, 0x12B };

static const u32 warpInEffectIDs[] = {
	0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23,
};

const char* cParticleFileNames[] = {
	"/scene/map/pollution/ms_m_ashios.jpa",
	"/scene/map/pollution/ms_m_spinos.jpa",
	"/scene/map/pollution/ms_m_tokeos.jpa",
};

class TBubbleCallBack
    : public JPACallBackBase2<JPABaseEmitter*, JPABaseParticle*> {
public:
	virtual void execute(JPABaseEmitter*, JPABaseParticle*);
};

class TWarpInCallBack
    : public JPACallBackBase2<JPABaseEmitter*, JPABaseParticle*> {
public:
	virtual void execute(JPABaseEmitter*, JPABaseParticle*);
};

static TBubbleCallBack bubbleCallBack;
static TWarpInCallBack warpInCallBack;

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
	if (emitter)
		emitter->setScale(JGeometry::TVec3<f32>(0.5f, 0.5f, 0.5f));
}

void TMario::toroccoEffect()
{
	JGeometry::TVec3<f32> dist(mPosition);
	dist.sub(mToroccoPos);
	f32 len = JGeometry::TVec3<f32>(dist).length();

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
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x10E, mModel->unk8->mNodeMatrices[1], 1, this);
}

void TMario::warpOutEffect(int type, f32 angle)
{
	switch (type) {
	case 0:
		gpMarioParticleManager->emitWithRotate(
		    0x40, &mPosition, 0, (s16)(angle * 182.04445f), 0, 0, this);
		return;
	case 1:
		gpMarioParticleManager->emitWithRotate(
		    0x41, &mPosition, 0, (s16)(angle * 182.04445f), 0, 0, this);
		return;
	default:
		break;
	}

	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x24, mModel->getModel()->getAnmMtx(unk3C4), 0, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x25, mModel->getModel()->getAnmMtx(mBoneIDs[10]), 0, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x26, mModel->getModel()->getAnmMtx(mBoneIDs[10]), 0, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x27, mModel->getModel()->getAnmMtx(mBoneIDs[4]), 0, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x28, mModel->getModel()->getAnmMtx(mBoneIDs[5]), 0, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x29, mModel->getModel()->getAnmMtx(mBoneIDs[6]), 0, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x2A, mModel->getModel()->getAnmMtx(mBoneIDs[7]), 0, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x2B, mModel->getModel()->getAnmMtx(mBoneIDs[8]), 0, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x2C, mModel->getModel()->getAnmMtx(mBoneIDs[9]), 0, this);

	if (checkFlag(MARIO_FLAG_HAS_FLUDD)) {
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x2D, mModel->getModel()->getAnmMtx(unk3C4), 0, this);
	}
}

void TMario::warpInLight()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0x51, &unk160[2], 0, this);
}

void TMario::warpInEffect()
{
	for (int i = 0; i < 10; ++i) {
		u16 joint;
		switch (i) {
		case 0:
			joint = unk3C4;
			break;
		case 1:
			joint = mBoneIDs[10];
			break;
		case 2:
			joint = mBoneIDs[10];
			break;
		case 3:
			joint = mBoneIDs[4];
			break;
		case 4:
			joint = mBoneIDs[5];
			break;
		case 5:
			joint = mBoneIDs[6];
			break;
		case 6:
			joint = mBoneIDs[7];
			break;
		case 7:
			joint = mBoneIDs[8];
			break;
		case 8:
			joint = mBoneIDs[9];
			break;
		case 9:
			joint = unk3C4;
			break;
		default:
			joint = unk3C4;
			break;
		}

		MtxPtr mtx = mModel->getModel()->getAnmMtx(joint);
		int id     = warpInEffectIDs[i];
		int doEmit = 1;
		if (id == 0x23 && !checkFlag(MARIO_FLAG_HAS_FLUDD))
			doEmit = 0;

		if (doEmit == 1) {
			JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtx(
			    id, mtx, 0, this);
			if (emitter) {
				emitter->unk114 = &warpInCallBack;
				emitter->unk120 = &mWarpInDir;
			}
		}
	}

	gpMarioParticleManager->emitAndBindToMtx(
	    0x3C, getCenterAnmMtx(), 0, nullptr);

	u8* actor       = *(u8**)((u8*)this + 0x68);
	u8* modelHolder = *(u8**)(actor + 0x78);
	J3DModel* model = *(J3DModel**)(modelHolder + 4);
	u16 joint       = *(u16*)(actor + 0x72);
	gpMarioParticleManager->emitAndBindToMtx(
	    0x1D6, model->getAnmMtx(joint), 2, actor);
}

void TWarpInCallBack::execute(JPABaseEmitter* emitter, JPABaseParticle* particle)
{
	TMario* mario   = gpMarioOriginal;
	f32 actionTimer = mario->mActionTimer;
	f32 x           = particle->unk14.x;
	f32 y           = particle->unk14.y;
	f32 z           = particle->unk14.z;
	s32 randomBits  = ((u32)particle >> 2) & 0x3F;
	f32 randomScale = randomBits;
	randomScale *= 0.0625f;
	randomScale += 1.0f;
	f32 marioScale  = mario->unk468;

	JGeometry::TVec3<f32> dir
	    = *(JGeometry::TVec3<f32>*)emitter->unk120 * marioScale;
	JGeometry::TVec3<f32> step = dir * actionTimer;
	JGeometry::TVec3<f32> offset = step * randomScale;

	f32 newX = x + offset.x;
	f32 newY = y + offset.y;
	f32 newZ = z + offset.z;
	particle->unk14.x = newX;
	particle->unk14.y = newY;
	particle->unk14.z = newZ;
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
		    0x1, &mWaterGun->getEmitPos0(), 0, this);
		gpMarioParticleManager->emitAndBindToPosPtr(
		    0x2, &mWaterGun->getEmitPos0(), 0, this);
		gpMarioParticleManager->emitAndBindToPosPtr(
		    0x3, &mWaterGun->getEmitPos0(), 0, this);
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
	if (mForwardVel > 30.0f)
		gpMarioParticleManager->emit(0x34, &mWaterRipplePos, 0, nullptr);
	SMS_EmitRippleTiny(&mWaterRipplePos);
}

void TMario::swimmingBubbleEffect()
{
	if (!isMario())
		return;
	if (checkFlag(0x400))
		return;

	if (unk160[1].y + mParticleParams.mBubbleDepth.value
	    < *(f32*)((u8*)this + 0xF0)) {
		if (isMario()) {
			JPABaseEmitter* emitter
			    = gpMarioParticleManager->emitParticleCallBack(
			        0x10C, &unk160[1], 1, &bubbleCallBack, this);
			if (emitter)
				emitter->setGlobalRTMatrix(mJointMtx0);
		}
	}

	if (unk160[2].y + mParticleParams.mBubbleDepth.value
	    < *(f32*)((u8*)this + 0xF0))
		bubbleFromBody();
}

#pragma dont_inline on
void TMario::bubbleFromBody()
{
	if (!isMario())
		return;

	f32 maxSpeed = mParticleParams.mBodyBubbleSpMax.value;
	f32 speed    = mForwardVel;
	f32 rate     = 0.0f;
	f32 minSpeed = mParticleParams.mBodyBubbleSpMin.value;
	if (speed > maxSpeed) {
		rate = 1.0f;
	} else if (speed > minSpeed) {
		rate = (speed - minSpeed) / (maxSpeed - minSpeed);
	}

	f32 childRate = mParticleParams.mBodyBubbleEmitMin.value
	    + rate
	        * (mParticleParams.mBodyBubbleEmitMax.value
	           - mParticleParams.mBodyBubbleEmitMin.value);
	JPABaseEmitter* emitter = gpMarioParticleManager->emitParticleCallBack(
	    0x111, &unk160[2], 1, &bubbleCallBack, this);
	if (emitter) {
		emitter->setGlobalRTMatrix(getCenterAnmMtx());
		emitter->mChildSpawnRate = childRate;
	}
}
#pragma dont_inline off

void TMario::bubbleFromMouth(int index)
{
	if (!isMario())
		return;

	const JGeometry::TVec3<f32>* pos = &unk160[1];
	const void* owner = (const void*)((u8*)this + index * 0x4290);
	JPABaseEmitter* emitter = gpMarioParticleManager->emitParticleCallBack(
	    0x10C, pos, 1, &bubbleCallBack, owner);
	if (emitter)
		emitter->setGlobalRTMatrix(mJointMtx0);
}

void TBubbleCallBack::execute(JPABaseEmitter*, JPABaseParticle* particle)
{
	if (gpMarioOriginal->checkFlag(MARIO_FLAG_HELMET_FLW_CAMERA))
		return;

	JGeometry::TVec3<f32> pos;
	particle->getCurrentPosition(pos);
	if (pos.y > *(f32*)((u8*)gpMarioOriginal + 0xF0)) {
		particle->setDeleteParticleFlag();
		if (gpMarioOriginal->mParticleParams.mBubbleToRipple.value != 0.0f)
			gpMarioParticleManager->emit(0x33, &pos, 0, nullptr);
	}
}

void TMario::inOutWaterEffect(f32)
{
	JGeometry::TVec3<f32> pos = mPosition;
	pos.y                     = *(f32*)((u8*)this + 0xF0);

	if (checkFlag(MARIO_FLAG_IN_SHALLOW_WATER)
	    || ((mPrevState & MARIO_FLAG_IN_SHALLOW_WATER) ? true : false)) {
		((TMarioEffect*)mMarioEffect)->setJumpIntoWaterEffectSmall();
		gpMarioParticleManager->emit(0x31, &pos, 0, nullptr);
		return;
	}

	((TMarioEffect*)mMarioEffect)->setJumpIntoWaterEffect();
	f32 velY = mVel.y;
	if (velY < 0.0f)
		velY = -velY;

	if (velY > *(f32*)((u8*)this + 0x22C4)) {
		rumbleStart(0x15, *(s16*)((u8*)this + 0x27F8));
		if (!checkActionFlag(0x200))
			gpMarioParticleManager->emit(0x2F, &pos, 0, nullptr);
		gpMarioParticleManager->emit(0x30, &pos, 0, nullptr);
		gpMarioParticleManager->emit(0x1D4, &pos, 2, nullptr);
	} else {
		if (!checkActionFlag(0x200))
			gpMarioParticleManager->emit(0x31, &pos, 0, nullptr);
		gpMarioParticleManager->emit(0x32, &pos, 0, nullptr);
		gpMarioParticleManager->emit(0x1D5, &pos, 2, nullptr);
	}
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
	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emit(0x11, &mPosition, 0, nullptr);
	static JGeometry::TVec3<f32> scale(0.7f, 0.7f, 0.7f);

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

	{
		MtxPtr jointMtx = mJointMtx1;
		emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x121, jointMtx, 1, this);
	}
	if (emitter) {
		emitter->unk154.setAll(scale);
		emitter->unk174.setAll(scale);
	}

	{
		MtxPtr jointMtx = mJointMtx1;
		emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x123, jointMtx, 1, this);
	}
	if (emitter) {
		emitter->unk154.setAll(scale);
		emitter->unk174.setAll(scale);
	}

	{
		MtxPtr jointMtx = mJointMtx1;
		emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x122, jointMtx, 1, this);
	}
	if (emitter) {
		emitter->unk154.setAll(scale);
		emitter->unk174.setAll(scale);
	}
}

void TMario::frontSlipEffect()
{
	u16 bgType = mGroundPlane->mBGType;
	bool waterSlip;
	if (bgType == BG_TYPE_WET_GROUND || bgType == BG_TYPE_SHADED_WET_GROUND
	    || bgType == BG_TYPE_CAM_NOCLIP_WET_GROUND
	    || bgType == BG_TYPE_CAM_NOCLIP_SHADED_WET_GROUND)
		waterSlip = true;
	else
		waterSlip = false;

	if (waterSlip || (mAction == ACTION_CATCHING && mActionState == 1)) {
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x1EA, getCenterAnmMtx(), 3, this);
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x112, getCenterAnmMtx(), 1, this);
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x113, getCenterAnmMtx(), 1, this);
	} else {
		if (mPosition.y < *(f32*)((u8*)this + 0xF0))
			return;

		if (checkFlag(0x40000)) {
			calcGroundMtx(unk160[2]);
			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x110, mGroundMtx, 1, this);
			gpMarioParticleManager->emitAndBindToPosPtr(
			    0x10F, &unk160[2], 1, this);
		} else {
			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x103, mModel->getModel()->getAnmMtx(0), 1, this);
		}
	}
}

void TMario::strongTouchDownEffect()
{
	gpMarioParticleManager->emitWithRotate(
	    0x10, &mPosition, 0, mFaceAngle.y, 0, 0, nullptr);
	s16 angle = mFaceAngle.y;
	gpMarioParticleManager->emitWithRotate(
	    0x11, &mPosition, 0, angle, 0, 0, nullptr);
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
	JGeometry::TVec3<f32> pos;
	pos.x = mtx[0][3];
	pos.y = mtx[1][3];
	pos.z = mtx[2][3];
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
		JGeometry::TVec3<f32> scale(1.2f, 1.2f, 1.2f);
		emitter->setScale(scale);
		JGeometry::TVec3<f32> pos = mPosition;
		pos.y += 30.0f;
		emitter->unk160.set(pos);
	}
}
