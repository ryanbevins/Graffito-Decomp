// NPC/NpcChange.cpp -- partial decomp.

#include <NPC/NpcBase.hpp>
#include <NPC/NpcNerve.hpp>
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
