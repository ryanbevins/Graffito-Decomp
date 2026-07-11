#define JGEOMETRY_TVEC3_DIV_OUT_OF_LINE
#include <Animal/BoidLeader.hpp>
#include <Enemy/Graph.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <stdlib.h>

static inline f32 callMsWrap(f32 t, f32 l, f32 r)
{
	return MsWrap<f32>(t, l, r);
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
{
	mParam20 = 6.0f;
	mParam24 = 150.0f;
	mParam28 = 2.0f;
	mParam2C = 2.0f;
	mParam30 = 10.0f;
	mParam34 = 0.01f;

	JGeometry::TVec3<f32> zeroGoal;
	zeroGoal.set(0.0f, 0.0f, 0.0f);
	mGoalActor = nullptr;
	mGoalPos   = zeroGoal;
	mGoalForce = 1.0f;

	unk58 = 0;
	JGeometry::TVec3<f32> zeroRepel;
	zeroRepel.set(0.0f, 0.0f, 0.0f);
	mRepelActor = nullptr;
	mRepelPos   = zeroRepel;
	mRepelRange = 0.0f;
	mRepelForce = 1.0f;
	mGraphGoal.set(0.0f, 0.0f, 0.0f);
	mGoalOffset.zero();

	mFlags |= 1;
}

void TBoidLeader::calcBoids()
{
	if (!(mFlags & 1))
		return;

	TBoid* end = mBoidData + mNumActors;
	for (TBoid* boid = mBoidData; boid != end; ++boid) {
		boid->mForce.zero();
		boid->mAverageForward.zero();
		boid->mCenterDir.zero();
		boid->mNeighborCount = 0;
	}

	for (TBoid* boid = mBoidData; boid != end; ++boid) {
		for (TBoid* other = boid + 1; other != end; ++other) {
			JGeometry::TVec3<f32> diff = boid->mPosition;
			diff.sub(other->mPosition);
			f32 dist2 = diff.squared();
			if (dist2 < 0.001f)
				continue;

			f32 radius = mParam24;
			if (!(dist2 < radius * radius))
				continue;

			JGeometry::TVec3<f32> avoid = diff;
			avoid.div(dist2);
			avoid.scale(radius);
			boid->mForce.add(avoid);

			JGeometry::TVec3<f32> otherAvoid = diff;
			otherAvoid.div(dist2);
			otherAvoid.scale(radius);
			other->mForce.sub(otherAvoid);

			boid->mAverageForward.add(other->mForward);
			other->mAverageForward.add(boid->mForward);
			boid->mCenterDir.add(other->mPosition);
			other->mCenterDir.add(boid->mPosition);
			boid->mNeighborCount++;
			other->mNeighborCount++;
		}

		if (boid->mNeighborCount <= 0)
			continue;

		f32 invCount = 1.0f / (f32)boid->mNeighborCount;
		boid->mCenterDir.scale(invCount);
		boid->mCenterDir.sub(boid->mPosition);
		boid->mCenterDir.normalize();

		boid->mAverageForward.scale(invCount);
		boid->mAverageForward.normalize();
		boid->mAverageForward.sub(boid->mForward);
		boid->mAverageForward.normalize();
	}

	for (TBoid* boid = mBoidData; boid != end; ++boid) {
		JGeometry::TVec3<f32> force = calcForces(boid);

		if (force.squared() > JGeometry::TUtil<f32>::epsilon()) {
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

			f32 targetYaw;
			if (force.z == 0.0f) {
				if (force.x >= 0.0f)
					targetYaw = 90.0f;
				else
					targetYaw = -90.0f;
			} else if (force.z >= 0.0f) {
				targetYaw = (f32)matan(force.z, force.x) * 0.005493164f;
			} else {
				targetYaw = 180.0f
				            - (f32)matan(-force.z, force.x) * 0.005493164f;
			}

			f32 wrapped = callMsWrap(boid->mYaw, targetYaw - 180.0f,
			                         targetYaw + 180.0f);
			f32 turn = targetYaw - wrapped;
			if (turn < -0.01f)
				turn = -mParam28;
			else if (turn > 0.01f)
				turn = mParam28;

			boid->mYaw += turn;
			while (boid->mYaw >= 360.0f)
				boid->mYaw -= 360.0f;
			while (boid->mYaw < 0.0f)
				boid->mYaw += 360.0f;
		}

		Mtx rot;
		MsMtxSetRotRPH(rot, boid->mPitch, boid->mYaw, boid->mRoll);
		boid->mForward.set(rot[0][2], rot[1][2], rot[2][2]);
		PSVECNormalize((Vec*)&boid->mForward, (Vec*)&boid->mForward);

		f32 forceLen = JGeometry::TUtil<f32>::sqrt(force.dot(force));
		JGeometry::TVec3<f32> velocity = boid->mForward;
		velocity.scale(mParam20 + boid->mPhase);
		velocity.scale(forceLen);
		boid->mPosition.add(velocity);
	}
}

void TBoidLeader::setGraph(TGraphWeb* graph,
                           const JGeometry::TVec3<f32>& pos)
{
	if (graph != nullptr) {
		if (graph->isDummy() == 0) {
			if (mGraphTracer == nullptr)
				mGraphTracer = new TGraphTracer();

			mGraphTracer->setGraph(graph);
			mGraphTracer->setTo(
			    graph->findNearestNodeIndex(pos, 0xffffffff));
			mGraphGoal.set(mGraphTracer->unk0->indexToPoint(
			    mGraphTracer->mCurrIdx));
			mFlags |= 4;
		}
	}
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
			int next = mGraphTracer->unk0->getRandomNextIndex(
			    mGraphTracer->mCurrIdx, mGraphTracer->mPrevIdx, 0xffffffff);
			mGraphTracer->moveTo(next);
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
	JGeometry::TVec3<f32> result;

	if (mFlags & 4) {
		result = mGraphGoal;
		result.sub(pos);
		result.normalize();
		return result;
	}

	const JGeometry::TVec3<f32>* goal;
	if (mGoalActor != nullptr)
		goal = &mGoalActor->mPosition;
	else
		goal = &mGoalPos;

	result = *goal;
	result.add(mGoalOffset);
	result.sub(pos);

	f32 length = result.length();
	if (length > 0.0f) {
		result.scale(1.0f / length);
		result.scale(mGoalForce);
	} else {
		result.zero();
	}

	return result;
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
		const JGeometry::TVec3<f32>* repelPos;
		if (mRepelActor != nullptr)
			repelPos = &mRepelActor->mPosition;
		else
			repelPos = &mRepelPos;
		away.sub(*repelPos);

		f32 repelLen2 = away.squared();
		if (repelLen2 > 0.0f && repelLen2 < mRepelRange * mRepelRange) {
			away.setLength(mRepelForce);
			result = away;
		}
	}

	return result;
}

TBoidLeader::~TBoidLeader() { }
