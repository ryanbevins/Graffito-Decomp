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
	if (status == 0x18100340)
		return true;
	if (status >= 0x10100344)
		return false;
	if (status < 0x10100341)
		return false;
	return true;
}

bool TCameraMarioData::isMarioLeanMirror() const
{
	if (SMS_GetMarioGrPlane() == nullptr)
		return false;
	const TLiveActor* actor = SMS_GetMarioGrPlane()->mActor;
	if (actor == nullptr)
		return false;
	return ((const THitActor*)actor)->mActorType == 0x400000CF;
}

bool TCameraMarioData::isMarioSlider() const
{
	if (SMS_GetMarioGrPlane() == nullptr)
		return false;
	u16 type = SMS_GetMarioGrPlane()->mBGType;
	return type == 0xC || type == 0x800C || type == 0xA00C;
}

bool TCameraMarioData::isMarioIndoor() const
{
	if (SMS_GetMarioGrPlane() == nullptr)
		return false;
	u16 type = SMS_GetMarioGrPlane()->mBGType;
	if (type == 0x106 || type == 0x105)
		return true;
	u16 offset = (u16)(type - 0x108);
	return offset <= 1;
}

bool TCameraMarioData::isMarioRocketing() const
{
	u32 status = SMS_GetMarioStatus();
	if (status == 0x88C)
		return true;
	if (status >= 0x88E)
		return false;
	if (status >= 0x88B)
		return true;
	return false;
}

bool TCameraMarioData::isMarioGoDown() const
{
	if (mDistY != 0.0f) {
		f32 diff = gpMarioPos->y - gpMarioOriginal->mLastSafePos.y;
		if (diff < 0.0f)
			return true;
	}
	return false;
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
