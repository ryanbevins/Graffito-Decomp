#include <Player/MarioMain.hpp>
#include <MSound/MSoundBGM.hpp>

#include <M3DUtil/M3UJoint.hpp>
#include <Map/Map.hpp>
#include <JSystem/JMath.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Map/MapData.hpp>
#include <Map/MapCollisionData.hpp>
#include <MSound/MSound.hpp>
#include <Map/PollutionManager.hpp>
#define WATERGUN_EMIT_IS_EMITTING
#include <Player/Watergun.hpp>
#undef WATERGUN_EMIT_IS_EMITTING
#include <Player/NozzleBase.hpp>
#include <Player/NozzleTrigger.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <MSound/MSoundSE.hpp>
#include <fake_tgmath.h>

// NOTE: -inline deferred means functions must be in REVERSE address order.

static f32 dummy1431[3] = { 1.0f, 1.0f, 1.0f };
static f32 dummy1411[3] = { 1.0f, 1.0f, 1.0f };
static u32 dummy1210[4] = { 0, 2, 1, 3 };

static const char* MtxCalcTypeName[4]
    = { "MActorMtxCalcType_Basic "
        "\x83\x4E\x83\x89\x83\x56\x83\x62\x83\x4E\x83\x58\x83\x50"
        "\x81\x5B\x83\x8B\x82\x6E\x82\x6D",
	    "MActorMtxCalcType_Softimage "
        "\x83\x4E\x83\x89\x83\x56\x83\x62\x83\x4E\x83\x58\x83\x50"
        "\x81\x5B\x83\x8B\x82\x6E\x82\x65\x82\x65",
	    "MActorMtxCalcType_MotionBlend "
        "\x83\x82\x81\x5B\x83\x56\x83\x87\x83\x93\x83\x75\x83\x8C"
        "\x83\x93\x83\x68",
	    "MActorMtxCalcType_User "
        "\x83\x86\x81\x5B\x83\x55\x81\x5B\x92\xE8\x8B\x60" };

static inline BOOL checkRotateStartInput(TMario* mario, int* stickDir)
{
	BOOL result = mario->checkStickRotate(stickDir);
	return result;
}

static inline BOOL checkRotateStartTrigger(TMario* mario, int* stickDir)
{
	BOOL result = checkRotateStartInput(mario, stickDir);
	return result;
}

// considerRotateStart - 0x8013C118
BOOL TMario::considerRotateStart()
{
	int stickDir;
	if (checkRotateStartTrigger(this, &stickDir) != 1)
		goto fail;

	TWaterGun* gun;
	if ((gun = mWaterGun) == nullptr)
		goto fail;

	u8 canSpray;
	if (gun->mCurrentWater == 0) {
		canSpray = 0;
	} else {
		s32 kind = gun->getCurrentNozzle()->getNozzleKind();
		if (kind == 1) {
			TNozzleTrigger* trigger = (TNozzleTrigger*)gun->getCurrentNozzle();
			if (trigger->unk385 == 1) {
				canSpray = 1;
			} else {
				canSpray = 0;
			}
		} else {
			if (gun->getCurrentNozzle()->unk378 > 0.0f) {
				canSpray = 1;
			} else {
				canSpray = 0;
			}
		}
	}

	if (canSpray) {
		if (stickDir > 0) {
			changePlayerStatus(0x0441, 0, false);
		} else {
			changePlayerStatus(0x0442, 0, false);
		}
		return;
	} else {
		goto fail;
	}

fail:
	return false;
}

// isRunningInWater - 0x8013C0D0
bool TMario::isRunningInWater()
{
	u8 flag;
	if (mState & 0x30000) {
		flag = 1;
	} else {
		flag = 0;
	}
	if (flag) {
		if (mFloorPosition.z < mPosition.y + mRunParams.mSwimDepth.get()) {
			return 1;
		}
	}
	return 0;
}

// doRunningAnimation - 0x8013BFA4
BOOL TMario::doRunningAnimation()
{
	s32 redo = 1;
	f32 speed;
	f32 intended = mIntendedMag;
	f32 fwdVel = mForwardVel;
	if (intended > fwdVel) {
		speed = intended;
	} else {
		speed = fwdVel;
	}

	f32 clampedSpeed = speed;
	if (speed < 4.0f)
		clampedSpeed = 4.0f;

	f32 anmSpd1 = 1.0f;
	f32 softWalk = 0.1f;
	f32 walk2Soft = 0.0f;

	while (redo) {
		u16 anmId = mAnimationId;
		switch (anmId) {
		case 0x72:
		default: {
			if (mForwardVel >= mDeParams.mDashMax.get() - anmSpd1) {
				setAnimation(0xF5, anmSpd1);
				redo = 0;
				break;
			}

			if (clampedSpeed < mRunParams.mWalk2Soft.get()) {
				setAnimation(0x92, anmSpd1);
				redo = 0;
				break;
			}

			f32 anmSpeed = clampedSpeed * mRunParams.mRunAnmSpeedMult.get() + mRunParams.mRunAnmSpeedBase.get();

			if (isRunningInWater()) {
				f32 depth = mFloorPosition.z - mPosition.y;
				f32 swimDepth = mRunParams.mSwimDepth.get();
				f32 brake = anmSpd1 - mRunParams.mInWaterBrake.get();
				f32 ratio = depth / swimDepth;
				f32 scale = anmSpd1 - ratio * brake;
				anmSpeed = anmSpeed * (scale * mRunParams.mInWaterAnmBrake.get());
			}

			setAnimation(0x72, anmSpeed);

			f32 walkSp = mRunParams.mMotBlendWalkSp.get();
			f32 runSp = mRunParams.mMotBlendRunSp.get();
			f32 blend;
			if (anmSpeed < walkSp)
				blend = 0.0f;
			if (runSp < anmSpeed)
				blend = 1.0f;
			if (walkSp <= anmSpeed && anmSpeed <= runSp) {
				blend = (anmSpeed - walkSp) / (runSp - walkSp);
			}

			unk414.z = anmSpd1 - blend;

			if (onYoshi()) {
				mModel->unk20->unk18->unk50 = walk2Soft;
			} else {
				mModel->unk20->unk18->unk50 = unk414.z;
			}

			redo = 0;
			break;
		}
		case 0x92: {
			if (clampedSpeed > mRunParams.mSoft2Walk.get()) {
				setAnimation(0x72, 1.0f);
				redo = 0;
				break;
			}
			f32 mult;
			if ((mult = clampedSpeed) < softWalk)
				mult = softWalk;
			mult *= mRunParams.mSoftStepAnmMult.value;
			setAnimation(0x92, mult);
			redo = 0;
			break;
		}
		case 0xF5: {
			if (mForwardVel < mDeParams.mDashMax.get() - anmSpd1) {
				setAnimation(0x72, anmSpd1);
				redo = 0;
				break;
			}
			f32 anmSpeed = mRunParams.mRunAnmSpeedMult.get() * clampedSpeed
			               + mRunParams.mRunAnmSpeedBase.get();
			setAnimation(0xF5, anmSpeed);
			redo = 0;
			break;
		}
		}
	}

	return 1;
}

void TMario::getSlopeNormalAccele(f32* accelUp, f32* accelDown)
{
	if ((u8)isForceSlip()) {
		*accelUp = mSlipParamsAll.mSlopeAcceleUp.value;
		*accelDown = mSlipParamsAll.mSlopeAcceleDown.value;
		return;
	}

	const TBGCheckData* ground = mGroundPlane;
	u16 type = ground->mBGType;

	u8 isOil;
	if (type == 0x0c || type == 0x800c || type == 0xa00c)
		isOil = 1;
	else
		isOil = 0;
	if (isOil) {
		*accelUp = mSlipParamsAllSlider.mSlopeAcceleUp.value;
		*accelDown = mSlipParamsAllSlider.mSlopeAcceleDown.value;
		return;
	}

	u8 isAll;
	if (type == 0x02 || type == 0x8002)
		isAll = 1;
	else
		isAll = 0;
	if (isAll) {
		*accelUp = mSlipParams45.mSlopeAcceleUp.value;
		*accelDown = mSlipParams45.mSlopeAcceleDown.value;
		return;
	}

	u8 isWater;
	if (type == 0x04 || type == 0x4004 || type == 0x8004 || type == 0xc004)
		isWater = 1;
	else
		isWater = 0;
	if (isWater) {
		if (ground->mNormal.y > 0.99f) {
			*accelUp = mSlipParamsWaterGround.mSlopeAcceleUp.value;
			*accelDown = mSlipParamsWaterGround.mSlopeAcceleDown.value;
		} else {
			*accelUp = mSlipParamsWaterSlope.mSlopeAcceleUp.value;
			*accelDown = mSlipParamsWaterSlope.mSlopeAcceleDown.value;
		}
		return;
	}

	*accelUp = mSlipParamsNormal.mSlopeAcceleUp.value;
	*accelDown = mSlipParamsNormal.mSlopeAcceleDown.value;
}

void TMario::getSlopeSlideAccele(f32* accelUp, f32* accelDown)
{
	if ((u8)isForceSlip()) {
		*accelUp = mSlipParamsAll.mSlideAcceleUp.value;
		*accelDown = mSlipParamsAll.mSlideAcceleDown.value;
		return;
	}

	const TBGCheckData* ground = mGroundPlane;
	u16 type = ground->mBGType;

	u8 isOil;
	if (type == 0x0c || type == 0x800c || type == 0xa00c)
		isOil = 1;
	else
		isOil = 0;
	if (isOil) {
		*accelUp = mSlipParamsAllSlider.mSlideAcceleUp.value;
		*accelDown = mSlipParamsAllSlider.mSlideAcceleDown.value;
		return;
	}

	u8 isAll;
	if (type == 0x02 || type == 0x8002)
		isAll = 1;
	else
		isAll = 0;
	if (isAll) {
		*accelUp = mSlipParams45.mSlideAcceleUp.value;
		*accelDown = mSlipParams45.mSlideAcceleDown.value;
		return;
	}

	u8 isWater;
	if (type == 0x04 || type == 0x4004 || type == 0x8004 || type == 0xc004)
		isWater = 1;
	else
		isWater = 0;
	if (isWater) {
		if (ground->mNormal.y > 0.99f) {
			*accelUp = mSlipParamsWaterGround.mSlideAcceleUp.value;
			*accelDown = mSlipParamsWaterGround.mSlideAcceleDown.value;
		} else {
			*accelUp = mSlipParamsWaterSlope.mSlideAcceleUp.value;
			*accelDown = mSlipParamsWaterSlope.mSlideAcceleDown.value;
		}
		return;
	}

	*accelUp = mSlipParamsNormal.mSlideAcceleUp.value;
	*accelDown = mSlipParamsNormal.mSlideAcceleDown.value;
}

// getChangeAngleSpeed - returns angle change speed (s16 param -> f32 -> * forwardVel * 1/32)
// Despite being declared void, the asm returns a value in f1
f32 TMario::getChangeAngleSpeed()
{
	f32 angleSpeed;

	if ((u8)isForceSlip()) {
		angleSpeed = (f32) mSlipParamsAll.mSlideAngleYSp.value;
	} else {
		const TBGCheckData* ground = mGroundPlane;
		u16 type = ground->mBGType;

		u8 isOil;
		if (type == 0x0c || type == 0x800c || type == 0xa00c)
			isOil = 1;
		else
			isOil = 0;
		if (isOil) {
			angleSpeed = (f32) mSlipParamsAllSlider.mSlideAngleYSp.value;
		} else {
			u8 isAll;
			if (type == 0x02 || type == 0x8002)
				isAll = 1;
			else
				isAll = 0;
			if (isAll) {
				angleSpeed = (f32) mSlipParams45.mSlideAngleYSp.value;
			} else {
				u8 isWater;
				if (type == 0x04 || type == 0x4004
				    || type == 0x8004 || type == 0xc004)
					isWater = 1;
				else
					isWater = 0;
				if (isWater) {
					if (ground->mNormal.y > 0.99f) {
						angleSpeed = (f32) *(s16*)((u8*)this
						                           + 0x2F70);
					} else {
						angleSpeed = (f32) *(s16*)((u8*)this
						                           + 0x2E8C);
					}
				} else {
					angleSpeed
					    = (f32) mSlipParamsNormal.mSlideAngleYSp.value;
				}
			}
		}
	}

	return (angleSpeed * mForwardVel) * 0.03125f;
}

// getSlideStickMult - 0x8013B8E8
f32 TMario::getSlideStickMult()
{
	if ((u8)isForceSlip()) {
		return mSlipParamsAll.mStickSlideMult.get();
	}

	u16 groundType = mGroundPlane->mBGType;

	u8 isIce;
	if (groundType == 0x0C || groundType == 0x800C || groundType == 0xA00C) {
		isIce = 1;
	} else {
		isIce = 0;
	}
	if (isIce) {
		return mSlipParamsAllSlider.mStickSlideMult.get();
	}

	u8 isSand;
	if (groundType == 0x02 || groundType == 0x8002) {
		isSand = 1;
	} else {
		isSand = 0;
	}
	if (isSand) {
		return mSlipParams45.mStickSlideMult.get();
	}

	return mSlipParamsNormal.mStickSlideMult.get();
}

// slideProcess - 0x80139E28
void TMario::slideProcess(f32 accelUp, f32 accelDown)
{
	const TBGCheckData* ground = mGroundPlane;
	s32 slopeAngle = matan(ground->mNormal.z, ground->mNormal.x);

	register f32 sqrtMag = ground->mNormal.x * ground->mNormal.x
	                       + ground->mNormal.z * ground->mNormal.z;
	if (sqrtMag > 0.0f) {
		f64 root = __frsqrte(sqrtMag);
		f64 refined = 0.5f * root * (3.0f - sqrtMag * (root * root));
		volatile f32 result = sqrtMag * refined;
		sqrtMag             = result;
	}

	s16 faceY = mFaceAngle.y;
	f32 accelUpLocal;
	f32 accelDownLocal;
	s32 angleDiff = mSlopeAngle - faceY;
	getSlopeSlideAccele(&accelUpLocal, &accelDownLocal);

	s16 diffExt = (s16)angleDiff;
	if (diffExt > -16384 && diffExt < 16384) {
		accelUp = accelUp + accelUpLocal * sqrtMag;
	} else {
		accelUp = accelUp + accelDownLocal * sqrtMag;
	}

	u16 uAngle = (u16)slopeAngle;
	mSlideVelX = mSlideVelX + accelUp * JMASSin(uAngle);
	mSlideVelZ = mSlideVelZ + accelUp * JMASCos(uAngle);

	mSlideVelX = mSlideVelX * accelDown;
	mSlideVelZ = mSlideVelZ * accelDown;

	mSlideAngle = (u16)matan(mSlideVelZ, mSlideVelX);

	f32 negThresh = -0.1f;
	f32 posThresh = 0.1f;
	s32 angleChange;
	if (negThresh < mSlideVelX && mSlideVelX < posThresh
	    && negThresh < mSlideVelZ && mSlideVelZ < posThresh) {
		// velocity very small, skip angle processing
	} else {
		s16 slideAngle = mSlideAngle;
		s16 faceDiff = (s16)(mFaceAngle.y - slideAngle);
		angleChange  = (s16)faceDiff;

		if (angleChange > 0 && angleChange <= 16384) {
			f32 changeAngleSpd = getChangeAngleSpeed();
			angleChange = (s32)((f32)angleChange - changeAngleSpd);
			if (angleChange < 0)
				angleChange = 0;
		} else if (angleChange > -16384 && angleChange < 0) {
			f32 changeAngleSpd = getChangeAngleSpeed();
			angleChange = (s32)((f32)angleChange + changeAngleSpd);
			if (angleChange > 0)
				angleChange = 0;
		} else if (angleChange > 16384 && angleChange < 0x8000) {
			f32 changeAngleSpd = getChangeAngleSpeed();
			angleChange = (s32)((f32)angleChange + changeAngleSpd);
			if (angleChange > 0x8000)
				angleChange = 0x8000;
		} else if (angleChange > -32768 && angleChange < -16384) {
			f32 changeAngleSpd = getChangeAngleSpeed();
			angleChange = (s32)((f32)angleChange - changeAngleSpd);
			if (angleChange < -32768)
				angleChange = -32768;
		}

		mFaceAngle.y = (s16)(mSlideAngle + angleChange);
	}

	mVel.x = mSlideVelX;
	f32 zero = 0.0f;
	mVel.y = zero;
	mVel.z = mSlideVelZ;

	register f32 velMag = mSlideVelX * mSlideVelX + mSlideVelZ * mSlideVelZ;
	if (velMag > zero) {
		f64 root = __frsqrte(velMag);
		f64 refined = 0.5f * root * (3.0f - velMag * (root * root));
		volatile f32 result = velMag * refined;
		velMag             = result;
	}
	mForwardVel = velMag;

	f32 maxSlideSpeed = 50.0f;
	if (mForwardVel > maxSlideSpeed) {
		mSlideVelX = maxSlideSpeed * mSlideVelX / mForwardVel;
		mSlideVelZ = maxSlideSpeed * mSlideVelZ / mForwardVel;
	}

	if (angleChange < -16384 || angleChange > 16384) {
		mForwardVel *= -1.0f;
	}
}

// doSliding - 0x80139A3C
int TMario::doSliding(f32 stopThreshold)
{
	s32 result = 0;

	// Compute sine/cosine of face-slide direction difference
	s16 slideDir = mSlideAngle;
	s16 faceDir = mIntendedYaw;
	u16 angleDiff = (u16)(faceDir - slideDir);
	f32 stickCos = JMASCos(angleDiff);
	f32 sinVal   = JMASSin(angleDiff);

	// Adjust acceleration based on forward velocity sign
	if (stickCos < 0.0f) {
		f32 fwdVel = mForwardVel;
		if (fwdVel >= 0.0f) {
			f32 factor = 0.01f * (0.5f * fwdVel) + 0.5f;
			stickCos *= factor;
		}
	}

	// Select slide friction (slideStop) based on action / ground type
	f32 slideStop;
	if ((mAction - 0x00840000) == 0x045D) {
		slideStop = mSlipParamsOil.mSlipFriction.value;
	} else if ((u8)isForceSlip()) {
		slideStop = mSlipParamsAll.mSlipFriction.value;
	} else {
		const TBGCheckData* ground2 = mGroundPlane;
		u16 type2 = ground2->mBGType;
		u8 isOil2;
		if (type2 == 0x0c || type2 == 0x800c || type2 == 0xa00c)
			isOil2 = 1;
		else
			isOil2 = 0;
		if (isOil2) {
			slideStop = mSlipParamsAllSlider.mSlipFriction.value;
		} else {
			u8 isAll2;
			if (type2 == 0x02 || type2 == 0x8002)
				isAll2 = 1;
			else
				isAll2 = 0;
			if (isAll2) {
				slideStop = mSlipParams45.mSlipFriction.value;
			} else {
				u8 isWater2;
				if (type2 == 0x04 || type2 == 0x4004 || type2 == 0x8004 || type2 == 0xc004)
					isWater2 = 1;
				else
					isWater2 = 0;
				if (isWater2) {
					if (ground2->mNormal.y > 0.99f)
						slideStop = mSlipParamsWaterGround.mSlipFriction.value;
					else
						slideStop = mSlipParamsWaterSlope.mSlipFriction.value;
				} else if (onYoshi()) {
					slideStop = mSlipParamsYoshi.mSlipFriction.value;
				} else {
					slideStop = mSlipParamsNormal.mSlipFriction.value;
					if ((mAction - 0x00800000) == 0x0456) {
						if (mActionState == 1)
							slideStop = mDeParams.mWasOnWaterSlip.value;
						u8 isInShallow;
						if (mState & 0x30000)
							isInShallow = 1;
						else
							isInShallow = 0;
						if (isInShallow)
							slideStop = mDeParams.mInWaterSlip.value;
					}
				}
			}
		}
	}
	f32 intendedMag = mIntendedMag;
	f32 stickScale  = 0.03125f;
	f32 slideAdjust = intendedMag * stickScale;
	slideStop += 0.02f * (slideAdjust * stickCos);

	// Compute slide deceleration
	f32 oldSpeed = mSlideVelX * mSlideVelX + mSlideVelZ * mSlideVelZ;
	if (oldSpeed > 0.0f) {
		f64 root = __frsqrte(oldSpeed);
		f64 refined = 0.5f * root * (3.0f - oldSpeed * (root * root));
		volatile f32 result = oldSpeed * refined;
		oldSpeed            = result;
	}

	f32 stickMult = getSlideStickMult();
	intendedMag = mIntendedMag;
	f32 stickEffect = intendedMag * stickScale;
	f32 accelX = mSlideVelZ * stickEffect * sinVal;
	mSlideVelX = mSlideVelX + accelX * stickMult;

	stickMult = getSlideStickMult();
	intendedMag = mIntendedMag;
	stickEffect = intendedMag * stickScale;
	f32 accelZ = mSlideVelX * stickEffect * sinVal;
	mSlideVelZ = mSlideVelZ - accelZ * stickMult;

	// Compute new speed
	f32 newSxSq  = mSlideVelX * mSlideVelX;
	f32 newSzSq  = mSlideVelZ * mSlideVelZ;
	f32 newSpeed = newSxSq + newSzSq;
	if (newSpeed > 0.0f) {
		f64 root = __frsqrte(newSpeed);
		f64 refined = 0.5f * root * (3.0f - newSpeed * (root * root));
		volatile f32 result = newSpeed * refined;
		newSpeed           = result;
	}

	// Keep speed from increasing
	if (oldSpeed > 0.0f && newSpeed > 0.0f) {
		mSlideVelX = mSlideVelX * oldSpeed / newSpeed;
		mSlideVelZ = mSlideVelZ * oldSpeed / newSpeed;
	}

	slideProcess(0.0f, slideStop);

	// Check ground type for stop conditions
	const TBGCheckData* ground3 = mGroundPlane;
	u16 type3 = ground3->mBGType;
	u8 isSlippery;
	if (type3 == 0x01 || type3 == 0x4001 || type3 == 0x8001 || type3 == 0xC001)
		isSlippery = 1;
	else
		isSlippery = 0;
	if (!isSlippery) {
		u8 isOil3;
		if (type3 == 0x0c || type3 == 0x800c || type3 == 0xa00c)
			isOil3 = 1;
		else
			isOil3 = 0;
		if (!isOil3) {
			if (mForwardVel * mForwardVel < stopThreshold * stopThreshold) {
				setPlayerVelocity(0.0f);
				mInput = mInput & ~0x8;
				result = 1;
			}
		}
	}

	return result;
}

void TMario::slopeProcess()
{
	const TBGCheckData* ground = mGroundPlane;

	const JGeometry::TVec3<f32>* normal = &ground->mNormal;
	f32 nx                              = normal->x;
	f32 nxSq                            = nx * nx;
	f32 nz                              = normal->z;
	register f32 sqrtMag = nxSq + nz * nz;
	if (sqrtMag > 0.0f) {
		const f64 half  = 0.5;
		const f64 three = 3.0;
		f64 guess       = __frsqrte((f64)sqrtMag);
		guess = half * guess * (three - guess * guess * sqrtMag);
		guess = half * guess * (three - guess * guess * sqrtMag);
		guess = half * guess * (three - guess * guess * sqrtMag);
		volatile f32 result = (f32)(sqrtMag * guess);
		sqrtMag             = result;
	}

	s16 faceY = mFaceAngle.y;
	s16 slopeAngle = mSlopeAngle;
	s32 angleDiff = slopeAngle - faceY;

	f32 accelUp;
	f32 accelDown;
	getSlopeNormalAccele(&accelUp, &accelDown);

	s16 diffExt = (s16)angleDiff;
	if (diffExt > -16384 && diffExt < 16384) {
		mForwardVel = mForwardVel + accelUp * sqrtMag;
	} else {
		mForwardVel = mForwardVel - accelDown * sqrtMag;
	}

	if (mForwardVel > mDeParams.mRunningMax.value) {
		mForwardVel = mDeParams.mRunningMax.value;
	}

	s16 modelAngle = mFaceAngle.y;
	mSlideAngle = modelAngle;

	u16 angle = mFaceAngle.y;
	f32 sinVal = JMASSin(angle);
	mSlideVelX = mForwardVel * sinVal;

	angle = mFaceAngle.y;
	f32 cosVal = JMASCos(angle);
	mSlideVelZ = mForwardVel * cosVal;

	mVel.x = mSlideVelX;
	mVel.y = 0.0f;
	mVel.z = mSlideVelZ;
}

// doRunning - 0x8013B5DC
void TMario::doRunning()
{
	f32 maxSpeed = mRunParams.mMaxSpeed.value;
	f32 intended = mIntendedMag;
	if (intended < maxSpeed)
		maxSpeed = intended;
	f32 runMult = maxSpeed;

	if (onYoshi()) {
		runMult = runMult * mYoshiParams.mRunYoshiMult.value;
	}

	f32 fwdVel = mForwardVel;
	if (fwdVel <= 0.0f) {
		// Accelerate from zero
		mForwardVel = fwdVel + mRunParams.mVelMinusBrake.value;
	} else if (fwdVel <= runMult) {
		// Accelerate towards target
		f32 addVelDiv = mRunParams.mAddVelDiv.value;
		f32 addBase = mRunParams.mAddBase.value;
		mForwardVel = fwdVel + (addBase - fwdVel * addVelDiv);
	} else {
		// Decelerate from above target
		const TBGCheckData* ground = mGroundPlane;
		if (ground->mNormal.y >= mRunParams.mDecStartNrmY.value) {
			f32 decBrake = mRunParams.mDecBrake.value;
			mForwardVel = fwdVel - decBrake;
			mForwardVel = mForwardVel - mYoshiParams.mDecBrake.value;
		}
	}

	if (mForwardVel < 0.0f)
		mForwardVel = 0.0f;

	// Compute angle change speed based on mPumpState
	s16 angleChange;
	u8 isPumpState;
	if (mPumpState == 0 || mPumpState == 1)
		isPumpState = 1;
	else
		isPumpState = 0;
	if (isPumpState) {
		s16 minRot = mDeParams.mPumpingRotSpMin.value;
		s16 maxRot = mDeParams.mPumpingRotSpMax.value;
		f32 fwdSpd = mForwardVel;
		f32 scale = 0.03125f;
		angleChange = (s16)(scale * (fwdSpd * (maxRot - minRot)) + minRot);
	} else {
		s16 minRot = mDeParams.mRunningRotSpMin.value;
		s16 maxRot = mDeParams.mRunningRotSpMax.value;
		f32 fwdSpd = mForwardVel;
		f32 scale = 0.03125f;
		angleChange = (s16)(scale * (fwdSpd * (maxRot - minRot)) + minRot);
	}

	if (onYoshi()) {
		angleChange = (s16)((f32)angleChange * mYoshiParams.mRotYoshiMult.value);
	}

	u8 isInWater;
	if (mState & 0x4000)
		isInWater = 1;
	else
		isInWater = 0;
	if (isInWater) {
		angleChange = mRunParams.mDashRotSp.value;
	}

	u8 isInShallow;
	if (mState & 0x30000)
		isInShallow = 1;
	else
		isInShallow = 0;
	if (isInShallow) {
		if (mFloorPosition.z < mPosition.y + mRunParams.mSwimDepth.value) {
			isInShallow = 1;
		} else {
			isInShallow = 0;
		}
	}
	if (isInShallow) {
		f32 depth = mFloorPosition.z - mPosition.y;
		f32 swimDepth = mRunParams.mSwimDepth.value;
		f32 brake = 1.0f - mRunParams.mInWaterBrake.value;
		f32 ratio = depth / swimDepth;
		mForwardVel = mForwardVel * (1.0f - ratio * brake);
	}

	s16 yawDiff = (s16)(mIntendedYaw - mFaceAngle.y);
	s32 converged = IConverge((s16)yawDiff, 0, (s16)angleChange, (s16)angleChange);
	mFaceAngle.y = mIntendedYaw - converged;

	slopeProcess();
}

// getSurfingParamsWater - 0x8013ACA0
TMario::TSurfingParams& TMario::getSurfingParamsWater()
{
	switch (unk389) {
	case 1:
		return mSurfingParamsWaterYellow;
	case 2:
		return mSurfingParamsWaterGreen;
	default:
		return mSurfingParamsWaterRed;
	}
}

// doSurfing - 0x8013B198
void TMario::doSurfing()
{
	f32 waterHeight = mPosition.y - mVel.y;
	const TBGCheckData* waterPlane;
	gpMap->checkGround(mPosition.x, waterHeight, mPosition.z, &waterPlane);

	u16 groundType = waterPlane->mBGType;

	u8 isSurfType;
	if (groundType == 0x100 || groundType == 0x101
	    || (u16)(groundType - 0x102) <= 3
	    || groundType == 0x4104)
		isSurfType = 1;
	else
		isSurfType = 0;

	f32 rotMin, rotMax, powMin, powMax;
	if (isSurfType) {
		u8 color = unk389;
		TSurfingParams& params = getSurfingParamsWater();
		rotMin = params.mRotMin.get();

		TSurfingParams& params2 = getSurfingParamsWater();
		rotMax = params2.mRotMax.get();

		TSurfingParams& params3 = getSurfingParamsWater();
		powMin = params3.mPowMin.get();

		TSurfingParams& params4 = getSurfingParamsWater();
		powMax = params4.mPowMax.get();
	} else {
		u8 color = unk389;
		TSurfingParams* params;
		switch (color) {
		case 1: params = (TSurfingParams*)((u8*)this + 0x1BC4); break;
		case 2: params = (TSurfingParams*)((u8*)this + 0x1F6C); break;
		default: params = (TSurfingParams*)((u8*)this + 0x181C); break;
		}
		rotMin = *(f32*)((u8*)params + 0x18);
		switch (color) {
		case 1: params = (TSurfingParams*)((u8*)this + 0x1BC4); break;
		case 2: params = (TSurfingParams*)((u8*)this + 0x1F6C); break;
		default: params = (TSurfingParams*)((u8*)this + 0x181C); break;
		}
		rotMax = *(f32*)((u8*)params + 0x2C);
		switch (color) {
		case 1: params = (TSurfingParams*)((u8*)this + 0x1BC4); break;
		case 2: params = (TSurfingParams*)((u8*)this + 0x1F6C); break;
		default: params = (TSurfingParams*)((u8*)this + 0x181C); break;
		}
		powMin = *(f32*)((u8*)params + 0x40);
		switch (color) {
		case 1: params = (TSurfingParams*)((u8*)this + 0x1BC4); break;
		case 2: params = (TSurfingParams*)((u8*)this + 0x1F6C); break;
		default: params = (TSurfingParams*)((u8*)this + 0x181C); break;
		}
		powMax = *(f32*)((u8*)params + 0x54);
	}

	// Clamp intended magnitude to speed range
	f32 clampedMag = 0.03125f * mIntendedMag;
	f32 speedInput = clampedMag;
	if (speedInput > powMax)
		speedInput = powMax;
	if (speedInput < powMin)
		speedInput = powMin;

	// Accelerate or decelerate forward velocity
	f32 fwdVel = mForwardVel;
	if (fwdVel <= 0.0f) {
		// Accelerate forward
		mForwardVel = fwdVel + 1.1f;
	} else if (fwdVel <= speedInput) {
		// Under target speed - check ground and accelerate
		u16 gt2 = mGroundPlane->mBGType;
		u8 isSurf2;
		if (gt2 == 0x100 || gt2 == 0x101
		    || (u16)(gt2 - 0x102) <= 3
		    || gt2 == 0x4104)
			isSurf2 = 1;
		else
			isSurf2 = 0;

		f32 accel;
		if (isSurf2) {
			TSurfingParams& p = getSurfingParamsWater();
			accel = p.mAccel.get();
		} else {
			u8 color = unk389;
			TSurfingParams* params;
			switch (color) {
			case 1: params = (TSurfingParams*)((u8*)this + 0x1BC4); break;
			case 2: params = (TSurfingParams*)((u8*)this + 0x1F6C); break;
			default: params = (TSurfingParams*)((u8*)this + 0x181C); break;
			}
			accel = *(f32*)((u8*)params + 0x68);
		}

		f32 accelDelta = 1.1f - fwdVel / accel;
		f32* forwardVel = &mForwardVel;
		*forwardVel = *forwardVel + accelDelta;
	} else {
		// Over target speed - check ground slope
		if (mGroundPlane->mNormal.y >= 0.95f) {
			mForwardVel = fwdVel - 0.3f;
		}
	}

	if (mForwardVel > powMax)
		mForwardVel = powMax;

	// Interpolate rotation speed
	f32 rotRange = speedInput - powMin;
	f32 maxRange = powMax - powMin;
	f32 powRange = rotMax - rotMin;
	f32 rotSpeed = rotMin + (rotRange / maxRange) * powRange;

	s16 yawDiff = (s16)(mIntendedYaw - mFaceAngle.y);
	s16 rotSpeedS16 = (s16)rotSpeed;
	s32 converged = IConverge((s16)yawDiff, 0, rotSpeedS16, rotSpeedS16);
	mFaceAngle.y = mIntendedYaw - converged;

	slopeProcess();

	// Check if on surfing ground for special handling
	u16 gt3 = waterPlane->mBGType;
	u8 isSurf3;
	if (gt3 == 0x100 || gt3 == 0x101 || (u16)(gt3 - 0x102) <= 3
	    || gt3 == 0x4104)
		isSurf3 = 1;
	else
		isSurf3 = 0;
	if (isSurf3) {
		surfingEffect();
	}
}

void TMario::doPushingAnimation(const Vec& target)
{
	f32 dx = mPosition.x - target.x;
	f32 dz = mPosition.z - target.z;

	if (mForwardVel > 6.0f) {
		setPlayerVelocity(6.0f);
	}

	s16 wallAngle;
	s16 angleDiff;
	if (mWallPlane != nullptr) {
		wallAngle = getWallAngle();
		angleDiff = wallAngle - mFaceAngle.y;
	}

	if (mWallPlane == nullptr)
		goto fail;

	s16 extDiff = (s16)angleDiff;
	if (extDiff < -29127)
		goto fail;
	if (extDiff <= 29127)
		goto ok;

fail:
	setAnimation(0x6c, 1.0f);
	startVoice(0x7094);
	return;

ok:
	f32 distSq = dz * dz;
	distSq = dx * dx + distSq;
	if (distSq > 0.0f) {
		f64 root = __frsqrte(distSq);
		volatile f32 result
		    = 0.5f * root * (3.0f - distSq * (root * root)) * distSq;
		distSq = result;
	}

	f32 speed = 2.0f * distSq;

	if ((s16)angleDiff < 0) {
		setAnimation(0x80, speed);
	} else {
		setAnimation(0x7f, speed);
	}

	mFaceAngle.x = 0;
	mModelFaceAngle = (s16)(wallAngle + 0x8000);
}

// running - 0x80139FA8
BOOL TMario::running()
{
	mActionTimer++;

	// Check held object throw
	TTakeActor* held = mHeldObject;
	BOOL throwResult;
	if (held != nullptr) {
		u8 isJumpHeld;
		if (mInput & 0x2000)
			isJumpHeld = 1;
		else
			isJumpHeld = 0;
		if (isJumpHeld) {
			s32 heldType = held->mActorType;
			u8 isBitSet;
			if (heldType & 0x10000000)
				isBitSet = 1;
			else
				isBitSet = 0;
			if (isBitSet) {
				throwResult = changePlayerStatus(0x80000588, 0, false);
				goto throwDone;
			} else {
				s32 statusBase = 0x80000000;
				switch (heldType) {
				case 0x80000001:
					throwResult = changePlayerStatus(statusBase + 0x588, 0, false);
					goto throwDone;
				default:
					if (mForwardVel > 16.0f) {
						throwResult = changePlayerStatus(statusBase + 0x588, 0, false);
						goto throwDone;
					} else if (canPut()) {
						throwResult = changePlayerStatus(statusBase + 0x387, 0, false);
						goto throwDone;
					} else {
						goto throwFalse;
					}
				}
			}
		} else {
			goto throwFalse;
		}
	} else {
		goto throwFalse;
	}

throwFalse:
	throwResult = 0;
throwDone:
	if (throwResult != 0)
		return true;

	// Check jump input
	BOOL doJump;
	if (mInput & 0x08) {
		if (mForwardVel <= 0.1f)
			goto jumpStart;
		if (isFrontSlip(0) == 0)
			goto jumpFail;
	} else {
		goto jumpFail;
	}
jumpStart:
	doJump = 1;
	goto jumpCheck;
jumpFail:
	doJump = 0;
jumpCheck:
	if (doJump != 0) {
		changePlayerStatus(0x50, 0, false);
		return;
	}

	// Check crouch (0x10)
	if (mInput & 0x10) {
		if (mActionState == 1) {
			mFaceAngle.y = (s16)mActionArg;
			changePlayerStatus(0x0C400209, 0, false);
			return;
		}
		if (mActionTimer > 0xF0 && mForwardVel >= 16.0f
		    && mGroundPlane->mNormal.y >= 0.17364818f) {
			changePlayerStatus(0x04000445, 0, false);
		} else {
			changePlayerStatus(0x0400044A, 0, false);
		}
		return;
	}

	// Check dash jump while in water-jet state (0x4000)
	u8 inJetState;
	if (mState & 0x4000)
		inJetState = 1;
	else
		inJetState = 0;
	if (inJetState && (mInput & 0x2)
	    && mForwardVel > mDeParams.mDashMax.value - 1.0f) {
		changePlayerJumping(0x0888, 0);
		return;
	}

	// Check B button (0x2) for tri jump
	if (mInput & 0x2) {
		changePlayerTriJump();
		return;
	}

	// Check spray jump (0x8000) unless on Yoshi
	if (!onYoshi() && (mInput & 0x8000)) {
		BOOL catchJump;
		if (mInput & 0x8000) {
			if (mForwardVel > *(f32*)((u8*)this + 0x1020)
			    && mIntendedMag > 0.75f) {
				mVel.y = 20.0f;
				catchJump = changePlayerStatus(0x0080088A, 1, false);
				goto catchJumpDone;
			}
		}
		catchJump = 0;
	catchJumpDone:
		if (catchJump != 0)
			return true;
		changePlayerStatus(0x384, 0, false);
	}

	// Check crouch (0x20)
	if (mInput & 0x20) {
		if (mActionState == 1) {
			mFaceAngle.y = (s16)mActionArg;
			changePlayerStatus(0x0C400209, 0, false);
			return;
		}
		if (mActionTimer > 0xF0 && mForwardVel >= 16.0f
		    && mGroundPlane->mNormal.y >= 0.17364818f) {
			changePlayerStatus(0x04000445, 0, false);
		} else {
			changePlayerStatus(0x0400044A, 0, false);
		}
		return;
	}

	// Check turn
	u8 isPumping;
	if (mPumpState == 0 || mPumpState == 1)
		isPumping = 1;
	else
		isPumping = 0;
	BOOL shouldTurn;
	if (isPumping) {
		shouldTurn = 0;
	} else if (onYoshi()) {
		shouldTurn = 0;
	} else {
		s16 yawDiff = (s16)(mIntendedYaw - mFaceAngle.y);
		shouldTurn = 1;
		if (yawDiff >= -18204 && yawDiff <= 18204)
			shouldTurn = 0;
	}

	u32 shouldTurnCheck = (u8)shouldTurn;
	if (shouldTurnCheck != 0) {
		if (mForwardVel >= *(f32*)((u8*)this + 0x1034)) {
			emitParticle(0x15, (s16)(mFaceAngle.y + 0x8000));
			emitParticle(0x17, (s16)(mFaceAngle.y + 0x8000));
			emitParticle(0x16, (s16)(mFaceAngle.y + 0x8000));
			changePlayerStatus(0x0443, 0, false);
			return;
		}
	}

	// Check squat slip start
	if ((u8)canSquat()) {
		setPlayerVelocity(0.0f);
		changePlayerStatus(0x0C008220, 0, false);
		return;
	}

	// Check rocket start
	if (rocketCheck()) {
		mRocketTargetY = mFloorPosition.y + *(f32*)((u8*)mWaterGun + 0x1D40);
		changePlayerStatus(0x088B, 0, false);
		return;
	}

	// Main running logic
	u8 isWallBit = 0;
	mActionState = isWallBit;
	Vec prevPos = *(Vec*)&mPosition;

	doRunning();
	setNormalAttackArea();

	int walkResult = walkProcess();
	switch (walkResult) {
	case 0:
		changePlayerStatus(0x088C, 0, false);
		setAnimation(0x56, 1.0f);
		break;
	case 1:
		doRunningAnimation();
		break;
	case 2: {
		if (onYoshi()) {
			setPlayerVelocity(0.0f);
			break;
		}

		if (mState & 0x2)
			isWallBit = 1;
		if (isWallBit)
			break;

		if (mForwardVel > mDeParams.mClashSpeed.value) {
			emitParticle(12);
			changePlayerDropping(0x000208B0, 0);
			return;
		}

		if (mInput & 0x2) {
			f32 wallY = mPosition.y + *(f32*)((u8*)this + 0x80C);
			if (gpMap->isTouchedOneWall(mPosition.x, wallY, mPosition.z, 50.0f) == 1) {
				mVel.y = 52.0f;
				mFaceAngle.y = (s16)(mFaceAngle.y + 0x8000);
				setPlayerVelocity(50.0f);
				changePlayerStatus(0x02000886, 0, false);
				return;
			}
		}

		const TBGCheckData* wall = mWallPlane;
		u8 isWallType;
		if (wall != nullptr && wall->mBGType == 0x10A)
			isWallType = 1;
		else
			isWallType = 0;
		if (isWallType) {
			const Vec* normal = (const Vec*)&wall->mNormal;
			s32 wallAngle = matan(normal->z, normal->x);
			mFaceAngle.y = (s16)(wallAngle + 0x8000);
			mModelFaceAngle = mFaceAngle.y;
			changePlayerStatus(0x3000036B, 0, false);
			return;
		}

		doPushingAnimation(prevPos);
		mDashTimer = 0;
		mState = mState & ~0x4000;
		break;
	}
	default:
		break;
	}

	checkDescent();

	u8 inJetState2;
	if (mState & 0x4000)
		inJetState2 = 1;
	else
		inJetState2 = 0;
	if (inJetState2) {
		setPlayerVelocity(mDeParams.mDashMax.value);
		startSoundActor(0x19);
	}
	return false;
}

// rotating - 0x80139E80
BOOL TMario::rotating()
{
	if (mInput & 0x2) {
		if (mAction == 0x0441) {
			changePlayerStatus(0x0896, 0, false);
			return;
		} else {
			changePlayerStatus(0x0895, 0, false);
			return;
		}
	}

	setAnimation(244, 1.0f);
	emitRotateShootEffect();
	emitBlurSpinJump();

	mActionTimer++;
	if (mActionTimer > 120) {
		changePlayerStatus(0x0C400201, 0, false);
		return;
	}

	doRunning();
	switch (walkProcess()) {
	case 0:
		changePlayerStatus(0x088C, 0, false);
		break;
	}

	if (mAction == 0x0441) {
		mModelFaceAngle = (s16)(mActionTimer << 12);
	} else {
		mModelFaceAngle = (u16)(-(mActionTimer << 12));
	}
	return false;
}

BOOL TMario::fireDashing()
{
	if (mInput & 0x02) {
		changePlayerStatus(0x000208b4, 0, false);
		return;
	}

	u16 timer = mActionTimer;
	mActionTimer = timer + 1;
	if (timer > 160) {
		changePlayerStatus(0x04000440, 0, false);
		return;
	}

	u8 isWallHit;
	if (mState & 0x00030000)
		isWallHit = 1;
	else
		isWallHit = 0;
	if (isWallHit) {
		changePlayerStatus(0x04000440, 0, false);
		return;
	}

	if (mForwardVel < 0.0f)
		mForwardVel = 0.0f;

	if (mForwardVel > 8.0f)
		mForwardVel = 8.0f;

	mForwardVel = FConverge(mForwardVel, 48.0f, 32.0f, 4.0f);

	if (mInput & 0x01) {
		s32 yawDiff = mIntendedYaw - mFaceAngle.y;
		s32 result = IConverge((s16)yawDiff, 0, 1536, 1536);
		mFaceAngle.y = mIntendedYaw - result;
	}

	slopeProcess();
	int walkResult = walkProcess();
	if (walkResult == 0) {
		changePlayerStatus(0x000208b5, 0, false);
	}

	f32 anmSpeed = 0.5f * mForwardVel * 0.1f;
	setAnimation(0x29, anmSpeed);
	return false;
}

BOOL TMario::walkEnd()
{
	u32 input = mInput;
	if (!(input & 0x10)) {
		BOOL doJump;
		if (input & 0x08) {
			if (mForwardVel <= 0.1f)
				goto jumpStart;
			if (isFrontSlip(0) == 0)
				goto jumpFail;
		} else {
			goto jumpFail;
		}
	jumpStart:
		doJump = true;
		goto jumpCheck;
	jumpFail:
		doJump = false;
	jumpCheck:

		if (doJump) {
			changePlayerStatus(0x50, 0, false);
			return;
		}

		input = mInput;
		if (input & 0x02) {
			changePlayerTriJump();
			return;
		}

		if (input & 0x01) {
			changePlayerStatus(0x04000440, 0, false);
			return;
		}

		if (input & 0x8000) {
			changePlayerStatus(0x384, 0, false);
			return;
		}
	}

	f32 fwdVel;
	f32 anmMult;
	int stickDir;
	BOOL rotated;
	if (checkStickRotate(&stickDir) == 1 && mWaterGun != nullptr) {
		if (mWaterGun->isEmitting()) {
			if (stickDir > 0) {
				rotated = changePlayerStatus(0x441, 0, false);
				goto rotateDone;
			} else {
				rotated = changePlayerStatus(0x442, 0, false);
				goto rotateDone;
			}
		} else {
			goto rotateFalse;
		}
	} else {
		goto rotateFalse;
	}

rotateFalse:
	rotated = 0;
rotateDone:
	if (rotated != 0) {
		return true;
	}

	s32 stopped = 0;
	f32 newVel = FConverge(mForwardVel, 0.0f, 1.0f, 1.0f);
	mForwardVel = newVel;
	if (0.0f == newVel)
		stopped = 1;

	setPlayerVelocity(mForwardVel);

	if (stopped) {
		changePlayerStatus(0x0C400201, 0, false);
		return;
	}

	int walkResult = walkProcess();
	switch (walkResult) {
	case 0:
		changePlayerStatus(0x088c, 0, false);
		break;
	case 2:
		setPlayerVelocity(0.0f);
		break;
	default:
		break;
	}

	fwdVel = mForwardVel;
	anmMult = 0.25f;
	f32 anmSpeed = fwdVel * anmMult;
	if (anmSpeed < 0.1f)
		anmSpeed = 0.1f;
	setAnimation(0x48, anmSpeed);
	return false;
}

BOOL TMario::surfing()
{
	setAnimation(0x6d, 1.0f);

	if (mActionTimer > 0) {
		mActionTimer = mActionTimer - 1;
		return false;
	}

	if (mInput & 0x02) {
		f32 posY = mPosition.y;
		f32 up = 1.0f;
		mPosition.y = posY + up;
		changePlayerStatus(0x0281089a, 0, false);
		return;
	}

	doSurfing();
	walkProcess();
	int walkResult = walkProcess();

	switch (walkResult) {
	case 1:
		break;
	case 0:
		changePlayerStatus(0x0081089b, 0, false);
		return;
	case 2: {
		const TBGCheckData* wall = mWallPlane;
		if (wall == nullptr) {
			setPlayerVelocity(0.0f);
			loserExec();
			return;
		}

		const JGeometry::TVec3<f32>* normal = &wall->mNormal;
		f32 nz = normal->z;
		f32 nx = normal->x;
		s16 faceDiff = matan(nz, nx) - mFaceAngle.y;

		const TBGCheckData* ground = mGroundPlane;
		u16 groundType = ground->mBGType;

		u8 isSurfType;
		if (groundType == 0x100 || groundType == 0x101
		    || (u16)(groundType - 0x102) <= 3
		    || groundType == 0x4104)
			isSurfType = 1;
		else
			isSurfType = 0;

		s16 clashAngle;
		f32 clashSpeed;
		if (isSurfType) {
			u8 color = *(u8*)((u8*)this + 0x389);
			TSurfingParams* params;
			switch (color) {
			case 1:
				params = (TSurfingParams*)((u8*)this + 0x19F0);
				break;
			case 2:
				params = (TSurfingParams*)((u8*)this + 0x1D98);
				break;
			default:
				params = (TSurfingParams*)((u8*)this + 0x1648);
				break;
			}
			clashAngle = *(s16*)((u8*)params + 0x1D0);

			switch (color) {
			case 1:
				params = (TSurfingParams*)((u8*)this + 0x19F0);
				break;
			case 2:
				params = (TSurfingParams*)((u8*)this + 0x1D98);
				break;
			default:
				params = (TSurfingParams*)((u8*)this + 0x1648);
				break;
			}
			clashSpeed = *(f32*)((u8*)params + 0x1BC);
		} else {
			u8 color = *(u8*)((u8*)this + 0x389);
			TSurfingParams* params;
			switch (color) {
			case 1:
				params = (TSurfingParams*)((u8*)this + 0x1BC4);
				break;
			case 2:
				params = (TSurfingParams*)((u8*)this + 0x1F6C);
				break;
			default:
				params = (TSurfingParams*)((u8*)this + 0x181C);
				break;
			}
			clashAngle = *(s16*)((u8*)params + 0x1D0);

			switch (color) {
			case 1:
				params = (TSurfingParams*)((u8*)this + 0x1BC4);
				break;
			case 2:
				params = (TSurfingParams*)((u8*)this + 0x1F6C);
				break;
			default:
				params = (TSurfingParams*)((u8*)this + 0x181C);
				break;
			}
			clashSpeed = *(f32*)((u8*)params + 0x1BC);
		}

		s16 clash = clashAngle;
		s32 negClash = -clash;
		if ((s16)faceDiff < negClash || clash < (s16)faceDiff) {
			if (mForwardVel > clashSpeed) {
				s32 hpMax = mDeParams.mHpMax.value;
				decHP(hpMax);
				changePlayerStatus(0x000208b3, 0, true);
				mForwardVel = -mForwardVel * 0.03125f;
				mVel.y = 0.0f;
				return;
			}
		}

		setPlayerVelocity(0.0f);
		break;
	}
	default:
		break;
	}
	return false;
}

// turnning - 0x8013A39C
BOOL TMario::turnning()
{
	// Check held object throw
	TTakeActor* held = mHeldObject;
	BOOL throwResult;
	if (held != nullptr) {
		u8 isJumpHeld;
		if (mInput & 0x2000)
			isJumpHeld = 1;
		else
			isJumpHeld = 0;
		if (isJumpHeld) {
			s32 heldType = held->mActorType;
			u8 isBitSet;
			if (heldType & 0x10000000)
				isBitSet = 1;
			else
				isBitSet = 0;
			if (isBitSet) {
				throwResult = changePlayerStatus(0x80000588, 0, false);
				goto throwDone;
			} else {
				s32 statusBase = 0x80000000;
				switch (heldType) {
				case 0x80000001:
					throwResult = changePlayerStatus(statusBase + 0x588, 0, false);
					goto throwDone;
				default:
					if (mForwardVel > 16.0f) {
						throwResult = changePlayerStatus(statusBase + 0x588, 0, false);
						goto throwDone;
					} else if (canPut()) {
						throwResult = changePlayerStatus(statusBase + 0x387, 0, false);
						goto throwDone;
					} else {
						goto throwFalse;
					}
				}
			}
		} else {
			goto throwFalse;
		}
	} else {
		goto throwFalse;
	}

throwFalse:
	throwResult = 0;
throwDone:
	if (throwResult != 0)
		return true;

	// Check jump
	if (mInput & 0x08) {
		changePlayerStatus(0x50, 0, false);
		return;
	}

	// Check B press
	if (mInput & 0x02) {
		changePlayerJumping(0x0887, 0);
		return;
	}

	// Check stick rotate
	int stickDir;
	BOOL rotated;
	if (checkStickRotate(&stickDir) == 1 && mWaterGun != nullptr) {
		if (mWaterGun->isEmitting()) {
			if (stickDir > 0) {
				rotated = changePlayerStatus(0x441, 0, false);
				goto rotateDone;
			} else {
				rotated = changePlayerStatus(0x442, 0, false);
				goto rotateDone;
			}
		} else {
			goto rotateFalse;
		}
	} else {
		goto rotateFalse;
	}

rotateFalse:
	rotated = 0;
rotateDone:
	if (rotated != 0)
		return true;

	// Check crouch
	if (mInput & 0x20) {
		changePlayerStatus(0x04000445, 0, false);
		return;
	}

	// Check turn state
	BOOL shouldTurn;
	u32 pumpState = mPumpState;
	u8 isPump;
	if (pumpState == 0 || pumpState == 1)
		isPump = 1;
	else
		isPump = 0;
	if (isPump) {
		shouldTurn = 0;
	} else if (onYoshi()) {
		shouldTurn = 0;
	} else {
		s16 yawDiff = (s16)(mIntendedYaw - mFaceAngle.y);
		shouldTurn = 1;
		if (yawDiff >= -18204 && yawDiff <= 18204)
			shouldTurn = 0;
	}

	u32 shouldTurnCheck = (u8)shouldTurn;
	if (!shouldTurnCheck) {
		changePlayerStatus(0x04000440, 0, false);
		return;
	}

	// Decelerate
	s32 stopped = 0;
	f32 newVel = FConverge(mForwardVel, 0.0f, 4.0f, 4.0f);
	mForwardVel = newVel;
	if (0.0f == newVel)
		stopped = 1;

	slopeProcess();

	if (stopped) {
		mFaceAngle.y = mIntendedYaw;
		setPlayerVelocity(8.0f);
		changePlayerStatus(0x0444, 0, false);
		return;
	}

	switch (walkProcess()) {
	case 0:
		changePlayerStatus(0x088C, 0, false);
		break;
	}

	if (mForwardVel >= 18.0f) {
		setAnimation(0xBC, 1.0f);
	} else {
		setAnimation(0xBD, 1.0f);

		if (isLast1AnimeFrame()) {
			f32 fwdVel = mForwardVel;
			if (fwdVel > 0.0f) {
				mFaceAngle.y = mIntendedYaw;
				setPlayerVelocity(-fwdVel);
				changePlayerStatus(0x04000440, 0, false);
			} else {
				mFaceAngle.y = mIntendedYaw;
				setPlayerVelocity(8.0f);
				changePlayerStatus(0x04000440, 0, false);
			}
		}
	}
	return false;
}

BOOL TMario::turnEnd()
{
	// Part 1: check held object for throw/put
	BOOL result;
	TTakeActor* held = mHeldObject;
	if (held != nullptr) {
		u8 isJumpHeld;
		if (mInput & 0x2000)
			isJumpHeld = 1;
		else
			isJumpHeld = 0;

		if (isJumpHeld) {
			s32 heldType = held->mActorType;

			u8 isBitSet;
			if (heldType & 0x10000000)
				isBitSet = 1;
			else
				isBitSet = 0;
			if (isBitSet) {
				result = changePlayerStatus(0x80000588, 0, false);
				goto throwDone;
			} else {
				switch (heldType) {
				case 0x80000001:
					result = changePlayerStatus(0x80000588, 0, false);
					goto throwDone;
				default:
					if (mForwardVel > 16.0f) {
						result = changePlayerStatus(0x80000588, 0, false);
						goto throwDone;
					} else if (canPut()) {
						result = changePlayerStatus(0x80000387, 0, false);
						goto throwDone;
					} else {
						goto throwFalse;
					}
				}
			}
		} else {
			goto throwFalse;
		}
	} else {
		goto throwFalse;
	}

throwFalse:
	result = 0;
throwDone:
	if (result != 0) {
		return true;
	}

	// Part 2: check input flags
	u32 input = mInput;
	if (input & 0x08) {
		changePlayerStatus(0x50, 0, false);
		return;
	}

	if (input & 0x02) {
		changePlayerJumping(0x887, 0);
		return;
	}

	// Part 3: check stick rotate
	int stickDir;
	BOOL rotated;
	if (checkStickRotate(&stickDir) == 1 && mWaterGun != nullptr) {
		if (mWaterGun->isEmitting()) {
			if (stickDir > 0) {
				rotated = changePlayerStatus(0x441, 0, false);
				goto rotateDone;
			} else {
				rotated = changePlayerStatus(0x442, 0, false);
				goto rotateDone;
			}
		} else {
			goto rotateFalse;
		}
	} else {
		goto rotateFalse;
	}

rotateFalse:
	rotated = 0;
rotateDone:
	if (rotated != 0) {
		return true;
	}

	// Part 4: do running and animation
	doRunning();
	setAnimation(0xbd, 1.0f);

	if (walkProcess() == 0) {
		changePlayerStatus(0x088c, 0, false);
	}

	if (isLast1AnimeFrame()) {
		changePlayerStatus(0x04000440, 0, false);
	}

	mFaceAngle.x = 0;
	s16 modelAngle = mModelFaceAngle;
	mModelFaceAngle = (s16)(modelAngle + 0x8000);
	return false;
}

// slippingBasic - 0x80138DF8
void TMario::slippingBasic(int statusOnStop, int slipStatus, int slipArg)
{
	isForceSlip();

	// Check for B button to enter fire slide jump
	if ((mInput & 0x2) && canSlipJump() == 1) {
		changePlayerStatus(0x02000880, 0, false);
		return;
	}

	if (mInput & 0x8000) {
		// L trigger water landing
		mVel.y = 20.0f;
		changePlayerStatus(0x0080088A, 1, false);
		return;
	} else {
		int walkResult = walkProcess();
		switch (walkResult) {
		case 0:
			changePlayerStatus(slipStatus, 0, false);
			return;
		case 1:
			setAnimation(slipArg, 1.0f);
			mSubState |= 0x8;
			frontSlipEffect();
			return;
		case 2: {
			if (onYoshi()) {
				setPlayerVelocity(0.0f);
				return;
			}

			// Check speed for emit particle
			f32 fwdVel = mForwardVel;
			if (fwdVel < 0.0f)
				fwdVel = -fwdVel;
			if (fwdVel > mDeParams.mClashSpeed.value) {
				emitParticle(12);
			}

			if (isSlipStart()) {
				const TBGCheckData* wall = mWallPlane;
				if (wall != nullptr) {
					const JGeometry::TVec3<f32>* normal = &wall->mNormal;
					s32 wallAngle = matan(normal->z, normal->x);

					f32 velSq = mSlideVelX * mSlideVelX
					            + mSlideVelZ * mSlideVelZ;
					if (velSq > 0.0f) {
						f64 root = __frsqrte(velSq);
						f64 refined = 0.5f * root * (3.0f - velSq * (root * root));
						volatile f32 result = velSq * refined;
						velSq = result;
					}

					f32 speedScaled = (f32)(velSq * 0.9);
					if (speedScaled < 4.0f)
						speedScaled = 4.0f;

					s16 wallAngleShort = wallAngle;
					s16 slideDirAngle = mSlideAngle;
					s16 wallDiff = (s16)(slideDirAngle - wallAngleShort);
					s16 newAngle = (s16)(wallAngleShort - wallDiff + 0x8000);
					mSlideAngle = newAngle;

					u16 uAngle = mSlideAngle;
					f32 slideVelX = speedScaled * JMASSin(uAngle);
					mSlideVelX = slideVelX;
					mVel.x = slideVelX;

					uAngle = mSlideAngle;
					f32 slideVelZ = speedScaled * JMASCos(uAngle);
					mSlideVelZ = slideVelZ;
					mVel.z = slideVelZ;

					// Play wall slip sound
					u8 groundAttr = ((u8*)mWallPlane)[6];
					u32 soundId = gpMSound->getWallSound(groundAttr, mForwardVel);
					MSound* sound = gpMSound;
					if (sound->gateCheck(soundId)) {
						MSoundSESystem::MSoundSE::startSoundActor(soundId, (const Vec*)&mPosition,
						                                           0, nullptr, 0, 4);
					}
				}
			} else {
				if (mForwardVel > 16.0f) {
					playerRefrection(1);
					changePlayerDropping(0x00020466, 0);
				} else {
					setPlayerVelocity(0.0f);
					changePlayerStatus(statusOnStop, 0, false);
				}
			}

			mSubState |= 0x8;
			return;
		}
		default:
			return;
		}
	}
}

#pragma dont_inline on

// slipForeCommon - 0x80138D30
BOOL TMario::slipForeCommon(int statusOnStop, int jumpStatus, int slipStatus, int slipArg)
{
	if (mActionTimer > 20 && canSlipJump()) {
		if (mInput & 0x2) {
			changePlayerJumping(jumpStatus, 0);
			return;
		}
	} else {
		mActionTimer++;
	}

	f32 slideStop = getSlideStopNormal();
	if (doSliding(slideStop)) {
		return changePlayerStatus(statusOnStop, 0, false);
	} else {
		slippingBasic(statusOnStop, slipStatus, slipArg);
	}
	return false;
}

// slipBackCommon - 0x80138C50
BOOL TMario::slipBackCommon(int statusOnStop, int slipStatus, int slipArg)
{
	u16 timer = mActionTimer;
	if (timer > 20) {
		u32 input = mInput;
		if (!(input & 0x8)) {
			if (input & 0x2) {
				if (canSlipJump()) {
					changePlayerDropping(0x08A6, 0);
					return;
				}
			}
		}
	} else {
		mActionTimer = timer + 1;
	}

	f32 slideStop = getSlideStopNormal();
	if (doSliding(slideStop)) {
		return changePlayerStatus(statusOnStop, 0, false);
	} else {
		slippingBasic(statusOnStop, slipStatus, slipArg);
	}
	return false;
}

#pragma dont_inline off

// catching - 0x80138AFC
BOOL TMario::catching()
{
	u32 input = mInput;
	if (!(input & 0x8)) {
		if (input & 0x2) {
			if (mForwardVel > mJumpParams.mRotBroadEnableV.get()) {
				changePlayerStatus(0x02000889, 0, false);
				return;
			} else {
				changePlayerStatus(0x08A6, 0, false);
				return;
			}
		}
	}

	u8 onWater;
	if (mState & 0x10) {
		onWater = 1;
	} else {
		onWater = 0;
	}
	if (onWater) {
		mActionState = 1;
	}

	f32 slideStop = getSlideStopCatch();
	if (doSliding(slideStop)) {
		setPlayerVelocity(0.0f);
		changePlayerStatus(902, 0, false);
		return;
	}

	slippingBasic(902, 0x088C, 136);

	if (gpMSound->gateCheck(0x1009)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x1009, (const Vec*)&mPosition, 0,
		                                           nullptr, 0, 4);
	}

	J3DFrameCtrl& frameCtrl = getMotionFrameCtrl();
	if (frameCtrl.getFrame() > 45.0f) {
		getMotionFrameCtrl().setFrame(45.0f);
	}
	return false;
}

// oilRun - 0x80138530
BOOL TMario::oilRun()
{
	u32 input = mInput;

	// Check B button for jump
	if (input & 0x2) {
		setPlayerVelocity(0.0f);
		changePlayerJumping(0x02000880, 0);
		return;
	}

	// Check L trigger for water landing
	if (input & 0x8000) {
		mVel.y = 20.0f;
		changePlayerStatus(0x0080088A, 1, false);
		return;
	}

	// Check if velocity is very small
	f32 negThresh = -1.0f;
	f32 velX;
	if (negThresh < (velX = mVel.x) && velX < 1.0f) {
		f32 velZ;
		if (negThresh < (velZ = mVel.z) && velZ < 1.0f) {
			setPlayerVelocity(0.0f);
			changePlayerStatus(0x0C400201, 0, false);
			return;
		}
	}

	// Stamp pollution
	f32 polSize = mDirtyParams.mPolSizeRun.value;
	f32 posZ = mPosition.z;
	f32 posY = mPosition.y;
	f32 posX = mPosition.x;
	TPollutionManager* pollution = gpPollution;
	pollution->stamp(1, posX, posY, posZ, polSize);

	// Compute angle rotation
	s16 rotSpeed = mDirtyParams.mSlipRotate.value;
	s16 yawDiff = mIntendedYaw - mFaceAngle.y;
	s16 targetRot = (s16)((f32)rotSpeed);
	s32 converged = IConverge((s16)yawDiff, 0, (s16)targetRot, (s16)targetRot);
	mFaceAngle.y = mIntendedYaw - converged;

	// Apply stick input to velocity
	u16 stickAngle = mFaceAngle.y;
	f32 intendedMag = mIntendedMag;
	f32 slipRunSp = mDirtyParams.mSlipRunSp.value;
	f32 sinVal = JMASSin(stickAngle);
	mVel.x = mVel.x + intendedMag * sinVal * slipRunSp;

	u16 stickAngle2 = mFaceAngle.y;
	f32 intendedMag2 = mIntendedMag;
	f32 slipRunSp2 = mDirtyParams.mSlipRunSp.value;
	f32 cosVal = JMASCos(stickAngle2);
	mVel.z = mVel.z + intendedMag2 * cosVal * slipRunSp2;

	// Decrement slip timer
	s32 slipTimer = unk13C;
	unk13C = slipTimer - 1;
	if (unk13C <= 0) {
		unk13C = 0;
		unk138 = 0.0f;
	}

	// Apply friction
	mVel.x = mVel.x * unk138;
	mVel.z = mVel.z * unk138;

	// Reset forward velocity and slide components
	mForwardVel = 0.0f;
	mSlideVelX = 0.0f;
	mSlideVelZ = 0.0f;

	// Check if stationary
	f32 mag = mIntendedMag;
	if (0.0f == mag) {
		if (mInput & 0x4000) {
			setAnimation(0x98, 1.0f);
		} else {
			setAnimation(0xC3, 1.0f);
		}
	} else {
		f32 anmSpeed = 0.5f * mag * mDirtyParams.mSlipAnmSpeed.value;
		setAnimation(0x72, anmSpeed);
		startVoiceIfNoVoice(30931);

		if (gpMSound->gateCheck(0x1001)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x1001, (Vec*)&mPosition, 0, nullptr, 0, 4);
		}
	}

	switch (walkProcess()) {
	case 0:
		changePlayerStatus(0x088C, 0, false);
		return;
	case 1:
	case 2:
		break;
	}
	return false;
}

BOOL TMario::oilSlip()
{
	if (mInput & 0x02) {
		setPlayerVelocity(0.0f);
		changePlayerJumping(0x02000880, 0);
		return;
	}

	// Convert s16 rotation speed param to float
	s16 rotSpeed = mDirtyParams.mSlipCatchRotate.value;
	s16 yawDiff = mIntendedYaw - mFaceAngle.y;
	s16 targetRot = (s16)((f32)rotSpeed);
	s32 converged = IConverge((s16)yawDiff, 0, (s16)targetRot, (s16)targetRot);
	mFaceAngle.y = mIntendedYaw - converged;

	// Decrement slip timer
	s32 slipTimer = unk13C;
	unk13C = slipTimer - 1;

	if (unk13C <= 0) {
		unk13C = 0;
		unk138 = 0.0f;
		changePlayerStatus(0x00800456, 0, false);
	}

	// Pollution stamp
	f32 polSize = mDirtyParams.mPolSizeSlip.value;
	f32 posZ = mPosition.z;
	f32 posY = mPosition.y;
	f32 posX = mPosition.x;
	TPollutionManager* pollution = gpPollution;
	pollution->stamp(1, posX, posY, posZ, polSize);

	// Sound effect
	if (gpMSound->gateCheck(0x1141)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x1141, (Vec*)&mPosition, 0, nullptr, 0, 4);
	}

	// Compute velocity from stick input and cos table
	// angle = (u16)(faceAngle.y - intendedYaw), then cos lookup
	s16 stickAngle = mFaceAngle.y - mIntendedYaw;
	f32 cosVal = JMASCos(stickAngle);
	f32 stickMult = mDirtyParams.mSlipCatchSp.value;
	mForwardVel = mForwardVel + mIntendedMag * cosVal * stickMult;

	// Apply friction
	mForwardVel = mForwardVel * unk138;

	setPlayerVelocity(mForwardVel);

	// Check if speed is near zero (between -1 and 1)
	if (-1.0f < mForwardVel && mForwardVel < 1.0f) {
		setPlayerVelocity(0.0f);
		changePlayerStatus(0x386, 0, false);
		return;
	}

	switch (walkProcess()) {
	case 0:
		changePlayerStatus(0x088c, 0, false);
		return;
	case 1:
	case 2:
	default:
		break;
	}

	// Clamp animation frame to 50
	J3DFrameCtrl& frameCtrl = getMotionFrameCtrl();
	if (frameCtrl.getFrame() > 50.0f) {
		J3DFrameCtrl& frameCtrl2 = getMotionFrameCtrl();
		frameCtrl2.setFrame(50.0f);
	}
	return false;
}

// downingCommon - 0x801384A8
f32 TMario::downingCommon(int anmId, f32 threshold, int nextState)
{
	f32 prevFrame = setAnimation(anmId, 1.0f);
	if (prevFrame < threshold) {
		slopeProcess();
		mForwardVel *= 0.96f;
		if (mForwardVel * mForwardVel < 1.0f) {
			setPlayerVelocity(0.0f);
		}
	} else if (mForwardVel >= 0.0f) {
		setPlayerVelocity(3.0f);
	} else {
		setPlayerVelocity(-3.0f);
	}

	if (walkProcess() == 0) {
		if (mForwardVel >= 0.0f) {
			changePlayerStatus(0x000208B1, nextState, false);
		} else {
			changePlayerStatus(0x000208B0, nextState, false);
		}
	} else {
		if (isLast1AnimeFrame()) {
			changePlayerStatus(0x0C400201, 0, false);
		}
	}
	return prevFrame;
}

// loserDown - 0x80138384
BOOL TMario::loserDown()
{
	slopeProcess();
	mForwardVel *= 0.9f;
	if (mForwardVel * mForwardVel < 1.0f) {
		setPlayerVelocity(0.0f);
	}

	setAnimation(275, 1.0f);

	switch (mActionState) {
	case 0:
		startVoice(30813);
		mActionState++;
		break;
	case 1:
		if (gpMSound->checkMarioVoicePlaying(0) != 0)
			break;
		mActionTimer = 0;
		mActionState++;
		break;
	case 2:
		if (mActionTimer++ > 60) {
			mActionState++;
		}
		break;
	case 3:
		startVoice(30817);
		mActionState++;
		break;
	case 4:
		break;
	}
	return false;
}

// jumpSlipCommon - 0x8013824C
int TMario::jumpSlipCommon(short anmId, u32 status)
{
	if (mInput & 0x1) {
		slopeProcess();
		mForwardVel *= 0.98f;
		if (mForwardVel * mForwardVel < 1.0f) {
			setPlayerVelocity(0.0f);
		}
	} else if (mForwardVel >= 0.5f) {
		mForwardVel = FConverge(mForwardVel, 0.0f, 100.0f, 100.0f);
		slopeProcess();
	} else {
		mVel.y = 0.0f;
	}

	int result = walkProcess();
	switch (result) {
	case 1:
		break;
	case 0:
		changePlayerStatus(status, 0, false);
		break;
	case 2:
		setAnimation(108, 1.0f);
		break;
	}

	setAnimation(anmId, 1.0f);
	return result;
}

// jumpSlipEvents - 0x80138114
BOOL TMario::jumpSlipEvents(TMario::JumpSlipRecord* record)
{
	if (mInput & 0x10) {
		changePlayerStatus(record->mStatus, 0, false);
		return;
	}

	mActionTimer++;
	if (mActionTimer >= record->mTimer) {
		changePlayerStatus(record->mStatus, 0, false);
		return;
	}

	u32 input = mInput;
	if (input & 0x2) {
		u32 jumpStatus = record->mJumpStatus;
		if ((jumpStatus - 0x02000000) == 0x0881) {
			if (mForwardVel >= mJumpParams.mSecJumpEnableSp.get()) {
				changePlayerJumping(0x02000881, 0);
				return;
			}
		}
		if (jumpStatus == 0x0882) {
			if (mForwardVel >= mJumpParams.mTriJumpEnableSp.get()) {
				changePlayerJumping(0x0882, 0);
				return;
			}
		}
		changePlayerJumping(0x02000880, 0);
		return;
	} else if (input & 0x8000) {
		mVel.y = 0.0f;
		changePlayerStatus(0x0080088A, 1, false);
		return;
	} else if (input & 0x4) {
		changePlayerStatus(record->mFallbackStatus, 0, false);
		return;
	}
	return 0;
}

// moveMain - 0x80138000
BOOL TMario::moveMain()
{
	static JumpSlipRecord sRecords[] = {
		{16, 0, 0x0C000230, 0x02000881, 0x0000088C, 0x50},
		{16, 0, 0x0C000232, 0x02000881, 0x0000088C, 0x50},
		{16, 0, 0x0C000231, 0x00000882, 0x0000088C, 0x50},
		{16, 0, 0x0C000233, 0x02000880, 0x0000088C, 0x50},
		{4, 0, 0x0800023A, 0x02000880, 0x0000088C, 0x50},
		{24, 0, 0x0800023B, 0x00000888, 0x0000088C, 0x50},
	};

	BOOL result = 0;
	checkEnforceJump();
	checkReturn();

	u32 action = mAction;
	u8 flag;
	if (action & 0x40000)
		flag = 1;
	else
		flag = 0;
	if (flag) {
		u8 isMoveAction;
		if (action & 0x4045C)
			isMoveAction = 1;
		else
			isMoveAction = 0;
		if (!isMoveAction) {
			u8 isNoisyAction;
			if (action & 0x84045D)
				isNoisyAction = 1;
			else
				isNoisyAction = 0;
			if (!isNoisyAction) {
				if (gpMSound->gateCheck(0x1009)) {
					MSoundSESystem::MSoundSE::startSoundActor(
					    0x1009, (Vec*)&mPosition, 0, nullptr, 0, 4);
				}
			}
		}
	}

	switch (mAction) {
	case 0x04000440:
		result = running();
		break;
	case 0x0441:
	case 0x0442:
		result = rotating();
		break;
	case 0x0443:
		result = turnning();
		break;
	case 0x0444:
		result = turnEnd();
		break;
	case 0x04000445: {
		BOOL caseResult;
		if (!(mInput & 0x10) && (mInput & 0xF)) {
			caseResult = checkAllMotions();
			goto finish_04000445;
		}
		int stopped = 0;
		f32 convergedVel = FConverge(mForwardVel, 0.0f, 4.0f, 4.0f);
		mForwardVel = convergedVel;
		if (convergedVel == 0.0f)
			stopped = 1;
		slopeProcess();
		if (stopped) {
			caseResult = changePlayerStatus(0x0C00023D, 0, false);
			goto finish_04000445;
		} else {
			int wp = walkProcess();
			switch (wp) {
			case 0:
				changePlayerStatus(0x088C, 0, false);
				break;
			case 2:
				if (mForwardVel > 16.0f) {
					playerRefrection(1);
					changePlayerDropping(0x00020462, 0);
				} else {
					setPlayerVelocity(0.0f);
					changePlayerStatus(0x0C00023D, 0, false);
				}
				break;
			}
		}
		setAnimation(15, 1.0f);
		caseResult = 0;
	finish_04000445:
		result = caseResult;
		break;
	}
	case 0x00810446:
		result = surfing();
		break;
	case 0x00800447:
		soundTorocco();
		toroccoEffect();
		result = 0;
		break;
	case 0x0400044A:
		result = walkEnd();
		break;
	case 0x044C: {
		BOOL caseResult;
		s16 savedAngle = mFaceAngle.y;
		f32 stopNormal = getSlideStopNormal();
		if (doSliding(stopNormal)) {
			caseResult = changePlayerStatus(0x0C00023E, 0, false);
		} else {
			slippingBasic(0x0C00023E, 0x0200088E, 15);
			mFaceAngle.y = savedAngle;
			caseResult = 0;
		}
		result = caseResult;
		break;
	}
	case 0x00020449:
		result = fireDashing();
		break;
	case 0x00840452:
		result = slipForeCommon(0x0C00023E, 0x02000880, 0x0200088E, 145);
		break;
	case 0x00840453:
		result = slipBackCommon(902, 0x088C, 137);
		break;
	case 0x00800456:
		result = catching();
		break;
	case 0x04808459: {
		BOOL caseResult;
		setNormalAttackArea();
		if (mInput & 0x8) {
			caseResult = changePlayerStatus(0x00840452, 0, false);
		} else if (mInput & 0x2) {
			caseResult = changePlayerJumping(0x02000880, 0);
		} else if (mInput & 0x10) {
			caseResult = changePlayerStatus(0x04000445, 0, false);
		} else {
			caseResult = slipForeCommon(0x0C008220, 0x02000880, 0x088C, 151);
		}
		result = caseResult;
		break;
	}
	case 0x0004045C:
		result = oilRun();
		break;
	case 0x0084045D:
		result = oilSlip();
		break;
	case 0x0004045E: {
		s16 timer = unk13C;
		unk13C = timer - 1;
		if (unk13C <= 0) {
			unk13C = 0;
			unk138 = 0.0f;
			changePlayerStatus(0x00800456, 0, false);
		}
		f32 polSize = mDirtyParams.mPolSizeSlip.value;
		f32 posZ    = mPosition.z;
		f32 posY    = mPosition.y;
		f32 posX    = mPosition.x;
		TPollutionManager* pollution = gpPollution;
		pollution->stamp(1, posX, posY, posZ, polSize);
		result = slipBackCommon(902, 0x088C, 137);
		break;
	}
	case 0x00020460:
		if (mActionTimer == 0) {
			mActionTimer++;
			emitParticle(12);
			rumbleStart(21, mMotorParams.mMotorWall.value);
		}
		downingCommon(1, 86.0f, mActionArg);
		result = 0;
		break;
	case 0x00020461:
		if (mActionTimer == 0) {
			mActionTimer++;
			emitParticle(12);
			rumbleStart(21, mMotorParams.mMotorWall.value);
		}
		downingCommon(44, 42.0f, mActionArg);
		result = 0;
		break;
	case 0x00020462:
		if (mActionTimer == 0) {
			mActionTimer++;
			emitParticle(12);
			rumbleStart(21, mMotorParams.mMotorWall.value);
		}
		downingCommon(123, 88.0f, mActionArg);
		result = 0;
		break;
	case 0x00020463:
		if (mActionTimer == 0) {
			mActionTimer++;
			emitParticle(12);
			rumbleStart(21, mMotorParams.mMotorWall.value);
		}
		downingCommon(124, 80.0f, mActionArg);
		result = 0;
		break;
	case 0x00020464:
		if (mActionTimer == 0) {
			mActionTimer++;
			emitParticle(12);
			rumbleStart(21, mMotorParams.mMotorWall.value);
		}
		downingCommon(116, 200.0f, mActionArg);
		result = 0;
		break;
	case 0x00020465:
		if (mActionTimer == 0) {
			mActionTimer++;
			emitParticle(12);
			rumbleStart(21, mMotorParams.mMotorWall.value);
		}
		downingCommon(117, 100.0f, mActionArg);
		result = 0;
		break;
	case 0x00020466:
		if (mActionTimer == 0) {
			mActionTimer++;
			emitParticle(12);
			rumbleStart(21, mMotorParams.mMotorWall.value);
		}
		downingCommon(138, 128.0f, mActionArg);
		result = 0;
		break;
	case 0x00020467:
		result = loserDown();
		break;
	case 0x04000470: {
		BOOL slipResult;
		if (jumpSlipEvents(&sRecords[0])) {
			slipResult = 1;
		} else {
			jumpSlipCommon(78, 0x088C);
			slipResult = 0;
		}
		result = slipResult;
		break;
	}
	case 0x04000471: {
		BOOL slipResult;
		if (jumpSlipEvents(&sRecords[1])) {
			slipResult = 1;
		} else {
			jumpSlipCommon(87, 0x088C);
			slipResult = 0;
		}
		result = slipResult;
		break;
	}
	case 0x04000472: {
		BOOL slipResult;
		if (jumpSlipEvents(&sRecords[2])) {
			slipResult = 1;
		} else {
			jumpSlipCommon(75, 0x088C);
			slipResult = 0;
		}
		result = slipResult;
		break;
	}
	case 0x04000473: {
		BOOL slipResult;
		if (jumpSlipEvents(&sRecords[3])) {
			slipResult = 1;
		} else {
			int commonResult = jumpSlipCommon(190, 0x088C);
			if (commonResult != 2) {
				mFaceAngle.x = 0;
				s16 angle = mModelFaceAngle;
				mModelFaceAngle = angle + 0x8000;
			}
			slipResult = 0;
		}
		result = slipResult;
		break;
	}
	case 0x04000478: {
		BOOL slipResult;
		if (jumpSlipEvents(&sRecords[4])) {
			slipResult = 1;
		} else {
			jumpSlipCommon(192, 0x088C);
			slipResult = 0;
		}
		result = slipResult;
		break;
	}
	case 0x0479: {
		BOOL slipResult;
		if (!(mInput & 0x4000)) {
			mInput &= ~0x2;
		}
		if (jumpSlipEvents(&sRecords[5])) {
			slipResult = 1;
		} else {
			jumpSlipCommon(152, 0x088C);
			slipResult = 0;
		}
		result = slipResult;
		break;
	}
	default:
		break;
	}

	return result;
}
