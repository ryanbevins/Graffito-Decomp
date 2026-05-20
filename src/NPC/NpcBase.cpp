#include <NPC/NpcBase.hpp>

BOOL TBaseNPC::isSunflower() const
{
	BOOL result = FALSE;
	if ((s32)mActorType < 0x0400001C && (s32)mActorType >= 0x0400001A)
		result = TRUE;
	return result;
}

BOOL TBaseNPC::isJellyFishMare() const
{
	BOOL result = FALSE;
	switch (mActorType) {
	case 0x0400000F:
	case 0x04000014:
		result = TRUE;
	}
	return result;
}

BOOL TBaseNPC::isSpecialMareW() const
{
	BOOL result = FALSE;
	if ((s32)mActorType < 0x04000016 && (s32)mActorType >= 0x04000014)
		result = TRUE;
	return result;
}

BOOL TBaseNPC::isSpecialMareM() const
{
	BOOL result = FALSE;
	if ((s32)mActorType < 0x04000013 && (s32)mActorType >= 0x0400000F)
		result = TRUE;
	return result;
}

BOOL TBaseNPC::isNormalMareW() const
{
	BOOL result = FALSE;
	switch (mActorType) {
	case 0x04000013:
		result = TRUE;
	}
	return result;
}

BOOL TBaseNPC::isNormalMareM() const
{
	BOOL result = FALSE;
	switch (mActorType) {
	case 0x0400000E:
		result = TRUE;
	}
	return result;
}

BOOL TBaseNPC::isSpecialMonteW() const
{
	BOOL result = FALSE;
	switch (mActorType) {
	case 0x0400000D:
		result = TRUE;
	}
	return result;
}

BOOL TBaseNPC::isSpecialMonteM() const
{
	BOOL result = FALSE;
	if ((s32)mActorType < 0x0400000A && (s32)mActorType >= 0x04000006)
		result = TRUE;
	return result;
}

BOOL TBaseNPC::isNormalMonteW() const
{
	BOOL result = FALSE;
	if ((s32)mActorType < 0x0400000D && (s32)mActorType >= 0x0400000A)
		result = TRUE;
	return result;
}

BOOL TBaseNPC::isNormalMonteM() const
{
	BOOL result = FALSE;
	if ((s32)mActorType < 0x04000006 && (s32)mActorType >= 0x04000001)
		result = TRUE;
	return result;
}
