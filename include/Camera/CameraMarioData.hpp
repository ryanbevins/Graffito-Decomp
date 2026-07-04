#ifndef CAMERA_CAMERAMARIODATA_HPP
#define CAMERA_CAMERAMARIODATA_HPP

#include <types.h>
#include <dolphin/mtx.h>

class TCameraMarioData {
public:
	TCameraMarioData();

	bool isMarioClimb(u32 status) const;
	bool isMarioLeanMirror() const;
	bool isMarioSlider() const;
	bool isMarioIndoor() const;
	bool isMarioRocketing() const;
	bool isMarioGoDown() const;
	void calcAndSetMarioData();
	void addMoveCameraAndMario(const Vec& v)
	{
		mPosX += v.x;
		mPosY += v.y;
		mPosZ += v.z;
	}

	/* 0x00 */ f32 mPosX;
	/* 0x04 */ f32 mPosY;
	/* 0x08 */ f32 mPosZ;
	/* 0x0C */ f32 mDistXZ;
	/* 0x10 */ f32 mDistY;
	/* 0x14 */ u32 mStatus;
	/* 0x18 */ u32 mStatusTimer;
	/* 0x1C */ f32 mNozzleAngleRatio;
};

extern TCameraMarioData* gpCameraMario;

#endif
