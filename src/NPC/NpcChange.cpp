// NPC/NpcChange.cpp -- partial decomp.

#include <Camera/Camera.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JMath.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <Map/Map.hpp>
#include <Map/PollutionManager.hpp>
#include <MarioUtil/MapUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <NPC/NpcBalloon.hpp>
#include <NPC/NpcBase.hpp>
#include <NPC/NpcNerve.hpp>
#include <NPC/NpcSave.hpp>
#include <NPC/NpcTrample.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/Spine.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarDirector.hpp>

bool TBaseNPC::isNerveWalk() const
{
	bool result                       = false;
	const TNerveBase<TLiveActor>* cur = mSpine->getLatestNerve();
	if (cur == &TNerveNPCGraphWander::theNerve()
	    || cur == &TNerveNPCUTurn::theNerve()
	    || cur == &TNerveNPCGraphWait::theNerve()) {
		result = true;
	}
	return result;
}

bool TBaseNPC::isNerveMaybeDontMovement() const
{
	bool result                       = false;
	const TNerveBase<TLiveActor>* cur = mSpine->getLatestNerve();
	if (cur == &TNerveNPCWaitContinue::theNerve()
	    || cur == &TNerveNPCWaitMarioApproach::theNerve()
	    || cur == &TNerveNPCSink::theNerve()) {
		result = true;
	}
	return result;
}

bool TBaseNPC::isNerveMaybeDontCalcAnim0() const
{
	bool result                       = false;
	const TNerveBase<TLiveActor>* cur = mSpine->getLatestNerve();
	if (cur == &TNerveNPCWaitContinue::theNerve()
	    || cur == &TNerveNPCWaitMarioApproach::theNerve()) {
		result = true;
	}
	return result;
}

bool TBaseNPC::isNerveMaybeDontCalcAnim1() const
{
	bool result                       = false;
	const TNerveBase<TLiveActor>* cur = mSpine->getLatestNerve();
	if (cur == &TNerveNPCGraphWait::theNerve()
	    || cur == &TNerveNPCWaitContinue::theNerve()
	    || cur == &TNerveNPCWaitMarioApproach::theNerve()) {
		result = true;
	}
	return result;
}

#pragma dont_inline on
bool TBaseNPC::isNerveCanGoToTalk() const
{
	bool result                       = false;
	const TNerveBase<TLiveActor>* cur = mSpine->getLatestNerve();
	if (cur == &TNerveNPCGraphWander::theNerve()
	    || cur == &TNerveNPCUTurn::theNerve()
	    || cur == &TNerveNPCGraphWait::theNerve()
	    || cur == &TNerveNPCTurnToMario::theNerve()
	    || cur == &TNerveNPCWet::theNerve()
	    || cur == &TNerveNPCRecoverAfter::theNerve()
	    || cur == &TNerveNPCMad::theNerve()
	    || cur == &TNerveNPCMareStand::theNerve()) {
		if (mSpine->getCurrentNerve() != nullptr
		    || (mSpine->peekTopNerveOrNull() != &TNerveNPCWet::theNerve()
		        && mSpine->peekTopNerveOrNull()
		               != &TNerveNPCTalk::theNerve())) {
			result = true;
		}
	}
	return result;
}
#pragma dont_inline off

bool TBaseNPC::isNerveCanGoToMad() const
{
	bool result                       = false;
	const TNerveBase<TLiveActor>* cur = mSpine->getLatestNerve();
	if (cur == &TNerveNPCGraphWander::theNerve()
	    || cur == &TNerveNPCUTurn::theNerve()
	    || cur == &TNerveNPCGraphWait::theNerve()
	    || cur == &TNerveNPCWaitContinue::theNerve()
	    || cur == &TNerveNPCWaitMarioApproach::theNerve()
	    || cur == &TNerveNPCTurnToMario::theNerve()
	    || cur == &TNerveNPCWet::theNerve()) {
		result = true;
	}
	return result;
}

void TBaseNPC::changeNerveFromTalk_()
{
	const TNerveBase<TLiveActor>* cur = mSpine->getCurrentNerve();
	const TNerveBase<TLiveActor>* top = mSpine->peekTopNerveOrNull();

	if (cur == &TNerveNPCWet::theNerve()) {
		TNerveBase<TLiveActor>* popped = mSpine->popNerve();
		if (popped != nullptr)
			mSpine->becomeNerveAfterPop(popped);
	} else if (cur == nullptr && top == &TNerveNPCTalk::theNerve()) {
		TNerveBase<TLiveActor>* popped = mSpine->popNerve();
		if (popped != nullptr)
			mSpine->becomeNerveAfterPop(popped);
	} else {
		TNerveNPCTalk::theNerve();
	}

	mSpine->setNext(nullptr);

	if (unk17C != nullptr) {
		if (checkLiveFlag(LIVE_FLAG_UNK20000000)) {
			offLiveFlag(LIVE_FLAG_UNK20000000);
		} else {
			mSpine->setNext(&TNerveNPCThrow::theNerve());
		}
	}

	mLiveFlag &= ~0x02000000;
}

void TBaseNPC::changeNerveToMad_()
{
	if (mSpine->getCurrentNerve() == &TNerveNPCWet::theNerve()) {
		mSpine->setNext(&TNerveNPCMad::theNerve());
	} else {
		mSpine->pushNerve(&TNerveNPCMad::theNerve());
	}
}

void TBaseNPC::releaseTaken_()
{
	f32 dist           = mPtrSaveNormal->mThrowSpeedXZ.get();
	s16 angle          = CLBRoundf<s16>(mTakenBy->mRotation.y * 182.04445f);
	mVelocity.set(JMASSin(angle) * dist, mPtrSaveNormal->mThrowSpeedY.get(),
	              JMASCos(angle) * dist);
	mLiveFlag         |= 0x10000000;
	unk1DC             = CLBPalFrame<long>(15);
	mTakenBy           = nullptr;
	if (mActorType - 0x04000000 == 0x18) {
		peachTiredIn_();
		mNpcBalloon->setNextMessage(0, -1);
	}
	mSpine->setNext(&TNerveNPCWaitMarioApproach::theNerve());
}

void TBaseNPC::behaveToBeTaken_(THitActor* taker)
{
	s16 selfAngle  = CLBDegToShortAngle(mRotation.y);
	s16 takerAngle = CLBDegToShortAngle(taker->mRotation.y);
	mAngleYDiffWhenTaken = takerAngle - selfAngle;
	*(u32*)((u8*)mUnk18C + 8) = *(u32*)mUnk18C;
	unk64 |= 0x1;
	mLiveFlag &= ~0x00420010;
	if (mActorType - 0x04000000 == 0x18) {
		peachParasolOut_();
		mNpcBalloon->setNextMessage(0xE004F, 0x2EE);
	}
	mSpine->setNext(&TNerveNPCWaitContinue::theNerve());
	mHolder  = (TTakeActor*)taker;
	mTakenBy = taker;
}

void TBaseNPC::behaveToBeTrampled_()
{
	bool isChildFlag = isChild();
	u32 soundId      = 0x8826;
	bool isType      = true;
	if (!isNormalMonteM() && !isSpecialMonteM())
		isType = false;
	if (isType) {
		if (isChildFlag)
			soundId = 0x88AB;
		else
			soundId = 0x8844;
	} else {
		isType = true;
		if (!isNormalMonteW() && !isSpecialMonteW())
			isType = false;
		if (isType) {
			if (isChildFlag)
				soundId = 0x88AC;
			else
				soundId = 0x8845;
		} else {
			isType = true;
			if (!isNormalMareM() && !isSpecialMareM())
				isType = false;
			if (isType) {
				if (isChildFlag) {
					soundId = 0x88AD;
				} else if ((s32)mActorType >= 0x04000012) {
					soundId = 0x8846;
				} else if ((s32)mActorType >= 0x04000010) {
					soundId = 0x88AF;
				}
			} else {
				isType = true;
				if (!isNormalMareW() && !isSpecialMareW())
					isType = false;
				if (isType) {
					if (isChildFlag)
						soundId = 0x88AE;
					else
						soundId = 0x8847;
				} else {
					if (mActorType == 0x04000017) {
						soundId = 0x8848;
					} else if ((s32)mActorType < 0x04000017
					           && (s32)mActorType >= 0x04000016) {
						soundId = 0x8849;
					}
				}
			}
		}
	}

	if (gpMSound->gateCheck(soundId)) {
		MSoundSESystem::MSoundSE::startSoundNpcActor(
		    soundId, (const Vec*)&mPosition, 0, nullptr, 0, 4);
	}

	mNpcTrample->startTrample();

	bool isMareKind = true;
	if (!isNormalMareM() && !isNormalMareW())
		isMareKind = false;
	if (!isMareKind) {
		isMareKind = true;
		if (!isSpecialMareM() && !isSpecialMareW())
			isMareKind = false;
	}
	if (!isMareKind)
		return;
	if (!isBehaveToWaterNpc())
		return;
	if (mActionFlag & 0x4200)
		return;

	s32 kind = *(s32*)((u8*)unkD0 + 0x14);
	if (kind == 0x1B || kind == 7) {
		const TNerveBase<TLiveActor>* cur    = mSpine->getCurrentNerve();
		const TNerveBase<TLiveActor>* latest = mSpine->getLatestNerve();
		if (cur == &TNerveNPCWet::theNerve()) {
			mSpine->pushNerve(&TNerveNPCWet::theNerve());
			mSpine->setNext(nullptr);
		} else if (cur == nullptr
		           && latest == &TNerveNPCWet::theNerve()) {
			mSpine->setNext(&TNerveNPCWet::theNerve());
		}
		return;
	}

	bool transitionable                  = false;
	const TNerveBase<TLiveActor>* latest = mSpine->getLatestNerve();
	if (latest == &TNerveNPCGraphWander::theNerve()
	    || latest == &TNerveNPCUTurn::theNerve()
	    || latest == &TNerveNPCGraphWait::theNerve()
	    || latest == &TNerveNPCWaitContinue::theNerve()
	    || latest == &TNerveNPCWaitMarioApproach::theNerve()
	    || latest == &TNerveNPCTurnToMario::theNerve()
	    || latest == &TNerveNPCTalk::theNerve()) {
		if (mSpine->getCurrentNerve() != nullptr
		    || (mSpine->peekTopNerveOrNull() != &TNerveNPCWet::theNerve()
		        && mSpine->peekTopNerveOrNull()
		               != &TNerveNPCTalk::theNerve())) {
			transitionable = true;
		}
	}
	if (transitionable)
		mSpine->pushNerve(&TNerveNPCWet::theNerve());
}

void TBaseNPC::behaveToHitObject_(THitActor* hitter, EnumHitNpcObjectKind kind)
{
	if (mActionFlag & 0x4000) {
		if (kind != HIT_NPC_OBJECT_KIND_WATER_SPRAY)
			return;
		bool ok  = true;
		bool m12 = ok;
		if (gpMarDirector->unk124 != 1 && gpMarDirector->unk124 != 2)
			m12 = false;
		if (!m12) {
			bool m34 = true;
			if (gpMarDirector->unk124 != 3
			    && gpMarDirector->unk124 != 4)
				m34 = false;
			if (!m34)
				ok = false;
		}
		if (ok)
			return;
		gpMarioParticleManager->emit(0xE7, &hitter->mPosition, 0, nullptr);
		gpMSound->startSoundSet(0x6802, (const Vec*)&mPosition, 0, 0.0f, 0, 0, 4);
		if (gpMSound->gateCheck(0x8837)) {
			MSoundSESystem::MSoundSE::startSoundNpcActor(
			    0x8837, (const Vec*)&mPosition, 0, nullptr, 0, 4);
		}
		mFireScaleMul -= mPtrSaveNormal->mSLFireDecSpeed.get();
		if (mFireScaleMul <= 0.0f) {
			mFireScaleMul = 0.0f;
			mActionFlag &= ~0x4088;
			npcHappyIn(1);
		}
		return;
	}

	if (kind == HIT_NPC_OBJECT_KIND_WATER_SPRAY) {
		bool dirOk = true;
		bool m12   = dirOk;
		if (gpMarDirector->unk124 != 1 && gpMarDirector->unk124 != 2)
			m12 = false;
		if (!m12) {
			bool m34 = true;
			if (gpMarDirector->unk124 != 3
			    && gpMarDirector->unk124 != 4)
				m34 = false;
			if (!m34)
				dirOk = false;
		}
		if (!dirOk) {
			if (isPollutionNpc()) {
				if (mSpine->getCurrentNerve()
				    == &TNerveNPCWet::theNerve()) {
					if (unk178 > 0.0f) {
						unk178 -= mNpcSaveIndividual->mPollutionCleanSpeed.get();
						if (unk178 <= 0.0f) {
							unk178  = 0.0f;
							unk1E0  = 0x3c;
							unk1DA |= 0x2;
						}
					}
				}
			}
		}
	}

	if (mActionFlag & 0x600)
		return;

	bool transitionable                  = false;
	const TNerveBase<TLiveActor>* latest = mSpine->getLatestNerve();
	if (latest == &TNerveNPCGraphWander::theNerve()
	    || latest == &TNerveNPCUTurn::theNerve()
	    || latest == &TNerveNPCGraphWait::theNerve()
	    || latest == &TNerveNPCWaitContinue::theNerve()
	    || latest == &TNerveNPCWaitMarioApproach::theNerve()
	    || latest == &TNerveNPCTurnToMario::theNerve()
	    || latest == &TNerveNPCTalk::theNerve()) {
		if (mSpine->getCurrentNerve() != nullptr
		    || (mSpine->peekTopNerveOrNull() != &TNerveNPCWet::theNerve()
		        && mSpine->peekTopNerveOrNull()
		               != &TNerveNPCTalk::theNerve())) {
			transitionable = true;
		}
	}
	if (!transitionable)
		return;

	if (mActionFlag & 0x800)
		return;

	bool specialBlock = false;
	if (mActorType - 0x04000000 == 0x18 && (unk1D8 & 0x2))
		specialBlock = true;
	if (specialBlock)
		return;

	bool sunBlock = false;
	if (isSunflower() && (unk1D8 & 0x2))
		sunBlock = true;
	if (sunBlock)
		return;

	if (unk178 != 0.0f && kind == HIT_NPC_OBJECT_KIND_UNK1)
		return;

	if (mActorType - 0x04000000 == 0x6) {
		s32 v = *(s32*)((u8*)unkD0 + 0x14);
		if (v != 0x4 && v != 0x6)
			return;
	}

	if (mSpine->getCurrentNerve() == &TNerveNPCTalk::theNerve()
	    && *(s32*)((u8*)mSpine + 0x20) < 0x4)
		return;

	if (kind == HIT_NPC_OBJECT_KIND_UNK1)
		mLiveFlag |= 0x04000000;

	mSpine->pushNerve(&TNerveNPCWet::theNerve());
}

void TBaseNPC::behaveToSandBomb_(const TLiveActor* bomb)
{
	f32 old = unk1C8;
	unk1C8  = SMS_GetSandRiseUpRatio(bomb);
	f32 cur = unk1C8;
	if (cur > old && cur > 0.05f && old > 0.001f) {
		bool ok                             = false;
		const TNerveBase<TLiveActor>* nerve = mSpine->getLatestNerve();
		if (nerve == &TNerveNPCGraphWander::theNerve()
		    || nerve == &TNerveNPCUTurn::theNerve()
		    || nerve == &TNerveNPCGraphWait::theNerve()
		    || nerve == &TNerveNPCWaitContinue::theNerve()
		    || nerve == &TNerveNPCWaitMarioApproach::theNerve()
		    || nerve == &TNerveNPCTurnToMario::theNerve()
			    || nerve == &TNerveNPCWet::theNerve()
			    || nerve == &TNerveNPCMareStand::theNerve()) {
			TMarDirector* director = gpMarDirector;
			bool canBlow = director->isThing();
			if (!canBlow)
				ok = true;
		}
		if (ok) {
			f32 blownVel = mPtrSaveNormal->mSLBlownVelocity.get();
			mPosition.y += blownVel;
			mLiveFlag   |= 0x80;
			JGeometry::TVec3<f32> tmp(0.0f, blownVel, 0.0f);
			mVelocity = tmp;
			if (mSpine->getCurrentNerve() == &TNerveNPCWet::theNerve()) {
				mSpine->setNext(&TNerveNPCBlown::theNerve());
			} else {
				mSpine->pushNerve(&TNerveNPCBlown::theNerve());
			}
		}
	}
}

bool TBaseNPC::isStateGoToMad_() const
{
	bool result = false;
	if (isMadNpc() && !(mActionFlag & 0x4600) && 0.0f == unk178
	    && isInMadSearchRange()) {
		result = true;
	}
	return result;
}

bool TBaseNPC::isNowCanTaken() const
{
	bool result = false;
	u32 flag    = mLiveFlag;
	if ((flag & 0x100000) && mActorType - 0x04000000 != 0x1C
	    && mHolder == nullptr && mHeldObject == nullptr
	    && !(flag & 0x840007) && !(unk64 & 0x1)) {
		bool match = false;
		const TNerveBase<TLiveActor>* cur = mSpine->getLatestNerve();
		if (cur == &TNerveNPCGraphWander::theNerve()
		    || cur == &TNerveNPCUTurn::theNerve()
		    || cur == &TNerveNPCGraphWait::theNerve()
		    || cur == &TNerveNPCWaitContinue::theNerve()
		    || cur == &TNerveNPCWaitMarioApproach::theNerve()
		    || cur == &TNerveNPCTurnToMario::theNerve()
		    || cur == &TNerveNPCWet::theNerve()
		    || cur == &TNerveNPCSink::theNerve()
		    || cur == &TNerveNPCRecoverFromSink::theNerve()
		    || cur == &TNerveNPCRecoverAfter::theNerve()) {
			match = true;
		}
		if (match)
			result = true;
	}
	return result;
}

void TBaseNPC::changeNerveProc_()
{
	bool earlyExit                       = false;
	const TNerveBase<TLiveActor>* latest = mSpine->getLatestNerve();
	if (latest == &TNerveNPCTalk::theNerve()) {
		mLiveFlag   |= 0x20000;
		earlyExit    = true;
		mLiveFlag   &= ~0x40000;
	} else {
		bool doTransition = false;
		if (mLiveFlag & 0x40000) {
			doTransition = true;
		} else if (unk1E0 == 0 && !isJellyFishMare()
		           && !gpCamera->isTalkCameraInbetween()
		           && mHolder == nullptr && !(mLiveFlag & 0xC10207)
		           && !(mActionFlag & 0x4000) && unk178 == 0.0f) {
			bool sunflowerWaiting = false;
			if (isSunflower() && (unk1D8 & 0x2))
				sunflowerWaiting = true;
			if (!sunflowerWaiting && isNerveCanGoToTalk()) {
				bool actorTypeOk = true;
				if (mActorType - 0x04000000 == 0x6) {
					if (*(s32*)((u8*)unkD0 + 0x14) != 0x4)
						actorTypeOk = false;
				}
				if (actorTypeOk && !SMS_IsMarioOpeningDoor()) {
					bool inCube = true;
					if (gpMarDirector->mMap == 0x7) {
						JGeometry::TVec3<f32> aboveHead(mPosition.x,
						                                mPosition.y + 75.0f,
						                                mPosition.z);
						if (!SMS_IsInSameCameraCube((const Vec&)aboveHead))
							inCube = false;
					}
					if (inCube) {
						f32 maxDist, sightHeight;
						if (unk17C != nullptr) {
							sightHeight = mPtrSaveNormal->mSLThrowTalkAcceptHeight.get();
							maxDist     = mPtrSaveNormal->mSLThrowTalkAcceptDist.get();
						} else if (mActorType - 0x04000000 == 0x1A) {
							maxDist     = mPtrSaveNormal->mSLSunflowerLTalkDist.get();
							sightHeight = mPtrSaveNormal->mTalkAcceptHeight.get();
						} else {
							maxDist     = mPtrSaveNormal->mTalkAcceptDist.get();
							sightHeight = mPtrSaveNormal->mTalkAcceptHeight.get();
						}
						f32 zRad;
						if ((mActionFlag & 0x401) || isSunflower()
						    || mActorType - 0x04000000 == 0x1D)
							zRad = mPtrSaveNormal->mSLSitTalkAcceptDegree.get();
						else
							zRad = mPtrSaveNormal->mTalkAcceptDegree.get();

						f32 diffY = gpMarioPos->y - mPosition.y;
						f32 absY  = fabsf(diffY);
						if (absY < sightHeight
						    && isInSight(*gpMarioPos, maxDist, zRad, -1.0f)
						    && MsIsInSight(
						           mPosition,
						           (f32)(*gpMarioAngleY ^ 0x8000)
						               * 0.005493164f,
						           *gpMarioPos, maxDist,
						           mPtrSaveNormal->mSLMarioTalkAcceptDegree.get(),
						           0.0f)) {
							doTransition = true;
						}
					}
				}
			}
		}
		if (doTransition) {
			onLiveFlag(LIVE_FLAG_UNK20000);
			if (checkLiveFlag(LIVE_FLAG_UNK40000)) {
				earlyExit = true;
				offLiveFlag(LIVE_FLAG_UNK40000);
				const TNerveBase<TLiveActor>* c
				    = mSpine->getCurrentNerve();
				if (c == &TNerveNPCWet::theNerve()
				    || c == &TNerveNPCRecoverAfter::theNerve()
				    || c == &TNerveNPCMad::theNerve()) {
					mSpine->setNext(&TNerveNPCTalk::theNerve());
				} else {
					mSpine->pushNerve(&TNerveNPCTalk::theNerve());
				}
				offLiveFlag(LIVE_FLAG_UNK2000000);
			}
		} else {
			mLiveFlag &= ~0x60000;
			mLiveFlag &= ~0x40000;
		}
	}

	if (earlyExit)
		return;
	if (!isPollutionNpc())
		return;
	if (mActionFlag & 0x4600)
		return;
	if (mSinkTimer == nullptr)
		return;
	if (latest == &TNerveNPCSetPosAfterSinkBottom::theNerve())
		return;

	if (!mSinkTimer->advance())
		return;
	mSinkTimer->mCounter = 0;

	if (latest == &TNerveNPCSink::theNerve()) {
		if (gpPollution->isPolluted(mPosition.x, mSinkBaseY, mPosition.z))
			return;
		mSpine->setNext(&TNerveNPCRecoverFromSink::theNerve());
		unk64     &= ~0x1;
		mLiveFlag &= ~0x00800000;
		requestNpcAnm_((EnumNpcAnmKind)0x1A,
		               (EnumNpcStopMotionBlendOnOff)0x1);
		if (gpMSound->gateCheck(0x3811)) {
			MSoundSESystem::MSoundSE::startSoundNpcActor(
			    0x3811, (const Vec*)&mPosition, 0, nullptr, 0, 4);
		}
		return;
	}

	if (mLiveFlag & 0x00400000)
		return;
	if (mLiveFlag & 0x80)
		return;
	if (!gpPollution->isPolluted(mPosition.x, mPosition.y, mPosition.z))
		return;

	bool canSink                    = false;
	const TNerveBase<TLiveActor>* c = mSpine->getLatestNerve();
	if (c == &TNerveNPCGraphWander::theNerve()
	    || c == &TNerveNPCUTurn::theNerve()
	    || c == &TNerveNPCGraphWait::theNerve()
	    || c == &TNerveNPCWaitContinue::theNerve()
	    || c == &TNerveNPCWaitMarioApproach::theNerve()
	    || c == &TNerveNPCTurnToMario::theNerve()) {
		if (mSpine->getCurrentNerve() != nullptr
		    || mSpine->peekTopNerveOrNull() != &TNerveNPCSink::theNerve()) {
			canSink = true;
		}
	}
	if (!canSink)
		return;

	mSinkBaseY   = mPosition.y;
	mVelocity.x  = 0.0f;
	mVelocity.y  = 0.0f;
	mVelocity.z  = 0.0f;
	mLiveFlag   &= 0xF7FDFFFF;
	mLiveFlag   |= 0x00400010;
	npcFallIn();
	mSpine->pushNerve(&TNerveNPCSink::theNerve());
}

void TBaseNPC::setPosAndInitAfterSinkBottom()
{
	JGeometry::TVec3<f32> resetPos
	    = *(const JGeometry::TVec3<f32>*)((u8*)this + 0x194);
	bool polluted
	    = gpPollution->isPolluted(resetPos.x, resetPos.y, resetPos.z);
	mLiveFlag    &= 0xF539FFE0;
	mLiveFlag    |= 0x01000000;
	mHolder       = nullptr;
	mHeldObject   = nullptr;
	if (mSinkTimer)
		mSinkTimer->mCounter = 0;

	mHitPoints = getSaveParam() ? getSaveParam()->mSLHitPointMax.get() : 1;

	if (mMultiMtxEffect != nullptr) {
		s32 i = 0;
		while (i < *(u16*)mMultiMtxEffect) {
			void* p = (*(void***)((u8*)mMultiMtxEffect + 0x10))[i];
			*(u16*)((u8*)p + 0x4) |= 0x2;
			i++;
		}
	}

	*(s32*)((u8*)mSpine + 0x8)        = 0;
	*(s32*)((u8*)mUnk18C + 0x24)      = 0;
	*(f32*)((u8*)mUnk18C + 0x28)      = 0.0f;
	**(s32**)((u8*)this + 0x190)      = -1;
	unk1CC                             = 0;
	unk1D0                             = 0.0f;

	if (polluted && isPollutionNpc() && !(mActionFlag & 0x400)) {
		unk64 |= 0x1;
		mSpine->setNext(*(const TNerveBase<TLiveActor>**)((u8*)mSpine + 0x18));
		mSpine->pushNerve(&TNerveNPCSink::theNerve());
		mSpine->pushNerve(&TNerveNPCSink::theNerve());

		mLiveFlag |= 0x00C00010;
		mGroundHeight = gpMap->checkGroundIgnoreWaterSurface(
		    resetPos.x, resetPos.y + mBodyScale * mHeadHeight, resetPos.z,
		    &mGroundPlane);
		mSinkBaseY  = mGroundHeight;
		resetPos.y  = mSinkBaseY - mNpcSaveIndividual->mSinkHeight.value;
		mVelocity.x   = 0.0f;
		mVelocity.y   = 0.0f;
		mVelocity.z   = 0.0f;
	} else {
		unk64 &= ~0x1;
		mSpine->setNext(*(const TNerveBase<TLiveActor>**)((u8*)mSpine + 0x18));
		mSpine->pushNerve(
		    *(const TNerveBase<TLiveActor>**)((u8*)mSpine + 0x18));
		resetPos.y += 50.0f;
		mVelocity.x = 0.0f;
		mVelocity.y = 5.0f;
		mVelocity.z = 0.0f;
		mLiveFlag |= 0x8000;
		mLiveFlag |= 0x80;
	}

	mPosition.x = resetPos.x;
	mPosition.y = resetPos.y;
	mPosition.z = resetPos.z;
	mRotation.x = *(f32*)((u8*)this + 0x1A0);
	mRotation.y = *(f32*)((u8*)this + 0x1A4);
	mRotation.z = *(f32*)((u8*)this + 0x1A8);
	mLinearVelocity.x = 0.0f;
	mLinearVelocity.y = 0.0f;
	mLinearVelocity.z = 0.0f;
	*(s32*)((u8*)unk124 + 0x8) = -1;
	*(s32*)((u8*)unk124 + 0x4) = -1;
	goToShortestNextGraphNode();
	npcWaitIn();
	randomizeBckAndBtpFrame_();
}

void TBaseNPC::kill() { }
