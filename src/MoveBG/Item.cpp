#define JGEOMETRY_ITEM_TVEC3_CTOR_SET_VEC
#include <MoveBG/Item.hpp>
#include <MoveBG/ItemManager.hpp>
#include <Camera/Camera.hpp>
#include <GC2D/GCConsole2.hpp>
#include <Map/MapMirror.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorUtil.hpp>
#include <System/Application.hpp>
#include <System/StageUtil.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/Particles.hpp>
#include <Strategic/MirrorActor.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/question.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <MarioUtil/LightUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/Yoshi.hpp>

#include <stdlib.h>

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
	unk158 = unk15C = unk160 = 0.0f;
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

u32 TShine::mPromiLife[4] = { 30, 15, 0, 0 };
f32 TShine::mSenkoRate[4] = { 0.15f, 0.1f, 0.05f, 0.025f };
f32 TShine::mKiraRate[4]  = { 1.0f, 0.6f, 0.3f, 0.1f };
f32 TShine::mBowRate[4]   = { 1.0f, 1.0f, 0.0f, 0.0f };
f32 TShine::mCircleRateY  = 0.5f;
f32 TShine::mUpSpeed      = 1.0f;
f32 TShine::mSpeedDownRate = 0.99f;

void TShine::calc()
{
	MtxPtr mtx = mMActor->getModel()->mNodeMatrices[2];

	if (checkLiveFlag(LIVE_FLAG_DEAD | LIVE_FLAG_CLIPPED_OUT
	                  | LIVE_FLAG_UNK200))
		return;

	unk198 = gpMarioParticleManager->emitAndBindToMtxPtr(0x128, mtx, 1, this);
	unk19C = gpMarioParticleManager->emitAndBindToMtxPtr(0x129, mtx, 1, this);

	if (!unk1B4) {
		unk194
		    = gpMarioParticleManager->emitAndBindToMtxPtr(0x127, mtx, 1, this);
		unk1A0
		    = gpMarioParticleManager->emitAndBindToMtxPtr(0x12A, mtx, 1, this);
	}

	JGeometry::TVec3<f32>* cameraPos
	    = (JGeometry::TVec3<f32>*)((u8*)gpCamera + 0x124);
	f32 dx = cameraPos->x - mPosition.x;
	f32 dy = cameraPos->y - mPosition.y;
	f32 dz = cameraPos->z - mPosition.z;
	f32 distance
	    = JGeometry::TUtil<f32>::sqrt(dx * dx + dy * dy + dz * dz);

	s16 promiLife;
	f32 senkoRate;
	f32 kiraRate;
	f32 bowRate;
	if (distance < 2000.0f) {
		promiLife = (s16)mPromiLife[0];
		senkoRate = mSenkoRate[0];
		kiraRate  = mKiraRate[0];
		bowRate   = mBowRate[0];
	} else if (distance < 4000.0f) {
		promiLife = (s16)mPromiLife[1];
		senkoRate = mSenkoRate[1];
		kiraRate  = mKiraRate[1];
		bowRate   = mBowRate[1];
	} else if (distance < 6000.0f) {
		promiLife = (s16)mPromiLife[2];
		senkoRate = mSenkoRate[2];
		kiraRate  = mKiraRate[2];
		bowRate   = mBowRate[2];
	} else {
		promiLife = (s16)mPromiLife[3];
		senkoRate = mSenkoRate[3];
		kiraRate  = mKiraRate[3];
		bowRate   = mBowRate[3];
	}

	JPABaseEmitter* emitter = unk194;
	if (emitter) {
		emitter->mBaseLifetime = promiLife;
		emitter->unk154.x      = unk1A8;
		emitter->unk154.y      = unk1AC;
		emitter->unk154.z      = unk1B0;
		emitter->unk174.x      = unk1A8;
		emitter->unk174.y      = unk1AC;
		emitter->unk174.z      = unk1B0;
	}
	emitter = unk198;
	if (emitter) {
		emitter->mChildSpawnRate = senkoRate;
		emitter->unk154.x        = unk1A8;
		emitter->unk154.y        = unk1AC;
		emitter->unk154.z        = unk1B0;
		emitter->unk174.x        = unk1A8;
		emitter->unk174.y        = unk1AC;
		emitter->unk174.z        = unk1B0;
	}
	emitter = unk19C;
	if (emitter) {
		emitter->mChildSpawnRate = kiraRate;
		emitter->unk154.x        = unk1A8;
		emitter->unk154.y        = unk1AC;
		emitter->unk154.z        = unk1B0;
		emitter->unk174.x        = unk1A8;
		emitter->unk174.y        = unk1AC;
		emitter->unk174.z        = unk1B0;
	}
	emitter = unk1A0;
	if (emitter) {
		emitter->mChildSpawnRate = bowRate;
		emitter->unk154.x        = unk1A8;
		emitter->unk154.y        = unk1AC;
		emitter->unk154.z        = unk1B0;
		emitter->unk174.x        = unk1A8;
		emitter->unk174.y        = unk1AC;
		emitter->unk174.z        = unk1B0;
	}

	unk1A4[0] = 1;
}

#pragma dont_inline on
void TShine::movingCircle()
{
	f32 oldY = mPosition.y;
	unk158 += 180.0f / (s32)unk168;

	f32 t = (f32)((s32)unk168 - mLifeTimer) / (f32)(s32)unk168;
	mPosition.x += unk17C;
	f32 y = unk164 + t * (mInitialPosition.y - unk164);
	mPosition.y = y + unk160 * JMASSin((s16)(unk158 * 182.04445f));
	unk188     = mPosition.y - oldY;
	mPosition.z += unk184;
	mRotation.y += 7.0f;
	MsWrap<f32>(mRotation.y, 0.0f, 360.0f);

	if (!isLifeTimerActive()) {
		unk16C     = 7.0f;
		mLifeTimer = unk178;
		mState     = 0xF;
	}
}
#pragma dont_inline off

void TShine::control()
{
	if (!isState(0x10))
		TMapObjGeneral::control();

	if (isState(0x10)) {
		JGeometry::TVec3<f32>* marioPos = gpMarioPos;
		mPosition.x = marioPos->x;
		mPosition.y = marioPos->y;
		mPosition.z = marioPos->z;
		return;
	}

	switch (mState) {
	case 1: {
		mRotation.y += unk16C;

		if (gpMSound->gateCheck(0x81C1))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x81C1, mPosition, 0, nullptr, 0, 4);

		Mtx* nodeMatrices = mMActor->getModel()->mNodeMatrices;
		f32 lightX        = nodeMatrices[2][0][3];
		f32 lightY        = nodeMatrices[2][1][3];
		f32 lightZ        = nodeMatrices[2][2][3];
		TLightWithDBSetManager* light = gpLightManager;
		GXColor color                 = { 0xFF, 0xFF, 0xFF, 0xFF };
		light->unk18.r                = color.r;
		light->unk18.g                = color.g;
		light->unk18.b                = color.b;
		light->unk18.a                = color.a;
		light->unk54                  = 1;
		light->unk1C.x                = lightX;
		light->unk1C.y                = lightY;
		light->unk1C.z                = lightZ;
		light->unk54                  = 1;
		break;
	}
	case 0xB:
		if (isLifeTimerActive())
			return;
		unkF8 &= 0xF800FEFF;
		mLifeTimer = unk170;
		mState     = 0xC;
		break;
	case 0xC:
		if (gpMSound->gateCheck(0x81C1))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x81C1, mPosition, 0, nullptr, 0, 4);

		mPosition.y += mUpSpeed;
		if (isLifeTimerActive())
			return;

		if (unk154 == 3) {
			mLifeTimer = unk170;
			mState     = 0xE;
		} else {
			unk164     = mPosition.y;
			mLifeTimer = unk168;
			mState     = 0xD;
		}
		break;
	case 0xE:
		if (gpMSound->gateCheck(0x81C1))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x81C1, mPosition, 0, nullptr, 0, 4);

		mPosition.y -= mUpSpeed;
		if (isLifeTimerActive())
			return;

		unk16C     = 7.0f;
		mLifeTimer = unk178;
		mState     = 0xF;
		break;
	case 0xD:
		if (gpMSound->gateCheck(0x81C1))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x81C1, mPosition, 0, nullptr, 0, 4);
		movingCircle();
		break;
	case 0xF:
		if (gpMSound->gateCheck(0x81C1))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x81C1, mPosition, 0, nullptr, 0, 4);

		if (mPosition.y > mInitialPosition.y) {
			mPosition.y += unk188;
			unk188 *= mSpeedDownRate;
		} else {
			mPosition.y = mInitialPosition.y;
		}

		if (unk16C > 2.0f)
			unk16C -= 0.1f;
		else
			unk16C = 2.0f;

		mRotation.y += unk16C;
		MsWrap<f32>(mRotation.y, 0.0f, 360.0f);

		if (isLifeTimerActive())
			return;

		if (checkMapObjFlag(0x20000000))
			MSBgm::setTrackVolume(0, 1.0f, 10, 0);

		offHitFlag(HIT_FLAG_NO_COLLISION);
		mState = 0x11;
		break;
	case 0x11:
		mRotation.y += unk16C;
		MsWrap<f32>(mRotation.y, 0.0f, 360.0f);

		if (gpMSound->gateCheck(0x81C1))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x81C1, mPosition, 0, nullptr, 0, 4);
		break;
	case 0x12: {
		CPolarSubCamera* camera = gpCamera;
		bool demoCamera =
		    camera->isSimpleDemoCamera() || camera->isOnGoingDemoCamera();
		bool isDemoCamera = demoCamera ? true : false;
		if (isDemoCamera)
			return;
		if (isLifeTimerActive())
			return;

		JDrama::TNameRef* cameraRef = JDrama::TNameRefGen::search<JDrama::TNameRef>(
		    "シャイン（いききなり出現）カメラ");
		unk18C = *(u32*)((u8*)cameraRef + 0x2C);
		gpMarDirector->fireStartDemoCamera(
		    "シャイン（いききなり出現）カメラ", &mPosition, -1, 0.0f, true,
		    appearWithTimeCallback, (u32)this, nullptr, JDrama::TFlagT<u16>(0));
		mState = 0x11;
		break;
	}
	}
}

void TShine::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if ((flags & 2) && !checkLiveFlag(LIVE_FLAG_DEAD) && !isState(1))
		offLiveFlag(LIVE_FLAG_UNK200);

	TMapObjGeneral::perform(flags, graphics);
}

BOOL TShine::receiveMessage(THitActor*, u32)
{
	offMapObjFlag(0x8000000);
	JGeometry::TVec3<f32>* marioPos = gpMarioPos;
	mPosition.x = marioPos->x;
	mPosition.y = marioPos->y;
	mPosition.z = marioPos->z;
	mRotation.y = ((f32)*gpMarioAngleY * 180.0f) / 32768.0f;

	MsMtxSetXYZRPH(getModel()->getBaseTRMtx(), mPosition.x,
	               mPosition.y - mYOffset, mPosition.z, mRotation.x,
	               mRotation.y, mRotation.z);

	if (SMS_IsMarioOnYoshi()) {
		if (unk1B4)
			getMActor()->setBck("shine_empty_demo_shine_get_yo");
		else
			getMActor()->setBck("shine_demo_shine_get_yo");
	} else {
		if (unk1B4)
			getMActor()->setBck("shine_empty_demo_shine_get");
		else
			getMActor()->setBck("shine_demo_shine_get");
	}

	unk1A8 = 0.5f;
	unk1AC = 0.5f;
	unk1B0 = 0.5f;
	mState = 0x10;
	return TRUE;
}

void TShine::touchPlayer(THitActor* actor)
{
	actor->receiveMessage(this, HIT_MESSAGE_ATTACK);

	TLightWithDBSetManager* light = gpLightManager;
	light->unk1C.x = 200000.0f;
	light->unk1C.y = 500000.0f;
	light->unk1C.z = 200000.0f;

	if (gpMSound->gateCheck(0x4814))
		MSoundSESystem::MSoundSE::startSoundActor(0x4814, mPosition, 0,
		                                          nullptr, 0, 4);

	mMActor->setBck("shine_float");
	onHitFlag(HIT_FLAG_NO_COLLISION);
}

void TShine::appearWithTime(int total_time, int up_time, int circle_time,
                            int down_time)
{
	TMapObjGeneral::appear();
	onHitFlag(HIT_FLAG_NO_COLLISION);
	mLifeTimer = unk150;
	offMapObjFlag(0x80000);
	TFlagManager::smInstance->setBool(true, 0x50000);

	if (up_time >= 0)
		unk174 = up_time;
	if (circle_time >= 0)
		unk170 = circle_time;
	if (down_time >= 0)
		unk178 = down_time;

	unk168 = total_time - (unk174 + unk170 + unk178);
	unk158 = 0.0f;
	unk17C = (mInitialPosition.x - mPosition.x) / unk168;

	f32 targetY = mPosition.y + mUpSpeed * unk170;
	f32 yDiff   = mInitialPosition.y - targetY;
	unk180      = yDiff / unk168;
	unk184      = (mInitialPosition.z - mPosition.z) / unk168;
	unk15C      = getDistanceXZ(mInitialPosition);
	if (unk15C == 0.0f)
		unk15C = 1000.0f;
	if (yDiff > 0.0f)
		unk15C += yDiff;
	unk160 = unk15C * mCircleRateY;

	if (gpMSound->gateCheck(0x4821))
		MSoundSESystem::MSoundSE::startSoundActor(0x4821, mPosition, 0,
		                                          nullptr, 0, 4);

	mLifeTimer = unk174;
	mState     = 0xB;
	onHitFlag(HIT_FLAG_NO_COLLISION);
}

s32 TShine::appearWithTimeCallback(u32 shine, u32 event)
{
	if (event == 0) {
		TShine* self = (TShine*)shine;
		self->appearWithTime(self->unk18C, -1, -1, -1);
		gpMarDirector->unk4E |= 1;
	} else if (event == 1) {
		gpMarDirector->unk4E &= ~1;
	}

	return 0;
}

void TShine::appearSimple(int circle_time)
{
	TMapObjGeneral::appear();
	onHitFlag(HIT_FLAG_NO_COLLISION);
	mLifeTimer = unk150;
	offMapObjFlag(0x80000);
	TFlagManager::smInstance->setBool(true, 0x50000);

	unk174   = 60;
	unk170   = circle_time;
	unk178   = 60;
	unk154   = 3;
	unk158   = 0.0f;
	unk15C   = 0.0f;
	unk160   = 0.0f;
	mUpSpeed = 2.0f;
	mInitialPosition = mPosition;

	if (gpMSound->gateCheck(0x4821))
		MSoundSESystem::MSoundSE::startSoundActor(0x4821, mPosition, 0,
		                                          nullptr, 0, 4);

	mLifeTimer = unk174;
	mState     = 0xB;
	onHitFlag(HIT_FLAG_NO_COLLISION);
}

void TShine::appearWithDemo(const char* camera_name)
{
	JDrama::TNameRef* camera
	    = JDrama::TNameRefGen::search<JDrama::TNameRef>(camera_name);
	unk18C = *(u32*)((u8*)camera + 0x2C);
	JDrama::TFlagT<u16> flag(0);

	gpMarDirector->fireStartDemoCamera(
	    camera_name, &mPosition, -1, 0.0f, true, appearWithTimeCallback,
	    (u32)this, nullptr, flag);
}

void TShine::kill()
{
	TMapObjGeneral::kill();
	unk154 = 1;
}

void TShine::makeMActors()
{
	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActorKeeper->mModelLoaderFlags = 0x10220000;

	if (TFlagManager::smInstance->getShineFlag((u8)unk134)
	    && strcmp(mName, "シャイン（マニ屋用）") != 0) {
		mMActor = initMActor("shine_empty.bmd", nullptr, getSDLModelFlag());
		unk1B4  = TRUE;
	} else {
		mMActor = initMActor("shine.bmd", nullptr, getSDLModelFlag());
	}
}

void TShine::initMapObj()
{
	TMapObjGeneral::initMapObj();
	unk14C    = 480;
	unk150    = 120;
	unk1A4[0] = 0;
	unk1A8    = 1.0f;
	unk1AC    = 1.0f;
	unk1B0    = 1.0f;
	unk170    = 240;
	unk174    = 0;
	unk178    = 240;
}

void TShine::loadAfter()
{
	TMapObjGeneral::loadAfter();

	if (unk154 == 2) {
		mLifeTimer = 240;
		mState     = 0x12;
	} else if (unk154 == 1) {
		makeObjDead();
	}
}

void TShine::loadBeforeInit(JSUMemoryInputStream& stream)
{
	char type[0x20];
	stream.readString(type, sizeof(type));

	if (strcmp("normal", type) == 0)
		unk154 = 0;
	else if (strcmp("quickly", type) == 0)
		unk154 = 2;
	else
		unk154 = 1;

	int shine_id;
	stream.read(&shine_id, sizeof(shine_id));
	if (shine_id == -1)
		shine_id = 120;
	unk134 = shine_id;

	int in_stage;
	stream.read(&in_stage, sizeof(in_stage));
	if (in_stage + 1 >= 2)
		in_stage = -1;
	unk190 = in_stage + 1;
}

TShine::TShine(const char* name)
    : TItem(name)
    , unk154(0)
    , unk158(0.0f)
    , unk15C(0.0f)
    , unk160(0.0f)
    , unk164(0.0f)
    , unk168(0)
    , unk16C(2.0f)
    , unk170(0)
    , unk174(0)
    , unk178(0)
    , unk17C(0.0f)
    , unk180(0.0f)
    , unk184(0.0f)
    , unk188(0.0f)
    , unk18C(0)
    , unk190(0)
    , unk194(0)
    , unk198(0)
    , unk19C(0)
    , unk1A0(0)
    , unk1A8(0.0f)
    , unk1AC(0.0f)
    , unk1B0(0.0f)
    , unk1B4(0)
{
}

inline void TEggYoshi::startBalloonAnim()
{
	switch (unk14C) {
	case 0x40000394:
		unk148->getFrameCtrl(3)->setFrame(7.0f);
		break;
	case 0x40000393:
		unk148->getFrameCtrl(3)->setFrame(3.0f);
		break;
	case 0x40000391:
		unk148->getFrameCtrl(3)->setFrame(5.0f);
		break;
	case 0x40000392:
		unk148->getFrameCtrl(3)->setFrame(9.0f);
		break;
	case 0x40000390:
		unk148->getFrameCtrl(3)->setFrame(11.0f);
		break;
	}
}

void TEggYoshi::decideRandomLoveFruit()
{
	if (gpMarDirector->getCurrentMap() == 7 && gpMarDirector->unk7D == 1) {
		unk14C = 0x40000392;
	} else if (gpMarDirector->getCurrentMap() == 3) {
		unk14C = 0x40000393;
	} else if (gpMarDirector->getCurrentMap() == 1
	           && strcmp(mName, "ヨッシーの卵（ieマリオ用）") == 0) {
		unk14C = 0x40000394;
	} else {
		switch ((int)(rand() * 0.000030517578f * 4.0f)) {
		case 0:
			unk14C = 0x40000394;
			break;
		case 1:
			unk14C = 0x40000391;
			break;
		case 2:
			unk14C = 0x40000392;
			break;
		default:
			unk14C = 0x40000390;
			break;
		}
	}
}

void TEggYoshi::touchFruit(THitActor* fruit)
{
	if (isState(0xE) || isState(6))
		return;

	if (unk14C == fruit->mActorType) {
		startAnim(1);
		unk148->getFrameCtrl(3)->setFrame(11.0f);

		f32 dx    = fruit->mPosition.x - mPosition.x;
		f32 dz    = fruit->mPosition.z - mPosition.z;
		mRotation.y = matan(dz, dx) * 0.005493164f;

		mState = 0xB;
		unk150 = (TMapObjGeneral*)fruit;

		if (gpMSound->gateCheck(0x483F))
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x483F, 0, nullptr,
			                                             0);
	} else if (animIsFinished()) {
		startAnim(2);
		unk148->getFrameCtrl(3)->setFrame(12.0f);

		if (gpMSound->gateCheck(0x483E))
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x483E, 0, nullptr,
			                                             0);

		mState = 0xD;
	}
}

void TEggYoshi::touchActor(THitActor* actor)
{
	if (isState(1) || isState(0xD)) {
		if (actor->isActorType(0x80000001)) {
			TTakeActor* taker = (TTakeActor*)actor;
			if (taker->mHeldObject && TMapObjBase::isFruit(taker->mHeldObject)) {
				TTakeActor* held = taker->mHeldObject;
				touchFruit(held);
			}
		}

		if (TMapObjBase::isFruit(actor))
			touchFruit(actor);
	}
}

void TEggYoshi::control()
{
	TMapObjBase::control();

	switch ((u32)mState) {
	case 1:
		break;
	case 0xD:
		if (animIsFinished()) {
			startAnim(0);
			startBalloonAnim();
			mState = 1;
		}
		break;
	case 0xB:
		if (animIsFinished()) {
			startAnim(3);
			TYoshi* yoshi = (TYoshi*)SMS_GetYoshi();
			BOOL notEgg = ((u8)yoshi->mState == TYoshi::EGG) ? FALSE : TRUE;
			if (!notEgg) {
				JGeometry::TVec3<f32> pos(mPosition);
				yoshi->appearFromEgg(pos, mRotation.y, this);
				yoshi->setEggYoshiPtr(this);
			}
			mState = 0xC;
		}
		break;
	case 0xC:
		if (animIsFinished()) {
			makeObjDead();
			mState = 0;
		}
		break;
	case 0xF: {
		JGeometry::TVec3<f32> velocity(mVelocity);
		if (velocity.y == 0.0f)
			mState = 0x10;
		break;
	}
	case 0x10:
		break;
	}
}

void TEggYoshi::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TMapObjGeneral::perform(flags, graphics);

	if (isState(0xC) || isState(0) || isState(6) || isState(2)
	    || isState(0xE) || isState(0xF))
		return;

	if (isState(0x10))
		return;

	if (flags & 2) {
		MtxPtr modelMtx = getModel()->mNodeMatrices[0];
		PSMTXCopy(modelMtx, unk148->getModel()->getBaseTRMtx());
	}

	unk148->perform(flags, graphics);
}

void TEggYoshi::startFruit()
{
	receiveMessage(nullptr, HIT_MESSAGE_UNK10);

	if (isState(0) || isState(0xE) || isState(0xF) || isState(0x10))
		receiveMessage(nullptr, HIT_MESSAGE_UNK10);
}

BOOL TEggYoshi::receiveMessage(THitActor* sender, u32 message)
{
	if (message == HIT_MESSAGE_TAKE) {
		hold((TTakeActor*)sender);
		return TRUE;
	}

	if (message == HIT_MESSAGE_UNK7 || message == HIT_MESSAGE_UNK8) {
		mVelocity.y = 10.0f;
		offLiveFlag(LIVE_FLAG_UNK10);
		mState = 0xF;
		return TRUE;
	}

	if (message == HIT_MESSAGE_UNK10) {
		JGeometry::TVec3<f32> velocity(mVelocity);

		makeObjAppeared();
		mVelocity.y = velocity.y;
		offLiveFlag(LIVE_FLAG_UNK10);
		decideRandomLoveFruit();
		startBalloonAnim();
		mState = 1;
		return TRUE;
	}

	if (message == HIT_MESSAGE_UNK6) {
		makeObjAppeared();
		decideRandomLoveFruit();
		startBalloonAnim();
	}

	return FALSE;
}

void TEggYoshi::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);

	if (strcmp(unkF4, "eggYoshiEvent") == 0) {
		if (TFlagManager::smInstance->getFlag(0x60003) == 1) {
			mState = 0xE;
		} else {
			makeObjDead();
			return;
		}
	} else if (gpMarDirector->getCurrentMap() == 1) {
		if (!TFlagManager::smInstance->getBool(0x1038F)) {
			makeObjDead();
			return;
		}
	} else if (!TFlagManager::smInstance->getShineFlag(0x21)) {
		makeObjDead();
		return;
	}

	unk148 = SMS_MakeMActorWithAnmData(
	    "/scene/mapObj/eggYoshi_fukidashi.bmd",
	    mManager->getMActorAnmData(), 3, 0x10210000);
	MtxPtr modelMtx = getModel()->mNodeMatrices[0];
	PSMTXCopy(modelMtx, unk148->getModel()->getBaseTRMtx());
	unk148->setBck("eggyoshi_fukidashi_wait");
	unk148->setBtp("eggyoshi_fukidashi");
	unk148->getFrameCtrl(3)->setRate(0.0f);
	decideRandomLoveFruit();

	if (!isState(0xE))
		startBalloonAnim();
}

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
		taken(actor);

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
		hold((TTakeActor*)sender);
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
		taken(sender);
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
		gpMarDirector->mConsole->startAppearBalloon(0xE0057, true);
		unk166 = TRUE;
	}

	if (!unk15C && !unk166) {
		gpMarDirector->mConsole->startAppearBalloon(0xE0056, true);
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
	if (strcmp(valid, "valid") == 0)
		unk15C = TRUE;
	else
		unk15C = FALSE;

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

	TMapObjBase::initPacketMatColor(getModel(), GX_TEVREG1,
	                                (const GXColorS10*)&unk15E);
	startAnim(3);
	TMapObjBase::initPacketMatColor(getModel(), GX_TEVREG1,
	                                (const GXColorS10*)&unk15E);
	startAnim(2);
	TMapObjBase::initPacketMatColor(getModel(), GX_TEVREG1,
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
    , unk166(FALSE)
{
	unk15E = 0xFF;
	unk160 = 0xFF;
	unk162 = 0xFF;
	unk164 = 100;
}
