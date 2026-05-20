#include <NPC/NpcBase.hpp>

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

BOOL TBaseNPC::isPollutionNpc() const
{
	BOOL result = FALSE;
	switch (mActorType) {
	case 0x04000001:
	case 0x04000002:
	case 0x04000004:
	case 0x0400000A:
	case 0x0400000B:
	case 0x0400000E:
	case 0x04000013:
	case 0x04000016:
		result = TRUE;
	}
	return result;
}

BOOL TBaseNPC::isChild() const
{
	BOOL result = FALSE;
	if (mScaling.x < 0.7f && mScaling.y < 0.7f && mScaling.z < 0.7f)
		result = TRUE;
	return result;
}

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
