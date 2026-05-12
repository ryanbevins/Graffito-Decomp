#include <Animal/BoidLeader.hpp>

class TBoid {
public:
	TBoid();
	/* 0x00 */ JGeometry::TVec3<f32> mPos;
	/* 0x0C */ JGeometry::TVec3<f32> mVel;
	/* 0x18 */ JGeometry::TVec3<f32> mForce;
	/* 0x24 */ JGeometry::TVec3<f32> mGoal;
};

TBoid::TBoid()
    : mPos(0.0f, 0.0f, 0.0f)
    , mVel(0.0f, 0.0f, 0.0f)
    , mForce(0.0f, 0.0f, 0.0f)
    , mGoal(0.0f, 0.0f, 0.0f)
{
}

TBoidLeader::TBoidLeader(int count, const char* name)
{
	(void)name;
	mNumActors = count;
	mBoidData  = nullptr;
	unk18      = 0;
	mFlags     = 0;
}

TBoidLeader::~TBoidLeader() { }

void TBoidLeader::setGraph(TGraphWeb* graph, const JGeometry::TVec3<f32>& pos)
{
	(void)graph;
	(void)pos;
}

void TBoidLeader::calcForces(const TBoid* boid) const { (void)boid; }
void TBoidLeader::calcGoalForce(const JGeometry::TVec3<f32>& goal) const
{
	(void)goal;
}

void TBoidLeader::perform(u32 flags, JDrama::TGraphics* gfx)
{
	(void)flags;
	(void)gfx;
}

void TBoidLeader::calcBoids() { }
