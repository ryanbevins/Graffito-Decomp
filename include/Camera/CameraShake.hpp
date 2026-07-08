#ifndef CAMERA_CAMERA_SHAKE_HPP
#define CAMERA_CAMERA_SHAKE_HPP

#include <JSystem/JGeometry.hpp>
#include <System/ParamInst.hpp>
#include <System/Params.hpp>

class TCamSaveShake : public TParams {
public:
	TCamSaveShake(const char*);

	/* 0x08 */ TParamRT<s16> mShakeTime;
	/* 0x1C */ TParamRT<f32> mShakeAmpX;
	/* 0x30 */ TParamRT<s16> mShakeVelX;
	/* 0x44 */ TParamRT<f32> mShakeAmpY;
	/* 0x58 */ TParamRT<s16> mShakeVelY;
	/* 0x6C */ TParamRT<f32> mShakeAmpZ;
	/* 0x80 */ TParamRT<s16> mShakeVelZ;
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
		/* 0x00 */ f32 mAmp;
		/* 0x04 */ f32 mDec;
		/* 0x08 */ s16 mVel;
	};

	struct TCamShakeInfo {
		/* 0x00 */ s32 mMode;
		/* 0x04 */ u8  mIsKeep;
		/* 0x05 */ u8  mIsDecreasing;
		/* 0x06 */ u16 mDuration;
		/* 0x08 */ u16 mFrame;
		/* 0x0C */ TCamShakeAngle mAngleX;
		/* 0x18 */ TCamShakeAngle mAngleY;
		/* 0x24 */ TCamShakeAngle mAngleZ;

		bool isActive() const { return mDuration != 0 ? true : false; }
		void reset()
		{
			mMode             = 1;
			mIsKeep          = 0;
			mIsDecreasing    = 0;
			mFrame            = 0;
			mDuration         = 0;
			mAngleX.mDec      = 0.0f;
			mAngleX.mAmp      = 0.0f;
			mAngleX.mVel      = 0;
			mAngleY.mDec      = 0.0f;
			mAngleY.mAmp      = 0.0f;
			mAngleY.mVel      = 0;
			mAngleZ.mDec      = 0.0f;
			mAngleZ.mAmp      = 0.0f;
			mAngleZ.mVel      = 0;
		}
	};

	TCameraShake();
	TCamShakeInfo* getUseShakeData_();

	inline void setShakeAngleOne_(TCamShakeAngle* angle, f32 amp,
	                              s16 initAngle, u16 duration, f32 strength)
	{
		f32 ampSigned;
		s16 angleSigned;
		if (strength < 0.0f) {
			ampSigned   = -amp;
			angleSigned = -initAngle;
		} else {
			ampSigned   = amp;
			angleSigned = initAngle;
		}
		angle->mAmp = ampSigned;
		angle->mDec = ampSigned / (f32)duration;
		angle->mVel = angleSigned;
	}

	inline void setShakeAngleAll_(TCamShakeInfo* info, const TCamSaveShake* save,
	                              u16 duration, f32 strength)
	{
		const u8* sd = (const u8*)save;
		setShakeAngleOne_(&info->mAngleX, *(const f32*)(sd + 0x2C) * strength,
		                  *(const s16*)(sd + 0x40), duration, strength);
		setShakeAngleOne_(&info->mAngleY, *(const f32*)(sd + 0x54) * strength,
		                  *(const s16*)(sd + 0x68), duration, strength);
		setShakeAngleOne_(&info->mAngleZ, *(const f32*)(sd + 0x7C) * strength,
		                  *(const s16*)(sd + 0x90), duration, strength);
	}

	void startShake(EnumCamShakeMode, f32);
	void keepShake(EnumCamShakeMode, f32);
	void execShake(const JGeometry::TVec3<f32>&, JGeometry::TVec3<f32>*,
	               JGeometry::TVec3<f32>*);

	static const char* mCamShakeNameSave[];

	/* 0x000 */ s16 mRollAccum;
	/* 0x004 */ TCamShakeInfo mShakeInfo[32];
	/* 0x604 */ TCamSaveShake* mShakeData[41];
};

#endif
