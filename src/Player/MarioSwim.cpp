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

	if (mFloorPosition.y + mSwimParams.mEndDepth.value > mFloorPosition.z) {
		changePlayerStatus(0x0C400201, 0, false);
		return;
	}

	mForwardVel += 0.03125f * mIntendedMag * mSwimParams.mMoveSp.value;
	mForwardVel *= mSwimParams.mMoveBrake.value;

	s16 rotMin;
	s16 rotMax;
	u32 pumpState = mPumpState;
	if (pumpState == 0 || pumpState == 1) {
		rotMin = mSwimParams.mPumpingRotSpMin.value;
		rotMax = mSwimParams.mPumpingRotSpMax.value;
	} else {
		rotMin = mSwimParams.mSwimmingRotSpMin.value;
		rotMax = mSwimParams.mSwimmingRotSpMax.value;
	}

	s16 rotSpeed = (s16)(0.03125f * mForwardVel
	                         * ((f32)rotMax - (f32)rotMin)
	                     + (f32)rotMin);
	s16 yawDiff  = mIntendedYaw - mFaceAngle.y;
	mFaceAngle.y = mIntendedYaw
	               - IConverge(yawDiff, 0, rotSpeed, rotSpeed);

	setPlayerVelocity(mForwardVel);

	mVel.y -= mSwimParams.mGravity.value;

	f32 depthRatio = (mFloorPosition.z - mPosition.y)
	                 / mSwimParams.mFloatHeight.value;
	if (depthRatio < 0.0f)
		depthRatio = 0.0f;
	if (depthRatio > 1.0f)
		depthRatio = 1.0f;

	u16 animId = mAnimationId;
	if (animId == 0x107 || animId == 0x106 || mAction == 0x22D2) {
		depthRatio *= mSwimParams.mWaitBouyancy.value;
	} else {
		depthRatio *= mSwimParams.mMoveBouyancy.value;
	}

	mVel.y += depthRatio;
	mVel.y *= mSwimParams.mUpDownBrake.value;

	int result = jumpProcess(1);
	if (result == 2) {
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
	}

	if (mFloorPosition.z > 400.0f + mFloorPosition.y) {
		if (mPosition.y < 0.03125f + mFloorPosition.y) {
			if (mAction != 0x22D2) {
				*(u32*)((u8*)this + 0x1A8) = *(u32*)((u8*)this + 0x10);
				*(u32*)((u8*)this + 0x1AC) = *(u32*)((u8*)this + 0x14);
				*(u32*)((u8*)this + 0x1B0) = *(u32*)((u8*)this + 0x18);
				*(f32*)((u8*)this + 0x1AC) = mFloorPosition.y;
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
	if (!(mInput & 0x02))
		return FALSE;

	if (checkFlag(MARIO_FLAG_FLUDD_EMITTING)) {
		if (!isUnderWater()) {
			mPosition.y = 1.0f + mFloorPosition.z;
			startSoundActor(0x828);
			return changePlayerJumping(0x888, 0);
		}
	}

	f32 floorY = mFloorPosition.z;
	f32 canJumpDepth = mSwimParams.mCanJumpDepth.value;
	f32 curY = mPosition.y;
	f32 depth = floorY - canJumpDepth;

	if (depth >= curY) {
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

// swimPaddle: 0x80152014, size 0x130
BOOL TMario::swimPaddle()
{
	f32 animSpeed = 0.5f;
	if (checkFlag(MARIO_FLAG_FLUDD_EMITTING)) {
		animSpeed = 5.0f;
	}
	setAnimation(0x119, animSpeed);

	if (checkFlag(MARIO_FLAG_FLUDD_EMITTING)) {
		f32 paddleUp = mDeParams.mDashMax.value;
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
	if (checkFlag(MARIO_FLAG_IS_PERFORMING)) {
		changePlayerStatus(0x224E1, 0, false);
	}

	if (checkSwimJump())
		return FALSE;

	// Wall collision check
	f32 stickMag = mIntendedMag;
	if (stickMag > 0.0f) {
		const TBGCheckData* wall = mWallPlane;
		if (wall != NULL) {
			u16 wallType = *(u16*)((u8*)wall + 0);
			if (wallType == 0x10A) {
				f32 wallZ = *(f32*)((u8*)wall + 0x34 + 8);
				f32 wallX = *(f32*)((u8*)wall + 0x34);
				s16 angle = (s16)atan2f(wallZ, wallX);
				s16 yawDiff = mModelFaceAngle - angle;
				if (yawDiff < -21845 || yawDiff > 21845) {
					s16 newAngle = angle + 0x8000;
					mModelFaceAngle = (s16)newAngle;
					s16 storedAngle = mModelFaceAngle;
					mFaceAngle.z = storedAngle;
					f32 posY = mPosition.y;
					f32 addY = *(f32*)((u8*)0 + 0);
					mPosition.y = posY + addY;
					changePlayerStatus(0x3000036B, 0, false);
				}
			}
		}
	}

	// Clamp Y
	f32 waterSurface = mPosition.y;
	f32 floatHeight = mSwimParams.mFloatHeight.value;
	f32 curY = mPosition.y;
	f32 minY = waterSurface - floatHeight;
	if (curY <= minY)
		mPosition.y = minY;

	*(f32*)((u8*)this + 0x2AC) = mPosition.y;

	// Check FLUDD
	if (checkFlag(MARIO_FLAG_FLUDD_EMITTING)) {
		if (mAction != 0x24D4 && mAction != 0x24D5) {
			changePlayerStatus(0x24D4, 0, false);
			return FALSE;
		}
	}

	switch (mAction) {
	case 0x22D1:
		doSwimming();
		if (checkActionFlag(0x2000))
			return 1;
		setAnimation(0x115, 0.0f);
		if (checkSwimJump())
			changePlayerStatus(0x22D2, 0, false);
		return FALSE;

	case 0x22D2:
		setAnimation(0x116, 0.0f);
		if (mInput & 0x01)
			changePlayerStatus(0x24D3, 0, false);
		doSwimming();
		if (checkActionFlag(0x2000))
			return 1;
		setAnimation(0x116, 0.0f);
		if (gpMSound->gateCheck(0x1950))
			startSoundActor(0x1950);
		return FALSE;

	case 0x24D3:
		setAnimation(0x117, 0.0f);
		doSwimming();
		if (checkSwimJump())
			changePlayerStatus(0x24D4, 0, false);
		doSwimming();
		if (checkActionFlag(0x2000))
			return 1;
		return FALSE;

	case 0x24D4: {
		setAnimation(0x118, 0.0f);
		f32 fwdVel = mForwardVel;
		f32 accel = mSwimParams.mPaddleSpeedUp.value;
		mForwardVel = fwdVel + accel;
		f32 velY = mVel.y;
		f32 accelY = mSwimParams.mPaddleJumpUp.value;
		mVel.y = velY + accelY;
		doSwimming();
		if (checkSwimJump())
			changePlayerStatus(0x24D5, 0, false);
		doSwimming();
		if (checkActionFlag(0x2000))
			return 1;
		return FALSE;
	}

	case 0x24D5:
		swimPaddle();
		return FALSE;

	case 0x24D6:
		setAnimation(0x11A, 0.0f);
		doSwimming();
		if (checkSwimJump())
			changePlayerStatus(0x24D7, 0, false);
		doSwimming();
		if (checkActionFlag(0x2000))
			return 1;
		return FALSE;

	case 0x24D7:
		setAnimation(0x11B, 0.0f);
		doSwimming();
		if (checkSwimJump())
			changePlayerStatus(0x22D2, 0, false);
		doSwimming();
		if (checkActionFlag(0x2000))
			return 1;
		return FALSE;

	case 0x24D8: {
		setAnimation(0x11C, 0.0f);
		f32 velY = mVel.y;
		f32 accelUp = mSwimParams.mFloatUp.value;
		mVel.y = velY + accelUp;
		doSwimming();
		if (checkSwimJump()) {
			setAnimation(0x116, 0.0f);
			changePlayerStatus(0x22D2, 0, false);
		}
		doSwimming();
		if (!(checkActionFlag(0x2000)))
			return 1;
		return FALSE;
	}

	case 0x24D9:
		doSwimming();
		setAnimation(0x128, 296);
		doSwimming();
		if (checkSwimJump()) {
			mSwimParams.mWaitSinkTime.value = *(s16*)((u8*)this + 0x0366);
		}
		{
			s16 timer = *(s16*)((u8*)this + 0x0366);
			if (timer > 0) {
				*(s16*)((u8*)this + 0x0366) = timer - 1;
				f32 velY = mVel.y;
				f32 sinkSpeed = mSwimParams.mWaitSinkSpeed.value;
				mVel.y = velY - sinkSpeed;
			}
		}
		doSwimming();
		if (checkSwimJump()) {
			setAnimation(0x116, 0.0f);
			changePlayerStatus(0x24D6, 0, false);
		}
		doSwimming();
		if (checkActionFlag(0x2000))
			return 1;
		return FALSE;

	case 0x24DA:
		doSwimming();
		setAnimation(0x24DA, 298);
		doSwimming();
		if (checkSwimJump())
			changePlayerStatus(0x22D2, 0, false);
		return FALSE;

	case 0x224E0:
		doSwimming();
		setAnimation(0x24E0, 268);
		return FALSE;

	case 0x224E1:
		doSwimming();
		setAnimation(0x24E1, 299);
		return FALSE;
	}

	return FALSE;
}
