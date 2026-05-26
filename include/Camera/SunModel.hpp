#ifndef CAMERA_SUN_MODEL_HPP
#define CAMERA_SUN_MODEL_HPP

#include <JSystem/JDrama/JDRActor.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/JGeometry.hpp>
#include <dolphin/types.h>

class J3DModel;
class J3DModelData;
class J3DAnmTextureSRTKey;
class TMapStaticObj;

class TSunModel : public JDrama::TActor {
public:
	TSunModel(bool, const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);

	void getZBufValue();
	void calcDispRatioAndScreenPos_();
	void calcOtherFPosFromCenterAndRadius_(JGeometry::TVec2<f32>* out,
	                                       const JGeometry::TVec2<f32>& center,
	                                       f32 radius);

public:
	/* 0x44 */ J3DModelData*       mModelData;
	/* 0x48 */ J3DModel*           mModel;
	/* 0x4C */ J3DAnmTextureSRTKey* mAnmTexSRT;
	/* 0x50 */ J3DFrameCtrl        mFrameCtrl;
	/* 0x64 */ TMapStaticObj*      mMapStaticObj;
	/* 0x68 */ u8                  mUnk68;
	/* 0x69 */ u8                  _69[3];
	/* 0x6C */ f32                 mUnk6C;
	/* 0x70 */ f32                 mUnk70;
	/* 0x74 */ u8                  mUnk74;
	/* 0x75 */ u8                  _75[3];
	/* 0x78 */ f32                 mUnk78;
	/* 0x7C */ f32                 mUnk7C;
	/* 0x80 */ u8                  mUnk80;
	/* 0x81 */ u8                  _81[3];
	/* 0x84 */ f32                 mUnk84;
	/* 0x88 */ f32                 mUnk88;
	/* 0x8C */ u8                  _8C[0x9C - 0x8C];
	/* 0x9C */ f32                 mUnk9C;
	/* 0xA0 */ f32                 mUnkA0;
	/* 0xA4 */ f32                 mUnkA4;
	/* 0xA8 */ f32                 mUnkA8;
	/* 0xAC */ f32                 unkAC;
	/* 0xB0 */ f32                 mUnkB0;
	/* 0xB4 */ JGeometry::TVec2<s16> mZBufCoords[17];
	/* 0xF8 */ f32                 unkF8;
	/* 0xFC */ f32                 unkFC;
	/* 0x100 */ JGeometry::TVec2<f32> mInnerCircle[8];
	/* 0x140 */ JGeometry::TVec2<f32> mOuterCircle[8];
	/* 0x180 */ u8                 mZBufVisible[17];
	/* 0x191 */ u8                 mVisibleCount;
	/* 0x192 */ u8                 _192[2];
	/* 0x194 */ f32                mUnk194;
	/* 0x198 */ Vec                mPos198;
	/* 0x1A4 */ f32                mUnk1A4;
	/* 0x1A8 */ f32                mUnk1A8;
	/* 0x1AC */ u8                 mFlags;
	/* 0x1AD */ u8                 _1AD[3];
};

extern TSunModel* gpSunModel;

#endif
