
#include <Player/Yoshi.hpp>
#include <Player/MarioMain.hpp>
#include <Player/MarioAccess.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <MSound/MAnmSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSoundBGM.hpp>
#include <System/Application.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarioGamePad.hpp>
#include <System/MarDirector.hpp>
#include <MoveBG/Item.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorAnm.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <Player/ModelWaterManager.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/ModelUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <MarioUtil/ShadowUtil.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DShape.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DTransform.hpp>
#include <JSystem/J3D/J3DGraphBase/Components/J3DGXColorS10.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DAnmLoader.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <Player/Tongue.hpp>
#include <Strategic/MirrorActor.hpp>
#include <Strategic/question.hpp>
#include <dolphin/os/OSCache.h>
#include <stdlib.h>

class TNozzleBase {
public:
	u8 _0[0x364];
	virtual void init();
	virtual s32 getNozzleKind() const;
	virtual s16 getGunAngle();
};

class TWaterGun {
public:
	TNozzleBase* getCurrentNozzle() const;
};

extern "C" void* __vt__10TTakeActor[];
extern "C" void* __vt__12TYoshiTongue[];

static const char cDirtyFileName[] = "/scene/map/pollution/H_ma_rak.bti";
static const char cDirtyTexName[]  = "H_ma_rak_dummy";
static const GXColor bodyColor[]   = {
	{ 0x40, 0xa1, 0x24, 0xff },
	{ 0xff, 0x8c, 0x1c, 0xff },
	{ 0xaa, 0x4c, 0xff, 0xff },
	{ 0xff, 0xa0, 0xbe, 0xff },
};

static BOOL YoshiHeadCtrl(J3DNode* node, int param)
{
	if (param == 0) {
		s16 gunAngle = ((TWaterGun*)SMS_GetMarioWaterGun())
		                   ->getCurrentNozzle()
		                   ->getGunAngle();
		Mtx mtx;
		MsMtxSetRotRPH(mtx, 0.0f, 0.0f, SHORTANGLE2DEG(gunAngle));
		MTXConcat(J3DSys::mCurrentMtx, mtx, J3DSys::mCurrentMtx);
	}
	return TRUE;
}

// Reverse address order for -inline deferred

// getFrameCtrl - 0x80150920
J3DFrameCtrl* TYoshi::getFrameCtrl() const {
	return mActor->getFrameCtrl(0);
}

// getMtxPtrFootL - 0x80150904
MtxPtr TYoshi::getMtxPtrFootL() const {
	return mActor->unk4->getAnmMtx(_3e);
}

// getMtxPtrFootR - 0x801508E8
MtxPtr TYoshi::getMtxPtrFootR() const {
	return mActor->unk4->getAnmMtx(mJointFootR);
}

// init - 0x8014FE5C
void TYoshi::init(TMario* mario) {
	mMario = mario;
	mState = EGG;
	*(u8*)((u8*)this + 0x01) = 0;
	s16 unk06 = *(s16*)((u8*)this + 0x06);
	mSubState = unk06;
	*(s16*)((u8*)this + 0x04) = 7200;
	*(s16*)((u8*)this + 0x06) = 7200;
	mMaxJuice = 21300;
	mCurJuice = mMaxJuice;
	mTranslation.x = 0.0f;
	mTranslation.y = 0.0f;
	*(f32*)((u8*)this + 0x28) = 0.0f;
	*(f32*)((u8*)this + 0x2C) = 0.0f;
	*(s16*)((u8*)this + 0x70) = 0;
	*(s16*)((u8*)this + 0x72) = 384;

	MActorAnmData* anmData = new MActorAnmData();
	*(MActorAnmData**)((u8*)this + 0x30) = anmData;
	anmData->init("yoshi", 0);

	mActor = new MActor(anmData);
	void* res = JKRFileLoader::getGlbResource("/yoshi/yoshi_model.bmd");
	J3DModelData* modelData = J3DModelLoaderDataBase::load(res, 0x10040000);
	J3DModel* model = new J3DModel(modelData, 0, 1);
	mActor->setModel(model, 0);

	if (mActor->unkC)
		mActor->unkC->initNormalMotionBlend();
	mActor->offMakeDL();

	JUTNameTab* jointName = mActor->unk4->getModelData()->getJointName();
	mJoint = jointName->getIndex("null_tongue");
	_3e = jointName->getIndex("jnt_foot_L");
	mJointFootR = jointName->getIndex("jnt_foot_R");
	mJointCenter = jointName->getIndex("center");

	J3DModel* handL = SMS_CreatePartsModel("/yoshi/yoshi_hand2_l.bmd", 0x10040000);
	J3DModel* handR = SMS_CreatePartsModel("/yoshi/yoshi_hand2_r.bmd", 0x10040000);
	mHandL = handL;
	mHandR = handR;

	{
		u8* dst = *(u8**)((u8*)handL->getModelData()->getTexture() + 4);
		u8* src
		    = *(u8**)((u8*)mActor->unk4->getModelData()->getTexture() + 4);
		for (int j = 0; j < 8; ++j)
			((u32*)dst)[j] = ((u32*)src)[j];
		*(u32*)(dst + 0x1C) = (u32)((src + *(u32*)(dst + 0x1C)) - dst);
		*(u32*)(dst + 0x0C) = (u32)((src + *(u32*)(dst + 0x0C)) - dst);
		DCFlushRange(dst, 0x20);
	}
	{
		u8* dst = *(u8**)((u8*)handR->getModelData()->getTexture() + 4);
		u8* src
		    = *(u8**)((u8*)mActor->unk4->getModelData()->getTexture() + 4);
		for (int j = 0; j < 8; ++j)
			((u32*)dst)[j] = ((u32*)src)[j];
		*(u32*)(dst + 0x1C) = (u32)((src + *(u32*)(dst + 0x1C)) - dst);
		*(u32*)(dst + 0x0C) = (u32)((src + *(u32*)(dst + 0x0C)) - dst);
		DCFlushRange(dst, 0x20);
	}

	*(J3DAnmTransform**)((u8*)this + 0x4C)
	    = (J3DAnmTransform*)J3DAnmLoaderDataBase::load(
	        JKRFileLoader::getGlbResource("/yoshi/yoshi_eat.bck"));
	*(J3DMtxCalc**)((u8*)this + 0x54) = J3DNewMtxCalcAnm(
	    mActor->unk4->getModelData()->getUnkC(),
	    *(J3DAnmTransform**)((u8*)this + 0x4C));
	*(J3DAnmTransform**)((u8*)this + 0x50)
	    = (J3DAnmTransform*)J3DAnmLoaderDataBase::load(
	        JKRFileLoader::getGlbResource("/yoshi/yoshi_eat_end.bck"));
	*(J3DMtxCalc**)((u8*)this + 0x58) = J3DNewMtxCalcAnm(
	    mActor->unk4->getModelData()->getUnkC(),
	    *(J3DAnmTransform**)((u8*)this + 0x50));

	*(f32*)((u8*)this + 0x6C) = (f32)*(s16*)((u8*)this + 0x62);
	*(f32*)((u8*)this + 0x68) = 1.0f;
	*(u8*)((u8*)this + 0x60) = 0;
	*(f32*)((u8*)this + 0x68) = 0.0f;

	TYoshiTongue* tongue = (TYoshiTongue*)::operator new(sizeof(TYoshiTongue));
	if (tongue) {
		new ((THitActor*)tongue) THitActor("HitActor");
		*(void**)tongue = __vt__10TTakeActor;
		*(void**)((u8*)tongue + 0x20) = (u8*)__vt__10TTakeActor + 0x24;
		*(u32*)((u8*)tongue + 0x68) = 0;
		*(u32*)((u8*)tongue + 0x6C) = 0;
		*(void**)tongue = __vt__12TYoshiTongue;
		*(void**)((u8*)tongue + 0x20) = (u8*)__vt__12TYoshiTongue + 0x24;
	}
	mTongue = tongue;
	tongue->init(this);

	*(f32*)((u8*)this + 0x80) = 0.01f;
	mRedComponent = 0x40;
	mGreenComponent = 0xA1;
	mBlueComponent = 0x24;
	*(f32*)((u8*)this + 0x90) = 160.0f;

	TRidingInfo* ridingInfo = new TRidingInfo;
	ridingInfo->unk0 = 0;
	ridingInfo->localPos.x = 0.0f;
	ridingInfo->localPos.y = 0.0f;
	ridingInfo->localPos.z = 0.0f;
	ridingInfo->unk10 = 0.0f;
	*(TRidingInfo**)((u8*)this + 0x94) = ridingInfo;

	*(f32*)((u8*)this + 0x98) = 20.0f;
	*(f32*)((u8*)this + 0x9C) = 30.0f;
	*(f32*)((u8*)this + 0xA0) = 0.1f;
	*(f32*)((u8*)this + 0xA4) = 0.045f;
	*(J3DDrawBuffer**)((u8*)this + 0xA8) = new J3DDrawBuffer(0x20);
	*(J3DDrawBuffer**)((u8*)this + 0xAC) = new J3DDrawBuffer(0x20);
	*(u32*)((u8*)this + 0xB0) = 0;
	*(u32*)((u8*)this + 0xB4) = 0;
	(*(J3DDrawBuffer**)((u8*)this + 0xA8))->frameInit();
	(*(J3DDrawBuffer**)((u8*)this + 0xAC))->frameInit();

	mFlutterState = 2;
	mFlutterTimer = 0;
	mMaxFlutterTimer = 120;
	mMaxVSpdStartFlutter = -5.0f;
	mFlutterAcceleration = 1.2f;
	mType = GREEN;
	*(u32*)((u8*)this + 0xD8) = 2000;
	*(u32*)((u8*)this + 0xD4) = *(u32*)((u8*)this + 0xD8);
	*(u8*)((u8*)this + 0xDC) = 0;
	*(u16*)((u8*)this + 0xDE) = 0;
	*(u16*)((u8*)this + 0xE0) = 0;
	*(f32*)((u8*)this + 0xE4) = 0.1f;
	*(u16*)((u8*)this + 0xE8) = 120;
	*(u16*)((u8*)this + 0xEA) = 240;
	*(f32*)((u8*)this + 0xEC) = 4.0f;
	mCurBtpIdx = 4;
	mEgg = 0;

	mEmitJoint = jointName->getIndex("jnt_mouth");
	mFootLJoint2 = jointName->getIndex("jnt_neck_2");
	mMtxTrans.zero();
	mMtxTrans2.zero();
	mSpineScale = 80.0f;

	mActor->getFrameCtrl(0)->setRate(0.5f);
	mActor->getFrameCtrl(3)->setRate(0.5f);
	mActor->unk4->setBaseTRMtx(mMario->mModel->getModel()->getBaseTRMtx());
	mActor->unk4->calc();

	modelData = mActor->unk4->getModelData();
	for (u16 i = 0; i < modelData->getShapeNum(); ++i)
		modelData->getShapeNodePointer(i)->onFlag(1);

	PSMTXCopy(mActor->unk4->getAnmMtx(37), handL->getBaseTRMtx());
	PSMTXCopy(mActor->unk4->getAnmMtx(32), handR->getBaseTRMtx());
	handL->calc();
	handR->calc();

	for (u16 i = 0; i < handL->getModelData()->getShapeNum(); ++i)
		handL->getModelData()->getShapeNodePointer(i)->onFlag(1);
	for (u16 i = 0; i < handR->getModelData()->getShapeNum(); ++i)
		handR->getModelData()->getShapeNodePointer(i)->onFlag(1);

	mBckPlayer = new MAnmSound(gpMSound);
	mBckPlayer->initAnmSound(0, 1, 0.0f);
	mBckPlayer2 = new MAnmSound(gpMSound);
	mBckPlayer2->initAnmSound(0, 1, 0.0f);

	mAnimFrameRates = new void*[26];
	for (int i = 0; i < 26; ++i)
		mAnimFrameRates[i] = 0;
	mAnimFrameRates[0]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_born.bas");
	mAnimFrameRates[1]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_damage.bas");
	mAnimFrameRates[2] = JKRFileLoader::getGlbResource(
	    "/yoshi/bas/yoshi_demo_shine_get.bas");
	mAnimFrameRates[3]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_eat.bas");
	mAnimFrameRates[4]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_eat_end.bas");
	mAnimFrameRates[5]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_goff.bas");
	mAnimFrameRates[6]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_hip_end.bas");
	mAnimFrameRates[8]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_hip_start.bas");
	mAnimFrameRates[9]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_hold_jump.bas");
	mAnimFrameRates[10]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_jump.bas");
	mAnimFrameRates[11]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_jump_end.bas");
	mAnimFrameRates[13]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_pivot.bas");
	mAnimFrameRates[14]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_ride.bas");
	mAnimFrameRates[15]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_run.bas");
	mAnimFrameRates[16]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_sidewalk_l.bas");
	mAnimFrameRates[17]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_sidewalk_r.bas");
	mAnimFrameRates[18]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_slide_end.bas");
	mAnimFrameRates[22]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_wait.bas");
	mAnimFrameRates[23]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_wait_alone.bas");
	mAnimFrameRates[24]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_walk.bas");
	mAnimFrameRates[25]
	    = JKRFileLoader::getGlbResource("/yoshi/bas/yoshi_water_die.bas");

	changeAnimation(23);
}

// initInLoadAfter - 0x8014FD88
void TYoshi::initInLoadAfter()
{
	mTongue->initInLoadAfter();

	TMirrorActor* mirror = new TMirrorActor("ヨッシーin鏡");
	mirror->init(mActor->getModel(), 4);

	for (int i = 0; i < 2; ++i) {
		TMirrorActor* footMirror = new TMirrorActor("ヨッシー手in鏡");
		footMirror->init((&mHandL)[i], 4);
	}

	mActor->getModel()->getModelData()->getJointNodePointer(21)->setCallBack(
	    YoshiHeadCtrl);
}

// thinkBtp - 0x8014FCD8
void TYoshi::thinkBtp(int animIdx) {
	u16 btpIdx = 4;
	switch (animIdx) {
	case 0:
		btpIdx = 0;
		break;
	case 1:
		btpIdx = 1;
		break;
	case 9:
		btpIdx = 2;
		break;
	case 25:
		btpIdx = 3;
		break;
	}

	if (mCurBtpIdx != btpIdx) {
		mActor->setBtpFromIndex(btpIdx);
		J3DFrameCtrl* ctrl = mActor->getFrameCtrl(3);
		ctrl->setRate(0.5f);
		mCurBtpIdx = btpIdx;
	}
}

// changeAnimation - 0x8014FC3C
void TYoshi::changeAnimation(int id) {
	int curAnm = mActor->getCurAnmIdx(0);
	if (id == curAnm)
		return;

	MActor* actor = mActor;
	if (!actor->checkCurBckFromIndex(id)) {
		actor->setBckFromIndex(id);
	}
	thinkBtp(id);
	mBckPlayer->initAnmSound(mAnimFrameRates[id], 1, 0.0f);
}

#pragma dont_inline on
// getEmitPosDir - 0x8014FBF0
void TYoshi::getEmitPosDir(JGeometry::TVec3<f32>* pos, JGeometry::TVec3<f32>* dir) const {
	J3DModel* model = mActor->unk4;
	MtxPtr mtx = model->getAnmMtx(mJoint);
	dir->x = mtx[0][0];
	dir->y = mtx[1][0];
	dir->z = mtx[2][0];
	pos->x = mtx[0][3];
	pos->y = mtx[1][3];
	pos->z = mtx[2][3];
}
#pragma dont_inline off

// setEggYoshiPtr - 0x8014FBE8
void TYoshi::setEggYoshiPtr(TEggYoshi* egg) {
	mEgg = egg;
}

// appearFromEgg - 0x8014FA60
bool TYoshi::appearFromEgg(const JGeometry::TVec3<f32>& pos, f32 angle, TEggYoshi* egg) {
	*(JGeometry::TVec3<f32>*)((u8*)this + 0x14) = pos;
	mTranslation                                  = pos;
	mTranslation.y += 1.0f;
	*(s16*)((u8*)this + 0x70) = angle * (65536.0f / 360.0f);
	mState                    = (State)2;

	changeAnimation(0);

	TTakeActor* fruit = (TTakeActor*)egg->getFruit();
	if (mMario->getHeldObject() == fruit) {
		fruit->receiveMessage(mMario->getFloorHitActor(), HIT_MESSAGE_UNK8);
		fruit->mHolder      = nullptr;
		mMario->mHeldObject = nullptr;
	}
	fruit->receiveMessage(mMario->getFloorHitActor(), HIT_MESSAGE_UNKB);
	doEat(fruit->getActorType());
	mCurJuice = mMaxJuice;
	mEgg      = egg;
	*(s16*)((u8*)this + 0x02) = *(s16*)((u8*)this + 0x04);
	return true;
}

// disappear - 0x8014F94C
bool TYoshi::disappear() {
	u8 state = (u8)mState;
	if (isHatched()) {
		if (state == MOUNTED) {
			mMario->getOffYoshi(true);
		}
		u32 marioState = mMario->mState;
		u8 inWater;
		if (marioState & 0x00030000) {
			inWater = 1;
		} else {
			inWater = 0;
		}
		if (inWater) {
			mState = DROWNING;
			changeAnimation(25);
		} else {
			mState = DYING;
		}
		mType = 0;
		mSubState = 30;
		return true;
	}
	return false;
}

// kill - 0x8014F834
void TYoshi::kill() {
	u8 state = (u8)mState;
	if (isHatched()) {
		if (state == MOUNTED) {
			mMario->getOffYoshi(true);
		}
		u32 marioState = mMario->mState;
		u8 inWater;
		if (marioState & 0x00030000) {
			inWater = 1;
		} else {
			inWater = 0;
		}
		if (inWater) {
			mState = DROWNING;
			changeAnimation(25);
		} else {
			mState = DYING;
		}
		mType = 0;
		mSubState = 30;
	}
	mBckPlayer->stop();
	mBckPlayer2->stop();
}

// ride - 0x8014F744
void TYoshi::ride() {
	mState = MOUNTED;
	changeAnimation(22);
	gpModelWaterManager->unk5D5F = mType;
	if (gpMSound->gateCheck(0x7921))
		MSoundSESystem::MSoundSE::startSoundActor(0x7921, (Vec*)&mTranslation,
		                                           0, nullptr, 0, 4);
	*(u8*)((u8*)gpMSound + 0x88) = 1;
	MSBgm::setStageBgmYoshiPercussion(true);
	gpMarDirector->fireRideYoshi(this);
}

// getOff - 0x8014F594
void TYoshi::getOff(bool knockedOff) {
	if ((u8)mState != MOUNTED)
		return;

	*(f32*)((u8*)this + 0x2C) = 0.0f;
	mState = UNMOUNTED;
	*(s16*)((u8*)this + 0x02) = *(s16*)((u8*)this + 0x04);

	if ((u8)knockedOff == 1) {
		changeAnimation(1);
		if (gpMSound->gateCheck(0x7918))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x7918, (Vec*)&mTranslation, 0, nullptr, 0, 4);
		SMSRumbleMgr->start(0x15, 0x14, (f32*)nullptr);
	} else {
		changeAnimation(23);
		if (gpMSound->gateCheck(0x7924))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x7924, (Vec*)&mTranslation, 0, nullptr, 0, 4);
	}
	SMS_RideMoveCalcLocalPos(*(TRidingInfo**)((u8*)this + 0x94), mTranslation);
	*(u8*)((u8*)gpMSound + 0x88) = 1;
	MSBgm::setStageBgmYoshiPercussion(false);
}

// thinkAnimation - 0x8014F1A0
void TYoshi::thinkAnimation()
{
	f32 frameRate = mMario->getMotionFrameCtrl().getRate();
	u16 curAnim   = mActor->getCurAnmIdx(0);
	u16 nextAnim  = curAnim;
	u32 action    = mMario->mAction;

	if (action & 0x400) {
		int changed;
		if (curAnim == 0xC) {
			nextAnim = 0xB;
			changed  = 1;
		} else {
			changed = 0;
		}

		if (!changed) {
			nextAnim = 0xF;
			if (action == 0x00800456 || action == 0x0084045D
			    || action == 0x0004045E
			    || ((action & 0x40000) ? true : false)) {
				nextAnim = 0x13;
			}
		}
	} else if (action & 0x800) {
		if (mFlutterState == 1) {
			nextAnim = 9;
		} else if (action == 0x008008A9 && mMario->mActionState < 4) {
			u16 actionState = mMario->mActionState;
			if (actionState < 2) {
				nextAnim = 8;
			} else {
				nextAnim = 7;
			}
		} else if (mMario->mVel.y > 0.0f) {
			nextAnim = 0xA;
		} else {
			nextAnim = 0xC;
		}
	} else if ((action & 0x200)
	           && (action == 0x386 || action == 0x0C00023D
	               || action == 0x0C00023E)) {
		nextAnim = 0x12;
	} else {
		int pumping;
		if (action & 0x8000) {
			pumping = 1;
		} else {
			pumping = 0;
		}

		int pumpSelected = 0;
		if (pumping) {
			if (mMario->mGamePad->mMeaning & 0x2000) {
				E_SIDEWALK_TYPE sideType;
				f32 sideStick;
				mMario->getSideWalkValues(&sideType, &frameRate,
				                           &sideStick);
				switch (sideType) {
				case (E_SIDEWALK_TYPE)0:
					nextAnim = 0x16;
					pumpSelected = 1;
					break;
				case (E_SIDEWALK_TYPE)1:
					nextAnim = 0x10;
					pumpSelected = 1;
					break;
				case (E_SIDEWALK_TYPE)2:
					nextAnim = 0x11;
					pumpSelected = 1;
					break;
				}
			} else if (mMario->mGamePad->mMeaning & 0x400) {
				nextAnim = 0xD;
				pumpSelected = 1;
			}
		}

		if (!pumpSelected) {
			if (mMario->mAction == 0x0080023C) {
				nextAnim = 6;
			} else if (mMario->mAction == 0x1302) {
				nextAnim = 2;
			} else {
				nextAnim = 0x16;
			}
		}
	}

	if (curAnim == 0x18)
		curAnim = 0xF;

	if ((u16)nextAnim == 0x18)
		nextAnim = 0xF;

	if ((u16)nextAnim != curAnim) {
		u16 checkedAnim = nextAnim;
		if (checkedAnim == 0xF) {
			mActor->setBckFromIndex(0x18);
			mActor->setBckFromIndex(0xF);
		} else {
			MActor* actor = mActor;
			if (!actor->checkCurBckFromIndex(checkedAnim))
				actor->setBckFromIndex(checkedAnim);
		}

		thinkBtp((u16)nextAnim);
		mBckPlayer->initAnmSound(mAnimFrameRates[(u16)nextAnim], 1, 0.0f);
	}

	if ((u16)nextAnim == 0xF) {
		f32 blend = (mMario->mForwardVel - *(f32*)((u8*)this + 0x98))
		            / (*(f32*)((u8*)this + 0x9C)
		               - *(f32*)((u8*)this + 0x98));
		if (blend < 0.0f)
			blend = 0.0f;
		if (blend > 1.0f)
			blend = 1.0f;

		if (mActor->unkC != nullptr)
			mActor->unkC->setMotionBlendRatio(1.0f - blend);

		J3DAnmTransform* oldAnm;
		if (mActor->unkC == nullptr) {
			oldAnm = nullptr;
		} else {
			oldAnm = mActor->unkC->getOldMotionBlendAnmPtr();
		}
		*(f32*)((u8*)oldAnm + 0x4) = mActor->getFrameCtrl(0)->getFrame();

		if (mMario->mAction == 0x0004045C) {
			frameRate = mMario->getMotionFrameCtrl().getRate();
		} else {
			frameRate = *(f32*)((u8*)this + 0xA4) * mMario->mForwardVel
			            + *(f32*)((u8*)this + 0xA0);
		}
	} else if (mActor->unkC != nullptr) {
		mActor->unkC->setMotionBlendRatio(0.0f);
	}

	mActor->getFrameCtrl(0)->setRate(frameRate);
}

// thinkUpper - 0x8014EF78
void TYoshi::thinkUpper()
{
	if ((u8)mState != MOUNTED)
		return;

	((J3DFrameCtrl*)((u8*)this + 0x5C))->update();

	J3DJoint* upperAnm = mActor->unk4->getModelData()->getJointNodePointer(18);

	u8 active;
	if (mTongue->mState != TYoshiTongue::STATE_IDLE) {
		active = 1;
	} else {
		active              = 0;
		TWaterGun* waterGun = mMario->mWaterGun;
		if (*(s32*)((u8*)waterGun + 0x1C80) != 0) {
			TNozzleBase* nozzle = waterGun->getCurrentNozzle();
			typedef s32 (*GetNozzleKind)(TNozzleBase*);
			GetNozzleKind getNozzleKind
			    = *(GetNozzleKind*)(*(u32*)((u8*)nozzle + 0x364) + 0xC);
			if (getNozzleKind(nozzle) == 1) {
				nozzle = waterGun->getCurrentNozzle();
				if (*(s8*)((u8*)nozzle + 0x385) == 1) {
					active = 1;
				} else {
					active = 0;
				}
			} else {
				nozzle = waterGun->getCurrentNozzle();
				if (*(f32*)((u8*)nozzle + 0x378) > 0.0f) {
					active = 1;
				} else {
					active = 0;
				}
			}
		}
	}

	if (active) {
		if (upperAnm->getMtxCalc() != *(J3DMtxCalc**)((u8*)this + 0x54)) {
			*(f32*)((u8*)this + 0x6C) = *(s16*)((u8*)this + 0x62);
			*(f32*)((u8*)this + 0x68) = 1.0f;
			*(s16*)((u8*)this + 0x64)
			    = *(s16*)(*(u32*)((u8*)this + 0x4C) + 0x2);
			*(f32*)((u8*)this + 0x6C) = 0.0f;
			upperAnm->setMtxCalc(*(J3DMtxCalc**)((u8*)this + 0x54));
			mBckPlayer2->initAnmSound(mAnimFrameRates[3], 1, 0.0f);
		}
		*(f32*)(*(u32*)((u8*)this + 0x4C) + 0x4)
		    = *(f32*)((u8*)this + 0x6C);
	} else {
		if (upperAnm->getMtxCalc() == *(J3DMtxCalc**)((u8*)this + 0x54)) {
			*(f32*)((u8*)this + 0x6C) = *(s16*)((u8*)this + 0x62);
			*(f32*)((u8*)this + 0x68) = 1.0f;
			*(s16*)((u8*)this + 0x64)
			    = *(s16*)(*(u32*)((u8*)this + 0x50) + 0x2);
			*(f32*)((u8*)this + 0x6C) = 0.0f;
			upperAnm->setMtxCalc(*(J3DMtxCalc**)((u8*)this + 0x58));
			mBckPlayer2->initAnmSound(mAnimFrameRates[4], 1, 0.0f);
		} else if (upperAnm->getMtxCalc() == *(J3DMtxCalc**)((u8*)this + 0x58)) {
			int ended;
			if (*(u8*)((u8*)this + 0x61) & 3) {
				ended = 1;
			} else {
				ended = 0;
			}
			if (ended)
				upperAnm->setMtxCalc(nullptr);
		}
		*(f32*)(*(u32*)((u8*)this + 0x50) + 0x4)
		    = *(f32*)((u8*)this + 0x6C);
	}
}

// doSearch - 0x8014EA18
inline void TYoshi::emitTongue()
{
	JGeometry::TVec3<f32> pos;
	JGeometry::TVec3<f32> dir;
	getEmitPosDir(&pos, &dir);
	JGeometry::TVec3<f32> vel;
	vel.set(mMario->mVel);
	mTongue->emit(pos, dir, vel);

	for (int i = 0; i < 10; ++i) {
		mTongue->movement();
		if (JGeometry::TVec3<f32>(mTongue->mTipPos - pos).length()
		    > mTongueSearchLength)
			break;
	}

	mSearch.mState = 3;
}

inline void TYoshi::thinkEat()
{
	if (mTongue->mActorTypeInMouth != 0) {
		doEat(mTongue->mActorTypeInMouth);
		if (mMario->onYoshi())
			SMSRumbleMgr->start(0x15, 0x14, (f32*)nullptr);
		mTongue->mActorTypeInMouth = 0;
	}
}

void TYoshi::doSearch()
{
	switch (mSearch.mState) {
	case 0: {
		--mSearch.mWait;
		if (mSearch.mWait <= 0) {
			THitActor* target = mTongue->findTarget(false, false);
			if (target != nullptr) {
				JGeometry::TVec3<f32> diff;
				diff.sub(target->mPosition, mTranslation);
				mSearch.mTargetAngle = matan(diff.z, diff.x);
				mSearch.mState       = 1;
				return;
			}
			mSearch.mWait = (s16)((f32)(mSearch.mWaitMax - mSearch.mWaitMin)
			                           * MsRandF()
			                       + (f32)mSearch.mWaitMin);
		}
		return;
	}
	case 1: {
		s16 prev  = mEggRotSpeed;
		s16 delta = (s16)(mSearch.mTurnRate
		                  * (f32)((s16)mSearch.mTargetAngle - prev));
		mEggRotSpeed = prev + delta;
		if (delta > -0x100 && delta < 0x100) {
			emitTongue();
			changeAnimation(3);
		}
		break;
	}
	case 2: {
		if (mTongue->findTarget(false, true) != nullptr) {
			emitTongue();
		} else {
			mSearch.mWait = (s16)((f32)(mSearch.mWaitMax - mSearch.mWaitMin)
			                           * MsRandF()
			                       + (f32)mSearch.mWaitMin);
			mSearch.mState = 0;
		}
		break;
	}
	case 3:
		thinkEat();
		if (mTongue->mState == TYoshiTongue::STATE_IDLE) {
			mSearch.mWait = (s16)((f32)(mSearch.mWaitMax - mSearch.mWaitMin)
			                           * MsRandF()
			                       + (f32)mSearch.mWaitMin);
			mSearch.mState = 0;
			changeAnimation(23);
		}
		break;
	}
}

// doEat - 0x8014E8F4
void TYoshi::doEat(u32 fruitID)
{
	int yoshiType;
	int isFruit = 1;

	switch (fruitID) {
	case 0x40000391:
	case 0x40000392:
		yoshiType = 1;
		break;
	case 0x40000393:
	case 0x40000395:
		yoshiType = 2;
		break;
	case 0x40000390:
	case 0x40000394:
		yoshiType = 3;
		break;
	default:
		isFruit = 0;
		break;
	}

	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x3d, mActor->unk4->getAnmMtx(mEmitJoint), 0, this);

	if (isFruit == 1) {
		mType     = yoshiType;
		mCurJuice = mMaxJuice;
		gpMarioParticleManager->emitAndBindToPosPtr(0x3e, &mMtxTrans2, 0,
		                                            this);

		Vec* soundPos = (Vec*)&mTongue->mTipPos;
		if (gpMSound->gateCheck(0x1947))
			MSoundSESystem::MSoundSE::startSoundActor(0x1947, soundPos, 0,
			                                           nullptr, 0, 4);
	}
}

// thinkHoldOut - 0x8014E794
void TYoshi::thinkHoldOut()
{
	switch (mFlutterState) {
	case 0:
		if (mMario->mVel.y < mMaxVSpdStartFlutter
		    && (mMario->mGamePad->mMeaning & TMarioGamePad::MEANING_0x80))
			mFlutterState = 1;
		break;
	case 1:
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x119, mActor->unk4->getAnmMtx(mEmitJoint), 1, this);

		if (mMario->mVel.y < 0.0f
		    && 0.0f <= mFlutterAcceleration + mMario->mVel.y) {
			if (gpMSound->gateCheck(0x7926))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x7926, (Vec*)&mTranslation, 0, nullptr, 0, 4);
		}

		if (mFlutterTimer != 0) {
			--mFlutterTimer;
			mMario->mVel.y += mFlutterAcceleration;
		} else {
			mFlutterState = 2;
		}

		if (!(mMario->mGamePad->mMeaning & TMarioGamePad::MEANING_0x80))
			mFlutterState = 2;
		break;
	case 2:
		mFlutterTimer = 0;
		break;
	}
}

// movement - 0x8014DAF4
void TYoshi::movement()
{
	if (gpMarDirector->unk124 != 3 && gpMarDirector->unk124 != 4
	    && !gpMarDirector->isTalkModeNow()) {
		if (!mMario->checkStatusType(0x1000) && mCurJuice > 0) {
			--mCurJuice;
		}
	}

	thinkEat();

	switch ((u8)mState) {
	case 2:
		if (mActor->curAnmEndsNext(0, nullptr)) {
			mState = (State)6;
			changeAnimation(23);
			gpMSound->startMarioVoice(0x7919, 1, 1);
		}
		break;
	case 6: {
		if (mActor->curAnmEndsNext(0, nullptr))
			mActor->setBckFromIndex(23);

		f32 angle = mEggRotSpeed * (360.0f / 65536.0f);
		SMS_RideMoveByGroundActor(*(TRidingInfo**)((u8*)this + 0x94),
		                          &mTranslation, &angle);
		mEggRotSpeed = angle * (65536.0f / 360.0f);

		const TBGCheckData* ground;
		JGeometry::TVec3<f32> trans = mTranslation;
		f32 groundY
		    = gpMap->checkGround(trans.x, 200.0f + trans.y, trans.z, &ground);
		mActor->setLightData(ground, mTranslation);

		*(f32*)((u8*)this + 0x2c)
		    -= *(f32*)((u8*)mMario + 0xb18);
		mTranslation.y += *(f32*)((u8*)this + 0x2c);

		if (groundY > mTranslation.y) {
			if (ground->isIllegalData() || ground->isWaterSurface()
			    || ground->isDeathPlane()) {
				if ((u8)mState != 0) {
					if ((u8)mState == 8)
						mMario->getOffYoshi(true);
					if (mMario->mState & 0x30000) {
						mState = (State)3;
						changeAnimation(25);
					} else {
						mState = (State)4;
					}
					mType = 0;
					*(s16*)((u8*)this + 0x02) = 30;
				}
				break;
			}
			mTranslation.y                 = groundY;
			*(f32*)((u8*)this + 0x2c) = 0.0f;
		}

		doSearch();

		if (mCurJuice <= 0 && (u8)mState != 0) {
			if ((u8)mState == 8)
				mMario->getOffYoshi(true);
			if (mMario->mState & 0x30000) {
				mState = (State)3;
				changeAnimation(25);
			} else {
				mState = (State)4;
			}
			mType = 0;
			*(s16*)((u8*)this + 0x02) = 30;
		}
		break;
	}
	case 7: {
		s16 curAngle = mEggRotSpeed;
		s16 target   = mMario->mFaceAngle.y;
		mEggRotSpeed
		    = curAngle + (s16)(*(f32*)((u8*)this + 0xe4)
		                       * (s16)(target - curAngle));
		break;
	}
	case 8:
		mTranslation              = mMario->mPosition;
		mEggRotSpeed             = mMario->mFaceAngle.y;

		if (mMario->mGamePad->mEnabledFrameMeaning & 0x100) {
			emitTongue();
		}

		if (mCurJuice <= 0 && (u8)mState != 0) {
			if ((u8)mState == 8)
				mMario->getOffYoshi(true);
			if (mMario->mState & 0x30000) {
				mState = (State)3;
				changeAnimation(25);
			} else {
				mState = (State)4;
			}
			mType = 0;
			*(s16*)((u8*)this + 0x02) = 30;
		}
		break;
	case 1:
		--*(s16*)((u8*)this + 0x02);
		if (*(s16*)((u8*)this + 0x02) <= 0) {
			mState = (State)6;
			changeAnimation(23);
			*(s16*)((u8*)this + 0x02) = *(s16*)((u8*)this + 0x04);
		}
		break;
	case 3:
		if (mActor->getFrameCtrl(0)->checkPass(60.0f)) {
			gpMarioParticleManager->emitAndBindToPosPtr(
			    0x3f, (JGeometry::TVec3<f32>*)((u8*)this + 0x74), 0, this);
		}
		if (mActor->curAnmEndsNext(0, nullptr)) {
			mState = (State)5;
			*(s16*)((u8*)this + 0x02) = 30;
		}
		break;
	case 4:
		gpMarioParticleManager->emitAndBindToPosPtr(
		    0x3f, (JGeometry::TVec3<f32>*)((u8*)this + 0x74), 0, this);
		mState = (State)5;
		*(s16*)((u8*)this + 0x02) = 30;
		break;
	case 5:
		mState = EGG;
		if (mEgg != nullptr)
			mEgg->startFruit();
		break;
	}

	if ((u8)mState != 0) {
		f32 blend = *(f32*)((u8*)this + 0x80);
		f32 redBlend = blend * ((f32)bodyColor[mType].r - mRedComponent);
		mRedComponent += redBlend;
		f32 greenBlend
		    = blend * ((f32)bodyColor[mType].g - mGreenComponent);
		mGreenComponent += greenBlend;
		f32 blueBlend = blend * ((f32)bodyColor[mType].b - mBlueComponent);
		mBlueComponent += blueBlend;

		mTongue->movement();

		const TBGCheckData* ground;
		f32 groundY = gpMap->checkGround(mTranslation.x, mTranslation.y + 200.0f,
		                                 mTranslation.z, &ground);
		mActor->setLightData(ground, mTranslation);

		if ((u8)mState == 6 && groundY > mTranslation.y) {
			if (ground->isWaterSurface()) {
				if ((u8)mState != 0) {
					if ((u8)mState == 8)
						mMario->getOffYoshi(true);
					if (mMario->mState & 0x30000) {
						mState = (State)3;
						changeAnimation(25);
					} else {
						mState = (State)4;
					}
					mType = 0;
					*(s16*)((u8*)this + 0x02) = 30;
				}
			} else {
				mTranslation.y                 = groundY;
				*(f32*)((u8*)this + 0x2c) = 0.0f;
			}
		}

		mActor->unk4->getModelData()->offFlag1OnAllShapes();
		mHandL->getModelData()->offFlag1OnAllShapes();
		mHandR->getModelData()->offFlag1OnAllShapes();
	} else {
		mActor->unk4->getModelData()->onFlag1OnAllShapes();
		mHandL->getModelData()->onFlag1OnAllShapes();
		mHandR->getModelData()->onFlag1OnAllShapes();
	}

	if (mMario->onYoshi())
		gpModelWaterManager->unk5D5F = mType;

	MtxPtr mtx = mActor->unk4->getAnmMtx(mJointCenter);
	*(f32*)((u8*)this + 0x74) = mtx[0][3];
	*(f32*)((u8*)this + 0x78) = mtx[1][3];
	*(f32*)((u8*)this + 0x7c) = mtx[2][3];
}

// calcAnim - 0x8014D6B8
void TYoshi::calcAnim()
{
	Mtx rootMtx;
	u8 state = (u8)mState;

	switch (state) {
	case 2:
	case 3:
	case 6:
	case 7:
		J3DGetTranslateRotateMtx(0, *(s16*)((u8*)this + 0x70), 0,
		                         mTranslation.x, mTranslation.y,
		                         mTranslation.z, rootMtx);
		break;
	case 8:
		thinkAnimation();
		PSMTXCopy(mMario->getTakenMtx(), rootMtx);
		break;
	case 1:
		J3DGetTranslateRotateMtx(0, *(s16*)((u8*)this + 0x70), 0,
		                         mTranslation.x,
		                         mTranslation.y
		                             + 100.0f * *(s16*)((u8*)this + 0x02),
		                         mTranslation.z, rootMtx);
		break;
	case 5:
		J3DGetTranslateRotateMtx(0, *(s16*)((u8*)this + 0x70), 0,
		                         mTranslation.x, mTranslation.y,
		                         mTranslation.z, rootMtx);
		break;
	case 0:
	case 4:
		break;
	}

	if (isHatched()) {
		thinkUpper();

		switch (mActor->getCurAnmIdx(0)) {
		case 12:
		case 10:
		case 15:
			mActor->unk4->getModelData()->getShapeNodePointer(0)->onFlag(1);
			mActor->unk4->getModelData()->getShapeNodePointer(1)->onFlag(1);

			mHandL->getModelData()->offFlag1OnAllShapes();
			mHandR->getModelData()->offFlag1OnAllShapes();
			break;
		default:
			mActor->unk4->getModelData()->getShapeNodePointer(0)->offFlag(1);
			mActor->unk4->getModelData()->getShapeNodePointer(1)->offFlag(1);

			mHandL->getModelData()->onFlag1OnAllShapes();
			mHandR->getModelData()->onFlag1OnAllShapes();
			break;
		}

		mActor->unk4->setBaseTRMtx(rootMtx);
		mActor->calcAnm();
		mHandL->setBaseTRMtx(mActor->unk4->getAnmMtx(37));
		mHandR->setBaseTRMtx(mActor->unk4->getAnmMtx(32));
		mHandL->calc();
		mHandR->calc();

		Mtx tongueMtx;
		PSMTXCopy(mActor->unk4->getAnmMtx(mJoint), tongueMtx);
		mTongue->calcAnim(tongueMtx);
	}

	MtxPtr emitMtx = mActor->unk4->getAnmMtx(mEmitJoint);
	mMtxTrans.x    = emitMtx[0][3];
	mMtxTrans.y    = emitMtx[1][3];
	mMtxTrans.z    = emitMtx[2][3];

	MtxPtr footMtx = mActor->unk4->getAnmMtx(mFootLJoint2);
	mMtxTrans2.x   = footMtx[0][3];
	mMtxTrans2.y   = footMtx[1][3];
	mMtxTrans2.z   = footMtx[2][3];

	u32 soundFlags = mMario->mSoundFlags;
	mBckPlayer->animeLoop((Vec*)&mTranslation,
	                      mActor->getFrameCtrl(0)->getFrame(),
	                      mActor->getFrameCtrl(0)->getRate(),
	                      soundFlags + 0x10000000, 4);
	mBckPlayer2->animeLoop((Vec*)&mMtxTrans, *(f32*)((u8*)this + 0x6c),
	                       *(f32*)((u8*)this + 0x68),
	                       soundFlags + 0x10000000, 4);
}

// viewCalc - 0x8014D638
void TYoshi::viewCalc() {
	u8 state = (u8)mState;
	int active;
	if (state == 0) {
		active = 0;
	} else {
		active = 1;
	}
	if (!active)
		return;

	mActor->viewCalc();
	mHandL->viewCalc();
	mHandR->viewCalc();
	mTongue->viewCalc();
}

// entry - 0x8014D37C
void TYoshi::entry()
{
	u8 state = (u8)mState;
	int active;
	if (state == 0) {
		active = 0;
	} else {
		active = 1;
	}
	if (!active)
		return;

	u8 shouldDraw = 1;
	if (state == 6 || state == 8) {
		s32 juice = mCurJuice;
		if (juice >= 0x168 && juice < 0x258) {
			if (!(juice & 0x10))
				shouldDraw = 0;
		}
		if (mCurJuice < 0x168 && !(juice & 0x8))
			shouldDraw = 0;
	}

	if (state == 0)
		shouldDraw = 0;

	if (mCurJuice < 0x258)
		mType = 0;

	if ((u8)mState == 0) {
		active = 0;
	} else {
		active = 1;
	}
	if (!active)
		return;
	if (shouldDraw != 1)
		return;

	J3DModelData* data = mActor->unk4->getModelData();

	J3DGXColorS10 color;
	color.color.r = mRedComponent;
	color.color.g = mGreenComponent;
	color.color.b = mBlueComponent;
	color.color.a = 0xff;

	for (u16 i = 0; i < data->getMaterialNum(); ++i)
		data->getMaterialNodePointer(i)->getTevBlock()->setTevColor(2, color);

	mHandL->getModelData()->getMaterialNodePointer(0)->getTevBlock()->setTevColor(
	    2, color);
	mHandR->getModelData()->getMaterialNodePointer(0)->getTevBlock()->setTevColor(
	    2, color);

	mActor->entry();
	(*(J3DModel**)((u8*)this + 0x44))->entry();
	(*(J3DModel**)((u8*)this + 0x48))->entry();
	mTongue->entry();

	TCircleShadowRequest request;
	request.unk0  = mTranslation;
	f32 spineScale = mSpineScale;
	request.unkC   = spineScale;
	request.unk10  = spineScale;
	gpBindShadowManager->request(request, 0);

	gpQuestionManager->request(mTranslation, mSpineScale);
}
