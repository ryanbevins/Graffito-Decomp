#include "Enemy/PathNode.hpp"
#include "Strategic/SolidStack.hpp"
#include <Strategic/Spine.hpp>
#include <NPC/NpcNerve.hpp>
#include <NPC/NpcBase.hpp>

DEFINE_NERVE(TNerveNPCGraphWander, TLiveActor) { }

DEFINE_NERVE(TNerveNPCUTurn, TLiveActor) { }

DEFINE_NERVE(TNerveNPCGraphWait, TLiveActor) { }

DEFINE_NERVE(TNerveNPCWaitContinue, TLiveActor) { }

DEFINE_NERVE(TNerveNPCWaitMarioApproach, TLiveActor) { }

DEFINE_NERVE(TNerveNPCTurnToMario, TLiveActor) { }

DEFINE_NERVE(TNerveNPCWet, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (spine->getTime() == 0) {
		npc->npcWetIn();
	}
	if (npc->npcWetting())
		return TRUE;
	return FALSE;
}

DEFINE_NERVE(TNerveNPCSink, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	npc->npcSinking();
	return FALSE;
}

DEFINE_NERVE(TNerveNPCRecoverFromSink, TLiveActor) { }

DEFINE_NERVE(TNerveNPCRecoverAfter, TLiveActor) { }

DEFINE_NERVE(TNerveNPCSetPosAfterSinkBottom, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	npc->setPosAndInitAfterSinkBottom();
	return TRUE;
}

DEFINE_NERVE(TNerveNPCTalk, TLiveActor) { }

DEFINE_NERVE(TNerveNPCThrow, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (spine->getTime() == 0) {
		npc->npcThrowIn();
	}
	if (npc->npcThrowing())
		return TRUE;
	return FALSE;
}

DEFINE_NERVE(TNerveNPCMad, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (spine->getTime() == 0) {
		npc->npcMadIn();
	}
	if (npc->npcMadding())
		return TRUE;
	return FALSE;
}

DEFINE_NERVE(TNerveNPCBlown, TLiveActor) { }

DEFINE_NERVE(TNerveNPCMareStand, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (spine->getTime() == 0) {
		npc->npcMareStandIn();
	}
	if (npc->npcMareStanding())
		return TRUE;
	return FALSE;
}
