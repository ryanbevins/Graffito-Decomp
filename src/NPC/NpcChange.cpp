// NPC/NpcChange.cpp -- partial decomp.

#include <NPC/NpcBase.hpp>
#include <NPC/NpcNerve.hpp>
#include <Strategic/Spine.hpp>

void TBaseNPC::kill() { }

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
