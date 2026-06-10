#include <MoveBG/Item.hpp>
#include <MoveBG/ItemManager.hpp>
#include <GC2D/GCConsole2.hpp>
#include <Map/MapMirror.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <System/Application.hpp>
#include <System/StageUtil.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/Particles.hpp>
#include <Strategic/MirrorActor.hpp>
#include <Strategic/question.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <Player/MarioAccess.hpp>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

f32 TItem::mAppearedScaleSpeed = 0.01f;

void TItem::appeared()
{
	if (checkMapObjFlag(0x40000) && !isLifeTimerActive()) {
		if (unk148)
			unk148->receiveMessage(this, HIT_MESSAGE_UNK5);

		if (isActorType(0x2000000f) || isActorType(0x20000010)) {
			if (gpMSound->gateCheck(0x484C))
				MSoundSESystem::MSoundSE::startSoundActor(0x484C, mPosition, 0,
				                                          nullptr, 0, 4);
		}
	}

	TMapObjGeneral::appeared();
}

void TItem::taken(THitActor* param_1)
{
	param_1->receiveMessage(this, HIT_MESSAGE_ATTACK);
	kill();
	if (checkMapObjFlag(0x80000)) {
		makeObjDefault();
		appear();
	}
}

void TItem::touchPlayer(THitActor* param_1)
{
	if ((param_1->isActorType(0x80000001) || param_1->isActorType(0x8000083))
	    && !checkHitFlag(HIT_FLAG_NO_COLLISION))
		taken(param_1);
}

BOOL TItem::receiveMessage(THitActor* sender, u32 message)
{
	if (message == HIT_MESSAGE_SPRAYED_BY_WATER)
		return false;

	if (message == HIT_MESSAGE_UNKB) {
		taken(sender);
		return true;
	}

	return TMapObjGeneral::receiveMessage(sender, message);
}

void TItem::calcRootMatrix()
{
	if (!checkMapObjFlag(0x8000000))
		TMapObjGeneral::calcRootMatrix();
}

void TItem::calc()
{
	if (!checkMapObjFlag(0x4000000) && !isState(6)) {
		MtxPtr src = gpItemManager->unk40;

		MtxPtr mtx;
		if (checkMapObjFlag(0x100))
			mtx = getModel()->getAnmMtx(0);
		else
			mtx = getModel()->getBaseTRMtx();

		mtx[0][0] = src[0][0];
		mtx[0][1] = src[0][1];
		mtx[0][2] = src[0][2];
		mtx[0][3] = mPosition.x;

		mtx[1][0] = src[1][0];
		mtx[1][1] = src[1][1];
		mtx[1][2] = src[1][2];
		mtx[1][3] = mPosition.y;

		mtx[2][0] = src[2][0];
		mtx[2][1] = src[2][1];
		mtx[2][2] = src[2][2];
		mtx[2][3] = mPosition.z;
	}

	if (isState(6) && checkMapObjFlag(0x100))
		TMapObjGeneral::calcRootMatrix();
}

void TItem::appearing()
{
	if (unkF8 & 0x2000000) {
		if (mScaling.x < 2.0f) {
			mScaling.add((Vec) { mAppearedScaleSpeed * 2.0f,
			                     mAppearedScaleSpeed * 2.0f,
			                     mAppearedScaleSpeed * 2.0f });
		} else {
			makeObjAppeared();
			unk64 |= 1;
			unkF8 &= ~0x40000;
		}
	} else {
		TMapObjGeneral::appearing();
	}
}

void TItem::killByTimer(int param_1)
{
	unk14C = param_1;
	mLifeTimer = unk150;

	offMapObjFlag(0x10000000);
	onHitFlag(HIT_FLAG_NO_COLLISION);
	offMapObjFlag(0x40000);
}

void TItem::appear()
{
	TMapObjGeneral::appear();
	onHitFlag(HIT_FLAG_NO_COLLISION);
	mLifeTimer = unk150;
	offMapObjFlag(0x40000);
}

void TItem::perform(u32 param_1, JDrama::TGraphics* param_2)
{
	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return;

	if ((param_1 & 1) && checkHitFlag(HIT_FLAG_NO_COLLISION)
	    && !isLifeTimerActive()) {
		offHitFlag(HIT_FLAG_NO_COLLISION);
		if (!checkMapObjFlag(0x10000000)) {
			onMapObjFlag(0x40000);
			mLifeTimer = unk14C;
		}
	}

	TMapObjGeneral::perform(param_1, param_2);
}

void TItem::initMapObj()
{
	TMapObjGeneral::initMapObj();
	unk14C = 480;
	unk150 = 120;
}

void TItem::load(JSUMemoryInputStream& stream)
{
	TMapObjGeneral::load(stream);
	onMapObjFlag(0x10000000);
}

TItem::TItem(const char* name)
    : TMapObjGeneral(name)
    , unk148(0)
    , unk14C(0)
    , unk150(0)
{
}

void TCoin::taken(THitActor* param_1)
{
	u8 thing = gpApplication.mCurrArea.unk0;
	TFlagManager::getInstance()->incGoldCoinFlag(SMS_getShineStage(thing), 1);

	if (gpMSound->gateCheck(0x4811))
		MSoundSESystem::MSoundSE::startSoundActor(0x4811, mPosition, 0, nullptr,
		                                          0, 4);

	if (unk148)
		unk148->receiveMessage(this, HIT_MESSAGE_UNK8);

	if (TFlagManager::smInstance->getFlag(0x40002) == 100) {
		TShine* shine = JDrama::TNameRefGen::search<TShine>(
		    "シャイン（１００枚コイン用）");

		gpItemManager->makeShineAppearWithDemo(
		    "シャイン（１００枚コイン用）",
		    "シャイン（１００枚コイン用）カメラ", mPosition.x, mPosition.y,
		    mPosition.z);
	}

	TItem::taken(param_1);
}

void TCoin::makeObjDead()
{
	TItem::makeObjDead();
	if (unk154)
		unk154->unk1A |= 1;
}

void TCoin::appearWithoutSound()
{
	TItem::appear();
	gpMarioParticleManager->emitAndBindToMtxPtr(0x58, getModel()->getAnmMtx(0),
	                                            0, this);
	if (isActorType(0x2000000e))
		offMapObjFlag(0x10000000);
}

void TCoin::appear()
{
	if (isActorType(0x20000010)) {
		if (!TFlagManager::smInstance->getBlueCoinFlag(
		        gpMarDirector->getCurrentMap(), unk134))
			SMSGetMSound()->startSoundSystemSE(0x4843, 0, nullptr, 0);
	} else {
		SMSGetMSound()->startSoundSystemSE(0x4813, 0, nullptr, 0);
	}

	appearWithoutSound();
}

void TCoin::makeObjAppeared()
{
	TItem::makeObjAppeared();
	if (unk154)
		unk154->unk1A &= ~1;
}

void TCoin::perform(u32 param_1, JDrama::TGraphics* param_2)
{
	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return;

	if ((param_1 & 1) && checkLiveFlag(LIVE_FLAG_UNK10)) {
		// TODO: this is some kind of a tricky inline, used in a few places
		bool bVar2 = true;
		if (gpMarDirector->unk124 != 1 && gpMarDirector->unk124 != 2)
			bVar2 = false;

		if (bVar2) {
			bVar2 = true;
			if (gpMarDirector->unk124 != 3 && gpMarDirector->unk124 != 4)
				bVar2 = false;
			if (!bVar2)
				return;
		}

		if (isLifeTimerActive()) {
			--mLifeTimer;
		} else {
			if (checkHitFlag(HIT_FLAG_NO_COLLISION)) {
				offHitFlag(HIT_FLAG_NO_COLLISION);
				if (!checkMapObjFlag(0x10000000)) {
					onMapObjFlag(0x40000);
					mLifeTimer = unk14C;
				}
			} else {
				if (!checkMapObjFlag(0x10000000)) {
					if (unk148 != 0)
						unk148->receiveMessage(this, HIT_MESSAGE_UNK5);
					makeObjDead();
				}
			}
		}

		if (getColNum())
			for (int i = 0; i < getColNum(); ++i)
				touchActor(mCollisions[i]);

	} else {
		if ((param_1 & 4) && getMActor() == nullptr) {
			gpQuestionManager->request(mPosition, 60.0f);
		}

		TItem::perform(param_1, param_2);
	}
}

void TCoin::loadAfter()
{
	TItem::loadAfter();
	if (!gpMirrorModelManager->isInMirror(mPosition))
		return;

	if (gpMarDirector->getCurrentMap() == 2) {
		const TBGCheckData* check;
		gpMap->checkGround(mPosition, &check);
		if (!check->isWaterSurface())
			return;
	}

	unk154 = new TMirrorActor("コインin鏡");
	unk154->init(getModel(), 0x18);
}

void TCoin::initMapObj()
{
	TItem::initMapObj();
	SMS_LoadParticle("/scene/mapObj/ms_watcoin_kira.jpa", 0x58);
}

TCoin::TCoin(const char* name)
    : TItem(name)
    , unk154(0)
{
}

void TFlowerCoin::load(JSUMemoryInputStream& stream)
{
	TCoin::load(stream);
	stream.read(&unk158, 4);
}

void TCoinEmpty::appear() { }

void TCoinEmpty::makeObjAppeared() { }

void TCoinEmpty::kill() { }

TCoinEmpty::TCoinEmpty(const char* name)
    : TCoin(name)
{
}

void TCoinRed::taken(THitActor* param_1)
{
	TFlagManager::getInstance()->incFlag(0x60000, 1);

	if (gpMSound->gateCheck(0x4846))
		MSoundSESystem::MSoundSE::startSoundActor(0x4846, mPosition, 0, nullptr,
		                                          0, 4);

	if (unk148)
		unk148->receiveMessage(this, HIT_MESSAGE_UNK8);

	TItem::taken(param_1);
}

TCoinRed::TCoinRed(const char* name)
    : TCoin(name)
{
	unk160 = 0.0f;
	unk15C = 0.0f;
	unk158 = 0.0f;
}

void TCoinBlue::makeObjAppeared()
{
	if (TFlagManager::getInstance()->getBlueCoinFlag(
	        gpMarDirector->getCurrentMap(), getUnk134()))
		return;

	TCoin::makeObjAppeared();
}

void TCoinBlue::taken(THitActor* param_1)
{
	TMarDirector* director = gpMarDirector;
	director->fireGetBlueCoin(this);

	if (unk148)
		unk148->receiveMessage(this, HIT_MESSAGE_UNK8);

	TItem::taken(param_1);
}

void TCoinBlue::loadBeforeInit(JSUMemoryInputStream& stream)
{
	int thing;
	stream.read(&thing, 4);
	if (thing == -1)
		thing = 0;
	setUnk134(thing);
}

void TCoinBlue::load(JSUMemoryInputStream& stream)
{
	TCoin::load(stream);
	if (TFlagManager::getInstance()->getBlueCoinFlag(
	        gpMarDirector->getCurrentMap(), getUnk134()))
		makeObjDead();
}

TCoinBlue::TCoinBlue(const char* name)
    : TCoin(name)
{
}

u32 TShine::mPromiLife     = 1;
u32 TShine::mSenkoRate     = 1;
u32 TShine::mKiraRate      = 1;
u32 TShine::mBowRate       = 1;
u32 TShine::mCircleRateY   = 1;
u32 TShine::mUpSpeed       = 1;
u32 TShine::mSpeedDownRate = 1;
u32 TShine::mSpeedDownTime = 1;

void TShine::calc() { }

void TShine::movingCircle() { }

void TShine::control() { }

void TShine::perform(u32, JDrama::TGraphics*) { }

BOOL TShine::receiveMessage(THitActor*, u32) { }

void TShine::touchPlayer(THitActor*) { }

void TShine::appearWithTime(int, int, int, int) { }

void TShine::appearWithTimeCallback(u32, u32) { }

void TShine::appearSimple(int) { }

void TShine::appearWithDemo(const char*) { }

void TShine::kill() { }

void TShine::makeMActors() { }

void TShine::initMapObj() { }

void TShine::loadAfter() { }

void TShine::loadBeforeInit(JSUMemoryInputStream& stream) { }

TShine::TShine(const char* name)
    : TItem(name)
{
}

void TEggYoshi::decideRandomLoveFruit() { }

void TEggYoshi::touchFruit(THitActor*) { }

void TEggYoshi::touchActor(THitActor*) { }

void TEggYoshi::control() { }

void TEggYoshi::perform(u32, JDrama::TGraphics*) { }

void TEggYoshi::startFruit() { }

BOOL TEggYoshi::receiveMessage(THitActor*, u32) { }

void TEggYoshi::load(JSUMemoryInputStream&) { }

TEggYoshi::TEggYoshi(const char* name)
    : TMapObjGeneral(name)
{
	unk148 = 0;
	unk14C = 0;
	unk150 = 0;
}

void TItemNozzle::touchPlayer(THitActor* actor)
{
	if (isState(6))
		return;

	if (SMS_IsMarioOnYoshi())
		return;

	if ((actor->isActorType(0x80000001) || actor->isActorType(0x8000083))
	    && !checkHitFlag(HIT_FLAG_NO_COLLISION))
		put();

	int nozzleType;
	if (isActorType(0x2000001F)) {
		nozzleType = 4;
	} else if (isActorType(0x20000022)) {
		nozzleType = 1;
	} else if (isActorType(0x2000002A)) {
		nozzleType = 5;
	} else {
		nozzleType = 4;
	}

	if (gpMSound->gateCheck(0x484E))
		MSoundSESystem::MSoundSE::startSoundActor(0x484E, mPosition, 0,
		                                          nullptr, 0, 4);

	gpItemManager->resetNozzleBoxesModel(nozzleType);
	gpMarDirector->fireGetNozzle(this);
}

void TItemNozzle::put()
{
	offHitFlag(HIT_FLAG_NO_COLLISION);
	mState = 1;
}

BOOL TItemNozzle::receiveMessage(THitActor* sender, u32 message)
{
	if (message == HIT_MESSAGE_TAKE) {
		put();
		return TRUE;
	}

	if (message == HIT_MESSAGE_UNK7) {
		mVelocity.x = 0.0f;
		mVelocity.y = 20.0f;
		mVelocity.z = 0.0f;
		offLiveFlag(LIVE_FLAG_UNK10);
		onHitFlag(HIT_FLAG_NO_COLLISION);
		mState = 0xB;
		return TRUE;
	}

	if (message == HIT_MESSAGE_SPRAYED_BY_WATER)
		return FALSE;

	if (message == HIT_MESSAGE_UNKB) {
		put();
		return TRUE;
	}

	return TMapObjGeneral::receiveMessage(sender, message);
}

void TItemNozzle::appearing()
{
	if (!checkLiveFlag(LIVE_FLAG_UNK10))
		return;

	mState = 1;
	offHitFlag(HIT_FLAG_NO_COLLISION);
}

void TItemNozzle::control() { TMapObjGeneral::control(); }

void TItemNozzle::initMapObj()
{
	TItem::initMapObj();
	unk14C = 7200;
}

void TItemNozzle::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	onMapObjFlag(0x10000000);

	if (strcmp(unkF4, "rocket_nozzle_item") == 0) {
		if (TFlagManager::smInstance->getFlag(0x60003) != 3)
			makeObjDead();
	} else if (strcmp(unkF4, "back_nozzle_item") == 0) {
		if (TFlagManager::smInstance->getFlag(0x60003) != 2)
			makeObjDead();
	}
}

void TNozzleBox::makeModelValid()
{
	if (!unk14C->checkLiveFlag(LIVE_FLAG_DEAD)) {
		unk14C->kill();
		appear();
	}

	makeObjAppeared();
	offHitFlag(HIT_FLAG_UNK4);
	SMS_ShowAllShapePacket(getModel());
	unk15C = TRUE;
}

void TNozzleBox::breaking()
{
	if (animIsFinished())
		makeObjDead();
}

BOOL TNozzleBox::receiveMessage(THitActor* sender, u32 message)
{
	if (unk15C && sender->isActorType(0x80000001)
	    && message == HIT_MESSAGE_TRAMPLE && !SMS_IsMarioHeadSlideAttack()) {
		sender->receiveMessage(this, HIT_MESSAGE_ATTACK);
		throwObjToFront(unk14C, 50.0f, unk150, unk154);

		if (gpMSound->gateCheck(0x3801))
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x3801, 0, nullptr,
			                                             0);

		kill();
		return TRUE;
	}

	if (message == HIT_MESSAGE_UNK5)
		makeModelValid();

	return FALSE;
}

void TNozzleBox::touchPlayer(THitActor*)
{
	if (unk148 == 4
	    && !TFlagManager::smInstance->getNozzleRight(
	        gpMarDirector->getCurrentMap(), 0)
	    && !TFlagManager::smInstance->getNozzleRight(
	        gpMarDirector->getCurrentMap(), 1)
	    && !unk166) {
		gpMarDirector->mConsole->startAppearBalloon(0xE57, true);
		unk166 = TRUE;
	}

	if (!unk15C && !unk166) {
		gpMarDirector->mConsole->startAppearBalloon(0xE56, true);
		unk166 = TRUE;
	}
}

void TNozzleBox::control()
{
	TMapObjGeneral::control();
	if (unk148 != 4 && unk166 && mColCount == 0)
		unk166 = FALSE;
}

void TNozzleBox::loadAfter()
{
	TMapObjGeneral::loadAfter();

	JGeometry::TVec3<f32> position(0.0f, 0.0f, 0.0f);
	JGeometry::TVec3<f32> rotation(0.0f, 0.0f, 0.0f);
	JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
	unk14C = (TItem*)TMapObjBaseManager::newAndRegisterObj(
	    unk158, position, rotation, scale);
	unk14C->unk148 = this;

	switch (unk148) {
	case 4:
		makeModelValid();
		break;

	case 1:
	case 5:
		if (unk15C) {
			makeModelValid();
		} else {
			if (!unk14C->checkLiveFlag(LIVE_FLAG_DEAD)) {
				unk14C->kill();
				appear();
			}
			onHitFlag(HIT_FLAG_UNK4);
			startAnim(3);
			unk15C = FALSE;
		}
		break;
	}
}

void TNozzleBox::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);

	unk158 = stream.readString();

	char valid[32];
	stream.readString(valid, sizeof(valid));
	unk15C = strcmp(valid, "valid") == 0 ? TRUE : FALSE;

	if (strcmp(unk158, "normal_nozzle_item") == 0) {
		unk148 = 4;
		unk15E = 0;
		unk160 = 0;
		unk162 = 0xFF;
	} else if (strcmp(unk158, "rocket_nozzle_item") == 0) {
		unk148 = 1;
		unk15E = 0xFF;
		unk160 = 0;
		unk162 = 0;
		if (TFlagManager::smInstance->getNozzleRight(
		        gpMarDirector->getCurrentMap(), 0)) {
			unk15C = TRUE;
			unk166 = TRUE;
		}
	} else if (strcmp(unk158, "back_nozzle_item") == 0) {
		unk148 = 5;
		unk15E = 0x5A;
		unk160 = 0x5A;
		unk162 = 0x78;
		if (TFlagManager::smInstance->getNozzleRight(
		        gpMarDirector->getCurrentMap(), 1)) {
			unk15C = TRUE;
			unk166 = TRUE;
		}
	}

	stream.read(&unk150, 4);
	unk150 *= 0.02f;

	stream.read(&unk154, 4);
	if (unk154 < 0.0f)
		unk154 = 20.0f;

	TMapObjBase::initPacketMatColor(getModel(), GX_TEVREG2,
	                                (const GXColorS10*)&unk15E);
	startAnim(3);
	TMapObjBase::initPacketMatColor(getModel(), GX_TEVREG2,
	                                (const GXColorS10*)&unk15E);
	startAnim(2);
	TMapObjBase::initPacketMatColor(getModel(), GX_TEVREG2,
	                                (const GXColorS10*)&unk15E);
	startAnim(0);
}

TNozzleBox::TNozzleBox(const char* name)
    : TMapObjGeneral(name)
    , unk148(0)
    , unk14C(0)
    , unk150(0.0f)
    , unk154(0.0f)
    , unk158(0)
    , unk15C(TRUE)
    , unk15E(0xFF)
    , unk160(0xFF)
    , unk162(0xFF)
    , unk164(100)
    , unk166(FALSE)
{
}
