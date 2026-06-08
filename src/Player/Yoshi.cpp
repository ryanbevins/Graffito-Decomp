
#include <Player/Yoshi.hpp>
#include <Player/MarioMain.hpp>
#include <Player/MarioAccess.hpp>
#include <Map/Map.hpp>
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
#include <Player/ModelWaterManager.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/ModelUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <Player/Tongue.hpp>
#include <Strategic/MirrorActor.hpp>

class TNozzleBase {
public:
	virtual void init();
	virtual s32 getNozzleKind() const;
	virtual s16 getGunAngle();
};

class TWaterGun {
public:
	TNozzleBase* getCurrentNozzle() const;
};

static int YoshiHeadCtrl(J3DNode* node, int param)
{
	Mtx mtx;
	if (param == 0) {
		s16 gunAngle = ((TWaterGun*)SMS_GetMarioWaterGun())
		                   ->getCurrentNozzle()
		                   ->getGunAngle();
		MsMtxSetRotRPH(mtx, 0.0f, 0.0f, SHORTANGLE2DEG(gunAngle));
		MTXConcat(J3DSys::mCurrentMtx, mtx, J3DSys::mCurrentMtx);
	}
	return 1;
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
	u16 footRJoint = *(u16*)((u8*)this + 0x40);
	return mActor->unk4->getAnmMtx(footRJoint);
}

// init - 0x8014FE5C (675 instructions - extremely complex)
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
	// The rest of init is extremely complex (allocations, model setup, etc.)
	// TODO: implement remaining ~600 instructions
}

// initInLoadAfter - 0x8014FD88
void TYoshi::initInLoadAfter()
{
	((TYoshiTongue*)_38)->initInLoadAfter();

	TMirrorActor* mirror = new TMirrorActor("jnt_foot_L");
	mirror->init(mActor->unk4, 4);

	for (int i = 0; i < 2; ++i) {
		TMirrorActor* footMirror = new TMirrorActor("jnt_foot_R");
		footMirror->init(*(J3DModel**)((u8*)this + 0x44 + i * 4), 4);
	}

	mActor->unk4->getModelData()->getJointNodePointer(21)->setCallBack(
	    YoshiHeadCtrl);
}

// thinkBtp - 0x8014FCD8
void TYoshi::thinkBtp(int animIdx) {
	int btpIdx = 4;
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
	default:
		break;
	}

	if (mCurBtpIdx != (u16)btpIdx) {
		mActor->setBtpFromIndex(btpIdx);
		J3DFrameCtrl* ctrl = mActor->getFrameCtrl(3);
		ctrl->setRate(0.0f);
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
	((MAnmSound*)mBckPlayer)->initAnmSound((void*)mAnimFrameRates[id], 1, 0.0f);
}

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

	TMapObjGeneral* fruit     = (TMapObjGeneral*)((TEggYoshi*)egg)->unk150;
	TMapObjGeneral* heldFruit = fruit;
	if (mMario->mHeldObject == fruit) {
		heldFruit->receiveMessage(&mMario->mFloorHitActor, HIT_MESSAGE_UNK8);
		heldFruit->mHolder  = nullptr;
		mMario->mHeldObject = nullptr;
	}
	fruit->receiveMessage(&mMario->mFloorHitActor, HIT_MESSAGE_UNKB);
	doEat(fruit->mActorType);
	mCurJuice = mMaxJuice;
	mEgg      = egg;
	*(s16*)((u8*)this + 0x02) = *(s16*)((u8*)this + 0x04);
	return true;
}

// disappear - 0x8014F94C
bool TYoshi::disappear() {
	u8 state = (u8)mState;
	int active;
	if (state != 0) {
		active = 1;
	} else {
		active = 0;
	}
	if (!active)
		return false;

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

// kill - 0x8014F834
void TYoshi::kill() {
	u8 state = (u8)mState;
	int active;
	if (state != 0) {
		active = 1;
	} else {
		active = 0;
	}
	if (active) {
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
	((MAnmSound*)mBckPlayer)->stop();
	((MAnmSound*)mBckPlayer2)->stop();
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
void TYoshi::thinkAnimation() {
	// TODO: implement - 253 instructions
}

// thinkUpper - 0x8014EF78
void TYoshi::thinkUpper()
{
	if ((u8)mState != MOUNTED)
		return;

	((J3DFrameCtrl*)((u8*)this + 0x5C))->update();

	void* upperAnm = *(void**)((u8*) * (void**)((u8*) * (void**)(
	                                            (u8*)mActor + 0x4)
	                                            + 0x4)
	                           + 0x20);
	upperAnm      = *(void**)((u8*)upperAnm + 0x48);

	int active;
	if (*(u16*)((u8*)_38 + 0x7C) != 0) {
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
				if (*(u8*)((u8*)nozzle + 0x385) == 1) {
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
		if (*(void**)((u8*)upperAnm + 0x58)
		    != *(void**)((u8*)this + 0x54)) {
			*(f32*)((u8*)this + 0x6C) = *(s16*)((u8*)this + 0x62);
			*(f32*)((u8*)this + 0x68) = 1.0f;
			*(s16*)((u8*)this + 0x64)
			    = *(s16*)(*(u32*)((u8*)this + 0x4C) + 0x2);
			*(f32*)((u8*)this + 0x6C) = 0.0f;
			*(void**)((u8*)upperAnm + 0x58) = *(void**)((u8*)this + 0x54);
			((MAnmSound*)mBckPlayer2)
			    ->initAnmSound((void*)mAnimFrameRates[3], 1, 0.0f);
		}
		*(f32*)(*(u32*)((u8*)this + 0x4C) + 0x4)
		    = *(f32*)((u8*)this + 0x6C);
	} else {
		if (*(void**)((u8*)upperAnm + 0x58)
		    == *(void**)((u8*)this + 0x54)) {
			*(f32*)((u8*)this + 0x6C) = *(s16*)((u8*)this + 0x62);
			*(f32*)((u8*)this + 0x68) = 1.0f;
			*(s16*)((u8*)this + 0x64)
			    = *(s16*)(*(u32*)((u8*)this + 0x50) + 0x2);
			*(f32*)((u8*)this + 0x6C) = 0.0f;
			*(void**)((u8*)upperAnm + 0x58) = *(void**)((u8*)this + 0x58);
			((MAnmSound*)mBckPlayer2)
			    ->initAnmSound((void*)mAnimFrameRates[4], 1, 0.0f);
		} else if (*(void**)((u8*)upperAnm + 0x58)
		           == *(void**)((u8*)this + 0x58)) {
			int ended;
			if (*(u8*)((u8*)this + 0x61) & 3) {
				ended = 1;
			} else {
				ended = 0;
			}
			if (ended)
				*(void**)((u8*)upperAnm + 0x58) = nullptr;
		}
		*(f32*)(*(u32*)((u8*)this + 0x50) + 0x4)
		    = *(f32*)((u8*)this + 0x6C);
	}
}

// doSearch - 0x8014EA18
void TYoshi::doSearch() {
	// TODO: implement - 344 instructions
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

		Vec* soundPos = (Vec*)((u8*)_38 + 0xb8);
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
void TYoshi::movement() {
	// TODO: implement - 800+ instructions
}

// calcAnim - 0x8014D6B8
void TYoshi::calcAnim() {
	// TODO: implement - 271 instructions
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
	(*(J3DModel**)((u8*)this + 0x44))->viewCalc();
	(*(J3DModel**)((u8*)this + 0x48))->viewCalc();
	((TYoshiTongue*)_38)->viewCalc();
}

// entry - 0x8014D37C
void TYoshi::entry() {
	// TODO: implement - 175 instructions
}
