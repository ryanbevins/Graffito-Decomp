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
	CLBCrossToPolar(mTargetAt, mTargetPos, &mDist, &mAngleX, &mAngleY);
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
    const JGeometry::TVec3<f32>& pos, const JGeometry::TVec3<f32>& at,
    const JGeometry::TVec3<f32>& prev)
{
	mTargetPos.x = pos.x;
	mTargetPos.y = pos.y;
	mTargetPos.z = pos.z;
	mTargetAt.x  = at.x;
	mTargetAt.y  = at.y;
	mTargetAt.z  = at.z;
	if (mFrameCount > 0) {
		f32 frames = (f32)mFrameCount;
		if (mChaseFrame != 0.0f) {
			CLBChaseConstantSpecifyFrame(&mChaseFrame, 0.0f, frames);
		}
		f32 localDist;
		s16 localAngleX;
		s16 localAngleY;
		CLBCrossToPolar(prev, at, &localDist, &localAngleX, &localAngleY);
		CLBChaseConstantSpecifyFrame(&mSubDist, localDist, frames);
		if (frames < 0.001f) {
			mSubAngleX = localAngleX;
		} else {
			mSubAngleX
			    += CLBRoundf<s16>((f32)(s16)(localAngleX - mSubAngleX)
			                      * (1.0f / frames));
		}
		if (frames < 0.001f) {
			mSubAngleY = localAngleY;
		} else {
			mSubAngleY
			    += CLBRoundf<s16>((f32)(s16)(localAngleY - mSubAngleY)
			                      * (1.0f / frames));
		}
		f32 dx = prev.x - at.x;
		if (!(dx >= 0.0f)) {
			dx = -dx;
		}
		f32 dz = prev.z - at.z;
		if (!(dz >= 0.0f)) {
			dz = -dz;
		}
		if (dx > 0.1f || dz > 0.1f) {
			Vec warp;
			CLBPolarToCross(prev, &warp, mSubDist, mSubAngleX, mSubAngleY);
			mTargetAt.x = warp.x;
			mTargetAt.z = warp.z;
		}
		CLBCrossToPolar(mTargetAt, pos, &localDist, &localAngleX, &localAngleY);
		CLBChaseConstantSpecifyFrame(&mDist, localDist, frames);
		if (frames < 0.001f) {
			mAngleX = localAngleX;
		} else {
			mAngleX += CLBRoundf<s16>((f32)(s16)(localAngleX - mAngleX)
			                          * (1.0f / frames));
		}
		if (mUseAngularVelocity == 0) {
			if (frames < 0.001f) {
				mAngleY = localAngleY;
			} else {
				mAngleY += CLBRoundf<s16>(
				    (f32)(s16)(localAngleY - mAngleY) * (1.0f / frames));
			}
		} else {
			mAngleY = (s16)(mAngleY + mAngularVelocity);
		}
		CLBPolarToCross(mTargetAt, &mTargetPos, mDist, mAngleX, mAngleY);
		mFrameCount -= 1;
		if (mFrameCount == 0) {
			mUseAngularVelocity = 0;
		}
	}
	mPrevAt.x = prev.x;
	mPrevAt.y = prev.y;
	mPrevAt.z = prev.z;
}
