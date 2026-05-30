#include <MoveBG/MapObjBianco.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionData.hpp>
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
#include <System/EmitterViewObj.hpp>
#include <System/Particles.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>
#include <JSystem/JMath.hpp>
#include <dolphin/mtx.h>
#include <math.h>
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

	unk194 = findMapObj("BiaBell 0");
	unk198 = findMapObj("BiaBell 1");
	unk19C = findMapObj("BiaBell 2");
	unk190 = 1;
}

void TBellWatermill::control()
{
	TMapObjTurn::control();

	if (unk190 != 0) {
		unk178 += unk16C;
		if (unk178 > unk17C)
			unk178 -= unk17C;
		mRotation.y = unk178;
	}

	if (unk194 != nullptr)
		unk194->mRotation.y = mRotation.y;
	if (unk198 != nullptr)
		unk198->mRotation.y = mRotation.y;
	if (unk19C != nullptr)
		unk19C->mRotation.y = mRotation.y;

	if (gpMSound->gateCheck(0x3066)) {
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x3066, (const Vec*)&mPosition, nullptr, fabsf(unk16C), 0, 0,
		    nullptr, 0, 4);
	}
}

u32 TBellWatermill::touchWater(THitActor* water)
{
	if (waterHitPlane(water)) {
		unk16C += unk180;
		if (unk16C > unk174)
			unk16C = unk174;
	}
	return 1;
}

TBiancoBell::TBiancoBell(const char* name)
    : TMapObjBase(name)
{
}

void TBiancoBell::initMapObj() { TMapObjBase::initMapObj(); }

void TBiancoBell::touchPlayer(THitActor*)
{
	if (marioIsOn())
		startAnim(1);
}

u32 TBiancoBell::touchWater(THitActor* water)
{
	if (waterHitPlane(water))
		startAnim(1);
	return 1;
}

TLampSeesawMain::TLampSeesawMain(const char* name)
    : TLampSeesaw(name)
    , unk144(0.0f)
    , unk148(50.0f)
    , unk14C(-1.0f)
    , unk150(1.0f)
{
}

void TLampSeesawMain::loadAfter()
{
	TMapObjBase::loadAfter();
	unk138 = (TLampSeesaw*)findMapObj("ランプシーソー（従）");
	if (unk138 != nullptr)
		unk138->unk138 = this;
}

void TLampSeesawMain::control()
{
	TMapObjBase::control();
	if (unk144 > 0.0f) {
		unk144 -= unk148;
		if (unk144 < 0.0f)
			unk144 = 0.0f;
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
    , unk13C(0.0f)
    , unk140(50.0f)
{
}

void TLampSeesaw::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	f32 y;
	stream.read(&y, 4);
	unk13C = mInitialPosition.y - y;
	stream.read(&unk140, 4);
	unk140 *= 0.5f;
}

void TLampSeesaw::touchPlayer(THitActor*)
{
	if (marioIsOn() && unk138 != nullptr)
		unk138->pushDown(1.0f);
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
    , unk158(360.0f + (f32)rand() * (1.0f / 32768.0f))
    , unk15C(0)
    , unk160(0)
{
}

void TBiancoMiniWindmill::initMapObj()
{
	THideObjBase::initMapObj();
	unk15C = 0;
	unk160 = 0;
}

void TBiancoMiniWindmill::control()
{
	THideObjBase::control();
	unk150 += unk154;
	unk154 *= mFriction;
	if (unk154 > mRotSpeedMax)
		unk154 = mRotSpeedMax;
	if (unk154 < -mRotSpeedMax)
		unk154 = -mRotSpeedMax;
	mRotation.y = unk150;
}

void TBiancoMiniWindmill::calc()
{
	THideObjBase::calc();
	sMessengerPosZ = 200.0f + JMASin(mRotation.y) * 300.0f;
	sMessengerPosY = 6400.0f + JMACos(mRotation.y) * 300.0f;
}

u32 TBiancoMiniWindmill::touchWater(THitActor* water)
{
	if (waterHitPlane(water))
		unk154 += mRotWaterAccel;
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
	unk140 = findMapObj("BiaWatermillVertical 0");
	unk148 = findMapObj("BiaTurnBridge 0");
	unk14C = findMapObj("BiaTurnBridge 1");
}

void TBiancoWatermillVertical::control()
{
	TMapObjBase::control();

	unk138 *= mRotSpeedDownRate;
	if (unk138 > mRotSpeedMax)
		unk138 = mRotSpeedMax;
	if (unk138 < -mRotSpeedMax)
		unk138 = -mRotSpeedMax;

	mRotation.x += unk138;
	if (unk148 != nullptr)
		unk148->mRotation.y += unk138 * mBridgeRotRate;
	if (unk14C != nullptr)
		unk14C->mRotation.y -= unk138 * mBridgeRotRate;

	if (gpMSound->gateCheck(0x3067)) {
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x3067, (const Vec*)&mPosition, nullptr, fabsf(unk138), 0, 0,
		    nullptr, 0, 4);
	}
}

void TBiancoWatermillVertical::setGroundCollision()
{
	TMapObjBase::setGroundCollision();
	if (unk140 != nullptr)
		unk140->setGroundCollision();
}

u32 TBiancoWatermillVertical::touchWater(THitActor* water)
{
	if (waterHitPlane(water))
		unk138 += mRotAccel;
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
