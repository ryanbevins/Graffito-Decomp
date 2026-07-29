#include <Enemy/BossHanachan.hpp>
#include <Enemy/BossHanachanPartsBase.hpp>
#include <Enemy/BossHanachanSaveParams.hpp>

#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

#include <Camera/cameralib.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorAnm.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/MapUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/ShadowUtil.hpp>
#include <Map/MapCollisionEntry.hpp>
#define MAP_COLLISION_ENTRY_DEFINE_SET_UP_TRANS
#include <Map/MapCollisionEntry.hpp>
#undef MAP_COLLISION_ENTRY_DEFINE_SET_UP_TRANS
#include <Map/MapData.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/ModelWaterManager.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <System/MarDirector.hpp>

extern const char* cMapCollisionJointName;
extern const char* cBodyMapCollisionFileName;
extern const char* cHeadMapCollisionFileName;
extern const char* cLegJointName_L3;
extern const char* cLegJointName_R3;
extern const char* cNoseHallJointName_L;
extern const char* cNoseHallJointName_R;

static const char* sFootJointName[2] = { "foot_L", "foot_R" };

const char* cMapCollisionJointName    = "center";
const char* cBodyMapCollisionFileName = "/scene/bosshanachan/hanabody_col.col";
const char* cHeadMapCollisionFileName = "/scene/bosshanachan/hanahead_col.col";
const char* cLegJointName_L3          = "leg_L3";
const char* cLegJointName_R3          = "leg_R3";
const char* cNoseHallJointName_L      = "L_hall";
const char* cNoseHallJointName_R      = "R_hall";

template s16 CLBPalFrame<s16>(s16);

static inline bool BHPartsIsCurBckDone(MActor* a)
{
	bool result = true;
	if (a == nullptr)
		return result;
	J3DFrameCtrl* fc = a->getFrameCtrl(0);
	if (fc == nullptr)
		return result;
	bool skip = result;
	if (fc->checkState(J3DFrameCtrl::STATE_COMPLETED_ONCE) ? result : false) {
	} else if (fc->checkState(J3DFrameCtrl::STATE_LOOPED_ONCE) ? true : false) {
	} else {
		skip = false;
	}
	if (!skip) {
		if (!(fc->getFrame() + 0.1f >= (f32)fc->getEnd()))
			result = false;
	}
	return result;
}

BOOL TBossHanachanPartsHead::receiveMessage(THitActor* sender, u32 message)
{
	bool inOk   = true;
	bool inTalk = inOk;
	if (gpMarDirector->unk124 != 1 && gpMarDirector->unk124 != 2)
		inTalk = false;
	if (!inTalk) {
		if (gpMarDirector->unk124 != 4)
			inOk = false;
	}
	if (inOk)
		return FALSE;

	BOOL ret                          = FALSE;
	const TNerveBase<TLiveActor>* cur = mOwner->mSpine->getLatestNerve();
	bool isTumble                     = false;
	if (cur == &TNerveBossHanachanTumble::theNerve()
	    || cur == &TNerveBossHanachanDown::theNerve()) {
		if (mActorType == 0x08000015) {
			bool isFlipped = (mRotation.z == 179.0f || mRotation.z == -179.0f) ? true : false;
			if (isFlipped) isTumble = true;
		} else {
			isTumble = true;
		}
	}

	if (isTumble) {
		switch (message) {
		case 0:
			if (mCurAnm == 5) {
				getMActor()->getFrameCtrl(0)->setFrame(0.0f);
			} else {
				setAnm_((EnumBossHanachanAnmKind)5, (EnumBossHanachanStopMotionBlendOnOff)0);
			}
			ret = TRUE;
			break;
		case 1:
			setAnm_((EnumBossHanachanAnmKind)6, (EnumBossHanachanStopMotionBlendOnOff)0);
			mWaterHit->onWaterHitCounter();
			ret = TRUE;
			break;
		default:
			break;
		}
	}
	return ret;
}

BOOL TBossHanachanPartsBody::receiveMessage(THitActor* sender, u32 message)
{
	bool inOk   = true;
	bool inTalk = inOk;
	if (gpMarDirector->unk124 != 1 && gpMarDirector->unk124 != 2)
		inTalk = false;
	if (!inTalk) {
		if (gpMarDirector->unk124 != 4)
			inOk = false;
	}
	if (inOk)
		return FALSE;

	BOOL ret                          = FALSE;
	const TNerveBase<TLiveActor>* cur = mOwner->mSpine->getLatestNerve();
	bool isTumble                     = false;
	if (cur == &TNerveBossHanachanTumble::theNerve()
	    || cur == &TNerveBossHanachanDown::theNerve()) {
		if (mActorType == 0x08000015) {
			bool isFlipped = (mRotation.z == 179.0f || mRotation.z == -179.0f) ? true : false;
			if (isFlipped) isTumble = true;
		} else {
			isTumble = true;
		}
	}

	if (isTumble) {
		switch (message) {
		case 0:
			if (mCurAnm == 5) {
				getMActor()->getFrameCtrl(0)->setFrame(0.0f);
			} else {
				setAnm_((EnumBossHanachanAnmKind)5, (EnumBossHanachanStopMotionBlendOnOff)0);
			}
			ret = TRUE;
			break;
		case 1: {
			bool same = (unk114 == mOwner->unk174);
			if (mCurAnm <= 0x11) {
				switch (mCurAnm) {
				case 0:
				case 2:
				case 5:
				case 13:
				case 15:
				case 16:
				case 17:
					if (same) {
						setAnm_((EnumBossHanachanAnmKind)6, (EnumBossHanachanStopMotionBlendOnOff)0);
						mOwner->execDamage();
					} else if (mCurAnm == 13) {
						getMActor()->getFrameCtrl(0)->setFrame(0.0f);
					} else {
						setAnm_((EnumBossHanachanAnmKind)13, (EnumBossHanachanStopMotionBlendOnOff)0);
					}
					ret = TRUE;
					break;
				default:
					break;
				}
			}
		} break;
		default:
			break;
		}
	}
	return ret;
}

BOOL TBossHanachanPartsBody::setAnm_(
    EnumBossHanachanAnmKind anmKind,
    EnumBossHanachanStopMotionBlendOnOff stopMotionBlend)
{
	static const int sBodyBckIndex[18] = { 0x13, 0x0F, 0x0A, 0x0D, 0x00, 0x0C,
		                                   0x09, 0x02, 0x03, 0x04, 0x05, 0x06,
		                                   0x07, 0x08, 0x10, 0x01, 0x11, 0x12 };

	BOOL ret = FALSE;
	if (mCurAnm != anmKind) {
		mPrevAnm = mCurAnm;
		mCurAnm  = anmKind;

		if (getMActor()->getCurAnmIdx(MActor::ANM_TYPE_BCK)
		    != sBodyBckIndex[anmKind]) {
			int idx = sBodyBckIndex[anmKind];
			if (unk114 == mOwner->unk174) {
				switch (anmKind) {
				case 2:
					idx = 0xB;
					break;
				case 3:
					idx = 0xE;
					break;
				default:
					break;
				}
			}
			getMActor()->setBckFromIndex(idx);
			ret = TRUE;
			if (stopMotionBlend == 1) {
				mPalFrame->unk24 = mPalFrame->mFrame;
			} else {
				mPalFrame->unk24 = 0;
			}
			setCurAnmSound();
		}

		if (anmKind == 0xF) {
			getMActor()->setBrkFromIndex(0);
			getMActor()->getFrameCtrl(5)->setAttribute(0);
			getMActor()->getModel()->unlock();
		}
	}
	return ret;
}

BOOL TBossHanachanPartsHead::setAnm_(
    EnumBossHanachanAnmKind anmKind,
    EnumBossHanachanStopMotionBlendOnOff stopMotionBlend)
{
	static const int sHeadBckIndex[18] = { 0x24, 0x20, 0x1D, 0x1F, 0x14, 0x1E,
		                                   0x1C, 0x16, 0x17, 0x18, 0x19, 0x1A,
		                                   0x1B, 0x1C, 0x21, 0x15, 0x22, 0x23 };
	static const int sHeadBtpIndex[18] = { 0, 0, 1, 2, 1, 2, 2, 0, 0,
		                                   0, 0, 0, 0, 2, 0, 0, 1, 1 };
	static const int sHeadBtkIndex[18] = { 0, 0, 0, 1, 0, 1, 1, 0, 0,
		                                   0, 0, 0, 0, 0, 0, 0, 0, 0 };

	BOOL ret = FALSE;
	if (mCurAnm != anmKind) {
		mPrevAnm = mCurAnm;
		mCurAnm  = anmKind;

		if (getMActor()->getCurAnmIdx(MActor::ANM_TYPE_BCK)
		    != sHeadBckIndex[anmKind]) {
			getMActor()->setBckFromIndex(sHeadBckIndex[anmKind]);
			ret = TRUE;
			if (stopMotionBlend == 1) {
				mPalFrame->unk24 = mPalFrame->mFrame;
			} else {
				mPalFrame->unk24 = 0;
			}
			setCurAnmSound();
		}

		if (getMActor()->getCurAnmIdx(MActor::ANM_TYPE_BTP)
		    != sHeadBtpIndex[anmKind]) {
			getMActor()->setBtpFromIndex(sHeadBtpIndex[anmKind]);
		}
		if (getMActor()->getCurAnmIdx(MActor::ANM_TYPE_BTK)
		    != sHeadBtkIndex[anmKind]) {
			getMActor()->setBtkFromIndex(sHeadBtkIndex[anmKind]);
		}

		if (anmKind == 0xF) {
			getMActor()->setBrkFromIndex(1);
			getMActor()->getFrameCtrl(5)->setAttribute(0);
		}
	}
	return ret;
}

void TBossHanachanPartsBase::considerSetAnm_(
    EnumBossHanachanNerveAnm nerveAnm)
{
	if (nerveAnm == 0) {
		switch (mCurAnm) {
		case 5:
		case 6:
		case 0xD:
		case 0x10:
		case 0x11:
			break;
		default:
			return;
		}
		if (BHPartsIsCurBckDone(getMActor()))
			setAnm_((EnumBossHanachanAnmKind)3, (EnumBossHanachanStopMotionBlendOnOff)0);
		return;
	}

	if (nerveAnm == 1) {
		bool hipDropping = false;
		if (SMS_IsMarioTouchGround4cm()) {
			const TBGCheckData* plane = SMS_GetMarioGroundPlane();
			if (plane != nullptr && plane->mActor == (TLiveActor*)this) {
				hipDropping = true;
			}
		}
		bool inGetUp = false;
		switch (mCurAnm) {
		case 5:
		case 6:
		case 0xD:
		case 0x10:
		case 0x11:
			inGetUp = true;
			break;
		}

		if (inGetUp) {
			if (!BHPartsIsCurBckDone(getMActor()))
				return;
			if (mActorType == 0x08000015 && hipDropping) {
				setAnm_((EnumBossHanachanAnmKind)2, (EnumBossHanachanStopMotionBlendOnOff)1);
			} else {
				setAnm_((EnumBossHanachanAnmKind)3, (EnumBossHanachanStopMotionBlendOnOff)0);
			}
			return;
		}

		if (mActorType != 0x08000015)
			return;

		bool fastAnm = false;
		if (mPalFrame->unk24 > 0)
			fastAnm = true;
		if (!fastAnm) {
			if (mPalFrame->unk28 > 0.0f)
				fastAnm = true;
		}
		if (fastAnm)
			return;

		if (mCurAnm == 2) {
			if (!hipDropping) {
				setAnm_((EnumBossHanachanAnmKind)3,
				        (EnumBossHanachanStopMotionBlendOnOff)1);
			}
		} else if (hipDropping) {
			setAnm_((EnumBossHanachanAnmKind)2,
			        (EnumBossHanachanStopMotionBlendOnOff)1);
		}
		return;
	}

	if (nerveAnm == 2) {
		if (mAnmCounter > 0)
			mAnmCounter -= 1;
		if (mAnmCounter != 0)
			return;
		bool done = BHPartsIsCurBckDone(getMActor());

		switch (mCurAnm) {
		case 7:
			if (done)
				setAnm_((EnumBossHanachanAnmKind)8, (EnumBossHanachanStopMotionBlendOnOff)0);
			return;
		case 8:
			if (done)
				setAnm_((EnumBossHanachanAnmKind)9, (EnumBossHanachanStopMotionBlendOnOff)0);
			return;
		case 10:
			if (done)
				setAnm_((EnumBossHanachanAnmKind)0xB, (EnumBossHanachanStopMotionBlendOnOff)0);
			return;
		case 0xC:
			return;
		case 0xB:
			if (done)
				setAnm_((EnumBossHanachanAnmKind)0xC, (EnumBossHanachanStopMotionBlendOnOff)0);
			return;
		default:
			break;
		}
		if (mRotation.z < 0.0f) {
			setAnm_((EnumBossHanachanAnmKind)7, (EnumBossHanachanStopMotionBlendOnOff)1);
		} else {
			setAnm_((EnumBossHanachanAnmKind)0xA, (EnumBossHanachanStopMotionBlendOnOff)1);
		}
		return;
	}

	if (nerveAnm == 3) {
		if (mAnmCounter > 0) {
			mAnmCounter -= 1;
			if (mAnmCounter == 0)
				setAnm_((EnumBossHanachanAnmKind)6, (EnumBossHanachanStopMotionBlendOnOff)0);
			return;
		}
		if (mCurAnm != 6)
			return;
		if (BHPartsIsCurBckDone(getMActor()))
			setAnm_((EnumBossHanachanAnmKind)4, (EnumBossHanachanStopMotionBlendOnOff)0);
		return;
	}

	if (nerveAnm == 4) {
		if (mAnmCounter > 0)
			mAnmCounter -= 1;
		if (mAnmCounter != 0)
			return;
		if (mCurAnm == 0xE)
			return;
		setAnm_((EnumBossHanachanAnmKind)0xE, (EnumBossHanachanStopMotionBlendOnOff)1);
		return;
	}

	if (nerveAnm == 5) {
		if (mAnmCounter > 0)
			mAnmCounter -= 1;
		if (mAnmCounter != 0)
			return;
		setAnm_((EnumBossHanachanAnmKind)0xF, (EnumBossHanachanStopMotionBlendOnOff)0);
	}
}

void TBossHanachanPartsBase::calcRotateZWhenGetUp_()
{
	if (mAnmCounter != 0)
		return;
	switch (mCurAnm) {
	case 8:
	case 0xB:
		break;
	default:
		return;
	}

	J3DFrameCtrl* fc = getMActor()->getFrameCtrl(0);
	f32 remain       = ((f32)fc->getEnd() - fc->getFrame()) * 2.0f;
	if (remain < 0.001f) {
		mRotation.z = 0.0f;
	} else {
		CLBChaseConstantSpecifyFrame<f32>(&mRotation.z, 0.0f, remain);
	}
}

const TLiveActor* TBossHanachanPartsBase::getSandActor_() const
{
	const TLiveActor* a = SMS_GetGroundActor(mGroundPlane, 0x400000CD);
	if (a == nullptr) {
		a = SMS_GetGroundActor(mGroundPlane, 0x400000CB);
	}
	return a;
}

void TBossHanachanPartsBase::copyFrameFromOldAnmToNewAnm_()
{
	J3DAnmBase* old = nullptr;
	if (getMActor()->unkC != nullptr)
		old = (J3DAnmBase*)getMActor()->unkC->unk24;

	J3DFrameCtrl* newFc = getMActor()->getFrameCtrl(0);
	if (old == nullptr || newFc == nullptr)
		return;

	f32 frame;
	if (getMActor()->unkC == nullptr) {
		frame = 0.0f;
	} else {
		frame = getMActor()->unkC->getOldMotionBlendFrame();
	}
	old->setFrame(frame);
	newFc->setFrame(frame);
}

bool TBossHanachanPartsBase::isCurBckAlreadyEnd_() const
{
	return BHPartsIsCurBckDone(getMActor());
}

void TBossHanachanPartsBase::setDamageFog_(JDrama::TGraphics* graphics)
{
	bool isBody = true;
	if (mActorType == 0x08000014)
		isBody = false;

	J3DModelData* modelData = getMActor()->getModel()->mModelData;
	u16 matCount            = modelData->mMaterialNum;
	JGeometry::TVec3<f32> pos(mCenterJointMtx[0][3], mCenterJointMtx[1][3],
	                          mCenterJointMtx[2][3]);

	const TNerveBase<TLiveActor>* cur = mOwner->mSpine->getLatestNerve();
	if (cur == &TNerveBossHanachanDamage::theNerve()) {
		SMS_AddDamageFogEffect(modelData, pos, graphics);
		if (isBody) {
			for (u16 i = 0; i < matCount; ++i) {
				modelData->mMaterials[i]->change();
			}
		}
		if (mAnmCounter == 0) {
			getMActor()->getModel()->unlock();
		}
	} else {
		SMS_ResetDamageFogEffect(modelData);
	}
}

void TBossHanachanPartsBase::entryCircleShadow_()
{
	const TNerveBase<TLiveActor>* cur = mOwner->mSpine->getCurrentNerve();
	if (cur == &TNerveBossHanachanDead::theNerve()
	    && mOwner->mSpine->getTime() > 0xC8) {
		return;
	}
	TCircleShadowRequest req;
	req.unk0.set(mCenterJointMtx[0][3], mCenterJointMtx[1][3],
	             mCenterJointMtx[2][3]);
	req.unkC = req.unk10 = mScaledBodyRadius;
	u32 actorType = mActorType;
	gpBindShadowManager->forceRequest(req, actorType);
}

void TBossHanachanPartsBase::moveMapCollision_()
{
	JGeometry::TVec3<f32> p(mCenterJointMtx[0][3], mCenterJointMtx[1][3],
	                        mCenterJointMtx[2][3]);
	mMapCollision->moveTrans(p);
}

void TBossHanachanPartsBase::changeTumbleAnmRate_()
{
	J3DFrameCtrl* fc = getMActor()->getFrameCtrl(0);
	switch (mCurAnm) {
	case 0x10:
	case 0x11:
		if (fc->getFrame() > 40.0f) {
			f32 remain = (f32)fc->getEnd() - fc->getFrame();
			f32 rate   = SMSGetAnmFrameRate();
			f32 cur    = fc->getRate();
			CLBChaseConstantSpecifyFrame<f32>(&cur, rate, remain);
			fc->setRate(cur);
		}
		break;
	default:
		fc->setRate(SMSGetAnmFrameRate());
		break;
	}
}

void TBossHanachanPartsBody::initFootHitActor_(TIdxGroupObj* group)
{
	TBossHanachanCommonSaveParams* params = mOwner->mParams;
	MActor* a       = getMActor();
	JUTNameTab* tab = a->getModel()->mModelData->unkB0;
	for (int i = 0; i < 2; ++i) {
		u16 idx         = tab->getIndex(sFootJointName[i]);
		TFootHitActor* foot = new TFootHitActor("ボスハナチャンの足");
		mFeet[i]            = foot;
		foot->initHitActor(0x80000001, idx, 0,
		                   params->mSLFootAttackRadius.value,
		                   params->mSLFootAttackHeight.value,
		                   params->mSLFootDamageRadius.value,
		                   params->mSLFootDamageHeight.value);
		group->add((THitActor*&)mFeet[i]);
		mFeet[i]->unk64 &= ~1;
		MtxPtr m = (MtxPtr)((u8*)a->getModel()->mNodeMatrices + idx * 0x30);
		mFeet[i]->mPosition.x = m[0][3];
		mFeet[i]->mPosition.y = m[1][3];
		mFeet[i]->mPosition.z = m[2][3];
	}
}

void TBossHanachanPartsBase::initMapCollisionAndHitActor_(TIdxGroupObj* group)
{
	const char* filename = nullptr;
	f32 attRad = 0.0f, attHei = 0.0f, damRad = 0.0f, damHei = 0.0f,
	    offY = 0.0f;

	TBossHanachanCommonSaveParams* params = mOwner->mParams;
	if (mActorType == 0x08000015) {
		filename = cBodyMapCollisionFileName;
		attRad   = params->mSLBodyAttackRadius.value;
		attHei   = params->mSLBodyAttackHeight.value;
		damRad   = params->mSLBodyDamageRadius.value;
		damHei   = params->mSLBodyDamageHeight.value;
		offY     = params->mSLBodyHitOffsetY.value;
	} else if (mActorType == 0x08000014) {
		filename = cHeadMapCollisionFileName;
		attRad   = params->mSLHeadAttackRadius.value;
		attHei   = params->mSLHeadAttackHeight.value;
		damRad   = params->mSLHeadDamageRadius.value;
		damHei   = params->mSLHeadDamageHeight.value;
		offY     = params->mSLHeadHitOffsetY.value;
	}

	MActor* a       = getMActor();
	JUTNameTab* tab = a->getModel()->mModelData->unkB0;
	u16 idx         = tab->getIndex(cMapCollisionJointName);
	mCenterJointMtx = (MtxPtr)((u8*)a->getModel()->mNodeMatrices + idx * 0x30);

	mMapCollision = new TMapCollisionMove();
	mMapCollision->init(filename, 0x8000, this);

	mWaterHit = new TWaterHitActor("ボスハナチャンのパーツ");
	mWaterHit->initHitActor(mActorType, 1, 0x80000000, attRad, attHei, damRad,
	                        damHei);
	group->add((THitActor*&)mWaterHit);
	mWaterHit->unk64 &= ~1;
	mWaterHit->mPosition.x = mCenterJointMtx[0][3];
	mWaterHit->mPosition.y = mCenterJointMtx[1][3] - offY;
	mWaterHit->mPosition.z = mCenterJointMtx[2][3];
}

TBossHanachanPartsBase::TBossHanachanPartsBase(TBossHanachan* owner,
                                               u32 actorType, int paramType,
                                               const char* name)
    : TLiveActor(name)
{
	mCurAnm         = 0x12;
	mPrevAnm        = 0x12;
	mOwner          = owner;
	mWaterHit       = nullptr;
	mMapCollision   = nullptr;
	mCenterJointMtx = nullptr;
	mAnmCounter     = 0;
	mPalFrame       = nullptr;

	mMActorKeeper = owner->mMActorKeeper;
	mMActor       = mMActorKeeper->createMActorFromNthData(paramType, 0);
	if (mMActor->unkC != nullptr) {
		mMActor->unkC->initNormalMotionBlend();
	}

	initHitActor(actorType, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	unk64 |= 1;
	if (actorType == 0x08000015) {
		mScaledBodyRadius = owner->mParams->mSLBodyShadowSize.value;
	} else if (actorType == 0x08000014) {
		mScaledBodyRadius = owner->mParams->mSLHeadShadowSize.value;
	}
	mLiveFlag |= 8;
	initAnmSound();
	mMActor->setLightType(1);

	TBHPalFrame* pal = new TBHPalFrame;
	if (pal != nullptr) {
		pal->unk0   = 1;
		pal->mFrame = (int)(s16)CLBPalFrame(
		    (s16)owner->mParams->mSLMotionBlendFrames.value);
		pal->unk8   = 0;
		pal->unkC   = 0.0f;
		pal->unk10  = 0.0f;
		pal->unk14  = 0.0f;
		pal->unk18  = 0.0f;
		pal->unk1C  = 0.0f;
		pal->unk20  = 0.0f;
		pal->unk24  = 0;
		pal->unk28  = 0.0f;
	}
	mPalFrame = pal;
}

TBossHanachanPartsHead::TBossHanachanPartsHead(TBossHanachan* owner,
                                               const char* name)
    : TBossHanachanPartsBase(owner, 0x08000014, 1, name)
{
	MActor* a       = getMActor();
	JUTNameTab* tab = a->getModel()->mModelData->unkB0;
	u16 idxL        = tab->getIndex(cNoseHallJointName_L);
	mLeftNoseHallJointMtx
	    = (MtxPtr)((u8*)a->getModel()->mNodeMatrices + idxL * 0x30);
	u16 idxR = tab->getIndex(cNoseHallJointName_R);
	mRightNoseHallJointMtx
	    = (MtxPtr)((u8*)a->getModel()->mNodeMatrices + idxR * 0x30);
}

TBossHanachanPartsBody::TBossHanachanPartsBody(TBossHanachan* owner,
                                               const char* name)
    : TBossHanachanPartsBase(owner, 0x08000015, 0, name)
{
	unk114 = 0;
	unk120 = 0.0f;
	unk124 = 0.0f;
	unk128 = 0.0f;
	unk12C = 0.0f;
	unk130 = 0.0f;
	unk134 = 0.0f;
	unk138 = 0.0f;
	unk13C = 0.0f;
	unk140 = 0.0f;
	unk144 = 0.0f;
	unk148 = 0.0f;
	unk154 = 0.0f;
	unk158 = 0.0f;
	unk15C = 0.0f;

	J3DModel* model = getModel();
	JUTNameTab* tab = model->mModelData->unkB0;
	u16 idxL        = tab->getIndex(cLegJointName_L3);
	mLeftLegJointMtx = (MtxPtr)((u8*)model->mNodeMatrices + idxL * 0x30);
	u16 idxR        = tab->getIndex(cLegJointName_R3);
	mRightLegJointMtx = (MtxPtr)((u8*)model->mNodeMatrices + idxR * 0x30);
}
