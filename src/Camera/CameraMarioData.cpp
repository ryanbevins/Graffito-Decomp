#include <Camera/CameraMarioData.hpp>
#include <Map/MapData.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/HitActor.hpp>
#include <Camera/cameralib.hpp>

extern void* gpMarioOriginal;

class TNozzleBase {
public:
	struct TParamS16Proxy {
		u8 _0[0x10];
		s16 value;
	};

	struct TEmitParams {
		u8 _0[0x1AC];
		TParamS16Proxy mLAngleMin;
	} mEmitParams;
	u8 _1C0[0x1A4];

	virtual void init();
	virtual s32 getNozzleKind() const;
	virtual s16 getGunAngle();
};

class TWaterGun {
public:
	TNozzleBase* getCurrentNozzle() const;
};

template <> f32 CLBCalcRatio<s16>(s16, s16, s16);

TCameraMarioData* gpCameraMario;

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
		result = SMS_GetMarioGrPlane()->isIndoors();
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
	    && gpMarioPos->y
	           - ((JGeometry::TVec3<f32>*)((u8*)gpMarioOriginal + 0x29C))->y
	           < 0.0f) {
		result = true;
	}
	return result;
}

void TCameraMarioData::calcAndSetMarioData()
{
	u32 status = SMS_GetMarioStatus();
	switch (status) {
	case 0x3800034B:
	case 0x3000054C:
		mDistXZ = 0.0f;
		mDistY  = 0.0f;
		break;
	default:
		JGeometry::TVec3<f32>* lastSafePos
		    = (JGeometry::TVec3<f32>*)((u8*)gpMarioOriginal + 0x29C);
		f32 dx                            = gpMarioPos->x - lastSafePos->x;
		f32 dy                            = gpMarioPos->y - lastSafePos->y;
		f32 dz                            = gpMarioPos->z - lastSafePos->z;
		mDistXZ = dx * dx + dz * dz;
		mDistY  = dy * dy;
		if (mDistXZ > 100.0f)
			mDistXZ = 100.0f;
		if (mDistY > 100.0f)
			mDistY = 100.0f;
		break;
	}

	if (mStatus != status) {
		mStatus      = status;
		mStatusTimer = 0;
	} else {
		mStatusTimer++;
	}

	s16 angleMin = ((TWaterGun*)SMS_GetMarioWaterGun())
	                   ->getCurrentNozzle()
	                   ->mEmitParams.mLAngleMin.value;
	s16 gunAngle
	    = ((TWaterGun*)SMS_GetMarioWaterGun())->getCurrentNozzle()->getGunAngle();
	mNozzleAngleRatio = CLBCalcRatio<s16>(0, angleMin, gunAngle);

	f32 ratio = mNozzleAngleRatio;
	if (ratio > 1.0f) {
		ratio = 1.0f;
	} else if (ratio < 0.0f) {
		ratio = 0.0f;
	}
	mNozzleAngleRatio = ratio;
}
