#include <Camera/CameraInbetween.hpp>
#include <Camera/cameralib.hpp>

TCameraInbetween::TCameraInbetween()
{
	mFrameTotal         = 1;
	mFrameCount         = 0;
	mTargetPos.x        = 0.0f;
	mTargetPos.y        = 0.0f;
	mTargetPos.z        = 0.0f;
	mTargetAt.x         = 0.0f;
	mTargetAt.y         = 0.0f;
	mTargetAt.z         = 0.0f;
	mPrevAt.x           = 0.0f;
	mPrevAt.y           = 0.0f;
	mPrevAt.z           = 0.0f;
	mUseAngularVelocity = 0;
	mAngularVelocity    = 0;
	mChaseFrame         = 0.0f;
}

void TCameraInbetween::initCameraInbetween(
    const JGeometry::TVec3<f32>& pos, const JGeometry::TVec3<f32>& at,
    const JGeometry::TVec3<f32>& prev)
{
	mTargetPos.x = pos.x;
	mTargetPos.y = pos.y;
	mTargetPos.z = pos.z;
	mTargetAt.x  = at.x;
	mTargetAt.y  = at.y;
	mTargetAt.z  = at.z;
	mPrevAt.x    = prev.x;
	mPrevAt.y    = prev.y;
	mPrevAt.z    = prev.z;
	CLBCrossToPolar(mTargetAt, mTargetPos, &mDist, &mAngleX, &mAngleY);
	CLBCrossToPolar(mPrevAt, mTargetAt, &mSubDist, &mSubAngleX, &mSubAngleY);
}

void TCameraInbetween::startCameraInbetween(int frames)
{
	mFrameCount         = frames;
	mFrameTotal         = frames;
	mUseAngularVelocity = 0;
	CLBCrossToPolar(mTargetAt, mTargetPos, &mDist, &mAngleX, &mAngleY);
	CLBCrossToPolar(mPrevAt, mTargetAt, &mSubDist, &mSubAngleX, &mSubAngleY);
}

void TCameraInbetween::addMoveCameraAndMario(const Vec& d)
{
	mTargetPos.x += d.x;
	mTargetPos.y += d.y;
	mTargetPos.z += d.z;
	mTargetAt.x += d.x;
	mTargetAt.y += d.y;
	mTargetAt.z += d.z;
	mPrevAt.x += d.x;
	mPrevAt.y += d.y;
	mPrevAt.z += d.z;
	CLBCrossToPolar(mTargetPos, mTargetAt, &mDist, &mAngleX, &mAngleY);
	CLBCrossToPolar(mPrevAt, mTargetAt, &mSubDist, &mSubAngleX, &mSubAngleY);
}

void TCameraInbetween::warpPosAndAt(const Vec& pos, const Vec& at)
{
	mTargetPos.x = pos.x;
	mTargetPos.y = pos.y;
	mTargetPos.z = pos.z;
	mTargetAt.x  = at.x;
	mTargetAt.y  = at.y;
	mTargetAt.z  = at.z;
	CLBCrossToPolar(mTargetAt, mTargetPos, &mDist, &mAngleX, &mAngleY);
	CLBCrossToPolar(mPrevAt, mTargetAt, &mSubDist, &mSubAngleX, &mSubAngleY);
}

void TCameraInbetween::execCameraInbetween(
    const JGeometry::TVec3<f32>& outPosTarget,
    const JGeometry::TVec3<f32>& outAtTarget,
    const JGeometry::TVec3<f32>& outPrevTarget)
{
	(void)outPosTarget;
	(void)outAtTarget;
	(void)outPrevTarget;
	if (mFrameCount > 0) {
		mFrameCount -= 1;
	}
}
