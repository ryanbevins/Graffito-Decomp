#include <MoveBG/MapObjMonte.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Map/MapCollisionManager.hpp>
#include <M3DUtil/MActor.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/Watergun.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
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

static inline f32 randSigned() { return 1.0f - 2.0f * randUnit(); }

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
	stream.read(&unk144, 4);

	unk138.x = 5000.0f;
	unk138.y = 5000.0f;
	unk154   = 0.998f;

	Mtx mtx;
	MsMtxSetXYZRPH(mtx, 0.0f, 0.0f, 0.0f, mRotation.x, mRotation.y,
	               mRotation.z);
	unk148.x = mtx[0][2] * windScale * 0.01f;
	unk148.y = mtx[1][2] * windScale * 0.01f;
	unk148.z = mtx[2][2] * windScale * 0.01f;
}

void TFluffManager::loadAfter()
{
	TMapObjBase::loadAfter();
	unk160 = 0;
	unk164 = 32;
	unk168 = new TFluff*[unk164];

	TFluff* first = new TFluff("１つ目のわた毛");
	first->initAndRegister("Fluff");
	first->unk168 = this;
	first->unk16C = 1;
	unk158        = first;
	first->makeObjAppeared();
	first->mPosition = mPosition;
	first->mRotation = mRotation;
	first->mInitialPosition.set(randSigned() * unk138.x, randUnit() * mPosition.y,
	                            randSigned() * unk138.y);
	unk168[unk160++] = first;

	TFluff* second = new TFluff("２つ目のわた毛");
	second->initAndRegister("Fluff");
	second->unk168 = this;
	unk15C         = second;
	second->mPosition = mPosition;
	second->mRotation = mRotation;
	second->mInitialPosition.set(randSigned() * unk138.x,
	                             randUnit() * mPosition.y,
	                             randSigned() * unk138.y);
	second->appear();
	unk168[unk160++] = second;

	for (int i = 2; i < unk164; ++i) {
		TFluff* fluff = new TFluff("わた毛");
		fluff->initAndRegister("Fluff");
		fluff->unk168 = this;
		unk168[unk160++] = fluff;
		fluff->makeObjAppeared();
	}
}

void TFluffManager::control()
{
	TMapObjBase::control();

	if (unk15C == 0) {
		for (int i = 0; i < unk160; ++i) {
			if (unk168[i] != 0 && unk168[i]->isState(4)) {
				unk15C = unk168[i];
				break;
			}
		}
	}

	if (unk158 != 0 && unk15C != 0 && unk15C->isState(4)) {
		unk15C->appear();
		unk15C = 0;
	}

	if (unk148.x < mWindMin && unk148.x > -mWindMin)
		unk148.x += gpMapObjManager->unkD0.x * 0.01f;
	if (unk148.z < mWindMin && unk148.z > -mWindMin)
		unk148.z += gpMapObjManager->unkD0.z * 0.01f;

	unk148.x *= unk154;
	unk148.y *= unk154;
	unk148.z *= unk154;
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
	if (mgr != 0) {
		mPosition.x = mgr->mPosition.x + randSigned() * mgr->unk138.x;
		mPosition.y = randUnit() * mgr->mPosition.y;
		mPosition.z = mgr->mPosition.z + randSigned() * mgr->unk138.y;
		mInitialPosition = mPosition;
	}

	mScaling.x = 0.0001f;
	mScaling.y = 0.0001f;
	mScaling.z = 0.0001f;
	zeroVec(unk154);
	zeroVec(mVelocity);
	unk150 = 0.2f + 0.6f * randUnit();

	f32 angle = randUnit() * 360.0f;
	unk140    = JMASin(angle);
	unk144    = JMACos(angle);
	unk148    = randUnit() * 360.0f;
	unk14C    = 0.3f;
	mRotation.y = randUnit() * 360.0f;
	mState      = 2;
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
		if (mScaling.x >= 1.0f) {
			oneVec(mScaling);
			startAnim(0);
			mState = 1;
		}
		break;
	case 1:
		if (mPosition.y <= -unk138 || fabsf(mPosition.x - mInitialPosition.x) > unk138
		    || fabsf(mPosition.z - mInitialPosition.z) > unk138) {
			kill();
		}
		break;
	case 3:
		mScaling.x -= mScaleDownSpeed;
		mScaling.y -= mScaleDownSpeed;
		mScaling.z -= mScaleDownSpeed;
		if (mScaling.x <= 0.1f) {
			if (gpMarioParticleManager != 0)
				gpMarioParticleManager->emit(0xE5, &mPosition, 0, this);
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
		if (mLifeTimer <= 0 && unk168 != 0 && unk168->unk15C == 0)
			unk168->unk15C = this;
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
	if (mPosition.y < -unk138)
		mPosition.y = unk138;

	if (unk168 != 0) {
		mVelocity.x += unk168->unk148.x;
		mVelocity.z += unk168->unk148.z;
	}

	unk148 = MsWrap(unk148 + unk14C, 0.0f, 360.0f);
	mVelocity.x += unk140 * (unk150 * JMASin(unk148));
	mVelocity.z += unk144 * (unk150 * JMACos(unk148));

	mPosition.x += mVelocity.x;
	mPosition.z += mVelocity.z;
	mVelocity.x *= unk164;
	mVelocity.z *= unk164;

	if (mHeldObject != 0)
		gpMarioPos->y -= unk13C;
}

u32 TFluff::touchWater(THitActor* sender)
{
	JGeometry::TVec3<f32>* speed = getWaterSpeed(sender);
	if (speed != 0) {
		mVelocity.x += speed->x * unk160;
		mVelocity.y += speed->y * unk160;
		mVelocity.z += speed->z * unk160;
	}
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
	MTXIdentity(unk14C);
	zeroVec(unk17C);
}

void TSwingBoard::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	stream.read(&unk138, 4);
	if (unk138 == -1.0f)
		unk138 = 5000.0f;
	stream.read(&unk140, 4);
	if (unk140 <= 0.0f || unk140 > 10.0f)
		unk140 = 0.003f;

	unk17C.set(mPosition.x, mPosition.y + unk138, mPosition.z);
	unk148 = randUnit();
	unk13C = randUnit() * 360.0f;
	unk144 = randSigned() * 0.5f;
	MsMtxSetXYZRPH(unk14C, 0.0f, 0.0f, 0.0f, 0.0f, mRotation.y, 0.0f);
}

void TSwingBoard::control()
{
	TMapObjBase::control();

	if (marioIsOn()) {
		TWaterGun* gun = (TWaterGun*)SMS_GetMarioWaterGun();
		if (gun != 0 && gun->mIsEmitWater) {
			MtxPtr emitMtx = gun->getNozzleMtx();
			unk144 += emitMtx[2][0] * unk140;
		}
	}

	unk13C += unk144;
	unk144 -= unk13C * mReturnAccelRate;
	unk144 *= mSpeedDownRate;
	mRotation.x = -unk13C;

	Mtx rot;
	MsMtxSetXYZRPH(rot, 0.0f, 0.0f, 0.0f, mRotation.x, 0.0f, 0.0f);
	MTXConcat(unk14C, rot, getModel()->getBaseTRMtx());
	getModel()->getBaseTRMtx()[0][3] = mPosition.x;
	getModel()->getBaseTRMtx()[1][3] = mPosition.y;
	getModel()->getBaseTRMtx()[2][3] = mPosition.z;

	mPosition.y = unk17C.y - unk138 * JMACos(unk13C);
}

void TSwingBoard::draw() const
{
	TMapObjBase::draw();
	initDraw();

	MtxPtr mtx = getModel()->getBaseTRMtx();
	JGeometry::TVec3<f32> left(mtx[0][3] - mBoardWidth * 0.5f, mtx[1][3],
	                           mtx[2][3]);
	JGeometry::TVec3<f32> right(mtx[0][3] + mBoardWidth * 0.5f, mtx[1][3],
	                            mtx[2][3]);
	JGeometry::TVec3<f32> upperLeft(left.x, unk17C.y, left.z);
	JGeometry::TVec3<f32> upperRight(right.x, unk17C.y, right.z);
	drawOneRope(upperLeft, left);
	drawOneRope(upperRight, right);
}

void TSwingBoard::initDraw() const
{
	JUTTexture tex(gpMapObjManager->unkCC);
	tex.load(GX_TEXMAP0);
	GXSetNumChans(0);
	GXSetNumTexGens(1);
	GXSetNumTevStages(1);
	GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXSetCullMode(GX_CULL_NONE);
}

void TSwingBoard::drawOneRope(const JGeometry::TVec3<f32>& top,
                              const JGeometry::TVec3<f32>& bottom) const
{
	drawRopeQuad(top, bottom, mRopeWidthX, mRopeWidthZ, mTexPosRate);
}

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
		unk18.set(0.0f, -130.0f, 119000.0f);
		unk24.set(0.0f, -660.0f, 130500.0f);
		mRopeHeight = 500.0f;
		unk3C.set(0.8f, 0.5f, 1.2f);
	} else if (gpMarDirector->mMap == 0x08) {
		unk10       = 19;
		unk18.set(0.0f, 0.0f, 9000.0f);
		unk24.set(0.0f, -900.0f, 17000.0f);
		mRopeHeight = 70.0f;
		unk3C.set(1.0f, 0.5f, 1.0f);
	} else {
		unk10       = 0;
		mRopeHeight = 0.0f;
		return;
	}

	unk30.x = unk24.x - unk18.x;
	unk30.y = unk24.z - unk18.z;
	f32 len = JGeometry::TUtil<f32>::sqrt(unk30.x * unk30.x
	                                      + unk30.y * unk30.y);
	if (len > 0.0f) {
		unk30.x /= len;
		unk30.y /= len;
	}

	unk14 = new THangingBridgeBoard*[unk10];
	for (int i = 0; i < unk10; ++i) {
		f32 rate = (unk10 <= 1) ? 0.0f : (f32)i / (f32)(unk10 - 1);
		JGeometry::TVec3<f32> pos(unk18.x + (unk24.x - unk18.x) * rate,
		                           unk18.y + (unk24.y - unk18.y) * rate,
		                           unk18.z + (unk24.z - unk18.z) * rate);
		JGeometry::TVec3<f32> rot(0.0f, 0.0f, 0.0f);
		JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
		const char* objName = gpMarDirector->mMap == 0x0D ? "PinnaHangingBridgeBoard"
		                                                  : "HangingBridgeBoard";
		THangingBridgeBoard* board
		    = (THangingBridgeBoard*)TMapObjBaseManager::newAndRegisterObj(
		        objName, pos, rot, scale);
		unk14[i]     = board;
		board->unk1BC = this;
		board->calcDefaultMtx();
	}

	for (int i = 0; i < unk10; ++i) {
		unk14[i]->unk194 = (i > 0) ? unk14[i - 1] : 0;
		unk14[i]->unk198 = (i + 1 < unk10) ? unk14[i + 1] : 0;
		unk14[i]->unk19C = (i > 1) ? unk14[i - 2] : 0;
		unk14[i]->unk1A0 = (i + 2 < unk10) ? unk14[i + 2] : 0;
	}

	unk38 = new f32[mPointNumBetweenBoards];
	for (int i = 0; i < mPointNumBetweenBoards; ++i)
		unk38[i] = sinf(((f32)i / (f32)mPointNumBetweenBoards) * 3.1415927f);
}

void THangingBridge::perform(unsigned long flags, JDrama::TGraphics*)
{
	if ((flags & 8) == 0 || unk14 == 0)
		return;

	initDraw();
	for (int i = 0; i < unk10; ++i) {
		unk14[i]->drawOneRope(unk14[i]->unk1A4[0]);
		unk14[i]->drawOneRope(unk14[i]->unk1A4[1]);
	}
	drawRopeBetweenBoards(0.0f, mPointNumBetweenBoards);
	drawRopeBetweenBoards(mRopeHeight, mPointNumBetweenBoards);
}

void THangingBridge::initDraw() const
{
	JUTTexture tex(gpMapObjManager->unkCC);
	tex.load(GX_TEXMAP0);
	GXSetNumChans(0);
	GXSetNumTexGens(1);
	GXSetNumTevStages(1);
	GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXSetCullMode(GX_CULL_NONE);
}

void THangingBridge::drawRopeBetweenBoards(f32 y, int pointNum) const
{
	if (unk10 < 2 || unk14 == 0)
		return;

	for (int i = 0; i + 1 < unk10; ++i) {
		JGeometry::TVec3<f32> a = unk14[i]->mPosition;
		JGeometry::TVec3<f32> b = unk14[i + 1]->mPosition;
		a.y += y;
		b.y += y;
		JGeometry::TVec2<f32> tex(0.0f, 0.0f);
		drawLowerMinus(a, b, tex, pointNum);
		drawLowerPlus(a, b, tex, pointNum);
		drawUpper(a, b, tex, pointNum);
	}
}

void THangingBridge::drawUpper(const JGeometry::TVec3<f32>& a,
                               const JGeometry::TVec3<f32>& b,
                               const JGeometry::TVec2<f32>&, int) const
{
	drawRopeQuad(a, b, mRopeWidthBetweenBoards, 0.0f, mBetweenBoardsTexPosRate);
}

void THangingBridge::drawLowerPlus(const JGeometry::TVec3<f32>& a,
                                   const JGeometry::TVec3<f32>& b,
                                   const JGeometry::TVec2<f32>&, int) const
{
	JGeometry::TVec3<f32> aa(a.x, a.y - mRopeWidthBetweenBoardsY, a.z);
	JGeometry::TVec3<f32> bb(b.x, b.y - mRopeWidthBetweenBoardsY, b.z);
	drawRopeQuad(aa, bb, mRopeWidthBetweenBoards, 0.0f,
	             mBetweenBoardsTexPosRate);
}

void THangingBridge::drawLowerMinus(const JGeometry::TVec3<f32>& a,
                                    const JGeometry::TVec3<f32>& b,
                                    const JGeometry::TVec2<f32>&, int) const
{
	JGeometry::TVec3<f32> aa(a.x, a.y + mRopeWidthBetweenBoardsY, a.z);
	JGeometry::TVec3<f32> bb(b.x, b.y + mRopeWidthBetweenBoardsY, b.z);
	drawRopeQuad(aa, bb, mRopeWidthBetweenBoards, 0.0f,
	             mBetweenBoardsTexPosRate);
}

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
	TLeanBlock::setGroundCollision();
}

void THangingBridgeBoard::calcDefaultMtx()
{
	makeRootMtxRotX(unk164);
	Mtx rotY;
	makeRootMtxRotY(rotY);
	MTXConcat(rotY, unk164, unk164);
	mVelocity.y = 0.0f;
	mPosition.y = mInitialPosition.y;
}

void THangingBridgeBoard::control()
{
	TLeanBlock::control();

	if (marioIsOn()) {
		f32 accel = marioHipAttack() ? mMarioHipDropAccelY : mMarioAccelY;
		mVelocity.y -= accel;
		if (unk194 != 0)
			unk194->mVelocity.y -= accel * 0.5f;
		if (unk198 != 0)
			unk198->mVelocity.y -= accel * 0.5f;
		if (unk19C != 0)
			unk19C->mVelocity.y -= accel * 0.25f;
		if (unk1A0 != 0)
			unk1A0->mVelocity.y -= accel * 0.25f;
	}

	f32 target = mInitialPosition.y - mPosition.y;
	mVelocity.y += target * mReturnAccelRate;
	mVelocity.y *= mSpeedDownRate;
	mPosition.y += mVelocity.y;

	if (getModel() != 0)
		copyTransToVec(unk1A4[0], getModel()->getBaseTRMtx());

	unk1A4[1] = unk1A4[0];
	unk1A4[0].x -= mRopeWidthX;
	unk1A4[0].y += 70.0f;
	unk1A4[0].z -= mRopeWidthZ;
	unk1A4[1].x += mRopeWidthX;
	unk1A4[1].y += 70.0f;
	unk1A4[1].z += mRopeWidthZ;
}

void THangingBridgeBoard::drawOneRope(const JGeometry::TVec3<f32>& bottom) const
{
	JGeometry::TVec3<f32> top = bottom;
	top.y += THangingBridge::mRopeHeight;
	drawRopeQuad(top, bottom, mRopeWidthX, mRopeWidthZ, mTexPosRate);
}

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
	mDamageRadius = 1400.0f * mScaling.y;
	calcEntryRadius();
	mPosition.y = mInitialPosition.y + mDamageHeight;
}
