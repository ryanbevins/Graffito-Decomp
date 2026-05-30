#include <MoveBG/MapObjBianco.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Map/MapCollisionData.hpp>
#include <Map/MapCollisionManager.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorUtil.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSSetSound.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <System/Application.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/Particles.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>
#include <JSystem/JMath.hpp>
#include <dolphin/mtx.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern MSound* gpMSound;
extern JGeometry::TVec3<f32>* gpMarioPos;

static f32 sRadius = 2380.0f;
static f32 sSubZ   = 150.0f;
static f32 sSpeed  = 0.05f;
static f32 sAngleAdd;

f32 TMapObjRootPakkun::mTremblePower = 15.0f;
f32 TMapObjRootPakkun::mTrembleAccel = 0.95f;
f32 TMapObjRootPakkun::mTrembleBrake = 0.98f;
s32 TMapObjRootPakkun::mTrembleTime  = 360;

f32 TBiancoWatermillVertical::mRotAccel         = 0.15f;
f32 TBiancoWatermillVertical::mRotSpeedDownRate = 0.005f;
f32 TBiancoWatermillVertical::mRotSpeedMax      = 3.0f;
f32 TBiancoWatermillVertical::mBridgeRotRate    = 0.03f;

f32 TBiancoMiniWindmill::mRotWaterAccel = 0.01f;
f32 TBiancoMiniWindmill::mFriction      = 0.01f;
f32 TBiancoMiniWindmill::mRotSpeedMax   = 10.0f;

static f32 sMessengerPosZ = 200.0f;
static f32 sMessengerPosY = 6400.0f;

f32 TLeafBoatRotten::mAlphaDownSpeed          = 0.5f;
f32 TLeafBoatRotten::mCollisionRemoveAlpha    = 100.0f;
s16 TLeafBoatRotten::mRottenColor[4]          = { 100, 100, 180, 255 };

void TWoodLog::control()
{
	TMapObjFloatOnSea::control();

	Mtx inv;
	JGeometry::TVec3<f32> marioPos;
	JGeometry::TVec3<f32> localPos;
	JGeometry::TVec3<f32> requestPos;

	PSMTXInverse(getModel()->mNodeMatrices[0], inv);
	marioPos.set(*gpMarioPos);
	PSMTXMultVec(inv, (Vec*)&marioPos, (Vec*)&localPos);

	if (SMS_IsMarioStatusTypeSwimming() && -232.0f < localPos.y
	    && -141.0f < localPos.x && localPos.x < 141.0f
	    && -441.0f < localPos.z && localPos.z < 441.0f) {
		if (localPos.x > 0.0f)
			localPos.x = 141.0f;
		else
			localPos.x = -141.0f;
		PSMTXMultVec(getModel()->mNodeMatrices[0], (Vec*)&localPos,
		             (Vec*)&requestPos);
		SMS_MarioMoveRequest(requestPos);
	}
}

TBellWatermill::TBellWatermill(const char* name)
    : TMapObjTurn(name)
    , unk16C(0.0f)
    , unk170(0.0f)
    , unk174(0.0f)
    , unk178(0.0f)
    , unk17C(0.0f)
    , unk180(0.0f)
    , unk184(0.0f)
    , unk188(0.0f)
    , unk18C(0.0f)
    , unk190(0)
    , unk194(nullptr)
    , unk198(nullptr)
    , unk19C(nullptr)
    , unk1A0(0)
    , unk1A4(0)
{
}

static inline TMapObjBase* findMapObj(const char* name)
{
	JDrama::TNameRefGen* gen = JDrama::TNameRefGen::instance;
	JDrama::TNameRef* root   = gen->mRootNameRef;
	u16 key                  = JDrama::TNameRef::calcKeyCode(name);
	return (TMapObjBase*)root->searchF(key, name);
}

void TBellWatermill::loadAfter()
{
	TMapObjTurn::loadAfter();

	unk150 = 2;
	unk15C = -7.0f;
	unk160 = -3.5f;
	unk164 = 10.0f;
	unk18C = 10.0f;
	unk174 = 1000.0f;
	unk180 = 4.0f;
	unk184 = 0.5f;
	unk16C = 5.0f;
	unk188 = 1.0f;
	unk17C = 360.0f;

	unk194 = (TBiancoBell*)findMapObj("BiaBell 0");
	unk198 = (TBiancoBell*)findMapObj("BiaBell 1");
	unk19C = (TBiancoBell*)findMapObj("BiaBell 2");
	unk190 = 1;
}

static inline void ringBiancoBell(TBiancoBell* bell)
{
	if (bell == nullptr)
		return;

	J3DFrameCtrl* ctrl = bell->mMActor->getFrameCtrl(0);
	if (ctrl->getFrame() == 0.0f
	    || ctrl->getFrame() + ctrl->getRate() >= (f32)ctrl->getEnd() - 1.0f) {
		bell->startAnim(bell->unk138);
		bell->mMActor->getFrameCtrl(0)->setRate(SMSGetAnmFrameRate());
		if (bell->unk13A && gpMSound->gateCheck(0x89B8)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x89B8, (const Vec*)&bell->mPosition, 0, nullptr, 0, 4);
		}
	}
}

void TBellWatermill::control()
{
	TMapObjBase::control();

	if (unk158 == 0.0f && unk178 == 0.0f && unk170 == 0.0f)
		return;

	if (unk158 > unk16C)
		unk158 = unk16C;
	else if (unk158 < -unk16C)
		unk158 = -unk16C;

	unk154 += unk158;
	while (unk154 >= 360.0f)
		unk154 -= 360.0f;
	while (unk154 < 0.0f)
		unk154 += 360.0f;

	f32 volume = fabsf(unk158);
	if (gpMSound->gateCheck(0x3044)) {
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x3044, (const Vec*)&mPosition, nullptr, volume, 0, 0, &unk1A4,
		    0, 4);
	}

	if (fabsf(unk158) < fabsf(unk160))
		unk158 = 0.0f;

	if (!unk190 && unk158 != 0.0f)
		unk158 -= unk160;

	if (unk170 < 0.0f) {
		if (fabsf(unk178) < unk17C) {
			unk170 = 0.0f;
			unk178 = 0.0f;
		} else {
			unk170 -= unk178;
			unk178 *= -unk188;
		}
	} else if (!unk190 && unk170 != 0.0f) {
		unk178 -= unk184;
	}

	unk170 += unk178;
	if (unk170 > unk174) {
		ringBiancoBell(unk194);
		ringBiancoBell(unk198);
		ringBiancoBell(unk19C);

		unk170 = unk174;
		if (unk1A0) {
			for (int i = 0; i < 5; ++i) {
				TMapObjBase* coin = gpItemManager->makeObjAppeared(0x2000000E);
				if (coin != nullptr) {
					coin->mPosition = mPosition;
					coin->mVelocity.x = 10.0f;
					coin->mVelocity.y
					    = 10.0f + 100.0f * ((f32)rand() * (1.0f / 32768.0f));
					coin->mVelocity.z
					    = 10.0f * ((f32)rand() * (1.0f / 32768.0f)) - 5.0f;
					coin->offLiveFlag(LIVE_FLAG_UNK10);
				}
			}
			unk1A0 = 0;
		}
	}

	mPosition.y = mInitialPosition.y + mYOffset + unk170;
	unk190     = 0;
	mRotation.z = unk154;

	Mtx zRot;
	makeRootMtxRotZ(zRot);
	zRot[0][3] = 0.0f;
	zRot[1][3] = 0.0f;
	zRot[2][3] = 0.0f;

	if (mRotation.y != 0.0f) {
		Mtx yRot;
		makeRootMtxRotY(yRot);
		yRot[0][3] = 0.0f;
		yRot[1][3] = 0.0f;
		yRot[2][3] = 0.0f;
		PSMTXConcat(yRot, zRot, zRot);
	}

	zRot[0][3] = mPosition.x;
	zRot[1][3] = mPosition.y - mYOffset;
	zRot[2][3] = mPosition.z;
	PSMTXCopy(zRot, getModel()->mNodeMatrices[0]);
}

u32 TBellWatermill::touchWater(THitActor* water)
{
	unk190 = 1;
	if (fabsf(unk158) > unk16C)
		unk178 += unk180;

	unk158 += unk15C;
	if (unk158 > unk164)
		unk158 = unk164;

	return 1;
}

TBiancoBell::TBiancoBell(const char* name)
    : TMapObjBase(name)
    , unk138(0)
    , unk13A(0)
{
}

void TBiancoBell::initMapObj()
{
	TMapObjBase::initMapObj();

	if (strcmp(mName, "BiaBell 0") == 0) {
		unk138 = 1;
		unk13A = 0;
	} else if (strcmp(mName, "BiaBell 1") == 0) {
		unk138 = 2;
		unk13A = 1;
	} else {
		unk138 = 3;
		unk13A = 0;
	}
}

void TBiancoBell::touchPlayer(THitActor*)
{
	if (mMActor->getFrameCtrl(0)->getFrame() == 0.0f
	    || mMActor->getFrameCtrl(0)->getFrame()
	            + mMActor->getFrameCtrl(0)->getRate()
	        >= (f32)mMActor->getFrameCtrl(0)->getEnd() - 1.0f) {
		startAnim(4);
		mMActor->getFrameCtrl(0)->setRate(SMSGetAnmFrameRate());
		if (gpMSound->gateCheck(0x89B8)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x89B8, (const Vec*)&mPosition, 0, nullptr, 0, 4);
		}
	}
}

u32 TBiancoBell::touchWater(THitActor* water)
{
	if (mMActor->getFrameCtrl(0)->getFrame() == 0.0f
	    || mMActor->getFrameCtrl(0)->getFrame()
	            + mMActor->getFrameCtrl(0)->getRate()
	        >= (f32)mMActor->getFrameCtrl(0)->getEnd() - 1.0f) {
		startAnim(4);
		mMActor->getFrameCtrl(0)->setRate(SMSGetAnmFrameRate());
		if (gpMSound->gateCheck(0x89B8)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x89B8, (const Vec*)&mPosition, 0, nullptr, 0, 4);
		}
	}
	return 1;
}

TLampSeesawMain::TLampSeesawMain(const char* name)
    : TLampSeesaw(name)
    , unk144(0.0f)
    , unk148(0.998f)
    , unk14C(0.8f)
    , unk150(0.5f)
{
}

void TLampSeesawMain::loadAfter()
{
	u32 len = strlen("ランプシーソーＡ");
	u8 c0   = mName[len];
	u8 c1   = mName[len + 1];
	u8 c2   = mName[len + 2];
	u8 c3   = mName[len + 3];
	char name[0x40];

	snprintf(name, 0x40, "ランプシーソーＢ００");
	name[len]     = c0;
	name[len + 1] = c1;
	name[len + 2] = c2;
	name[len + 3] = c3;

	unk138 = (TLampSeesaw*)findMapObj(name);
	unk138->unk138 = this;
}

void TLampSeesawMain::control()
{
	TMapObjBase::control();
	switch (mState) {
	case 2: {
		f32 nextY = mPosition.y + unk144;
		if (nextY < unk13C || unk138->mPosition.y - unk144 < unk138->unk13C) {
			if (fabsf(unk144) < unk150)
				unk144 = 0.0f;
			else
				unk144 *= -unk14C;
		} else {
			mPosition.y       = nextY;
			unk138->mPosition.y -= unk144;
		}

		unk144 *= unk148;
		if (fabsf(unk144) < unk150)
			mState = 3;
		break;
	}
	case 3: {
		if (unk144 < 0.0f)
			unk144 = -unk150;
		else
			unk144 = unk150;

		f32 nextY = mPosition.y + unk144;
		if (nextY < unk13C || unk138->mPosition.y - unk144 < unk138->unk13C) {
			if (fabsf(unk144) < unk150)
				unk144 = 0.0f;
			else
				unk144 *= -unk14C;
		} else {
			mPosition.y       = nextY;
			unk138->mPosition.y -= unk144;
		}

		unk144 *= unk148;
		if (fabsf(unk144) <= unk150
		    && fabsf(mInitialPosition.y - mPosition.y) < unk150) {
			unk144 = 0.0f;
			mState = 1;
		}
		break;
	}
	}
}

void TLampSeesawMain::touchPlayer(THitActor*)
{
	if (marioIsOn())
		pushDown(unk140);
}

void TLampSeesawMain::pushDown(f32 power)
{
	mState = 2;
	unk144 -= power;
}

TLampSeesaw::TLampSeesaw(const char* name)
    : TMapObjBase(name)
    , unk138(nullptr)
    , unk140(0.01f)
{
}

void TLampSeesaw::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	f32 y;
	stream.read(&y, 4);
	unk13C = mInitialPosition.y - y;
	stream.read(&unk140, 4);
	unk140 *= 0.0001f;
}

void TLampSeesaw::touchPlayer(THitActor*)
{
	if (marioIsOn())
		unk138->pushDown(-unk140);
}

void TLampSeesaw::pushDown(f32) { }

TLeafBoatRotten::TLeafBoatRotten(const char* name)
    : TLeafBoat(name)
    , unk170(0)
    , unk174(0.0f)
    , unk178(255)
    , unk17A(255)
    , unk17C(255)
    , unk17E(255)
{
}

void TLeafBoatRotten::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	stream.read(&unk170, 4);
	unk170 *= 10;
	SMS_InitPacket_OneTevColor(getModel(), 0, GX_TEVREG0,
	                           (const GXColorS10*)&unk178);
}

void TLeafBoatRotten::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TMapObjBase::perform(flags, graphics);
}

void TLeafBoatRotten::control()
{
	TLeafBoat::control();

	if (marioIsOn() && isState(1)) {
		mLifeTimer = unk170;
		mState     = 2;
	}

	if (isState(2)) {
		f32 ratio = (f32)mLifeTimer / (f32)unk170;
		unk178    = (s16)((f32)TLeafBoatRotten::mRottenColor[0] * ratio
		               + (255.0f - 255.0f * ratio));
		unk17A    = (s16)((f32)TLeafBoatRotten::mRottenColor[1] * ratio
		               + (255.0f - 255.0f * ratio));
		unk17C    = (s16)((f32)TLeafBoatRotten::mRottenColor[2] * ratio
		               + (255.0f - 255.0f * ratio));
		if (mLifeTimer <= 0) {
			unk174 = 255.0f;
			mState = 3;
		}
	} else if (isState(3)) {
		unk174 -= mAlphaDownSpeed;
		unk17E = (s16)unk174;
		if (unk174 < mCollisionRemoveAlpha)
			removeMapCollision();
		if (unk174 <= 0.0f) {
			mScaling.set(360.0f, 360.0f, 360.0f);
			makeObjDead();
			makeObjDefault();
			unk178 = 255;
			unk17A = 255;
			unk17C = 255;
			unk17E = 255;
			mState = 1;
		}
	}
}

TLeafBoat::TLeafBoat(const char* name)
    : TMapObjBase(name)
    , unk138(0.0f)
    , unk13C(0.0f)
    , unk140(0.0f)
    , unk144(90.0f)
    , unk148(0.0f)
    , unk14C(300.0f)
    , unk150(90.0f)
    , unk154(500.0f)
    , unk158(550.0f)
    , unk15C(0.3f)
    , unk160(0)
    , unk164(0.0f)
    , unk168(0.0f)
    , unk16C(0.0f)
{
}

void TLeafBoat::initMapObj()
{
	TMapObjBase::initMapObj();
	mState     = 1;
	mLifeTimer = 0;
}

void TLeafBoat::calc()
{
	TMapObjBase::calc();
	mRotation.x = unk138;
	mRotation.z = unk13C;
}

void TLeafBoat::control()
{
	TMapObjBase::control();
	unk138 *= 0.998f;
	unk13C *= 0.998f;
	mPosition.x += mVelocity.x;
	mPosition.y += mVelocity.y;
	mPosition.z += mVelocity.z;
	mVelocity.scale(0.8f);
}

void TLeafBoat::bind()
{
	TMapObjBase::bind();
	if (mPosition.y < unk14C) {
		mPosition.y = unk14C;
		if (mVelocity.y < 0.0f)
			mVelocity.y = 0.0f;
	}
}

void TLeafBoat::touchWall(JGeometry::TVec3<f32>* normal,
                          TBGWallCheckRecord*)
{
	if (normal != nullptr) {
		mVelocity.x += normal->x * 5.0f;
		mVelocity.z += normal->z * 5.0f;
	}
}

void TLeafBoat::touchActor(THitActor* actor)
{
	if (TMapObjBase::isFruit(actor))
		sendMsgToAll(0xE);
}

TBiancoMiniWindmill::TBiancoMiniWindmill(const char* name)
    : THideObjBase(name)
    , unk150((f32)rand() * (1.0f / 32768.0f) * 300.0f)
    , unk154(0.0f)
    , unk158(1.0f + (f32)rand() * (1.0f / 32768.0f))
    , unk15C(nullptr)
    , unk160(nullptr)
{
}

void TBiancoMiniWindmill::initMapObj()
{
	TMapObjBase::initMapObj();
	unk13C = 0.0f;

	unk15C = new TMapObjMessenger(
	    "\x92\x6E\x8C\x60\x83\x49\x83\x75\x83\x57\x83\x46\x83\x81\x83\x62\x83\x5A\x83\x93\x83\x57\x83\x83\x81\x5B");
	unk15C->initHitActor(0, 1, 0, 0.0f, 0.0f, 300.0f, 500.0f);

	unk15C->mPosition.x = mPosition.x + sMessengerPosZ * JMASin(mRotation.y);
	unk15C->mPosition.y = mPosition.y + sMessengerPosY;
	unk15C->mPosition.z = mPosition.z + sMessengerPosZ * JMACos(mRotation.y);
}

void TBiancoMiniWindmill::control()
{
	if (unk154 > unk158)
		unk154 -= mFriction;
	else
		unk154 = unk158;

	unk150 += unk154;

	while (unk150 >= 360.0f)
		unk150 -= 360.0f;
	while (unk150 < 0.0f)
		unk150 += 360.0f;
}

void TBiancoMiniWindmill::calc()
{
	Mtx rotMtx;
	rotMtx[0][0] = JMACos(unk150);
	rotMtx[0][1] = -JMASin(unk150);
	rotMtx[0][2] = 0.0f;
	rotMtx[0][3] = 0.0f;
	rotMtx[1][0] = JMASin(unk150);
	rotMtx[1][1] = JMACos(unk150);
	rotMtx[1][2] = 0.0f;
	rotMtx[1][3] = 0.0f;
	rotMtx[2][0] = 0.0f;
	rotMtx[2][1] = 0.0f;
	rotMtx[2][2] = 1.0f;
	rotMtx[2][3] = 0.0f;

	J3DModel* model = getModel();
	PSMTXConcat(model->mNodeMatrices[0], rotMtx, rotMtx);

	MtxPtr jointMtx = getModel()->mNodeMatrices[1];
	rotMtx[0][3]   = jointMtx[0][3];
	rotMtx[1][3]   = jointMtx[1][3];
	rotMtx[2][3]   = jointMtx[2][3];
	PSMTXCopy(rotMtx, getModel()->mNodeMatrices[1]);

	f32 volume = fabsf(unk154);
	if (gpMSound->gateCheck(0x3045)) {
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x3045, (const Vec*)&mPosition, nullptr, volume, 0, 0, &unk160,
		    0, 4);
	}
}

u32 TBiancoMiniWindmill::touchWater(THitActor* water)
{
	JGeometry::TVec3<f32>* waterPos
	    = ((TMapObjBase*)water)->getWaterPos(water);
	if (waterPos->y < mPosition.y + sMessengerPosY - 300.0f)
		return 1;

	JGeometry::TVec3<f32>* waterSpeed
	    = ((TMapObjBase*)water)->getWaterSpeed(water);
	MtxPtr mtx = getModel()->mNodeMatrices[0];
	f32 dot    = waterSpeed->x * mtx[0][2] + waterSpeed->y * mtx[1][2]
	          + waterSpeed->z * mtx[2][2];
	if (dot > 0.0f)
		return 0;

	unk154 += mRotWaterAccel;
	if (unk154 > mRotSpeedMax) {
		unk154 = mRotSpeedMax;
		JGeometry::TVec3<f32> pos(mPosition.x, unk15C->mPosition.y + 550.0f,
		                          mPosition.z);
		unk13C = 0.0f;
		appearObjFromPoint(pos);
	}
	return 1;
}

TBiancoWatermillVertical::TBiancoWatermillVertical(const char* name)
    : TMapObjBase(name)
    , unk138(0.0f)
    , unk13C(0.0f)
    , unk140(nullptr)
    , unk144(0)
    , unk148(nullptr)
    , unk14C(nullptr)
{
}

void TBiancoWatermillVertical::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	stream.read(&unk13C, 4);
	unk13C /= 1000.0f;
	unk138 = unk13C;
}

void TBiancoWatermillVertical::loadAfter()
{
	TMapObjBase::loadAfter();
	if (strcmp(mName, "BiaWatermillVertical 0") == 0)
		unk140 = findMapObj("BiaTurnBridge 0");
	else
		unk140 = findMapObj("BiaTurnBridge 1");
	mBodyRadius = 1000.0f;
}

void TBiancoWatermillVertical::control()
{
	if (unk138 != unk13C) {
		if (unk138 > unk13C) {
			unk138 -= mRotSpeedDownRate;
			if (unk138 < unk13C)
				unk138 = unk13C;
		} else {
			unk138 += mRotSpeedDownRate;
			if (unk138 > unk13C)
				unk138 = unk13C;
		}
	}

	mRotation.y += unk138;
	f32 rot = mRotation.y;
	while (rot >= 360.0f)
		rot -= 360.0f;
	while (rot < 0.0f)
		rot += 360.0f;
	mRotation.y = rot;

	f32 bridgeRot = unk138 * mBridgeRotRate;
	unk140->mRotation.y += bridgeRot;
	f32 bridgeObjRot = unk140->mRotation.y;
	while (bridgeObjRot >= 360.0f)
		bridgeObjRot -= 360.0f;
	while (bridgeObjRot < 0.0f)
		bridgeObjRot += 360.0f;
	unk140->mRotation.y = bridgeObjRot;

	f32 volume = fabsf(unk138);
	if (gpMSound->gateCheck(0x3040)) {
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x3040, (const Vec*)&mPosition, nullptr, volume, 0, 0, &unk148,
		    0, 4);
	}

	const Vec* bridgePosition = (const Vec*)&unk140->mPosition;
	f32 bridgeVolume = fabsf(bridgeRot);
	if (gpMSound->gateCheck(0x3042)) {
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x3042, bridgePosition, nullptr, bridgeVolume, 0, 0, &unk14C,
		    0, 4);
	}
}

void TBiancoWatermillVertical::setGroundCollision()
{
	if (unk144 != 0 || mColCount != 0) {
		J3DModel* model              = getModel();
		TMapCollisionManager* colman = mMapCollisionManager;
		MtxPtr mtx                   = model->getAnmMtx(0);
		TMapCollisionBase* base      = colman->getUnk8();
		if (base)
			base->moveMtx(mtx);
		unk144 = 0;
	}
}

u32 TBiancoWatermillVertical::touchWater(THitActor* water)
{
	if (((TMapObjBase*)water)->getWaterPlane(water) == nullptr) {
		unk144 = 1;
		return 0;
	}

	if (!((TMapObjBase*)water)->waterHitPlane(water))
		return 0;

	JGeometry::TVec3<f32>* waterPos
	    = ((TMapObjBase*)water)->getWaterPos(water);
	JGeometry::TVec3<f32>* waterSpeed
	    = ((TMapObjBase*)water)->getWaterSpeed(water);

	JGeometry::TVec3<f32> speed(waterSpeed->x, 0.0f, waterSpeed->z);
	if (speed.x != 0.0f || speed.z != 0.0f)
		MsVECNormalize((Vec*)&speed, (Vec*)&speed);

	JGeometry::TVec3<f32> target;
	getVerticalVecToTargetXZ(waterPos->x, waterPos->z, &target);
	MsVECNormalize((Vec*)&target, (Vec*)&target);

	f32 rate = (mBodyRadius - getDistanceXZ(*waterPos)) / mBodyRadius;
	f32 dot  = speed.x * target.x + speed.y * target.y + speed.z * target.z;
	if (dot > 0.0f) {
		if (unk138 < mRotSpeedMax)
			unk138 += mRotAccel * rate;
	} else {
		if (unk138 > -mRotSpeedMax)
			unk138 -= mRotAccel * rate;
	}
	return 1;
}

TBiancoWatermill::TBiancoWatermill(const char* name)
    : TMapObjBase(name)
    , unk138(10000.0f)
    , unk13C(nullptr)
{
}

void TBiancoWatermill::initMapObj()
{
	TMapObjBase::initMapObj();
	if (strcmp(unkF4, "BiaWatermill01") == 0) {
		mBodyRadius = 1200.0f;
	} else if (strcmp(unkF4, "BiaWatermill00") == 0) {
		mBodyRadius = 1200.0f;
	}
}

void TBiancoWatermill::control()
{
	mRotation.z -= unk138;
	f32 volume = fabsf(unk138);
	if (gpMSound->gateCheck(0x3043)) {
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x3043, (const Vec*)&mPosition, nullptr, volume, 0, 0, &unk13C,
		    0, 4);
	}
}

u32 TBiancoWatermill::touchWater(THitActor*) { return 0; }

void TBiancoWatermill::turnByEnemy(THitActor*, const TBGCheckData*)
{
}

void TMapObjRootPakkun::initMapObj()
{
	TMapObjBase::initMapObj();
	unk138 = new TTrembleModelEffect;
	unk138->init(mMActor->unk4);
	unk138->tremble(100.0f, 1.0f, 1.0f, 12000);
}

void TMapObjRootPakkun::drawObject(JDrama::TGraphics* graphics)
{
	TLiveActor::drawObject(graphics);

	if (fabsf(gpMarioPos->z - mPosition.z) < 500.0f) {
		unk138->movement();
		BOOL active;
		if (mLifeTimer > 0)
			active = TRUE;
		else
			active = FALSE;
		if (!active) {
			unk138->tremble(mTremblePower, mTrembleAccel, mTrembleBrake,
			                mTrembleTime);
			mLifeTimer = mTrembleTime;
		}
	}
}

void TBigWindmill::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);

	for (int i = 0; i < 4; ++i) {
		JGeometry::TVec3<f32> pos(0.0f, 0.0f, 0.0f);
		JGeometry::TVec3<f32> rot(0.0f, 0.0f, 0.0f);
		JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
		unk138[i] = TMapObjBaseManager::newAndRegisterObj("bigWindmillBlock",
		                                                   pos, rot, scale);
		unk138[i]->appear();
		unk138[i]->getModel()->calc();
	}
}

void TBigWindmill::control()
{
	TMapObjBase::control();
	f32 rotZ = mRotation.z - sSpeed;
	while (rotZ >= 360.0f)
		rotZ -= 360.0f;
	while (rotZ < 0.0f)
		rotZ += 360.0f;
	mRotation.z = rotZ;
	setRootMtxRotZ();

	f32 volume = fabsf(sSpeed);
	if (gpMSound->gateCheck(0x3047)) {
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x3047, (const Vec*)&mPosition, nullptr, volume, 0, 0, &unk148, 0,
		    4);
	}

	f32 angle = mRotation.z + sAngleAdd;
	for (int i = 0; i < 4; ++i) {
		TMapObjBase* block = unk138[i];
		J3DModel* model   = block->getModel();
		MtxPtr mtx        = model->mNodeMatrices[0];
		f32 rad           = angle * 0.017453294f;

		mtx[0][3] = mPosition.x + sRadius * cosf(rad);
		mtx[1][3] = mPosition.y + sRadius * sinf(rad) - mYOffset;
		mtx[2][3] = mPosition.z - sSubZ;

		block->mPosition.x = mtx[0][3];
		block->mPosition.y = mtx[1][3];
		block->mPosition.z = mtx[2][3];

		angle += 90.0f;
		if (angle > 360.0f)
			angle -= 360.0f;
	}
}
