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
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JGeometry/JGUtil.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
#include <dolphin/mtx.h>
#include <string.h>

#include <M3DUtil/InfectiousStrings.hpp>

// Static const class members (matching mAutoMeltScale__9TIceBlock etc in sdata)
const f32 TSandBlock::sSandScaleUp    = 0.005f;
const f32 TSandBlock::sSandScaleDown  = 0.0125f;
const f32 TSandBlock::sSandScaleMin   = 0.01f;
const s32 TSandBlock::sWaitTimeToFall = 60;
const s32 TSandBlock::sSandWaitTime   = 120;

const f32 TIceBlock::sMeltSpeedWater = 0.01f;
const f32 TIceBlock::sMeltSpeedAuto  = 0.0005f;
const f32 TIceBlock::sAutoMeltScale  = 0.8f;

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
		mLifeTimer = sWaitTimeToFall;
		mState     = 3;
	}
}

void TSandBlock::control()
{
	TMapObjBase::control();
	switch (mState) {
	case 1: {
		mScaling.x += sSandScaleUp;
		mScaling.y += sSandScaleUp;
		mScaling.z += sSandScaleUp;
		if (mScaling.y >= mInitialScaling.y) {
			mScaling.x = mInitialScaling.x;
			mScaling.y = mInitialScaling.y;
			mScaling.z = mInitialScaling.z;
			mState     = 1;
		}
		break;
	}
	case 2: {
		mScaling.y -= sSandScaleDown;
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
		if (mScaling.y < sSandScaleMin) {
			mScaling.x = mScaling.y;
			mScaling.z = mScaling.y;
			sleep();
			mLifeTimer = sSandWaitTime;
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
		f32 nx = unk140 * (dx / unk138);
		f32 nz = unk140 * (dz / unk13C);
		unk158.x += nx;
		unk158.y += 0.0f;
		unk158.z += nz;
		unk158.y -= unk144;
	}
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
		len = JGeometry::TUtil<f32>::sqrt(lenSq);
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
		len = JGeometry::TUtil<f32>::sqrt(lenSq);
	}
	f32 angle = len * unk148;
	makeObjMtxRotByAxis(axis, angle, out);
	concatOnlyRotFromRight(out, unk164, out);
}

void TLeanBlock::calcDefaultMtx()
{
	JGeometry::gekko_ps_copy12(unk164, getModel()->unk20);
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

	// init unk164 to identity Mtx
	unk164[2][3] = 0.0f;
	unk164[2][2] = 1.0f;
	unk164[2][1] = 0.0f;
	unk164[2][0] = 0.0f;
	unk164[1][3] = 0.0f;
	unk164[1][2] = 0.0f;
	unk164[1][1] = 1.0f;
	unk164[1][0] = 0.0f;
	unk164[0][3] = 0.0f;
	unk164[0][2] = 0.0f;
	unk164[0][1] = 0.0f;
	unk164[0][0] = 1.0f;
}

u32 TIceBlock::getSDLModelFlag() const { return 0; }

u32 TIceBlock::touchWater(THitActor* sender)
{
	getWaterSpeed(sender);
	int waterId    = TMapObjBase::getWaterID(sender);
	u16 waterFlags = ((u16*)((char*)gpModelWaterManager + 0x414))[waterId];
	if ((waterFlags & 0xF) != 1) {
		return 0;
	}
	gpMarioParticleManager->emit(0xE7, &mPosition, 0, 0);
	gpMSound->startSoundSet(0x6802, (Vec*)&sender->mPosition, 0, 0.0f, 0, 0, 0);
	if (gpMSound->gateCheck(0x3079)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x3079, (Vec*)&sender->mPosition, 0, 0, 0, 4);
	}
	mScaling.x -= sMeltSpeedWater;
	mScaling.y -= sMeltSpeedWater;
	mScaling.z -= sMeltSpeedWater;
	mScaledBodyRadius = mScaling.x * mMapObjData->unk30;
	if (mScaling.y > mInitialScaling.y) {
		mScaling.y = mInitialScaling.y;
	} else if (mScaling.y < 0.01f) {
		mScaling.y = 0.01f;
	}
	if (mScaling.x < 0.0f) {
		kill();
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
	mScaling.z = 80.0f * mScaling.x;
	calcEntryRadius();
	mScaling.z = 250.0f * mScaling.y;
	calcEntryRadius();
	if (mScaling.y < sAutoMeltScale * mInitialScaling.y) {
		mScaling.x -= sMeltSpeedAuto;
		mScaling.y -= sMeltSpeedAuto;
		mScaling.z -= sMeltSpeedAuto;
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
		performOnlyDraw(0, (JDrama::TGraphics*)NULL);
		unk64 |= 1;
		removeMapCollision();
		if (mScaling.x < 0.1f) {
			kill();
		}
	}
}

void TIceBlock::calc()
{
	Mtx mtx;
	SMS_GetLightPerspectiveForEffectMtx(mtx);
	// fabricated: chain of unk pointers through J3DModelData
	void* model       = (void*)getModel();
	void* data        = *(void**)((char*)model + 0x4);
	void* chain       = *(void**)((char*)data + 0x28);
	void* entry       = *(void**)chain;
	void* info        = *(void**)((char*)entry + 0x24);
	(*(void (**)(void*, MtxPtr))((*(void***)info)[1]))(info, mtx);
}

void TIceBlock::initMapObj()
{
	TMapObjBase::initMapObj();
	SMS_LoadParticle("/scene/mapObj/IceBlockA.jpa", 0x157);
	SMS_LoadParticle("/scene/mapObj/IceBlockB.jpa", 0x158);
}

void TBrickBlock::kill()
{
	THideObjBase::kill();
	emitAndScale(0x60, 0, &mPosition);
	emitAndScale(0x61, 0, &mPosition);
	emitAndScale(0x62, 0, &mPosition);
	if (gpMSound->gateCheck(0x3878)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x3878, (Vec*)&mPosition, 0, 0, 0, 4);
	}
	SMSRumbleMgr->start(0x15, 0x14, (Vec*)&mPosition);
	makeObjDead();
}

BOOL TBrickBlock::receiveMessage(THitActor* sender, u32 message)
{
	if (sender->isActorTypeOf(0x80000000)) {
		if (marioHeadAttack()) {
			kill();
			return TRUE;
		}
	}
	if (sender->isActorTypeOf(0x80000800)) {
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
		PSMTXConcat(getModel()->unk20, scratch, scratch);
		// Note: original has two concat calls; second writes back
	}
}

void TTelesaBlock::setGroundCollision()
{
	if (mMapCollisionManager && mMapCollisionManager->unk8) {
		// virtual call vtable[0x3] (changeData-like, accepts pos/rot/scale)
		(*(void (**)(TMapCollisionBase*, Vec*, S16Vec*, Vec*))(
		    (*(void***)mMapCollisionManager->unk8)[3]))(
		    mMapCollisionManager->unk8, (Vec*)&mPosition,
		    (S16Vec*)&mRotation, (Vec*)&unk140);
	}
}

BOOL TSuperHipDropBlock::receiveMessage(THitActor* sender, u32 message)
{
	if (message != 3)
		return FALSE;
	emitEffect();
	if (unk150) {
		TFlagManager::smInstance->setBool(1, 0x1038C);
	}
	if (gpMSound->gateCheck(0x3821)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x3821, (Vec*)&mPosition, 0, 0, 0, 4);
	}
	return TRUE;
}

void TSuperHipDropBlock::loadAfter()
{
	THideObjBase::loadAfter();
	if (strcmp(mName, "テレサブロック") == 0) {
		unk150 = 1;
		if (TFlagManager::smInstance->getBool(0x1038C)) {
			kill();
		}
		mLiveFlag |= 0x8;
	}
}

// rogue includes for static init (JALList<*> instantiations)
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSSetSound.hpp>
