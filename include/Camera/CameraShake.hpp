#ifndef CAMERA_CAMERA_SHAKE_HPP
#define CAMERA_CAMERA_SHAKE_HPP

#include <JSystem/JGeometry.hpp>

class TCamSaveShake {
public:
	TCamSaveShake(const char*);
};

enum EnumCamShakeMode {
	CAM_SHAKE_MODE_UNK2  = 0x2,
	CAM_SHAKE_MODE_UNK3  = 0x3,
	CAM_SHAKE_MODE_UNK4  = 0x4,
	CAM_SHAKE_MODE_UNK5  = 0x5,
	CAM_SHAKE_MODE_UNK6  = 0x6,
	CAM_SHAKE_MODE_UNK7  = 0x7,
	CAM_SHAKE_MODE_UNK12 = 0x12,
	CAM_SHAKE_MODE_UNK13 = 0x13,
	CAM_SHAKE_MODE_UNK14 = 0x14,
	CAM_SHAKE_MODE_UNK15 = 0x15,
};

class TCameraShake;

extern TCameraShake* gpCameraShake;

class TCameraShake {
public:
	struct TCamShakeAngle {
		/* 0x00 */ f32 mPhase;
		/* 0x04 */ f32 mDecrement;
		/* 0x08 */ s16 mAngle;
		/* 0x0A */ u16 mPad;
	};

	struct TCamShakeInfo {
		/* 0x00 */ s32 mMode;
		/* 0x04 */ u8  mPause;
		/* 0x05 */ u8  mActiveSet;
		/* 0x06 */ u16 mDuration;
		/* 0x08 */ u16 mCurFrame;
		/* 0x0A */ u16 mPad;
		/* 0x0C */ TCamShakeAngle mAngleX;
		/* 0x18 */ TCamShakeAngle mAngleY;
		/* 0x24 */ TCamShakeAngle mAngleZ;
	};

	TCameraShake();
	TCamShakeInfo* getUseShakeData_();
	void setShakeAngleOne_(TCameraShake::TCamShakeAngle*, f32, s16, u16, f32);
	void setShakeAngleAll_(TCameraShake::TCamShakeInfo*, const TCamSaveShake*,
	                       u16, f32);
	void startShake(EnumCamShakeMode, f32);
	void keepShake(EnumCamShakeMode, f32);
	void execShake(const JGeometry::TVec3<f32>&, JGeometry::TVec3<f32>*,
	               JGeometry::TVec3<f32>*);

	/* 0x000 */ s16 mYaw;
	/* 0x002 */ u16 mPad;
	/* 0x004 */ TCamShakeInfo mShakeInfos[32];
	/* 0x604 */ TCamSaveShake* mShakeSaveData[41];
};

#endif
