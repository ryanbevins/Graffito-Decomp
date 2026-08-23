#define JG_TUTIL_SQRT_OUT_OF_LINE
#include <MoveBG/MapObjMonte.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Map/MapCollisionManager.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/Watergun.hpp>
#include <Player/Yoshi.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/JAudio/JAInterface/JAISound.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <math.h>
#include <stdlib.h>

f32 THangingBridgeBoard::mMarioAccelY       = 0.15f;
f32 THangingBridgeBoard::mMarioHipDropAccelY = 2.0f;
f32 THangingBridgeBoard::mReturnAccelRate   = 0.005f;
f32 THangingBridgeBoard::mSpeedDownRate     = 0.98f;
f32 THangingBridgeBoard::mRopeWidthX        = 10.0f;
f32 THangingBridgeBoard::mRopeWidthZ        = 7.0f;
f32 THangingBridgeBoard::mTexPosRate        = 0.01f;

f32 THangingBridge::mRopeWidthBetweenBoards  = 10.0f;
f32 THangingBridge::mRopeWidthBetweenBoardsY = 10.0f;
s32 THangingBridge::mPointNumBetweenBoards   = 10;
f32 THangingBridge::mBetweenBoardsTexPosRate = 0.01f;

f32 TSwingBoard::mBoardWidth      = 315.0f;
f32 TSwingBoard::mRopeWidthX      = 10.0f;
f32 TSwingBoard::mRopeWidthZ      = 7.0f;
f32 TSwingBoard::mTexPosRate      = 0.01f;
f32 TSwingBoard::mReturnAccelRate = 0.0001f;
f32 TSwingBoard::mSpeedDownRate   = 0.998f;

f32 TFluff::mScaleUpSpeed   = 0.05f;
f32 TFluff::mScaleDownSpeed = 0.01f;
f32 TFluffManager::mWindMin = 1.0f;

f32 THangingBridge::mRopeHeight;

static inline f32 randUnit() { return (f32)rand() * (1.0f / 32768.0f); }

static inline f32 randSigned() { return 2.0f * randUnit() - 1.0f; }

static inline void zeroVec(JGeometry::TVec3<f32>& v)
{
	v.x = 0.0f;
	v.y = 0.0f;
	v.z = 0.0f;
}

static inline void oneVec(JGeometry::TVec3<f32>& v)
{
	v.x = 1.0f;
	v.y = 1.0f;
	v.z = 1.0f;
}

static inline void copyTransToVec(JGeometry::TVec3<f32>& dst, MtxPtr mtx)
{
	dst.x = mtx[0][3];
	dst.y = mtx[1][3];
	dst.z = mtx[2][3];
}

static inline void drawRopeQuad(const JGeometry::TVec3<f32>& a,
                                const JGeometry::TVec3<f32>& b, f32 widthX,
                                f32 widthY, f32 texRate)
{
	f32 tx = fabsf(b.x - a.x) * texRate + fabsf(b.y - a.y) * texRate
	         + fabsf(b.z - a.z) * texRate;

	GXBegin(GX_QUADS, GX_VTXFMT0, 4);
	GXPosition3f32(a.x - widthX, a.y, a.z - widthY);
	GXTexCoord2f32(0.0f, 0.0f);
	GXPosition3f32(a.x + widthX, a.y, a.z + widthY);
	GXTexCoord2f32(0.0f, 1.0f);
	GXPosition3f32(b.x + widthX, b.y, b.z + widthY);
	GXTexCoord2f32(tx, 1.0f);
	GXPosition3f32(b.x - widthX, b.y, b.z - widthY);
	GXTexCoord2f32(tx, 0.0f);
	GXEnd();
}

struct TBridgeBoardOverride {
	f32 x;
	f32 y;
	f32 z;
	f32 rotX;
};

TFluffManager::TFluffManager(const char* name)
    : TMapObjBase(name)
{
	zeroVec(unk138);
	unk144 = 0;
	unk154 = 0.0f;
	unk158 = 0;
	unk15C = 0;
	unk160 = 0;
	unk164 = 0;
	zeroVec(unk148);
}

void TFluffManager::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	stream.read(&unk138.z, 4);
	f32 windScale;
	stream.read(&windScale, 4);
	windScale *= 0.01f;
	stream.read(&unk144, 4);

	unk138.x = 5000.0f;
	unk138.y = 5000.0f;
	unk154   = 0.998f;

	TPosition3f mtx;
	MsMtxSetXYZRPH(mtx, 0.0f, 0.0f, 0.0f, mRotation.x, mRotation.y,
	               mRotation.z);
	unk148.set(0.0f, 0.0f, 1.0f);
	mtx.mult(unk148, unk148);
	unk148.scale(windScale);
}

void TFluffManager::loadAfter()
{
	unk160 = 0;
	unk164 = 32;
	unk168 = new TFluff*[unk164];

	TFluff* first = new TFluff("１つ目のわた毛");
	first->initAndRegister("Fluff");
	first->unk168 = this;
	unk158        = first;
	unk158->unk16C = 1;
	unk158->appear();
	unk158->mPosition.set(mPosition);
	unk158->mRotation.set(mRotation);
	JGeometry::TVec3<f32> firstPos(randSigned() * unk138.x,
	                               randUnit() * mPosition.y,
	                               randSigned() * unk138.y);
	unk158->mInitialPosition = firstPos;
	unk168[unk160]          = unk158;
	unk160 += 1;

	TFluff* second = new TFluff("２つ目のわた毛");
	second->initAndRegister("Fluff");
	second->unk168 = this;
	unk15C         = second;
	unk15C->mPosition.set(mPosition);
	unk15C->mRotation.set(mRotation);
	JGeometry::TVec3<f32> secondPos(randSigned() * unk138.x,
	                                randUnit() * mPosition.y,
	                                randSigned() * unk138.y);
	unk15C->mInitialPosition = secondPos;
	unk15C->makeObjDead();
	unk168[unk160] = unk15C;
	unk160 += 1;

	for (int i = 2; i < unk164; ++i) {
		TFluff* fluff = new TFluff("わた毛");
		fluff->initAndRegister("Fluff");
		fluff->unk168 = this;
		unk168[unk160] = fluff;
		unk168[unk160]->appear();
		unk160 += 1;
	}
}

void TFluffManager::control()
{
	switch (mState) {
	case 1:
		if (unk15C == 0
		    && unk158->mPosition.y - 100.0f < mPosition.y - unk138.z) {
			for (int i = 3; i < unk164; ++i) {
				if (unk168[i]->unk16C != 0 || unk168[i]->mHeldObject != 0)
					continue;

				f32 dx   = unk168[i]->mPosition.x - gpMarioPos->x;
				f32 dy   = unk168[i]->mPosition.y - gpMarioPos->y;
				f32 dz   = unk168[i]->mPosition.z - gpMarioPos->z;
				f32 dist = JGeometry::TUtil<f32>::sqrt(dx * dx + dy * dy
				                                       + dz * dz);
				if (dist > 3000.0f) {
					unk15C = unk168[i];
					unk168[i]->kill();
					break;
				}
			}
		}

		if (unk158->mPosition.y < mPosition.y - unk138.z) {
			if (gpMSound->gateCheck(0x3884)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x3884, (Vec*)&unk158->mPosition, 0, 0, 0, 4);
			}
			mLifeTimer = unk144;
			mState     = 2;
		}
		break;

	case 2:
	{
		TMapObjManager* manager = gpMapObjManager;
		f32 windX              = manager->unkD0.x + unk148.x;
		f32 windY              = manager->unkD0.y + unk148.y;
		f32 windZ              = manager->unkD0.z + unk148.z;
		manager->unkD0.x       = windX;
		manager->unkD0.y       = windY;
		manager->unkD0.z       = windZ;
		bool timerActive = mLifeTimer > 0 ? true : false;
		if (timerActive)
			break;
		mState = 3;
		break;
	}

	case 3: {
		TMapObjManager* manager = gpMapObjManager;
		f32 windX              = manager->unkD0.x * unk154;
		f32 windY              = manager->unkD0.y * unk154;
		f32 windZ              = manager->unkD0.z * unk154;

		if (fabsf(windX) < mWindMin && fabsf(windY) < mWindMin
		    && fabsf(windZ) < mWindMin) {
			windX = 0.0f;
			windY = 0.0f;
			windZ = 0.0f;

			unk158 = unk15C;
			unk158->mRotation        = mRotation;
			unk158->mInitialRotation = mRotation;
			unk158->appear();
			unk158->mPosition        = mPosition;
			unk158->mInitialPosition = mPosition;
			zeroVec(unk158->mRotation);
			unk158->mInitialRotation = unk158->mRotation;
			unk158->unk148           = 0.0f;
			unk158->unk150           = 1.0f;
			unk158->unk16C           = 1;
			unk15C                   = 0;
			mState                   = 1;
		}

		manager->unkD0.x = windX;
		manager->unkD0.y = windY;
		manager->unkD0.z = windZ;
		break;
	}
	}
}

TFluff::TFluff(const char* name)
    : TMapObjBase(name)
    , unk138(0.0f)
    , unk13C(0.0f)
    , unk140(0.0f)
    , unk144(0.0f)
    , unk148(0.0f)
    , unk14C(0.0f)
    , unk150(0.0f)
    , unk160(1.0f)
    , unk164(0.95f)
    , unk168(0)
    , unk16C(0)
{
	unk154.zero();
}

void TFluff::initMapObj()
{
	TMapObjBase::initMapObj();
	unk138 = 4800.0f;
	unk13C = 0.5f;
}

void TFluff::appear()
{
	makeObjAppeared();

	TFluffManager* mgr = unk168;
	f32 posY;
	f32 posZ;
	posZ = randSigned() * mgr->unk138.y;
	posY = randUnit() * unk168->mPosition.y;
	f32 posX = randSigned() * mgr->unk138.x;
	mPosition.x = posX;
	mPosition.y = posY;
	mPosition.z = posZ;
	mInitialPosition = mPosition;

	mScaling.x = 0.0001f;
	mScaling.y = 0.0001f;
	mScaling.z = 0.0001f;
	unk154.z = 0.0f;
	unk154.y = 0.0f;
	unk154.x = 0.0f;
	unk148 = 0.0f;
	unk150 = 0.2f + 0.8f * randUnit();
	unk140 = sinf(3.14f * mRotation.y / 180.0f);
	unk144 = cosf(3.14f * mRotation.y / 180.0f);
	unk148 = randUnit() * 360.0f;
	unk14C = 0.3f;
	mState = 2;
}

void TFluff::control()
{
	TMapObjBase::control();
	move();

	switch (mState) {
	case 2:
		mScaling.x += mScaleUpSpeed;
		mScaling.y += mScaleUpSpeed;
		mScaling.z += mScaleUpSpeed;
		if (mScaling.x > 1.0f) {
			oneVec(mScaling);
			setObjHitData(0);
			mState = 1;
		}
		break;
	case 1:
		mGroundHeight = gpMap->checkGround(mPosition, &mGroundPlane);
		{
			JGeometry::TVec3<f32> velocity = mVelocity;
			if (velocity.y < 0.0f
			    && (mGroundHeight > mPosition.y - unk13C
			        || mPosition.y < -1000.0f))
				kill();
		}
		if (gpMap->isTouchedOneWall(mPosition.x, mPosition.y, mPosition.z,
		                            100.0f))
			kill();
		if (mPosition.x < -14848.0f || 14848.0f < mPosition.x
		    || mPosition.z < -19968.0f || 19968.0f < mPosition.z) {
			kill();
		}
		break;
	case 3:
		mScaling.x -= mScaleDownSpeed;
		mScaling.y -= mScaleDownSpeed;
		mScaling.z -= mScaleDownSpeed;
		if (mScaling.x < 0.1f) {
			gpMarioParticleManager->emitAndBindToPosPtr(0xE5, &mPosition, 0,
			                                            0);
			if (gpMSound->gateCheck(0x387D)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x387D, (Vec*)&mPosition, 0, 0, 0, 4);
			}
			mScaling.x = 0.0001f;
			mScaling.y = 0.0001f;
			mScaling.z = 0.0001f;
			mLifeTimer = 240;
			mState     = 4;
		}
		break;
	case 4:
		if (isLifeTimerActive())
			break;
		appear();
		mRotation.x      = 0.0f;
		mRotation.y      = randUnit() * 360.0f;
		mRotation.z      = 0.0f;
		mInitialRotation = mRotation;
		unk16C           = 0;
		TFluffManager* manager = unk168;
		if (manager->unk15C == 0) {
			manager->unk15C = this;
			manager->unk15C->makeObjDead();
		}
		break;
	}
}

void TFluff::kill()
{
	if (mHeldObject != 0) {
		mHeldObject->receiveMessage(this, 8);
		mHeldObject = 0;
	}
	mState = 3;
}

void TFluff::move()
{
	mPosition.y -= unk13C;
	if (mPosition.y < 0.0f)
		mPosition.y = 5000.0f;

	unk154.x += unk150 * gpMapObjManager->unkD0.x;
	unk154.z += unk150 * gpMapObjManager->unkD0.z;

	JGeometry::TVec3<f32> velocity = mVelocity;
	unk154.x += velocity.x;
	unk154.y += velocity.y;
	unk154.z += velocity.z;

	f32 speedDown = unk164;
	mVelocity.x *= speedDown;
	mVelocity.y *= speedDown;
	mVelocity.z *= speedDown;

	f32 sway = unk138 * sinf(3.14f * unk148 / 180.0f);
	f32 posX   = mInitialPosition.x + sway * (unk144 + unk140);
	mPosition.x = unk154.x + posX;
	mPosition.y += unk150 * gpMapObjManager->unkD0.y;
	f32 posZ   = mInitialPosition.z + sway * (unk140 - unk144);
	mPosition.z = unk154.z + posZ;

	Vec* wind = &gpMapObjManager->unkD0;
	f32 windMag = wind->x * wind->x + wind->y * wind->y + wind->z * wind->z;
	if (windMag <= 0.0000038146973f) {
		unk148 += unk14C;
		if (unk148 > 360.0f)
			unk148 -= 360.0f;
	}

	if (mHeldObject != 0 && mHeldObject->isActorType(0x80000001))
		gpMarioPos->y -= unk13C;
}

u32 TFluff::touchWater(THitActor* sender)
{
	JGeometry::TVec3<f32>* pos = ((TMapObjBase*)sender)->getWaterPos(sender);
	JGeometry::TVec3<f32> normal;
	getNormalVecFromTarget(pos->x, pos->y, pos->z, &normal);
	mVelocity.x -= normal.x * unk160;
	mVelocity.y -= normal.y * unk160;
	mVelocity.z -= normal.z * unk160;
	return 1;
}

f32 TFluff::getRadiusAtY(f32) const { return 0.0f; }

void TGoalFlag::initMapObj() { TMapObjBase::initMapObj(); }

void TGoalFlag::touchActor(THitActor* actor)
{
	if (actor->isActorType(0x80000001)) {
		if (!TFlagManager::smInstance->getBool(0x50005))
			TFlagManager::smInstance->setBool(true, 0x50005);
		actor->receiveMessage(this, 0xE);
		return;
	}

	if (actor->isActorType(0x08000002))
		actor->receiveMessage(this, 0xE);
}

f32 TGoalFlag::getRadiusAtY(f32) const { return 0.0f; }

TSwingBoard::TSwingBoard(const char* name)
    : TMapObjBase(name)
    , unk138(5000.0f)
    , unk13C(0.0f)
    , unk140(0.0f)
    , unk144(0.0f)
    , unk148(0.0f)
    , unk188(0)
{
	unk14C[1][0] = unk14C[2][0] = unk14C[0][1] = unk14C[2][1]
	    = unk14C[0][2] = unk14C[1][2] = unk14C[0][3] = unk14C[1][3]
	    = unk14C[2][3] = 0.0f;
	unk14C[0][0] = unk14C[1][1] = unk14C[2][2] = 1.0f;
	unk17C.zero();
}

void TSwingBoard::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	stream.read(&unk138, 4);
	if (unk138 == -1.0f)
		unk138 = 5000.0f;
	stream.read(&unk140, 4);
	if (unk140 > 10.0f || unk140 == 0.0f)
		unk140 = 0.003f;

	unk17C.set(mPosition.x, mPosition.y + unk138, mPosition.z);
	unk148 = 0.05f * ((1.0f + randUnit()) * 0.5f);
	unk13C = 20.0f * (randUnit() - 0.5f);
	if (unk13C > 0.0f)
		unk144 = unk148 * -randUnit();
	else
		unk144 = unk148 * randUnit();

	s16 angle = DEG2SHORTANGLE(mRotation.y);
	f32 sinY  = jmaSinTable[static_cast<u16>(angle) >> jmaSinShift];
	f32 cosY  = jmaCosTable[static_cast<u16>(angle) >> jmaSinShift];
	unk14C[0][0] = cosY;
	unk14C[0][1] = 0.0f;
	unk14C[0][2] = sinY;
	unk14C[0][3] = 0.0f;
	unk14C[1][0] = 0.0f;
	unk14C[1][1] = 1.0f;
	unk14C[1][2] = 0.0f;
	unk14C[1][3] = 0.0f;
	unk14C[2][0] = -sinY;
	unk14C[2][1] = 0.0f;
	unk14C[2][2] = cosY;
	unk14C[2][3] = 0.0f;
}

void TSwingBoard::control()
{
	TMapObjBase::control();

	if (marioIsOn()) {
		if (marioIsOn()) {
			TWaterGun* gun = (TWaterGun*)SMS_GetMarioWaterGun();
			if ((s32)gun->mIsEmitWater != 0) {
				gun           = (TWaterGun*)SMS_GetMarioWaterGun();
				MtxPtr emitMtx = gun->getEmitMtx(0);
				f32 emitX      = -emitMtx[0][0];
				f32 emitZ      = -emitMtx[2][0];
				MtxPtr mtx     = getModel()->mNodeMatrices[0];
				f32 emitY      = 0.0f;
				f32 push       = mtx[1][2] * emitY;
				push += mtx[0][2] * emitX;
				push += mtx[2][2] * emitZ;
				unk144 += unk140 * push;
			}
		}
	}

	unk13C += unk144;
	f32 oldSpeed = unk144;
	unk144 -= unk13C * mReturnAccelRate;
	if (fabsf(unk144) > unk148)
		unk144 *= mSpeedDownRate;

	if (oldSpeed * unk144 <= 0.0f) {
		if (unk188 != 0)
			unk188->stop(1);

		if (unk144 > 0.0f) {
			f32 volume = __fabsf(unk13C);
			if (gpMSound->gateCheck(0x3867)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x3867, (const Vec*)&mPosition, nullptr, volume, 0, 0,
				    &unk188, 0, 4);
			}
		} else {
			f32 volume = __fabsf(unk13C);
			if (gpMSound->gateCheck(0x3868)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x3868, (const Vec*)&mPosition, nullptr, volume, 0, 0,
				    &unk188, 0, 4);
			}
		}
	}

	mRotation.x = -unk13C;

	Mtx rot;
	f32 sinAngle = sinf(mRotation.x / 180.0f * 3.14f);
	f32 cosAngle = cosf(mRotation.x / 180.0f * 3.14f);
	rot[0][0]    = 1.0f;
	rot[0][1]    = 0.0f;
	rot[0][2]    = 0.0f;
	rot[0][3]    = 0.0f;
	rot[1][0]    = 0.0f;
	rot[1][1]    = cosAngle;
	rot[1][2]    = -sinAngle;
	rot[1][3]    = 0.0f;
	rot[2][0]    = 0.0f;
	rot[2][1]    = sinAngle;
	rot[2][2]    = cosAngle;
	rot[2][3]    = 0.0f;

	MtxPtr baseMtx = getModel()->mNodeMatrices[0];
	PSMTXConcat(unk14C, rot, baseMtx);

	cosf(unk13C / 180.0f * 3.14f);
	sinf(unk13C / 180.0f * 3.14f);

	mPosition.x = mInitialPosition.x - baseMtx[0][1] * unk138;
	mPosition.y = unk138 + mInitialPosition.y - baseMtx[1][1] * unk138;
	mPosition.z = mInitialPosition.z - baseMtx[2][1] * unk138;
	baseMtx[0][3] = mPosition.x;
	baseMtx[1][3] = mPosition.y;
	baseMtx[2][3] = mPosition.z;
}

void TSwingBoard::draw() const
{
	initDraw();

	MtxPtr mtx = getModel()->mNodeMatrices[0];
	const f32* initialZ = &mInitialPosition.z;
	JGeometry::TVec3<f32> upper;
	JGeometry::TVec3<f32> lower;

	lower.x = mInitialPosition.x + mBoardWidth * mtx[0][0];
	lower.y = unk138 + mInitialPosition.y;
	lower.z = *initialZ + mBoardWidth * mtx[2][0];
	upper.x = mPosition.x + mBoardWidth * mtx[0][0];
	upper.y = mPosition.y + 60.0f;
	upper.z = mPosition.z + mBoardWidth * mtx[2][0];
	drawOneRope(upper, lower);

	lower.x = mInitialPosition.x - mBoardWidth * mtx[0][0];
	lower.z = *initialZ - mBoardWidth * mtx[2][0];
	upper.x = mPosition.x - mBoardWidth * mtx[0][0];
	upper.z = mPosition.z - mBoardWidth * mtx[2][0];
	drawOneRope(upper, lower);
}

#pragma dont_inline on
void TSwingBoard::initDraw() const
{
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXLoadPosMtxImm(j3dSys.mViewMtx, GX_PNMTX0);
	GXSetCurrentMtx(GX_PNMTX0);
	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanMatColor(GX_COLOR0A0,
	                  (GXColor) { 0xff, 0xff, 0xff, 0xff });
	GXSetNumTexGens(1);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0,
	                  GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
	JUTTexture tex(gpMapObjManager->unkCC);
	tex.load(GX_TEXMAP0);
	GXSetNumTevStages(1);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO,
	                GX_CC_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
	GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GXSetCullMode(GX_CULL_BACK);
}

void TSwingBoard::drawOneRope(const JGeometry::TVec3<f32>& top,
                              const JGeometry::TVec3<f32>& bottom) const
{
	f32 texTop = unk138 * mTexPosRate;
	f32 texBot = 0.0f;
	f32 topXPlus = top.x + mRopeWidthX;
	f32 topXMinus = top.x - mRopeWidthX;
	f32 bottomXPlus = bottom.x + mRopeWidthX;
	f32 bottomXMinus = bottom.x - mRopeWidthX;
	f32 topZPlus = top.z + mRopeWidthZ;
	f32 topZMinus = top.z - mRopeWidthZ;
	f32 bottomZPlus = bottom.z + mRopeWidthZ;
	f32 bottomZMinus = bottom.z - mRopeWidthZ;

	GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 8);
	GXPosition3f32(top.x, top.y, topZPlus);
	GXTexCoord2f32(0.0f, texTop);
	GXPosition3f32(bottom.x, bottom.y, bottomZPlus);
	GXTexCoord2f32(0.0f, texBot);
	GXPosition3f32(topXPlus, top.y, topZMinus);
	GXTexCoord2f32(1.0f, texTop);
	GXPosition3f32(bottomXPlus, bottom.y, bottomZMinus);
	GXTexCoord2f32(1.0f, texBot);
	GXPosition3f32(topXMinus, top.y, topZMinus);
	GXTexCoord2f32(2.0f, texTop);
	GXPosition3f32(bottomXMinus, bottom.y, bottomZMinus);
	GXTexCoord2f32(2.0f, texBot);
	GXPosition3f32(top.x, top.y, topZPlus);
	GXTexCoord2f32(3.0f, texTop);
	GXPosition3f32(bottom.x, bottom.y, bottomZPlus);
	GXTexCoord2f32(3.0f, texBot);
}
#pragma dont_inline off

THangingBridge::THangingBridge(const char* name)
    : JDrama::TViewObj(name)
    , unk10(0)
    , unk14(0)
    , unk38(0)
{
	zeroVec(unk3C);
}

void THangingBridge::loadAfter()
{
	JDrama::TNameRef::loadAfter();

	if (gpMarDirector->mMap == 0x0D) {
		unk10       = 14;
		unk18.set(1550.0f, 2980.0f, -9410.0f);
		unk24.set(3570.0f, 2455.0f, -9410.0f);
		mRopeHeight = 200.0f;
		unk3C.y = 0.8f;
		unk3C.z = 0.5f;
		unk3C.x = 150.0f;
	} else if (gpMarDirector->mMap == 0x08) {
		unk10       = 19;
		unk18.set(0.0f, 0.0f, 11356.0f);
		unk24.set(0.0f, -750.0f, 17743.0f);
		mRopeHeight = 1000.0f;
		unk3C.y = 1.0f;
		unk3C.z = 0.5f;
		unk3C.x = 315.0f;
	}

	unk30.x = unk24.x - unk18.x;
	unk30.y = unk24.z - unk18.z;
	unk30.normalize();
	f32 cosRot = cosf(1.5707964f);
	f32 sinRot = sinf(1.5707964f);
	f32 dirX   = unk30.x;
	f32 dirZ   = unk30.y;
	unk30.x    = dirX * cosRot - dirZ * sinRot;
	unk30.y    = dirX * sinRot + dirZ * cosRot;

	unk14 = new THangingBridgeBoard*[unk10];
	for (int i = 0; i < unk10; ++i) {
		f32 rate = (f32)i / (f32)(unk10 - 1);
		f32 rotY = gpMarDirector->mMap == 0x0D ? 90.0f : 0.0f;
		sinf(3.14f * rate);
		JGeometry::TVec3<f32> pos(unk18.x + (unk24.x - unk18.x) * rate,
		                           unk18.y + (unk24.y - unk18.y) * rate,
		                           unk18.z + (unk24.z - unk18.z) * rate);
		JGeometry::TVec3<f32> rot(15.0f, rotY, 0.0f);
		if (gpMarDirector->mMap == 0x08) {
			JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
			unk14[i]
			    = (THangingBridgeBoard*)TMapObjBaseManager::newAndRegisterObj(
			        "HangingBridgeBoard", pos, rot, scale);
		} else {
			JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
			unk14[i]
			    = (THangingBridgeBoard*)TMapObjBaseManager::newAndRegisterObj(
			        "PinnaHangingBridgeBoard", pos, rot, scale);
		}
		unk14[i]->unk1BC = this;
		unk14[i]->appear();
	}

	if (gpMarDirector->mMap == 0x08) {
		const TBridgeBoardOverride cSirenaBoardOverrides[] = {
			{ 0.0f, -130.0f, 11965.0f, 30.0f },
			{ 0.0f, -275.0f, 12225.0f, 29.0f },
			{ 0.0f, -415.0f, 12490.0f, 28.0f },
			{ 0.0f, -540.0f, 12760.0f, 26.0f },
			{ 0.0f, -660.0f, 13035.0f, 24.0f },
			{ 0.0f, -770.0f, 13315.0f, 22.0f },
			{ 0.0f, -875.0f, 13595.0f, 20.0f },
			{ 0.0f, -960.0f, 13895.0f, 12.0f },
			{ 0.0f, -1020.0f, 14190.0f, 8.0f },
			{ 0.0f, -1060.0f, 14490.0f, 4.0f },
			{ 0.0f, -1090.0f, 14790.0f, 2.0f },
			{ 0.0f, -1090.0f, 15090.0f, 0.0f },
			{ 0.0f, -1080.0f, 15395.0f, -4.0f },
			{ 0.0f, -1040.0f, 15695.0f, -6.0f },
			{ 0.0f, -995.0f, 15990.0f, -8.0f },
			{ 0.0f, -945.0f, 16285.0f, -8.0f },
			{ 0.0f, -900.0f, 16580.0f, -8.0f },
			{ 0.0f, -855.0f, 16880.0f, -8.0f },
			{ 0.0f, -800.0f, 17175.0f, -10.0f },
			{ -1.0f, 0.0f, 0.0f, 0.0f },
			{ -99999.0f, 0.0f, 0.0f, 0.0f },
		};

		for (int i = 0; i < unk10; ++i) {
			const TBridgeBoardOverride& data = cSirenaBoardOverrides[i];
			if (data.x == -1.0f)
				break;

			THangingBridgeBoard* board = unk14[i];
			board->mInitialPosition.set(data.x, data.y, data.z);
			board->mPosition.set(board->mInitialPosition);
			board->mRotation.x = data.rotX;
			board->calcDefaultMtx();
		}
	}

	if (gpMarDirector->mMap == 0x0D) {
		unk18.set(1350.0f, 2980.0f, -9410.0f);
		unk24.set(3650.0f, 2455.0f, -9410.0f);
	}

	for (int i = 0; i < unk10; ++i) {
		THangingBridgeBoard* board = unk14[i];
		if (i > 0)
			board->unk194 = unk14[i - 1];
		if (i > 1)
			board->unk19C = unk14[i - 2];
		if (i < unk10 - 1)
			board->unk198 = unk14[i + 1];
		if (i < unk10 - 2)
			board->unk1A0 = unk14[i + 2];
	}

	unk38 = new f32[mPointNumBetweenBoards];
	f32 angle = 0.0f;
	f32 step  = 1.0f / (f32)mPointNumBetweenBoards;
	for (int i = 0; i < mPointNumBetweenBoards; ++i) {
		unk38[i] = 50.0f * sinf(3.14f * angle);
		angle += step;
	}
}

void THangingBridge::perform(unsigned long flags, JDrama::TGraphics*)
{
	if ((flags & 8) == 0)
		return;

	initDraw();
	for (int i = 0; i < unk10; ++i) {
		THangingBridgeBoard* board    = unk14[i];
		JGeometry::TVec3<f32> ropePos = board->unk1A4[0];
		board->drawOneRope(ropePos);
		ropePos = board->unk1A4[1];
		board->drawOneRope(ropePos);
	}
	drawRopeBetweenBoards(0.0f, mPointNumBetweenBoards);
	drawRopeBetweenBoards(mRopeHeight, 1);
}

#pragma dont_inline on
void THangingBridge::initDraw() const
{
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXLoadPosMtxImm(j3dSys.mViewMtx, GX_PNMTX0);
	GXSetCurrentMtx(GX_PNMTX0);
	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanMatColor(GX_COLOR0A0,
	                  (GXColor) { 0xff, 0xff, 0xff, 0xff });
	GXSetNumTexGens(1);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0,
	                  GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
	if (gpMarDirector->mMap == 0x0D) {
		JUTTexture tex(gpMapObjManager->unkCC);
		tex.load(GX_TEXMAP0);
	} else {
		JUTTexture tex(gpMapObjManager->unkCC);
		tex.load(GX_TEXMAP0);
	}
	GXSetNumTevStages(1);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO,
	                GX_CC_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
	GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GXSetCullMode(GX_CULL_BACK);
}

void THangingBridge::drawRopeBetweenBoards(f32 y, int pointNum) const
{
	JGeometry::TVec2<f32> tex(unk30.x * mRopeWidthBetweenBoards,
	                          unk30.y * mRopeWidthBetweenBoards);
	f32 offX       = unk30.x * unk3C.x;
	f32 offZ       = unk30.y * unk3C.x;
	u16 vertexNum  = (u16)((unk10 + 2) * pointNum * 2);
	JGeometry::TVec3<f32> prev;
	JGeometry::TVec3<f32> next;
	JGeometry::TVec3<f32> end;

	GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, vertexNum);
	prev.set(unk18.x + offX, unk18.y + y, unk18.z + offZ);
	for (int i = 0; i < unk10; ++i) {
		next = unk14[i]->unk1A4[0];
		next.y += y;
		drawLowerMinus(prev, next, tex, pointNum);
		prev = next;
	}
	end.set(unk24.x + offX, unk24.y + y, unk24.z + offZ);
	drawLowerMinus(prev, end, tex, pointNum);
	prev = end;
	drawLowerMinus(prev, end, tex, pointNum);

	GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, vertexNum);
	prev.set(unk18.x + offX, unk18.y + y, unk18.z + offZ);
	for (int i = 0; i < unk10; ++i) {
		next = unk14[i]->unk1A4[0];
		next.y += y;
		drawLowerPlus(prev, next, tex, pointNum);
		prev = next;
	}
	end.set(unk24.x + offX, unk24.y + y, unk24.z + offZ);
	drawLowerPlus(prev, end, tex, pointNum);
	prev = end;
	drawLowerPlus(prev, end, tex, pointNum);

	GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, vertexNum);
	prev.set(unk18.x + offX, unk18.y + y, unk18.z + offZ);
	for (int i = 0; i < unk10; ++i) {
		next = unk14[i]->unk1A4[0];
		next.y += y;
		drawUpper(prev, next, tex, pointNum);
		prev = next;
	}
	end.set(unk24.x + offX, unk24.y + y, unk24.z + offZ);
	drawUpper(prev, end, tex, pointNum);
	prev = end;
	drawUpper(prev, end, tex, pointNum);

	GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, vertexNum);
	prev.set(unk18.x - offX, unk18.y + y, unk18.z - offZ);
	for (int i = 0; i < unk10; ++i) {
		next = unk14[i]->unk1A4[1];
		next.y += y;
		drawLowerMinus(prev, next, tex, pointNum);
		prev = next;
	}
	end.set(unk24.x - offX, unk24.y + y, unk24.z - offZ);
	drawLowerMinus(prev, end, tex, pointNum);
	prev = end;
	drawLowerMinus(prev, end, tex, pointNum);

	GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, vertexNum);
	prev.set(unk18.x - offX, unk18.y + y, unk18.z - offZ);
	for (int i = 0; i < unk10; ++i) {
		next = unk14[i]->unk1A4[1];
		next.y += y;
		drawLowerPlus(prev, next, tex, pointNum);
		prev = next;
	}
	end.set(unk24.x - offX, unk24.y + y, unk24.z - offZ);
	drawLowerPlus(prev, end, tex, pointNum);
	prev = end;
	drawLowerPlus(prev, end, tex, pointNum);

	GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, vertexNum);
	prev.set(unk18.x - offX, unk18.y + y, unk18.z - offZ);
	for (int i = 0; i < unk10; ++i) {
		next = unk14[i]->unk1A4[1];
		next.y += y;
		drawUpper(prev, next, tex, pointNum);
		prev = next;
	}
	end.set(unk24.x - offX, unk24.y + y, unk24.z - offZ);
	drawUpper(prev, end, tex, pointNum);
	prev = end;
	drawUpper(prev, end, tex, pointNum);
}

void THangingBridge::drawUpper(const JGeometry::TVec3<f32>& a,
                               const JGeometry::TVec3<f32>& b,
                               const JGeometry::TVec2<f32>& tex,
                               int pointNum) const
{
	f32 x     = a.x;
	f32 y     = a.y;
	f32 z     = a.z;
	f32 inv   = 1.0f / (f32)pointNum;
	f32 stepX = (b.x - a.x) * inv;
	f32 stepY = (b.y - a.y) * inv;
	f32 stepZ = (b.z - a.z) * inv;

	for (int i = 0; i < pointNum; ++i) {
		f32 ropeY  = y - unk38[i];
		f32 texPos = mBetweenBoardsTexPosRate * (x + z);
		GXPosition3f32(x + tex.x, ropeY, z + tex.y);
		GXTexCoord2f32(0.0f, texPos);
		GXPosition3f32(x - tex.x, ropeY, z - tex.y);
		GXTexCoord2f32(1.0f, texPos);
		x += stepX;
		y += stepY;
		z += stepZ;
	}
}

void THangingBridge::drawLowerPlus(const JGeometry::TVec3<f32>& a,
                                   const JGeometry::TVec3<f32>& b,
                                   const JGeometry::TVec2<f32>& tex,
                                   int pointNum) const
{
	f32 x     = a.x;
	f32 y     = a.y;
	f32 z     = a.z;
	f32 inv   = 1.0f / (f32)pointNum;
	f32 stepX = (b.x - a.x) * inv;
	f32 stepY = (b.y - a.y) * inv;
	f32 stepZ = (b.z - a.z) * inv;

	for (int i = 0; i < pointNum; ++i) {
		f32 ropeY  = y - unk38[i];
		f32 texPos = mBetweenBoardsTexPosRate * (x + z);
		GXPosition3f32(x, ropeY - mRopeWidthBetweenBoardsY, z);
		GXTexCoord2f32(0.0f, texPos);
		GXPosition3f32(x + tex.x, ropeY, z + tex.y);
		GXTexCoord2f32(1.0f, texPos);
		x += stepX;
		y += stepY;
		z += stepZ;
	}
}

void THangingBridge::drawLowerMinus(const JGeometry::TVec3<f32>& a,
                                    const JGeometry::TVec3<f32>& b,
                                    const JGeometry::TVec2<f32>& tex,
                                    int pointNum) const
{
	f32 x     = a.x;
	f32 y     = a.y;
	f32 z     = a.z;
	f32 inv   = 1.0f / (f32)pointNum;
	f32 stepX = (b.x - a.x) * inv;
	f32 stepY = (b.y - a.y) * inv;
	f32 stepZ = (b.z - a.z) * inv;

	for (int i = 0; i < pointNum; ++i) {
		f32 ropeY  = y - unk38[i];
		f32 texPos = mBetweenBoardsTexPosRate * (x + z);
		GXPosition3f32(x - tex.x, ropeY, z - tex.y);
		GXTexCoord2f32(0.0f, texPos);
		GXPosition3f32(x, ropeY - mRopeWidthBetweenBoardsY, z);
		GXTexCoord2f32(1.0f, texPos);
		x += stepX;
		y += stepY;
		z += stepZ;
	}
}
#pragma dont_inline off

THangingBridgeBoard::THangingBridgeBoard(const char* name)
    : TLeanBlock(name)
{
	unk1BC = 0;
	unk194 = 0;
	unk198 = 0;
	unk19C = 0;
	unk1A0 = 0;
	unk1A4[0].zero();
	unk1A4[1].zero();
}

void THangingBridgeBoard::initMapObj()
{
	TLeanBlock::initMapObj();
	unk140 = 0.01f;
	unk144 = 0.02f;
	unk148 = 0.08f;
}

void THangingBridgeBoard::setGroundCollision()
{
	int yoshiActive = (u8)((TYoshi*)SMS_GetYoshi())->mState == 0 ? 0 : 1;
	if (yoshiActive
	    && mPosition.x - mBodyRadius
	           < ((TYoshi*)SMS_GetYoshi())->mTranslation.x
	    && mPosition.x + mBodyRadius
	           > ((TYoshi*)SMS_GetYoshi())->mTranslation.x
	    && mPosition.z - mBodyRadius
	           < ((TYoshi*)SMS_GetYoshi())->mTranslation.z
	    && mPosition.z + mBodyRadius
	           > ((TYoshi*)SMS_GetYoshi())->mTranslation.z) {
		J3DModel* model            = getModel();
		MtxPtr mtx                 = model->mNodeMatrices[0];
		if (mMapCollisionManager->unk8 != 0) {
			mMapCollisionManager->unk8->moveMtx(mtx);
		}
		return;
	}

	TMapObjBase::setGroundCollision();
}

void THangingBridgeBoard::calcDefaultMtx()
{
	Mtx rotX;
	makeRootMtxRotX(rotX);
	Mtx rotY;
	makeRootMtxRotY(rotY);
	PSMTXConcat(rotY, rotX, rotY);
	TSMtx34f& defaultMtx = *(TSMtx34f*)(void*)unk164;
	defaultMtx.set((const f32(*)[4])rotY);
	mVelocity.y = 0.0f;
	mPosition.y = mInitialPosition.y;
}

void THangingBridgeBoard::control()
{
	TLeanBlock::control();

	if (marioIsOn()) {
		mVelocity.y -= mMarioAccelY;
		f32 accelY = mMarioAccelY;
		if (unk194 != 0) {
			f32 accel = accelY * unk1BC->unk3C.y;
			unk194->mVelocity.y -= accel;
			if (unk19C != 0) {
				f32 accel2 = accelY * unk1BC->unk3C.z;
				unk19C->mVelocity.y -= accel2;
			}
		}
		if (unk198 != 0) {
			f32 accel = accelY * unk1BC->unk3C.y;
			unk198->mVelocity.y -= accel;
			if (unk1A0 != 0) {
				f32 accel2 = accelY * unk1BC->unk3C.z;
				unk1A0->mVelocity.y -= accel2;
			}
		}
	}

	if (marioHipAttack()) {
		mVelocity.y -= mMarioHipDropAccelY;
		f32 accelY = mMarioHipDropAccelY;
		if (unk194 != 0) {
			f32 accel = accelY * unk1BC->unk3C.y;
			unk194->mVelocity.y -= accel;
			if (unk19C != 0) {
				f32 accel2 = accelY * unk1BC->unk3C.z;
				unk19C->mVelocity.y -= accel2;
			}
		}
		if (unk198 != 0) {
			f32 accel = accelY * unk1BC->unk3C.y;
			unk198->mVelocity.y -= accel;
			if (unk1A0 != 0) {
				f32 accel2 = accelY * unk1BC->unk3C.z;
				unk1A0->mVelocity.y -= accel2;
			}
		}
	}

	mPosition.y += mVelocity.y;
	f32 target = mInitialPosition.y - mPosition.y;
	mVelocity.y += target * mReturnAccelRate;
	mVelocity.y *= mSpeedDownRate;

	MtxPtr mtx = getModel()->mNodeMatrices[0];
	unk1A4[0].x = mPosition.x - mtx[0][0] * unk1BC->unk3C.x;
	unk1A4[0].y = mPosition.y - mtx[1][0] * unk1BC->unk3C.x + 70.0f;
	unk1A4[0].z = mPosition.z - mtx[2][0] * unk1BC->unk3C.x;
	unk1A4[1].x = mPosition.x + mtx[0][0] * unk1BC->unk3C.x;
	unk1A4[1].y = mPosition.y + mtx[1][0] * unk1BC->unk3C.x + 70.0f;
	unk1A4[1].z = mPosition.z + mtx[2][0] * unk1BC->unk3C.x;
}

#pragma dont_inline on
void THangingBridgeBoard::drawOneRope(const JGeometry::TVec3<f32>& bottom) const
{
	f32 y      = bottom.y;
	f32 topY   = y + THangingBridge::mRopeHeight;
	f32 x      = bottom.x;
	f32 xPlus  = x + mRopeWidthX;
	f32 xMinus = x - mRopeWidthX;
	f32 z      = bottom.z;
	f32 zPlus  = z + mRopeWidthZ;
	f32 zMinus = z - mRopeWidthZ;
	f32 texBot = mTexPosRate * (y - y);
	f32 texTop = mTexPosRate * (topY - y);

	GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 8);
	GXPosition3f32(x, topY, zPlus);
	GXTexCoord2f32(0.0f, texTop);
	GXPosition3f32(x, y, zPlus);
	GXTexCoord2f32(0.0f, texBot);
	GXPosition3f32(xMinus, topY, zMinus);
	GXTexCoord2f32(1.0f, texTop);
	GXPosition3f32(xMinus, y, zMinus);
	GXTexCoord2f32(1.0f, texBot);
	GXPosition3f32(xPlus, topY, zMinus);
	GXTexCoord2f32(2.0f, texTop);
	GXPosition3f32(xPlus, y, zMinus);
	GXTexCoord2f32(2.0f, texBot);
	GXPosition3f32(x, topY, zPlus);
	GXTexCoord2f32(3.0f, texTop);
	GXPosition3f32(x, y, zPlus);
	GXTexCoord2f32(3.0f, texBot);
}
#pragma dont_inline off

void TJumpMushroom::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	int bgType;
	stream.read(&bgType, 4);
	if (mMapCollisionManager != 0)
		mMapCollisionManager->unk8->setAllData((s16)bgType);
}

BOOL TJumpMushroom::receiveMessage(THitActor*, unsigned long)
{
	startAnim(1);
	return TRUE;
}

void TMapObjMonteRoot::initMapObj()
{
	TMapObjBase::initMapObj();
	mDamageHeight = 1400.0f * mScaling.y;
	calcEntryRadius();
	mPosition.y = mInitialPosition.y + mYOffset;
}
