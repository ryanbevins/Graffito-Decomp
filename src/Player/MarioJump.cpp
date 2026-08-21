#define JG_TUTIL_SQRT_OUT_OF_LINE
#include <Player/MarioMain.hpp>
#include <MSound/MSoundBGM.hpp>

#include <Map/Map.hpp>
#include <JSystem/JMath.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Map/MapData.hpp>
#include <Map/MapCollisionData.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <Player/Watergun.hpp>
#include <Player/NozzleBase.hpp>
#include <Player/NozzleTrigger.hpp>
#include <Strategic/LiveActor.hpp>
#include <Camera/CameraShake.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <System/MarDirector.hpp>
#include <System/MarioGamePad.hpp>
#include <fake_tgmath.h>

// NOTE: -inline deferred means functions must be in REVERSE address order.

BOOL TMario::startJumpWall()
{
	if (mWallPlane != nullptr) {
		const JGeometry::TVec3<f32>& normal = mWallPlane->getNormal();
		s16 angle = matan(normal.z, normal.x) + 0x8000;
		emitParticle(24, angle);
		emitParticle(25, angle);
	}
	mVel.y = 52.0f;
	mFaceAngle.y += 0x8000;
	if (mVel.y + (160.0f + mPosition.y) >= mFloorPosition.x)
		mVel.y = 1.0f;
	return changePlayerStatus(0x02000886, 0, false);
}

void TMario::doJumping()
{
	mForwardVel = mForwardVel * mJumpParams.mJumpSpeedBrake.value;
	f32 sideVel = 0.0f;
	if (mInput & 1) {
		s16 intendedYaw = mIntendedYaw;
		s16 faceY = mFaceAngle.y;
		f32 intendedMag = mIntendedMag;
		s16 angleDiff = intendedYaw - faceY;
		if (mAction == 0x088B) {
			u8 hasFludd;
			if (mState & MARIO_FLAG_HAS_FLUDD) hasFludd = 1; else hasFludd = 0;
			if (hasFludd) {
				TWaterGun* gun = mWaterGun;
				u8 nozzleReady;
				if (gun->mCurrentWater == 0) {
					nozzleReady = FALSE;
				} else {
					s32 kind = gun->getCurrentNozzle()->getNozzleKind();
					if (kind == 1) {
						TNozzleTrigger* t = (TNozzleTrigger*)gun->getCurrentNozzle();
						if (t->unk385 == TNozzleTrigger::ACTIVE) nozzleReady = TRUE;
						else nozzleReady = FALSE;
					} else {
						if (gun->getCurrentNozzle()->unk378 > 0.0f)
							nozzleReady = TRUE;
						else
							nozzleReady = FALSE;
					}
				}
				if (nozzleReady)
					intendedMag = 2.5f * intendedMag;
			}
		}
		if (mAction == 0x02000886) {
			if (mVel.y > 0.0f) {
				s16 d = (s16)angleDiff;
				if (d < -16384 || d > 16384) intendedMag = 0.0f;
			}
		}
		f32 accel;
		if (onYoshi() && mYoshi->mFlutterState == 1) {
			s16 d = (s16)angleDiff;
			if (d > -16384 && d < 16384) accel = mYoshiParams.mHoldOutAccCtrlF.value;
			else accel = mYoshiParams.mHoldOutAccCtrlB.value;
		} else accel = getJumpAccelControl();
		u16 au = (u16)angleDiff;
		mForwardVel = accel * intendedMag * JMASCos(au) + mForwardVel;
		sideVel = intendedMag * JMASSin(au) * getJumpSlideControl();
	}
	if (mForwardVel > 32.0f) mForwardVel -= 0.2f;
	if (mForwardVel < -16.0f) mForwardVel += 0.4f;
	mSlideVelX = mForwardVel * JMASSin(mFaceAngle.y);
	mSlideVelZ = mForwardVel * JMASCos(mFaceAngle.y);
	mSlideVelX += sideVel * JMASSin((u16)(mFaceAngle.y + 16384));
	mSlideVelZ += sideVel * JMASCos((u16)(mFaceAngle.y + 16384));
	mVel.x = mSlideVelX;
	mVel.z = mSlideVelZ;
	if (mVel.y < 0.0f) {
		// Pointer math slop
		*(f32*)((u8*)this + 0x50) = mDeParams.mTrampleRadius.value;
		calcEntryRadius();
		*(f32*)((u8*)this + 0x54) = mDeParams.mAttackHeight.value;
		calcEntryRadius();
	} else {
		// Pointer math slop
		*(f32*)((u8*)this + 0x50) = mDeParams.mPushupRadius.value;
		calcEntryRadius();
		*(f32*)((u8*)this + 0x54) = mDeParams.mPushupHeight.value;
		calcEntryRadius();
	}
}

int TMario::jumpingBasic(int statusId, int anmId, int groundCheck)
{
	doJumping();
	int result = jumpProcess(groundCheck);
	switch (result) {
	case 0:
		setAnimation(anmId, 1.0f);
		break;
	case 1: {
		if (mGroundPlane->getActor())
			((TLiveActor*)mGroundPlane->getActor())->receiveMessage(this, 0);
		u8 canCatch = 0, shouldCatch = 1;
		u8 hy = (mSubState & 0x100) ? shouldCatch : 0;
		if (hy == true) shouldCatch = 0;
		if (mLastGroundPos.y - mPosition.y <= mDeParams.mDamageFallHeight.value)
			shouldCatch = 0;
		if (onYoshi()) shouldCatch = 0;
		u16 bg = *(u16*)((u8*)mGroundPlane);
		u8 badGround;
		if (bg != 0x0A && bg != 0x800A && bg != 0x0108) {
			u8 r; if (bg == 0x07 || bg == 0x8007) r = 1; else r = 0;
			if (!r) {
				u8 t;
				if (bg == 0x0108 || bg == 0x08 || bg == 0x8008)
					t = 1;
				else
					t = 0;
				if (!t) {
					u8 w; if (bg == 0x09 || bg == 0x8009) w = 1; else w = 0;
					if (w) badGround = 1; else badGround = 0;
				} else
					badGround = 1;
			} else
				badGround = 1;
		} else
			badGround = 1;
		if (badGround)
			shouldCatch = 0;
		if (mVel.y > -70.0f)
			shouldCatch = 0;
		if (shouldCatch) {
			u8 inSand;
			if (mState & 0x40000)
				inSand = TRUE;
			else
				inSand = FALSE;
			if (inSand) {
				sinkInSandEffect();
				changePlayerStatus(0x0002033C, 0, false);
				break;
			}
			u8 hasFludd;
			if (mState & MARIO_FLAG_HAS_FLUDD)
				hasFludd = TRUE;
			else
				hasFludd = FALSE;
			if (hasFludd) {
				if (*(u8*)((u8*)mWaterGun + 0x1C84) != 2) {
					mTrembleModelEffect->tremble(
					    mJumpParams.mTremblePower.value,
					    mJumpParams.mTrembleAccele.value,
					    mJumpParams.mTrembleBrake.value,
					    mJumpParams.mTrembleTime.get());
					changePlayerStatus(0x0479, 0, false);
					rumbleStart(21, mMotorParams.mMotorHipDrop.value);
					canCatch = 1;
					startVoice(0x789E);
					if (gpMSound->gateCheck(0x193E))
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x193E, (const Vec*)&mPosition, 0, nullptr,
						    0, 4);
					strongTouchDownEffect();
					floorDamageExec(1, 3, 0,
					                mMotorParams.mMotorReturn.value);
				}
			}
		}
		mLastGroundPos.y = mPosition.y;
		changePlayerStatus(statusId, 0, false);
		if (!canCatch) { rumbleStart(20, mMotorParams.mMotorWall.value / 2); stopVoice(); }
		u32 pa = mPrevAction;
		if (pa == 0x0887 || (pa - 0x0895 <= 1)) strongTouchDownEffect();
		else smallTouchDownEffect();
		break;
	}
	case 2: {
		if (mAction == 0x0893) break;
		if (mForwardVel > mDeParams.mClashSpeed.value) {
			emitParticle(12);
			changePlayerDropping(0x000208B0, 0);
			break;
		}
		setAnimation(anmId, 1.0f);
		if (onYoshi()) { setPlayerVelocity(0.0f); break; }
		if (mWallPlane) {
			// Pointer math slop
			u16 wt = *(u16*)((u8*)mWallPlane);
			u8 rp; if (wt == 5 || wt == 0x8005) rp = 1; else rp = 0;
			if (rp) { changePlayerStatus(0x088D, 0, false); setPlayerVelocity(0.0f); break; }
		}
		if (mWallPlane) {
			// Pointer math slop
			u16 wt = *(u16*)((u8*)mWallPlane);
			u8 fc; if (wt == 0x010A) fc = 1; else fc = 0;
			if (fc) {
				s16 wa = matan(mWallPlane->getNormal().z, mWallPlane->getNormal().x);
				mFaceAngle.y = wa + 0x8000;
				mModelFaceAngle = mFaceAngle.y;
				if (mAction == 0x0887) mModelFaceAngle = mModelFaceAngle - 0x8000;
				rumbleStart(21, mMotorParams.mMotorWall.value);
				changePlayerStatus(0x3000036C, 0, false);
				break;
			}
		}
		if (mForwardVel > 16.0f && mAction != 0x088D) {
			playerRefrection(0);
			mFaceAngle.y += 0x8000;
			if (mWallPlane) {
				changePlayerStatus(0x08A7, 0, false);
				if (isMario()) {
					rumbleStart(21, mMotorParams.mMotorWall.value);
					gpCameraShake->startShake((EnumCamShakeMode)1, 1.0f);
					u32 sid = gpMSound->getWallSound(
					    *(u8*)((u8*)mWallPlane + 6), mForwardVel);
					if (gpMSound->gateCheck(sid))
						MSoundSESystem::MSoundSE::startSoundActor(sid, (const Vec*)&mPosition, 0, nullptr, 0, 4);
				}
				break;
			}
			if (mVel.y > 0.0f)
				mVel.y = 0.0f;
			if (mForwardVel >= 38.0f) {
				changePlayerStatus(0x000208B0, 0, false);
				break;
			}
			if (mForwardVel > 8.0f)
				setPlayerVelocity(-8.0f);
			changePlayerStatus(0x000208B6, 0, false);
		} else {
			setPlayerVelocity(0.0f);
		}
		break;
	}
	case 3:
		setAnimation(0x33, 1.0f);
		changePlayerDropping(0x3800034B, 0);
		break;
	case 4:
		rumbleStart(21, mMotorParams.mMotorWall.value);
		changePlayerStatus(0x08200348, 0, false);
		break;
	}
	return result;
}

#pragma dont_inline on
BOOL TMario::considerJumpRotate()
{
	int dir;
	if (checkStickRotate(&dir) == 1) {
		switch (dir) {
		case 2: mAction = 0x0896; break;
		case 3: mAction = 0x0895; break;
		}
		return TRUE;
	}
	return FALSE;
}
#pragma dont_inline off

#pragma dont_inline on
BOOL TMario::checkBackTrig()
{
	if (mInput & 0x8000) {
		TMarioGamePad* pad = mGamePad;
		if (pad->mEnabledFrameMeaning & 0x2000)
			return changePlayerStatus(0x008008A9, 0, false);
		if (!onYoshi()) {
			f32 jumpCatchSpeed = mJumpParams.mJumpJumpCatchSp.value;
			setPlayerVelocity(jumpCatchSpeed);
			return changePlayerStatus(0x0080088A, 0, false);
		}
	}
	return FALSE;
}
#pragma dont_inline off

BOOL TMario::landing()
{
	if (mVel.y < 0.0f) {
		u16 t = mActionTimer; mActionTimer = t + 1;
		if (t > 240) { mActionTimer = 240; startSoundActor(0x786B); mActionArg = 3; }
	}
	if (checkBackTrig())
		return TRUE;
	if (rocketCheck()) return TRUE;
	s32 anm;
	switch (mActionArg) {
	case 0: anm = 86; break;
	case 1: anm = 144; break;
	case 3: anm = 288; break;
	}
	jumpingBasic(0x04000471, anm, 3);
	return FALSE;
}

BOOL TMario::jumpCatch()
{
	if (mInput & 0x8000) {
		TMarioGamePad* pad = mGamePad;
		if (pad->mEnabledFrameMeaning & 0x2000) { return changePlayerStatus(0x008008A9, 0, false); }
	}
	setAnimation(136, 1.0f);
	doJumping();
	int r = jumpProcess(0);
	switch (r) {
	case 1: {
		u8 cc = 1;
		u8 hy = (mSubState & 0x100) ? cc : 0;
		if (hy == true) cc = 0;
		if (mLastGroundPos.y - mPosition.y <= mDeParams.mDamageFallHeight.value) cc = 0;
		if (onYoshi()) cc = 0;
		u16 bg = *(u16*)((u8*)mGroundPlane);
		u8 badGround;
		if (bg != 0x0A && bg != 0x800A && bg != 0x0108) {
			u8 t; if (bg == 0x07 || bg == 0x8007) t = 1; else t = 0;
			if (!t) {
				u8 r;
				if (bg == 0x0108 || bg == 0x08 || bg == 0x8008)
					r = 1;
				else
					r = 0;
				if (!r) {
					u8 w; if (bg == 0x09 || bg == 0x8009) w = 1; else w = 0;
					if (w) badGround = 1; else badGround = 0;
				} else
					badGround = 1;
			} else
				badGround = 1;
		} else
			badGround = 1;
		if (badGround)
			cc = 0;
		if (mVel.y > 0.0f) cc = 0;
		if (cc) {
			u8 cf; if (mState & 0x40000) cf = 1; else cf = 0;
			if (cf) { sinkInSandEffect(); changePlayerStatus(0x0002033C, 1, false); break; }
		}
		changePlayerStatus(0x00800456, 0, false);
		break;
	}
	case 2: {
		if (mWallPlane) {
			// Pointer math slop
			u8 fc; if (*(u16*)((u8*)mWallPlane) == 0x010A) fc = 1; else fc = 0;
			if (fc) { changePlayerDropping(0x3000036C, 0); break; }
		}
		playerRefrection(1);
		if (mVel.y > 0.0f) mVel.y = 0.0f;
		emitParticle(12);
		changePlayerDropping(0x000208B0, 0);
		break;
	}
	}
	return FALSE;
}

#pragma dont_inline on
int TMario::jumpDownCommon(int statusId, int anmId, f32 velY)
{
	setPlayerVelocity(velY);
	int r = jumpProcess(0);
	switch (r) {
	case 0: setAnimation(anmId, 1.0f); break;
	case 1: changePlayerStatus(statusId, mActionArg, false); break;
	case 2:
		setAnimation(2, 1.0f);
		playerRefrection(0);
		if (mVel.y > 0.0f) mVel.y = 0.0f;
		setPlayerVelocity(-velY);
		break;
	}
	return r;
}
#pragma dont_inline off

BOOL TMario::stayWall()
{
	u16 t = mActionTimer; mActionTimer = t + 1;
	if (mActionTimer > 60) mActionTimer = 60;
	if (mInput & 2) {
		if (mWallPlane) {
			s16 wa = matan(mWallPlane->getNormal().z, mWallPlane->getNormal().x);
			s16 na = wa + 0x8000;
			emitParticle(24, na); emitParticle(25, na);
		}
		mVel.y = 52.0f;
		mFaceAngle.y = mFaceAngle.y + 0x8000;
		if (160.0f + mPosition.y + mVel.y >= mFloorPosition.x)
			mVel.y = 1.0f;
		return changePlayerStatus(0x02000886, 0, false);
	}
	int handled;
	if (mInput & 0x8000) {
		TMarioGamePad* pad = mGamePad;
		if (pad->mEnabledFrameMeaning & 0x2000) {
			handled = changePlayerStatus(0x008008A9, 0, false);
			goto checkHandled;
		} else {
			if (!onYoshi()) {
				f32 jumpCatchSpeed = mJumpParams.mJumpJumpCatchSp.value;
				setPlayerVelocity(jumpCatchSpeed);
				handled = changePlayerStatus(0x0080088A, 0, false);
				goto checkHandled;
			}
		}
	}
	handled = FALSE;
checkHandled:
	if (handled)
		return TRUE;
	if (mActionTimer < 20) {
		mActionTimer = mActionTimer + 1;
		mVel.x = 0.0f; mVel.y = 0.0f; mVel.z = 0.0f;
	} else {
		mVel.y = (f32)-(s32)mActionTimer * 0.5f;
	}
	if (mWallPlane) {
		mPosition.x -= mWallPlane->mNormal.x;
		mPosition.z -= mWallPlane->mNormal.z;
	}
	int jr = jumpProcess(0);
	switch (jr) {
	case 1:
		mFaceAngle.y += 0x8000;
		return changePlayerStatus(0x088C, 0, false);
	}
	if (!mWallPlane) {
		mFaceAngle.y += 0x8000;
		setPlayerVelocity(0.0f);
		mVel.y = 0.0f;
		return changePlayerStatus(0x088C, 0, false);
	}
	setAnimation(204, 1.0f);
	if (mVel.y < -10.0f) {
		wallSlipEffect();
		if (gpMSound->gateCheck(0x113F))
			MSoundSESystem::MSoundSE::startSoundActor(0x113F, (const Vec*)&mPosition, 0, nullptr, 0, 4);
	}
	return FALSE;
}

BOOL TMario::catchStop()
{
	if (mActionState == 0) { mVel.y = 30.0f; mActionState = 1; }
	doJumping();
	int r = jumpProcess(0);
	switch (r) {
	case 0:
		if (mActionState == 1) setAnimation(111, 1.0f);
		else setAnimation(86, 1.0f);
		break;
	case 1: changePlayerStatus(0x0C000232, 0, false); break;
	case 2: setPlayerVelocity(0.0f); break;
	}
	if (mActionState == 1 && isLast1AnimeFrame()) mActionState = 2;
	return FALSE;
}

BOOL TMario::slipFalling()
{
	u16 t = mActionTimer; mActionTimer = t + 1;
	if (mActionTimer > 120 && mPosition.y - mFloorPosition.y > 0.0f) {
		return changePlayerStatus(0x088C, 1, false);
	}
	mForwardVel *= mJumpParams.mJumpSpeedBrake.value;
	if (mInput & 1) {
		s16 ad = mIntendedYaw - mFaceAngle.y;
		u16 au = (u16)ad;
		f32 control = 0.03125f * mIntendedMag;
		mForwardVel += control * JMASCos(au) * getJumpAccelControl();
		mFaceAngle.y = (s16)(control * JMASSin(au) * getJumpSlideControl()
		                       + (f32)mFaceAngle.y);
	}
	if (mForwardVel > 32.0f) mForwardVel -= 0.2f;
	if (mForwardVel < -16.0f) mForwardVel += 0.4f;
	f32 vx = mForwardVel * JMASSin(mFaceAngle.y); mSlideVelX = vx; mVel.x = vx;
	f32 vz = mForwardVel * JMASCos(mFaceAngle.y); mSlideVelZ = vz; mVel.z = vz;
	int jr = jumpProcess(0);
	switch (jr) {
	case 1:
		if (mActionState == 0 && mVel.y < 0.0f
		    && mGroundPlane->getNormal().y >= 0.9848077f) {
			mVel.y = -mVel.y * 0.5f; mActionState = 1;
		} else changePlayerStatus(0x00840452, 0, false);
		break;
	case 2:
		if (mVel.y > 0.0f) mVel.y = 0.0f;
		rumbleStart(21, mMotorParams.mMotorWall.value);
		changePlayerStatus(0x000208B0, 0, false);
		break;
	}
	setAnimation(145, 1.0f);
	return FALSE;
}

BOOL TMario::fireDowning()
{
	if (mActionTimer == 1) startVoice(0x7849);
	u16 t2 = mActionTimer; mActionTimer = t2 + 1;
	if (!(mInput & 1))
		mForwardVel = FConverge(mForwardVel, 0.0f, 0.35f, 0.35f);
	if (mInput & 1) {
		s16 ad = mIntendedYaw - mFaceAngle.y; u16 au = (u16)ad;
		f32 ac = 0.03125f * mIntendedMag * mJumpParams.mFireDownControl.get();
		mForwardVel += ac * JMASCos(au);
		mFaceAngle.y = (s16)(1024.0f * (ac * JMASSin(au)) + (f32)mFaceAngle.y);
		if (mForwardVel < 0.0f) {
			mFaceAngle.y += 0x8000;
			mForwardVel *= -1.0f;
		}
		if (mForwardVel > 32.0f)
			mForwardVel -= 2.0f;
	}
	f32 vx = mForwardVel * JMASSin(mFaceAngle.y); mSlideVelX = vx; mVel.x = vx;
	f32 vz = mForwardVel * JMASCos(mFaceAngle.y); mSlideVelZ = vz; mVel.z = vz;
	int jr = jumpProcess(0);
	switch (jr) {
	case 1:
		if (mActionState < 2 && mVel.y < 0.0f) {
			mVel.y = -mVel.y * 0.4f;
			setPlayerVelocity(0.5f * mForwardVel);
			mActionState = mActionState + 1;
		} else { startVoice(0x7852); changePlayerStatus(0x08000239, 0, false); }
		break;
	case 2:
		playerRefrection(0);
		break;
	}
	setAnimation(41, 1.0f);
	return FALSE;
}

BOOL TMario::thrownDowning()
{
	s16 ad = mIntendedYaw - mFaceAngle.y; u16 au = (u16)ad;
	f32 ac = 160.0f * mIntendedMag;
	f32 ta = mJumpParams.mThrownAccel.value;
	mForwardVel += ac * JMASCos(au) * ta;
	f32 ts = mJumpParams.mThrownSlide.value;
	mFaceAngle.y = (s16)(ac * JMASSin(au) * ts + (f32)mFaceAngle.y);
	mForwardVel *= mJumpParams.mThrownBrake.value;
	f32 vx = mForwardVel * JMASSin(mFaceAngle.y); mSlideVelX = vx; mVel.x = vx;
	f32 vz = mForwardVel * JMASCos(mFaceAngle.y); mSlideVelZ = vz; mVel.z = vz;
	int jr = jumpProcess(0);
	switch (jr) {
	case 1:
		if (mActionState < 2 && mVel.y < 0.0f) {
			mVel.y = -mVel.y * 2.0f;
			setPlayerVelocity(1024.0f * mForwardVel);
			mActionState = mActionState + 1;
		} else
			return changePlayerStatus(0x0C000223, 0, false);
		break;
	case 2:
		playerRefrection(0);
		break;
	}
	setAnimation(288, 1.0f);
	return FALSE;
}

BOOL TMario::boardJumping()
{
	setAnimation(109, 1.0f);
	if (mVel.y < 0.0f) {
		// Pointer math slop
		*(f32*)((u8*)this + 0x50) = mDeParams.mTrampleRadius.get(); calcEntryRadius();
		*(f32*)((u8*)this + 0x54) = mDeParams.mAttackHeight.get(); calcEntryRadius();
	} else {
		// Pointer math slop
		*(f32*)((u8*)this + 0x50) = mDeParams.mPushupRadius.get(); calcEntryRadius();
		*(f32*)((u8*)this + 0x54) = mDeParams.mPushupHeight.get(); calcEntryRadius();
	}
	switch (jumpProcess(0)) {
	case 1:
		if (mVel.y < 0.0f)
			changePlayerStatus(0x00810446, 0, false);
		break;
	case 2:
		if (!mWallPlane) {
			setPlayerVelocity(0.0f);
			loserExec();
		} else {
			s16 d = matan(mWallPlane->getNormal().z, mWallPlane->getNormal().x)
			        - mFaceAngle.y;
			s32 mx = mSurfingParamsWaterRed.mClashAngle.get();
			if ((d < -mx || mx < d)
			    && mForwardVel > mSurfingParamsWaterRed.mClashSpeed.get()) {
				loserExec();
			} else {
				setPlayerVelocity(0.0f);
			}
		}
		break;
	}
	return FALSE;
}

BOOL TMario::rocketCheck()
{
	u8 canRocket = TRUE;
	if (mAction == ACTION_ROCKETING) canRocket = FALSE;
	if (mAction == ACTION_ROCKET_END) canRocket = FALSE;
	u8 hasFludd; if (mState & MARIO_FLAG_HAS_FLUDD) hasFludd = TRUE; else hasFludd = FALSE;
	if (hasFludd) {
		// Pointer math slop
		if (*(u8*)((u8*)mWaterGun->getCurrentNozzle() + 0x18) != 1) canRocket = FALSE;
		u8 isPumpIdle; if (mPumpState == 0) isPumpIdle = TRUE; else isPumpIdle = FALSE;
		if (!isPumpIdle) canRocket = FALSE;
		TWaterGun* g = mWaterGun;
		u8 nozzleReady;
		if (g->mCurrentWater == 0)
			nozzleReady = FALSE;
		else {
			s32 k = g->getCurrentNozzle()->getNozzleKind();
			if (k == 1) {
				TNozzleTrigger* t = (TNozzleTrigger*)g->getCurrentNozzle();
				if (t->unk385 == TNozzleTrigger::ACTIVE) nozzleReady = TRUE;
				else nozzleReady = FALSE;
			} else {
				if (g->getCurrentNozzle()->unk378 > 0.0f) nozzleReady = TRUE;
				else nozzleReady = FALSE;
			}
		}
		if (!nozzleReady) canRocket = FALSE;
	} else canRocket = FALSE;
	if ((u8)canRocket == TRUE) {
		mRocketTargetY
		    = mPosition.y + mWaterGun->mWatergunParams.mHHoverHeight.get();
		return changePlayerStatus(ACTION_ROCKETING, 0, false);
	}
	return FALSE;
}

BOOL TMario::rocketing()
{
	u8 hasFludd;
	if (mState & MARIO_FLAG_HAS_FLUDD)
		hasFludd = TRUE;
	else
		hasFludd = FALSE;
	if (!hasFludd) {
		return changePlayerStatus(ACTION_ROCKET_END, 0, false);
	}

	if (*(u8*)((u8*)mWaterGun->getCurrentNozzle() + 0x18) != 1) {
		return changePlayerStatus(ACTION_ROCKET_END, 0, false);
	}

	u8 isPumpIdle;
	if (mPumpState == 0)
		isPumpIdle = TRUE;
	else
		isPumpIdle = FALSE;
	if (!isPumpIdle) {
		return changePlayerStatus(ACTION_ROCKET_END, 0, false);
	}

	TWaterGun* gun = mWaterGun;
	u8 canRocket;
	if (gun->mCurrentWater == 0) {
		canRocket = FALSE;
	} else {
		s32 nozzleKind = gun->getCurrentNozzle()->getNozzleKind();
		if (nozzleKind == 1) {
			TNozzleTrigger* nozzle = (TNozzleTrigger*)gun->getCurrentNozzle();
			if (nozzle->unk385 == TNozzleTrigger::ACTIVE)
				canRocket = TRUE;
			else
				canRocket = FALSE;
		} else {
			if (gun->getCurrentNozzle()->unk378 > 0.0f)
				canRocket = TRUE;
			else
				canRocket = FALSE;
		}
	}
	if (!canRocket) {
		return changePlayerStatus(ACTION_ROCKET_END, 0, false);
	}

	if (mInput & 1) {
		if ((int)mWaterGun->mCurrentNozzle == TWaterGun::Hover) {
			s16 angleDiff = mIntendedYaw - mFaceAngle.y;
			f32 stickMag  = mIntendedMag;
			if ((angleDiff > -0x1555 && angleDiff < 0x1555)
			    || angleDiff < -0x6AAA || angleDiff > 0x6AAA) {
				s16 reaction;
				if (angleDiff >= -0x4000 && angleDiff <= 0x4000) {
					s16 gunAngle
					    = *(s16*)((u8*)mWaterGun->getCurrentNozzle() + 0x324);
					reaction = (s16)(0.03125f * -stickMag * (f32)gunAngle
					                 * JMASCos(angleDiff));
				} else {
					s16 gunAngle
					    = *(s16*)((u8*)mWaterGun->getCurrentNozzle() + 0x338);
					reaction = (s16)(0.03125f * -stickMag * (f32)gunAngle
					                 * JMASCos(angleDiff));
				}
				mWaterGun->unk1CC2 = reaction;
				mWaterGun->unk1CC4 = reaction;
				mForwardVel += stickMag * JMASCos(angleDiff)
				                * mDivingParams.mAccelControl.value;
			} else {
				s16 gunAngle
				    = *(s16*)((u8*)mWaterGun->getCurrentNozzle() + 0x310);
				s16 reaction = (s16)(0.03125f * -stickMag * (f32)gunAngle
				                     * JMASSin(angleDiff));
				u8 hasFludd2;
				if (mState & MARIO_FLAG_HAS_FLUDD)
					hasFludd2 = TRUE;
				else
					hasFludd2 = FALSE;
				if (hasFludd2) {
					mWaterGun->unk1CC2 = -reaction;
					mWaterGun->unk1CC4 = reaction;
					s16 convergeAngle = mIntendedYaw - mFaceAngle.y;
					mFaceAngle.y
					    = mIntendedYaw
					      - IConverge(convergeAngle, 0,
					                  mHoverParams.mRotSp.value,
					                  mHoverParams.mRotSp.value);
				}
			}
		}
	} else {
		mWaterGun->unk1CC2 = 0;
		mWaterGun->unk1CC4 = 0;
	}

	u16 fa = mFaceAngle.y;
	mSlideVelX = mForwardVel * JMASSin(fa); mSlideVelZ = mForwardVel * JMASCos(fa);
	mVel.x = mSlideVelX; mVel.z = mSlideVelZ;
	if ((int)mWaterGun->mCurrentNozzle == TWaterGun::Hover) {
		mVel.y = (mRocketTargetY - mPosition.y) * mHoverParams.mAccelRate.value;
		mForwardVel *= mHoverParams.mBrake.value;
	}
	switch (jumpProcess(2)) {
	case 4:
		rumbleStart(21, mMotorParams.mMotorWall.value);
		changePlayerStatus(ACTION_ROOF_CHECK, 0, false);
		break;
	}

	if (mRoofPlane) {
		if (160.0f + mPosition.y > mFloorPosition.x)
			mPosition.y = mFloorPosition.x - 160.0f;
	}
	setAnimation(86, 1.0f);
	return FALSE;
}

BOOL TMario::hipAttacking()
{
	s32 i = 0; f32 md = 70.0f;
	// Pointer math slop
	while (i < *(u16*)((u8*)this + 0x48)) {
		// Pointer math slop
		THitActor* a = ((THitActor**)*(u32*)((u8*)this + 0x44))[i];
		u32 at = *(u32*)((u8*)a + 0x4C);
		u8 it; if (at == 0x4000000B) it = 1; else it = 0;
		if (it) {
			// Pointer math slop
			f32 dx = *(f32*)((u8*)a + 0x10) - mPosition.x;
			f32 dy = *(f32*)((u8*)a + 0x14) - mPosition.y;
			f32 dz = *(f32*)((u8*)a + 0x18) - mPosition.z;
			f32 d = JGeometry::TUtil<f32>::sqrt(dx*dx + dy*dy + dz*dz);
			if (d > md) { mPosition.x = *(f32*)((u8*)a + 0x10); mPosition.z = *(f32*)((u8*)a + 0x18); }
		}
		i++;
	}
	switch (mActionState) {
	case 0: startVoice(0x788F); mActionState = 1;
	case 1: {
		if (mFloorPosition.y > mPosition.y) { mPosition.y = 1.0f + mFloorPosition.y; changePlayerStatus(0x0080023C, 0, false); break; }
		if (mActionTimer < 40) {
			f32 f = (f32)(40 - mActionTimer) * 0.5f;
			if (160.0f + mPosition.y + f < mFloorPosition.x) {
				mPosition.y += f * 0.25f;
				// Pointer math slop
				*(f32*)((u8*)this + 0x104) = mPosition.y;
			}
		}
		setPlayerVelocity(0.0f);
		// Pointer math slop
		*(f32*)((u8*)this + 0x50) = 0.0f; calcEntryRadius();
		setAnimation(60, 1.0f);
		u16 tt = mActionTimer; mActionTimer = tt + 1;
		if (mActionTimer >= 60) { mActionTimer = 0; mActionState = 2; }
		mVel.y = 0.0f;
		int r = jumpProcess(0);
		if (r == 1) { changePlayerStatus(0x0C000230, 0, false); break; }
		if (r == 2) { setPlayerVelocity(-16.0f); if (mVel.y > 0.0f) mVel.y = 0.0f; changePlayerStatus(0x000208B0, 0, false); break; }
		break;
	}
	case 2: case 3: {
		setAnimation(61, 1.0f);
		u16 tt = mActionTimer; mActionTimer = tt + 1;
		if ((s16)mActionTimer > mJumpParams.mSuperHipAttackCt.value) mActionState = 3;
		if (mActionState == 2) { mVel.y = mJumpParams.mHipAttackSpeedY.value; emitBlurHipDrop(); }
		else { mVel.y = mJumpParams.mSuperHipAttackSpeedY.value; emitBlurHipDropSuper(); }
		// Pointer math slop
		*(f32*)((u8*)this + 0x50) = mDeParams.mHipdropRadius.value; calcEntryRadius();
		*(f32*)((u8*)this + 0x54) = mDeParams.mAttackHeight.value; calcEntryRadius();
		int r = jumpProcess(0);
		if (r == 1) {
			if (isMario()) {
				if (mActionState == 2) { SMSRumbleMgr->start(0, (f32*)nullptr); gpCameraShake->startShake((EnumCamShakeMode)0, 1.0f); }
				else { rumbleStart(21, 30); gpCameraShake->startShake((EnumCamShakeMode)39, 1.0f); }
			}
			if (mGroundPlane->getActor()) {
				// Pointer math slop
				if (!onYoshi() && *(u32*)((u8*)mGroundPlane->getActor() + 0x4C) == 0x4000006A) {
					emitParticle(57, (const JGeometry::TVec3<f32>*)&mPosition);
					mPosition.y -= 5.0f;
					((TLiveActor*)mGroundPlane->getActor())->receiveMessage(this, 3);
					startVoice(0x78D3); changePlayerStatus(0x00200346, 0, false); break;
				}
				if (mActionState == 2) ((TLiveActor*)mGroundPlane->getActor())->receiveMessage(this, 1);
				else { ((TLiveActor*)mGroundPlane->getActor())->receiveMessage(this, 3); ((TLiveActor*)mGroundPlane->getActor())->receiveMessage(this, 1); }
			}
			if (mActionState == 2) { emitParticle(20); emitParticle(19); emitParticle(18); }
			else { emitParticle(67); emitParticle(68); emitParticle(69); emitParticle(70); }
			changePlayerStatus(0x0080023C, 0, false);
		} else if (r == 2) {
			setPlayerVelocity(-16.0f); if (mVel.y > 0.0f) mVel.y = 0.0f;
			changePlayerStatus(0x000208B0, 0, false);
			rumbleStart(21, mMotorParams.mMotorWall.value);
			if (gpMSound->gateCheck(0x180E)) MSoundSESystem::MSoundSE::startSoundActor(0x180E, (const Vec*)&mPosition, 0, nullptr, 0, 4);
		}
		break;
	}
	}
	mModelFaceAngle = mFaceAngle.y;
	return FALSE;
}

BOOL TMario::diving()
{
	if (mHolder != nullptr) {
		MtxPtr mtx = mHolder->getTakingMtx();
		mPosition.x = mtx[0][3];
		mPosition.y = mtx[1][3];
		mPosition.z = mtx[2][3];
		mFaceAngle.x = 0;
		mModelFaceAngle = mFaceAngle.y;
		mFaceAngle.z = 0;
		setAnimation(0x120, 1.0f);
		return FALSE;
	}

	if (mInput & 1) {
		s16 angleDiff = mIntendedYaw - mFaceAngle.y;
		f32 stickMag  = mIntendedMag;
		if ((angleDiff > -0x1555 && angleDiff < 0x1555)
		    || angleDiff < -0x6AAA || angleDiff > 0x6AAA) {
			s16 gunAngle
			    = *(s16*)((u8*)mWaterGun->getCurrentNozzle() + 0x310);
			s16 reaction = (s16)(0.03125f * -stickMag * (f32)gunAngle
			                     * JMASCos(angleDiff));
			if (checkFlag(MARIO_FLAG_HAS_FLUDD)) {
				mWaterGun->unk1CC2 = reaction;
				mWaterGun->unk1CC4 = reaction;
				mForwardVel += stickMag * JMASCos(angleDiff)
				                * mDivingParams.mAccelControl.value;
			}
		} else {
			s16 gunAngle
			    = *(s16*)((u8*)mWaterGun->getCurrentNozzle() + 0x310);
			s16 reaction = (s16)(0.03125f * -stickMag * (f32)gunAngle
			                     * JMASSin(angleDiff));
			if (checkFlag(MARIO_FLAG_HAS_FLUDD)) {
				mWaterGun->unk1CC2 = -reaction;
				mWaterGun->unk1CC4 = reaction;
				s16 convergeAngle = mIntendedYaw - mFaceAngle.y;
				mFaceAngle.y
				    = mIntendedYaw
				      - IConverge(convergeAngle, 0,
				                  mDivingParams.mRotSp.value,
				                  mDivingParams.mRotSp.value);
			}
		}
		setAnimation(0x137, 1.0f);
	} else {
		u8 isNearFloor;
		if (mPosition.y <= mFloorPosition.y + 4.0f)
			isNearFloor = TRUE;
		else
			isNearFloor = FALSE;
		if (isNearFloor) {
			setPlayerVelocity(0.0f);
			switch (mAnimationId) {
			case 0x138:
				if (isLast1AnimeFrame())
					setAnimation(0xC3, 1.0f);
				break;
			case 0xC3:
				break;
			default:
				setAnimation(0x138, 1.0f);
				break;
			}
		} else {
			setAnimation(0x137, 1.0f);
		}

		if (checkFlag(MARIO_FLAG_HAS_FLUDD)) {
			mWaterGun->unk1CC2 = 0;
			mWaterGun->unk1CC4 = 0;
		}
	}

	if (gpMSound->gateCheck(0x101F)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x101F, nullptr, 0,
		                                          nullptr, 0, 4);
	}

	bubbleFromBody();
	mForwardVel *= mDivingParams.mSeaBrake.value;
	mSlideVelX = mForwardVel * JMASSin(mFaceAngle.y);
	mSlideVelZ = mForwardVel * JMASCos(mFaceAngle.y);
	mVel.x     = mSlideVelX;
	mVel.y *= mDivingParams.mSeaBrakeY.value;
	mVel.z = mSlideVelZ;
	jumpProcess(0);
	return FALSE;
}

BOOL TMario::jumpMain()
{
	if (mHeldObject != nullptr) {
		u8 pressed;
		if (mInput & 0x2000)
			pressed = TRUE;
		else
			pressed = FALSE;
		if (pressed)
			changePlayerStatus(0x820008AB, 0, false);
	}

	BOOL result;
	JGeometry::TVec3<f32> pos;
	switch (mAction) {
	case 0x0884:
	case 0x089C:
	case 0x02000880: {
		BOOL caseResult;
		BOOL handled;
		if (checkBackTrig())
			handled = TRUE;
		else if (rocketCheck())
			handled = TRUE;
		else if (considerJumpRotate())
			handled = TRUE;
		else
			handled = FALSE;

		if (handled) {
			caseResult = TRUE;
		} else {
			switch (mAction) {
			case 0x089C:
				jumpingBasic(0x000208B8, 0x120, 0);
				break;
			case 0x0884: {
				int anm;
				if (mVel.y >= 0.0f)
					anm = 0x50;
				else
					anm = 0x4C;
				jumpingBasic(0x04000470, anm, 3);
				break;
			}
			default:
				jumpingBasic(0x04000470, 0x4D, 3);
				break;
			}
			caseResult = FALSE;
		}
		result = caseResult;
		break;
	}
	case 0x02000881: {
		BOOL caseResult;
		if (mActionTimer == 0) {
			mActionTimer++;
			TMario* mario = this;
			mario->rumbleStart(20, mMotorParams.mMotorWall.get());
		}

		int anm;
		if (mVel.y >= 0.0f)
			anm = 0x50;
		else
			anm = 0x4C;

		BOOL handled;
		if (checkBackTrig())
			handled = TRUE;
		else if (rocketCheck())
			handled = TRUE;
		else if (considerJumpRotate())
			handled = TRUE;
		else
			handled = FALSE;

		if (handled)
			caseResult = TRUE;
		else {
			jumpingBasic(0x04000472, anm, 3);
			caseResult = FALSE;
		}
		result = caseResult;
		break;
	}
	case 0x088C:
	case 0x088D:
		result = landing();
		break;
	case 0x0882: {
		BOOL caseResult;
		if (mActionTimer == 0) {
			mActionTimer++;
			TMario* mario = this;
			mario->rumbleStart(21, mMotorParams.mMotorWall.get());
		}

		BOOL handled;
		if (mInput & 0x8000) {
			TMarioGamePad* pad = mGamePad;
			if (pad->mEnabledFrameMeaning & 0x2000) {
				handled = changePlayerStatus(0x008008A9, 0, false);
			} else if (!onYoshi()) {
				f32 jumpCatchSpeed = mJumpParams.mJumpJumpCatchSp.get();
				setPlayerVelocity(jumpCatchSpeed);
				handled = changePlayerStatus(0x0080088A, 0, false);
			} else {
				goto checkBackTrigFalse882;
			}
		} else {
checkBackTrigFalse882:
			handled = FALSE;
		}

		if (handled)
			caseResult = TRUE;
		else if (rocketCheck())
			caseResult = TRUE;
		else {
			jumpingBasic(0x04000478, 0x6F, 0);
			caseResult = FALSE;
		}
		result = caseResult;
		break;
	}
	case 0x0883: {
		BOOL caseResult;
		BOOL handled;
		if (mInput & 0x8000) {
			TMarioGamePad* pad = mGamePad;
			if (pad->mEnabledFrameMeaning & 0x2000) {
				handled = changePlayerStatus(0x008008A9, 0, false);
			} else if (!onYoshi()) {
				f32 jumpCatchSpeed = mJumpParams.mJumpJumpCatchSp.get();
				setPlayerVelocity(jumpCatchSpeed);
				handled = changePlayerStatus(0x0080088A, 0, false);
			} else {
				goto checkBackTrigFalse883;
			}
		} else {
checkBackTrigFalse883:
			handled = FALSE;
		}

		if (handled) {
			caseResult = TRUE;
		} else if (rocketCheck()) {
			caseResult = TRUE;
		} else {
			if (mActionState == 0 && isLast1AnimeFrame())
				mActionState = 1;

			if (mActionState == 0)
				jumpingBasic(0x04000472, 0xF7, 0);
			else
				jumpingBasic(0x04000472, 0x56, 0);
			caseResult = FALSE;
		}
		result = caseResult;
		break;
	}
	case 0x0887: {
		BOOL caseResult;
		BOOL handled;
		if (checkBackTrig())
			handled = TRUE;
		else if (rocketCheck())
			handled = TRUE;
		else if (considerJumpRotate())
			handled = TRUE;
		else
			handled = FALSE;

		if (handled) {
			caseResult = TRUE;
		} else {
			BOOL backHandled;
			if (mInput & 0x8000) {
				TMarioGamePad* pad = mGamePad;
				if (pad->mEnabledFrameMeaning & 0x2000) {
					backHandled = changePlayerStatus(0x008008A9, 0, false);
				} else if (!onYoshi()) {
					f32 jumpCatchSpeed = mJumpParams.mJumpJumpCatchSp.get();
					setPlayerVelocity(jumpCatchSpeed);
					backHandled = changePlayerStatus(0x0080088A, 0, false);
				} else {
					goto checkBackTrigFalse887;
				}
			} else {
checkBackTrigFalse887:
				backHandled = FALSE;
			}

			if (backHandled) {
				caseResult = TRUE;
			} else {
				int jumpResult = jumpingBasic(0x04000473, 0xBF, 1);
				if (jumpResult != 3) {
					mFaceAngle.x = 0;
					mModelFaceAngle += 0x8000;
				}
				caseResult = FALSE;
			}
		}
		result = caseResult;
		break;
	}
	case 0x02000886: {
		BOOL caseResult;
		BOOL handled;
		if (mInput & 0x8000) {
			TMarioGamePad* pad = mGamePad;
			if (pad->mEnabledFrameMeaning & 0x2000) {
				handled = changePlayerStatus(0x008008A9, 0, false);
			} else if (!onYoshi()) {
				f32 jumpCatchSpeed = mJumpParams.mJumpJumpCatchSp.get();
				setPlayerVelocity(jumpCatchSpeed);
				handled = changePlayerStatus(0x0080088A, 0, false);
			} else {
				goto checkBackTrigFalse20886;
			}
		} else {
checkBackTrigFalse20886:
			handled = FALSE;
		}

		if (handled) {
			caseResult = TRUE;
		} else if (considerJumpRotate()) {
			caseResult = TRUE;
		} else if (rocketCheck()) {
			caseResult = TRUE;
		} else {
			jumpingBasic(0x04000470, 0xCB, 3);
			caseResult = FALSE;
		}
		result = caseResult;
		break;
	}
	case 0x02000885: {
		BOOL caseResult;
		if (mInput & 0x8000) {
			caseResult = changePlayerStatus(0x0080088A, 0, false);
		} else {
			setPlayerVelocity(0.98f * mForwardVel);
			switch (jumpProcess(0)) {
			case 1: {
				u32 status;
				if (mForwardVel < 0.0f)
					status = 0x50;
				else
					status = 0x04000470;
				changePlayerStatus(status, 0, false);
				break;
			}
			case 2:
				setPlayerVelocity(0.0f);
				break;
			}
			setAnimation(0x4D, 1.0f);
			caseResult = FALSE;
		}
		result = caseResult;
		break;
	}
	case 0x000208B4:
		if (mActionTimer == 1)
			startVoice(0x7849);
		mActionTimer++;
		setPlayerVelocity(mForwardVel);
		if (jumpProcess(0) == 1)
			changePlayerStatus(0x00020449, 0, false);
		int anim;
		if (mActionArg == 0)
			anim = 0x4D;
		else
			anim = 0x29;
		setAnimation(anim, 1.0f);
		result = FALSE;
		break;
	case 0x000208B5:
		setPlayerVelocity(mForwardVel);
		if (jumpProcess(0) == 1)
			changePlayerStatus(0x00020449, 0, false);
		setAnimation(0x56, 1.0f);
		result = FALSE;
		break;
	case 0x0888:
		jumpingBasic(0x04000440, 0xF6, 1);
		mState |= 0x4000;
		result = FALSE;
		break;
	case 0x02000889:
		jumpingBasic(0x04000440, 0x10F, 1);
		result = FALSE;
		break;
	case 0x0081089B:
	case 0x0281089A:
		result = boardJumping();
		break;
	case 0x088B:
		result = rocketing();
		break;
	case 0x0895:
	case 0x0896: {
		BOOL caseResult;
		BOOL handled;
		if (mInput & 0x8000) {
			TMarioGamePad* pad = mGamePad;
			if (pad->mEnabledFrameMeaning & 0x2000) {
				handled = changePlayerStatus(0x008008A9, 0, false);
			} else if (!onYoshi()) {
				f32 jumpCatchSpeed = mJumpParams.mJumpJumpCatchSp.get();
				setPlayerVelocity(jumpCatchSpeed);
				handled = changePlayerStatus(0x0080088A, 0, false);
			} else {
				goto checkBackTrigFalse895;
			}
		} else {
checkBackTrigFalse895:
			handled = FALSE;
		}

		if (handled) {
			caseResult = TRUE;
		} else if (rocketCheck()) {
			caseResult = TRUE;
		} else {
			setAnimation(0xF4, 1.0f);
			emitBlurSpinJump();
			jumpingBasic(0x04000472, mAnimationId, 0);
			mActionTimer++;
			if (mAction == 0x0896)
				mModelFaceAngle = (s16)(mActionTimer << 12);
			else
				mModelFaceAngle = (u16)-(mActionTimer << 12);
			if ((gpMarDirector->unk58 & 0x3F) == 0)
				rumbleStart(20, mMotorParams.mMotorWall.get() / 2);
			caseResult = FALSE;
		}
		result = caseResult;
		break;
	}
	case 0x0892: {
		BOOL caseResult;
		if (mPrevAction == 0x10000358) {
			if (mActionArg == 0)
				setAnimation(0xF8, 1.0f);
			else
				setAnimation(0x6F, 1.0f);
		} else {
			setAnimation(0x4D, 1.0f);
		}

		BOOL handled;
		if (checkBackTrig())
			handled = TRUE;
		else if (rocketCheck())
			handled = TRUE;
		else if (considerJumpRotate())
			handled = TRUE;
		else
			handled = FALSE;

		if (handled)
			caseResult = TRUE;
		else {
			jumpingBasic(0x04000472, mAnimationId, 3);
			caseResult = FALSE;
		}
		result = caseResult;
		break;
	}
	case 0x0893: {
		BOOL caseResult;
		if (mPrevAction == 0x10000358) {
			if (mActionArg == 0)
				setAnimation(0xF8, 1.0f);
			else
				setAnimation(0x6F, 1.0f);
		} else {
			setAnimation(0x4D, 1.0f);
		}

		BOOL handled;
		if (checkBackTrig())
			handled = TRUE;
		else if (rocketCheck())
			handled = TRUE;
		else if (considerJumpRotate())
			handled = TRUE;
		else
			handled = FALSE;

		if (handled)
			caseResult = TRUE;
		else {
			jumpingBasic(0x04000472, mAnimationId, 3);
			caseResult = FALSE;
		}
		result = caseResult;
		break;
	}
	case 0x0894: {
		if (mAnimationId == 0xF0 && isLast1AnimeFrame())
			setAnimation(0xF1, 1.0f);

		mForwardVel  = 0.0f;
		mIntendedMag = 0.0f;
		jumpingBasic(0x560, mAnimationId, 3);
		if (mAction == 0x560)
			setAnimation(0xF2, 1.0f);

		pos = mPosition;
		if (mHeldObject->moveRequest(pos) == 1)
			mPosition = pos;
		result = FALSE;
		break;
	}
	case 0x0080088A:
		result = jumpCatch();
		break;
	case 0x820008AB:
		setAnimation(0x65, 1.0f);
		checkThrowObject();
		doJumping();
		switch (jumpProcess(0)) {
		case 1:
			mAction = 0x80000A36;
			break;
		case 2:
			setPlayerVelocity(0.0f);
			break;
		}
		result = FALSE;
		break;
	case 0x000208B0:
		if (mActionTimer == 0) {
			mActionTimer++;
			rumbleStart(21, 20);
		}
		jumpDownCommon(0x00020462, 2, -16.0f);
		result = FALSE;
		break;
	case 0x000208B1:
		if (mActionTimer == 0) {
			mActionTimer++;
			rumbleStart(21, 20);
		}
		jumpDownCommon(0x00020463, 0x2D, 16.0f);
		result = FALSE;
		break;
	case 0x000208B2:
		if (mActionTimer == 0) {
			mActionTimer++;
			rumbleStart(21, 20);
		}
		jumpDownCommon(0x00020461, 0x2D, 16.0f);
		result = FALSE;
		break;
	case 0x000208B3:
		if (mActionTimer == 0) {
			mActionTimer++;
			rumbleStart(21, 20);
		}
		jumpDownCommon(0x00020460, 2, -16.0f);
		result = FALSE;
		break;
	case 0x000208B6:
		jumpDownCommon(0x04000471, 0x56, mForwardVel);
		result = FALSE;
		break;
	case 0x000208BA:
		jumpDownCommon(0x04000471, 0x56, mForwardVel);
		result = FALSE;
		break;
	case 0x08A7:
		result = stayWall();
		break;
	case 0x08A6:
		result = catchStop();
		break;
	case 0x0200088E:
		result = slipFalling();
		break;
	case 0x000208B7:
		result = fireDowning();
		break;
	case 0x000208B8:
		result = thrownDowning();
		break;
	case 0x02000890: {
		BOOL caseResult;
		BOOL handled;
		if (checkBackTrig())
			handled = TRUE;
		else if (rocketCheck())
			handled = TRUE;
		else if (considerJumpRotate())
			handled = TRUE;
		else
			handled = FALSE;

		if (handled)
			caseResult = TRUE;
		else {
			jumpingBasic(0x04000472, mAnimationId, 3);
			caseResult = FALSE;
		}
		result = caseResult;
		break;
	}
	case 0x008008A9:
		result = hipAttacking();
		break;
	case 0x0891:
		result = diving();
		break;
	case 0x000208B9:
		jumpProcess(0);
		result = FALSE;
		break;
	}
	return result;
}
