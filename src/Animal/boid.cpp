#define JGEOMETRY_TVEC3_DIV_OUT_OF_LINE
#include <Animal/BoidLeader.hpp>
#include <Enemy/Graph.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <stdlib.h>

static inline f32 callMsWrap(f32 t, f32 l, f32 r)
{
	return MsWrap<f32>(t, l, r);
}

static inline const JGeometry::TVec3<f32>&
getPathPoint(THitActor* actor, const JGeometry::TVec3<f32>& point)
{
	if (actor != nullptr)
		return actor->getPosition();

	return point;
}

#pragma dont_inline on
void JGeometry::TVec3<f32>::div(f32 divisor)
{
	f32 scale = 1.0f / divisor;
	x *= scale;
	y *= scale;
	z *= scale;
}
#pragma dont_inline off

TBoid::TBoid()
{
	mNeighborCount = 0;
	mPhase         = 0.0f;
	mPosition.zero();
	mRoll  = 0.0f;
	mYaw   = 0.0f;
	mPitch = 0.0f;
	mForward.set(0.0f, 0.0f, 1.0f);
	mPhase = (f32)rand() * (1.0f / 32768.0f);
}

TBoidLeader::TBoidLeader(int count, const char* name)
    : JDrama::TViewObj(name)
    , mNumActors(count)
    , mBoidData(new TBoid[count])
    , mGraphTracer(nullptr)
    , mFlags(0)
    , mParam20(6.0f)
    , mParam24(150.0f)
    , mParam28(2.0f)
    , mParam2C(2.0f)
    , mParam30(10.0f)
    , mParam34(0.01f)
    , mGoalTarget(JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f))
    , mGoalForce(1.0f)
    , unk58(0)
    , mRepelTarget(JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f))
    , mRepelRange(0.0f)
    , mRepelForce(1.0f)
    , mGraphGoal(0.0f, 0.0f, 0.0f)
{
	mGoalOffset.zero();

	mFlags |= 1;
}

void TBoidLeader::calcBoids()
{
	if (mFlags & 1) {
		TBoid* end = mBoidData + mNumActors;

		for (TBoid* boid = mBoidData; boid != end; ++boid) {
			boid->mForce.zero();
			boid->mCenterDir.zero();
			boid->mAverageForward.zero();
			boid->mNeighborCount = 0;
		}

		for (TBoid* boid = mBoidData; boid != end; ++boid) {
			for (TBoid* other = boid + 1; other != end; ++other) {
				JGeometry::TVec3<f32> diff = boid->mPosition;
				diff -= other->mPosition;
				f32 dist2 = diff.squared();
				if (dist2 < 0.001f)
					continue;

				f32 radius = mParam24;
				if (dist2 < radius * radius) {
					boid->mForce += diff / dist2 * radius;
					other->mForce -= diff / dist2 * radius;

					boid->mAverageForward += other->mForward;
					other->mAverageForward += boid->mForward;
					boid->mCenterDir += other->mPosition;
					other->mCenterDir += boid->mPosition;
					boid->mNeighborCount++;
					other->mNeighborCount++;
				}
			}

			if (boid->mNeighborCount > 0) {
				f32 inv = 1.0f / boid->mNeighborCount;
				boid->mCenterDir *= inv;
				boid->mCenterDir -= boid->mPosition;
				boid->mCenterDir.normalize();

				boid->mAverageForward *= inv;
				f32 mag = VECMag((Vec*)&boid->mAverageForward);
				if (mag > 0.0f) {
					boid->mAverageForward *= 1.0f / mag;
					boid->mAverageForward -= boid->mForward;
					boid->mAverageForward.normalize();
				}
			}
		}

		for (TBoid* boid = mBoidData; boid != end; ++boid) {
			JGeometry::TVec3<f32> force = calcForces(boid);
			if (!force.isZero()) {
				if (force.y < -0.01f) {
					boid->mPitch += mParam2C;
					if (boid->mPitch > mParam30)
						boid->mPitch = mParam30;
				} else if (force.y > 0.01f) {
					boid->mPitch -= mParam2C;
					if (boid->mPitch < -mParam30)
						boid->mPitch = -mParam30;
				} else {
					boid->mPitch *= 0.98f;
				}

				f32 targetYaw = MsGetRotFromZaxisY(force);
				f32 diff = MsAngleDiff(targetYaw, boid->mYaw);
				if (diff < -0.01f)
					diff = -mParam28;
				else if (diff > 0.01f)
					diff = mParam28;

				f32 newYaw = boid->mYaw + diff;
				while (newYaw >= 360.0f)
					newYaw -= 360.0f;
				while (newYaw < 0.0f)
					newYaw += 360.0f;
				boid->mYaw = newYaw;
			}

			Mtx rot;
			MsMtxSetRotRPH(rot, boid->mPitch, boid->mYaw, boid->mRoll);
			boid->mForward.set(rot[0][2], rot[1][2], rot[2][2]);
			VECNormalize((Vec*)&boid->mForward, (Vec*)&boid->mForward);

			boid->mPosition += boid->mForward
			                   * (mParam20 + boid->mPhase) * force.length();
		}
	}
}

void TBoidLeader::setGraph(TGraphWeb* graph,
                           const JGeometry::TVec3<f32>& pos)
{
	if (graph == nullptr || graph->isDummy())
		return;

	if (mGraphTracer == nullptr)
		mGraphTracer = new TGraphTracer();

	mGraphTracer->setGraph(graph);
	mGraphTracer->setTo(graph->findNearestNodeIndex(pos, -1));
	mGraphGoal.set(mGraphTracer->getCurrentPos());
	mFlags |= 4;
}

void TBoidLeader::perform(u32 flags, JDrama::TGraphics*)
{
	if (!(flags & 2))
		return;

	if (mFlags & 4) {
		JGeometry::TVec3<f32> node
		    = mGraphTracer->unk0->indexToPoint(mGraphTracer->mCurrIdx);
		node.sub(mGraphGoal);

		if (node.squared() < 10000.0f) {
			TGraphTracer* tracer = mGraphTracer;
			int next = tracer->unk0->getRandomNextIndex(
			    tracer->mCurrIdx, tracer->mPrevIdx, 0xffffffff);
			tracer->moveTo(next);
		} else {
			PSVECNormalize((Vec*)&node, (Vec*)&node);
			node.scale(0.9f * mParam20);
			mGraphGoal.add(node);
		}

		calcBoids();
	}
}

JGeometry::TVec3<f32>
TBoidLeader::calcGoalForce(const JGeometry::TVec3<f32>& pos) const
{
	JGeometry::TVec3<f32> force;

	if (mFlags & 4) {
		force.set(mGraphGoal);
		force -= pos;
		force.normalize();
	} else {
		force.set(mGoalTarget.getPoint());
		force += mGoalOffset;
		force -= pos;
		f32 length = force.length();
		if (0.0f < length) {
			force.scale(1.0f / length);
			force *= mGoalForce;
		} else {
			force.zero();
		}
	}

	return force;
}

JGeometry::TVec3<f32> TBoidLeader::calcForces(const TBoid* boid) const
{
	JGeometry::TVec3<f32> result = boid->mForce;
	result += boid->mAverageForward * mParam34;
	result += boid->mCenterDir;
	result += calcGoalForce(boid->mPosition);

	f32 len2 = result.squared();
	if (len2 == 0.0f)
		return JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f);

	f32 scale = ((5.0f * ((f32)rand() * (1.0f / 32768.0f))) + 95.0f) * 0.01f;
	result.scale(scale);

	result.setLength(1.0f);

	if (mRepelRange > 0.0f) {
		JGeometry::TVec3<f32> away = boid->mPosition;
		away.sub(mRepelTarget.getPoint());

		f32 repelLen2 = away.squared();
		if (repelLen2 > 0.0f && repelLen2 < mRepelRange * mRepelRange) {
			away.setLength(mRepelForce);
			result = away;
		}
	}

	return result;
}

TBoidLeader::~TBoidLeader() { }
