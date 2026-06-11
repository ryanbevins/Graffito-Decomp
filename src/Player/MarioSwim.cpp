#include <Player/MarioMain.hpp>

#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <JSystem/JMath.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Player/Watergun.hpp>
#include <Strategic/LiveActor.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSound.hpp>
#include <System/EmitterViewObj.hpp>
#include <dolphin/mtx.h>
#include <fake_tgmath.h>
#include <Camera/Camera.hpp>

// MarioSwim: -inline deferred, functions in REVERSE address order.

// doSwimming: 0x801522B4, size 0x384
void TMario::doSwimming()
{
	if (mInput & 0x8000) {
		changePlayerStatus(0x24D9, 0, false);
		return;
	}

	if (mFloorPosition.y + mSwimParams.mEndDepth.get() > mFloorPosition.z) {
		changePlayerStatus(0x0C400201, 0, false);
		return;
	}

	mForwardVel += mIntendedMag * mSwimParams.mMoveSp.get() * 0.03125f;
	mForwardVel *= mSwimParams.mMoveBrake.get();

	f32 rotMin;
	f32 rotMax;
	u32 pumpState = mPumpState;
	bool isPumping;
	if (pumpState == 0 || pumpState == 1)
		isPumping = true;
	else
		isPumping = false;
	if (isPumping) {
		rotMin = mSwimParams.mPumpingRotSpMin.get();
		rotMax = mSwimParams.mPumpingRotSpMax.get();
	} else {
		rotMin = mSwimParams.mSwimmingRotSpMin.get();
		rotMax = mSwimParams.mSwimmingRotSpMax.get();
	}

	s16 rotSpeed = (s16)(mForwardVel * (rotMax - rotMin) * 0.03125f
	                     + rotMin);
	s16 yawDiff  = mIntendedYaw - mFaceAngle.y;
	mFaceAngle.y = mIntendedYaw
	               - IConverge(yawDiff, 0, rotSpeed, rotSpeed);

	setPlayerVelocity(mForwardVel);

	mVel.y -= mSwimParams.mGravity.get();

	f32 depthRatio = (mFloorPosition.z - mPosition.y)
	                 / mSwimParams.mFloatHeight.get();
	if (depthRatio < 0.0f)
		depthRatio = 0.0f;
	if (depthRatio > 1.0f)
		depthRatio = 1.0f;

	u16 animId = mAnimationId;
	if (animId == 0x107 || animId == 0x106 || mAction == 0x22D2) {
		depthRatio *= mSwimParams.mWaitBouyancy.get();
	} else {
		depthRatio *= mSwimParams.mMoveBouyancy.get();
	}

	mVel.y += depthRatio;
	mVel.y *= mSwimParams.mUpDownBrake.get();

	switch (jumpProcess(1)) {
	case 1:
		break;
	case 2:
		if (checkFlag(MARIO_FLAG_FLUDD_EMITTING)) {
			if (isUnderWater()) {
				mForwardVel = -mForwardVel * 0.8f;
				changePlayerStatus(0x24DA, 0, true);
			} else {
				changePlayerStatus(0x208B3, 0, true);
				mForwardVel = -mForwardVel * 0.8f;
				mVel.y      = 50.0f;
			}
		} else {
			setPlayerVelocity(0.0f);
			mVel.y = 0.0f;
		}
		break;
	}

	if (mFloorPosition.z > 400.0f + mFloorPosition.y) {
		if (mPosition.y < 0.03125f + mFloorPosition.y) {
			if (mAction != 0x22D2) {
				unk1A8 = mPosition;
				unk1A8.y = mFloorPosition.y;
				gpMarioParticleManager->emitAndBindToPosPtr(
				    0x11E, &unk1A8, 1, this);
			}
		}
	}

	f32 minY = 35.0f + mFloorPosition.y;
	if (mPosition.y < minY)
		mPosition.y = minY;
}

// checkSwimJump: 0x80152144, size 0x170
BOOL TMario::checkSwimJump()
{
	if (mInput & 0x02) {
		if (checkFlag(MARIO_FLAG_FLUDD_EMITTING)) {
			if (!isUnderWater()) {
				mPosition.y = 1.0f + mFloorPosition.z;
				startSoundActor(0x828);
				return changePlayerJumping(0x888, 0);
			}
		}

		f32 depth = mFloorPosition.z - mSwimParams.mCanJumpDepth.value;
		if (depth < mPosition.y) {
			u8 shouldDive = 0;
			f32 stickMag  = mIntendedMag;
			if (stickMag == 0.0f)
				shouldDive = 1;
			if (mWallPlane != NULL)
				shouldDive = 1;
			s16 diff = mModelFaceAngle - mIntendedYaw;
			if (diff < -21845 || diff > 21845)
				shouldDive = 1;

			if ((u8)shouldDive == 1) {
				inOutWaterEffect(mFloorPosition.z);
				changePlayerStatus(0x02000880, 0, false);
				return TRUE;
			}
		}

		if (mIntendedMag == 0.0f)
			return changePlayerStatus(0x24D8, 0, false);

		return changePlayerStatus(0x24D4, 0, false);
	}

	return FALSE;
}

// swimPaddle: 0x80152014, size 0x130
BOOL TMario::swimPaddle()
{
	f32 animSpeed = 0.5f;
	if (checkFlag(MARIO_FLAG_FLUDD_EMITTING)) {
		animSpeed = 5.0f;
	}
	setAnimation(0x119, animSpeed);

	if (checkFlag(MARIO_FLAG_FLUDD_EMITTING)) {
		f32 paddleUp = mDeParams.mDashMax.get();
		setPlayerVelocity(paddleUp);
		startSoundActor(0x19);
		startSoundActor(0x117D);
	}

	f32 stickMag = mIntendedMag;
	if (stickMag == 0.0f) {
		changePlayerStatus(0x24D6, 0, false);
	}

	doSwimming();

	if (!checkActionFlag(0x2000)) {
		return 1;
	}

	if (checkFlag(MARIO_FLAG_FLUDD_EMITTING)) {
		if (!isUnderWater()) {
			surfingEffect();
		}
	}
	return 0;
}

// swimMain: 0x8015191C, size 0x6F8
BOOL TMario::swimMain()
{
	if (checkFlag(MARIO_FLAG_GAME_OVER)) {
		changePlayerStatus(0x224E1, 0, false);
	}

	if (checkSwimJump() == TRUE)
		return FALSE;

	// Wall collision check
	f32 stickMag = mIntendedMag;
	if (stickMag > 0.0f) {
		const TBGCheckData* wall = mWallPlane;
		if (wall != NULL) {
			bool isFence;
			if (wall->mBGType == 0x10A)
				isFence = true;
			else
				isFence = false;
			if (isFence) {
				const JGeometry::TVec3<f32>& normal = wall->getNormal();
				s16 angle   = matan(normal.z, normal.x);
				s16 yawDiff = mFaceAngle.y - angle;
				if (yawDiff < -21845 || yawDiff > 21845) {
					mFaceAngle.y     = angle + 0x8000;
					mModelFaceAngle  = mFaceAngle.y;
					mPosition.y     += 100.0f;
					changePlayerStatus(0x3000036B, 0, false);
				}
			}
		}
	}

	// Clamp Y
	f32 minY = mFloorPosition.z - mSwimParams.mFloatHeight.value;
	if (mPosition.y <= minY)
		mPosition.y = minY;

	*(f32*)((u8*)this + 0x2AC) = mFloorPosition.z;

	// Check FLUDD
	if (checkFlag(MARIO_FLAG_FLUDD_EMITTING)) {
		if (mAction != 0x24D4 && mAction != 0x24D5) {
			return changePlayerStatus(0x24D4, 0, false);
		}
	}

	switch (mAction) {
	case 0x22D1:
		doSwimming();
		if (!checkActionFlag(0x2000))
			return TRUE;
		setAnimation(0x115, 1.0f);
		if (isLast1AnimeFrame())
			changePlayerStatus(0x22D2, 0, false);
		return FALSE;

	case 0x22D2:
		setAnimation(0x116, 1.0f);
		if (mInput & 0x01) {
			return changePlayerStatus(0x24D3, 0, false);
		}
		doSwimming();
		if (!checkActionFlag(0x2000))
			return TRUE;
		setAnimation(0x116, 1.0f);
		if (gpMSound->gateCheck(0x1950))
			MSoundSESystem::MSRandPlay::startSeRandPlay(0x1950, 0);
		return FALSE;

	case 0x24D3:
		setAnimation(0x117, 1.0f);
		if (isLast1AnimeFrame())
			changePlayerStatus(0x24D4, 0, false);
		doSwimming();
		if (!checkActionFlag(0x2000))
			return TRUE;
		return FALSE;

	case 0x24D4: {
		setAnimation(0x118, 1.0f);
		f32 fwdVel = mForwardVel;
		f32 accel = mSwimParams.mPaddleSpeedUp.value;
		mForwardVel = fwdVel + accel;
		f32 velY = mVel.y;
		f32 accelY = mSwimParams.mPaddleJumpUp.value;
		mVel.y = velY + accelY;
		if (isLast1AnimeFrame())
			changePlayerStatus(0x24D5, 0, false);
		doSwimming();
		if (!checkActionFlag(0x2000))
			return TRUE;
		return FALSE;
	}

	case 0x24D5:
		return swimPaddle();

	case 0x24D6:
		setAnimation(0x11A, 1.0f);
		if (isLast1AnimeFrame()) {
			return changePlayerStatus(0x24D7, 0, false);
		}
		doSwimming();
		if (!checkActionFlag(0x2000))
			return TRUE;
		return FALSE;

	case 0x24D7:
		setAnimation(0x11B, 1.0f);
		if (isLast1AnimeFrame()) {
			return changePlayerStatus(0x22D2, 0, false);
		}
		doSwimming();
		if (!checkActionFlag(0x2000))
			return TRUE;
		return FALSE;

	case 0x24D8: {
		setAnimation(0x11C, 1.0f);
		f32 velY = mVel.y;
		f32 accelUp = mSwimParams.mFloatUp.value;
		mVel.y = velY + accelUp;
		if (isLast1AnimeFrame()) {
			setAnimation(0x116, 1.0f);
			return changePlayerStatus(0x22D2, 0, false);
		}
		doSwimming();
		if (!(checkActionFlag(0x2000)))
			return TRUE;
		return FALSE;
	}

	case 0x24D9:
		setAnimation(0x128, 1.0f);
		if (getMotionFrameCtrl().checkPass(16.0f))
			unk366 = mSwimParams.mWaitSinkTime.value;
		{
			int timer = unk366;
			if (timer > 0) {
				unk366 = timer - 1;
				f32 velY = mVel.y;
				f32 sinkSpeed = mSwimParams.mWaitSinkSpeed.value;
				mVel.y = velY - sinkSpeed;
			}
		}
		if (isLast1AnimeFrame()) {
			setAnimation(0x116, 1.0f);
			return changePlayerStatus(0x24D6, 0, false);
		}
		doSwimming();
		if (!checkActionFlag(0x2000))
			return TRUE;
		return FALSE;

	case 0x24DA:
		doSwimming();
		jumpingDemoCommon(0x24DA, 0x12A, 0.0f);
		if (isLast1AnimeFrame())
			changePlayerStatus(0x22D2, 0, false);
		return FALSE;

	case 0x224E0:
		doSwimming();
		jumpingDemoCommon(0x224E0, 0x10C, 0.0f);
		return FALSE;

	case 0x224E1:
		doSwimming();
		jumpingDemoCommon(0x224E1, 0x12B, 0.0f);
		return FALSE;
	}

	return FALSE;
}
