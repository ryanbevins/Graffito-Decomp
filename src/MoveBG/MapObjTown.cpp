#define JDRAMA_TFLAG_CTOR_DECL_ONLY
#include <MoveBG/MapObjTown.hpp>
#undef JDRAMA_TFLAG_CTOR_DECL_ONLY
#include <MoveBG/Item.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Map/MapCollisionManager.hpp>
#include <M3DUtil/MActor.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioMain.hpp>
#include <System/Application.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>
#include <dolphin/mtx.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

TMapObjSwitch* gpMapObjSwitch;

u8 SMS_getShineIDofExStage(u8);

f32 TManhole::mDownHeight            = 12.0f;
f32 TManhole::mDownSpeed             = 1.5f;
f32 TManhole::mVibrationSpeed        = 0.05f;
f32 TManhole::mVibrationEndHeight    = 0.001f;
f32 TManhole::mVibrationDecreaseRate = 0.07f;

static inline f32 callMsWrap(f32 t, f32 l, f32 r)
{
	return MsWrap<f32>(t, l, r);
}

#define START_MAP_OBJ_SOUND(soundID, pos)                                      \
	do {                                                                       \
		if (gpMSound->gateCheck(soundID)) {                                    \
			MSoundSESystem::MSoundSE::startSoundActor(soundID, (Vec*)&(pos),   \
			                                          0, nullptr, 0, 4);       \
		}                                                                      \
	} while (0)

TDoor::TDoor(const char* name)
    : TMapObjBase(name)
    , unk138(0)
{
}

void TDoor::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);

	s32 doorType;
	stream.read(&doorType, 4);
	if (doorType != 0)
		unk138 = 1;
}

void TDoor::touchPlayer(THitActor* player)
{
	if (!player->isActorTypeOf(ACTOR_TYPE_PLAYER))
		return;

	u32 message = 0x11;
	if (unk138)
		message = 0x12;
	if (!player->receiveMessage(this, message))
		return;

	TMario* mario = (TMario*)player;
	if (mario->mAction == 0x1320) {
		if (mario->mHeldObject)
			startAnim(4);
		else
			startAnim(2);
	} else if (mario->mAction == 0x1321) {
		if (mario->mHeldObject)
			startAnim(3);
		else
			startAnim(1);
	}
}

TManhole::TManhole(const char* name)
    : TMapObjGeneral(name)
    , unk148(0.0f)
    , unk14C(0.0f)
    , unk150(0)
    , unk151(1)
    , unk152(0)
    , unk154(nullptr)
    , unk158(nullptr)
{
}

void TManhole::initMapObj()
{
	TMapObjGeneral::initMapObj();
	unk158 = new TMapCollisionWarp;
	unk158->init("/scene/mapObj/manholeRoof.col", 0, this);
}

void TManhole::loadAfter()
{
	TMapObjBase::loadAfter();
	mMActor->getFrameCtrl(0)->setRate(0.0f);
}

void TManhole::makeManholeUnuseful(const TMapObjBase* obj)
{
	if (unk154 != nullptr)
		return;

	unk154 = obj;
	J3DModel* model         = getModel();
	TMapCollisionWarp* warp = unk158;
	MTXCopy(model->mNodeMatrices[0], warp->unk20);
	warp->setUp();
}

void TManhole::setGroundCollision()
{
	void* yoshi = SMS_GetYoshi();
	int hasYoshi;
	if (!*(u8*)yoshi)
		hasYoshi = 0;
	else
		hasYoshi = 1;

	if (hasYoshi) {
		if (mPosition.x - mBodyRadius < *(f32*)((u8*)SMS_GetYoshi() + 0x20)
		    && mPosition.x + mBodyRadius > *(f32*)((u8*)SMS_GetYoshi() + 0x20)
		    && mPosition.z - mBodyRadius < *(f32*)((u8*)SMS_GetYoshi() + 0x28)
		    && mPosition.z + mBodyRadius
		           > *(f32*)((u8*)SMS_GetYoshi() + 0x28)) {
			if (mMapCollisionManager->unk8)
				mMapCollisionManager->unk8->moveTrans(mPosition);
			return;
		}
	}

	TMapObjBase::setGroundCollision();
}

void TManhole::calc()
{
	f32 sum = mMActor->getFrameCtrl(0)->getFrame()
	          + mMActor->getFrameCtrl(0)->getRate();

	if ((mMActor->getFrameCtrl(0)->getFrame() <= 45.0f && 45.0f < sum)
	    || (mMActor->getFrameCtrl(0)->getFrame() <= 125.0f && 125.0f < sum))
		START_MAP_OBJ_SOUND(0x383c, mPosition);
}

void TManhole::appeared()
{
	if (unk154 != nullptr) {
		if (unk154->checkLiveFlag(LIVE_FLAG_DEAD)) {
			unk158->remove();
			unk154 = nullptr;
		} else {
			return;
		}
	}

	if (unk150 != 0 && gpMarioOriginal->mVel.y <= 0.0f) {
		mMapCollisionManager->unk8->setAllBGType(0x107);
		unk150 = 0;
	}

	if (!(u8)animationFinished())
		return;

	if (unk152 == 1 && mColCount == 0) {
		unk152 = 0;
		START_MAP_OBJ_SOUND(0x383e, mPosition);
	}

	if (unk14C > mVibrationEndHeight) {
		s16 phase = (s16)(unk148 * 32768.0f);
		mPosition.y = mInitialPosition.y + unk14C * JMASCos(phase);
		unk148 += mVibrationSpeed;
		if (unk148 >= 2.0f)
			unk148 -= 2.0f;
		unk14C -= mVibrationDecreaseRate;
	} else {
		if (unk151 == 0) {
			setUpMapCollision(0);
			unk151 = 1;
		}
		onMapObjFlag(0x100);
		mPosition.y = mInitialPosition.y;
	}
}

BOOL TManhole::animationFinished()
{
	J3DFrameCtrl* frameCtrl = getMActor()->getFrameCtrl(0);
	if (frameCtrl->getRate() == 0.0f)
		return TRUE;

	f32 next = getMActor()->getFrameCtrl(0)->getFrame()
	           + getMActor()->getFrameCtrl(0)->getRate();
	if (frameCtrl->getFrame() < 79.0f && next >= 79.0f) {
		frameCtrl->setFrame(79.0f);
		frameCtrl->setRate(0.0f);
		calcRootMatrix();
		getModel()->calc();
		onMapObjFlag(0x100);
		return TRUE;
	}

	f32 end = frameCtrl->getEnd();
	if (frameCtrl->getFrame() < end && next >= end) {
		frameCtrl->setFrame(0.0f);
		frameCtrl->setRate(0.0f);
		calcRootMatrix();
		getModel()->calc();
		onMapObjFlag(0x100);
		return TRUE;
	}

	return FALSE;
}

void TManhole::touchPlayer(THitActor*)
{
	mState = 1;

	if (!(u8)animationFinished()) {
		mPosition.y = mInitialPosition.y;
		return;
	}

	if (gpMarioOriginal->mAction == 0x8008a9
	    && gpMarioOriginal->mPosition.y < mPosition.y) {
		mMActor->getFrameCtrl(0)->setRate(SMSGetAnmFrameRate());
		mMActor->getFrameCtrl(0)->setFrame(
		    mMActor->getFrameCtrl(0)->getFrame() + SMSGetAnmFrameRate());
		START_MAP_OBJ_SOUND(0x383b, mPosition);
		offMapObjFlag(0x100);
		SMSRumbleMgr->start(0x15, 0xf, (f32*)nullptr);
		return;
	}

	if (gpMarioOriginal->mPosition.y < mPosition.y
	    && gpMarioOriginal->mVel.y > 0.0f) {
		mMActor->getFrameCtrl(0)->setRate(SMSGetAnmFrameRate());
		mMActor->getFrameCtrl(0)->setFrame(
		    mMActor->getFrameCtrl(0)->getFrame() + SMSGetAnmFrameRate());
		offMapObjFlag(0x100);
		mMapCollisionManager->unk8->setAllBGType(0x400);
		START_MAP_OBJ_SOUND(0x383b, mPosition);
		unk150 = 1;
		SMSRumbleMgr->start(0x15, 0xf, (f32*)nullptr);
		return;
	}

	if (gpMarioOriginal->mPosition.y
	    <= 4.0f + gpMarioOriginal->mFloorPosition.y) {
		if (unk151 != 0) {
			setUpMapCollision(1);
			unk151 = 0;
		}
		if (unk152 == 0) {
			unk152 = 1;
			START_MAP_OBJ_SOUND(0x383d, mPosition);
		}
		f32 minY = mInitialPosition.y - mDownHeight;
		if (mPosition.y > minY)
			mPosition.y -= mDownSpeed;
		else
			mPosition.y = minY;
		unk148 = 1.0f;
		unk14C = mInitialPosition.y - mPosition.y;
		offMapObjFlag(0x100);
	} else {
		if (unk152 != 0) {
			unk152 = 0;
			START_MAP_OBJ_SOUND(0x383e, mPosition);
		}
		appeared();
	}
}

void TMapObjBillboard::touchActor(THitActor* actor)
{
	if (!animIsFinished()) {
		J3DFrameCtrl* frameCtrl = mMActor->getFrameCtrl(0);
		if (frameCtrl->getFrame() < 33.0f)
			return;
	}

	f32 angle = getRotYFromAxisX(actor->mPosition) * 57.295776f;
	angle     = mRotation.y + angle;
	angle     = callMsWrap(angle, 0.0f, 360.0f);
	if (angle < 0.0f || 180.0f < angle)
		startAnim(2);
	else
		startAnim(1);

	if (gpMSound->gateCheck(0x384f)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x384f, (Vec*)&mPosition, 0,
		                                          &unk150, 0, 4);
	}
}

u32 TMapObjBillboard::touchWater(THitActor* actor)
{
	if (animIsFinished()
	    || mMActor->getFrameCtrl(0)->getFrame() >= 33.0f) {
		f32 angle
		    = mRotation.y + getRotYFromAxisX(actor->mPosition) * 57.295776f;
		angle = callMsWrap(angle, 0.0f, 360.0f);
		if (angle < 0.0f || angle > 180.0f)
			startAnim(2);
		else
			startAnim(1);

		if (gpMSound->gateCheck(0x384f)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x384f, (Vec*)&mPosition, 0, &unk150, 0, 4);
		}
	}

	if (unk138 != nullptr && unk14C != 0) {
		JGeometry::TVec3<f32> rot = mRotation;
		JGeometry::TVec3<f32> pos = mPosition;
		rot.y -= 90.0f;
		pos.y += mYOffset;

		TMapObjBase* obj = nullptr;
		if (unk138->mActorType == 0x2000000e)
			obj = gpItemManager->makeObjAppear(0x2000000e);
		else
			obj = unk138;

		if (obj != nullptr) {
			TMapObjBase::throwObjFromPointWithRot(obj, pos, rot, unk13C,
			                                      unk140);
			emitEffect();
		}
		unk14C = 0;
	}

	return 1;
}

void TMapObjChangeStage::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);

	u32 stage;
	stream.read(&stage, 4);
	unk138 = stage;
}

void TMapObjChangeStage::touchPlayer(THitActor*)
{
	gpMarDirector->setNextStage(unk138, nullptr);
	onHitFlag(HIT_FLAG_NO_COLLISION);
	mColCount = 0;
	START_MAP_OBJ_SOUND(0x197a, mPosition);
}

void TMapObjChangeStageHipDrop::initMapObj()
{
	TMapObjBase::initMapObj();
	SMS_LoadParticle("/scene/mapObj/ms_ex_hahen.jpa", 0x63);
}

void TMapObjChangeStageHipDrop::touchPlayer(THitActor*)
{
	if (SMS_IsMarioStatusHipDrop()
	    && gpMarioPos->y + *gpMarioSpeedY < SMS_GetMarioGrLevel()) {
		gpMarDirector->setNextStage(unk138, nullptr);
		gpMarioParticleManager->emit(0x63, &mPosition, 0, nullptr);
	}
}

void TMapObjStartDemo::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	u32 demoID;
	stream.read(&demoID, 4);
	unk138 = demoID;
}

void TMapObjStartDemo::touchPlayer(THitActor*)
{
	TMarDirector* director = gpMarDirector;
	director->fireStreamingMovie((u8)unk138);
}

void TDamageObj::load(JSUMemoryInputStream& stream)
{
	JDrama::TActor::load(stream);

	char buf[32];
	stream.readString(buf, 32);

	if (strcmp(buf, "normal") == 0) {
		init(0x10000036);
	} else if (strcmp(buf, "water") == 0) {
		init(0x40000053);
	}
}

void TDamageObj::init(u32 actorType)
{
	initHitActor(actorType, 1, 0x80000000, mScaling.x * 50.0f,
	             mScaling.y * 100.0f, 0.0f, 0.0f);
	offHitFlag(HIT_FLAG_NO_COLLISION);
	TMapObjBase::joinToGroup("マップグループ", this);
}

void TDamageObj::perform(u32 flags, JDrama::TGraphics* graphics)
{
	THitActor::perform(flags, graphics);
	if (mColCount != 0)
		mCollisions[0]->receiveMessage(this, HIT_MESSAGE_ATTACK);
}

TMapObjWaterSpray::TMapObjWaterSpray(const char* name)
    : TMapObjBase(name)
    , unk138(0x154)
    , unk13C(0.0f)
    , unk14C(-1)
{
	unk14C.set(0, 0, 0, 0);
	unk140.zero();
}

void TMapObjWaterSpray::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);

	if (strcmp(unkF4, "WaterSprayCylinder") == 0) {
		unk138 = 0x154;
		SMS_LoadParticle("/scene/mapObj/ms_shib_cyl1.jpa", unk138);
	} else {
		unk138 = 0x155;
		SMS_LoadParticle("/scene/mapObj/ms_shib_cub1.jpa", unk138);
	}

	stream >> unk13C;
	if (unk13C > 100.0f)
		unk13C = 0.5f;
	else
		unk13C = unk13C / 100.0f;

	f32 scale;
	stream >> scale;
	if (scale == -1.0f)
		scale = 1.0f;
	else
		scale /= 100.0f;
	unk140.set(scale, scale, scale);

	u32 color;
	stream >> color;
	unk14C.r = color;
	stream >> color;
	unk14C.g = color;
	stream >> color;
	unk14C.b = color;
	stream >> color;
	unk14C.a = color;
}

void TMapObjWaterSpray::calc()
{
	JPABaseEmitter* emitter = gpMarioParticleManager->emit(unk138, &mPosition,
	                                                       1, this);
	if (emitter == nullptr)
		return;

	s16 rotationX = (s16)mRotation.x;
	s16 rotationY = (s16)mRotation.y;
	s16 rotationZ = (s16)mRotation.z;
	emitter->setRotation(rotationX, rotationY, rotationZ);
	emitter->unk154.set(mScaling);
	emitter->unk174.set(mScaling);
	emitter->mChildSpawnRate = unk13C;
	emitter->unk174.set(unk140);
	emitter->setParamColor(unk14C.r, unk14C.g, unk14C.b);
	emitter->unk180.a = unk14C.a;
}

THideObjInfo::THideObjInfo(const char* name)
    : JDrama::TActor(name)
    , unk44(0)
    , unk48(0.0f)
    , unk4C(0.0f)
{
}

void THideObjInfo::load(JSUMemoryInputStream& stream)
{
	JDrama::TActor::load(stream);

	s32 eventID;
	s32 objectID;
	TMapObjBase::loadHideObjInfo(stream, &eventID, &unk48, &unk4C, &objectID);
	unk44 = TMapObjBaseManager::getActorTypeByEventID(eventID);
	gpMapObjSwitch->registerObjInfo(this);
}

void THideObjInfo::action(long param)
{
	TMapObjBase* obj = gpItemManager->makeObjAppear(
	    mPosition.x, mPosition.y, mPosition.z, unk44, true);
	if (obj == nullptr)
		return;

	TMapObjBase::throwObjFromPointWithRot(obj, mPosition, mRotation, unk48,
	                                      unk4C);
	bool isItem = (obj->mActorType == 0x2000000e) ? true : false;
	if (isItem)
		((TItem*)obj)->unk14C = param;
}

TMapObjSwitch::TMapObjSwitch(const char* name)
    : TMapObjBase(name)
    , unk138(0)
    , unk13C(0)
    , unk140(0)
    , unk148(0xff)
    , unk14A(0xff)
    , unk14C(0xff)
    , unk14E(0xff)
{
	for (int i = 0; i < unk138; ++i)
		unk144[i] = nullptr;

	gpMapObjSwitch = this;
}

void TMapObjSwitch::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);

	stream.read(&unk140, 4);
	if (unk140 <= 0)
		unk140 = 0x4b0;
	else
		unk140 *= 10;

	s32 r;
	s32 g;
	s32 b;
	stream.read(&r, 4);
	stream.read(&g, 4);
	stream.read(&b, 4);
	unk148 = (u8)r;
	unk14A = (u8)g;
	unk14C = (u8)b;

	unk138 = 100;
	unk144 = new THideObjInfo*[unk138];

	SMS_LoadParticle("/scene/mapObj/ms_watcoin_hit.jpa", 0x57);
}

BOOL TMapObjSwitch::receiveMessage(THitActor*, u32 message)
{
	if (message == HIT_MESSAGE_HIP_DROP) {
		startBck("objswitch");
		START_MAP_OBJ_SOUND(0x384c, mPosition);
		removeMapCollision();
		for (int i = 0; i < unk13C; ++i)
			unk144[i]->action(unk140);

		gpMarDirector->fireStartDemoCamera("マップオブジェクトスイッチ",
		                                   &mPosition, -1, 0.0f, true, nullptr,
		                                   0, nullptr, JDrama::TFlagT<u16>(0));
		mLifeTimer = unk140;
		onHitFlag(HIT_FLAG_NO_COLLISION);
		return TRUE;
	}

	return FALSE;
}

void TMapObjSwitch::control()
{
	TMapObjBase::control();
	if (isLifeTimerActive())
		gpMSound->playTimer(mLifeTimer);
}

TRedCoinSwitch::TRedCoinSwitch(const char* name)
    : TMapObjBase(name)
    , unk138(0)
    , unk13C(0xff)
    , unk13E(0xff)
    , unk140(0xff)
    , unk142(0xff)
{
}

void TRedCoinSwitch::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);

	s32 timer;
	stream.read(&timer, 4);
	unk138 = timer;
	if (unk138 <= 0)
		unk138 = 0x4b0;
	else
		unk138 *= 10;

	u8 shine = SMS_getShineIDofExStage(gpMarDirector->mMap);
	if (shine != 0xff) {
		TFlagManager* flagManager = TFlagManager::smInstance;
		if (!flagManager->getShineFlag(shine))
			makeObjDead();
	}
}

void TRedCoinSwitch::loadAfter()
{
	TMapObjBase::loadAfter();

	char buf[64];
	for (int i = 0; i < 8; ++i) {
		snprintf(buf, sizeof(buf), "赤コイン %d", i);
		TMapObjBase* coin = JDrama::TNameRefGen::search<TMapObjBase>(buf);
		coin->makeObjDead();
	}
}

void TRedCoinSwitch::control()
{
	TMapObjBase::control();

	switch (mState) {
	case 2:
		if (mMActor->curAnmEndsNext(0, nullptr)) {
			mLifeTimer = 120;
			mState     = 3;
			TFlagManager::smInstance->setBool(true, 0x50009);
		}
		break;
	case 3:
		if (!isLifeTimerActive())
			mState = 4;
		break;
	}
}

BOOL TRedCoinSwitch::receiveMessage(THitActor*, u32 message)
{
	if (message == HIT_MESSAGE_HIP_DROP) {
		startBck("redcoinswitch");
		J3DFrameCtrl* frameCtrl = mMActor->getFrameCtrl(0);
		((u32*)((u8*)gpMarDirector->unk18[0] + 0xe8))[0]
		    = frameCtrl->getEnd() * 2 + 60;
		START_MAP_OBJ_SOUND(0x384c, mPosition);
		removeMapCollision();
		onHitFlag(HIT_FLAG_NO_COLLISION);
		mState = 2;
		return TRUE;
	}

	return FALSE;
}

void TBasketReverse::initMapObj()
{
	mPosition.y += 200.0f;
	TMapObjBase::initMapObj();
}

void TBasketReverse::kill()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0xe5, &mPosition, 0, nullptr);
	gpMarioParticleManager->emitAndBindToPosPtr(0xe6, &mPosition, 0, nullptr);
	START_MAP_OBJ_SOUND(0x380a, mPosition);
	if (gpMSound->gateCheck(0x4849))
		MSoundSESystem::MSoundSE::startSoundSystemSE(0x4849, 0, nullptr, 0);
	TMapObjBase::makeObjDead();
}
