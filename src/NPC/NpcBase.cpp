#include <NPC/NpcBase.hpp>

BOOL TBaseNPC::isSunflower() const
{
	if ((s32)mActorType >= 0x0400001C)
		return FALSE;
	if ((s32)mActorType < 0x0400001A)
		return FALSE;
	return TRUE;
}

BOOL TBaseNPC::isSpecialMareW() const
{
	if ((s32)mActorType >= 0x04000016)
		return FALSE;
	if ((s32)mActorType < 0x04000014)
		return FALSE;
	return TRUE;
}

BOOL TBaseNPC::isSpecialMareM() const
{
	if ((s32)mActorType >= 0x04000013)
		return FALSE;
	if ((s32)mActorType < 0x0400000F)
		return FALSE;
	return TRUE;
}

BOOL TBaseNPC::isNormalMareW() const
{
	if (mActorType != 0x04000013)
		return FALSE;
	return TRUE;
}

BOOL TBaseNPC::isNormalMareM() const
{
	if (mActorType != 0x0400000E)
		return FALSE;
	return TRUE;
}

BOOL TBaseNPC::isSpecialMonteW() const
{
	if (mActorType != 0x0400000D)
		return FALSE;
	return TRUE;
}

BOOL TBaseNPC::isSpecialMonteM() const
{
	if ((s32)mActorType >= 0x0400000A)
		return FALSE;
	if ((s32)mActorType < 0x04000006)
		return FALSE;
	return TRUE;
}

BOOL TBaseNPC::isNormalMonteW() const
{
	if ((s32)mActorType >= 0x0400000D)
		return FALSE;
	if ((s32)mActorType < 0x0400000A)
		return FALSE;
	return TRUE;
}

BOOL TBaseNPC::isNormalMonteM() const
{
	if ((s32)mActorType >= 0x04000006)
		return FALSE;
	if ((s32)mActorType < 0x04000001)
		return FALSE;
	return TRUE;
}
