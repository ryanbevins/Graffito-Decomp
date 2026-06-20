#ifndef CAMERA_CAMERAKINDPARAM_HPP
#define CAMERA_CAMERAKINDPARAM_HPP

#include <types.h>

// Flat extracted params (size 0xAC)
class TCameraKindParam {
public:
	void copySaveParam(const class TCamSaveKindParam&);
	void inbetweenData(const TCameraKindParam&, f32 ratio);

public:
	/* 0x00 */ union { f32 unk00; f32 mFovy; };
	/* 0x04 */ union { f32 unk04; f32 mNearClip; };
	/* 0x08 */ union { f32 unk08; f32 mDistMin; };
	/* 0x0C */ union { f32 unk0C; f32 mDistMax; };
	/* 0x10 */ union { f32 unk10; f32 mCushionMin; };
	/* 0x14 */ union { f32 unk14; f32 mCushionMax; };
	/* 0x18 */ union { s16 unk18; s16 mXAngleMin; };
	/* 0x1A */ union { s16 unk1A; s16 mXAngleMax; };
	/* 0x1C */ union { f32 unk1C; f32 mXRotRatioManualSpeed; };
	/* 0x20 */ union { s16 unk20; s16 mYAngleManualSpeedXMin; };
	/* 0x22 */ union { s16 unk22; s16 mYAngleManualSpeedXMax; };
	/* 0x24 */ union { f32 unk24; f32 mAtOffsetY; };
	/* 0x28 */ union { f32 unk28; f32 mXRotRatioAtOffsetY; };
	/* 0x2C */ union { f32 unk2C; f32 mTargetAtJumpOffsetY; };
	/* 0x30 */ union { f32 unk30; f32 mAtJumpOffsetSpeed; };
	/* 0x34 */ union { f32 unk34; f32 mHeightPanChaseRateY; };
	/* 0x38 */ union { f32 unk38; f32 mSecureViewChase; };
	/* 0x3C */ union { f32 unk3C; f32 mSecureViewDistXMin; };
	/* 0x40 */ union { f32 unk40; f32 mSecureViewDistZMin; };
	/* 0x44 */ union { f32 unk44; f32 mSecureViewDistXMax; };
	/* 0x48 */ union { f32 unk48; f32 mSecureViewDistZMax; };
	/* 0x4C */ union { f32 unk4C; f32 mHoldAddDistXZMin; };
	/* 0x50 */ union { f32 unk50; f32 mHoldAddDistXZMax; };
	/* 0x54 */ union { s16 unk54; s16 mHoldOffsetAngleXMin; };
	/* 0x56 */ union { s16 unk56; s16 mHoldOffsetAngleXMax; };
	/* 0x58 */ union { s16 unk58; s16 mOffsetAngleX; };
	/* 0x5A */ union { s16 unk5A; s16 mOffsetAngleY; };
	/* 0x5C */ union { f32 unk5C; f32 mOffsetLookatXZ; };
	/* 0x60 */ union { s16 unk60; s16 mMaxAddAngleY; };
	/* 0x64 */ union { u32 unk64; s32 mAutoChaseStartFrame; };
	/* 0x68 */ union { u32 unk68; s32 mAutoChaseCompleteFrame; };
	/* 0x6C */ union { f32 unk6C; f32 mFollowSpeedXmin; };
	/* 0x70 */ union { f32 unk70; f32 mFollowSpeedXmax; };
	/* 0x74 */ union { f32 unk74; f32 mJumpFollowSpeedXmin; };
	/* 0x78 */ union { f32 unk78; f32 mJumpFollowSpeedXmax; };
	/* 0x7C */ union { f32 unk7C; f32 mObstructMaginfXmin; };
	/* 0x80 */ union { f32 unk80; f32 mObstructMaginfXmax; };
	/* 0x84 */ union { f32 unk84; f32 mInHouseMaginfXmin; };
	/* 0x88 */ union { f32 unk88; f32 mInHouseMaginfXmax; };
	/* 0x8C */ union { f32 unk8C; f32 mLFollowMaginfXmin; };
	/* 0x90 */ union { f32 unk90; f32 mLFollowMaginfXmax; };
	/* 0x94 */ union { f32 unk94; f32 mPosChaseRateXZ; };
	/* 0x98 */ union { f32 unk98; f32 mPosChaseRateXZ_C; };
	/* 0x9C */ union { f32 unk9C; f32 mPosChaseRateY; };
	/* 0xA0 */ union { f32 unkA0; f32 mPosChaseRateY_C; };
	/* 0xA4 */ union { f32 unkA4; f32 mAtChaseRateXZ; };
	/* 0xA8 */ union { f32 unkA8; f32 mAtChaseRateY; };
};

// TCamSaveKindParam: TParamRT-based save struct. Defined in camerasave.cpp.
class TCamSaveKindParam;

#endif
