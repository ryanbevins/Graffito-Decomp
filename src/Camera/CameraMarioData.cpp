#include <Camera/CameraMarioData.hpp>
#include <Map/MapData.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioMain.hpp>
#include <Player/WaterGun.hpp>
#include <Player/NozzleBase.hpp>
#include <Strategic/HitActor.hpp>
#include <Camera/cameralib.hpp>

TCameraMarioData::TCameraMarioData()
{
	mPosX             = 0.0f;
	mPosY             = 0.0f;
	mPosZ             = 0.0f;
	mDistXZ           = 0.0f;
	mDistY            = 0.0f;
	mStatus           = 0;
	mStatusTimer      = 0;
	mNozzleAngleRatio = 0.0f;
}

bool TCameraMarioData::isMarioClimb(u32 status) const
{
	bool result = false;
	switch (status) {
	case 0x18100340:
	case 0x10100341:
	case 0x10100342:
	case 0x10100343:
		result = true;
	}
	return result;
}

bool TCameraMarioData::isMarioLeanMirror() const
{
	bool result = false;
	if (SMS_GetMarioGrPlane() != nullptr) {
		const TLiveActor* actor = SMS_GetMarioGrPlane()->mActor;
		if (actor != nullptr) {
			if (((const THitActor*)actor)->mActorType == 0x400000CF)
				result = true;
		}
	}
	return result;
}

bool TCameraMarioData::isMarioSlider() const
{
	bool result = false;
	if (SMS_GetMarioGrPlane() != nullptr) {
		u16 type = SMS_GetMarioGrPlane()->mBGType;
		bool match;
		if (type == 0xC || type == 0x800C || type == 0xA00C) {
			match = true;
		} else {
			match = false;
		}
		result = match;
	}
	return result;
}

bool TCameraMarioData::isMarioIndoor() const
{
	bool result = false;
	if (SMS_GetMarioGrPlane() != nullptr) {
		u16 type = SMS_GetMarioGrPlane()->mBGType;
		bool match;
		if (type == 0x106 || type == 0x105 || (u16)(type - 0x108) <= 1) {
			match = true;
		} else {
			match = false;
		}
		result = match;
	}
	return result;
}

bool TCameraMarioData::isMarioRocketing() const
{
	bool result = false;
	switch (SMS_GetMarioStatus()) {
	case 0x88B:
	case 0x88D:
		result = true;
	}
	return result;
}

bool TCameraMarioData::isMarioGoDown() const
{
	bool result = false;
	if (mDistY != 0.0f
	    && gpMarioPos->y - gpMarioOriginal->mLastSafePos.y < 0.0f) {
		result = true;
	}
	return result;
}

void TCameraMarioData::calcAndSetMarioData()
{
	u32 status = SMS_GetMarioStatus();
	if (status == 0x3800034B || status == 0x3000054C) {
		mDistXZ = 0.0f;
		mDistY  = 0.0f;
	} else {
		f32 dx  = gpMarioPos->x - gpMarioOriginal->mLastSafePos.x;
		f32 dy  = gpMarioPos->y - gpMarioOriginal->mLastSafePos.y;
		f32 dz  = gpMarioPos->z - gpMarioOriginal->mLastSafePos.z;
		mDistXZ = dx * dx + dz * dz;
		mDistY  = dy * dy;
		if (mDistXZ > 1.0e9f)
			mDistXZ = 1.0e9f;
		if (mDistY > 1.0e9f)
			mDistY = 1.0e9f;
	}

	if (mStatus != status) {
		mStatus      = status;
		mStatusTimer = 0;
	} else {
		mStatusTimer++;
	}

	// TODO: nozzle angle ratio uses a virtual method via vtable at +0x364
	mNozzleAngleRatio = 0.0f;
}
