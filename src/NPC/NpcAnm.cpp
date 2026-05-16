// NPC/NpcAnm.cpp -- partial decomp.

#include <NPC/NpcBase.hpp>
#include <Strategic/LiveActor.hpp>

BOOL TBaseNPC::npcBlowning()
{
	BOOL flag;
	if (mLiveFlag & LIVE_FLAG_AIRBORNE)
		flag = TRUE;
	else
		flag = FALSE;
	if (flag)
		return FALSE;
	return TRUE;
}
