#ifndef CAMERA_CAMERAINBETWEEN_HPP
#define CAMERA_CAMERAINBETWEEN_HPP

#include <JSystem/JGeometry.hpp>
#include <dolphin/mtx.h>

class TCameraInbetween {
public:
	TCameraInbetween();
	void execCameraInbetween(const JGeometry::TVec3<f32>&,
	                         const JGeometry::TVec3<f32>&,
	                         const JGeometry::TVec3<f32>&);
	void initCameraInbetween(const JGeometry::TVec3<f32>&,
	                         const JGeometry::TVec3<f32>&,
	                         const JGeometry::TVec3<f32>&);
	void startCameraInbetween(int);
	void addMoveCameraAndMario(const Vec&);
	void warpPosAndAt(const Vec&, const Vec&);

	/* 0x00 */ s32 mFrameTotal;
	/* 0x04 */ s32 mFrameCount;
	/* 0x08 */ f32 mDist;
	/* 0x0C */ s16 mAngleX;
	/* 0x0E */ s16 mAngleY;
	/* 0x10 */ f32 mSubDist;
	/* 0x14 */ s16 mSubAngleX;
	/* 0x16 */ s16 mSubAngleY;
	/* 0x18 */ JGeometry::TVec3<f32> mTargetPos;
	/* 0x24 */ JGeometry::TVec3<f32> mTargetAt;
	/* 0x30 */ JGeometry::TVec3<f32> mPrevAt;
	/* 0x3C */ s32 mUseAngularVelocity;
	/* 0x40 */ s32 mAngularVelocity;
	/* 0x44 */ f32 mChaseFrame;
};

#endif
