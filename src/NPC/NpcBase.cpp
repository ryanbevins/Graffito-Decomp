#define JG_TUTIL_SQRT_OUT_OF_LINE

#include <Camera/Camera.hpp>
#include <Camera/cameralib.hpp>
#include <Enemy/Conductor.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/MtxUtil.hpp>
#include <System/Particles.hpp>
#include <JSystem/JDrama/JDRActor.hpp>
#include <JSystem/JGeometry/JGVec3.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <MarioUtil/MapUtil.hpp>
#include <NPC/NpcBalloon.hpp>
#include <NPC/NpcBase.hpp>
#include <NPC/NpcCoin.hpp>
#include <NPC/NpcInbetween.hpp>
#include <NPC/NpcManager.hpp>
#include <NPC/NpcNerve.hpp>
#include <NPC/NpcParts.hpp>
#include <NPC/NpcSave.hpp>
#include <NPC/NpcTrample.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/Spine.hpp>
#include <System/MarDirector.hpp>
#include <dolphin/mtx.h>
#include <math.h>

template <class T> static inline T max(const T& a, const T& b)
{
	return a > b ? a : b;
}

#pragma dont_inline on

void TBaseNPC::setBalloonMessage(u32 msg, long timer)
{
	mNpcBalloon->setNextMessage(msg, timer);
}

void TBaseNPC::setDummyConnectActor(const JDrama::TActor* actor)
{
	if (mActorType != 0x0400001C)
		return;
	mDummyConnectActor    = actor;
	*(Vec*)&mPosition     = *(Vec*)&mDummyConnectActor->mPosition;
	*(Vec*)&mRotation     = *(Vec*)&mDummyConnectActor->mRotation;
}

BOOL TSpineEnemy::isReachedToGoal() const
{
	const Vec* point;
	if (unk104.unk0 != nullptr)
		point = (const Vec*)&unk104.unk0->mPosition;
	else
		point = (const Vec*)&unk104.unk4;

	Vec diff = *point;
	diff.x -= mPosition.x;
	diff.y -= mPosition.y;
	diff.z -= mPosition.z;

	f32 length = JGeometry::TUtil<f32>::sqrt(
	    diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
	if (length < 100.0f)
		return TRUE;
	return FALSE;
}

void TBaseNPC::updateForbidCount_()
{
	if (unk1E0 != 0)
		unk1E0 = unk1E0 - 1;
	if (unk1E2 != 0)
		unk1E2 = unk1E2 - 1;
	if (unk1E4 != 0)
		unk1E4 = unk1E4 - 1;
}

JGeometry::TVec3<f32> TBaseNPC::getFocalPoint() const
{
	return JGeometry::TVec3<f32>(
	    mPosition.x,
	    mEffectScaleBase.y
	            * (mNpcSaveIndividual->mSLBodyHeight.value
	               - mNpcSaveIndividual->mSLLookatHeight.value)
	        + mPosition.y,
	    mPosition.z);
}

JGeometry::TVec3<f32> TBaseNPC::getCursorPos() const
{
	f32 bodyY = mEffectScaleBase.y * mNpcSaveIndividual->mSLBodyHeight.value
	            + mPosition.y;
	return JGeometry::TVec3<f32>(
	    mPosition.x, bodyY + mNpcSaveIndividual->mSLCursorHeight.value,
	    mPosition.z);
}

TNpcSaveNormal* TBaseNPC::mPtrSaveNormal;
s16 TBaseNPC::mAngleYDiffWhenTaken;
TBaseNPC* gpCurrentNpc;

void TBaseNPC::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (mActorType == 0x0400001C) {
		if (flags & 1) {
			if (mDummyConnectActor != nullptr) {
				*(Vec*)&mPosition = *(Vec*)&mDummyConnectActor->mPosition;
				*(Vec*)&mRotation = *(Vec*)&mDummyConnectActor->mRotation;
			}
			if (!(mLiveFlag & 1)) {
				updateForbidCount_();
				updateSquareToMario();
				control();
				if (graphics->unk0 & 2)
					changeNerveProc_();
			}
		}
		return;
	}

	if (mActorType == 0x0400001D) {
		if (mLiveFlag & 0x201)
			return;
		if ((flags & 1) && !(mLiveFlag & 1)) {
			updateForbidCount_();
			updateSquareToMario();
			control();
			if (graphics->unk0 & 2)
				changeNerveProc_();
		}
		TLiveActor::performOnlyDraw(flags, graphics);
		return;
	}

	bool doUpdate = true;
	if (mLiveFlag & 1) {
		doUpdate = false;
	} else if (mLiveFlag & LIVE_FLAG_UNK200) {
		doUpdate = false;
	} else if ((flags & 0x204) && (mLiveFlag & 6)) {
		doUpdate = false;
	} else if ((flags & 1) && mHolder == nullptr && !(mLiveFlag & 0x80)
	           && belongToGround() == 0
	           && *(int*)((u8*)mSpine + 0x20) != 0
	           && mActorType != 0x04000018
	           && isNerveMaybeDontMovement()
	           && !(mLiveFlag & 0x800000)) {
		f32 clip = gpConductor->unk84.mEnemyFarClip.value;
		if (mManager != nullptr)
			clip = *((TNPCManager*)mManager)->unk58;

		CPolarSubCamera* camera = gpCamera;
		f32 sight = matan(camera->unk148.z - camera->unk124.z,
		                  camera->unk148.x - camera->unk124.x)
		            * (360.0f / 65536.0f);
		camera = gpCamera;
		JGeometry::TVec3<f32> cameraPos;
		cameraPos.x = camera->unk124.x;
		cameraPos.y = camera->unk124.y;
		cameraPos.z = camera->unk124.z;
		if (!MsIsInSight(cameraPos, sight, mPosition, clip + 500.0f, 120.0f,
		                 800.0f)) {
			updateSquareToMario();
			doUpdate = false;
		}
	}

	if (!doUpdate) {
		mLiveFlag &= ~0x60000;
		return;
	}

	gpCurrentNpc = this;

	if (flags & 1) {
		moveObject();
		if (graphics->unk0 & 2) {
			changeNerveProc_();
			if (mHolder == nullptr) {
				if (isNerveWalk())
					walkAnmRateChange_();
				if (*(int*)((u8*)unkD0 + 0x14) == 4) {
					f32 frameRate = SMSGetAnmFrameRate();
					f32 rate = frameRate * mTurnSpeed
					           * mNpcSaveIndividual->mTurnAnmRate.value;
					f32 maxRate
					    = frameRate * mNpcSaveIndividual->mTurnAnmMaxRate.value;
					f32 minRate
					    = frameRate * mNpcSaveIndividual->mTurnAnmMinRate.value;
					if (rate > maxRate)
						rate = maxRate;
					else if (rate < minRate)
						rate = minRate;
					mMActor->setFrameRate(rate, 0);
				}
			}
			((TNpcInbetween*)mUnk18C)->execPosInbetween(&mPosition);
			if (unk1DC > 0) {
				unk1DC = unk1DC - 1;
				if (unk1DC == 0 && mHolder == nullptr) {
					unk64 &= ~1;
					mLiveFlag &= ~0x10000000;
				}
			}
			bool isLock = false;
			switch (mActorType) {
			case 0x0400000F:
			case 0x04000014:
				isLock = true;
			}
			if (!isLock && mActorType != 0x04000007)
				setVariableDamageRadius_();
			if (isPollutionNpc()) {
				unk174.a
				    = (u8)(unk178
				           * (f32)*((u8*)mNpcSaveIndividual + 0x310));
			}
		}
		flags &= ~1;
	}

	if (flags & 2) {
		if (mActionFlag & 0x4000) {
			if (gpMSound->gateCheck(0x8017)) {
				MSoundSESystem::MSoundSE::startSoundNpcActor(
				    0x8017, (const Vec*)&mPosition, 0, nullptr, 0, 4);
			}
		}
		if (!(mLiveFlag & 0x10006))
			emitParticle_();
	}

	bool drewWithAnim = false;
	if (flags & 2) {
		bool hasMtxEffectFlag = (mLiveFlag & 0x1000000) ? true : false;
		if (mLiveFlag & 7) {
			drewWithAnim = true;
			setGroundCollision();
			execMotionBlend_();
			getMActor()->frameUpdate();
			if (mNpcParts != nullptr && isPartsAnmNpc())
				mNpcParts->partsFrameUpdate();
		} else if (mHolder == nullptr && !(mLiveFlag & 0x80)
		           && belongToGround() == 0
		           && (isNerveMaybeDontCalcAnim0()
		               || isNerveMaybeDontCalcAnim1())) {
			f32 anmOffDist = getAnmOffDist_();
			JGeometry::TVec3<f32> diff;
			diff.x      = mPosition.x - gpCamera->unk124.x;
			diff.y      = mPosition.y - gpCamera->unk124.y;
			diff.z      = mPosition.z - gpCamera->unk124.z;
			f32 distSq  = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
			f32 distMax = CLBSquared<f32>(anmOffDist);
			if (distSq > distMax && !hasMtxEffectFlag
			    && *(int*)((u8*)mSpine + 0x20) > 2) {
				drewWithAnim = true;
				execMotionBlend_();
			}
		}

		if (hasMtxEffectFlag && !drewWithAnim && mMultiMtxEffect != nullptr) {
			for (int i = 0; i < mMultiMtxEffect->mNumBones; ++i)
				mMultiMtxEffect->mMtxEffectTbl[i]->mFlags |= 0x2;
		}

		if (drewWithAnim)
			flags &= ~2;

		if ((flags & 2) && mMultiMtxEffect != nullptr) {
			mMultiMtxEffect->setUserArea();
			if (mActorType == 0x04000018 && mHolder != nullptr) {
				for (int i = 0; i < mMultiMtxEffect->mNumBones; ++i)
					mMultiMtxEffect->mMtxEffectTbl[i]->mFlags |= 0x2;
			}
		}
	}

	if (flags & 0x200) {
		mLiveFlag &= ~0x1000000;
		JGeometry::TVec3<f32> diff;
		diff.x = mPosition.x - gpCamera->unk124.x;
		diff.y = mPosition.y - gpCamera->unk124.y;
		diff.z = mPosition.z - gpCamera->unk124.z;
		f32 distSq    = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
		f32 distSqMax
		    = CLBSquared<f32>(mNpcSaveIndividual->mAllDLLockDist.value);
		if (distSq > distSqMax) {
			bool isSunflower = false;
			if ((s32)mActorType < 0x0400001C
			    && (s32)mActorType >= 0x0400001A)
				isSunflower = true;
			if (!isSunflower)
				getModel()->lock();
		} else {
			getMActor()->unlockDLIfNeed();
		}
	}

	TSpineEnemy::perform(flags, graphics);
	if (mNpcParts != nullptr)
		mNpcParts->partsPerform(flags, graphics);
}

const GXColor* TBaseNPC::getPtrInitPollutionColor() const
{
	const GXColor* result = nullptr;
	bool           isPol  = false;
	switch (mActorType) {
	case 0x04000001:
	case 0x04000002:
	case 0x04000004:
	case 0x0400000A:
	case 0x0400000B:
	case 0x0400000E:
	case 0x04000013:
	case 0x04000016:
		isPol = true;
	}
	if (isPol) {
		result = (const GXColor*)&unk174;
	} else if (mActorType != 0x04000006) {
		bool inMonteRange = false;
		if ((s32)mActorType < 0x0400000A
		    && (s32)mActorType >= 0x04000006)
			inMonteRange = true;
		bool isMonte = true;
		if (!inMonteRange && !isSpecialMonteW())
			isMonte = false;
		if (isMonte) {
			result = (const GXColor*)&unk174;
		} else {
			bool inMareRange = false;
			if ((s32)mActorType < 0x04000013
			    && (s32)mActorType >= 0x0400000F)
				inMareRange = true;
			bool isMare = true;
			if (!inMareRange && !isSpecialMareW())
				isMare = false;
			if (isMare)
				result = (const GXColor*)&unk174;
		}
	}
	return result;
}

void TBaseNPC::execMotionBlend_()
{
	bool blending;
	if (((TNpcInbetween*)mUnk18C)->mMotionBlendTimer > 0)
		blending = true;
	else
		blending = false;
	if (!blending)
		setKeepAnm_();
	MActor* actor = mMActor;
	TNpcInbetween* inbetween = (TNpcInbetween*)mUnk18C;
	inbetween->execMotionBlend(actor);
	bool forced;
	if (((TNpcInbetween*)mUnk18C)->mForcedBlendRatio > 0.0f)
		forced = true;
	else
		forced = false;
	if (forced)
		mKeepAnmCtrl->reset();
}

void TBaseNPC::calcRootMatrix()
{
	if (mActorType == 0x0400001D) {
		TLiveActor::calcRootMatrix();
		return;
	}
	{
		bool blending;
		if (((TNpcInbetween*)mUnk18C)->mMotionBlendTimer > 0)
			blending = true;
		else
			blending = false;
		if (!blending)
			setKeepAnm_();
		MActor* actor = mMActor;
		TNpcInbetween* inbetween = (TNpcInbetween*)mUnk18C;
		inbetween->execMotionBlend(actor);
		bool forced;
		if (((TNpcInbetween*)mUnk18C)->mForcedBlendRatio > 0.0f)
			forced = true;
		else
			forced = false;
		if (forced)
			mKeepAnmCtrl->reset();
	}
	if (mHolder != nullptr && mSDLModel != nullptr) {
		MtxPtr takingMtx = mHolder->getTakingMtx();
		PSMTXCopy(takingMtx, mSDLModel->unk20);
		mSDLModel->calc();
		MtxPtr sdlMtx = mSDLMtx;
		PSMTXCopy(sdlMtx, getModel()->unk20);
	} else {
		TLiveActor::calcRootMatrix();
	}
}

void TBaseNPC::moveObject()
{
	if (mLiveFlag & 1)
		return;

	ensureTakeSituation();

	if (mTakenBy != nullptr && ((TTakeActor*)mTakenBy)->mHeldObject != this)
		releaseTaken_();

	if (mNpcTrample != nullptr) {
		if (mNpcTrample->updateTrample(mEffectScaleBase.y, &mScaling.y)
		    && isNerveCanGoToMad() && isStateGoToMad_())
			changeNerveToMad_();
	}

	{
		TNpcInbetween* ib = (TNpcInbetween*)mUnk18C;
		if (ib->mPosInbetweenTimer > 0) {
			mPosition.x = ib->mCurrentPos.x;
			mPosition.y = ib->mCurrentPos.y;
			mPosition.z = ib->mCurrentPos.z;
		}
	}

	if (mNpcCoin != nullptr)
		mNpcCoin->updateCoin();

	if (mNpcBalloon != nullptr) {
		s32 prevMsg = mNpcBalloon->_000;
		bool block  = true;
		bool mode12 = block;
		if (gpMarDirector->unk124 != 1 && gpMarDirector->unk124 != 2)
			mode12 = false;
		if (!mode12) {
			bool mode34 = true;
			if (gpMarDirector->unk124 != 3 && gpMarDirector->unk124 != 4)
				mode34 = false;
			if (!mode34)
				block = false;
		}
		if (!block && mNpcBalloon->updateBalloon()) {
			if (mHolder != nullptr) {
				if (prevMsg == 0xE0050) {
				} else if (prevMsg < 0xE0050) {
					if (prevMsg >= 0xE004F)
						mNpcBalloon->setNextMessage(0xE0051, 0x1C20);
				} else {
					if (prevMsg < 0xE0052)
						mNpcBalloon->setNextMessage(0xE004F, 0x1C20);
				}
			} else {
				if (prevMsg == 0xE0050)
					mNpcBalloon->setNextMessage(0xE0050, 0x1C20);
			}
		}
	}

	if (unk1E0 != 0)
		unk1E0 = unk1E0 - 1;
	if (unk1E2 != 0)
		unk1E2 = unk1E2 - 1;
	if (unk1E4 != 0)
		unk1E4 = unk1E4 - 1;

	updateSquareToMario();
	mLinearVelocity.x = mLinearVelocity.y = mLinearVelocity.z = 0.0f;
	mAngularVelocity.x = mAngularVelocity.y = mAngularVelocity.z = 0.0f;

	const TLiveActor* sand = SMS_GetGroundActor(mGroundPlane, 0x400000CD);
	if (sand != nullptr) {
		mHeadHeight = mPtrSaveNormal->mSLHeadHeightSandBomb.value;
		behaveToSandBomb_(sand);
	} else {
		mHeadHeight = mPtrSaveNormal->mSLHeadHeightNormal.value;
		unk1C8      = 0.0f;
	}

	control();

	if (!(mLiveFlag & 0x400000))
		calcRideMomentum();

	if ((mLiveFlag & 0x400000) && !(mLiveFlag & 0x1000000))
		emitSinkEffect_();

	const TNerveBase<TLiveActor>* latest = mSpine->getLatestNerve();
	if (latest == &TNerveNPCSetPosAfterSinkBottom::theNerve())
		return;

	execNpcObjCollision_();
	if (!(mLiveFlag & 0x10))
		bind();

	if (mHolder != nullptr) {
		MtxPtr mtx     = mHolder->getTakingMtx();
		mPosition.x    = mtx[0][3];
		mPosition.y    = mtx[1][3];
		mPosition.z    = mtx[2][3];
		if (mSDLModel == nullptr) {
			s16 angle = CLBRoundf<s16>(182.04445f * mHolder->mRotation.y);
			mRotation.y
			    = (f32)(s16)(angle - mAngleYDiffWhenTaken) * (360.0f / 65536.0f);
		}
	} else {
		if (!(mLiveFlag & 0x10) && isNerveWalk() && belongToGround() == 0
		    && mLinearVelocity.y > 5.0f)
			mLinearVelocity.y = 5.0f;
		mPosition.x += mLinearVelocity.x;
		mPosition.y += mLinearVelocity.y;
		mPosition.z += mLinearVelocity.z;
		mRotation.x += mAngularVelocity.x;
		mRotation.y += mAngularVelocity.y;
		mRotation.z += mAngularVelocity.z;
	}

	calcRidePos();
}

TBaseNPC::TBaseNPC(u32 actorType, const char* name)
    : TSpineEnemy(name)
{
	mSDLModel             = nullptr;
	mSDLMtx               = nullptr;
	mTakenBy              = nullptr;
	mSinkTimer            = nullptr;
	mMultiMtxEffect       = nullptr;
	mNpcKind              = -1;
	mNpcParts             = nullptr;
	_16C                  = 0;
	mActionFlag           = 0;
	unk174.r              = 0xFF;
	unk174.g              = 0xFF;
	unk174.b              = 0xFF;
	unk174.a              = 0;
	unk178                = 0.0f;
	unk17C                = nullptr;
	mNpcTrample           = nullptr;
	mNpcCoin              = nullptr;
	mNpcBalloon           = nullptr;
	mUnk18C               = nullptr;
	mKeepAnmCtrl          = nullptr;
	mSinkBaseY            = 0.0f;
	unk1C8                = 0.0f;
	unk1CC                = 0;
	unk1D0                = 0.0f;
	mDummyConnectActor    = nullptr;
	unk1D8                = 0;
	unk1D9                = 0;
	unk1DA                = 0;
	unk1DC                = 0;
	unk1E0                = 0x78;
	unk1E2                = 0;
	unk1E4                = 0;
	mPtrHappyEffectMtx    = nullptr;
	mPtrNoteEffectMtx     = nullptr;
	mNoteEffectPos.x      = 0.0f;
	mNoteEffectPos.y      = 0.0f;
	mNoteEffectPos.z      = 0.0f;
	mPtrPollutionEffectMtx  = nullptr;
	mPtrPollutionLEffectMtx = nullptr;
	mPtrPollutionREffectMtx = nullptr;
	mPtrSmokeEffectMtx      = nullptr;
	mSmokeEffectPos.x       = 0.0f;
	mSmokeEffectPos.y       = 0.0f;
	mSmokeEffectPos.z       = 0.0f;
	mFireScaleMul           = 1.0f;
	mWaterEffectPos.x       = 0.0f;
	mWaterEffectPos.y       = 0.0f;
	mWaterEffectPos.z       = 0.0f;
	mAnmFrameCounter        = nullptr;
	mNeckAngles             = nullptr;
	gpCurrentNpc            = nullptr;
	mAngleYDiffWhenTaken    = 0;
	mActorType              = actorType;

	if (mActorType == 0x0400001C)
		return;

	mKeepAnmCtrl = new TNpcKeepAnm;

	bool isLock = false;
	switch (actorType) {
	case 0x0400000F:
	case 0x04000014:
		isLock = true;
	}
	if (isLock)
		return;

	bool wantTrample = false;
	if (isNormalMonteM() || isNormalMonteW()) {
		wantTrample = true;
	} else if (isSpecialMonteM() || isSpecialMonteW()) {
		wantTrample = true;
	} else if (isNormalMareM() || isNormalMareW()) {
		wantTrample = true;
	} else if (isSpecialMareM() || isSpecialMareW()) {
		wantTrample = true;
	} else if ((s32)mActorType < 0x04000018 && (s32)mActorType >= 0x04000016) {
		wantTrample = true;
	}

	if (wantTrample) {
		TNpcTrample* tr = new TNpcTrample;
		if (tr != nullptr) {
			tr->unk0 = 0.0f;
			tr->unk4 = 0;
			tr->unk6 = 0;
			TNpcTrample::msAmpDecrease = 0.0f;
		}
		mNpcTrample = tr;
	}
}

TBaseNPC::~TBaseNPC() { }

void TBaseNPC::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	if (mActorType == 0x04000018) {
		if (gpMarDirector->mMap == 1 && gpMarDirector->unk7D == 1) {
			TNpcBalloon* balloon = new TNpcBalloon;
			if (balloon != nullptr) {
				balloon->_000                = 0;
				balloon->mBalloonAppearTimer = -1;
			}
			mNpcBalloon = balloon;
		}
	}
	gpMarDirector->entryNPC(this);
}

void TBaseNPC::load(JSUMemoryInputStream& stream)
{
	TSpineEnemy::load(stream);
	mResetPos         = mPosition;
	mResetRot         = mRotation;
	mEffectScaleBase = mScaling;
	mLoadRot           = getFocalPoint();
	if (!((s32)mActorType < 0x0400001E && (s32)mActorType >= 0x0400001C))
		setIndividualDifference_(stream);
}

BOOL TBaseNPC::receiveMessage(THitActor* sender, u32 message)
{
	bool result = false;
	switch (mActorType) {
	case 0x0400001C:
		break;
	default:
		if (message == 4) {
			if ((mLiveFlag & 0x100000) && mHolder == nullptr) {
				behaveToBeTaken_(sender);
				result = true;
			}
		} else if (message == 0) {
			behaveToBeTrampled_();
			result = true;
		} else if (message == 0xF) {
			bool shouldBehave = true;
			switch (mActorType) {
			case 0x04000007:
			case 0x04000008:
			case 0x0400000F:
			case 0x04000014:
			case 0x0400001C:
			case 0x0400001D:
				shouldBehave = false;
			}
			if (shouldBehave)
				behaveToHitObject_(sender, HIT_NPC_OBJECT_KIND_WATER_SPRAY);
			result = true;
		} else if (message == 0x10
		           || (message == 0xE && sender->mActorType == 0x4000005A)) {
			if (unk1E4 == 0) {
				JGeometry::TVec3<f32> scale;
				scale.x = 1.0f;
				scale.y = 1.0f;
				scale.z = 1.0f;
				SMS_EasyEmitParticle(
				    (E_SMS_EFFECT_ONETIME_NORMAL)0xA,
				    (const JGeometry::TVec3<f32>*)&sender->mPosition, nullptr,
				    scale);
				JGeometry::TVec3<f32> scale2;
				scale2.x = 1.0f;
				scale2.y = 1.0f;
				scale2.z = 1.0f;
				SMS_EasyEmitParticle(
				    (E_SMS_EFFECT_ONETIME_NORMAL)0xB,
				    (const JGeometry::TVec3<f32>*)&sender->mPosition, nullptr,
				    scale2);
				JGeometry::TVec3<f32> scale3;
				scale3.x = 1.0f;
				scale3.y = 1.0f;
				scale3.z = 1.0f;
				SMS_EasyEmitParticle(
				    (E_SMS_EFFECT_ONETIME_NORMAL)0xC,
				    (const JGeometry::TVec3<f32>*)&sender->mPosition, nullptr,
				    scale3);
				unk1E4 = 0x10;
			}

			bool shouldHit = false;
			if (isBeTrampledNpc()) {
				shouldHit = true;
			} else {
				switch (mActorType) {
				case 0x0400000E:
				case 0x04000010:
				case 0x04000011:
				case 0x04000013:
				case 0x04000015:
				case 0x04000016:
				case 0x04000017:
					shouldHit = true;
				}
			}
			if (shouldHit) {
				EnumHitNpcObjectKind kind = HIT_NPC_OBJECT_KIND_UNK2;
				if (message == 0x10)
					kind = HIT_NPC_OBJECT_KIND_UNK1;
				behaveToHitObject_(sender, kind);
			}
			result = true;
		}
		break;
	}
	return result;
}

bool TBaseNPC::isNeedNeckStraight() const
{
	bool  result = false;
	void* holder = (void*)mHolder;
	int   cur14  = *(int*)((u8*)unkD0 + 0x14);
	if (holder != nullptr && holder == (void*)gpMarioAddress) {
		result = true;
	} else if (unk178 != 0.0f) {
		result = true;
	} else {
		if (mActorType == 0x04000012) {
			result = true;
		} else if (mActorType == 0x04000019 && cur14 == 0x5) {
			result = true;
		} else {
			bool helper    = true;
			bool mareMatch = true;
			if (mActorType != 0x0400000E && !isNormalMareW())
				helper = false;
			if (!helper) {
				helper = true;
				if (!isSpecialMareM() && !isSpecialMareW())
					helper = false;
				if (!helper)
					mareMatch = false;
			}
			if (mareMatch && cur14 == 0xC) {
				result = true;
			} else {
				bool isUnk1D8Bit = (unk1D8 & 0x2) != 0;
				bool flagged     = isUnk1D8Bit || cur14 == 0x5;
				bool actorIs18   = (mActorType == 0x04000018);
				if (flagged && actorIs18)
					result = true;
			}
		}
	}
	return result;
}

f32 TBaseNPC::getAnmOffDist_()
{
	bool useOff = false;
	u8*  stateObj = (u8*)unkD0;
	f32  result  = gpCamera->mFar;
	int  state   = *(int*)(stateObj + 0x14);
	f32  camDist = mPtrSaveNormal->mSLDanceAnmOffDist.value;
	if ((mActionFlag & 0x204) || mActorType == 0x0400000D || state == 0xA
	    || state == 0x17)
	useOff = true;
	if (isNerveMaybeDontCalcAnim0()) {
		result = mNpcSaveIndividual->mWaitAnmOffDist0.value;
		if (useOff) {
			result = max<f32>(camDist, result);
		}
	} else if (isNerveMaybeDontCalcAnim1()) {
		result = mNpcSaveIndividual->mWaitAnmOffDist1.value;
		if (useOff) {
			result = max<f32>(camDist, result);
		}
	}
	return result;
}

bool TBaseNPC::isBeTrampledNpc() const
{
	bool result = false;
	bool partA  = false;
	switch (mActorType) {
	case 0x0400000F:
	case 0x04000014:
		partA = true;
	}
	if (!partA) {
		bool isMonte = true;
		if (!isNormalMonteM() && !isNormalMonteW())
			isMonte = false;
		if (!isMonte) {
			isMonte = true;
			if (!isSpecialMonteM() && !isSpecialMonteW())
				isMonte = false;
		}
		bool isMare = false;
		if (isMonte) {
			result = true;
		} else {
			bool mareMatch = true;
			bool helper    = true;
			if (mActorType != 0x0400000E && !isNormalMareW())
				helper = false;
			if (!helper) {
				helper = true;
				if (!isSpecialMareM() && !isSpecialMareW())
					helper = false;
				if (!helper)
					mareMatch = false;
			}
			if (mareMatch) {
				result = true;
			} else {
				if ((s32)mActorType < 0x04000018
				    && (s32)mActorType >= 0x04000016)
					result = true;
			}
		}
	}
	return result;
}

bool TBaseNPC::isSmallNpc() const
{
	bool result = false;
	bool partA  = false;
	if (mScaling.x < 0.7f && mScaling.y < 0.7f && mScaling.z < 0.7f)
		partA = true;
	if (!partA) {
		bool helper    = true;
		bool mareMatch = true;
		if (mActorType != 0x0400000E && !isNormalMareW())
			helper = false;
		if (!helper) {
			helper = true;
			if (!isSpecialMareM() && !isSpecialMareW())
				helper = false;
			if (!helper)
				mareMatch = false;
		}
		if (mareMatch) {
			result = true;
		} else {
			if ((s32)mActorType < 0x04000018
			    && (s32)mActorType >= 0x04000016)
				result = true;
		}
	} else {
		result = true;
	}
	return result;
}

bool TBaseNPC::isInBodyTurnSearchRange() const
{
	bool result = false;
	if (__fabsf(gpMarioPos->y - mPosition.y)
	    < mNpcSaveIndividual->mBodyTurnSearchHeight.value) {
		f32 aware  = mNpcSaveIndividual->mBodyTurnSearchAware.value;
		f32 degree = mNpcSaveIndividual->mBodyTurnSearchDegree.value;
		f32 dist   = mNpcSaveIndividual->mBodyTurnSearchDist.value;
		if (isInSight(*gpMarioPos, dist, degree, aware)) {
			result = true;
		}
	}
	return result;
}

bool TBaseNPC::isInMadSearchRange() const
{
	bool result = false;
	if (__fabsf(gpMarioPos->y - mPosition.y)
	    < mNpcSaveIndividual->mMadSearchHeight.value) {
		f32 aware  = mNpcSaveIndividual->mMadSearchAware.value;
		f32 degree = mNpcSaveIndividual->mMadSearchDegree.value;
		f32 dist   = mNpcSaveIndividual->mMadSearchDist.value;
		if (isInSight(*gpMarioPos, dist, degree, aware)) {
			result = true;
		}
	}
	return result;
}

bool TBaseNPC::isMadNpc() const
{
	bool result  = false;
	bool partA   = false;
	switch (mActorType) {
	case 0x04000001:
	case 0x04000002:
	case 0x04000003:
	case 0x04000004:
	case 0x04000005:
		partA = true;
	}
	bool helper = partA || isNormalMonteW();
	if (helper) {
		result = true;
	} else {
		switch (mActorType) {
		case 0x04000006:
		case 0x04000007:
		case 0x0400000D:
			result = true;
		}
	}
	return result;
}

bool TBaseNPC::isPartsAnmNpc() const
{
	bool result = false;
	if (isJellyFishMare()) {
		result = true;
	} else {
		switch (mActorType) {
		case 0x04000010:
		case 0x04000015:
		case 0x04000018:
			result = true;
		}
	}
	return result;
}

BOOL TBaseNPC::isBehaveToWaterNpc() const
{
	BOOL result = TRUE;
	switch (mActorType) {
	case 0x04000007:
	case 0x04000008:
	case 0x0400000F:
	case 0x04000014:
	case 0x0400001C:
	case 0x0400001D:
		result = FALSE;
	}
	return result;
}

bool TBaseNPC::isPollutionNpc() const
{
	bool result = false;
	switch (mActorType) {
	case 0x04000001:
	case 0x04000002:
	case 0x04000004:
	case 0x0400000A:
	case 0x0400000B:
	case 0x0400000E:
	case 0x04000013:
	case 0x04000016:
		result = true;
	}
	return result;
}

bool TBaseNPC::isChild() const
{
	bool result = false;
	if (mScaling.x < 0.7f && mScaling.y < 0.7f && mScaling.z < 0.7f)
		result = true;
	return result;
}

bool TBaseNPC::isSunflower() const
{
	bool result = false;
	if ((s32)mActorType < 0x0400001C && (s32)mActorType >= 0x0400001A)
		result = true;
	return result;
}

#pragma dont_inline off
bool TBaseNPC::isJellyFishMare() const
{
	bool result = false;
	switch (mActorType) {
	case 0x0400000F:
	case 0x04000014:
		result = true;
	}
	return result;
}
#pragma dont_inline on

bool TBaseNPC::isSpecialMareW() const
{
	bool result = false;
	if ((s32)mActorType < 0x04000016 && (s32)mActorType >= 0x04000014)
		result = true;
	return result;
}

bool TBaseNPC::isSpecialMareM() const
{
	bool result = false;
	if ((s32)mActorType < 0x04000013 && (s32)mActorType >= 0x0400000F)
		result = true;
	return result;
}

bool TBaseNPC::isNormalMareW() const
{
	bool result = false;
	switch (mActorType) {
	case 0x04000013:
		result = true;
	}
	return result;
}

bool TBaseNPC::isNormalMareM() const
{
	bool result = false;
	switch (mActorType) {
	case 0x0400000E:
		result = true;
	}
	return result;
}

bool TBaseNPC::isSpecialMonteW() const
{
	bool result = false;
	switch (mActorType) {
	case 0x0400000D:
		result = true;
	}
	return result;
}

bool TBaseNPC::isSpecialMonteM() const
{
	bool result = false;
	if ((s32)mActorType < 0x0400000A && (s32)mActorType >= 0x04000006)
		result = true;
	return result;
}

bool TBaseNPC::isNormalMonteW() const
{
	bool result = false;
	if ((s32)mActorType < 0x0400000D && (s32)mActorType >= 0x0400000A)
		result = true;
	return result;
}

bool TBaseNPC::isNormalMonteM() const
{
	bool result = false;
	if ((s32)mActorType < 0x04000006 && (s32)mActorType >= 0x04000001)
		result = true;
	return result;
}

#pragma dont_inline off
