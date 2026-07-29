#include <MoveBG/MapObjPinna.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/EffectObj.hpp>
#include <MoveBG/Item.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Map/MapData.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorUtil.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Player/MarioAccess.hpp>
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
#include <stdlib.h>
#include <string.h>

static s32 switchSnd;

f32 TShellCup::mOpenRotMax      = 90.0f;
f32 TShellCup::mShellDamageRot  = 45.0f;
f32 TShellCup::mWaterOpenAccel  = 5.0f;
f32 TShellCup::mCloseAccel      = 3.5f;
f32 TMerrygoround::mRotSpeed    = 0.1f;

#define START_PINNA_SOUND(soundID, pos)                                        \
	do {                                                                       \
		if (gpMSound->gateCheck(soundID)) {                                    \
			MSoundSESystem::MSoundSE::startSoundActor(soundID, (Vec*)&(pos),   \
			                                          0, nullptr, 0, 4);       \
		}                                                                      \
	} while (0)

#pragma dont_inline on
void MsMtxSetRotX(MtxPtr mtx, f32 angle)
{
	f32 s = JMASin(angle);
	f32 c = JMACos(angle);

	mtx[0][0] = 1.0f;
	mtx[0][1] = 0.0f;
	mtx[0][2] = 0.0f;
	mtx[0][3] = 0.0f;
	mtx[1][0] = 0.0f;
	mtx[1][1] = c;
	mtx[1][2] = -s;
	mtx[1][3] = 0.0f;
	mtx[2][0] = 0.0f;
	mtx[2][1] = s;
	mtx[2][2] = c;
	mtx[2][3] = 0.0f;
}
#pragma dont_inline off

static inline void setVecFromMtx(JGeometry::TVec3<f32>& dst, MtxPtr mtx)
{
	dst.x = mtx[0][3];
	dst.y = mtx[1][3];
	dst.z = mtx[2][3];
}

static inline TMapObjBase* makePinnaObj(const char* name)
{
	JGeometry::TVec3<f32> pos(0.0f, 0.0f, 0.0f);
	JGeometry::TVec3<f32> rot(0.0f, 0.0f, 0.0f);
	JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
	return TMapObjBaseManager::newAndRegisterObj(name, pos, rot, scale);
}

TPinnaCoaster::TPinnaCoaster(const char* name)
    : TMapObjBase(name)
    , unk138(nullptr)
{
	unk140.zero();
}

void TPinnaCoaster::initMapObj()
{
	TMapObjBase::initMapObj();

	unk138 = SMS_MakeMActorWithAnmData(
	    "/scene/mapObj/CoasterRail.bmd", gpMapObjManager->getUnk40(), 3,
	    0x10210000);
	unk138->setBck("coasterrail");
	unk138->setFrameRate(SMSGetAnmFrameRate() * 0.25f, 0);

	MsMtxSetXYZRPH(unk138->getModel()->getBaseTRMtx(), mPosition.x, mPosition.y,
	               mPosition.z, mRotation.x, mRotation.y, mRotation.z);
	unk140 = mPosition;
}

void TPinnaCoaster::control()
{
	TMapObjBase::control();

	if (unk138 != nullptr) {
		unk138->frameUpdate();
		unk138->calc();
		if (getModel() != nullptr)
			MTXCopy(getModel()->getBaseTRMtx(),
			        unk138->getModel()->getBaseTRMtx());
	}

	JGeometry::TVec3<f32> delta = mPosition;
	delta.sub(unk140);
	f32 speed = delta.length();

	if (switchSnd != 0 && gpMSound->gateCheck(0x305a)) {
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x305a, (Vec*)&mPosition, nullptr, speed, 0, 0, nullptr, 0, 4);
	}
	switchSnd ^= 1;
	unk140 = mPosition;
}

void TAmiKing::touchPlayer(THitActor*)
{
	SMS_SendMessageToMario(this, 9);
}

void TAmiKing::bind()
{
	if (checkLiveFlag(LIVE_FLAG_UNK10)) {
		gpMap->checkGround(mPosition.x, mPosition.y + mHeadHeight, mPosition.z,
		                   &mGroundPlane);
	} else {
		TLiveActor::bind();
	}
}

void TAmiKing::calcRootMatrix()
{
	TMapObjBase::calcRootMatrix();

	gpMarioParticleManager->emitAndBindToMtxPtr(0x184,
	                                            (MtxPtr)getModel()->mNodeMatrices,
	                                            1, this);

	MtxPtr mtx = getModel()->getAnmMtx(6);
	setVecFromMtx(unk13C, mtx);

	if (unk138 == 0) {
		Mtx rot;
		Vec offset = { 0.0f, 0.0f, 200.0f };
		MsMtxSetRotRPH(rot, 0.0f, mRotation.y, 0.0f);
		MTXMultVec(rot, &offset, &offset);
		unk13C.x += offset.x;
		unk13C.y += offset.y;
		unk13C.z += offset.z;

		JGeometry::TVec3<f32> scale(2.0f, 2.0f, 2.0f);
		JPABaseEmitter* emitter
		    = gpMarioParticleManager->emitAndBindToPosPtr(0x124, &unk13C, 1, this);
		if (emitter != nullptr)
			emitter->setScale(scale);
		START_PINNA_SOUND(0x214f, mPosition);
	} else {
		START_PINNA_SOUND(0x2120, mPosition);
	}
}

void TAmiKing::moveObject()
{
	TLiveActor::moveObject();

	if (unk138 != 0) {
		if (mMActor->checkCurAnm("amiking_flying1_start", 0)
		    && mMActor->curAnmEndsNext()) {
			mMActor->setBck("amiking_flying1_loop");
		}

		u16 type = mGroundPlane->mBGType;
		if ((type == 0x100 || type == 0x101 || (u16)(type - 0x102) <= 3
		     || type == 0x4104)
		    && !checkLiveFlag(LIVE_FLAG_AIRBORNE)) {
			START_PINNA_SOUND(0x2921, mPosition);
			JGeometry::TVec3<f32> scale(4.0f, 4.0f, 4.0f);
			JPABaseEmitter* emitter
			    = gpMarioParticleManager->emitAndBindToMtxPtr(
			        0xca, (MtxPtr)mMActor->getModel()->mNodeMatrices, 0, nullptr);
			if (emitter != nullptr)
				emitter->setScale(scale);

			TEffectColumWater* enemy
			    = (TEffectColumWater*)gpConductor->makeOneEnemyAppear(
			        mPosition, "エフェクト水柱マネージャー", 1);
			if (enemy != nullptr)
				enemy->generate(mPosition, scale);

			gpItemManager->makeShineAppearWithDemo("シャイン（観覧車シャインwp）",
			                                      "観覧車シャインカメラ",
			                                      mPosition.x, mPosition.y,
			                                      mPosition.z);

			TFerrisWheel* wheel
			    = JDrama::TNameRefGen::search<TFerrisWheel>("FerrisWheel");
			gpMarDirector->fireStartDemoCamera(
			    "観覧車正常化カメラ", &wheel->mPosition, -1, 0.0f, true,
			    TFerrisWheel::becomeCalmlyCallback, 0, wheel,
			    JDrama::TFlagT<u16>(0));
			kill();
		}
		return;
	}

	const TLiveActor* actor = mGroundPlane->getActor();
	if (actor != nullptr && actor->getActorType() == 0x4000006a) {
		const TMapObjBase* obj = static_cast<const TMapObjBase*>(actor);
		if (obj->mState >= 3 && obj->mState <= 6) {
			unk138 = 1;
			mVelocity.set(5.0f, 10.0f, -10.0f);
			offLiveFlag(LIVE_FLAG_UNK10);
			mMActor->setBck("amiking_flying1_start");
			setAnmSound(nullptr);
			gpMarDirector->fireStartDemoCamera(
			    "観覧ボス撃沈カメラ", &mPosition, -1, 0.0f, true, nullptr, 0,
			    nullptr, JDrama::TFlagT<u16>(0));
		}
	}
}

void TAmiKing::initMapObj()
{
	TMapObjBase::initMapObj();
	initAnmSound();
	mMActor->setBck("amiking_sleep1");
	setAnmSound("/scene/mapObj/amiking_sleep1.bas");
	offLiveFlag(LIVE_FLAG_UNK10);
}

void TAmiKing::loadAfter()
{
	TMapObjBase::loadAfter();
	SMS_LoadParticle("/scene/Mapobj/amiking.jpa", 0x184);
}

u32 TAmiKing::touchWater(THitActor*) { return TRUE; }

void TWaterRecoverObj::touchPlayer(THitActor* sender)
{
	if (!sender->isActorTypeOf(ACTOR_TYPE_PLAYER))
		return;

	if (!(mLifeTimer > 0 ? true : false)) {
		sender->receiveMessage(this, HIT_MESSAGE_ATTACK);
		mLifeTimer = 600;
	}
}

void TPinnaEntrance::loadAfter()
{
	TMapObjBase::loadAfter();

	JGeometry::TVec3<f32> rot(90.0f, 0.0f, 0.0f);
	JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
	JGeometry::TVec3<f32>* scalePtr = &scale;
	TMapObjBaseManager::newAndRegisterObj("GateManta", mPosition, rot,
	                                      *scalePtr);
}

void TBalloonKoopaJr::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	SMS_LoadParticle("/scene/mapObj/balloonKoopaJr.jpa", 0x5a);
	SMS_LoadParticle("/scene/mapObj/balloonKoopaJrA.jpa", 0x5b);
	SMS_LoadParticle("/scene/mapObj/balloonKoopaJrB.jpa", 0x5c);

	unk148 = mPosition;
	if (getModel() != nullptr) {
		JUTNameTab* names = getModel()->mModelData->getJointName();
		s32 idx           = names->getIndex("center");
		if (idx >= 0)
			setVecFromMtx(unk148, getModel()->getAnmMtx(idx));
	}
}

void TBalloonKoopaJr::kill()
{
	TMapObjGeneral::kill();

	emitAndScale(0x5a, 0, &unk148);
	emitAndScale(0x5b, 0, &unk148);
	emitAndScale(0x5c, 0, &unk148);
	TFlagManager::smInstance->incFlag(0x60001, 1);
	START_PINNA_SOUND(0x28b8, mPosition);
}

void TBalloonKoopaJr::touchActor(THitActor*) { kill(); }

void TChangeStageMerrygoround::calc()
{
	if (unk13C != 0) {
		JGeometry::TVec3<f32>* marioPos = gpMarioPos;
		gpMarioParticleManager->emitAndBindToPosPtr(0x100, marioPos, 1, this);
		marioPos = gpMarioPos;
		gpMarioParticleManager->emitAndBindToPosPtr(0x101, marioPos, 1, this);
	}
}

void TChangeStageMerrygoround::touchPlayer(THitActor* sender)
{
	bool timerActive = mLifeTimer > 0 ? true : false;
	if (timerActive)
		return;

	u8* yoshi = (u8*)SMS_GetYoshi();
	if (yoshi[0xd0] == 1) {
		gpMSound->startSoundSystemSE(0x4840, 0, nullptr, 0);
		TMapObjChangeStage::touchPlayer(sender);
		unk13C = 1;
	} else {
		gpMSound->startSoundSystemSE(0x483e, 0, nullptr, 0);
	}
	mLifeTimer = 600;
}

TMerrygoround::TMerrygoround(const char* name)
    : TMapObjBase(name)
    , unk1A0(nullptr)
    , unk1A4(0)
{
	for (int i = 0; i < 2; ++i) {
		unk138[i] = nullptr;
		unk140[i] = 0;
	}
	for (int i = 0; i < 9; ++i) {
		unk144[i] = nullptr;
		unk18C[i] = 0;
		unk168[i] = nullptr;
	}
}

void TMerrygoround::initMapObj()
{
	TMapObjBase::initMapObj();

	int eggNo  = 0;
	int poleNo = 0;
	J3DModelData* data = getModel()->mModelData;
	JUTNameTab* names  = data->getJointName();
	for (u16 i = 1; i < data->getJointNum(); ++i) {
		const char* name = names->getName(i);
		if (strstr(name, "egg") != nullptr && eggNo < 2) {
			unk140[eggNo++] = i;
		} else if (strcmp(name, "yoshi_warp") == 0) {
			unk1A4 = i;
		} else if (strcmp(name, "up") != 0 && strcmp(name, "down") != 0
		           && strcmp(name, "KAGE_2") != 0 && poleNo < 9) {
			unk18C[poleNo++] = i;
		}
	}

	for (int i = 0; i < 2; ++i) {
		unk138[i] = makePinnaObj("merry_egg");
		unk138[i]->initMapObj();
	}

	for (int i = 0; i < 9; ++i) {
		unk144[i] = makePinnaObj("merry_pole");
		unk144[i]->initMapObj();
		unk168[i] = new TMapCollisionMove;
		unk168[i]->init("/scene/mapObj/merry_yoshi.col", 0, this);
		unk168[i]->setUp();
	}

	unk1A0 = makePinnaObj("ChangeStageMerrygoround");
	if (unk1A0 != nullptr) {
		((TMapObjChangeStage*)unk1A0)->unk138 = 0x29;
		unk1A0->mScaling.x                  = 1.5f;
		unk1A0->appear();
		TMapObjBase::joinToGroup("マップグループ", unk1A0);
	}
}

void TMerrygoround::draw() const { }

void TMerrygoround::control()
{
	TMapObjBase::control();

	mRotation.y += mRotSpeed;
	if (mRotation.y > 360.0f)
		mRotation.y -= 360.0f;

	for (int i = 0; i < 2; ++i) {
		MtxPtr mtx = getModel()->getAnmMtx(unk140[i]);
		unk138[i]->setModelMtx(mtx);
		unk138[i]->mPosition.x = mtx[0][3];
		unk138[i]->mPosition.y = mtx[1][3];
		unk138[i]->mPosition.z = mtx[2][3];
	}

	for (int i = 0; i < 9; ++i) {
		MtxPtr mtx = getModel()->getAnmMtx(unk18C[i]);
		((TMerryPole*)unk144[i])->unk138.set(mtx);
		unk144[i]->mPosition.x = mtx[0][3];
		unk144[i]->mPosition.y = mtx[1][3] - 600.0f;
		unk144[i]->mPosition.z = mtx[2][3];
	}

	if (SMS_IsMarioOnYoshi())
		unk1A0->offLiveFlag(LIVE_FLAG_DEAD);
	else
		unk1A0->onLiveFlag(LIVE_FLAG_DEAD);

	MtxPtr mtx = getModel()->getAnmMtx(unk1A4);
	unk1A0->mPosition.x = mtx[0][3];
	unk1A0->mPosition.y = mtx[1][3] - 600.0f;
	unk1A0->mPosition.z = mtx[2][3];
}

TPinnaShell::TPinnaShell()
    : THitActor("シェル")
    , unk68(0)
    , unk6C(0.0f)
    , unk70(0.0f)
    , unk74(nullptr)
    , unk78(nullptr)
    , unk7C(0)
    , unk80(nullptr)
    , unk84(nullptr)
    , unk88(nullptr)
    , unk8C(nullptr)
{
	initHitActor(0x4000013a, 1, 0x80000000, 250.0f, 400.0f, 250.0f,
	             200.0f);
}

TShellCup::TShellCup(const char* name)
    : TMapObjBase(name)
    , unk498(nullptr)
    , unk49C(nullptr)
    , unk4A0(nullptr)
{
}

void TShellCup::initMapObj()
{
	TMapObjBase::initMapObj();

	for (int i = 0; i < 6; ++i) {
		TPinnaShell& shell = unk138[i];
		shell.unk68        = 0;
		shell.unk6C        = 0.0f;
		shell.unk70        = 8.0f;
		shell.unk74        = getModel()->getAnmMtx(i + 1);
		shell.unk78        = getModel()->mModelData->getJointNodePointer(i + 1);
		shell.unk7C        = (s32)((f32)rand() * (1.0f / 32768.0f) * 1200.0f);
		shell.unk8C        = this;

		TMapObjBase::joinToGroup("オブジェクトグループ", &shell);

		shell.unk84 = new TMapCollisionMove;
		shell.unk84->init("/mapObj/ShellCup", 0, this);
		shell.unk84->setUp();

		shell.unk88 = new TDamageObj("ダメージオブジェ");
		shell.unk88->mPosition = mPosition;
		shell.unk88->mScaling.set(2.0f, 1.2f, 2.0f);
		shell.unk88->init(0x10000036);
		shell.unk88->onHitFlag(HIT_FLAG_NO_COLLISION);
	}

	TMapCollisionStatic* collision = new TMapCollisionStatic;
	collision->init("/mapObj/ShellCup_rink", 2, this);
	collision->setMtx(getModel()->getBaseTRMtx());
	collision->setUp();
}

void TShellCup::loadAfter()
{
	TMapObjBase::loadAfter();

	for (int i = 0; i < 6; ++i)
		TMapObjBase::joinToGroup("オブジェクトグループ", unk138[i].unk88);

	unk498 = (TCoin*)TMapObjBaseManager::newAndRegisterObj("coin_blue");
	unk49C = gpItemManager->newAndRegisterCoinReal();
	unk4A0 = gpItemManager->newAndRegisterCoinReal();

	if (unk498 != nullptr) {
		unk498->setUnk134(2);
		if (!TFlagManager::smInstance->getBlueCoinFlag(
		        gpMarDirector->getCurrentMap(), unk498->getUnk134())) {
			unk498->appear();
			unk138[0].unk80 = unk498;
		}
	}
	if (unk49C != nullptr) {
		unk49C->appear();
		unk49C->onMapObjFlag(0x10000000);
		unk138[2].unk80 = unk49C;
	}
	if (unk4A0 != nullptr) {
		unk4A0->appear();
		unk4A0->onMapObjFlag(0x10000000);
		unk138[4].unk80 = unk4A0;
	}
}

void TShellCup::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TMapObjBase::perform(flags, graphics);

	if ((flags & 2) == 0)
		return;

	u8 directorState = gpMarDirector->unk124;
	bool talkMode    = true;
	if (directorState != 1 && directorState != 2)
		talkMode = false;
	if (talkMode) {
		bool canPerform = true;
		if (directorState != 3 && directorState != 4)
			canPerform = false;
		if (!canPerform)
			return;
	}

	for (int i = 0; i < 6; ++i) {
		TPinnaShell& shell = unk138[i];
		Mtx rot;
		MsMtxSetRotX(rot, shell.unk6C);
		TMapObjBase::concatOnlyRotFromRight(shell.unk74, rot, shell.unk74);
	}

	TCoin* coin = unk498;
	if (!(coin->mLiveFlag & LIVE_FLAG_DEAD)) {
		coin->mPosition.x = unk138[0].mPosition.x;
		coin->mPosition.y = unk138[0].mPosition.y;
		coin->mPosition.z = unk138[0].mPosition.z;
	}
	coin = unk49C;
	if (!(coin->mLiveFlag & LIVE_FLAG_DEAD)) {
		coin->mPosition.x = unk138[2].mPosition.x;
		coin->mPosition.y = unk138[2].mPosition.y;
		coin->mPosition.z = unk138[2].mPosition.z;
	}
	coin = unk4A0;
	if (!(coin->mLiveFlag & LIVE_FLAG_DEAD)) {
		coin->mPosition.x = unk138[4].mPosition.x;
		coin->mPosition.y = unk138[4].mPosition.y;
		coin->mPosition.z = unk138[4].mPosition.z;
	}
}

void TShellCup::control()
{
	mMActor->calc();

	for (int i = 0; i < 6; ++i)
		unk138[i].control();
}

inline static void startShellSystemSound(u32 soundID)
{
	if (gpMSound->gateCheck(soundID))
		MSoundSESystem::MSoundSE::startSoundSystemSE(soundID, 0, nullptr, 0);
}

void TPinnaShell::control()
{
	if (unk7C > 0)
		--unk7C;

	switch (unk68) {
	case 0:
		if (unk6C < 0.0f) {
			unk6C += TShellCup::mCloseAccel
			         * (0.5f + (f32)rand() * (1.0f / 32768.0f));
		} else {
			unk6C = 0.0f;
		}
		break;
	case 1:
		unk6C -= 0.8f;
		if (unk6C < -TShellCup::mOpenRotMax) {
			unk6C = -TShellCup::mOpenRotMax;
			unk7C = 360;

			if (unk80 != nullptr
			    && !(((TMapObjBase*)unk80)->mLiveFlag & LIVE_FLAG_DEAD)) {
				if (unk80->mActorType == 0x20000010)
					startShellSystemSound(0x483f);
				else
					startShellSystemSound(0x4813);
			} else {
				startShellSystemSound(0x483d);
			}

			unk68 = 2;
		}
		break;
	case 2:
		if (unk7C <= 0) {
			unk68 = 3;
			START_PINNA_SOUND(0x389f, mPosition);
		}
		break;
	case 3:
		unk6C += unk70;
		if (!(unk6C < -TShellCup::mShellDamageRot))
			unk88->offHitFlag(HIT_FLAG_NO_COLLISION);
		if (!(unk6C < 0.0f)) {
			unk6C = 0.0f;
			unk68 = 0;
			unk88->onHitFlag(HIT_FLAG_NO_COLLISION);
		}
		break;
	}

	TMapObjBase* parent = unk8C;
	mPosition.x = parent->mPosition.x + 0.7f * (unk74[0][3] - parent->mPosition.x);
	mPosition.y = unk74[1][3] - 100.0f;
	mPosition.z = parent->mPosition.z + 0.7f * (unk74[2][3] - parent->mPosition.z);

	unk88->mPosition.x = mPosition.x;
	unk88->mPosition.y = mPosition.y;
	unk88->mPosition.z = mPosition.z;

	if (mColCount != 0) {
		Mtx rot;
		MsMtxSetRotX(rot, unk6C);
		TMapObjBase::concatOnlyRotFromRight(unk74, rot, rot);
		unk84->moveMtx(rot);
	}
}

BOOL TPinnaShell::receiveMessage(THitActor* sender, u32 message)
{
	BOOL result = false;
	if (message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		gpMarioParticleManager->emit(
		    PARTICLE_MS_ENM_WATHIT, &sender->mPosition, 0, nullptr);
		gpMSound->startSoundSet(0x6802, (Vec*)&mPosition, 0, 0.0f, 0, 0, 4);

		if (unk68 == 0)
			unk6C -= TShellCup::mWaterOpenAccel;

		if (unk6C < -TShellCup::mOpenRotMax)
			unk68 = 1;
		result = true;
	}
	return result;
}

TViking::TViking(const char* name)
    : THorizontalViking(name)
    , unk14C(0)
    , unk150(0.0f)
    , unk154(0.0f)
    , unk158(0.0f)
{
}

void THorizontalViking::initMapObj()
{
	TMapObjBase::initMapObj();
	unk138 = 2500.0f;
	unk13C = 0.0008f;
	unk140 = 0.23f;
	reset();
}

void TViking::initMapObj()
{
	unk14C = 1;
	if (strcmp(mName, "viking 0") == 0) {
		unk138 = 1400.0f;
		unk13C = 0.001f;
		unk154 = 1.001f;
		unk158 = 0.999f;
		unk140 = -0.3f;
		unk150 = 0.3f;
	} else {
		unk138 = 1400.0f;
		unk13C = 0.001f;
		unk154 = 1.001f;
		unk158 = 0.999f;
		unk140 = 0.3f;
		unk150 = 0.3f;
	}
	mPosition.y -= unk138;
	TMapObjBase::initMapObj();
}

void TViking::loadAfter()
{
	TMapObjBase::loadAfter();
	reset();
}

void THorizontalViking::reset()
{
	unk144 = unk140;
	unk148 = 0.0f;
	if (unk144 > 0.0f) {
		mState = 1;
		return;
	}
	mState = 2;
}

void TViking::reset()
{
	unk144 = unk140;
	unk148 = 0.0f;
	if (unk140 > 0.0f) {
		mState = 1;
		return;
	}
	mState = 2;
}

inline static void swingViking(THorizontalViking* viking)
{
	switch (viking->mState) {
	case 1:
		viking->unk144 -= viking->unk13C;
		viking->unk148 += viking->unk144;
		if (viking->unk148 < 0.0f)
			viking->mState = 2;
		break;
	case 2:
		viking->unk144 += viking->unk13C;
		viking->unk148 += viking->unk144;
		if (viking->unk148 > 0.0f)
			viking->mState = 1;
		break;
	}
}

inline static void applyVikingPosition(THorizontalViking* viking, BOOL rotate)
{
	viking->mPosition.x = viking->mInitialPosition.x
	                     + viking->unk138
	                           * sinf((viking->unk148 / 180.0f) * 3.14f);
	f32 y = viking->mInitialPosition.y
	        + viking->unk138
	              * (1.0f - cosf((viking->unk148 / 180.0f) * 3.14f));
	viking->mPosition.y = viking->mYOffset + y;
	if (rotate) {
		viking->mRotation.z = viking->unk148;
		viking->updateObjMtx();
	}
}

inline static void startVikingSwingSound(TViking* viking)
{
	f32 volume = fabsf(viking->unk144);
	if (gpMSound->gateCheck(0x38a0)) {
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x38a0, &viking->mPosition, nullptr, volume, 0, 0, nullptr, 0, 4);
	}
}

void THorizontalViking::control()
{
	TMapObjBase::control();
	swingViking(this);
	applyVikingPosition(this, false);
}

void TViking::control()
{
	switch (unk14C) {
	case 0:
		swingViking(this);
		break;
	case 1:
		roll();
		break;
	}
	applyVikingPosition(this, true);
}

void TViking::roll()
{
	switch (mState) {
	case 1:
		unk144 *= unk154;
		unk144 -= unk13C;
		unk148 += unk144;
		if (unk148 < 0.0f) {
			startVikingSwingSound(this);
			mState = 2;
		}
		if (unk148 > 180.0f) {
			unk148 -= 360.0f;
			startVikingSwingSound(this);
			mState = 4;
		}
		break;
	case 2:
		unk144 *= unk154;
		unk144 += unk13C;
		unk148 += unk144;
		if (unk148 > 0.0f) {
			startVikingSwingSound(this);
			mState = 1;
		}
		if (unk148 < -180.0f) {
			unk148 += 360.0f;
			startVikingSwingSound(this);
			mState = 3;
		}
		break;
	case 3:
		unk144 *= unk158;
		unk144 -= unk13C;
		unk148 += unk144;
		if (unk148 < 0.0f) {
			startVikingSwingSound(this);
			if (unk144 > -unk150)
				mState = 2;
			else
				mState = 4;
		}
		break;
	case 4:
		unk144 *= unk158;
		unk144 += unk13C;
		unk148 += unk144;
		if (unk148 > 0.0f) {
			startVikingSwingSound(this);
			if (unk144 < unk150)
				mState = 1;
			else
				mState = 3;
		}
		break;
	}
}

TFerrisWheel::TFerrisWheel(const char* name)
    : TMapObjBase(name)
    , unk138(0)
    , unk13C(nullptr)
    , unk140(0.0f)
{
}

void TFerrisWheel::initMapObj()
{
	TMapObjBase::initMapObj();

	unk138 = getModel()->mModelData->getJointNum() - 1;
	unk13C = new TMapObjBase*[unk138];
	for (int i = 0; i < unk138; ++i) {
		unk13C[i] = makePinnaObj("FerrisGondola");
		unk13C[i]->initMapObj();
	}

	if (gpMarDirector->getCurrentStage() == 2)
		unk140 = 10.0f;
	else
		unk140 = SMSGetAnmFrameRate() * 0.25f;
}

void TFerrisWheel::control()
{
	TMapObjBase::control();

	if (mState == 2 && mLifeTimer <= 0) {
		f32 target = SMSGetAnmFrameRate() * 0.25f;
		if (unk140 > target) {
			unk140 -= 0.015f;
		} else {
			mState = 1;
		}
	}

	f32 target = SMSGetAnmFrameRate() * 0.25f;
	if (unk140 > target) {
		MSound* sound = gpMSound;
		if (sound->gateCheck(0x3085)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x3085, &mPosition, 0, (JAISound**)&sound->unk80, 0, 4);
		}
	}

	f32 speed = unk140;
	f32 frame = mMActor->getFrameCtrl(0)->getFrame();
	mMActor->getFrameCtrl(0)->setFrame(speed + frame);

	for (int i = 0; i < unk138; ++i) {
		TMapObjBase* gondola = unk13C[i];
		MtxPtr mtx = getModel()->getAnmMtx(i + 1);
		PSMTXCopy(mtx, gondola->getModel()->mNodeMatrices[0]);
		gondola->mPosition.x = mtx[0][3];
		gondola->mPosition.y = mtx[1][3] + gondola->mYOffset;
		gondola->mPosition.z = mtx[2][3];
	}
}

s32 TFerrisWheel::becomeCalmlyCallback(u32 actor, u32 param)
{
	if (param == 0) {
		TFerrisWheel* wheel = (TFerrisWheel*)actor;
		wheel->mState      = 2;
		MSound* sound = gpMSound;
		if (sound->unk80 != 0) {
			((JAISound*)sound->unk80)->setVolume(0.0f, 200, 0);
			((JAISound*)sound->unk80)->setPitch(0.5f, 200, 0);
		}
		wheel->mLifeTimer = 120;
	}
	return 0;
}
