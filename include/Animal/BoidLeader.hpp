#ifndef ANIMAL_BOID_LEADER_HPP
#define ANIMAL_BOID_LEADER_HPP

#include <Enemy/PathNode.hpp>
#include <JSystem/JDrama/JDRViewObj.hpp>
#include <JSystem/JGeometry.hpp>
#include <Strategic/HitActor.hpp>
#include <dolphin/types.h>

class TGraphWeb;
class TGraphTracer;

class TBoid {
public:
	TBoid();

	/* 0x00 */ JGeometry::TVec3<f32> mPosition;
	/* 0x0C */ f32 mPitch;
	/* 0x10 */ f32 mYaw;
	/* 0x14 */ f32 mRoll;
	/* 0x18 */ JGeometry::TVec3<f32> mForward;
	/* 0x24 */ JGeometry::TVec3<f32> mForce;
	/* 0x30 */ JGeometry::TVec3<f32> mAverageForward;
	/* 0x3C */ JGeometry::TVec3<f32> mCenterDir;
	/* 0x48 */ s32 mNeighborCount;
	/* 0x4C */ f32 mPhase;
};

class TBoidLeader : public JDrama::TViewObj {
public:
	TBoidLeader(int, const char*);
	virtual ~TBoidLeader();

	void setGraph(TGraphWeb*, const JGeometry::TVec3<f32>&);
	JGeometry::TVec3<f32> calcForces(const TBoid*) const;
	JGeometry::TVec3<f32>
	calcGoalForce(const JGeometry::TVec3<f32>&) const;
	void perform(u32, JDrama::TGraphics*);
	void calcBoids();

	/* 0x10 */ int mNumActors;
	/* 0x14 */ TBoid* mBoidData;
	/* 0x18 */ TGraphTracer* mGraphTracer;
	/* 0x1C */ u32 mFlags;
	/* 0x20 */ f32 mParam20;
	/* 0x24 */ f32 mParam24;
	/* 0x28 */ f32 mParam28;
	/* 0x2C */ f32 mParam2C;
	/* 0x30 */ f32 mParam30;
	/* 0x34 */ f32 mParam34;
	/* 0x38 */ THitActor* mGoalActor;
	/* 0x3C */ JGeometry::TVec3<f32> mGoalPos;
	/* 0x48 */ f32 mGoalForce;
	/* 0x4C */ JGeometry::TVec3<f32> mGoalOffset;
	/* 0x58 */ u32 unk58;
	/* 0x5C */ THitActor* mRepelActor;
	/* 0x60 */ JGeometry::TVec3<f32> mRepelPos;
	/* 0x6C */ f32 mRepelRange;
	/* 0x70 */ f32 mRepelForce;
	/* 0x74 */ JGeometry::TVec3<f32> mGraphGoal;
};

#endif
