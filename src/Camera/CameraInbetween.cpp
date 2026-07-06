#include <Camera/CameraInbetween.hpp>
#include <Camera/cameralib.hpp>

#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define CALC_POLAR_DATA()                                                      \
	do {                                                                       \
		CLBCrossToPolar(mTargetAt, mTargetPos, &mDist, &mAngleX, &mAngleY);    \
		CLBCrossToPolar(mPrevAt, mTargetAt, &mSubDist, &mSubAngleX,            \
		                &mSubAngleY);                                          \
	} while (0)

template <> s16 CLBRoundf<s16>(f32);

inline void CLBChaseConstantSpecifyFrame(s16* value, s16 target, f32 frames)
{
	if (frames < 0.001f) {
		*value = target;
		return;
	}

	s16 delta = target - *value;
	*value += CLBRoundf<s16>((1.0f / frames) * delta);
}

TCameraInbetween::TCameraInbetween()
    : mFrameTotal(1)
    , mFrameCount(0)
    , mTargetPos(0.0f, 0.0f, 0.0f)
    , mTargetAt(0.0f, 0.0f, 0.0f)
    , mPrevAt(0.0f, 0.0f, 0.0f)
    , mUseAngularVelocity(0)
    , mAngularVelocity(0)
    , mChaseFrame(0.0f)
{
}

void TCameraInbetween::warpPosAndAt(const Vec& pos, const Vec& at)
{
	mTargetPos.set(pos);
	mTargetAt.set(at);
	CALC_POLAR_DATA();
}

void TCameraInbetween::addMoveCameraAndMario(const Vec& offset)
{
	mTargetPos += offset;
	mTargetAt += offset;
	mPrevAt += offset;
	CALC_POLAR_DATA();
}

void TCameraInbetween::startCameraInbetween(int frames)
{
	mFrameCount         = frames;
	mFrameTotal         = frames;
	mUseAngularVelocity = 0;
	CALC_POLAR_DATA();
}

void TCameraInbetween::initCameraInbetween(
    const JGeometry::TVec3<f32>& pos, const JGeometry::TVec3<f32>& at,
    const JGeometry::TVec3<f32>& prev)
{
	mTargetPos.set(pos);
	mTargetAt.set(at);
	mPrevAt.set(prev);
	CALC_POLAR_DATA();
}

void TCameraInbetween::execCameraInbetween(
    const JGeometry::TVec3<f32>& pos, const JGeometry::TVec3<f32>& at,
    const JGeometry::TVec3<f32>& prev)
{
	mTargetPos.set(pos);
	mTargetAt.set(at);

	if (mFrameCount > 0) {
		f32 frames = mFrameCount;

		if (isThing())
			CLBChaseConstantSpecifyFrame(&mChaseFrame, 0.0f, frames);

		f32 dist;
		s16 angleX;
		s16 angleY;
		CLBCrossToPolar(prev, at, &dist, &angleX, &angleY);

		CLBChaseConstantSpecifyFrame(&mSubDist, dist, frames);
		CLBChaseConstantSpecifyFrame(&mSubAngleX, angleX, frames);
		CLBChaseConstantSpecifyFrame(&mSubAngleY, angleY, frames);

		if (ABS(prev.x - at.x) > 0.1f || ABS(prev.z - at.z) > 0.1f) {
			JGeometry::TVec3<f32> tmp;
			CLBPolarToCross(prev, &tmp, mSubDist, mSubAngleX, mSubAngleY);
			mTargetAt.x = tmp.x;
			mTargetAt.z = tmp.z;
		}

		CLBCrossToPolar(mTargetAt, pos, &dist, &angleX, &angleY);
		CLBChaseConstantSpecifyFrame(&mDist, dist, frames);
		CLBChaseConstantSpecifyFrame(&mAngleX, angleX, frames);
		if (mUseAngularVelocity == 0) {
			CLBChaseConstantSpecifyFrame(&mAngleY, angleY, frames);
		} else {
			mAngleY += mAngularVelocity;
		}

		CLBPolarToCross(mTargetAt, &mTargetPos, mDist, mAngleX, mAngleY);
		mFrameCount -= 1;
		if (mFrameCount == 0)
			mUseAngularVelocity = 0;
	}

	mPrevAt.set(prev);
}
