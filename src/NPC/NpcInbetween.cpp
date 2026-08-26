#include <NPC/NpcInbetween.hpp>
#include <M3DUtil/MActor.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>

void TNpcInbetween::execMotionBlend(MActor* mactor)
{
	f32 ratio = 0.0f;
	if (isForcedBlendRatio()) {
		mMotionBlendTimer = 0;
		J3DAnmTransform* old_ptr
		    = mactor->unkC == nullptr
		          ? nullptr
		          : mactor->unkC->getOldMotionBlendAnmPtr();
		if (old_ptr) {
			J3DFrameCtrl local = *mactor->getFrameCtrl(0);
			local.update();
			old_ptr->setFrame(local.getFrame());
			ratio = mForcedBlendRatio;
		}
	} else if (isMotionBlending()) {
		if (mMotionBlendTimer > 0)
			mMotionBlendTimer -= 1;
		if (mMotionBlendTimer > 0) {
			J3DAnmTransform* old_ptr;
			if (mactor->unkC == nullptr)
				old_ptr = nullptr;
			else
				old_ptr = mactor->unkC->getOldMotionBlendAnmPtr();
			if (old_ptr) {
				f32 frame;
				if (mactor->unkC == nullptr)
					frame = 0.0f;
				else
					frame = mactor->unkC->getOldMotionBlendFrame();
				old_ptr->setFrame(frame);
			}
			ratio = (1.0f / (f32)mMotionBlendFrame) * (f32)mMotionBlendTimer;
		}
	}
	if (mactor->unkC)
		mactor->unkC->setMotionBlendRatio(ratio);
}

void TNpcInbetween::execPosInbetween(JGeometry::TVec3<f32>* cur_pos)
{
	mCurrentPos.x = cur_pos->x;
	mCurrentPos.y = cur_pos->y;
	mCurrentPos.z = cur_pos->z;

	if (mPosInbetweenTimer >= 2) {
		mPosInbetweenTimer -= 1;
		f32 ratio  = 1.0f / (f32)mPosInbetweenFrame;
		f32 t      = ratio * (f32)mPosInbetweenTimer;
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
