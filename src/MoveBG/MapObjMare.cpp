#define JGEOMETRY_TVEC3_SUB_OUT_OF_LINE
#include <MoveBG/MapObjMare.hpp>
#undef JGEOMETRY_TVEC3_SUB_OUT_OF_LINE
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <MoveBG/MapObjWave.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <Player/ModelWaterManager.hpp>
#include <Player/Watergun.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <Map/Map.hpp>

// rogue includes for matching __sinit (15 JALList<T> templates)
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <Map/MapCollisionData.hpp>
#include <Map/MapData.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#define MAP_COLLISION_ENTRY_DEFINE_SET_UP_TRANS
#include <Map/MapCollisionEntry.hpp>
#undef MAP_COLLISION_ENTRY_DEFINE_SET_UP_TRANS
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <math.h>
#include <string.h>

class TMareEventDepressWall {
public:
	bool startEvent();
};

class TCannon {
public:
	void startChorobeiShout();
	bool isObject();
};

class TMapWireManager {
public:
	u32 getWireNo(const JGeometry::TVec3<f32>&) const;
	void getPointPosInNthWire(int, const JGeometry::TVec3<f32>&,
	                          JGeometry::TVec3<f32>*) const;
};

extern TMapWireManager* gpMapWireManager;

extern JGeometry::TVec3<f32>* gpMarioPos;
extern f32* gpMarioSpeedY;

void SMS_MarioMoveRequest(const JGeometry::TVec3<f32>&);
bool SMS_SendMessageToMario(THitActor*, u32);
void SMS_ThrowMario(const JGeometry::TVec3<f32>&, f32);

static JGeometry::TVec3<f32> fall_upper_pos(2827.0f, 8604.0f, 7202.0f);

f32 TCogwheelScale::mWaterLeakSpeed = 0.01f;
static f32 sRadius                  = 800.0f;
f32 TCogwheel::mRopeWidthX          = 10.0f;
f32 TCogwheel::mRopeWidthZ          = 7.0f;
f32 TCogwheel::mTexPosRate          = 0.01f;
f32 TCogwheel::mMinSpeed            = 3.0f;
static f32 mGrowStartFrame          = 90.0f;
static f32 mGrowEndFrame            = 175.0f;

// TMareEventPoint

u32 TCogwheelScale::touchWater(THitActor*)
{
	if (unk140 < unk144)
		unk140 += 1.0f;
	return 1;
}
BOOL TCogwheelScale::receiveMessage(THitActor* sender, u32 message)
{
	if (message == HIT_MESSAGE_HIP_DROP) {
		f32* speed = &unk158->unk138;
		*speed     = *speed + unk150;
		return true;
	}

	return TMapObjBase::receiveMessage(sender, message);
}
void TCogwheelScale::touchPlayer(THitActor*)
{
	if (marioIsOn())
		unk148 = unk13C;

	if (mPosition.y - mYOffset > gpMarioPos->y + 150.0f) {
		if ((unk154 && unk158->unk138 > 0.0f)
		    || (!unk154 && unk158->unk138 < 0.0f)) {
			TCogwheel* cogwheel = unk158;
			cogwheel->unk138 *= -cogwheel->unk148;
			if (fabsf(cogwheel->unk138) < TCogwheel::mMinSpeed)
				cogwheel->unk138 = 0.0f;

			if (marioHeadAttack()) {
				f32* speed = &unk158->unk138;
				*speed     = *speed * *gpMarioSpeedY * unk14C;
			}
		}
	}

	unk140 = 0.0f;
}
void TCogwheelScale::control()
{
	unk148 = 0.0f;
	TMapObjBase::control();

	if (unk140 > 0.0f) {
		unk140 -= mWaterLeakSpeed;
		f32 volume = __fabsf(unk140);

		if (gpMSound->gateCheck(0x3061))
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(
			    0x3061, (const Vec*)&mPosition, nullptr, volume, 0, 0,
			    nullptr, 0, 4);

		if (unk140 < 0.0f)
			unk140 = 0.0f;
	}
}
TCogwheelScale::TCogwheelScale(const char* name)
    : TMapObjBase(name)
{
	unk138 = 0.0f;
	unk13C = 0.0f;
	unk140 = 0.0f;
	unk144 = 0.0f;
	unk148 = 0.0f;
	unk14C = 0.01f;
	unk150 = 5.0f;
	unk154 = 0;
	unk158 = nullptr;
}
void TCogwheel::initDraw() const
{
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXLoadPosMtxImm(j3dSys.getViewMtx(), 0);
	GXSetCurrentMtx(0);
	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);

	static const GXColor sCogwheelColor = { 0x00, 0x00, 0x64, 0xFF };
	GXColor color;
	volatile GXColor temp;
	temp  = sCogwheelColor;
	color = *(GXColor*)&temp;
	GXSetChanMatColor(GX_COLOR0A0, color);
	GXSetNumTexGens(1);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3C, 0,
	                  0x7D);

	JUTTexture texture(gpMapObjManager->unkC8);
	texture.load(GX_TEXMAP0);

	GXSetNumTevStages(1);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO,
	                GX_CC_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1,
	                GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1,
	                GX_TEVPREV);
	GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
	GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GXSetCullMode(GX_CULL_BACK);
}

// TCogwheelScale
void TCogwheel::draw() const
{
	initDraw();

	f32 plateY = unk150->mPosition.y - unk150->mYOffset;
	f32 plateTopY = plateY + 600.0f;
	f32 plateTopTex = mTexPosRate * (plateTopY - plateY);
	f32 plateBottomTex = mTexPosRate * (unk154.y - plateY);
	f32 plateXMinus = unk154.x - mRopeWidthX;
	f32 plateXPlus = unk154.x + mRopeWidthX;
	f32 plateZMinus = unk154.z - mRopeWidthZ;
	f32 plateZPlus = unk154.z + mRopeWidthZ;

	GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 8);
	GXPosition3f32(plateXMinus, plateTopY, plateZMinus);
	GXTexCoord2f32(0.0f, plateTopTex);
	GXPosition3f32(plateXMinus, unk154.y, plateZMinus);
	GXTexCoord2f32(0.0f, plateBottomTex);
	GXPosition3f32(plateXPlus, plateTopY, plateZPlus);
	GXTexCoord2f32(1.0f, plateTopTex);
	GXPosition3f32(plateXPlus, unk154.y, plateZPlus);
	GXTexCoord2f32(1.0f, plateBottomTex);
	GXPosition3f32(plateXPlus, plateTopY, plateZMinus);
	GXTexCoord2f32(2.0f, plateTopTex);
	GXPosition3f32(plateXPlus, unk154.y, plateZMinus);
	GXTexCoord2f32(2.0f, plateBottomTex);
	GXPosition3f32(plateXMinus, plateTopY, plateZPlus);
	GXTexCoord2f32(3.0f, plateTopTex);
	GXPosition3f32(plateXMinus, unk154.y, plateZPlus);
	GXTexCoord2f32(3.0f, plateBottomTex);

	f32 potY = unk164->mPosition.y - unk164->mYOffset;
	f32 potTopY = potY + 1200.0f;
	f32 potTopTex = mTexPosRate * (potTopY - potY);
	f32 potBottomTex = mTexPosRate * (unk154.y - potY);
	f32 potXMinus = unk168.x - mRopeWidthX;
	f32 potXPlus = unk168.x + mRopeWidthX;
	f32 potZMinus = unk168.z - mRopeWidthZ;
	f32 potZPlus = unk168.z + mRopeWidthZ;

	GXBegin(GX_QUADS, GX_VTXFMT0, 8);
	GXPosition3f32(potXMinus, potTopY, potZMinus);
	GXTexCoord2f32(0.0f, potTopTex);
	GXPosition3f32(potXMinus, unk154.y, potZMinus);
	GXTexCoord2f32(0.0f, potBottomTex);
	GXPosition3f32(potXPlus, potTopY, potZPlus);
	GXTexCoord2f32(1.0f, potTopTex);
	GXPosition3f32(potXPlus, unk154.y, potZPlus);
	GXTexCoord2f32(1.0f, potBottomTex);
	GXPosition3f32(potXPlus, potTopY, potZMinus);
	GXTexCoord2f32(0.0f, potTopTex);
	GXPosition3f32(potXPlus, unk154.y, potZMinus);
	GXTexCoord2f32(0.0f, potBottomTex);
	GXPosition3f32(potXMinus, potTopY, potZPlus);
	GXTexCoord2f32(1.0f, potTopTex);
	GXPosition3f32(potXMinus, unk154.y, potZPlus);
	GXTexCoord2f32(1.0f, potBottomTex);
}
void TCogwheel::calc()
{
	mRotation.z = 360.0f * (-unk13C / (3.14f * (2.0f * sRadius)));

	Mtx zRot;
	Mtx yRot;
	makeRootMtxRotZ(zRot);
	zRot[0][3] = 0.0f;
	zRot[1][3] = 0.0f;
	zRot[2][3] = 0.0f;
	makeRootMtxRotY(yRot);
	yRot[0][3] = 0.0f;
	yRot[1][3] = 0.0f;
	yRot[2][3] = 0.0f;
	PSMTXConcat(yRot, zRot, getModel()->mNodeMatrices[0]);
	getModel()->mNodeMatrices[0][0][3] = mPosition.x;
	getModel()->mNodeMatrices[0][1][3] = mPosition.y;
	getModel()->mNodeMatrices[0][2][3] = mPosition.z;
}
void TCogwheel::control()
{
	TMapObjBase::control();

	unk13C += unk138;

	TCogwheelScale* pot   = unk164;
	TCogwheelScale* plate = unk150;
	f32 potWeight   = pot->unk138 + pot->unk140 + pot->unk148;
	f32 plateWeight = plate->unk138 + plate->unk140 + plate->unk148;
	unk138 += unk140 * (plateWeight - potWeight);
	unk138 *= unk144;

	if (unk13C < unk160 && unk138 < 0.0f) {
		unk138 *= -unk148;
		if (fabsf(unk138) < mMinSpeed)
			unk138 = 0.0f;
	}

	if (unk13C > unk14C - unk174 && unk138 > 0.0f) {
		unk138 *= -unk148;
		if (fabsf(unk138) < mMinSpeed)
			unk138 = 0.0f;
	}

	unk150->mPosition.y = mPosition.y - unk13C + unk150->mYOffset;
	unk164->mPosition.y = mPosition.y - (unk14C - unk13C);

	f32 volume = __fabsf(unk138);
	if (volume > mMinSpeed && gpMSound->gateCheck(0x3060))
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x3060, (const Vec*)&mPosition, nullptr, volume, 0, 0, nullptr,
		    0, 4);
}
void TCogwheel::initMapObj()
{
	TMapObjBase::initMapObj();

	f32 angle = mRotation.z * 0.017453294f;
	f32 cx    = sRadius * cosf(angle);
	f32 sz    = sRadius * sinf(angle);

	JGeometry::TVec3<f32> pos(mPosition.x + cx, mPosition.y,
	                          mPosition.z - sz);
	JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);

	unk150 = (TCogwheelScale*)TMapObjBaseManager::newAndRegisterObj(
	    "cogwheel_plate", pos, mRotation, scale);
	unk150->unk154 = 1;
	unk150->unk158 = this;
	unk150->appear();
	unk154 = pos;

	pos.set(mPosition.x - cx, mPosition.y, mPosition.z + sz);
	unk164 = (TCogwheelScale*)TMapObjBaseManager::newAndRegisterObj(
	    "cogwheel_pot", pos, mRotation, scale);
	unk164->unk154 = 0;
	unk164->unk158 = this;
	unk164->appear();
	unk168 = pos;

	if (strcmp(getName(), "\x93\x56\x94\x89\x8F\xE3") == 0) {
		unk140 = 0.003f;
		unk144 = 0.99f;
		unk148 = 0.8f;
		unk14C = 3800.0f;
		unk160 = 1000.0f;
		unk174 = 1800.0f;
	} else {
		unk140 = 0.008f;
		unk144 = 0.98f;
		unk148 = 0.8f;
		unk14C = 3950.0f;
		unk160 = 1000.0f;
		unk174 = 1900.0f;
	}

	unk164->unk138 = 0.0f;
	unk164->unk13C = 0.0f;
	unk164->unk144 = 14.0f;
	unk150->unk138 = 10.0f;
	unk150->unk13C = 0.0f;
	unk150->unk144 = 0.0f;

	unk13C = unk14C * 0.5f;
}
TCogwheel::TCogwheel(const char* name)
    : TMapObjBase(name)
{
	unk138 = 0.0f;
	unk13C = 0.0f;
	unk140 = 0.0f;
	unk144 = 0.0f;
	unk148 = 0.0f;
	unk14C = 0.0f;
	unk150 = nullptr;
	unk160 = 0.0f;
	unk164 = nullptr;
	unk174 = 0.0f;
	unk154.zero();
	unk168.zero();
}
void TMapObjElasticCode::draw() const
{
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXLoadPosMtxImm(j3dSys.getViewMtx(), 0);
	GXSetCurrentMtx(0);
	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	static const GXColor sElasticColor = { 0x00, 0x00, 0x64, 0xFF };
	GXColor color;
	volatile GXColor temp;
	temp  = sElasticColor;
	color = *(GXColor*)&temp;
	GXSetChanMatColor(GX_COLOR0A0, color);
	GXSetNumTexGens(0);
	GXSetNumTevStages(1);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL,
	              GX_COLOR0A0);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO,
	                GX_CC_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1,
	                GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1,
	                GX_TEVPREV);
	GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
	GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
	GXSetCullMode(GX_CULL_NONE);
	GXSetLineWidth(0x18, GX_TO_ZERO);

	GXBegin(GX_LINES, GX_VTXFMT0, 2);
	GXPosition3f32(mInitialPosition.x, mInitialPosition.y + 1000.0f,
	               mInitialPosition.z);
	GXPosition3f32(mPosition.x, mPosition.y, mPosition.z);
}

// TCogwheel
void TMapObjElasticCode::control()
{
	TMapObjBase::control();

	mVelocity.y *= unk140;
	f32 gravityY = getGravityY();
	mVelocity.y += unk13C * (mInitialPosition.y - mPosition.y) - gravityY;

	if (mHeldObject) {
		mVelocity.y -= unk138;
		JGeometry::TVec3<f32> pos = mHeldObject->mPosition;
		pos.y += mVelocity.y;
		mHeldObject->moveRequest(pos);
	}

	mPosition.y += mVelocity.y;
}
void TMapObjElasticCode::initMapObj()
{
	TMapObjBase::initMapObj();
	unk140  = 0.997f;
	mGravity = 0.01f;
	unk138  = 2.0f;
	unk13C  = 0.0005f;
}
inline static void updateGrowTreeHeight(TMapObjGrowTree* tree)
{
	J3DFrameCtrl* ctrl = tree->mMActor->getFrameCtrl(0);
	f32 frame          = ctrl->getFrame();

	if (frame > mGrowEndFrame) {
		tree->mDamageHeight = tree->unk138;
	} else if (frame > mGrowStartFrame) {
		f32 rate = (frame - mGrowStartFrame)
		           / (mGrowEndFrame - mGrowStartFrame);
		tree->mDamageHeight
		    = tree->unk148 + (tree->unk138 - tree->unk148) * rate;
	} else {
		tree->mDamageHeight = tree->unk148;
	}
	tree->calcEntryRadius();
}
u32 TMapObjGrowTree::touchWater(THitActor* water)
{
	if (water->mPosition.y > mPosition.y + unk148)
		return 0;

	if (mState == 1) {
		startAnim(1);
		J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
		ctrl->setRate(0.0f);
		mState = 2;
	}

	J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
	if (ctrl->getFrame() < ctrl->getEnd()) {
		soundBas(0x289A, 3.0f, unk13C);
		soundBas(0x289B, 67.0f, unk13C);
		soundBas(0x289C, 103.0f, unk13C);
		soundBas(0x289D, 137.0f, unk13C);
		ctrl->setFrame(ctrl->getFrame() + unk13C);
		updateGrowTreeHeight(this);

		if (mHeldObject) {
			JGeometry::TVec3<f32> pos = mHeldObject->mPosition;
			f32 move = 0.0f;
			if (mGrowStartFrame < ctrl->getFrame()
			    && ctrl->getFrame() < mGrowEndFrame)
				move = unk13C * unk138 / (mGrowEndFrame - mGrowStartFrame);
			pos.y += move;
			mHeldObject->moveRequest(pos);
		}
	}

	if (ctrl->getFrame() > mGrowEndFrame) {
		setUpMapCollision(0);
		mLifeTimer = unk144;
	}

	return 1;
}

// TMapObjElasticCode
void TMapObjGrowTree::control()
{
	TMapObjBase::control();

	if (mState != 2 || mHolder || mLifeTimer > 0)
		return;

	J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
	if (ctrl->getFrame() <= 0.0f)
		return;

	if (ctrl->getFrame() < mGrowEndFrame)
		removeMapCollision();

	ctrl->setFrame(ctrl->getFrame() - unk140);
	if (ctrl->getFrame() < 0.0f) {
		startAnim(0);
		mState = 1;
		return;
	}

	if (67.0f <= ctrl->getFrame() && ctrl->getFrame() <= 240.0f
	    && gpMSound->gateCheck(0x20C6))
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x20C6, (const Vec*)&mPosition, 0, nullptr, 0, 4);

	updateGrowTreeHeight(this);

	if (mHeldObject) {
		JGeometry::TVec3<f32> pos = mHeldObject->mPosition;
		f32 move = 0.0f;
		if (mGrowStartFrame < ctrl->getFrame()
		    && ctrl->getFrame() < mGrowEndFrame)
			move = unk140 * unk138 / (mGrowEndFrame - mGrowStartFrame);
		pos.y -= move;
		mHeldObject->moveRequest(pos);
	}
}
void TMapObjGrowTree::loadAfter()
{
	TMapObjBase::loadAfter();
	removeMapCollision();
}
void TMapObjGrowTree::initMapObj()
{
	TMapObjBase::initMapObj();
	unk138 = 1000.0f;
	unk13C = 0.5f;
	unk140 = 0.01f;
	unk144 = 360;
	unk148 = mDamageHeight;
	mMActor->setBtp("moyasi_wink");
}
TMapObjGrowTree::TMapObjGrowTree(const char* name)
    : TMapObjBase(name)
{
	unk138 = 0.0f;
	unk13C = 0.0f;
	unk140 = 0.0f;
	unk144 = 0;
	unk148 = 0.0f;
}
void TWireBell::initDraw() const
{
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXLoadPosMtxImm(j3dSys.getViewMtx(), 0);
	GXSetCurrentMtx(0);
	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);

	static const GXColor sWireBellColor = { 0x00, 0x00, 0x64, 0xFF };
	GXColor color;
	volatile GXColor temp;
	temp  = sWireBellColor;
	color = *(GXColor*)&temp;
	GXSetChanMatColor(GX_COLOR0A0, color);
	GXSetNumTexGens(1);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3C, 0,
	                  0x7D);

	JUTTexture texture(gpMapObjManager->unkC8);
	texture.load(GX_TEXMAP0);

	GXSetNumTevStages(1);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO,
	                GX_CC_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1,
	                GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1,
	                GX_TEVPREV);
	GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
	GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GXSetCullMode(GX_CULL_BACK);
}

// TMapObjGrowTree
void TWireBell::draw() const
{
	initDraw();

	f32 topY    = mPosition.y;
	f32 wireY   = unk14C.y;
	f32 minX    = unk14C.x - unk140;
	f32 maxX    = unk14C.x + unk140;
	f32 minZ    = unk14C.z - unk144;
	f32 maxZ    = unk14C.z + unk144;
	f32 texTop  = unk148 * (wireY - topY);
	f32 texBase = 0.0f;

	GXBegin(GX_QUADS, GX_VTXFMT0, 8);
	GXPosition3f32(minX, wireY, minZ);
	GXTexCoord2f32(0.0f, texTop);
	GXPosition3f32(minX, topY, minZ);
	GXTexCoord2f32(0.0f, texBase);
	GXPosition3f32(maxX, wireY, maxZ);
	GXTexCoord2f32(1.0f, texTop);
	GXPosition3f32(maxX, topY, maxZ);
	GXTexCoord2f32(1.0f, texBase);
	GXPosition3f32(maxX, wireY, minZ);
	GXTexCoord2f32(0.0f, texTop);
	GXPosition3f32(maxX, topY, minZ);
	GXTexCoord2f32(0.0f, texBase);
	GXPosition3f32(minX, wireY, maxZ);
	GXTexCoord2f32(1.0f, texTop);
	GXPosition3f32(minX, topY, maxZ);
	GXTexCoord2f32(1.0f, texBase);
}
void TWireBell::control()
{
	gpMapWireManager->getPointPosInNthWire(unk138, mPosition, &unk14C);
	mPosition.x = unk14C.x;
	mPosition.y = unk14C.y - unk13C;
	mPosition.z = unk14C.z;

	Mtx mtx;
	MsMtxSetTRS(mtx, mPosition.x, mPosition.y, mPosition.z, mRotation.x,
	            mRotation.y, mRotation.z, mScaling.x, mScaling.y, mScaling.z);
	PSMTXCopy(mtx, getModel()->mNodeMatrices[0]);
}
void TWireBell::loadAfter()
{
	TMapObjBase::loadAfter();
	unk138 = gpMapWireManager->getWireNo(mPosition);
}
TWireBell::TWireBell(const char* name)
    : TMapObjBase(name)
{
	unk138 = -1;
	unk13C = 200.0f;
	unk140 = 10.0f;
	unk144 = 5.0f;
	unk148 = 0.01f;
	unk14C.zero();
}
void TMapObjPuncher::touchPlayer(THitActor*)
{
	awake();
	startAnim(1);

	JGeometry::TVec3<f32> dir;
	makeVecToLocalZ(1.0f, &dir);

	JGeometry::TVec3<f32> request = *gpMarioPos;
	JGeometry::TVec3<f32> offset = dir;
	offset.scale(100.0f);
	request.add(offset);
	SMS_MarioMoveRequest(request);
	SMS_SendMessageToMario(this, 7);
	SMS_ThrowMario(dir, unk138);

	onHitFlag(HIT_FLAG_NO_COLLISION);

	JGeometry::TVec3<f32> scale(2.0f, 2.0f, 2.0f);
	emitAndScale(0xE5, 0, &mPosition, scale);
	emitAndScale(0xE6, 0, &mPosition, scale);

	if (gpMSound->gateCheck(0x387D))
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x387D, (const Vec*)&mPosition, 0, nullptr, 0, 4);

	mState = 2;
}

// TWireBell
void TMapObjPuncher::control()
{
	TMapObjBase::control();

	switch (mState) {
	case 1:
		break;
	case 2: {
		J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
		soundBas(0x385F, 101.0f, ctrl->getRate());

		if (!animIsFinished())
			break;

		JGeometry::TVec3<f32> scale(2.0f, 2.0f, 2.0f);
		emitAndScale(0xE5, 0, &mPosition, scale);
		emitAndScale(0xE6, 0, &mPosition, scale);

		if (gpMSound->gateCheck(0x387D))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x387D, (const Vec*)&mPosition, 0, nullptr, 0, 4);

		kill();
		break;
	}
	}
}
void TMapObjPuncher::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);

	s32 power;
	stream.read(&power, 4);
	unk138 = power;
	sleep();
	offHitFlag(HIT_FLAG_NO_COLLISION);
}
void TMuddyBoat::moveByWater()
{
	TWaterGun* waterGun = (TWaterGun*)SMS_GetMarioWaterGun();
	if ((s32)waterGun->mIsEmitWater == 0)
		return;

	MtxPtr emitMtx = ((TWaterGun*)SMS_GetMarioWaterGun())->getEmitMtx(0);
	JGeometry::TVec3<f32> dir(-emitMtx[0][0], 0.0f, -emitMtx[2][0]);
	MsVECNormalize((Vec*)&dir, (Vec*)&dir);

	MtxPtr modelMtx = getModel()->getAnmMtx(0);
	f32 forwardX    = modelMtx[0][2];
	f32 forwardZ    = modelMtx[2][2];
	JGeometry::TVec3<f32> forward(forwardX, 0.0f, forwardZ);
	f32 dot = dir.dot(forward);

	JGeometry::TVec3<f32> marioDir;
	getNormalVecFromTargetXZ(gpMarioPos->x, gpMarioPos->z, &marioDir);
	if (marioDir.x != 0.0f || marioDir.z != 0.0f)
		MsVECNormalize((Vec*)&marioDir, (Vec*)&marioDir);

	f32 side = (forward.z * (dir.x - forward.x)
	            - forward.x * (dir.z - forward.z))
	           * forward.dot(marioDir);
	if (side > 0.0f)
		unk14C += unk148 * (1.0f - fabsf(dot));
	else
		unk14C -= unk148 * (1.0f - fabsf(dot));

	if (dot > 0.0f)
		unk140 += dot * unk138;
	else
		unk140 += dot * unk13C;

	offLiveFlag(LIVE_FLAG_UNK10);
}

// TMapObjPuncher
void TMuddyBoat::calcRootMatrix() { }
void TMuddyBoat::kill()
{
	unk140 = 0.0f;
	unk14C = 0.0f;

	JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
	SMS_EasyEmitParticle(PARTICLE_MS_M_AMIATTACK, &unk170, nullptr, scale);

	if (gpMSound->gateCheck(0x387C))
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x387C, (const Vec*)&mPosition, 0, nullptr, 0, 4);

	MTXCopy(getModel()->getAnmMtx(0), getModel()->getBaseTRMtx());

	unkF8 &= ~0x100;
	onLiveFlag(LIVE_FLAG_UNK10);
	startAnim(1);
	startAnim(2);
	mState = 2;
}
#pragma dont_inline on
f32 TMapObjBase::getObjCollisionHeightOffset() const { return mYOffset; }
#pragma dont_inline off
void TMuddyBoat::bind()
{
	if (checkLiveFlag(LIVE_FLAG_UNK10))
		return;

	JGeometry::TVec3<f32> next = mPosition;
	MtxPtr mtx                  = getModel()->getAnmMtx(0);
	next.x += mtx[0][2] * unk140;
	next.z += mtx[2][2] * unk140;

	s32 cubeNo = gpCubeStream->getInCubeNo(*gpMarioPos);
	if (cubeNo != -1) {
		TCubeStreamInfo* stream
		    = (TCubeStreamInfo*)&(*gpCubeStream->unk14)[cubeNo];
		Mtx streamMtx;
		MsMtxSetXYZRPH(streamMtx, 0.0f, 0.0f, 0.0f,
		               (s16)(stream->unk18.x * 182.04445f),
		               (s16)(stream->unk18.y * 182.04445f),
		               (s16)(stream->unk18.z * 182.04445f));

		f32 dot = 0.0f;
		dot     = mtx[0][2] * streamMtx[0][2] + dot;
		dot     = mtx[2][2] * streamMtx[2][2] + dot;
		unk140 += 0.0001f * (dot * stream->unk40);
	}

	const TBGCheckData* ground;
	f32 groundHeight = gpMap->checkGroundIgnoreWaterSurface(next, &ground);
	if (groundHeight > (mPosition.y - mYOffset) - 100.0f
	    || ground->checkFlag(BG_CHECK_FLAG_ILLEGAL)) {
		next = mPosition;
		kill();
		mLinearVelocity.zero();
		return;
	}

	JGeometry::TVec3<f32> pos;
	pos.set(next.x + mtx[0][2] * unk160, mPosition.y - mYOffset,
	        next.z + mtx[2][2] * unk160);
	TBGWallCheckRecord front(pos, unk158, 4, TBGWallCheckRecord::DONT_MOVE_XZ);
	if (gpMap->isTouchedWallsAndMoveXZ(&front)) {
		TBGCheckData* wall = front.mResultWalls[0];
		f32 z = front.mCenter.z
		        - wall->getNormal().z * (50.0f + front.mRadius);
		f32 y = mPosition.y - getObjCollisionHeightOffset();
		unk170.x = front.mCenter.x
		           - wall->getNormal().x * (50.0f + front.mRadius);
		unk170.y = y + 100.0f;
		unk170.z = z;
		next     = mPosition;
		kill();
		mLinearVelocity.zero();
		return;
	}

	pos.set(next.x - mtx[0][2] * unk164, mPosition.y - mYOffset,
	        next.z - mtx[2][2] * unk164);
	TBGWallCheckRecord rear(pos, unk15C, 4, TBGWallCheckRecord::DONT_MOVE_XZ);
	if (gpMap->isTouchedWallsAndMoveXZ(&rear)) {
		TBGCheckData* wall = rear.mResultWalls[0];
		f32 z = rear.mCenter.z
		        - wall->getNormal().z * (50.0f + rear.mRadius);
		f32 y = mPosition.y - getObjCollisionHeightOffset();
		unk170.x
		    = rear.mCenter.x - wall->getNormal().x * (50.0f + rear.mRadius);
		unk170.y = y + 100.0f;
		unk170.z = z;
		next     = mPosition;
		kill();
		mLinearVelocity.zero();
		return;
	}

	pos.set(next.x, mPosition.y - mYOffset, next.z);
	TBGWallCheckRecord center(pos, unk154, 4,
	                          TBGWallCheckRecord::DONT_MOVE_XZ);
	if (gpMap->isTouchedWallsAndMoveXZ(&center)) {
		TBGCheckData* wall = center.mResultWalls[0];
		f32 z = center.mCenter.z
		        - wall->getNormal().z * (50.0f + center.mRadius);
		f32 y = mPosition.y - getObjCollisionHeightOffset();
		unk170.x = center.mCenter.x
		           - wall->getNormal().x * (50.0f + center.mRadius);
		unk170.y = y + 100.0f;
		unk170.z = z;
		next     = mPosition;
		kill();
		mLinearVelocity.zero();
		return;
	}

	JGeometry::TVec3<f32> displacement = next;
	displacement.sub(mPosition);
	mLinearVelocity = displacement;
}
void TMuddyBoat::control()
{
	TMapObjBase::control();

	if (marioIsOn())
		moveByWater();

	switch (mState) {
	case 1:
		unk140 *= unk144;
		f32 speed = __fabsf(unk140);
		if (gpMSound->gateCheck(0x3080)) {
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(
			    0x3080, (const Vec*)&mPosition, nullptr, speed, 0, 0,
			    nullptr, 0, 4);
		}
		if (unk14C != 0.0f) {
			mRotation.y += unk14C;
			mRotation.y = MsWrap(mRotation.y, 0.0f, 360.0f);
			unk14C *= unk150;
			if (fabsf(unk14C) < 0.0001f)
				unk14C = 0.0f;
		}
		break;
	case 2:
		if (animIsFinished()) {
			mLifeTimer = unk168;
			unkF8 |= 0x100;
			sleep();
			mState = 3;
		}
		break;
	case 3:
		bool timerActive = mLifeTimer > 0;
		if (!timerActive) {
			awake();
			makeObjDead();
			makeObjDefault();
			makeObjAppeared();
			JGeometry::TVec3<f32> scale(mScaling.x * 2.0f,
			                            mScaling.y * 2.0f,
			                            mScaling.z * 3.0f);
			unk170.set(mPosition.x, mPosition.y - mYOffset, mPosition.z);
			emitAndSRT(0xE4, 0, &unk170, mRotation, scale);
			emitAndSRT(0xE6, 0, &unk170, mRotation, scale);
			if (gpMSound->gateCheck(0x387D))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x387D, (const Vec*)&mPosition, 0, nullptr, 0, 4);
			mState = 1;
		}
		break;
	}
}
void TMuddyBoat::calc()
{
	f32 z = mPosition.z;
	f32 y = mPosition.y - mYOffset;
	y += gpMapObjWave->getWaveHeight(mPosition.x, z);
	f32 r = mRotation.y;
	f32 x = mPosition.x;

	MsMtxSetXYZRPH(getModel()->getAnmMtx(0), x, y, z, 0.0f, r, 0.0f);

	TPosition3f scaleMtx;
	scaleMtx.identity();
	PSMTXScale(scaleMtx, mInitialScaling.x, mInitialScaling.y,
	           mInitialScaling.z);
	PSMTXConcat(getModel()->getAnmMtx(0), scaleMtx, getModel()->getAnmMtx(0));

	if (unk140 != 0.0f) {
		unk170.set(mPosition.x, mPosition.y - mYOffset, mPosition.z);
		JGeometry::TVec3<f32> scale(mScaling.x * 3.0f, mScaling.y * 2.0f,
		                            mScaling.z * 3.0f);
		emitAndBindScale(0x1E8, 3, &unk170, scale);
		emitAndBindScale(0x107, 1, &unk170, scale);
		unk16C = 0;
	}
}
u32 TMuddyBoat::getSDLModelFlag() const { return 0; }
void TMuddyBoat::initMapObj()
{
	TMapObjBase::initMapObj();

	unk138 = 0.04f;
	unk144 = 0.998f;
	unk148 = 0.002f;
	unk150 = 0.997f;
	unk13C = 0.01f;
	unk168 = 0x258;

	if (gpMarDirector->mMap == 0x34) {
		unk158 = 126.0f;
		unk154 = 185.0f;
		unk15C = 150.0f;
		unk160 = 170.0f;
		unk164 = 185.0f;
	} else {
		unk158 = 100.0f;
		unk154 = 170.0f;
		unk15C = 150.0f;
		unk160 = 180.0f;
		unk164 = 100.0f;
	}

	unk17C.set(3.0f, 2.0f, 5.0f);
}
TMuddyBoat::TMuddyBoat(const char* name)
    : TMapObjBase(name)
{
	unk138 = 0.0f;
	unk13C = 0.0f;
	unk140 = 0.0f;
	unk144 = 0.0f;
	unk148 = 0.0f;
	unk14C = 0.0f;
	unk150 = 0.0f;
	unk154 = 0.0f;
	unk158 = 0.0f;
	unk15C = 0.0f;
	unk160 = 0.0f;
	unk164 = 0.0f;
	unk168 = 0;
	unk16C = 0;
	unk170.zero();
	unk17C.zero();
}
void TMareFall::calc()
{
	MSound* sound = gpMSound;
	if (sound->gateCheck(0x3007))
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x3007, (const Vec*)&mPosition, 0, nullptr, 0, 4);
	if (gpMSound->gateCheck(0x30A2))
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x30A2, (const Vec*)&fall_upper_pos, 0, nullptr, 0, 4);

	gpMarioParticleManager->emit(0x149, &mPosition, 1, this);
	gpMarioParticleManager->emit(0x14A, &mPosition, 1, this);
}

// TMuddyBoat
void TMareFall::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	SMS_LoadParticle("/scene/mapObj/mareFallSplash.jpa", 0x149);
	SMS_LoadParticle("/scene/mapObj/mareFallSmoke.jpa", 0x14A);
}
void TMareCork::loadAfter()
{
	unk138 = (TCannon*)JDrama::TNameRefGen::getInstance()
	             ->getRootNameRef()
	             ->search("\x96\x43\x91\xE4");
	if (((THitActor*)unk138)->receiveMessage(this, HIT_MESSAGE_TAKE))
		mHeldObject = (TTakeActor*)unk138;

	SMS_LoadParticle("/scene/map/map/ms_mare_gunwat_a.jpa", 0x14C);
	SMS_LoadParticle("/scene/map/map/ms_mare_gunwat_b.jpa", 0x14D);
	SMS_LoadParticle("/scene/map/map/ms_mare_gunwat_c.jpa", 0x14E);

	TMapObjBase::loadAfter();
	unk13C.x = 0.0f;
	unk13C.y = 0.0f;
	unk13C.z = 0.0f;
	initAnmSound();
}

// TMareFall
void TMareCork::moveObject()
{
	if (unk138->isObject() && !unk154) {
		mMActor->setBck("marecork");
		setAnmSound("/scene/mapObj/marecork.bas");
		removeMapCollision();
		unk154 = 1;
	}
}
void TMareCork::calcRootMatrix()
{
	if (unk154) {
		mMActor->getFrameCtrl(0)->checkPass(350.0f);
		if (mMActor->getFrameCtrl(0)->checkPass(250.0f)) {
			unk138->startChorobeiShout();

			gpItemManager->makeShineAppearWithDemo(
			    "\x83\x56\x83\x83\x83\x43\x83\x93\x81\x69\x83\x7B\x83\x58"
			    "\x97\x70\x81\x6A",
			    "\x83\x7B\x83\x58\x83\x56\x83\x83\x83\x43\x83\x93\x83\x4A"
			    "\x83\x81\x83\x89",
			    mPosition.x, mPosition.y, mPosition.z);

			unk148.set(2773.0f, 8618.0f, 7006.0f);
			JPABaseEmitter* emitter = gpMarioParticleManager->emitWithRotate(
			    0x44, &unk148, 0x4000, 0x0D82, 0, 0, nullptr);
			if (emitter) {
				emitter->unk154.set(2.5f, 2.5f, 2.5f);
				emitter->unk174.set(2.5f, 2.5f, 2.5f);
			}
		}
	}

	TMapObjBase::calcRootMatrix();
}
MtxPtr TMareCork::getTakingMtx() { return mMActor->unk4->mNodeMatrices[2]; }
void TMareCork::drawObject(JDrama::TGraphics* graphics)
{
	TLiveActor::drawObject(graphics);

	if (unk154) {
		J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
		if (ctrl->getFrame() > 250.0f) {
			unk148.set(2773.0f, 8618.0f, 7006.0f);
			if (gpMSound->gateCheck(0x30B1))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x30B1, (const Vec*)&unk148, 0, nullptr, 0, 4);

			gpMarioParticleManager->emitAndBindToPosPtr(0x14C, &unk13C, 1,
			                                            this);
			gpMarioParticleManager->emitAndBindToPosPtr(0x14D, &unk13C, 1,
			                                            this);
			gpMarioParticleManager->emitAndBindToPosPtr(0x14E, &unk13C, 1,
			                                            this);
		}
	}
}
BOOL TMareEventPoint::receiveMessage(THitActor* sender, u32 message)
{
	if (message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		if (!gpModelWaterManager->checkFlagBottom4Bits(
		        TMapObjBase::getWaterID(sender), 1)) {
			if (((TMapObjBase*)sender)->getWaterPlane(sender)) {
				if (((TMapObjBase*)sender)->getWaterPlane(sender)->getNormal().y < 0.1f) {
					if (unk68->startEvent()) {
						gpMarioParticleManager->emit(0xE7, &sender->mPosition,
						                             0, nullptr);
						gpMSound->startSoundSet(0x6802, (const Vec*)&mPosition,
						                         0, 0.0f, 0, 0, 4);
					}
					return true;
				}
			}
		}
	}

	return false;
}

// TMareCork
void TMareEventPoint::load(JSUMemoryInputStream& stream)
{
	JDrama::TActor::load(stream);
	initHitActor(0x40000236, 0, 0, 0.0f, 0.0f, 300.0f, 600.0f);
}
TMareEventPoint::~TMareEventPoint() { }
