#ifndef CAMERA_CAMERA_MAP_TOOL_HPP
#define CAMERA_CAMERA_MAP_TOOL_HPP

#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JGeometry.hpp>
#include <Strategic/NameRefAry.hpp>

class TCameraMapTool : public JDrama::TNameRef {
public:
	TCameraMapTool(const char* name = "<TCameraMapTool>")
	    : JDrama::TNameRef(name)
	{
	}

	void calcPosAndAt(JGeometry::TVec3<f32>*, JGeometry::TVec3<f32>*) const;
	void load(JSUMemoryInputStream&);

	f32 getYaw() const { return mPitchYaw.y; }
	s32 getCameraMode() const { return mCameraMode; }
	u32 getDemoLengthFrames() const { return mDemoLengthFrames; }

public:
	/* 0xC */ union {
		JGeometry::TVec3<f32> unkC;
		JGeometry::TVec3<f32> mPosition;
	};
	/* 0x18 */ union {
		JGeometry::TVec2<f32> unk18;
		JGeometry::TVec2<f32> mPitchYaw;
	};
	/* 0x20 */ u32 unk20;
	/* 0x24 */ union {
		s32 unk24;
		s32 mCameraMode;
	};
	/* 0x28 */ s32 unk28;
	/* 0x2C */ union {
		u32 unk2C;
		u32 mDemoLengthFrames;
	};
};

extern TNameRefAryT<TCameraMapTool>* gpCamMapToolTable;

#endif
