#include <MoveBG/MapObjTown.hpp>
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

	u32 message = unk138 ? 0x12 : 0x11;
	if (!player->receiveMessage(this, message))
		return;

	if (gpMarioOriginal->mAction == 0x1320) {
		if (gpMarioOriginal->mHolder)
			startAnim(4);
		else
			startAnim(2);
	} else if (gpMarioOriginal->mAction == 0x1321) {
		if (gpMarioOriginal->mHolder)
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
    , unk153(0)
    , unk154(nullptr)
    , unk158(nullptr)
{
}

void TManhole::initMapObj()
{
	TMapObjGeneral::initMapObj();
	unk158 = new TMapCollisionWarp;
	unk158->init("/scene/map/manholeRoof.col", 0, this);
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
	MTXCopy(getModel()->mNodeMatrices[0], unk158->unk20);
	unk158->setUp();
}

void TManhole::setGroundCollision()
{
	u8* yoshi = (u8*)SMS_GetYoshi();
	if (yoshi != nullptr && *yoshi != 0) {
		JGeometry::TVec3<f32>* yoshiPos
		    = (JGeometry::TVec3<f32>*)(yoshi + 0x10);
		f32 dx = yoshiPos->x - mPosition.x;
		f32 dz = yoshiPos->z - mPosition.z;
		if (dx * dx + dz * dz < mBodyRadius * mBodyRadius) {
			if (mMapCollisionManager != nullptr && mMapCollisionManager->unk8)
				mMapCollisionManager->unk8->moveTrans(mPosition);
			return;
		}
	}

	TMapObjBase::setGroundCollision();
}

void TManhole::calc()
{
	J3DFrameCtrl* frameCtrl = mMActor->getFrameCtrl(0);

	if (frameCtrl->checkPass(45.0f) || frameCtrl->checkPass(125.0f))
		START_MAP_OBJ_SOUND(0x383c, mPosition);

	TMapObjGeneral::calc();
}

void TManhole::appeared()
{
	if (unk154 != nullptr && unk154->checkLiveFlag(LIVE_FLAG_DEAD)) {
		unk158->remove();
		unk154 = nullptr;
	}

	if (unk150 != 0 && *gpMarioSpeedY <= 0.0f) {
		if (mMapCollisionManager != nullptr && mMapCollisionManager->unk8)
			mMapCollisionManager->unk8->setAllBGType(0x107);
		unk150 = 0;
	}

	if (!animationFinished())
		return;

	if (unk152 != 0 && mColCount == 0) {
		unk152 = 0;
		START_MAP_OBJ_SOUND(0x383e, mPosition);
	}

	if (unk14C > mVibrationEndHeight) {
		s16 phase = (s16)(unk148 * 32768.0f);
		mPosition.y = mInitialPosition.y + unk14C * JMASCos(phase);
		unk148 += mVibrationSpeed;
		if (unk148 > 2.0f)
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
	J3DFrameCtrl* frameCtrl = mMActor->getFrameCtrl(0);
	if (frameCtrl->getRate() == 0.0f)
		return TRUE;

	if (frameCtrl->checkPass(79.0f)
	    || frameCtrl->getFrame() + frameCtrl->getRate() >= frameCtrl->getEnd()) {
		frameCtrl->setFrame(0.0f);
		frameCtrl->setRate(0.0f);
		appeared();
		getModel()->calc();
		onMapObjFlag(0x100);
		return TRUE;
	}

	return FALSE;
}

void TManhole::touchPlayer(THitActor*)
{
	mState = 1;

	if (!animationFinished()) {
		mPosition.y = mInitialPosition.y;
		return;
	}

	if (gpMarioOriginal->mAction == 0x80008a9 && gpMarioPos->y < mPosition.y) {
		J3DFrameCtrl* frameCtrl = mMActor->getFrameCtrl(0);
		frameCtrl->setRate(SMSGetAnmFrameRate());
		frameCtrl->setFrame(frameCtrl->getFrame() + SMSGetAnmFrameRate());
		START_MAP_OBJ_SOUND(0x383b, mPosition);
		offMapObjFlag(0x100);
		SMSRumbleMgr->start(0x15, 0xf, (f32*)nullptr);
		return;
	}

	if (gpMarioPos->y < mPosition.y && *gpMarioSpeedY > 0.0f) {
		J3DFrameCtrl* frameCtrl = mMActor->getFrameCtrl(0);
		frameCtrl->setRate(SMSGetAnmFrameRate());
		frameCtrl->setFrame(frameCtrl->getFrame() + SMSGetAnmFrameRate());
		if (mMapCollisionManager != nullptr && mMapCollisionManager->unk8)
			mMapCollisionManager->unk8->setAllBGType(0x400);
		START_MAP_OBJ_SOUND(0x383b, mPosition);
		unk150 = 1;
		SMSRumbleMgr->start(0x15, 0xf, (f32*)nullptr);
		return;
	}

	if (gpMarioPos->y <= SMS_GetMarioGrLevel() + 4.0f) {
		if (unk151 != 0) {
			setUpMapCollision(1);
			unk151 = 0;
		}
		if (unk152 == 0) {
			unk152 = 1;
			START_MAP_OBJ_SOUND(0x383d, mPosition);
		}
		f32 minY = mInitialPosition.y - mDownHeight;
		if (mPosition.y > minY) {
			mPosition.y -= mDownSpeed;
			if (mPosition.y < minY)
				mPosition.y = minY;
		}
		unk148 = 1.0f;
		unk14C = mInitialPosition.y - mPosition.y;
		offMapObjFlag(0x100);
	} else if (unk152 != 0) {
		unk152 = 0;
		START_MAP_OBJ_SOUND(0x383e, mPosition);
	}
}

void TMapObjBillboard::touchActor(THitActor* actor)
{
	if (!animIsFinished()) {
		J3DFrameCtrl* frameCtrl = mMActor->getFrameCtrl(0);
		if (frameCtrl->getFrame() < 33.0f)
			return;
	}

	f32 angle = mRotation.y + getRotYFromAxisX(actor->mPosition) * 57.295776f;
	angle     = callMsWrap(angle, 0.0f, 360.0f);
	if (angle < 0.0f || angle > 180.0f)
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
	if (!animIsFinished()) {
		J3DFrameCtrl* frameCtrl = mMActor->getFrameCtrl(0);
		if (frameCtrl->getFrame() < 33.0f)
			return 1;
	}

	f32 angle = mRotation.y + getRotYFromAxisX(actor->mPosition) * 57.295776f;
	angle     = callMsWrap(angle, 0.0f, 360.0f);
	if (angle < 0.0f || angle > 180.0f)
		startAnim(2);
	else
		startAnim(1);

	if (gpMSound->gateCheck(0x384f)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x384f, (Vec*)&mPosition, 0,
		                                          &unk150, 0, 4);
	}

	if (unk138 != nullptr && unk14C != 0) {
		TMapObjBase* obj = nullptr;
		if (unk138->mActorType == 0x2000000e)
			obj = gpItemManager->makeObjAppear(0x2000000e);
		else
			obj = unk138;

		if (obj != nullptr) {
			TMapObjBase::throwObjFromPointWithRot(obj, mPosition, mRotation,
			                                      unk13C, unk140);
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
    , unk140(0.0f, 0.0f, 0.0f)
    , unk14C(0xff)
    , unk14D(0xff)
    , unk14E(0xff)
    , unk14F(0xff)
{
}

void TMapObjWaterSpray::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);

	if (strcmp(unkF4, "WaterSprayCylinder") == 0) {
		unk138 = 0x154;
		SMS_LoadParticle("/scene/mapObj/ms_shib_cyl1.jpa", 0x154);
	} else {
		unk138 = 0x155;
		SMS_LoadParticle("/scene/mapObj/ms_shib_cub1.jpa", 0x155);
	}

	f32 value;
	stream.read(&value, 4);
	if (value > 100.0f)
		unk13C = 0.5f;
	else
		unk13C = value / 100.0f;

	stream.read(&value, 4);
	if (value == -1.0f)
		value = 1.0f;
	else
		value /= 100.0f;
	unk140.set(value, value, value);

	u32 color;
	stream.read(&color, 4);
	unk14C = color;
	stream.read(&color, 4);
	unk14D = color;
	stream.read(&color, 4);
	unk14E = color;
	stream.read(&color, 4);
	unk14F = color;
}

void TMapObjWaterSpray::calc()
{
	JPABaseEmitter* emitter = gpMarioParticleManager->emit(unk138, &mPosition,
	                                                       1, this);
	if (emitter == nullptr)
		return;

	emitter->setRotation((s16)mRotation.x, (s16)mRotation.y,
	                     (s16)mRotation.z);
	emitter->unk154.set(mScaling);
	emitter->unk174.set(mScaling);
	emitter->mChildSpawnRate = unk13C;
	emitter->unk174.set(unk140);
	emitter->setParamColor(unk14C, unk14D, unk14E);
	emitter->unk180.a = unk14F;
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
	gpMapObjSwitch->unk144[gpMapObjSwitch->unk13C] = this;
	gpMapObjSwitch->unk13C++;
}

void THideObjInfo::action(long param)
{
	TMapObjBase* obj = gpItemManager->makeObjAppear(
	    mPosition.x, mPosition.y, mPosition.z, unk44, true);
	if (obj == nullptr)
		return;

	TMapObjBase::throwObjFromPointWithRot(obj, mPosition, mRotation, unk48,
	                                      unk4C);
	if (obj->mActorType == 0x2000000e)
		((TItem*)obj)->unk14C = param;
}

TMapObjSwitch::TMapObjSwitch(const char* name)
    : TMapObjBase(name)
    , unk138(0)
    , unk13C(0)
    , unk140(0)
    , unk144(nullptr)
    , unk148(0xff)
    , unk14A(0xff)
    , unk14C(0xff)
    , unk14E(0xff)
{
	gpMapObjSwitch = this;
}

void TMapObjSwitch::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);

	s32 timer;
	stream.read(&timer, 4);
	if (timer <= 0)
		unk140 = 0x4b0;
	else
		unk140 = timer * 10;

	u32 color;
	stream.read(&color, 4);
	unk148 = color;
	stream.read(&color, 4);
	unk14A = color;
	stream.read(&color, 4);
	unk14C = color;

	unk138 = 100;
	unk144 = new THideObjInfo*[unk138];

	SMS_LoadParticle("/scene/mapObj/ms_watcoin_hit.jpa", 0x57);
}

BOOL TMapObjSwitch::receiveMessage(THitActor*, u32 message)
{
	if (message != HIT_MESSAGE_HIP_DROP)
		return FALSE;

	startBck("objswitch");
	START_MAP_OBJ_SOUND(0x384c, mPosition);
	removeMapCollision();
	for (int i = 0; i < unk13C; ++i)
		unk144[i]->action(unk140);

	gpMarDirector->fireStartDemoCamera("マップオブジェクトスイッチ",
	                                   &mPosition, -1, 0.0f, true, nullptr, 0,
	                                   nullptr, JDrama::TFlagT<u16>(0));
	mLifeTimer = unk140;
	mLiveFlag |= LIVE_FLAG_DEAD;
	return TRUE;
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
	if (timer <= 0)
		unk138 = 0x4b0;
	else
		unk138 = timer * 10;

	u8 shine = SMS_getShineIDofExStage(gpMarDirector->mMap);
	if (shine != 0xff && !TFlagManager::smInstance->getShineFlag(shine))
		makeObjDead();
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
	case 1:
		break;
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
	makeObjDead();
}
