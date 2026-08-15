#include <Player/MarioMain.hpp>
#include <MSound/MSoundBGM.hpp>

#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <JSystem/JMath.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <System/MarDirector.hpp>
#include <Player/Watergun.hpp>
#include <Player/NozzleBase.hpp>
#include <Player/NozzleTrigger.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/M3UModelMario.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <System/MarioGamePad.hpp>

// NOTE: -inline deferred means functions must be in REVERSE address order.

// startTalking - 0x80145D44
BOOL TMario::startTalking()
{
	const TBGCheckData* ground = mGroundPlane;
	u8 isTalking;
	if (ground->mFlags & 0x10) {
		isTalking = 1;
	} else {
		isTalking = 0;
	}

	u8 notTalking;
	if (isTalking == 1) {
		notTalking = 0;
	} else {
		notTalking = 1;
	}

	if (notTalking) {
		f32 height = 1.0f + mFloorPosition.y;
		mPosition.y = height;
		changePlayerStatus(0x10001308, 0, true);
		return 1;
	}
	return 0;
}

// canSleep - 0x80145B88
BOOL TMario::canSleep()
{
	u8 hasFlags;
	if (mState & 0x00030000) {
		hasFlags = 1;
	} else {
		hasFlags = 0;
	}
	if (hasFlags)
		return 0;

	f32 sleepCheckDist = mDeParams.mSleepingCheckDist.value;
	f32 sleepCheckTol = mDeParams.mSleepingCheckHeight.value;
	const TBGCheckData* bgData;

	f32 groundY = gpMap->checkGround(
		mPosition.x - sleepCheckDist, 30.0f + mPosition.y, mPosition.z, &bgData);
	f32 floorY = mFloorPosition.y;
	if (groundY < floorY - sleepCheckTol || floorY + sleepCheckTol < groundY)
		return 0;

	groundY = gpMap->checkGround(
		mPosition.x + sleepCheckDist, 30.0f + mPosition.y, mPosition.z, &bgData);
	floorY = mFloorPosition.y;
	if (groundY < floorY - sleepCheckTol || floorY + sleepCheckTol < groundY)
		return 0;

	groundY = gpMap->checkGround(
		mPosition.x, 30.0f + mPosition.y, mPosition.z - sleepCheckDist, &bgData);
	floorY = mFloorPosition.y;
	if (groundY < floorY - sleepCheckTol || floorY + sleepCheckTol < groundY)
		return 0;

	groundY = gpMap->checkGround(
		mPosition.x, 30.0f + mPosition.y, mPosition.z + sleepCheckDist, &bgData);
	floorY = mFloorPosition.y;
	if (groundY < floorY - sleepCheckTol || floorY + sleepCheckTol < groundY)
		return 0;

	if (gpMap->isTouchedOneWall(mPosition.x, 30.0f + mPosition.y, mPosition.z, 80.0f))
		return 0;

	return 1;
}

// canPut - 0x80145AC8
BOOL TMario::canPut()
{
	if (gpMap->isTouchedOneWall(mPosition.x + 100.0f * JMASSin(mFaceAngle.y),
	                            10.0f + mPosition.y,
	                            mPosition.z + 100.0f * JMASCos(mFaceAngle.y),
	                            mHeldObject->getDamageRadius()))
		return 0;

	if (gpMap->isTouchedOneWall(mPosition.x, 10.0f + mPosition.y, mPosition.z,
	                            mHeldObject->getDamageRadius()))
		return 0;

	return 1;
}

// waitingCommonEvents - 0x801458A8
BOOL TMario::waitingCommonEvents()
{
	u32 input = mInput;

	if (input & 0x02) {
		if (considerRotateJumpStart())
			return 1;
		return changePlayerJumping(0x02000880, 0);
	}

	if (input & 0x04) {
		return changePlayerStatus(0x088C, 0, false);
	}

	if (input & 0x08) {
		return changePlayerStatus(0x50, 0, false);
	}

	if (input & 0x10) {
		return changePlayerStatus(0x0C000227, 0, false);
	}

	if (input & 0x01) {
		s16 faceY = mFaceAngle.y;
		s32 turnSpeed = mDeParams.mWaitingRotSp.value;
		s32 turnSpeed2 = turnSpeed;
		s32 diff = (s16)(mIntendedYaw - faceY);
		s32 converged = IConverge((int)diff, 0, (int)turnSpeed, (int)turnSpeed2);
		mFaceAngle.y = (s16)(mIntendedYaw - converged);

		if (mIntendedMag > mControllerParams.mStartToWalkLevel.value) {
			emitSmoke(mFaceAngle.y);
			return changePlayerStatus(0x04000440, 0, false);
		}
	}

	u8 hasJumpInput;
	if (mState & 0x00004000) {
		hasJumpInput = 1;
	} else {
		hasJumpInput = 0;
	}
	if (hasJumpInput) {
		return changePlayerStatus(0x04000440, 0, false);
	}

	if ((u8)canSquat()) {
		mForwardVel = 0.0f;
		return changePlayerStatus(0x0C008220, 0, false);
	}

	if (mInput & 0x00008000) {
		return changePlayerStatus(0x384, 0, false);
	}

	if (rocketCheck()) {
		TWaterGun* gun = mWaterGun;
		f32 rocketHeight = *(f32*)((u8*)gun + 0x1D40);
		mRocketTargetY = mFloorPosition.y + rocketHeight;
		return changePlayerStatus(0x088B, 0, false);
	}

	if (considerRotateStart())
		return 1;

	return 0;
}

// stopCommon - 0x801457EC
#pragma dont_inline on
void TMario::stopCommon(int animId, int nextState)
{
	waitProcess();
	setAnimation(animId, 1.0f);

	if (onYoshi()) {
		MActor* actor = *(MActor**)((u8*)mYoshi + 0x34);
		if (actor->curAnmEndsNext(0, 0)) {
			changePlayerStatus(nextState, 0, false);
			return;
		}
	} else {
		if (isLast1AnimeFrame()) {
			changePlayerStatus(nextState, 0, false);
		}
	}
}
#pragma dont_inline off

// changeMontemanWaitingAnim - 0x801457CC
void TMario::changeMontemanWaitingAnim()
{
	if (mAction != 0x0C400201)
		return;
	mActionState |= 0x02;
}

// waiting - 0x8014552C
BOOL TMario::waiting()
{
	if (waitingCommonEvents())
		return 1;

	if (isMario()) {
		u8 isPumpFive;
		if (mPumpState == 5) {
			isPumpFive = 1;
		} else {
			isPumpFive = 0;
		}
		if (isPumpFive) {
			u8 canSleepResult = canSleep();
			if (canSleepResult) {
				if (mAnimationId == 0xC3) {
					if (isAnimeLoopOrStop()) {
						if (mGroundPlane != nullptr) {
							if (mGroundPlane->mNormal.y > 0.99f) {
								mActionTimer++;
								if (mActionTimer >= 10) {
									return changePlayerStatus(0x0C400202, 0, false);
								}
							}
						}
					}
				}
			}
		}
	}

	u16 actionState = mActionState;

	if (actionState & 0x02) {
		setAnimation(0x114, 1.0f);
	} else if (gpMarDirector->unk124 == 3) {
		setAnimation(0xD9, 1.0f);
	} else {
		f32 val368 = *(f32*)((u8*)this + 0x368);
		int isPositive;
		if (val368 > 0.0f) {
			isPositive = 1;
		} else {
			isPositive = 0;
		}

		if (isPositive != 0) {
			setAnimation(0xE7, 1.0f);
		} else {
			if (mPumpState == 5
			    && (mPrevAction == 0x0C00023D
			        || (mState & 0x00000020 ? true : false))
			    && !(actionState & 0x01)) {
				// montemanWait
				setAnimation(0xDA, 1.0f);

				J3DFrameCtrl* frameCtrl = mModel->unkC;
				if (frameCtrl->checkPass(138.0f)) {
					emitSweat((s16)(mFaceAngle.y - 0x4000));
				}

				if (isLast1AnimeFrame()) {
					mActionState |= 0x01;
				}
			} else {
				// regularWait
				if (mHealth <= 3) {
					if (mAnimationId != 0x11D && mAnimationId != 0x127) {
						setAnimation(0x127, 1.0f);
					} else if (mAnimationId == 0x127) {
						if (isLast1AnimeFrame()) {
							setAnimation(0x11D, 1.0f);
						}
					}
				} else {
					if (0.0f == mIntendedMag) {
						setAnimation(0xC3, 1.0f);
					} else {
						setAnimation(0x12C, 1.0f);
					}
				}
			}
		}
	}

	waitProcess();
	return 0;
}

// sleepily - 0x8014540C
BOOL TMario::sleepily()
{
	if (waitingCommonEvents())
		return 1;

	if (*(f32*)((u8*)unk108 + 0x1c) > 0.0f
	    || *(f32*)((u8*)unk108 + 0x20) > 0.0f) {
		changePlayerStatus(0x0C400201, 0, false);
	}

	if (mActionState == 3) {
		return changePlayerStatus(0x0C000203, 0, false);
	}

	switch (mActionState) {
	case 0:
		setAnimation(0x12F, 1.0f);
		break;
	case 1:
		setAnimation(0x130, 1.0f);
		break;
	case 2:
		setAnimation(0x131, 1.0f);
		break;
	}

	if (isLast1AnimeFrame()) {
		mActionState++;
	}

	waitProcess();
	return 0;
}

// sleeping - 0x80145278
BOOL TMario::sleeping()
{
	u32 input = mInput;

	if ((input & 0xA41F) || *(f32*)((u8*)unk108 + 0x1c) > 0.0f
	    || *(f32*)((u8*)unk108 + 0x20) > 0.0f) {
		// wakeUp
		if (mActionState == 0) {
			startSoundActor(0x7883);
		} else {
			startSoundActor(0x789A);
		}
		return changePlayerStatus(0x0C000204, mActionState, false);
	}

	waitProcess();

	{
		u8 hasFlag;
		if (mSubState & 0x02) {
			hasFlag = 1;
		} else {
			hasFlag = 0;
		}
		if (hasFlag) {
			mSleepPos = mPosition;
			sleepingEffect();
		}
	}

	switch (mActionState) {
	case 0:
		setAnimation(0x132, 1.0f);
		if (isLast1AnimeFrame()) {
			u16 timer = mActionTimer;
			timer++;
			mActionTimer = timer;
			if ((u16)timer > 40) {
				mActionState++;
			}
		}
		break;
	case 1:
		setAnimation(0x134, 1.0f);
		if (isLast1AnimeFrame()) {
			mActionState++;
		}
		break;
	case 2:
		setAnimation(0x135, 1.0f);
		break;
	}

	return 0;
}

// getSideWalkValues - 0x801451A8
void TMario::getSideWalkValues(E_SIDEWALK_TYPE* outType, f32* outSpeed, f32* outStickMag)
{
	s16 faceY = mFaceAngle.y;
	s16 intendedYaw = mIntendedYaw;
	s16 diff = (s16)(intendedYaw - faceY);
	s16 diffExt = diff;
	u16 diffU = (u16)diffExt;
	s32 idx = (s32)diffU >> jmaSinShift;
	f32 sinVal = jmaSinTable[idx];
	f32 sideComponent = mIntendedMag * sinVal;
	f32 ff8 = mRunParams.mPumpingSlideSp.value;
	f32 sidewalkVel = sideComponent * ff8;

	if (0.0f == sidewalkVel) {
		*outType = (E_SIDEWALK_TYPE)0;
		*outSpeed = getMotionFrameCtrl().getRate();
	} else {
		f32 f100c = mRunParams.mPumpingSlideAnmSp.value;
		f32 speed = sidewalkVel * f100c;
		if (speed < 0.0f)
			speed = -speed;
		*outSpeed = speed;

		if (diffExt > 0) {
			*outType = (E_SIDEWALK_TYPE)1;
		} else {
			*outType = (E_SIDEWALK_TYPE)2;
		}
	}

	*outStickMag = sidewalkVel;
}

// squating - 0x80144DEC
BOOL TMario::squating()
{
	u32 input = mInput;

	if (input & 0x04) {
		return changePlayerStatus(0x088C, 0, false);
	}

	if (input & 0x08) {
		return changePlayerStatus(0x50, 0, false);
	}

	if (input & 0x10) {
		return changePlayerStatus(0x0C008222, 0, false);
	}

	if (!(input & 0x4000) && !(input & 0x200)) {
		return changePlayerStatus(0x0C008222, 0, false);
	}

	TWaterGun* gun;
	if ((gun = mWaterGun) != nullptr) {
		u8 hasFlag;
		if (mState & 0x00008000) {
			hasFlag = 1;
		} else {
			hasFlag = 0;
		}
		if (hasFlag)
			goto squatMain;
	}

squatStandup:
	return changePlayerStatus(0x0C008222, 0, false);

squatMain:
	if (input & 0x02) {
		TMarioGamePad* pad = mGamePad;
		if (pad->mMeaning & 0x0400) {
			if (gun != nullptr) {
				s32 isNotHipDropping = *(u8*)((u8*)gun + 0x1C84);
				if (isNotHipDropping == 0) {
					rumbleStart(21, mMotorParams.mMotorHipDrop.value);
					return changePlayerStatus(0x0883, 0, false);
				}
			}
		}
	}

	{
		TNozzleBase* nozzle = gun->getCurrentNozzle();
		u8 nozzleKind = *(u8*)((u8*)nozzle + 0x18);
		if (nozzleKind == 1) {
			gun = mWaterGun;
			u8 canSpray;
			if (gun->mCurrentWater == 0) {
				canSpray = 0;
			} else {
				TNozzleBase* nozzle2 = gun->getCurrentNozzle();
				s32 kind = nozzle2->getNozzleKind();
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
				TWaterGun* gun3 = mWaterGun;
				f32 rocketHeight = *(f32*)((u8*)gun3 + 0x1D40);
				mRocketTargetY = mFloorPosition.y + rocketHeight;
				return changePlayerStatus(0x088B, 0, false);
			}
		}
	}

	{
		TMarioGamePad* pad = mGamePad;
		u32 meaning = pad->mMeaning;

		if (meaning & 0x2000) {
			E_SIDEWALK_TYPE sideType;
			f32 sideSpeed;
			f32 stickMag;
			getSideWalkValues(&sideType, &sideSpeed, &stickMag);

			switch (sideType) {
			case 0:
				setAnimation(0x98, 1.0f);
				break;
			case 1:
				setAnimation(0x7F, sideSpeed);
				break;
			case 2:
				setAnimation(0x80, sideSpeed);
				break;
			}

			u16 faceAngle = mFaceAngle.y;
			s32 cosIdx = (s32)faceAngle >> jmaSinShift;
			f32 cosVal = jmaCosTable[cosIdx];
			mPosition.x = stickMag * cosVal + mPosition.x;

			faceAngle = mFaceAngle.y;
			s32 sinIdx = (s32)faceAngle >> jmaSinShift;
			f32 sinVal = jmaSinTable[sinIdx];
			mPosition.z = -(stickMag * sinVal) + mPosition.z;
		} else if (meaning & 0x0400) {
			f32 analogStick = *(f32*)((u8*)pad + 0xA8);
			u8 isPositive = 1;
			f32 absVal = __fabsf(analogStick);
			if (analogStick < 0.0f)
				isPositive = 0;

			f32 threshold = mControllerParams.mSquatRotMidAnalog.value;
			f32 maxSpeed = mControllerParams.mSquatRotMidValue.value;
			f32 turnSpeed;

			if (absVal < threshold) {
				turnSpeed = maxSpeed * (absVal / threshold);
			} else {
				f32 range = 1.0f - threshold;
				f32 excess = absVal - threshold;
				f32 speedRange = 1.0f - maxSpeed;
				turnSpeed = speedRange * (excess / range) + maxSpeed;
			}

			if (!isPositive)
				turnSpeed = -turnSpeed;

			s16 maxTurnRate = mDeParams.mWaitingRotSp.value;
			s32 negRate = -maxTurnRate;
			s16 faceY = mFaceAngle.y;
			s32 angleDelta = (s32)(turnSpeed * negRate);
			mFaceAngle.y = (s16)(faceY + angleDelta);

			setAnimation(0x98, 1.0f);
		}
	}

	waitProcess();
	return 0;
}

// squatStandup - 0x80144CD0
BOOL TMario::squatStandup()
{
	u32 input = mInput;

	if (input & 0x04) {
		return changePlayerStatus(0x088C, 0, false);
	}

	if (input & 0x08) {
		return changePlayerStatus(0x50, 0, false);
	}

	if (input & 0x02) {
		return changePlayerStatus(0x02000880, 0, false);
	}

	if (input & 0x01) {
		return changePlayerStatus(0x04000440, 0, false);
	}

	waitProcess();

	if (mAction == 0x0C000223) {
		setAnimation(0x121, 1.0f);
	} else {
		setAnimation(0x96, 1.0f);
	}

	if (isLast1AnimeFrame()) {
		changePlayerStatus(0x0C400201, 0, false);
	}

	return 0;
}

// jumpEndCommon - 0x80144C50
#pragma dont_inline on
BOOL TMario::jumpEndCommon(int animId, int nextState)
{
	waitProcess();
	setAnimation(animId, 1.0f);

	if (isLast1AnimeFrame()) {
		return changePlayerStatus(nextState, 0, false);
	}
	return 0;
}
#pragma dont_inline off

// jumpEndEvents - 0x80144BD8
#pragma dont_inline on
BOOL TMario::jumpEndEvents(u32 nextState)
{
	u32 input = mInput;

	if (input & 0x10) {
		return changePlayerStatus(0x0C400201, 0, false);
	}

	if (input & 0x02) {
		if (nextState == 0) {
			return changePlayerTriJump();
		} else {
			return changePlayerJumping(nextState, 0);
		}
	}

	if (input & 0x0F) {
		return checkAllMotions();
	}

	return 0;
}
#pragma dont_inline off

// waitMain - 0x80144300
BOOL TMario::waitMain()
{
	BOOL result = 0;

	checkEnforceJump();

	checkReturn();

	setNormalAttackArea();

	// Check held object for wall collision
	TTakeActor* heldObj = mHeldObject;
	if (heldObj != nullptr) {
		u8 hasInput;
		if (mInput & 0x2000) {
			hasInput = 1;
		} else {
			hasInput = 0;
		}
		if (hasInput) {
			s32 actorType = heldObj->mActorType;
			switch (actorType) {
			case 0x80000001:
				changePlayerStatus(0x80000588, 0, false);
				break;
			default:
				if (canPut()) {
					changePlayerStatus(0x80000387, 0, false);
				}
				break;
			}
		}
	}

	u32 action = mAction;

	switch (action) {
	case 0x0C400201:
		result = waiting();
		break;
	case 0x0C400202:
		result = sleepily();
		break;
	case 0x0C000203:
		result = sleeping();
		break;
	case 0x0C000204: {
		// Wakeup
		register BOOL tmpResult;
		u32 input = mInput;
		if (input & 0x04) {
			sleepingEffectKill();
			tmpResult = changePlayerStatus(0x088C, 0, false);
		} else if (input & 0x08) {
			sleepingEffectKill();
			tmpResult = changePlayerStatus(0x50, 0, false);
		} else if (waitingCommonEvents()) {
			sleepingEffectKill();
			return 1;
		} else {
			waitProcess();
			int animId;
			if (mActionArg == 0) {
				animId = 0x133;
			} else {
				animId = 0x136;
			}
			setAnimation(animId, 1.0f);
			if (isLast1AnimeFrame()) {
				sleepingEffectKill();
				tmpResult = changePlayerStatus(0x0C400201, 0, false);
			} else {
				tmpResult = 0;
			}
		}
		result = tmpResult;
		break;
	}
	case 0x0C008220:
		result = squating();
		break;
	case 0x0C008221:
		result = 0;
		break;
	case 0x0C008222:
	case 0x0C000223:
		result = squatStandup();
		break;
	case 0x0C00022F: {
		// action == 0x0C00022F: squat landing
		BOOL tmpResult;
		u32 input = mInput;
		if (input & 0x04) {
			tmpResult = changePlayerStatus(0x088C, 0, false);
		} else if (input & 0x08) {
			tmpResult = changePlayerStatus(0x50, 0, false);
		} else {
			waitProcess();
			setAnimation(0xF3, 1.0f);
			if (isLast1AnimeFrame()) {
				changePlayerStatus(0x0C400201, 0, false);
			}
			tmpResult = 0;
		}
		result = tmpResult;
		break;
	}
	case 0x0C000230: {
		// jumpEnd - landing type 1
		BOOL tmpResult;
		if (jumpEndEvents(0)) {
			tmpResult = 1;
		} else {
			waitProcess();
			setAnimation(0x4E, 1.0f);
			if (isLast1AnimeFrame()) {
				changePlayerStatus(0x0C400201, 0, false);
			}
			tmpResult = 0;
		}
		result = tmpResult;
		break;
	}
	case 0x0C000231: {
		// jumpEnd - landing type 2
		BOOL tmpResult;
		if (jumpEndEvents(0)) {
			tmpResult = 1;
		} else {
			waitProcess();
			setAnimation(0x4B, 1.0f);
			if (isLast1AnimeFrame()) {
				changePlayerStatus(0x0C400201, 0, false);
			}
			tmpResult = 0;
		}
		result = tmpResult;
		break;
	}
	case 0x0C000232: {
		// jumpEnd - landing type 3 (broadjump/fire)
		BOOL tmpResult;
		if (jumpEndEvents(0)) {
			tmpResult = 1;
		} else {
			waitProcess();
			setAnimation(0x57, 1.0f);
			if (isLast1AnimeFrame()) {
				changePlayerStatus(0x0C400201, 0, false);
			}
			tmpResult = 0;
		}
		result = tmpResult;
		break;
	}
	case 0x0C000233: {
		// fire jump end
		BOOL tmpResult;
		if (jumpEndEvents(0)) {
			tmpResult = 1;
		} else {
			waitProcess();
			setAnimation(0xBE, 1.0f);
			if (isLast1AnimeFrame()) {
				changePlayerStatus(0x0C400201, 0, false);
			}
			mFaceAngle.x     = (tmpResult = 0);
			mModelFaceAngle += 0x8000;
		}
		result = tmpResult;
		break;
	}
	case 0x80000A36:
		// throw end
		checkThrowObject();
		jumpEndCommon(0x65, 0x0C400201);
		result = 0;
		break;
	case 0x08000239: {
		// pullEnd
		BOOL tmpResult;
		mInput &= ~0x2010;
		if (jumpEndEvents(0)) {
			tmpResult = 1;
		} else {
			waitProcess();
			setAnimation(0x28, 1.0f);
			if (isLast1AnimeFrame()) {
				changePlayerStatus(0x0C400201, 0, false);
			}
			tmpResult = 0;
		}
		result = tmpResult;
		break;
	}
	case 0x0800023A: {
		BOOL tmpResult;
		if (jumpEndEvents(0x02000880)) {
			tmpResult = 1;
		} else {
			waitProcess();
			setAnimation(0x57, 1.0f);
			if (isLast1AnimeFrame()) {
				changePlayerStatus(0x0C400201, 0, false);
			}
			tmpResult = 0;
		}
		result = tmpResult;
		break;
	}
	case 0x0800023B: {
		// uTurnJumpEnd
		BOOL tmpResult;
		mInput &= ~0x2000;
		if (jumpEndEvents(0x02000880)) {
			if (mAction == 0x04000440) {
				tmpResult = changePlayerStatus(0x0C008222, 0, false);
			} else {
				tmpResult = 1;
			}
		} else {
			waitProcess();
			setAnimation(0x98, 1.0f);
			if (isLast1AnimeFrame()) {
				changePlayerStatus(0x0C008222, 0, false);
			}
			tmpResult = 0;
		}
		result = tmpResult;
		break;
	}
	case 0x0080023C: {
		// broadJumpEnd
		BOOL tmpResult;
		mActionState = 1;
		u32 input = mInput;
		if (input & 0x04) {
			tmpResult = changePlayerStatus(0x088C, 0, false);
		} else if (input & 0x08) {
			tmpResult = changePlayerStatus(0x00840452, 0, false);
		} else {
			waitProcess();
			setAnimation(0x3A, 1.0f);
			if (isLast1AnimeFrame()) {
				changePlayerStatus(0x0C00023E, 0, false);
			}
			tmpResult = 0;
		}
		result = tmpResult;
		break;
	}
	case 0x0C00023D: {
		// hipAttackEnd
		BOOL tmpResult;
		u32 input = mInput;
		if (!(input & 0x10) && (input & 0x0F)) {
			tmpResult = checkAllMotions();
		} else {
			stopCommon(0x10, 0x0C400201);
			tmpResult = 0;
		}
		result = tmpResult;
		break;
	}
	case 0x0C00023E: {
		// slipEnd
		BOOL tmpResult;
		u32 input = mInput;
		if (input & 0x0F) {
			tmpResult = checkAllMotions();
		} else {
			stopCommon(0x8F, 0x0C400201);
			tmpResult = 0;
		}
		result = tmpResult;
		break;
	}
	}

	return result;
}
