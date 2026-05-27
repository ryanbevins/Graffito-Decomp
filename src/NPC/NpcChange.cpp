// NPC/NpcChange.cpp -- partial decomp.

#include <Camera/Camera.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JMath.hpp>
#include <MSound/MSound.hpp>
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

BOOL TBaseNPC::isNerveWalk() const
{
	BOOL result                       = FALSE;
	const TNerveBase<TLiveActor>* cur = mSpine->getLatestNerve();
	if (cur == &TNerveNPCGraphWander::theNerve()
	    || cur == &TNerveNPCUTurn::theNerve()
	    || cur == &TNerveNPCGraphWait::theNerve()) {
		result = TRUE;
	}
	return result;
}

BOOL TBaseNPC::isNerveMaybeDontMovement() const
{
	BOOL result                       = FALSE;
	const TNerveBase<TLiveActor>* cur = mSpine->getLatestNerve();
	if (cur == &TNerveNPCWaitContinue::theNerve()
	    || cur == &TNerveNPCWaitMarioApproach::theNerve()
	    || cur == &TNerveNPCSink::theNerve()) {
		result = TRUE;
	}
	return result;
}

BOOL TBaseNPC::isNerveMaybeDontCalcAnim0() const
{
	BOOL result                       = FALSE;
	const TNerveBase<TLiveActor>* cur = mSpine->getLatestNerve();
	if (cur == &TNerveNPCWaitContinue::theNerve()
	    || cur == &TNerveNPCWaitMarioApproach::theNerve()) {
		result = TRUE;
	}
	return result;
}

BOOL TBaseNPC::isNerveMaybeDontCalcAnim1() const
{
	BOOL result                       = FALSE;
	const TNerveBase<TLiveActor>* cur = mSpine->getLatestNerve();
	if (cur == &TNerveNPCGraphWait::theNerve()
	    || cur == &TNerveNPCWaitContinue::theNerve()
	    || cur == &TNerveNPCWaitMarioApproach::theNerve()) {
		result = TRUE;
	}
	return result;
}

BOOL TBaseNPC::isNerveCanGoToTalk() const
{
	BOOL result                       = FALSE;
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
			result = TRUE;
		}
	}
	return result;
}

BOOL TBaseNPC::isNerveCanGoToMad() const
{
	BOOL result                       = FALSE;
	const TNerveBase<TLiveActor>* cur = mSpine->getLatestNerve();
	if (cur == &TNerveNPCGraphWander::theNerve()
	    || cur == &TNerveNPCUTurn::theNerve()
	    || cur == &TNerveNPCGraphWait::theNerve()
	    || cur == &TNerveNPCWaitContinue::theNerve()
	    || cur == &TNerveNPCWaitMarioApproach::theNerve()
	    || cur == &TNerveNPCTurnToMario::theNerve()
	    || cur == &TNerveNPCWet::theNerve()) {
		result = TRUE;
	}
	return result;
}

void TBaseNPC::behaveToHitObject_(THitActor* hitter, EnumHitNpcObjectKind kind)
{
	if (mActionFlag & 0x4000) {
		if ((s32)kind != 0)
			return;
		bool m12 = true;
		u8 mode  = gpMarDirector->unk124;
		if (mode != 1 && mode != 2)
			m12 = false;
		bool ok = true;
		if (!m12) {
			u8 m2 = gpMarDirector->unk124;
			if (m2 != 3 && m2 != 4)
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
		if (mFireScaleMul > 0.0f)
			return;
		mFireScaleMul = 0.0f;
		mActionFlag &= ~0x4089;
		npcHappyIn(1);
		return;
	}

	if ((s32)kind == 0) {
		bool m12 = true;
		u8 mode  = gpMarDirector->unk124;
		if (mode != 1 && mode != 2)
			m12 = false;
		bool dirOk = true;
		if (!m12) {
			u8 m2 = gpMarDirector->unk124;
			if (m2 != 3 && m2 != 4)
				dirOk = false;
		}
		if (!dirOk) {
			if (isPollutionNpc()) {
				if (mSpine->getCurrentNerve()
				    == &TNerveNPCWet::theNerve()) {
					if (unk178 > 0.0f) {
						unk178 -= mPtrSaveNormal->mSLCleanEffectScale.get();
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

	if (mActionFlag & 0x400)
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

	if (unk178 != 0.0f && (s32)kind == 1)
		return;

	if (mActorType - 0x04000000 == 0x6) {
		s32 v = *(s32*)((u8*)unkD0 + 0x14);
		if (v != 0x4 && v != 0x6)
			return;
	}

	if (mSpine->getCurrentNerve() == &TNerveNPCTalk::theNerve()
	    && *(s32*)((u8*)mSpine + 0x20) < 0x4)
		return;

	if ((s32)kind == 1)
		mActionFlag |= 0x400;

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
			bool m1   = true;
			u8 mode   = gpMarDirector->unk124;
			if (mode != 1 && mode != 2)
				m1 = false;
			bool dirOk = true;
			if (!m1) {
				if (gpMarDirector->unk124 != 4)
					dirOk = false;
			}
			if (dirOk)
				ok = true;
		}
		if (ok) {
			mPosition.y += mPtrSaveNormal->mSLBlownVelocity.get();
			mLiveFlag   |= 0x80;
			JGeometry::TVec3<f32> tmp(0.0f,
			                          mPtrSaveNormal->mSLBlownVelocity.get(),
			                          0.0f);
			mAngularVelocity = tmp;
			if (mSpine->getCurrentNerve() == &TNerveNPCWet::theNerve()) {
				mSpine->setNext(&TNerveNPCBlown::theNerve());
			} else {
				mSpine->pushNerve(&TNerveNPCBlown::theNerve());
			}
		}
	}
}

void TBaseNPC::releaseTaken_()
{
	s16 angle    = CLBRoundf<s16>(mTakenBy->mRotation.y * 182.04445f);
	f32 dist     = mPtrSaveNormal->mThrowSpeedXZ.get();
	mAngularVelocity.x = JMASSin(angle) * dist;
	mAngularVelocity.y = mPtrSaveNormal->mThrowSpeedY.get();
	mAngularVelocity.z = JMASCos(angle) * dist;
	mLiveFlag |= 0x10000000;
	unk1DC = CLBPalFrame<long>(15);
	mTakenBy = nullptr;
	if (mActorType - 0x04000000 == 0x18) {
		peachTiredIn_();
		mNpcBalloon->setNextMessage(0, -1);
	}
	mSpine->setNext(&TNerveNPCWaitMarioApproach::theNerve());
}

void TBaseNPC::behaveToBeTaken_(THitActor* taker)
{
	mAngleYDiffWhenTaken
	    = (s16)CLBRoundf<s16>(mRotation.y * 182.04445f)
	      - (s16)CLBRoundf<s16>(taker->mRotation.y * 182.04445f);
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
		if (mLiveFlag & 0x20000000) {
			mLiveFlag &= ~0x20000000;
		} else {
			mSpine->pushNerve(&TNerveNPCThrow::theNerve());
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

bool TBaseNPC::isNowCanTaken() const
{
	bool result = false;
	u32 flag    = mLiveFlag;
	if ((flag & 0x100000) && mActorType - 0x04000000 != 0x1C
	    && mHolder == nullptr && mHeldObject == nullptr
	    && !(flag & 0x840007) && !(unk64 & 0x1)) {
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
			result = true;
		}
	}
	return result;
}

BOOL TBaseNPC::isStateGoToMad_() const
{
	BOOL result = FALSE;
	if (isMadNpc() && !(mActionFlag & 0x4600) && 0.0f == unk178
	    && isInMadSearchRange()) {
		result = TRUE;
	}
	return result;
}

void TBaseNPC::kill() { }
