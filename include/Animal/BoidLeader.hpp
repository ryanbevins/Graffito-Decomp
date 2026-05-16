#ifndef ANIMAL_BOID_LEADER_HPP
#define ANIMAL_BOID_LEADER_HPP

#include <dolphin/types.h>
#include <JSystem/JGeometry.hpp>
#include <Enemy/PathNode.hpp>

class TGraphWeb;
class TBoid;

class TBoidLeader {
public:
	TBoidLeader(int, const char*);
	virtual ~TBoidLeader();

	void setGraph(TGraphWeb*, const JGeometry::TVec3<f32>&);
	void calcForces(const TBoid*) const;
	void calcGoalForce(const JGeometry::TVec3<f32>&) const;
	void perform(u32, JDrama::TGraphics*);
	void calcBoids();

	/* 0x04 */ u8 _padding04[0x0C];
	/* 0x10 */ int mNumActors;
	/* 0x14 */ u8* mBoidData;
	/* 0x18 */ u32 unk18;
	/* 0x1C */ u32 mFlags;
	/* 0x20 */ f32 mParam20;
	/* 0x24 */ f32 mParam24;
	/* 0x28 */ f32 mParam28;
	/* 0x2C */ f32 mParam2C;
	/* 0x30 */ f32 mParam30;
	/* 0x34 */ f32 mParam34;
	/* 0x38 */ u32 unk38;
	/* 0x3C */ JGeometry::TVec3<f32> unk3C;
	/* 0x48 */ u8 _padding48[0x14];
	/* 0x5C */ THitActor* mGoalActor;
	/* 0x60 */ JGeometry::TVec3<f32> mGoalPos;
	/* 0x6C */ f32 mParam6C;
	/* 0x70 */ f32 mParam70;
};

#endif
