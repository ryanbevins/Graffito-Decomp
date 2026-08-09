#define LIVEACTOR_GETMACTOR_OUT_OF_LINE
#include <MoveBG/MapObjRicco.hpp>
#include <MoveBG/MapObjBall.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/Item.hpp>
#include <Map/MapCollisionManager.hpp>
#include <Map/MapCollisionEntry.hpp>
#define MAP_COLLISION_ENTRY_DEFINE_SET_UP_TRANS
#include <Map/MapCollisionEntry.hpp>
#undef MAP_COLLISION_ENTRY_DEFINE_SET_UP_TRANS
#include <Strategic/HitActor.hpp>
#include <Strategic/LiveActor.hpp>
#include <System/MarDirector.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/Particles.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JGeometry/JGUtil.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <dolphin/mtx.h>
#include <string.h>

#include <M3DUtil/InfectiousStrings.hpp>

s32 TCraneRotY::mWaitTime = 120;
f32 TCraneUpDown::mRotSpeed = 0.1f;
s32 TCraneUpDown::mWaitTime = 120;
f32 TRiccoWatermill::mRotAccel = 1.0f;
f32 TRiccoWatermill::mRotSpeedMaxUp = 3.0f;
f32 TRiccoWatermill::mRotSpeedMaxDown = 1.0f;
f32 TRiccoWatermill::mRotDown = 0.05f;
f32 TRiccoWatermill::mSubmarineMoveRate = 0.5f;
f32 TRiccoWatermill::mSubmarineMaxTransY = 750.0f;
f32 TRiccoWatermill::mSubmarineBottomTransY = -950.0f;
s32 TRiccoWatermill::mWaitTime = 600;
f32 TRiccoWatermill::mSubmarineSurfaceTransY;
f32 TFruitLauncher::mObjSpeedXZ = 1.0f;
f32 TFruitLauncher::mObjSpeedY = 20.0f;
s32 TFruitLauncher::mFruitLiveTime = 4800;

Vec submarineCranePos_forSound;

void TCraneRotY::calc()
{
	setRootMtxRotY();
}

void TCraneRotY::control()
{
	TMapObjBase::control();
	switch (mState) {
	case 0:
		mRotation.y += unk144;
		if (mRotation.y > unk138 + unk140) {
			mLifeTimer = mWaitTime;
			mState     = 3;
		}
		break;
	case 1:
		if (!(mLifeTimer > 0 ? true : false)) {
			mState = 0;
		}
		break;
	case 2:
		mRotation.y -= unk144;
		if (mRotation.y < unk138 + unk13C) {
			mLifeTimer = mWaitTime;
			mState     = 1;
		}
		break;
	case 3:
		if (!(mLifeTimer > 0 ? true : false)) {
			mState = 2;
		}
		break;
	}
	if ((mState == 0 ? true : false) || (mState == 2 ? true : false)) {
		u32 sound = unk148;
		if (gpMSound->gateCheck(sound)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    sound, (Vec*)&mPosition, 0, 0, 0, 4);
		}
	}
}

void TCraneRotY::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	stream.read(&unk140, 4);
	unk138 = mRotation.y;
	unk144 = 0.5f + MsRandF() * 0.3f;
	if (strcmp(mName, "crane90 0") == 0) {
		unk148 = 0x3034;
	} else {
		unk148 = 0x3035;
	}
	mState = 0;
}

void TCraneUpDown::control()
{
	TMapObjBase::control();
	switch (mState) {
	case 0:
		mRotation.x += mRotSpeed;
		if (mRotation.x > unk140) {
			mLifeTimer = mWaitTime;
			mState     = 3;
		}
		break;
	case 1:
		if (!(mLifeTimer > 0 ? true : false)) {
			mState = 0;
		}
		break;
	case 2:
		mRotation.x -= mRotSpeed;
		if (mRotation.x < unk144) {
			mLifeTimer = mWaitTime;
			mState     = 1;
		}
		break;
	case 3:
		if (!(mLifeTimer > 0 ? true : false)) {
			mState = 2;
		}
		break;
	}
	{
		JGeometry::TVec3<f32>* pos = (JGeometry::TVec3<f32>*)&unk138->mPosition;
		pos->x                     = 0.0f;
		pos->y                     = 0.0f;
		pos->z                     = 100.0f;
		MtxPtr rotY                = getModel()->mNodeMatrices[0];
		Mtx scratch;
		PSMTXIdentity(scratch);
		s16 angY  = (s16)(mRotation.x * 182.04445f);
		f32 sinY  = jmaSinTable[(u16)angY >> jmaSinShift];
		f32 cosY  = jmaCosTable[(u16)angY >> jmaSinShift];
		rotY[0][0] = 1.0f;
		rotY[0][1] = 0.0f;
		rotY[0][2] = 0.0f;
		rotY[0][3] = 0.0f;
		rotY[1][0] = 0.0f;
		rotY[1][1] = cosY;
		rotY[1][2] = -sinY;
		rotY[1][3] = 0.0f;
		rotY[2][0] = 0.0f;
		rotY[2][1] = sinY;
		rotY[2][2] = cosY;
		rotY[2][3] = 0.0f;
		s16 angZ   = (s16)(mRotation.y * 182.04445f);
		f32 sinZ   = jmaSinTable[(u16)angZ >> jmaSinShift];
		f32 cosZ   = jmaCosTable[(u16)angZ >> jmaSinShift];
		scratch[0][0] = cosZ;
		scratch[0][1] = 0.0f;
		scratch[0][2] = sinZ;
		scratch[0][3] = 0.0f;
		scratch[1][0] = 0.0f;
		scratch[1][1] = 1.0f;
		scratch[1][2] = 0.0f;
		scratch[1][3] = 0.0f;
		scratch[2][0] = -sinZ;
		scratch[2][1] = 0.0f;
		scratch[2][2] = cosZ;
		scratch[2][3] = 0.0f;
		PSMTXConcat(rotY, scratch, rotY);
		PSMTXMultVec(rotY, (Vec*)&unk138->mPosition, (Vec*)&unk138->mPosition);
		unk138->mPosition.x += mPosition.x;
		unk138->mPosition.y += mPosition.y - mYOffset + unk138->mYOffset;
		unk138->mPosition.z += mPosition.z;
	}
	if ((mState == 2 ? true : false) || (mState == 0 ? true : false)) {
		u32 sound = unk13C;
		if (gpMSound->gateCheck(sound)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    sound, (Vec*)&mPosition, 0, 0, 0, 4);
		}
	}
}

void TCraneUpDown::initMapObj()
{
	TMapObjBase::initMapObj();
	mMapCollisionManager->unk8->setAllActor(0);
	unk138 = TMapObjBaseManager::newAndRegisterObj(
	    "craneCargoUpDown", JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f),
	    JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f),
	    JGeometry::TVec3<f32>(1.0f, 1.0f, 1.0f));
	unk138->appear();
	if (strcmp(mName, "craneUpDown 0") == 0) {
		unk144 = 30.0f;
		unk140 = -30.0f;
		unk13C = 0x3036;
	} else {
		unk144 = 30.0f;
		unk140 = -60.0f;
		unk13C = 0x3037;
	}
	mRotation.x = unk144 + (unk140 - unk144) * MsRandF();
}

void TCraneCargo::control()
{
	unk158.z = 0.0f;
	unk158.y = 0.0f;
	unk158.x = 0.0f;
	TMapObjBase::control();
}

void TCraneCargo::calc()
{
	updateRootMtxTrans();
	calcLeanMtx((MtxPtr)((u8*)getModel()->mNodeMatrices + 0x30));
}

u32 TRiccoWatermill::touchWater(THitActor* sender)
{
	if ((mState == 5) ? true : false) {
		return 1;
	}
	unk140 = 5;
	if ((mState == 1) ? true : false) {
		unk13C->setUpMapCollision(1);
	}
	unkF8 &= ~0x100;
	unk13C->unkF8 &= ~0x100;
	if (unk13C->mPosition.y < mSubmarineMaxTransY) {
		unk138 += mRotAccel;
		if (unk138 > mRotSpeedMaxUp) {
			unk138 = mRotSpeedMaxUp;
		}
		mState = 2;
	} else {
		unk138 = 0.0f;
	}
	return 1;
}

void TRiccoWatermill::control()
{
	TMapObjBase::control();
	if ((mState == 2 ? true : false) || (mState == 3 ? true : false)
	    || (mState == 4 ? true : false)) {
		if (0.0f != unk138) {
			mRotation.z -= unk138;
			f32 absRot = __fabsf(unk138);
			if (gpMSound->gateCheck(0x3031)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x3031, (Vec*)&mPosition, (Vec*)NULL, absRot, 0, 0,
				    (JAISound**)&unk14C, 0, 4);
			}
			f32 rate    = unk138 * mSubmarineMoveRate;
			unk13C->mPosition.y += rate;
			f32 absRate = __fabsf(rate);
			if (gpMSound->gateCheck(0x3030)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x3030, &submarineCranePos_forSound, (Vec*)NULL,
				    absRate, 0, 0, (JAISound**)&unk150, 0, 4);
			}
			absRate = __fabsf(rate);
			if (gpMSound->gateCheck(0x3023)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x3023, &submarineCranePos_forSound, (Vec*)NULL,
				    absRate, 0, 0, (JAISound**)&unk154, 0, 4);
			}
		}
		if (unk140 == 0) {
			unk138 -= mRotDown;
			if (unk138 < -mRotSpeedMaxDown) {
				unk138 = -mRotSpeedMaxDown;
			}
		} else {
			unk140--;
		}
	}
	switch (mState) {
	case 2:
		if (unk13C->mPosition.y > mSubmarineMaxTransY) {
			unk13C->mPosition.y = mSubmarineMaxTransY;
			if (!(mLifeTimer > 0 ? true : false)) {
				if (gpMSound->gateCheck(0x3832)) {
					MSoundSESystem::MSoundSE::startSoundActor(
					    0x3832, (Vec*)&unk13C->mPosition, 0, 0, 0, 4);
				}
				if (!unk144) {
					JGeometry::TVec3<f32> target(
					    50.0f, mSubmarineMaxTransY + 200.0f, 100.0f);
					throwObjToFrontFromPoint(unk148, target, 0.0f, 0.0f);
				}
				unk144 = 1;
			}
			if (unk138 < 0.0f) {
				if (unk144) {
					mState = 3;
				} else {
					mState = 4;
				}
			}
		}
		break;
	case 3:
		if (unk13C->mPosition.y <= mSubmarineSurfaceTransY) {
			unk13C->mPosition.y = mSubmarineSurfaceTransY;
			unk13C->setUpMapCollision(0);
			unk138 = 0.0f;
			if (gpMSound->gateCheck(0x3832)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x3832, (Vec*)&unk13C->mPosition, 0, 0, 0, 4);
			}
			mLifeTimer = mWaitTime;
			mState     = 5;
		}
		break;
	case 4:
		if (unk13C->mPosition.y <= mSubmarineBottomTransY) {
			unk13C->mPosition.y = mSubmarineBottomTransY;
			unk13C->setUpMapCollision(0);
			unk138 = 0.0f;
			unk13C->unkF8 |= 0x100;
			unkF8 |= 0x100;
			if (gpMSound->gateCheck(0x3833)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x3833, (Vec*)&unk13C->mPosition, 0, 0, 0, 4);
			}
			mState = 1;
		}
		break;
	}
}

void TRiccoWatermill::calc()
{
	setRootMtxRotZ();
}

void TRiccoWatermill::loadAfter()
{
	TMapObjBase::loadAfter();
	JDrama::TNameRef* root = JDrama::TNameRefGen::instance->mRootNameRef;
	unk13C = (TMapObjBase*)root->searchF(
	    JDrama::TNameRef::calcKeyCode("submarine"), "submarine");
	root = JDrama::TNameRefGen::instance->mRootNameRef;
	unk148 = (TMapObjBase*)root->searchF(
	    JDrama::TNameRef::calcKeyCode("水中スイッチ・押す擬似"),
	    "水中スイッチ・押す擬似");
	unk148->makeObjDead();
	unk13C->mPosition.y = mSubmarineBottomTransY;
	unk13C->removeMapCollision();
	unk13C->setUpCurrentMapCollision();
}

TRiccoWatermill::TRiccoWatermill(const char* name)
    : TMapObjBase(name)
{
	unk138 = 0.0f;
	unk13C = 0;
	unk140 = 0;
	unk144 = 0;
	unk148 = 0;
	unk14C = 0;
	unk150 = 0;
	unk154 = 0;
}

void TSurfGesoObj::initMapObj()
{
	TMapObjBase::initMapObj();
	if (strcmp(unkF4, "SurfGesoRed") == 0) {
		unk154.r = 0xFF;
		unk154.g = 0xB4;
		unk154.b = 0xFF;
		unk154.a = 0xFF;
	} else if (strcmp(unkF4, "SurfGesoYellow") == 0) {
		unk154.r = 0xFF;
		unk154.g = 0xFF;
		unk154.b = 0x7D;
		unk154.a = 0xFF;
	} else if (strcmp(unkF4, "SurfGesoGreen") == 0) {
		unk154.r = 0xB4;
		unk154.g = 0xFF;
		unk154.b = 0xB4;
		unk154.a = 0xFF;
	}
	SDLModelData* modelData = gpMapObjManager->mSurfGessoModelData;
	mMActor                 = SMS_MakeMActorFromSDLModelData(
        modelData, gpMapObjManager->getMActorAnmData(), 3);
	initPacketMatColor(getModel(), GX_TEVREG1, &unk154);
	mMActor->setBck("surfgeso_run1");
}

BOOL TFruitSwitch::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 1) {
		startBck("riccoswitch");
		unk64 |= 1;
		if (TMapCollisionBase* coll = mMapCollisionManager->unk8) {
			coll->remove();
		}
		((TFruitLauncher*)unk138)->fireObj();
		return TRUE;
	}
	return FALSE;
}

#pragma dont_inline on
MActor* TLiveActor::getMActor() const { return mMActor; }
#pragma dont_inline off

void TFruitLauncher::fireObj()
{
	gpMarioParticleManager->emitAndBindToPosPtr(
	    0x11, (JGeometry::TVec3<f32>*)&mPosition, 0, 0);
	if (gpMSound->gateCheck(0x384C)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x384C, (Vec*)&mPosition, 0, 0, 0, 4);
	}
	if (gpMSound->gateCheck(0x387D)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x387D, (Vec*)&mPosition, 0, 0, 0, 4);
	}
	unk140              = unk140 == 0 ? 1 : 0;
	TFruitSwitch* sw    = (&unk138)[unk140];
	sw->getMActor()->getFrameCtrl(0)->setFrame(0.0f);
	sw->unk64 &= ~1;
	sw->getModel()->calc();
	PSMTXCopy(sw->getModel()->mNodeMatrices[0],
	          sw->mMapCollisionManager->unk8->unk20);
	sw->mMapCollisionManager->unk8->setUp();

	TMapObjBase* obj = (TMapObjBase*)NULL;
	{
		f32 r1 = MsRandF() * 100.0f;
		if (r1 < 20.0f) {
			obj = gpItemManager->makeObjAppear(sw->mPosition.x, sw->mPosition.y,
			                                   sw->mPosition.z, 0x4000390,
			                                   false);
		} else if (r1 < 40.0f) {
			obj = gpItemManager->makeObjAppear(sw->mPosition.x, sw->mPosition.y,
			                                   sw->mPosition.z, 0x4000391,
			                                   false);
		} else if (r1 < 60.0f) {
			obj = gpItemManager->makeObjAppear(sw->mPosition.x, sw->mPosition.y,
			                                   sw->mPosition.z, 0x4000392,
			                                   false);
		} else if (r1 < 80.0f) {
			obj = gpItemManager->makeObjAppear(sw->mPosition.x, sw->mPosition.y,
			                                   sw->mPosition.z, 0x4000393,
			                                   false);
		} else {
			obj = gpItemManager->makeObjAppear(sw->mPosition.x, sw->mPosition.y,
			                                   sw->mPosition.z, 0x4000394,
			                                   false);
		}
	}
	if (obj == NULL) {
		f32 r2 = MsRandF() * 100.0f;
		if (r2 < 20.0f) {
			obj = gpItemManager->makeObjAppear(sw->mPosition.x, sw->mPosition.y,
			                                   sw->mPosition.z, 0x4000390,
			                                   false);
		} else if (r2 < 40.0f) {
			obj = gpItemManager->makeObjAppear(sw->mPosition.x, sw->mPosition.y,
			                                   sw->mPosition.z, 0x4000391,
			                                   false);
		} else if (r2 < 60.0f) {
			obj = gpItemManager->makeObjAppear(sw->mPosition.x, sw->mPosition.y,
			                                   sw->mPosition.z, 0x4000392,
			                                   false);
		} else if (r2 < 80.0f) {
			obj = gpItemManager->makeObjAppear(sw->mPosition.x, sw->mPosition.y,
			                                   sw->mPosition.z, 0x4000393,
			                                   false);
		} else {
			obj = gpItemManager->makeObjAppear(sw->mPosition.x, sw->mPosition.y,
			                                   sw->mPosition.z, 0x4000394,
			                                   false);
		}
	}
	if (obj == NULL) {
		f32 r3 = MsRandF() * 100.0f;
		if (r3 < 20.0f) {
			obj = gpItemManager->makeObjAppear(sw->mPosition.x, sw->mPosition.y,
			                                   sw->mPosition.z, 0x4000390,
			                                   false);
		} else if (r3 < 40.0f) {
			obj = gpItemManager->makeObjAppear(sw->mPosition.x, sw->mPosition.y,
			                                   sw->mPosition.z, 0x4000391,
			                                   false);
		} else if (r3 < 60.0f) {
			obj = gpItemManager->makeObjAppear(sw->mPosition.x, sw->mPosition.y,
			                                   sw->mPosition.z, 0x4000392,
			                                   false);
		} else if (r3 < 80.0f) {
			obj = gpItemManager->makeObjAppear(sw->mPosition.x, sw->mPosition.y,
			                                   sw->mPosition.z, 0x4000393,
			                                   false);
		} else {
			obj = gpItemManager->makeObjAppear(sw->mPosition.x, sw->mPosition.y,
			                                   sw->mPosition.z, 0x4000394,
			                                   false);
		}
	}
	if (obj != NULL) {
		obj->mPosition.x = sw->mPosition.x;
		obj->mPosition.y = sw->mPosition.y;
		obj->mPosition.z = sw->mPosition.z;
		f32 speedY       = -mObjSpeedY;
		f32 speedXZ      = mObjSpeedXZ;
		f32 speedZ       = speedXZ * (MsRandF() - 0.5f);
		f32 speedX       = speedXZ * (MsRandF() - 0.5f);
		obj->mVelocity.x = speedX;
		obj->mVelocity.y = speedY;
		obj->mVelocity.z = speedZ;
		obj->mLiveFlag &= ~0x10;
		TMarDirector* director = gpMarDirector;
		director->fireStartDemoCamera(
		    "フルーツスイッチメラメラ",
		    (JGeometry::TVec3<f32>*)&obj->mPosition, -1, 0.0f, true,
		    (s32 (*)(u32, u32))NULL, 0, (JDrama::TActor*)NULL,
		    JDrama::TFlagT<u16>(0));
		if (gpMSound->gateCheck(0x4849)) {
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x4849, 0,
			                                             (JAISound**)NULL, 0);
		}
		if (isFruit(obj)) {
			((TResetFruit*)obj)->killByTimer(mFruitLiveTime);
		}
	}
}

void TFruitLauncher::loadAfter()
{
	TMapObjBase::loadAfter();
	((TResetFruit*)TMapObjBaseManager::newAndRegisterObj(
	     "FruitCoconut", JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f),
	     JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f),
	     JGeometry::TVec3<f32>(1.0f, 1.0f, 1.0f)))
	    ->unk1A4
	    = 1;
	((TResetFruit*)TMapObjBaseManager::newAndRegisterObj(
	     "FruitDurian", JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f),
	     JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f),
	     JGeometry::TVec3<f32>(1.0f, 1.0f, 1.0f)))
	    ->unk1A4
	    = 1;
	((TResetFruit*)TMapObjBaseManager::newAndRegisterObj(
	     "FruitPapaya", JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f),
	     JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f),
	     JGeometry::TVec3<f32>(1.0f, 1.0f, 1.0f)))
	    ->unk1A4
	    = 1;
	((TResetFruit*)TMapObjBaseManager::newAndRegisterObj(
	     "FruitPine", JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f),
	     JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f),
	     JGeometry::TVec3<f32>(1.0f, 1.0f, 1.0f)))
	    ->unk1A4
	    = 1;
	((TResetFruit*)TMapObjBaseManager::newAndRegisterObj(
	     "FruitBanana", JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f),
	     JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f),
	     JGeometry::TVec3<f32>(1.0f, 1.0f, 1.0f)))
	    ->unk1A4
	    = 1;

	JDrama::TNameRef* root = JDrama::TNameRefGen::instance->mRootNameRef;
	unk138 = (TFruitSwitch*)root->searchF(
	    JDrama::TNameRef::calcKeyCode("タンクスイッチＡ"), "タンクスイッチＡ");
	unk138->unk138 = (TMapObjBase*)this;
	root = JDrama::TNameRefGen::instance->mRootNameRef;
	unk13C = (TFruitSwitch*)root->searchF(
	    JDrama::TNameRef::calcKeyCode("タンクスイッチＢ"), "タンクスイッチＢ");
	unk13C->unk138 = (TMapObjBase*)this;
	unk140         = 1;
	TFruitSwitch* fruitSwitch = unk138;
	fruitSwitch->startBck("riccoswitch");
	fruitSwitch->unk64 |= 1;
	{
		TMapCollisionBase* coll = fruitSwitch->mMapCollisionManager->unk8;
		if (coll) {
			coll->remove();
		}
	}
}

#include <MSound/MSoundBGM.hpp>
#include <MSound/MSSetSound.hpp>
