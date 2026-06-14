#define JGEOMETRY_GEKKO_PS_COPY12_OUT_OF_LINE
#include <MoveBG/MapObjBlock.hpp>
#include <MoveBG/MapObjHide.hpp>
#include <MoveBG/MapObjGeneral.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionManager.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Strategic/HitActor.hpp>
#include <Strategic/LiveActor.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/ModelWaterManager.hpp>
#include <System/MarDirector.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/Particles.hpp>
#include <System/FlagManager.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/MtxUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <M3DUtil/MActor.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JGeometry/JGUtil.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
#include <dolphin/mtx.h>
#include <string.h>

#include <M3DUtil/InfectiousStrings.hpp>

// Static const class members (matching mAutoMeltScale__9TIceBlock etc in sdata)
f32 TSandBlock::mSandScaleUp    = 0.075f;
f32 TSandBlock::mSandScaleDown  = 0.0075f;
f32 TSandBlock::mSandScaleMin   = 0.05f;
s32 TSandBlock::mWaitTimeToFall = 40;
s32 TSandBlock::mSandWaitTime   = 400;

f32 TIceBlock::mMeltSpeedWater = 0.003f;
f32 TIceBlock::mMeltSpeedAuto  = 0.004f;
f32 TIceBlock::mAutoMeltScale  = 0.2f;

void TBreakableBlock::touchPlayer(THitActor* sender)
{
	if (marioHipAttack()) {
		SMSRumbleMgr->start(0x15, 0x14, (Vec*)&mPosition);
		kill();
	}
}

void TSandBlock::touchPlayer(THitActor* sender)
{
	if (marioIsOn() && mState == 1) {
		mLifeTimer = mWaitTimeToFall;
		mState     = 3;
	}
}

void TSandBlock::control()
{
	TMapObjBase::control();
	switch (mState) {
	case 2: {
		mScaling.x += mSandScaleUp;
		mScaling.y += mSandScaleUp;
		mScaling.z += mSandScaleUp;
		if (mScaling.y >= mInitialScaling.x) {
			mScaling.x = mInitialScaling.x;
			mScaling.y = mInitialScaling.y;
			mScaling.z = mInitialScaling.z;
			mState     = 1;
		}
		break;
	}
	case 4: {
		mScaling.y -= mSandScaleDown;
		if (gpMSound->gateCheck(0x30aa)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x30aa, (Vec*)&mPosition, 0, 0, 0, 4);
		}
		JGeometry::TVec3<f32> baseScale;
		baseScale.x = mScaling.x;
		baseScale.y = mInitialScaling.y;
		baseScale.z = mScaling.z;
		emitAndScale(0x147, 1, &mPosition, baseScale);
		emitAndScale(0x148, 1, &mPosition, baseScale);
		if (mScaling.y < mSandScaleMin) {
			mScaling.x = mScaling.y;
			mScaling.z = mScaling.y;
			sleep();
			mLifeTimer = mSandWaitTime;
			mState     = 5;
		}
		break;
	}
	case 3: {
		if (mLifeTimer <= 0) {
			setUpMapCollision(1);
			mState = 4;
		}
		break;
	}
	case 5: {
		if (mLifeTimer <= 0) {
			f32 dist = getDistance(*gpMarioPos);
			if (dist > mScaling.x * 100.0f) {
				awake();
				JGeometry::TVec3<f32> saved;
				saved.x    = mScaling.x;
				saved.y    = mScaling.y;
				saved.z    = mScaling.z;
				mScaling.x = mInitialScaling.x;
				mScaling.y = mInitialScaling.y;
				mScaling.z = mInitialScaling.z;
				setUpMapCollision(0);
				mScaling.x = saved.x;
				mScaling.y = saved.y;
				mScaling.z = saved.z;
				mState     = 2;
			}
		}
		break;
	}
	}
}

void TSandBlock::initMapObj()
{
	TMapObjBase::initMapObj();
	mInitialScaling.x = mScaling.x;
	mInitialScaling.y = mScaling.y;
	mInitialScaling.z = mScaling.z;
	SMS_LoadParticle("/scene/mapObj/SandBlockBreakA.jpa", 0x147);
	SMS_LoadParticle("/scene/mapObj/SandBlockBreakB.jpa", 0x148);
}

void TLeanBlock::touchPlayer(THitActor* sender)
{
	if (marioIsOn()) {
		f32 dx = gpMarioPos->x - mPosition.x;
		f32 dz = gpMarioPos->z - mPosition.z;
		JGeometry::TVec3<f32> delta(unk140 * (dx / unk138), 0.0f,
		                             unk140 * (dz / unk13C));
		unk158.add(delta);
		unk158.y -= unk144;
	}
}

static inline f32 calcLeanLength(f32 mag)
{
	f64 root = __frsqrte(mag);
	return (f32)(0.5 * root * (3.0 - mag * (root * root)) * mag);
}

void TLeanBlock::control()
{
	TMapObjBase::control();
	J3DModel* model = getModel();
	f32 sumSq       = unk158.x * unk158.x + unk158.y * unk158.y
	            + unk158.z * unk158.z;
	if (sumSq <= 0.0000038146973f) {
		unk14C.y += unk144;
	} else {
		unk14C.x += unk158.x;
		unk14C.y += unk158.y;
		unk14C.z += unk158.z;
	}
	MsVECNormalize((Vec*)&unk14C, (Vec*)&unk14C);
	JGeometry::TVec3<f32> axis;
	axis.x = unk14C.x;
	axis.y = 0.0f;
	axis.z = unk14C.z;
	rotateVecByAxisY(&axis, 1.5707963f);
	f32 lenSq = unk14C.x * unk14C.x + unk14C.z * unk14C.z;
	f32 len   = 0.0f;
	if (lenSq > 0.0f) {
		len = calcLeanLength(lenSq);
	}
	f32 angle = len * unk148;
	Mtx tmp;
	makeObjMtxRotByAxis(axis, angle, tmp);
	concatOnlyRotFromRight(tmp, unk164, tmp);
	unk158.x = 0.0f;
	unk158.y = 0.0f;
	unk158.z = 0.0f;
}

void TLeanBlock::calcLeanMtx(MtxPtr out)
{
	f32 sumSq = unk158.x * unk158.x + unk158.y * unk158.y
	          + unk158.z * unk158.z;
	if (sumSq <= 0.0000038146973f) {
		unk14C.y += unk144;
	} else {
		unk14C.x += unk158.x;
		unk14C.y += unk158.y;
		unk14C.z += unk158.z;
	}
	MsVECNormalize((Vec*)&unk14C, (Vec*)&unk14C);
	JGeometry::TVec3<f32> axis;
	axis.x = unk14C.x;
	axis.y = 0.0f;
	axis.z = unk14C.z;
	rotateVecByAxisY(&axis, 1.5707963f);
	f32 lenSq = unk14C.x * unk14C.x + unk14C.z * unk14C.z;
	f32 len   = 0.0f;
	if (lenSq > 0.0f) {
		len = calcLeanLength(lenSq);
	}
	f32 angle = len * unk148;
	makeObjMtxRotByAxis(axis, angle, out);
	concatOnlyRotFromRight(out, unk164, out);
}

#pragma dont_inline on
void JGeometry::gekko_ps_copy12(register void* dst, register void* src)
{
	register f32 src0;
	register f32 src1;
	register f32 src2;
	register f32 src3;
	register f32 src4;
	register f32 src5;
#ifdef __MWERKS__ // clang-format off
	asm {
		psq_l src0, 0(src), 0, 0
		psq_l src1, 8(src), 0, 0
		psq_l src2, 16(src), 0, 0
		psq_l src3, 24(src), 0, 0
		psq_l src4, 32(src), 0, 0
		psq_l src5, 40(src), 0, 0
		psq_st src0, 0(dst), 0, 0
		psq_st src1, 8(dst), 0, 0
		psq_st src2, 16(dst), 0, 0
		psq_st src3, 24(dst), 0, 0
		psq_st src4, 32(dst), 0, 0
		psq_st src5, 40(dst), 0, 0
	}
#endif // clang-format on
}
#pragma dont_inline off

void TLeanBlock::calcDefaultMtx()
{
	JGeometry::gekko_ps_copy12(unk164, getModel()->mNodeMatrices);
}

void TLeanBlock::initMapObj()
{
	TMapObjBase::initMapObj();
	unk140 = 0.01f;
	unk144 = 0.005f;
	unk148 = 1.0f;
	unk138 = 100.0f * mScaling.x * 0.5f;
	unk13C = 100.0f * mScaling.z * 0.5f;
	// virtual setModelMtx vtable call
	((void (*)(TLeanBlock*, MtxPtr))((*(void***)this)[0x164 / 4]))(this, unk164);
}

TLeanBlock::TLeanBlock(const char* name)
    : TMapObjBase(name)
{
	unk138   = 0.0f;
	unk13C   = 0.0f;
	unk140   = 0.0f;
	unk144   = 0.0f;
	unk148   = 0.0f;
	unk158.x = 0.0f;
	unk158.y = 0.0f;
	unk158.z = 0.0f;
	unk14C.x = 0.0f;
	unk14C.y = 1.0f;
	unk14C.z = 0.0f;

	// init unk164 to identity Mtx (matches JGeometry::TMatrix34::identity())
	unk164[0][3] = unk164[1][3] = unk164[2][3] = 0.0f;
	unk164[0][2] = unk164[1][2] = 0.0f;
	unk164[0][1] = unk164[2][1] = 0.0f;
	unk164[1][0] = unk164[2][0] = 0.0f;
	unk164[0][0] = unk164[1][1] = unk164[2][2] = 1.0f;
}

u32 TIceBlock::getSDLModelFlag() const { return 0; }

u32 TIceBlock::touchWater(THitActor* sender)
{
	((TMapObjBase*)sender)->getWaterSpeed(sender);
	int waterId    = TMapObjBase::getWaterID(sender);
	u16 waterFlags = ((u16*)((char*)gpModelWaterManager + 0x414))[waterId];
	if ((waterFlags & 0xF) != 1) {
		return 0;
	}
	gpMarioParticleManager->emit(0xE7, &sender->mPosition, 0, 0);
	gpMSound->startSoundSet(0x6802, (Vec*)&mPosition, 0, 0.0f, 0, 0, 4);
	if (gpMSound->gateCheck(0x3079)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x3079, (Vec*)&mPosition, 0, 0, 0, 4);
	}
	mScaling.x -= mMeltSpeedWater;
	mScaling.y -= mMeltSpeedWater;
	mScaling.z -= mMeltSpeedWater;
	mScaledBodyRadius = mScaling.x * mMapObjData->unk30;
	if (mScaling.y > mInitialScaling.y) {
		mScaling.y = mInitialScaling.y;
	} else if (mScaling.y < 0.01f) {
		mScaling.y = 0.01f;
	}
	if (mScaling.x < 0.0f) {
		makeObjDead();
	}
	mColCount = 0;
	return 1;
}

void TIceBlock::control()
{
	JPABaseEmitter* emt = gpMarioParticleManager->emit(
	    0x157, &mPosition, 1, this);
	if (emt) {
		emt->unk154.x = mScaling.x;
		emt->unk154.y = mScaling.y;
		emt->unk154.z = mScaling.z;
	}
	emt = gpMarioParticleManager->emit(0x158, &mPosition, 1, this);
	if (emt) {
		emt->unk154.x = mScaling.x;
		emt->unk154.y = mScaling.y;
		emt->unk154.z = mScaling.z;
	}
	unk64 &= ~1;
	mDamageRadius = 80.0f * mScaling.x;
	calcEntryRadius();
	mDamageHeight = 250.0f * mScaling.y;
	calcEntryRadius();
	if (mScaling.y < mAutoMeltScale * mInitialScaling.y) {
		mScaling.x -= mMeltSpeedAuto;
		mScaling.y -= mMeltSpeedAuto;
		mScaling.z -= mMeltSpeedAuto;
		if (mScaling.y > mInitialScaling.y) {
			mScaling.y = mInitialScaling.y;
		} else if (mScaling.y < 0.01f) {
			mScaling.y = 0.01f;
		}
		mScaledBodyRadius = mScaling.x * mMapObjData->unk30;
		if (gpMSound->gateCheck(0x3079)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x3079, (Vec*)&mPosition, 0, 0, 0, 4);
		}
		setObjHitData(0);
		unk64 |= 1;
		removeMapCollision();
		if (mScaling.x < 0.1f) {
			makeObjDead();
		}
	}
}

void TIceBlock::calc()
{
	Mtx mtx;
	SMS_GetLightPerspectiveForEffectMtx(mtx);
	J3DMaterial* mat = getModel()->getModelData()->getMaterialNodePointer(0);
	mat->getTexGenBlock()->getTexMtx(1)->setEffectMtx(mtx);
}

void TIceBlock::initMapObj()
{
	TMapObjBase::initMapObj();
	SMS_LoadParticle("/scene/mapObj/IceBlockA.jpa", 0x157);
	SMS_LoadParticle("/scene/mapObj/IceBlockB.jpa", 0x158);
}

void TBrickBlock::kill()
{
	makeObjDead();
	emitAndScale(0x60, 0, &mPosition);
	emitAndScale(0x61, 0, &mPosition);
	emitAndScale(0x62, 0, &mPosition);
	if (gpMSound->gateCheck(0x3878)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x3878, (Vec*)&mPosition, 0, 0, 0, 4);
	}
	SMSRumbleMgr->start(0x15, 0x14, (Vec*)&mPosition);
	appearObj(100.0f);
}

BOOL TBrickBlock::receiveMessage(THitActor* sender, u32 message)
{
	if (sender->isActorTypeOf(0x80000000)) {
		if (marioHeadAttack()) {
			kill();
			return TRUE;
		}
	}
	if (sender->isActorType(0x08000005)) {
		if (message == 0xE) {
			kill();
			return TRUE;
		}
	}
	return FALSE;
}

void TBrickBlock::initMapObj()
{
	TMapObjBase::initMapObj();
	SMS_LoadParticle("/scene/mapObj/BrickBlockA.jpa", 0x60);
	SMS_LoadParticle("/scene/mapObj/BrickBlockB.jpa", 0x61);
	SMS_LoadParticle("/scene/mapObj/BrickBlockC.jpa", 0x62);
}

void TJuiceBlock::initMapObj()
{
	TMapObjBase::initMapObj();
	SMS_InitPacket_OneTevColor(mMActor->getModel(), 0,
	                            (GXTevRegID)1, (GXColorS10*)&unk138);
	unk140.x = 1.0f;
	unk140.y = 1.0f;
	unk140.z = 1.0f;
}

void TJuiceBlock::moveObject()
{
	TLiveActor::moveObject();
	if (unk14C != NULL) {
		// fabricated cast TSmallEnemy* / read mLiveFlag at 0xF0
		if ((((TLiveActor*)unk14C)->mLiveFlag) & 1) {
			kill();
		}
	}
}

void TJuiceBlock::kill()
{
	unk14C = 0;
	makeObjDead();
}

void TTelesaBlock::initMapObj() { TMapObjBase::initMapObj(); }

void TTelesaBlock::perform(u32 param, JDrama::TGraphics* graphics)
{
	mLiveFlag &= ~0x500;
	bool visible = false;
	u8 ds        = gpMarDirector->unk124;
	if (ds == 1 || ds == 2)
		visible = true;
	if (!visible) {
		TMapObjBase::perform(param, graphics);
	} else {
		if (param & 1) {
			TLiveActor::moveObject();
			if (unk14C && (((TLiveActor*)unk14C)->mLiveFlag & 1)) {
				kill();
			}
		}
		mMActor->perform(param, graphics);
	}
	if (param & 2) {
		Mtx scratch;
		scratch[0][1] = 0.0f;
		scratch[1][0] = 0.0f;
		scratch[1][2] = 0.0f;
		scratch[0][0] = unk140.x;
		scratch[0][2] = 0.0f;
		scratch[0][3] = 0.0f;
		scratch[1][1] = unk140.y;
		scratch[1][3] = 0.0f;
		scratch[2][0] = 0.0f;
		scratch[2][1] = 0.0f;
		scratch[2][2] = unk140.z;
		scratch[2][3] = 0.0f;
		J3DModel* model = getModel();
		PSMTXConcat(model->mNodeMatrices[1], scratch, model->mNodeMatrices[1]);

		scratch[0][0] = unk140.y;
		scratch[0][1] = 0.0f;
		scratch[0][2] = 0.0f;
		scratch[0][3] = 0.0f;
		scratch[1][0] = 0.0f;
		scratch[1][1] = unk140.y;
		scratch[1][2] = 0.0f;
		scratch[1][3] = 0.0f;
		scratch[2][0] = 0.0f;
		scratch[2][1] = 0.0f;
		scratch[2][2] = unk140.z;
		scratch[2][3] = 0.0f;
		model = getModel();
		PSMTXConcat(model->mNodeMatrices[0], scratch, model->mNodeMatrices[0]);
	}
}

void TTelesaBlock::setGroundCollision()
{
	if (mMapCollisionManager && mMapCollisionManager->unk8) {
		// virtual call vtable[0x3] (changeData-like, accepts pos/rot/scale)
		((void (*)(TMapCollisionBase*, Vec*, S16Vec*, Vec*))(
		    (*(void***)mMapCollisionManager->unk8)[3]))(
		    mMapCollisionManager->unk8, (Vec*)&mPosition,
		    (S16Vec*)&mRotation, (Vec*)&unk140);
	}
}

BOOL TSuperHipDropBlock::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 3) {
		kill();
		if (unk150) {
			TFlagManager::smInstance->setBool(1, 0x1038C);
		}
		if (gpMSound->gateCheck(0x3821)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x3821, (Vec*)&mPosition, 0, 0, 0, 4);
		}
		return TRUE;
	}
	return FALSE;
}

void TSuperHipDropBlock::loadAfter()
{
	THideObjBase::loadAfter();
	if (strcmp("モンテゲートブロック", mName) == 0) {
		unk150 = 1;
		if (TFlagManager::smInstance->getBool(0x1038C)) {
			makeObjDead();
		}
		mLiveFlag |= 0x8;
	}
}

// rogue includes for static init (JALList<*> instantiations)
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSSetSound.hpp>
