#ifndef CAMERA_CAMERA_BCK_HPP
#define CAMERA_CAMERA_BCK_HPP

#include <types.h>
#include <JSystem/JGeometry/JGVec3.hpp>

class MActor;
class MActorAnmData;

class TCameraBck {
public:
	TCameraBck();

	void setFrame(f32 frame);
	bool updateDemo(JGeometry::TVec3<f32>* pos, JGeometry::TVec3<f32>* lookat,
	                JGeometry::TVec3<f32>* up, f32* fov);
	void endDemo();
	s32 getTotalDemoFrames() const;
	void startDemo(const char* name, const JGeometry::TVec3<f32>* offset);
	bool isFileExist(const char* name) const;

public:
	/* 0x00 */ MActor* mActor;
	/* 0x04 */ MActorAnmData* mAnmData;
	/* 0x08 */ s32 mFovJointIdx;
	/* 0x0C */ Mtx* mPositionJointMtx;
	/* 0x10 */ Mtx* mLookatJointMtx;
	/* 0x14 */ const JGeometry::TVec3<f32>* mOffset;
};

extern const char* cPositionJointName;
extern const char* cLookatJointName;
extern const char* cCameraBckVolumeName;

#endif
