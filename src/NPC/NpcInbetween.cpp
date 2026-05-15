#include <NPC/NpcInbetween.hpp>
#include <M3DUtil/MActor.hpp>

void TNpcInbetween::execPosInbetween(JGeometry::TVec3<f32>* cur_pos)
{
	mCurrentPos.x = cur_pos->x;
	mCurrentPos.y = cur_pos->y;
	mCurrentPos.z = cur_pos->z;

	if (mPosInbetweenTimer >= 2) {
		mPosInbetweenTimer -= 1;
		f32 ratio  = 1.0f / ((f32)mPosInbetweenFrame - 1.0f);
		f32 t      = ratio * ((f32)mPosInbetweenTimer - 1.0f);
		cur_pos->x = t * (mTargetPos.x - mCurrentPos.x) + mCurrentPos.x;
		cur_pos->y = t * (mTargetPos.y - mCurrentPos.y) + mCurrentPos.y;
		cur_pos->z = t * (mTargetPos.z - mCurrentPos.z) + mCurrentPos.z;
	} else {
		mTargetPos.x       = cur_pos->x;
		mTargetPos.y       = cur_pos->y;
		mTargetPos.z       = cur_pos->z;
		mPosInbetweenTimer = 0;
	}
}
