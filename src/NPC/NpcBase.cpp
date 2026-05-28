#include <NPC/NpcBase.hpp>
#include <NPC/NpcSave.hpp>
#include <Player/MarioAccess.hpp>
#include <math.h>

#pragma dont_inline on

BOOL TBaseNPC::isInBodyTurnSearchRange() const
{
	BOOL result = FALSE;
	if (__fabsf(gpMarioPos->y - mPosition.y)
	    < mNpcSaveIndividual->mBodyTurnSearchHeight.value) {
		if (isInSight(*gpMarioPos, mNpcSaveIndividual->mBodyTurnSearchDist.value,
		              mNpcSaveIndividual->mBodyTurnSearchDegree.value,
		              mNpcSaveIndividual->mBodyTurnSearchAware.value)) {
			result = TRUE;
		}
	}
	return result;
}

bool TBaseNPC::isInMadSearchRange() const
{
	bool result = false;
	if (__fabsf(gpMarioPos->y - mPosition.y)
	    < mNpcSaveIndividual->mMadSearchHeight.value) {
		if (isInSight(*gpMarioPos, mNpcSaveIndividual->mMadSearchDist.value,
		              mNpcSaveIndividual->mMadSearchDegree.value,
		              mNpcSaveIndividual->mMadSearchAware.value)) {
			result = true;
		}
	}
	return result;
}

bool TBaseNPC::isMadNpc() const
{
	bool result  = false;
	bool partA   = false;
	BOOL helper  = TRUE;
	switch (mActorType) {
	case 0x04000001:
	case 0x04000002:
	case 0x04000003:
	case 0x04000004:
	case 0x04000005:
		partA = true;
	}
	if (!partA) {
		if (!isNormalMonteW())
			helper = FALSE;
	}
	if (helper) {
		result = true;
	} else {
		switch (mActorType) {
		case 0x04000006:
		case 0x04000007:
		case 0x0400000D:
			result = true;
		}
	}
	return result;
}

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

bool TBaseNPC::isPollutionNpc() const
{
	bool result = false;
	switch (mActorType) {
	case 0x04000001:
	case 0x04000002:
	case 0x04000004:
	case 0x0400000A:
	case 0x0400000B:
	case 0x0400000E:
	case 0x04000013:
	case 0x04000016:
		result = true;
	}
	return result;
}

bool TBaseNPC::isChild() const
{
	bool result = false;
	if (mScaling.x < 0.7f && mScaling.y < 0.7f && mScaling.z < 0.7f)
		result = true;
	return result;
}

bool TBaseNPC::isSunflower() const
{
	bool result = false;
	if ((s32)mActorType < 0x0400001C && (s32)mActorType >= 0x0400001A)
		result = true;
	return result;
}

bool TBaseNPC::isJellyFishMare() const
{
	bool result = false;
	switch (mActorType) {
	case 0x0400000F:
	case 0x04000014:
		result = true;
	}
	return result;
}

bool TBaseNPC::isSpecialMareW() const
{
	bool result = false;
	if ((s32)mActorType < 0x04000016 && (s32)mActorType >= 0x04000014)
		result = true;
	return result;
}

bool TBaseNPC::isSpecialMareM() const
{
	bool result = false;
	if ((s32)mActorType < 0x04000013 && (s32)mActorType >= 0x0400000F)
		result = true;
	return result;
}

bool TBaseNPC::isNormalMareW() const
{
	bool result = false;
	switch (mActorType) {
	case 0x04000013:
		result = true;
	}
	return result;
}

bool TBaseNPC::isNormalMareM() const
{
	bool result = false;
	switch (mActorType) {
	case 0x0400000E:
		result = true;
	}
	return result;
}

bool TBaseNPC::isSpecialMonteW() const
{
	bool result = false;
	switch (mActorType) {
	case 0x0400000D:
		result = true;
	}
	return result;
}

bool TBaseNPC::isSpecialMonteM() const
{
	bool result = false;
	if ((s32)mActorType < 0x0400000A && (s32)mActorType >= 0x04000006)
		result = true;
	return result;
}

bool TBaseNPC::isNormalMonteW() const
{
	bool result = false;
	if ((s32)mActorType < 0x0400000D && (s32)mActorType >= 0x0400000A)
		result = true;
	return result;
}

bool TBaseNPC::isNormalMonteM() const
{
	bool result = false;
	if ((s32)mActorType < 0x04000006 && (s32)mActorType >= 0x04000001)
		result = true;
	return result;
}
