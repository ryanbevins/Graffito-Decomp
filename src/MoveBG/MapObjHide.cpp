#include <MoveBG/MapObjHide.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/Item.hpp>
#include <MoveBG/MapObjBall.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <Strategic/HitActor.hpp>
#include <Player/MarioAccess.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <System/EmitterViewObj.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <JSystem/JSupport/JSUInputStream.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//
// TWoodBox
//

TWoodBox::TWoodBox(const char* name)
    : TBreakHideObj(name)
{
}

void TWoodBox::loadAfter()
{
	TMapObjBase::loadAfter();
	unk138 = TMapObjBaseManager::newAndRegisterObjByEventID(unk134, mName);
	if (unk138) {
		bool isBlueCoin = (unk138->mActorType == 0x20000010) ? true : false;
		if (isBlueCoin) {
			if (TFlagManager::smInstance->getBlueCoinFlag(
			        gpMarDirector->mMap, (u8)unk134))
				unk14C = 0;
		}
		bool isShine = (unk138->mActorType == 0x20000013) ? true : false;
		if (isShine) {
			int nlen = strlen(mName);
			unk144   = (u32) new char[nlen + 0x13];
			snprintf((char*)unk144, nlen + 0x13, "shine_%s", mName);
		}
	}
	checkOnManhole();
}

void TWoodBox::kill()
{
	startAnim(2);
	removeMapCollision();
	unk64 |= 1;
	mLiveFlag |= 0x18;
	mLifeTimer = -1;
	mState     = 2;

	if (gpMSound->gateCheck(0x380a))
		MSoundSESystem::MSoundSE::startSoundActor(0x380a, (Vec*)&mPosition, 0, 0,
		                                          0, 4);

	const TBGCheckData* bg0;
	f32 gy0 = gpMap->checkGround(gpMarioPos->x + -50.0f,
	                             gpMarioPos->y + 1000.0f,
	                             gpMarioPos->z + -50.0f, &bg0);
	if (10.0f + gy0 > gpMarioPos->y && bg0->mActor && bg0->mActor != this) {
		bool is1c = (bg0->mActor->mActorType == 0x4000001c) ? true : false;
		if (is1c)
			((TMapObjBase*)bg0->mActor)->kill();
	}
	const TBGCheckData* bg1;
	f32 gy1 = gpMap->checkGround(gpMarioPos->x + 50.0f,
	                             gpMarioPos->y + 1000.0f,
	                             gpMarioPos->z + -50.0f, &bg1);
	if (10.0f + gy1 > gpMarioPos->y && bg1->mActor && bg1->mActor != this) {
		bool is1c = (bg1->mActor->mActorType == 0x4000001c) ? true : false;
		if (is1c)
			((TMapObjBase*)bg1->mActor)->kill();
	}
	const TBGCheckData* bg2;
	f32 gy2 = gpMap->checkGround(gpMarioPos->x + -50.0f,
	                             gpMarioPos->y + 1000.0f,
	                             gpMarioPos->z + 50.0f, &bg2);
	if (10.0f + gy2 > gpMarioPos->y && bg2->mActor && bg2->mActor != this) {
		bool is1c = (bg2->mActor->mActorType == 0x4000001c) ? true : false;
		if (is1c)
			((TMapObjBase*)bg2->mActor)->kill();
	}
	const TBGCheckData* bg3;
	f32 gy3 = gpMap->checkGround(gpMarioPos->x + 50.0f,
	                             gpMarioPos->y + 1000.0f,
	                             gpMarioPos->z + 50.0f, &bg3);
	if (10.0f + gy3 > gpMarioPos->y && bg3->mActor && bg3->mActor != this) {
		bool is1c = (bg3->mActor->mActorType == 0x4000001c) ? true : false;
		if (is1c)
			((TMapObjBase*)bg3->mActor)->kill();
	}
}

//
// TBreakHideObj
//

void TBreakHideObj::initMapObj()
{
	TMapObjBase::initMapObj();
	bool isWaterMelon = (mActorType == 0x400002c3) ? true : false;
	if (isWaterMelon) {
		SMS_LoadParticle("/scene/mapObj/WaterMelonBlockA.jpa", 0x6b);
		SMS_LoadParticle("/scene/mapObj/WaterMelonBlockB.jpa", 0x6c);
	}
}

void TBreakHideObj::control()
{
	TMapObjBase::control();
	switch (mState) {
	case 2:
		if (animIsFinished()) {
			emitEffect();
			kill();
		}
		break;
	}
}

BOOL TBreakHideObj::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 1) {
		bool isWaterMelon = (mActorType == 0x400002c3) ? true : false;
		if (isWaterMelon) {
			emitAndScale(0x6b, 0, &mPosition);
			emitAndScale(0x6c, 0, &mPosition);
			if (gpMSound->gateCheck(0x38a3))
				MSoundSESystem::MSoundSE::startSoundActor(0x38a3, (Vec*)&mPosition,
				                                          0, 0, 0, 4);
		}
		kill();
		return TRUE;
	}
	return FALSE;
}

void TBreakHideObj::kill()
{
	startAnim(2);
	removeMapCollision();
	unk64 |= 1;
	mLiveFlag |= 0x18;
	mLifeTimer = -1;
	mState     = 2;
}

//
// THideObjPictureTwin
//

THideObjPictureTwin::THideObjPictureTwin(const char* name)
    : TWaterHitPictureHideObj(name)
{
	unk174 = 0;
	memset(unk178, 0, 0x19);
}

void THideObjPictureTwin::initMapObj()
{
	TMapObjBase::initMapObj();
	snprintf(unk178, 0x19, "%sCamera", mName);
}

void THideObjPictureTwin::loadAfter()
{
	TWaterHitPictureHideObj::loadAfter();
	if (strstr(mName, "ふたご落書きＡ")) {
		char buf[0x40];
		int len   = strlen("ふたご落書きＡ");
		char c0   = mName[len];
		char c1   = mName[len + 1];
		char c2   = mName[len + 2];
		char c3   = mName[len + 3];
		snprintf(buf, 0x40, "ふたご落書きＢ００");
		buf[len]     = c0;
		buf[len + 1] = c1;
		buf[len + 2] = c2;
		buf[len + 3] = c3;
		unk174 = (TMapObjBase*)JDrama::TNameRefGen::instance->mRootNameRef->search(buf);
		((THideObjPictureTwin*)unk174)->unk174 = (TMapObjBase*)this;
	}
}

void THideObjPictureTwin::afterFinishedAnim()
{
	removeMapCollision();
	unk64 |= 1;
	TMapObjBase* obj = unk138;
	if (unk138 && unk14C) {
		if (TMapObjBase::isCoin(unk138)) {
			bool isPlainCoin = (unk138->mActorType == 0x2000000e) ? true : false;
			if (isPlainCoin) {
				obj = gpItemManager->makeObjAppear(0x2000000e);
				if (!obj)
					return;
			}
			((TCoin*)obj)->appearWithoutSound();
		} else {
			obj->appear();
		}
		obj->mPosition.set(unk174->mPosition);
		Mtx localMtx;
		MsMtxSetRotRPH(localMtx, unk174->mRotation.x, unk174->mRotation.y,
		               unk174->mRotation.z);
		f32 fwd = unk13C;
		f32 mx  = localMtx[0][2];
		f32 my  = localMtx[1][2];
		f32 vy  = unk140;
		f32 mz  = localMtx[2][2];
		obj->mVelocity.x = mx * fwd;
		obj->mVelocity.y = my * fwd + vy;
		obj->mVelocity.z = mz * fwd;
		obj->mLiveFlag &= ~0x10;
		if (TMapObjBase::isCoin(obj)) {
			((TItem*)obj)->unk148 = (THitActor*)this;
			((TItem*)obj)->unk14C = unk148;
		}
		if (gpMSound->gateCheck(0x4843))
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x4843, 0, 0, 0);
		gpMarDirector->fireStartDemoCamera(unk178, (JGeometry::TVec3<f32>*)&obj->mPosition,
		                                   -1, 0.0f, true, 0, 0, 0,
		                                   JDrama::TFlagT<u16>(0));
	}
	mState = 3;
}

Vec* THideObjPictureTwin::getObjAppearPos() const { return (Vec*)&mPosition; }

//
// TWaterHitPictureHideObj
//

TWaterHitPictureHideObj::TWaterHitPictureHideObj(const char* name)
    : THideObjBase(name)
{
	unk150 = 1;
	unk154 = 0.0f;
	unk158 = 0.0f;
	unk15C = 0.0f;
	unk160 = 0.0f;
	unk164 = 0.0f;
	unk168 = 255.0f;
	unk16C = 0;
	unk16E = 0;
	unk170 = 0;
	unk172 = 0;
}

void TWaterHitPictureHideObj::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	long manholeFlag;
	loadHideObjInfo(stream, &manholeFlag, &unk13C, &unk140, (long*)&unk148);
	unk134 = manholeFlag;
	SMS_LoadParticle("/scene/mapObj/ms_watcoin_hit.jpa", 0x57);

	u32 r, g, b;
	stream.read(&r, 4);
	stream.read(&g, 4);
	stream.read(&b, 4);
	unk16C = (u16)(u8)r;
	unk16E = (u16)(u8)g;
	unk170 = (u16)(u8)b;
	unk172 = 0xff;
	unk154 = 1.8f;
	unk158 = 1.0f;
	unk15C = 0.4f;
}

#pragma dont_inline on
void TWaterHitPictureHideObj::loadAfter()
{
	TMapObjBase::loadAfter();
	unk138 = TMapObjBaseManager::newAndRegisterObjByEventID(unk134, mName);
	if (unk138) {
		bool isBlueCoin = (unk138->mActorType == 0x20000010) ? true : false;
		if (isBlueCoin) {
			if (TFlagManager::smInstance->getBlueCoinFlag(
			        gpMarDirector->mMap, (u8)unk134))
				unk14C = 0;
		}
		bool isShine = (unk138->mActorType == 0x20000013) ? true : false;
		if (isShine) {
			int nlen = strlen(mName);
			unk144   = (u32) new char[nlen + 0x13];
			snprintf((char*)unk144, nlen + 0x13, "shine_%s", mName);
		}
	}
	unk160 = unk150 ? unk168 : unk164;
}
#pragma dont_inline off

BOOL TWaterHitPictureHideObj::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 5) {
		bool any = (sender->mActorType == 0x2000000e) ? true : false;
		if (!any)
			any = (sender->mActorType == 0x2000000f) ? true : false;
		if (!any)
			any = (sender->mActorType == 0x20000010) ? true : false;
		if (any) {
			offHitFlag(0x1);
			offHitFlag(0x4);
			offHitFlag(0x2);
			mState = 1;
			unk14C = 1;
			return TRUE;
		}
	}
	bool isS3 = (mState == 3) ? true : false;
	if (isS3)
		return FALSE;
	bool is5a = (sender->mActorType == 0x4000005a) ? true : false;
	if (is5a) {
		mState = 2;
		return TRUE;
	}
	if (message == 5) {
		bool any = (sender->mActorType == 0x2000000e) ? true : false;
		if (!any)
			any = (sender->mActorType == 0x2000000f) ? true : false;
		if (!any)
			any = (sender->mActorType == 0x20000010) ? true : false;
		if (any)
			unk14C = 1;
	}
	return TMapObjBase::receiveMessage(sender, message);
}

void TWaterHitPictureHideObj::control()
{
	TMapObjBase::control();
	switch (mState) {
	case 1:
		if (unk150) {
			unk160 += unk15C;
			if (unk160 > unk168)
				unk160 = unk168;
		} else {
			unk160 -= unk15C;
			if (unk160 < unk164)
				unk160 = unk164;
		}
		unk172 = (u16)(u8)(s32)unk160;
		break;
	case 2:
		forward(unk158);
		break;
	}
}

void TWaterHitPictureHideObj::touchActor(THitActor* sender)
{
	bool match = (sender->mActorType == 0x4000005a) ? true : false;
	if (match)
		mState = 2;
}

u32 TWaterHitPictureHideObj::touchWater(THitActor* sender)
{
	JGeometry::TVec3<f32>* speed = TMapObjBase::getWaterSpeed(sender);
	J3DModel* model              = getModel();
	Mtx* nm                      = (Mtx*)model->mNodeMatrices;
	f32 dot                      = (*nm)[1][0] * speed->x + (*nm)[1][1] * speed->y
	          + (*nm)[1][2] * speed->z;
	if (dot > 0.0f)
		return 0;
	forward(unk154);
	if (mActorType == 0x400001a1)
		soundBas(0x296e, unk154, 200.0f);
	return 1;
}

void TWaterHitPictureHideObj::forward(f32 amt)
{
	if (unk150) {
		unk160 -= amt;
		if (unk160 < unk164)
			afterFinishedAnim();
	} else {
		unk160 += amt;
		if (unk160 > unk168)
			afterFinishedAnim();
	}
	f32 hi      = unk168;
	f32 clamped = unk160;
	f32 lo      = unk164;
	if (clamped > hi)
		clamped = hi;
	else if (clamped < lo)
		clamped = lo;
	unk160 = clamped;
	unk172 = (u16)(u8)(s32)unk160;
}

void TWaterHitPictureHideObj::afterFinishedAnim()
{
	removeMapCollision();
	unk64 |= 4;
	Vec* pos = getObjAppearPos();
	appearObjFromPoint(*(JGeometry::TVec3<f32>*)pos);
	mState = 3;
}

Vec* TWaterHitPictureHideObj::getObjAppearPos() const
{
	return (Vec*)&mPosition;
}

//
// THipDropHideObj
//

void THipDropHideObj::touchPlayer(THitActor* sender)
{
	if (SMS_IsMarioStatusHipDrop()) {
		appearObj(0.0f);
		makeObjDead();
	}
}

//
// TFruitBasketEvent
//

TFruitBasketEvent::TFruitBasketEvent(const char* name)
    : TFruitBasket(name)
{
	unk150          = 0;
	mFruitCounts[0] = 0;
	mFruitCounts[1] = 0;
	mFruitCounts[2] = 0;
	mFruitCounts[3] = 0;
	mFruitCounts[4] = 0;
}

void TFruitBasketEvent::reset()
{
	mFruitCounts[0] = 0;
	mFruitCounts[1] = 0;
	mFruitCounts[2] = 0;
	mFruitCounts[3] = 0;
	mFruitCounts[4] = 0;
}

int TFruitBasketEvent::getFruitNum(int i) const { return mFruitCounts[i]; }

void TFruitBasketEvent::countFruit(THitActor* sender)
{
	TFruitBasket::countFruit(sender);
	switch (sender->mActorType) {
	case 0x40000390:
		mFruitCounts[0]++;
		break;
	case 0x40000391:
		mFruitCounts[2]++;
		break;
	case 0x40000392:
		mFruitCounts[3]++;
		break;
	case 0x40000393:
		mFruitCounts[1]++;
		break;
	case 0x40000394:
		mFruitCounts[4]++;
		break;
	case 0x40000395:
		break;
	default:
		return;
	}
	((TResetFruit*)sender)->makeObjWaitingToAppear();
	mColCount = 0;
}

//
// TFruitBasket
//

void TFruitBasket::loadAfter()
{
	TMapObjBase::loadAfter();
	unk138 = TMapObjBaseManager::newAndRegisterObjByEventID(unk134, mName);
	if (unk138) {
		bool isBlueCoin = (unk138->mActorType == 0x20000010) ? true : false;
		if (isBlueCoin) {
			if (TFlagManager::smInstance->getBlueCoinFlag(
			        gpMarDirector->mMap, (u8)unk134))
				unk14C = 0;
		}
		bool isShine = (unk138->mActorType == 0x20000013) ? true : false;
		if (isShine) {
			int nlen = strlen(mName);
			unk144   = (u32) new char[nlen + 0x13];
			snprintf((char*)unk144, nlen + 0x13, "shine_%s", mName);
		}
	}
	if (mRotation.x != 0.0f) {
		mAttackRadius = 400.0f;
		calcEntryRadius();
		mAttackHeight = 200.0f;
		calcEntryRadius();
	}
}

void TFruitBasket::touchFruit(THitActor* sender)
{
	TFruitHitHideObj::touchFruit(sender);
}

void TFruitBasket::countFruit(THitActor* sender)
{
	mMActor->setBck("basket");
	if (unk138) {
		appearObj(0.0f);
		if (gpMSound->gateCheck(0x3809))
			MSoundSESystem::MSoundSE::startSoundActor(0x3809, (Vec*)&mPosition, 0,
			                                          0, 0, 4);
		if (gpMSound->gateCheck(0x480a))
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x480a, 0, 0, 0);
	} else if (!unk150) {
		if (gpMSound->gateCheck(0x384e))
			MSoundSESystem::MSoundSE::startSoundActor(0x384e, (Vec*)&mPosition, 0,
			                                          0, 0, 4);
	} else {
		bool match = (sender->mActorType == unk150) ? true : false;
		if (match) {
			if (gpMSound->gateCheck(0x3809))
				MSoundSESystem::MSoundSE::startSoundActor(0x3809, (Vec*)&mPosition,
				                                          0, 0, 0, 4);
			if (gpMSound->gateCheck(0x480a))
				MSoundSESystem::MSoundSE::startSoundSystemSE(0x480a, 0, 0, 0);
		} else {
			if (gpMSound->gateCheck(0x483d))
				MSoundSESystem::MSoundSE::startSoundSystemSE(0x483d, 0, 0, 0);
		}
	}
	((TResetFruit*)sender)->makeObjWaitingToAppear();
}

//
// TFruitHitHideObj
//

void TFruitHitHideObj::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	long manholeFlag;
	loadHideObjInfo(stream, &manholeFlag, &unk13C, &unk140, (long*)&unk148);
	unk134 = manholeFlag;
	SMS_LoadParticle("/scene/mapObj/ms_watcoin_hit.jpa", 0x57);
}

void TFruitHitHideObj::touchActor(THitActor* sender)
{
	if (TMapObjBase::isFruit(sender))
		touchFruit(sender);
}

void TFruitHitHideObj::touchFruit(THitActor* sender)
{
	if (unk138) {
		appearObj(50.0f);
		emitEffect();
		makeObjDead();
	}
	((TMapObjBase*)sender)->kill();
}

//
// TWaterHitHideObj
//

void TWaterHitHideObj::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	long manholeFlag;
	loadHideObjInfo(stream, &manholeFlag, &unk13C, &unk140, (long*)&unk148);
	unk134 = manholeFlag;
	SMS_LoadParticle("/scene/mapObj/ms_watcoin_hit.jpa", 0x57);
}

u32 TWaterHitHideObj::touchWater(THitActor* sender)
{
	if (unk138) {
		appearObj(50.0f);
		makeObjDead();
	}
	return 1;
}

//
// THideObj
//

void THideObj::touchPlayer(THitActor* sender)
{
	if (marioHeadAttack() && unk138) {
		appearObj(100.0f);
		makeObjDead();
	}
}

//
// THideObjBase
//

THideObjBase::THideObjBase(const char* name)
    : TMapObjBase(name)
{
	unk138   = 0;
	unk13C = 0.0f;
	unk140 = 0.0f;
	unk144 = 0;
	unk148 = 0;
	unk14C = 1;
}

void THideObjBase::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	long manholeFlag;
	loadHideObjInfo(stream, &manholeFlag, &unk13C, &unk140, (long*)&unk148);
	unk134 = manholeFlag;
	SMS_LoadParticle("/scene/mapObj/ms_watcoin_hit.jpa", 0x57);
}

void THideObjBase::loadAfter()
{
	TMapObjBase::loadAfter();
	unk138 = TMapObjBaseManager::newAndRegisterObjByEventID(unk134, mName);
	if (unk138) {
		bool isBlueCoin = (unk138->mActorType == 0x20000010) ? true : false;
		if (isBlueCoin) {
			if (TFlagManager::smInstance->getBlueCoinFlag(
			        gpMarDirector->mMap, (u8)unk134))
				unk14C = 0;
		}
		bool isShine = (unk138->mActorType == 0x20000013) ? true : false;
		if (isShine) {
			int nlen = strlen(mName);
			unk144   = (u32) new char[nlen + 0x13];
			snprintf((char*)unk144, nlen + 0x13, "shine_%s", mName);
		}
	}
}

BOOL THideObjBase::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 5) {
		bool any = (sender->mActorType == 0x2000000e) ? true : false;
		if (!any)
			any = (sender->mActorType == 0x2000000f) ? true : false;
		if (!any)
			any = (sender->mActorType == 0x20000010) ? true : false;
		if (any)
			unk14C = 1;
	}
	return TMapObjBase::receiveMessage(sender, message);
}

void THideObjBase::emitEffect()
{
	gpMarioParticleManager->emit(0x57, &mPosition, 0, 0);
}

void THideObjBase::appearObjFromPoint(const JGeometry::TVec3<f32>& pt)
{
	if (unk138 && unk14C) {
		bool isShine = (unk138->mActorType == 0x20000013) ? true : false;
		if (isShine) {
			unk138->mLiveFlag |= 0x10;
			unk138->mPosition.set(mPosition);
			((TShine*)unk138)->appearWithDemo((const char*)unk144);
		} else {
			bool isCoin = (unk138->mActorType == 0x2000000e) ? true : false;
			TMapObjBase* obj;
			if (isCoin) {
				obj = gpItemManager->makeObjAppear(0x2000000e);
				if (!obj)
					return;
			} else {
				obj = unk138;
			}
			TMapObjBase::throwObjToFrontFromPoint(obj, pt, unk13C, unk140);
			if (TMapObjBase::isCoin(obj)) {
				((TItem*)obj)->unk148 = (THitActor*)this;
				((TItem*)obj)->unk14C = unk148;
			}
			if (TMapObjBase::isFruit(obj))
				((TResetFruit*)obj)->makeObjLiving();
			emitEffect();
		}
		unk14C = 0;
	}
}

void THideObjBase::appearObj(f32 yOff)
{
	JGeometry::TVec3<f32> pt(mPosition.x, mPosition.y + yOff, mPosition.z);
	appearObjFromPoint(pt);
}
