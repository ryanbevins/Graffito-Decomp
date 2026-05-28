#include <Camera/cameralib.hpp>
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
#include <NPC/NpcNerve.hpp>
#include <NPC/NpcSave.hpp>
#include <NPC/NpcTrample.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/Spine.hpp>
#include <System/MarDirector.hpp>
#include <dolphin/mtx.h>
#include <math.h>

#pragma dont_inline on

void TBaseNPC::setBalloonMessage(u32 msg, long timer)
{
	mNpcBalloon->setNextMessage(msg, timer);
}

void TBaseNPC::setDummyConnectActor(const JDrama::TActor* actor)
{
	if (mActorType != 0x0400001C)
		return;
	mDummyConnectActor = actor;
	mPosition          = mDummyConnectActor->mPosition;
	mScaling           = mDummyConnectActor->mScaling;
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

Vec TBaseNPC::getFocalPoint() const
{
	return JGeometry::TVec3<f32>(
	    mPosition.x,
	    mEffectScaleBase.y
	            * (mNpcSaveIndividual->mSLBodyHeight.value
	               - mNpcSaveIndividual->mSLLookatHeight.value)
	        + mPosition.y,
	    mPosition.z);
}

Vec TBaseNPC::getCursorPos() const
{
	return JGeometry::TVec3<f32>(
	    mPosition.x,
	    mEffectScaleBase.y * mNpcSaveIndividual->mSLBodyHeight.value
	        + mPosition.y + mNpcSaveIndividual->mSLCursorHeight.value,
	    mPosition.z);
}

void TBaseNPC::execMotionBlend_()
{
	TNpcInbetween* ib = (TNpcInbetween*)mUnk18C;
	if (!ib->isMotionBlending())
		setKeepAnm_();
	ib->execMotionBlend(getMActor());
	if (ib->isForcedBlendRatio())
		mAnmRequest->mKind = -1;
}

void TBaseNPC::calcRootMatrix()
{
	if (mActorType == 0x0400001D) {
		TLiveActor::calcRootMatrix();
		return;
	}
	execMotionBlend_();
	if (mHolder != nullptr && mSDLModel != nullptr) {
		MtxPtr takingMtx = mHolder->getTakingMtx();
		PSMTXCopy(takingMtx, *(MtxPtr*)((u8*)mSDLModel + 0x20));
		mSDLModel->entry();
		PSMTXCopy(*(MtxPtr*)((u8*)getModel() + 0x20), *mSDLMtx);
	} else {
		TLiveActor::calcRootMatrix();
	}
}

void TBaseNPC::moveObject()
{
	if (mLiveFlag & 1)
		return;

	ensureTakeSituation();

	if (mHolder != nullptr && mHolder->mHeldObject != this)
		releaseTaken_();

	if (mNpcTrample != nullptr) {
		if (mNpcTrample->updateTrample(mEffectScaleBase.y, &mEffectScaleBase.y)
		    && isNerveCanGoToMad() && isStateGoToMad_())
			changeNerveToMad_();
	}

	{
		TNpcInbetween* ib = (TNpcInbetween*)mUnk18C;
		if (ib->mPosInbetweenTimer > 0)
			mPosition = ib->mCurrentPos;
	}

	if (mNpcCoin != nullptr)
		mNpcCoin->updateCoin();

	if (mNpcBalloon != nullptr) {
		u32 prevMsg = (u32)mNpcBalloon->_000;
		u8  state   = gpMarDirector->unk124;
		bool isEvent = (state == 1 || state == 2 || state == 3 || state == 4);
		if (isEvent && mNpcBalloon->updateBalloon()) {
			if (mHolder != nullptr) {
				if (prevMsg < 0xE0050) {
					if (prevMsg >= 0xE004F)
						mNpcBalloon->setNextMessage(0xE004F, 0x1C20);
				} else if (prevMsg > 0xE0050) {
					if (prevMsg < 0xE0052)
						mNpcBalloon->setNextMessage(0xE0051, 0x1C20);
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

	mAngularVelocity.x = 0.0f;

	if (SMS_GetGroundActor(mGroundPlane, 0) != nullptr) {
		mHeadHeight = mPtrSaveNormal->mSLHeadHeightSandBomb.value;
		behaveToSandBomb_(SMS_GetGroundActor(mGroundPlane, 0));
	} else {
		mHeadHeight = mPtrSaveNormal->mSLHeadHeightNormal.value;
		unk1C8      = 0.0f;
	}

	control();

	if (!(mLiveFlag & 0x80))
		calcRideMomentum();

	if ((mLiveFlag & 0x80) && !(mLiveFlag & 0x200))
		emitSinkEffect_();

	const TNerveBase<TLiveActor>* latest = mSpine->getLatestNerve();
	if (latest == &TNerveNPCSetPosAfterSinkBottom::theNerve())
		return;

	execNpcObjCollision_();
	if (!(mLiveFlag & 0x20))
		bind();

	if (mHolder != nullptr) {
		MtxPtr mtx     = mHolder->getTakingMtx();
		mPosition.x    = mtx[0][3];
		mPosition.y    = mtx[1][3];
		mPosition.z    = mtx[2][3];
		if (mSDLModel == nullptr) {
			f32 angle = CLBRoundf<s16>((180.0f / 3.14159265358979f) * mtx[1][0]);
			mRotation.y
			    = (f32)(s16)(angle - mAngleYDiffWhenTaken) * (360.0f / 65536.0f);
		}
	} else {
		if (!(mLiveFlag & 0x20) && isNerveWalk() && belongToGround() == 0
		    && mLinearVelocity.y > 0.0f)
			mLinearVelocity.y = 0.0f;
		mPosition.x += mLinearVelocity.x;
		mPosition.y += mLinearVelocity.y;
		mPosition.z += mLinearVelocity.z;
		mRotation.x += mAngularVelocity.x;
		mRotation.y += mAngularVelocity.y;
		mRotation.z += mAngularVelocity.z;
	}

	calcRidePos();
}

BOOL TBaseNPC::receiveMessage(THitActor* sender, u32 message)
{
	BOOL result = TRUE;
	if (mActorType == 0x0400001C)
		return result;

	if (message == 4) {
		if ((mLiveFlag & 0x100000) && mHolder == nullptr) {
			behaveToBeTaken_(sender);
			return TRUE;
		}
	}

	if (message == 0) {
		behaveToBeTrampled_();
		return TRUE;
	}

	if (message == 0xF) {
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
			behaveToHitObject_(sender, (EnumHitNpcObjectKind)0);
		return TRUE;
	}

	bool kinoMatch = false;
	if (message == 0x10) {
		kinoMatch = true;
	} else if (message == 0xE) {
		if (sender->mActorType != 0x4000005A)
			return TRUE;
		kinoMatch = true;
	}
	if (!kinoMatch)
		return TRUE;

	if (unk1E4 == 0) {
		JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
		SMS_EasyEmitParticle((E_SMS_EFFECT_ONETIME_NORMAL)0xA,
		                     (const JGeometry::TVec3<f32>*)&sender->mPosition,
		                     nullptr, scale);
		JGeometry::TVec3<f32> scale2(1.0f, 1.0f, 1.0f);
		SMS_EasyEmitParticle((E_SMS_EFFECT_ONETIME_NORMAL)0xB,
		                     (const JGeometry::TVec3<f32>*)&sender->mPosition,
		                     nullptr, scale2);
		JGeometry::TVec3<f32> scale3(1.0f, 1.0f, 1.0f);
		SMS_EasyEmitParticle((E_SMS_EFFECT_ONETIME_NORMAL)0xC,
		                     (const JGeometry::TVec3<f32>*)&sender->mPosition,
		                     nullptr, scale3);
		unk1E4 = 0x10;
	}

	bool shouldHit = isMadNpc();
	if (!shouldHit) {
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
		EnumHitNpcObjectKind kind
		    = message == 0x10 ? (EnumHitNpcObjectKind)1 : (EnumHitNpcObjectKind)2;
		behaveToHitObject_(sender, kind);
	}
	return result;
}

BOOL TBaseNPC::isInBodyTurnSearchRange() const
{
	BOOL result = FALSE;
	if (__fabsf(gpMarioPos->y - mPosition.y)
	    < mNpcSaveIndividual->mBodyTurnSearchHeight.value) {
		if (isInSight(*gpMarioPos, mNpcSaveIndividual->mBodyTurnSearchDist.value,
		              mNpcSaveIndividual->mBodyTurnSearchDegree.value,
		              mNpcSaveIndividual->mBodyTurnSearchAware.value)) {
			result = TRUE;
		}
	}
	return result;
}

bool TBaseNPC::isInMadSearchRange() const
{
	bool result = false;
	if (__fabsf(gpMarioPos->y - mPosition.y)
	    < mNpcSaveIndividual->mMadSearchHeight.value) {
		if (isInSight(*gpMarioPos, mNpcSaveIndividual->mMadSearchDist.value,
		              mNpcSaveIndividual->mMadSearchDegree.value,
		              mNpcSaveIndividual->mMadSearchAware.value)) {
			result = true;
		}
	}
	return result;
}

bool TBaseNPC::isMadNpc() const
{
	bool result  = false;
	bool partA   = false;
	BOOL helper  = TRUE;
	switch (mActorType) {
	case 0x04000001:
	case 0x04000002:
	case 0x04000003:
	case 0x04000004:
	case 0x04000005:
		partA = true;
	}
	if (!partA) {
		if (!isNormalMonteW())
			helper = FALSE;
	}
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

BOOL TBaseNPC::isPartsAnmNpc() const
{
	BOOL result   = FALSE;
	bool isGroupA = false;
	u32 type      = mActorType;
	switch (type) {
	case 0x0400000F:
	case 0x04000014:
		isGroupA = true;
	}
	if (isGroupA) {
		result = TRUE;
	} else {
		switch (type) {
		case 0x04000010:
		case 0x04000015:
		case 0x04000018:
			result = TRUE;
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
