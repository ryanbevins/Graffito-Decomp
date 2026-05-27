// NPC/NpcChange.cpp -- partial decomp.

#include <Camera/cameralib.hpp>
#include <JSystem/JMath.hpp>
#include <NPC/NpcBalloon.hpp>
#include <NPC/NpcBase.hpp>
#include <NPC/NpcNerve.hpp>
#include <NPC/NpcSave.hpp>
#include <Strategic/Spine.hpp>

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

void TBaseNPC::changeNerveToMad_()
{
	if (mSpine->getCurrentNerve() == &TNerveNPCWet::theNerve()) {
		mSpine->setNext(&TNerveNPCMad::theNerve());
	} else {
		mSpine->pushNerve(&TNerveNPCMad::theNerve());
	}
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
